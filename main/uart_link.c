#include "uart_link.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "UART_LINK";

// Frame structure
typedef struct {
    uint8_t start;
    uint8_t msg_type;
    uint16_t data_length;
    uint8_t data[FRAME_MAX_LENGTH];
    uint8_t checksum;
    uint8_t end;
} frame_t;

static velocity_command_t velocity_command = {0, 0, 0};
static uint32_t last_heartbeat_ms = 0;

/**
 * Calculate checksum
 */
static uint8_t calculate_checksum(const uint8_t *data, size_t len)
{
    uint8_t checksum = 0;
    for (size_t i = 0; i < len; i++) {
        checksum ^= data[i];
    }
    return checksum;
}

/**
 * Send frame over UART
 */
static esp_err_t uart_send_frame(const frame_t *frame)
{
    uint8_t buffer[FRAME_MAX_LENGTH + 6];
    size_t pos = 0;
    
    buffer[pos++] = frame->start;
    buffer[pos++] = frame->msg_type;
    buffer[pos++] = (frame->data_length >> 8) & 0xFF;
    buffer[pos++] = frame->data_length & 0xFF;
    
    memcpy(&buffer[pos], frame->data, frame->data_length);
    pos += frame->data_length;
    
    buffer[pos++] = calculate_checksum(buffer, pos);
    buffer[pos++] = frame->end;
    
    uart_write_bytes(UART_LINK_PORT, (const char *)buffer, pos);
    return ESP_OK;
}

/**
 * Parse received frame
 */
static esp_err_t uart_parse_frame(uint8_t *buffer, size_t len, frame_t *frame)
{
    if (len < 6 || buffer[0] != FRAME_START_BYTE || buffer[len-1] != FRAME_END_BYTE) {
        return ESP_ERR_INVALID_ARG;
    }
    
    frame->start = buffer[0];
    frame->msg_type = buffer[1];
    frame->data_length = (buffer[2] << 8) | buffer[3];
    
    if (frame->data_length > FRAME_MAX_LENGTH) {
        return ESP_ERR_INVALID_ARG;
    }
    
    memcpy(frame->data, &buffer[4], frame->data_length);
    frame->checksum = buffer[4 + frame->data_length];
    frame->end = buffer[5 + frame->data_length];
    
    // Verify checksum
    uint8_t expected_checksum = calculate_checksum(buffer, 4 + frame->data_length);
    if (expected_checksum != frame->checksum) {
        return ESP_ERR_INVALID_CRC;
    }
    
    return ESP_OK;
}

