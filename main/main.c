#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_flash.h"

// Bao gồm các module chúng ta đã tự thiết kế
#include "motor.h"
#include "encoder.h"
#include "kinematics.h"
#include "bno055.h"
#include "uart_link.h"

static const char *TAG = "MAIN";

// Chu kỳ vòng lặp chính (50ms = Tần số 20Hz)
// 20Hz là mức lý tưởng để tính Odom và chạy PID mượt mà cho Nav2
#define LOOP_TIME_MS 50 

void app_main(void) {
    // 1. Khởi tạo bộ nhớ NVS (Cần thiết cho hệ thống ESP-IDF hoạt động ổn định)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "=== STARTING LOW-LEVEL CONTROL FIRMWARE ===");

    // 2. Khởi tạo các module phần cứng và thuật toán
    motor_init();
    encoder_init();
    kinematics_init();
    uart_link_init();

    // 3. Khởi tạo các Task chạy ngầm
    // Đẩy task đọc I2C BNO055 sang Core 1 để không tranh chấp với PID
    xTaskCreatePinnedToCore(bno055_task, "bno055_task", 4096, NULL, 5, NULL, 1);

    // Giao tiếp USB Serial JTAG chạy trên Core 0
    xTaskCreatePinnedToCore(uart_tx_task, "uart_tx_task", 4096, NULL, 4, NULL, 0);
    xTaskCreatePinnedToCore(uart_rx_task, "uart_rx_task", 4096, NULL, 4, NULL, 0);

    ESP_LOGI(TAG, "All background tasks started successfully.");

    // Biến lưu trữ trạng thái xung của chu kỳ trước
    int last_count_fl = 0, last_count_rl = 0, last_count_fr = 0, last_count_rr = 0;

    // 4. VÒNG LẶP ĐIỀU KHIỂN CHÍNH (MAIN CONTROL LOOP)
    while (1) {
        int count_fl = 0, count_rl = 0, count_fr = 0, count_rr = 0;

        // Đọc tổng số xung hiện tại từ 4 bánh
        encoder_get_counts(&count_fl, &count_rl, &count_fr, &count_rr);

        // Tính số xung thay đổi (Delta) trong 50ms vừa qua
        int d_fl = count_fl - last_count_fl;
        int d_rl = count_rl - last_count_rl;
        int d_fr = count_fr - last_count_fr;
        int d_rr = count_rr - last_count_rr;

        // Lưu lại giá trị cho vòng lặp tiếp theo
        last_count_fl = count_fl;
        last_count_rl = count_rl;
        last_count_fr = count_fr;
        last_count_rr = count_rr;

        /* * LƯU Ý PHẦN CỨNG (Worst-case mitigation): 
         * Động cơ lắp đối xứng 2 bên xe thường bị ngược chiều quay cơ khí. 
         * Nếu khi thử tiến tới mà số xung bên Phải tăng (+), nhưng bên Trái lại giảm (-), 
         * bạn cần đảo dấu bằng cách thêm dấu trừ: d_fl = -d_fl; d_rl = -d_rl;
         */
        
        // Gộp trung bình xung của cơ cấu Skid-steer (Lấy trung bình 2 bánh cùng 1 bên)
        int delta_left = (d_fl + d_rl) / 2;
        int delta_right = (d_fr + d_rr) / 2;

        // Cập nhật tọa độ Odom (Động học thuận)
        kinematics_update_odom(delta_left, delta_right, (float)LOOP_TIME_MS / 1000.0f);

        // Tính vận tốc thực tế hiện tại (Xung/giây - pps)
        int current_pps_L = delta_left * (1000 / LOOP_TIME_MS);
        int current_pps_R = delta_right * (1000 / LOOP_TIME_MS);

        // Chạy bộ điều khiển PID băm xung ra Động cơ để bám sát lệnh từ Pi 4
        kinematics_pid_loop(current_pps_L, current_pps_R);

        // Ngủ chính xác 50ms rồi lặp lại
        vTaskDelay(pdMS_TO_TICKS(LOOP_TIME_MS));
    }
}