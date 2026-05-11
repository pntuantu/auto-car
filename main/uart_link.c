#include "uart_link.h"
#include "driver/usb_serial_jtag.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "kinematics.h"
#include "bno055.h"
#include <string.h>

static const char *TAG = "UART_LINK";

// Cấu trúc gói tin TX (ESP32 gửi Odom -> Pi)
// Yêu cầu GCC đóng gói chặt chẽ không có byte đệm (padding)
#pragma pack(push, 1)
typedef struct {
    uint8_t header[2]; // 0xAA, 0x55
    uint8_t type;      // 0x01 (Bản tin Odom)
    uint8_t length;    // Chiều dài payload
    float odom_x;
    float odom_y;
    float odom_theta;
    float linear_v;
    float angular_w;
    float quat_w;
    float quat_x;
    float quat_y;
    float quat_z;
    uint8_t calib_stat; // Gộp chung trạng thái calib
    uint8_t checksum;  // XOR toàn bộ payload
} OdomPacket_t;

// Cấu trúc gói tin RX (Pi gửi Cmd_vel -> ESP32)
typedef struct {
    float linear_v;
    float angular_w;
} CmdVelPayload_t;
#pragma pack(pop)

static uint8_t calculate_checksum(const uint8_t* data, size_t length) {
    uint8_t crc = 0;
    for (size_t i = 0; i < length; i++) {
        crc ^= data[i];
    }
    return crc;
}

void uart_link_init(void) {
    usb_serial_jtag_driver_config_t usb_cfg = {
        .rx_buffer_size = 1024,
        .tx_buffer_size = 1024
    };
    usb_serial_jtag_driver_install(&usb_cfg);
    ESP_LOGI(TAG, "USB Serial JTAG Link Initialized (Binary Protocol)");
}

void uart_tx_task(void *arg) {
    OdomPacket_t pkt;
    pkt.header[0] = 0xAA;
    pkt.header[1] = 0x55;
    pkt.type = 0x01;
    pkt.length = sizeof(OdomPacket_t) - 4; // Trừ header, type, length, checksum

    while (1) {
        // 1. Lấy dữ liệu Odom từ Kinematics
        kinematics_get_odom(&pkt.odom_x, &pkt.odom_y, &pkt.odom_theta, &pkt.linear_v, &pkt.angular_w);
        
        // 2. Lấy dữ liệu IMU
        pkt.quat_w = g_bno_quat_w;
        pkt.quat_x = g_bno_quat_x;
        pkt.quat_y = g_bno_quat_y;
        pkt.quat_z = g_bno_quat_z;

        // Gộp trạng thái calib (2 bit mỗi loại)
        pkt.calib_stat = (g_bno_calib_sys << 6) | (g_bno_calib_gyro << 4) | (g_bno_calib_accel << 2) | g_bno_calib_mag;

        // 3. Tính Checksum (từ phần byte payload)
        uint8_t* payload_ptr = (uint8_t*)&pkt.odom_x;
        pkt.checksum = calculate_checksum(payload_ptr, pkt.length);

        // 4. Bắn mảng byte qua USB
        usb_serial_jtag_write_bytes((const void*)&pkt, sizeof(OdomPacket_t), portMAX_DELAY);

        vTaskDelay(pdMS_TO_TICKS(50)); // Publish Odom ở 20Hz (Đủ mượt cho Nav2)
    }
}

void uart_rx_task(void *arg) {
    uint8_t rx_buf[64];
    int rx_state = 0;
    CmdVelPayload_t payload;
    uint8_t rx_idx = 0;
    uint8_t expected_len = sizeof(CmdVelPayload_t);

    while (1) {
        uint8_t ch;
        int len = usb_serial_jtag_read_bytes(&ch, 1, portMAX_DELAY);
        if (len > 0) {
            // State Machine Parse Gói Nhị Phân
            switch (rx_state) {
                case 0: if (ch == 0xAA) rx_state = 1; break; // Wait Header 1
                case 1: if (ch == 0x55) rx_state = 2; else rx_state = 0; break; // Wait Header 2
                case 2: if (ch == 0x02) rx_state = 3; else rx_state = 0; break; // Wait Type (0x02 = cmd_vel)
                case 3: if (ch == expected_len) { rx_state = 4; rx_idx = 0; } else rx_state = 0; break;
                case 4: // Read Payload
                    rx_buf[rx_idx++] = ch;
                    if (rx_idx >= expected_len) rx_state = 5;
                    break;
                case 5: // Checksum
                    if (ch == calculate_checksum(rx_buf, expected_len)) {
                        memcpy(&payload, rx_buf, expected_len);
                        // Nhận thành công, gửi lệnh xuống Kinematics
                        kinematics_set_target(payload.linear_v, payload.angular_w);
                    } else {
                        ESP_LOGW(TAG, "RX Checksum Error!");
                    }
                    rx_state = 0;
                    break;
            }
        }
    }
}