void uart_link_init(void)
{
    ESP_LOGI(TAG, "Initializing UART link with Pi");
    
    uart_config_t uart_config = {
        .baud_rate = UART_LINK_BAUDRATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    
    uart_driver_install(UART_LINK_PORT, UART_LINK_BUFFER_SIZE, 0, 0, NULL, 0);
    uart_param_config(UART_LINK_PORT, &uart_config);
    uart_set_pin(UART_LINK_PORT, UART_LINK_TX_PIN, UART_LINK_RX_PIN, 
                 UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    
    last_heartbeat_ms = esp_log_timestamp();
    
    ESP_LOGI(TAG, "UART link initialized at %d baud", UART_LINK_BAUDRATE);
}

void uart_link_send_velocity(float vx, float vy, float omega)
{
    frame_t frame = {FRAME_START_BYTE, MSG_TYPE_VELOCITY, 12, {0}, 0, FRAME_END_BYTE};
    
    // Pack float values as int16_t (scale by 100)
    int16_t vx_i = (int16_t)(vx * 100);
    int16_t vy_i = (int16_t)(vy * 100);
    int16_t omega_i = (int16_t)(omega * 100);
    
    frame.data[0] = (vx_i >> 8) & 0xFF;
    frame.data[1] = vx_i & 0xFF;
    frame.data[2] = (vy_i >> 8) & 0xFF;
    frame.data[3] = vy_i & 0xFF;
    frame.data[4] = (omega_i >> 8) & 0xFF;
    frame.data[5] = omega_i & 0xFF;
    
    frame.checksum = calculate_checksum(frame.data, frame.data_length);
    uart_send_frame(&frame);
}

void uart_link_send_imu(float qw, float qx, float qy, float qz,
                        float roll, float pitch, float yaw)
{
    frame_t frame = {FRAME_START_BYTE, MSG_TYPE_IMU_DATA, 28, {0}, 0, FRAME_END_BYTE};
    
    // Pack as int16_t (quaternion: scale by 1000, angles: scale by 100)
    int16_t *data = (int16_t *)frame.data;
    data[0] = (int16_t)(qw * 1000);
    data[1] = (int16_t)(qx * 1000);
    data[2] = (int16_t)(qy * 1000);
    data[3] = (int16_t)(qz * 1000);
    data[4] = (int16_t)(roll * 100);
    data[5] = (int16_t)(pitch * 100);
    data[6] = (int16_t)(yaw * 100);
    
    frame.checksum = calculate_checksum(frame.data, frame.data_length);
    uart_send_frame(&frame);
}

void uart_link_send_encoders(float encoder_data[4])
{
    frame_t frame = {FRAME_START_BYTE, MSG_TYPE_ENCODER_DATA, 8, {0}, 0, FRAME_END_BYTE};
    
    int16_t *data = (int16_t *)frame.data;
    for (int i = 0; i < 4; i++) {
        data[i] = (int16_t)(encoder_data[i] * 100);
    }
    
    frame.checksum = calculate_checksum(frame.data, frame.data_length);
    uart_send_frame(&frame);
}

void uart_link_send_telemetry(const telemetry_t *telemetry)
{
    frame_t frame = {FRAME_START_BYTE, MSG_TYPE_TELEMETRY, 10, {0}, 0, FRAME_END_BYTE};
    
    int16_t voltage = (int16_t)(telemetry->battery_voltage * 100);
    int16_t current = (int16_t)(telemetry->current_draw * 100);
    
    memcpy(&frame.data[0], &voltage, 2);
    memcpy(&frame.data[2], &current, 2);
    frame.data[4] = telemetry->system_status;
    frame.data[5] = telemetry->error_code;
    
    frame.checksum = calculate_checksum(frame.data, frame.data_length);
    uart_send_frame(&frame);
}

uint8_t uart_link_receive(void)
{
    uint8_t buffer[FRAME_MAX_LENGTH + 6];
    int len = uart_read_bytes(UART_LINK_PORT, buffer, sizeof(buffer), pdMS_TO_TICKS(10));
    
    if (len <= 0) {
        return 0;
    }
    
    frame_t frame;
    if (uart_parse_frame(buffer, len, &frame) != ESP_OK) {
        return 0;
    }
    
    last_heartbeat_ms = esp_log_timestamp();
    
    // Process message
    if (frame.msg_type == MSG_TYPE_VELOCITY) {
        // Parse velocity command
        int16_t *data = (int16_t *)frame.data;
        velocity_command.vx = data[0] / 100.0f;
        velocity_command.vy = data[1] / 100.0f;
        velocity_command.omega = data[2] / 100.0f;
    }
    else if (frame.msg_type == MSG_TYPE_EMERGENCY_STOP) {
        velocity_command.vx = 0.0f;
        velocity_command.vy = 0.0f;
        velocity_command.omega = 0.0f;
        ESP_LOGW(TAG, "Emergency stop received!");
    }
    
    return frame.msg_type;
}

velocity_command_t* uart_link_get_velocity_command(void)
{
    return &velocity_command;
}

uint8_t uart_link_is_connected(void)
{
    uint32_t current_time = esp_log_timestamp();
    // Check if heartbeat received within 1 second
    return (current_time - last_heartbeat_ms) < 1000;
}
