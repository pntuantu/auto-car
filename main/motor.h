#ifndef MOTOR_H
#define MOTOR_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Khởi tạo các chân PWM cho 2 mạch BTS7960
void motor_init(void);

// Điều khiển tốc độ và chiều quay
// pwm_left, pwm_right: Giá trị từ -1023 đến 1023
// Số dương: Tiến | Số âm: Lùi | 0: Dừng
void motor_set_pwm(int pwm_left, int pwm_right);

#ifdef __cplusplus
}
#endif

#endif // MOTOR_H