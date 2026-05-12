#include "kinematics.h"
#include "motor.h"
#include "esp_log.h"
#include <math.h>

static const char *TAG = "KINEMATICS";

/* --- THÔNG SỐ VẬT LÝ CỦA ROBOT --- */
#define WHEEL_RADIUS_M  0.0425f  // Bán kính bánh xe 85mm / 2 = 0.0425m
#define TRACK_WIDTH_M   0.1800f  // Khoảng cách giữa 2 tâm bánh xe (CẦN ĐO THỰC TẾ & CẬP NHẬT)
#define COUNTS_PER_REV  3960.0f  // Số xung/vòng của JGB37-520
#define MAX_PWM         1023

/* --- TRẠNG THÁI ODOMETRY --- */
static float odom_x = 0.0f;
static float odom_y = 0.0f;
static float odom_theta = 0.0f;
static float current_v = 0.0f;
static float current_w = 0.0f;

/* --- MỤC TIÊU VẬN TỐC (XUNG/GIÂY) --- */
static int target_pps_L = 0;
static int target_pps_R = 0;

/* --- CẤU TRÚC PID --- */
typedef struct {
    float kp, ki, kd;
    float integral;
    float prev_error;
} PID_Controller;

// Bạn có thể tinh chỉnh Kp, Ki, Kd tại đây sau khi test thực tế
static PID_Controller pid_L = {0.2f, 0.08f, 0.01f, 0.0f, 0.0f};
static PID_Controller pid_R = {0.2f, 0.08f, 0.01f, 0.0f, 0.0f};

/* Hàm phụ trợ chuẩn hóa góc về khoảng [-PI, PI] */
static float normalize_angle(float a) {
    while (a > (float)M_PI) a -= 2.0f * (float)M_PI;
    while (a < -(float)M_PI) a += 2.0f * (float)M_PI;
    return a;
}

void kinematics_init(void) {
    odom_x = odom_y = odom_theta = 0.0f;
    target_pps_L = target_pps_R = 0;
    ESP_LOGI(TAG, "Kinematics initialized. Mode: Skid-steer (R=%.3fm, L=%.3fm)", WHEEL_RADIUS_M, TRACK_WIDTH_M);
}

void kinematics_set_target(float linear_v, float angular_w) {
    // 1. Động học nghịch: Tính V_left, V_right (m/s) từ v và w
    float v_left_ms  = linear_v - (angular_w * TRACK_WIDTH_M / 2.0f);
    float v_right_ms = linear_v + (angular_w * TRACK_WIDTH_M / 2.0f);

    // 2. Chuyển đổi m/s sang xung/giây (pps) để cho PID dễ tính toán
    float meters_per_count = (2.0f * (float)M_PI * WHEEL_RADIUS_M) / COUNTS_PER_REV;
    target_pps_L = (int)(v_left_ms / meters_per_count);
    target_pps_R = (int)(v_right_ms / meters_per_count);
}

void kinematics_update_odom(int delta_left, int delta_right, float dt) {
    float meters_per_count = (2.0f * (float)M_PI * WHEEL_RADIUS_M) / COUNTS_PER_REV;

    // Quãng đường di chuyển của mỗi bên (mét)
    float dl = (float)delta_left * meters_per_count;
    float dr = (float)delta_right * meters_per_count;

    // Động học thuận
    float ds = 0.5f * (dl + dr); // Quãng đường tịnh tiến
    float dtheta = (dr - dl) / TRACK_WIDTH_M; // Góc xoay

    // Cập nhật vị trí dùng phương pháp Midpoint Runge-Kutta để tăng độ chuẩn xác
    float theta_mid = odom_theta + 0.5f * dtheta;
    odom_x += ds * cosf(theta_mid);
    odom_y += ds * sinf(theta_mid);
    odom_theta = normalize_angle(odom_theta + dtheta);

    // Cập nhật vận tốc hiện tại
    if (dt > 0.0f) {
        current_v = ds / dt;
        current_w = dtheta / dt;
    } else {
        current_v = current_w = 0.0f;
    }
}

void kinematics_get_odom(float *x, float *y, float *theta, float *v, float *w) {
    if(x) *x = odom_x;
    if(y) *y = odom_y;
    if(theta) *theta = odom_theta;
    if(v) *v = current_v;
    if(w) *w = current_w;
}

static int compute_pid(PID_Controller *pid, int target, int current) {
    float error = (float)(target - current);
    pid->integral += error;

    // Anti-windup: Chống tích lũy lỗi quá lớn
    if (pid->integral > 5000.0f) pid->integral = 5000.0f;
    if (pid->integral < -5000.0f) pid->integral = -5000.0f;

    float derivative = error - pid->prev_error;
    pid->prev_error = error;

    float output = (pid->kp * error) + (pid->ki * pid->integral) + (pid->kd * derivative);

    // Giới hạn đầu ra từ -1023 đến 1023 (âm để chạy lùi)
    if (output > MAX_PWM) output = MAX_PWM;
    if (output < -MAX_PWM) output = -MAX_PWM; 

    return (int)output;
}

void kinematics_pid_loop(int current_speed_left, int current_speed_right) {
    // Chống trôi xe: Reset integral nếu lệnh yêu cầu dừng và xe đã dừng hẳn
    if (target_pps_L == 0 && current_speed_left == 0) pid_L.integral = 0;
    if (target_pps_R == 0 && current_speed_right == 0) pid_R.integral = 0;

    int pwm_out_L = compute_pid(&pid_L, target_pps_L, current_speed_left);
    int pwm_out_R = compute_pid(&pid_R, target_pps_R, current_speed_right);

    // Gửi lệnh xuống Module Động cơ
    motor_set_pwm(pwm_out_L, pwm_out_R);
}