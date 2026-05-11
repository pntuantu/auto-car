#include "motor.h"
#include "driver/ledc.h"
#include "esp_log.h"
#include <stdlib.h> // cho hàm abs()

static const char *TAG = "MOTOR";

// Cấu hình chân theo sơ đồ của bạn
#define RPWM_RIGHT_PIN  4
#define LPWM_RIGHT_PIN  5
#define RPWM_LEFT_PIN   6
#define LPWM_LEFT_PIN   7

#define PWM_FREQ_HZ     5000
#define PWM_RESOLUTION  LEDC_TIMER_10_BIT // 0 - 1023
#define PWM_MAX_VALUE   1023

// Ánh xạ Kênh LEDC
#define LEDC_CH_RIGHT_FWD LEDC_CHANNEL_0 // RPWM_RIGHT
#define LEDC_CH_RIGHT_BWD LEDC_CHANNEL_1 // LPWM_RIGHT
#define LEDC_CH_LEFT_FWD  LEDC_CHANNEL_2 // RPWM_LEFT
#define LEDC_CH_LEFT_BWD  LEDC_CHANNEL_3 // LPWM_LEFT

void motor_init(void) {
    // 1. Cấu hình Timer
    ledc_timer_config_t timer_cfg = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_num  = LEDC_TIMER_0,
        .duty_resolution = PWM_RESOLUTION,
        .freq_hz = PWM_FREQ_HZ,
        .clk_cfg = LEDC_AUTO_CLK
    };
    ledc_timer_config(&timer_cfg);

    // 2. Cấu hình 4 Channel cho 4 chân PWM
    int pins[4]     = {RPWM_RIGHT_PIN, LPWM_RIGHT_PIN, RPWM_LEFT_PIN, LPWM_LEFT_PIN};
    int channels[4] = {LEDC_CH_RIGHT_FWD, LEDC_CH_RIGHT_BWD, LEDC_CH_LEFT_FWD, LEDC_CH_LEFT_BWD};

    for (int i = 0; i < 4; i++) {
        ledc_channel_config_t chan_cfg = {
            .speed_mode = LEDC_LOW_SPEED_MODE,
            .channel    = channels[i],
            .timer_sel  = LEDC_TIMER_0,
            .intr_type  = LEDC_INTR_DISABLE,
            .gpio_num   = pins[i],
            .duty       = 0,
            .hpoint     = 0
        };
        ledc_channel_config(&chan_cfg);
    }
    ESP_LOGI(TAG, "Motor initialized with BTS7960 config.");
}

void motor_set_pwm(int pwm_left, int pwm_right) {
    // Giới hạn giá trị PWM từ -1023 đến 1023
    if (pwm_left > PWM_MAX_VALUE) pwm_left = PWM_MAX_VALUE;
    if (pwm_left < -PWM_MAX_VALUE) pwm_left = -PWM_MAX_VALUE;
    if (pwm_right > PWM_MAX_VALUE) pwm_right = PWM_MAX_VALUE;
    if (pwm_right < -PWM_MAX_VALUE) pwm_right = -PWM_MAX_VALUE;

    // Điều khiển bánh Trái
    if (pwm_left >= 0) {
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CH_LEFT_FWD, pwm_left);
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CH_LEFT_BWD, 0);
    } else {
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CH_LEFT_FWD, 0);
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CH_LEFT_BWD, abs(pwm_left));
    }

    // Điều khiển bánh Phải
    if (pwm_right >= 0) {
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CH_RIGHT_FWD, pwm_right);
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CH_RIGHT_BWD, 0);
    } else {
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CH_RIGHT_FWD, 0);
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CH_RIGHT_BWD, abs(pwm_right));
    }

    // Cập nhật Duty Cycle ra chân GPIO
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CH_LEFT_FWD);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CH_LEFT_BWD);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CH_RIGHT_FWD);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CH_RIGHT_BWD);
}