// uart_pi.c
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/usb_serial_jtag.h"
#include "esp_log.h"
#include "esp_err.h"

#include "lwip/sockets.h"
#include "lwip/netdb.h"

#include "uart_pi.h"
#include "bno055.h"

static const char *TAG = "PI_LINK";

/* =========================
 * USB serial link (Pi <-> ESP32)
 * Pi side sees /dev/ttyACM0
 * ========================= */
#define CMD_TIMEOUT_MS 700
#define ODOM_PERIOD_MS 1000

/* =========================
 * Robot geometry
 * IMPORTANT: calibrate these
 * ========================= */
#define COUNTS_PER_REV 1560.0f
#define WHEEL_RADIUS_M 0.0325f
#define TRACK_WIDTH_M 0.1800f

static TickType_t last_cmd_tick = 0;
static TickType_t last_odom_tick = 0;

/* Odom state */
static float odom_x = 0.0f;
static float odom_y = 0.0f;
static float odom_theta = 0.0f;
static float odom_v = 0.0f;
static float odom_w = 0.0f;

static float normalize_angle(float a)
{
    while (a > (float)M_PI)
        a -= 2.0f * (float)M_PI;
    while (a < -(float)M_PI)
        a += 2.0f * (float)M_PI;
    return a;
}

static void usb_send_line(const char *line)
{
    if (line == NULL)
        return;

    usb_serial_jtag_write_bytes((const uint8_t *)line, strlen(line), 10);
    usb_serial_jtag_write_bytes((const uint8_t *)"\n", 1, 10);
}

static void send_ack(const char *msg)
{
    char buf[128];
    snprintf(buf, sizeof(buf), "ACK,%s", msg);
    usb_send_line(buf);
}

static void send_odom(void)
{
    char buf[256];
    /* Format: ODOM,x,y,theta_enc,v,w,theta_bno,roll_bno,pitch_bno,speed_L,speed_R
     * theta_bno: Yaw from BNO055 (ground truth orientation)
     * theta_enc: Theta from encoder delta (for comparison)
     */
    snprintf(buf, sizeof(buf),
             "ODOM,%.4f,%.4f,%.4f,%.4f,%.4f,%.2f,%.2f,%.2f,%d,%d",
             odom_x, odom_y, odom_theta, odom_v, odom_w,
             g_bno_heading_deg, g_bno_roll_deg, g_bno_pitch_deg,
             g_speed_left_pps, g_speed_right_pps);
    usb_send_line(buf);
}

void pi_link_init(void)
{
    last_cmd_tick = xTaskGetTickCount();
    last_odom_tick = xTaskGetTickCount();
    ESP_LOGI(TAG, "Pi USB link init done");
}

void pi_link_reset_odom(void)
{
    odom_x = 0.0f;
    odom_y = 0.0f;
    odom_theta = 0.0f;
    odom_v = 0.0f;
    odom_w = 0.0f;
    ESP_LOGI(TAG, "Odometry reset to (0, 0, 0)");
}

static void parse_command(char *line)
{
    if (line == NULL || strlen(line) == 0)
        return;

    ESP_LOGI(TAG, "RX: %s", line);

    if (strcmp(line, "PING") == 0)
    {
        send_ack("PONG");
        last_cmd_tick = xTaskGetTickCount();
        return;
    }

    if (strcmp(line, "STOP") == 0)
    {
        robot_stop();
        send_ack("STOP");
        last_cmd_tick = xTaskGetTickCount();
        return;
    }

    if (strcmp(line, "STATUS") == 0)
    {
        send_odom();
        last_cmd_tick = xTaskGetTickCount();
        return;
    }

    /* DIR,F,2500 */
    if (strncmp(line, "DIR,", 4) == 0)
    {
        char dir = 0;
        int speed = 0;

        if (sscanf(line, "DIR,%c,%d", &dir, &speed) == 2)
        {
            robot_set_direction(dir);
            robot_set_target_speed(speed);
            send_ack("DIR");
            last_cmd_tick = xTaskGetTickCount();
            return;
        }
    }

    send_ack("ERR");
}

void send_echo(const char *msg)
{
    char buf[160];
    snprintf(buf, sizeof(buf), "ECHO,%s", msg);
    usb_send_line(buf);
}

