// esp_dc.c
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "driver/gpio.h"
#include "driver/ledc.h"
#include "driver/pulse_cnt.h"
#include "driver/usb_serial_jtag.h"

#include "esp_log.h"
#include "esp_err.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "esp_http_server.h"

#include "bno055.h"
#include "uart_pi.h"

static const char *TAG = "ROBOT_MAIN";

/* =========================
 * Wi-Fi config
 * ========================= */
#define WIFI_SSID "Chuong_5G"
#define WIFI_PASS "chuongvan86"
#define WIFI_CONNECTED_BIT BIT0

static EventGroupHandle_t s_wifi_event_group;
static httpd_handle_t server = NULL;

/* =========================
 * Motor / PID config
 * ========================= */
#define RPWM_L_PIN 4
#define LPWM_L_PIN 5
#define RPWM_R_PIN 6
#define LPWM_R_PIN 7

#define PWM_FREQ_HZ 5000
#define MAX_PWM 1023
#define LOOP_TIME_MS 100
#define SPEED_STEP 300

#define ENC_FL_A 15
#define ENC_FL_B 16
#define ENC_RL_A 17
#define ENC_RL_B 18
#define ENC_FR_A 1
#define ENC_FR_B 2
#define ENC_RR_A 10
#define ENC_RR_B 11

pcnt_unit_handle_t pcnt_FL, pcnt_RL, pcnt_FR, pcnt_RR;

typedef enum
{
    DIR_STOP,
    DIR_FWD,
    DIR_BWD,
    DIR_LEFT,
    DIR_RIGHT
} robot_dir_t;

volatile robot_dir_t current_dir = DIR_STOP;
volatile int target_speed_pps = 2500;

typedef struct
{
    float kp;
    float ki;
    float kd;
    float integral;
    float prev_error;
    int max_out;
} PID_Controller;

PID_Controller pid_L = {.kp = 0.2, .ki = 0.08, .kd = 0.01, .integral = 0, .prev_error = 0, .max_out = MAX_PWM};
PID_Controller pid_R = {.kp = 0.2, .ki = 0.08, .kd = 0.01, .integral = 0, .prev_error = 0, .max_out = MAX_PWM};

/* Shared data for uart_pi.c */
int g_speed_left_pps = 0;
int g_speed_right_pps = 0;
int g_delta_left_count = 0;
int g_delta_right_count = 0;

/* =========================
 * Shared functions for uart_pi.c
 * ========================= */
void robot_set_direction(char cmd)
{
    switch (cmd)
    {
    case 'F':
        current_dir = DIR_FWD;
        break;
    case 'B':
        current_dir = DIR_BWD;
        break;
    case 'L':
        current_dir = DIR_LEFT;
        break;
    case 'R':
        current_dir = DIR_RIGHT;
        break;
    case 'S':
    default:
        current_dir = DIR_STOP;
        break;
    }
}

void robot_set_target_speed(int pps)
{
    target_speed_pps = pps;
    if (target_speed_pps < 0)
        target_speed_pps = 0;
}

void robot_stop(void)
{
    current_dir = DIR_STOP;
    pid_L.integral = 0;
    pid_L.prev_error = 0;
    pid_R.integral = 0;
    pid_R.prev_error = 0;
}

/* =========================
 * PID / motor helpers
 * ========================= */
int compute_pid(PID_Controller *pid, int target, int current)
{
    float error = (float)(target - current);

    pid->integral += error;
    if (pid->integral > 5000)
        pid->integral = 5000;
    if (pid->integral < -5000)
        pid->integral = -5000;

    float derivative = error - pid->prev_error;
    pid->prev_error = error;

    float output = (pid->kp * error) + (pid->ki * pid->integral) + (pid->kd * derivative);

    if (output > pid->max_out)
        output = pid->max_out;
    if (output < 0)
        output = 0;

    return (int)output;
}

void reset_pid(void)
{
    pid_L.integral = 0;
    pid_L.prev_error = 0;
    pid_R.integral = 0;
    pid_R.prev_error = 0;
}

