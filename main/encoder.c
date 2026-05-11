#include "encoder.h"
#include "driver/gpio.h"
#include "driver/pcnt.h"
#include "esp_log.h"

static const char *TAG = "ENCODER_MODULE";

// Encoder data structure
typedef struct {
    uint8_t pin_a;
    uint8_t pin_b;
    pcnt_unit_t pcnt_unit;
    int32_t prev_count;
    float velocity;
    float rpm;
} encoder_data_t;

static encoder_data_t encoders[4] = {
    {ENCODER_FL_A_PIN, ENCODER_FL_B_PIN, PCNT_UNIT_0, 0, 0, 0},
    {ENCODER_FR_A_PIN, ENCODER_FR_B_PIN, PCNT_UNIT_1, 0, 0, 0},
    {ENCODER_BL_A_PIN, ENCODER_BL_B_PIN, PCNT_UNIT_2, 0, 0, 0},
    {ENCODER_BR_A_PIN, ENCODER_BR_B_PIN, PCNT_UNIT_3, 0, 0, 0},
};

void encoder_init(void)
{
    ESP_LOGI(TAG, "Initializing encoder system");
    
    for (int i = 0; i < 4; i++) {
        // Configure GPIO pins for input
        gpio_config_t io_conf = {
            .pin_bit_mask = (1ULL << encoders[i].pin_a) | (1ULL << encoders[i].pin_b),
            .mode = GPIO_MODE_INPUT,
            .pull_up_en = GPIO_PULLUP_ENABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_DISABLE,
        };
        gpio_config(&io_conf);
        
        // Configure PCNT unit
        pcnt_config_t pcnt_config = {
            .pulse_gpio_num = encoders[i].pin_a,
            .ctrl_gpio_num = encoders[i].pin_b,
            .unit = encoders[i].pcnt_unit,
            .channel = PCNT_CHANNEL_0,
            .pos_mode = PCNT_COUNT_INC,
            .neg_mode = PCNT_COUNT_DEC,
            .lctrl_mode = PCNT_MODE_REVERSE,
            .hctrl_mode = PCNT_MODE_KEEP,
            .counter_h_lim = 32767,
            .counter_l_lim = -32768,
        };
        pcnt_unit_config(&pcnt_config);
        
        // Initialize PCNT
        pcnt_counter_pause(encoders[i].pcnt_unit);
        pcnt_counter_clear(encoders[i].pcnt_unit);
        pcnt_counter_resume(encoders[i].pcnt_unit);
    }
    
    ESP_LOGI(TAG, "Encoder initialization complete");
}

void encoder_update(void)
{
    for (int i = 0; i < 4; i++) {
        int16_t count = 0;
        pcnt_get_counter_value(encoders[i].pcnt_unit, &count);
        
        // Calculate velocity (distance per 20ms)
        int32_t pulse_delta = count - encoders[i].prev_count;
        float distance = (float)pulse_delta * WHEEL_CIRCUMFERENCE / ENCODER_PPR;
        encoders[i].velocity = distance / 0.02f; // 20ms update rate
        
        // Calculate RPM
        encoders[i].rpm = (float)pulse_delta * 50.0f / ENCODER_PPR; // 50 = 1000ms / 20ms
        
        encoders[i].prev_count = count;
    }
}

int32_t encoder_get_count(uint8_t encoder_id)
{
    if (encoder_id >= 4) {
        ESP_LOGW(TAG, "Invalid encoder ID: %d", encoder_id);
        return 0;
    }
    
    int16_t count = 0;
    pcnt_get_counter_value(encoders[encoder_id].pcnt_unit, &count);
    return (int32_t)count;
}

float encoder_get_velocity(uint8_t encoder_id)
{
    if (encoder_id >= 4) {
        ESP_LOGW(TAG, "Invalid encoder ID: %d", encoder_id);
        return 0.0f;
    }
    
    return encoders[encoder_id].velocity;
}

float encoder_get_rpm(uint8_t encoder_id)
{
    if (encoder_id >= 4) {
        ESP_LOGW(TAG, "Invalid encoder ID: %d", encoder_id);
        return 0.0f;
    }
    
    return encoders[encoder_id].rpm;
}

void encoder_reset(uint8_t encoder_id)
{
    if (encoder_id >= 4) {
        ESP_LOGW(TAG, "Invalid encoder ID: %d", encoder_id);
        return;
    }
    
    pcnt_counter_pause(encoders[encoder_id].pcnt_unit);
    pcnt_counter_clear(encoders[encoder_id].pcnt_unit);
    pcnt_counter_resume(encoders[encoder_id].pcnt_unit);
    encoders[encoder_id].prev_count = 0;
}

void encoder_reset_all(void)
{
    for (int i = 0; i < 4; i++) {
        encoder_reset(i);
    }
}
