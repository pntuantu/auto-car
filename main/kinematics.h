#ifndef KINEMATICS_H
#define KINEMATICS_H

#ifdef __cplusplus
extern "C" {
#endif

// Khởi tạo các thông số Odom và PID
void kinematics_init(void);

// Tính toán tọa độ và vận tốc thực tế của xe
// Gọi định kỳ sau mỗi khi tính được biến thiên xung (delta) của bánh xe
void kinematics_update_odom(int delta_left, int delta_right, float dt);

// Đặt mục tiêu vận tốc cho Robot (nhận từ Raspberry Pi)
// linear_v: Vận tốc thẳng (m/s) | angular_w: Vận tốc góc (rad/s)
void kinematics_set_target(float linear_v, float angular_w);

// Vòng lặp PID điều khiển động cơ, gọi định kỳ (VD: mỗi 50ms)
// Đầu vào: Vận tốc thực tế hiện tại của 2 bên bánh (tính bằng xung/giây - pps)
void kinematics_pid_loop(int current_speed_left, int current_speed_right);

// Lấy dữ liệu Odom để chuẩn bị đóng gói gửi lên Pi
void kinematics_get_odom(float *x, float *y, float *theta, float *v, float *w);

#ifdef __cplusplus
}
#endif

#endif // KINEMATICS_H