void motor_pwm_init(void)
{
    ledc_timer_config_t timer_cfg = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_num = LEDC_TIMER_0,
        .duty_resolution = LEDC_TIMER_10_BIT,
        .freq_hz = PWM_FREQ_HZ,
        .clk_cfg = LEDC_AUTO_CLK};
    ledc_timer_config(&timer_cfg);

    int pins[4] = {RPWM_L_PIN, LPWM_L_PIN, RPWM_R_PIN, LPWM_R_PIN};
    for (int i = 0; i < 4; i++)
    {
        ledc_channel_config_t chan_cfg = {
            .speed_mode = LEDC_LOW_SPEED_MODE,
            .channel = i,
            .timer_sel = LEDC_TIMER_0,
            .intr_type = LEDC_INTR_DISABLE,
            .gpio_num = pins[i],
            .duty = 0,
            .hpoint = 0};
        ledc_channel_config(&chan_cfg);
    }
}

pcnt_unit_handle_t setup_encoder(int pin_a, int pin_b)
{
    pcnt_unit_handle_t unit;
    pcnt_unit_config_t unit_config = {
        .high_limit = 30000,
        .low_limit = -30000};
    pcnt_new_unit(&unit_config, &unit);

    pcnt_glitch_filter_config_t filter_config = {.max_glitch_ns = 1000};
    pcnt_unit_set_glitch_filter(unit, &filter_config);

    pcnt_channel_handle_t chan_a, chan_b;
    pcnt_chan_config_t chan_a_cfg = {.edge_gpio_num = pin_a, .level_gpio_num = pin_b};
    pcnt_chan_config_t chan_b_cfg = {.edge_gpio_num = pin_b, .level_gpio_num = pin_a};
    pcnt_new_channel(unit, &chan_a_cfg, &chan_a);
    pcnt_new_channel(unit, &chan_b_cfg, &chan_b);

    pcnt_channel_set_edge_action(chan_a, PCNT_CHANNEL_EDGE_ACTION_DECREASE, PCNT_CHANNEL_EDGE_ACTION_INCREASE);
    pcnt_channel_set_level_action(chan_a, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_INVERSE);
    pcnt_channel_set_edge_action(chan_b, PCNT_CHANNEL_EDGE_ACTION_INCREASE, PCNT_CHANNEL_EDGE_ACTION_DECREASE);
    pcnt_channel_set_level_action(chan_b, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_INVERSE);

    pcnt_unit_enable(unit);
    pcnt_unit_clear_count(unit);
    pcnt_unit_start(unit);
    return unit;
}

void apply_pwm(int pwm_L, int pwm_R)
{
    switch (current_dir)
    {
    case DIR_FWD:
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, pwm_L);
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, 0);
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_2, pwm_R);
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_3, 0);
        break;
    case DIR_BWD:
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0);
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, pwm_L);
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_2, 0);
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_3, pwm_R);
        break;
    case DIR_LEFT:
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0);
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, pwm_L);
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_2, pwm_R);
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_3, 0);
        break;
    case DIR_RIGHT:
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, pwm_L);
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, 0);
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_2, 0);
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_3, pwm_R);
        break;
    case DIR_STOP:
    default:
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0);
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, 0);
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_2, 0);
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_3, 0);
        break;
    }

    for (int i = 0; i < 4; i++)
    {
        ledc_update_duty(LEDC_LOW_SPEED_MODE, i);
    }
}

/* =========================
 * Web UI
 * ========================= */
static const char *INDEX_HTML =
    "<!DOCTYPE html><html><head>"
    "<meta name='viewport' content='width=device-width, initial-scale=1'>"
    "<title>ESP32-S3 Car</title>"
    "<style>"
    "body{font-family:Arial;text-align:center;margin-top:30px;}"
    "button{width:110px;height:60px;font-size:22px;margin:8px;border-radius:12px;border:none;background:#1976d2;color:white;}"
    ".stop{background:#d32f2f;}"
    "</style></head><body>"
    "<h2>ESP32-S3 Car Control</h2>"
    "<div><button onclick=\"sendCmd('F')\">Forward</button></div>"
    "<div>"
    "<button onclick=\"sendCmd('L')\">Left</button>"
    "<button class='stop' onclick=\"sendCmd('S')\">Stop</button>"
    "<button onclick=\"sendCmd('R')\">Right</button>"
    "</div>"
    "<div><button onclick=\"sendCmd('B')\">Backward</button></div>"
    "<div style='margin-top:16px;'>"
    "<button onclick=\"sendCmd('P')\">Speed +</button>"
    "<button onclick=\"sendCmd('M')\">Speed -</button>"
    "</div>"
    "<p id='status'>Ready</p>"
    "<script>"
    "function sendCmd(cmd){"
    "fetch('/cmd?c='+encodeURIComponent(cmd))"
    ".then(r=>r.text())"
    ".then(t=>document.getElementById('status').innerText=t)"
    ".catch(()=>document.getElementById('status').innerText='Error');"
    "}"
    "</script></body></html>";

