#include "motor.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_log.h"

static const char *TAG = "MOTOR_MODULE";

// Motor PWM configuration
typedef struct {
    uint8_t pwm_pin;
    uint8_t dir_pin;
    ledc_channel_t pwm_channel;
    ledc_timer_t pwm_timer;
} motor_config_t;

static motor_config_t motors[4] = {
    {MOTOR_FL_PWM_PIN, MOTOR_FL_DIR_PIN, LEDC_CHANNEL_0, LEDC_TIMER_0},
    {MOTOR_FR_PWM_PIN, MOTOR_FR_DIR_PIN, LEDC_CHANNEL_1, LEDC_TIMER_0},
    {MOTOR_BL_PWM_PIN, MOTOR_BL_DIR_PIN, LEDC_CHANNEL_2, LEDC_TIMER_0},
    {MOTOR_BR_PWM_PIN, MOTOR_BR_DIR_PIN, LEDC_CHANNEL_3, LEDC_TIMER_0},
};

void motor_init(void)
{
    ESP_LOGI(TAG, "Initializing motor control system");
    
    // Configure LEDC timer
    ledc_timer_config_t ledc_timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_num = LEDC_TIMER_0,
        .duty_resolution = LEDC_TIMER_10_BIT,
        .freq_hz = PWM_FREQUENCY,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    ledc_timer_config(&ledc_timer);
    
    // Configure PWM channels and direction GPIO pins
    for (int i = 0; i < 4; i++) {
        // Configure PWM channel
        ledc_channel_config_t ledc_channel = {
            .gpio_num = motors[i].pwm_pin,
            .speed_mode = LEDC_LOW_SPEED_MODE,
            .channel = motors[i].pwm_channel,
            .intr_type = LEDC_INTR_DISABLE,
            .timer_sel = motors[i].pwm_timer,
            .duty = 0,
            .hpoint = 0,
        };
        ledc_channel_config(&ledc_channel);
        
        // Configure direction GPIO pin
        gpio_config_t io_conf = {
            .pin_bit_mask = (1ULL << motors[i].dir_pin),
            .mode = GPIO_MODE_OUTPUT,
            .pull_up_en = GPIO_PULLUP_DISABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_DISABLE,
        };
        gpio_config(&io_conf);
        gpio_set_level(motors[i].dir_pin, 0);
    }
    
    ESP_LOGI(TAG, "Motor initialization complete");
}

void motor_set_pwm(uint8_t motor_id, int16_t pwm_value)
{
    if (motor_id >= 4) {
        ESP_LOGW(TAG, "Invalid motor ID: %d", motor_id);
        return;
    }
    
    // Clamp PWM value
    if (pwm_value > MAX_PWM_VALUE) pwm_value = MAX_PWM_VALUE;
    if (pwm_value < -MAX_PWM_VALUE) pwm_value = -MAX_PWM_VALUE;
    
    // Set direction
    uint32_t direction = (pwm_value >= 0) ? 1 : 0;
    gpio_set_level(motors[motor_id].dir_pin, direction);
    
    // Set PWM duty
    uint32_t duty = (pwm_value < 0) ? -pwm_value : pwm_value;
    ledc_set_duty(LEDC_LOW_SPEED_MODE, motors[motor_id].pwm_channel, duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, motors[motor_id].pwm_channel);
}

void motor_set_velocity(uint8_t motor_id, float velocity)
{
    // TODO: Convert velocity (m/s) to PWM value using calibration curve
    // This requires encoder feedback and motor characterization
    int16_t pwm_value = (int16_t)(velocity * 100); // Placeholder conversion
    motor_set_pwm(motor_id, pwm_value);
}

void motor_stop_all(void)
{
    for (int i = 0; i < 4; i++) {
        motor_set_pwm(i, 0);
    }
}

void motor_stop(uint8_t motor_id)
{
    if (motor_id < 4) {
        motor_set_pwm(motor_id, 0);
    }
}
