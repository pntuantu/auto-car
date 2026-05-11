#ifndef BNO055_H
#define BNO055_H

#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

// Các biến lưu trữ Quaternion (đã chuẩn hóa về float)
extern volatile float g_bno_quat_w;
extern volatile float g_bno_quat_x;
extern volatile float g_bno_quat_y;
extern volatile float g_bno_quat_z;

// Trạng thái Calibration (0: Chưa calib -> 3: Calib chuẩn)
extern volatile uint8_t g_bno_calib_sys;
extern volatile uint8_t g_bno_calib_gyro;
extern volatile uint8_t g_bno_calib_accel;
extern volatile uint8_t g_bno_calib_mag;

// Khởi tạo BNO055 qua I2C
esp_err_t bno055_init(void);

// Task chạy ngầm để cập nhật dữ liệu liên tục
void bno055_task(void *arg);

#ifdef __cplusplus
}
#endif

#endif // BNO055_H