static esp_err_t root_get_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html");
    return httpd_resp_send(req, INDEX_HTML, HTTPD_RESP_USE_STRLEN);
}

static esp_err_t cmd_get_handler(httpd_req_t *req)
{
    char query[64];
    char param[8] = {0};

    if (httpd_req_get_url_query_str(req, query, sizeof(query)) == ESP_OK)
    {
        if (httpd_query_key_value(query, "c", param, sizeof(param)) == ESP_OK)
        {
            char c = param[0];

            switch (c)
            {
            case 'F':
                current_dir = DIR_FWD;
                ESP_LOGW(TAG, "[TIEN] HTTP");
                break;
            case 'B':
                current_dir = DIR_BWD;
                ESP_LOGW(TAG, "[LUI] HTTP");
                break;
            case 'L':
                current_dir = DIR_LEFT;
                ESP_LOGW(TAG, "[XOAY TRAI] HTTP");
                break;
            case 'R':
                current_dir = DIR_RIGHT;
                ESP_LOGW(TAG, "[XOAY PHAI] HTTP");
                break;
            case 'S':
                current_dir = DIR_STOP;
                reset_pid();
                ESP_LOGW(TAG, "[DUNG] HTTP");
                break;
            case 'P':
                target_speed_pps += SPEED_STEP;
                ESP_LOGI(TAG, "MUC TIEU TANG: %d Xung/s", target_speed_pps);
                break;
            case 'M':
                target_speed_pps -= SPEED_STEP;
                if (target_speed_pps < 0)
                    target_speed_pps = 0;
                ESP_LOGI(TAG, "MUC TIEU GIAM: %d Xung/s", target_speed_pps);
                break;
            default:
                break;
            }

            char resp[64];
            snprintf(resp, sizeof(resp), "CMD=%c SPEED=%d", c, target_speed_pps);
            httpd_resp_set_type(req, "text/plain");
            return httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
        }
    }

    httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing c");
    return ESP_FAIL;
}

static httpd_handle_t start_webserver(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t handle = NULL;

    if (httpd_start(&handle, &config) == ESP_OK)
    {
        httpd_uri_t root = {
            .uri = "/",
            .method = HTTP_GET,
            .handler = root_get_handler,
            .user_ctx = NULL};

        httpd_uri_t cmd = {
            .uri = "/cmd",
            .method = HTTP_GET,
            .handler = cmd_get_handler,
            .user_ctx = NULL};

        httpd_register_uri_handler(handle, &root);
        httpd_register_uri_handler(handle, &cmd);
        ESP_LOGI(TAG, "HTTP server started");
        return handle;
    }

    ESP_LOGE(TAG, "Failed to start HTTP server");
    return NULL;
}

/* =========================
 * Wi-Fi
 * ========================= */
static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        // ESP_LOGW(TAG, "Wi-Fi disconnected, reconnecting...");
        esp_wifi_connect();
        xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

static void wifi_init_sta(void)
{
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        NULL));

    wifi_config_t wifi_config = {0};
    strncpy((char *)wifi_config.sta.ssid, WIFI_SSID, sizeof(wifi_config.sta.ssid));
    strncpy((char *)wifi_config.sta.password, WIFI_PASS, sizeof(wifi_config.sta.password));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_sta finished. Connecting to %s", WIFI_SSID);
    // Đợi tối đa 5 giây, nếu không có Wi-Fi vẫn chạy tiếp các lệnh dưới
    xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT, pdFALSE, pdFALSE, pdMS_TO_TICKS(5000));
}

/* =========================
 * Main
 * ========================= */