void pi_uart_task(void *arg)
{
    uint8_t ch;
    char line[128];
    int idx = 0;

    while (1)
    {
        int len = usb_serial_jtag_read_bytes(&ch, 1, portMAX_DELAY);

        if (len > 0)
        {
            if (ch == '\n' || ch == '\r')
            {
                if (idx > 0)
                {
                    line[idx] = '\0';

                    send_echo(line);     // loop back to Pi
                    parse_command(line); // keep normal command handling

                    idx = 0;
                }
            }
            else
            {
                if (idx < (int)(sizeof(line) - 1))
                {
                    line[idx++] = (char)ch;
                }
                else
                {
                    idx = 0;
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

void pi_link_update_odom_from_counts(int delta_left_count, int delta_right_count, float dt)
{
    float meters_per_count = (2.0f * (float)M_PI * WHEEL_RADIUS_M) / COUNTS_PER_REV;

    /* Adjust signs if your encoder direction is reversed */
    float dl = -(float)delta_left_count * meters_per_count;
    float dr = (float)delta_right_count * meters_per_count;

    float ds = 0.5f * (dl + dr);
    float dtheta = (dr - dl) / TRACK_WIDTH_M;

    float theta_mid = odom_theta + 0.5f * dtheta;
    odom_x += ds * cosf(theta_mid);
    odom_y += ds * sinf(theta_mid);
    odom_theta = normalize_angle(odom_theta + dtheta);

    if (dt > 0.0f)
    {
        odom_v = ds / dt;
        odom_w = dtheta / dt;
    }
    else
    {
        odom_v = 0.0f;
        odom_w = 0.0f;
    }
}

void pi_link_periodic(void)
{
    TickType_t now = xTaskGetTickCount();

    /* Safety timeout: no command from Pi -> stop */
    if ((now - last_cmd_tick) * portTICK_PERIOD_MS > CMD_TIMEOUT_MS)
    {
        robot_stop();
    }

    if ((now - last_odom_tick) * portTICK_PERIOD_MS >= ODOM_PERIOD_MS)
    {
        send_odom();
        last_odom_tick = now;
    }
}

/* =========================
 * Telnet Server (WiFi remote access)
 * ========================= */
#define TELNET_PORT 23
#define TELNET_BACKLOG 1
#define TELNET_BUFFER_SIZE 256

static void telnet_send_line(int sock, const char *line)
{
    if (line == NULL || sock < 0)
        return;

    char buf[TELNET_BUFFER_SIZE];
    int len = snprintf(buf, sizeof(buf), "%s\r\n", line);
    if (len > 0)
    {
        send(sock, buf, len, 0);
    }
}

static void telnet_handle_client(int client_sock)
{
    char buffer[TELNET_BUFFER_SIZE];
    char line[128];
    int line_idx = 0;
    int bytes_received;

    ESP_LOGI(TAG, "Telnet client connected");

    telnet_send_line(client_sock, "Welcome to ESP32-S3 Robot Telnet Server");
    telnet_send_line(client_sock, "Commands: PING, STOP, STATUS, DIR,<F|B|L|R>,<speed>");

    while (1)
    {
        bytes_received = recv(client_sock, buffer, sizeof(buffer) - 1, 0);

        if (bytes_received <= 0)
        {
            ESP_LOGI(TAG, "Telnet client disconnected");
            break;
        }

        /* Process received data byte by byte */
        for (int i = 0; i < bytes_received; i++)
        {
            char ch = buffer[i];

            if (ch == '\n' || ch == '\r')
            {
                if (line_idx > 0)
                {
                    line[line_idx] = '\0';

                    ESP_LOGI(TAG, "Telnet RX: %s", line);

                    /* Parse command directly (reuse logic) */
                    if (strcmp(line, "PING") == 0)
                    {
                        telnet_send_line(client_sock, "ACK,PONG");
                        last_cmd_tick = xTaskGetTickCount();
                    }
                    else if (strcmp(line, "STOP") == 0)
                    {
                        robot_stop();
                        telnet_send_line(client_sock, "ACK,STOP");
                        last_cmd_tick = xTaskGetTickCount();
                    }
                    else if (strcmp(line, "STATUS") == 0)
                    {
                        send_odom();
                        last_cmd_tick = xTaskGetTickCount();
                    }
                    else if (strncmp(line, "DIR,", 4) == 0)
                    {
                        char dir = 0;
                        int speed = 0;
                        if (sscanf(line, "DIR,%c,%d", &dir, &speed) == 2)
                        {
                            robot_set_direction(dir);
                            robot_set_target_speed(speed);
                            telnet_send_line(client_sock, "ACK,DIR");
                            last_cmd_tick = xTaskGetTickCount();
                        }
                        else
                        {
                            telnet_send_line(client_sock, "ERR");
                        }
                    }
                    else
                    {
                        telnet_send_line(client_sock, "ERR");
                    }

                    line_idx = 0;
                }
            }
            else if (ch != '\0')
            {
                if (line_idx < (int)(sizeof(line) - 1))
                {
                    line[line_idx++] = ch;
                }
                else
                {
                    line_idx = 0;
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }

    close(client_sock);
}

static void telnet_server_task(void *arg)
{
    int listen_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_sock < 0)
    {
        ESP_LOGE(TAG, "Failed to create telnet socket");
        vTaskDelete(NULL);
        return;
    }

    /* Set socket option to reuse address */
    int opt = 1;
    setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in server_addr = {0};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(TELNET_PORT);

    if (bind(listen_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        ESP_LOGE(TAG, "Failed to bind telnet socket");
        close(listen_sock);
        vTaskDelete(NULL);
        return;
    }

    if (listen(listen_sock, TELNET_BACKLOG) < 0)
    {
        ESP_LOGE(TAG, "Failed to listen on telnet socket");
        close(listen_sock);
        vTaskDelete(NULL);
        return;
    }

    ESP_LOGI(TAG, "Telnet server listening on port %d", TELNET_PORT);

    while (1)
    {
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);

        int client_sock = accept(listen_sock, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client_sock < 0)
        {
            ESP_LOGW(TAG, "Failed to accept telnet connection");
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }

        telnet_handle_client(client_sock);
    }

    close(listen_sock);
    vTaskDelete(NULL);
}

void telnet_server_start(void)
{
    xTaskCreatePinnedToCore(
        telnet_server_task,
        "telnet_server_task",
        4096,
        NULL,
        3,
        NULL,
        1);
    ESP_LOGI(TAG, "Telnet server task created");
}