void app_main(void)
{
    usb_serial_jtag_driver_config_t usb_cfg = {
        .rx_buffer_size = 2048,
        .tx_buffer_size = 2048};
    ESP_ERROR_CHECK(usb_serial_jtag_driver_install(&usb_cfg));

    motor_pwm_init();
    pcnt_FL = setup_encoder(ENC_FL_A, ENC_FL_B);
    pcnt_RL = setup_encoder(ENC_RL_A, ENC_RL_B);
    pcnt_FR = setup_encoder(ENC_FR_A, ENC_FR_B);
    pcnt_RR = setup_encoder(ENC_RR_A, ENC_RR_B);

    apply_pwm(0, 0);

    xTaskCreatePinnedToCore(
        bno055_task,
        "bno055_task",
        4096,
        NULL,
        3,
        NULL,
        1);

    wifi_init_sta();
    server = start_webserver();

    pi_link_init();
    pi_link_reset_odom();
    telnet_server_start();

    xTaskCreatePinnedToCore(
        pi_uart_task,
        "pi_uart_task",
        4096,
        NULL,
        4,
        NULL,
        0);
    ESP_LOGI(TAG, "--- HE THONG MANUAL + PI USB LINK SAN SANG ---");

    int last_vFL = 0, last_vRL = 0, last_vFR = 0, last_vRR = 0;

    while (1)
    {
        int vFL = 0, vRL = 0, vFR = 0, vRR = 0;
        pcnt_unit_get_count(pcnt_FL, &vFL);
        pcnt_unit_get_count(pcnt_RL, &vRL);
        pcnt_unit_get_count(pcnt_FR, &vFR);
        pcnt_unit_get_count(pcnt_RR, &vRR);

        /* IMPORTANT: compute delta first */
        int dFL = vFL - last_vFL;
        int dRL = vRL - last_vRL;
        int dFR = vFR - last_vFR;
        int dRR = vRR - last_vRR;

        int raw_speed_FL = -(dFL) * (1000 / LOOP_TIME_MS);
        int raw_speed_RL = -(dRL) * (1000 / LOOP_TIME_MS);
        int raw_speed_FR = (dFR) * (1000 / LOOP_TIME_MS);
        int raw_speed_RR = (dRR) * (1000 / LOOP_TIME_MS);

        static int speed_FL = 0, speed_RL = 0, speed_FR = 0, speed_RR = 0;

        if (abs(raw_speed_FL) < 6000)
            speed_FL = raw_speed_FL;
        if (abs(raw_speed_RL) < 6000)
            speed_RL = raw_speed_RL;
        if (abs(raw_speed_FR) < 6000)
            speed_FR = raw_speed_FR;
        if (abs(raw_speed_RR) < 6000)
            speed_RR = raw_speed_RR;

        int avg_speed_L_signed = (speed_FL + speed_RL) / 2;
        int avg_speed_R_signed = (speed_FR + speed_RR) / 2;

        int avg_speed_L = abs(avg_speed_L_signed);
        int avg_speed_R = abs(avg_speed_R_signed);

        g_speed_left_pps = avg_speed_L_signed;
        g_speed_right_pps = avg_speed_R_signed;

        g_delta_left_count = (dFL + dRL) / 2;
        g_delta_right_count = (dFR + dRR) / 2;

        /* Update odometry and send to Pi */
        pi_link_update_odom_from_counts(g_delta_left_count, g_delta_right_count, LOOP_TIME_MS / 1000.0f);
        pi_link_periodic();

        /* Update last counts after delta is used */
        last_vFL = vFL;
        last_vRL = vRL;
        last_vFR = vFR;
        last_vRR = vRR;

        if (current_dir != DIR_STOP)
        {
            int pwm_out_L = compute_pid(&pid_L, target_speed_pps, avg_speed_L);
            int pwm_out_R = compute_pid(&pid_R, target_speed_pps, avg_speed_R);

            apply_pwm(pwm_out_L, pwm_out_R);

            ESP_LOGI(TAG,
                     "TARGET: %d | TRAI [Speed: %4d -> PWM: %4d] | PHAI [Speed: %4d -> PWM: %4d]",
                     target_speed_pps, avg_speed_L, pwm_out_L, avg_speed_R, pwm_out_R);
        }
        else
        {
            apply_pwm(0, 0);
        }

        vTaskDelay(pdMS_TO_TICKS(LOOP_TIME_MS));
    }
}