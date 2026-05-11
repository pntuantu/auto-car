#include "encoder.h"
#include "driver/pulse_cnt.h"
#include "esp_log.h"

static const char *TAG = "ENCODER";

// Sơ đồ chân Encoder
#define ENC_FL_A 18
#define ENC_FL_B 17
#define ENC_RL_A 1
#define ENC_RL_B 2
#define ENC_FR_A 11
#define ENC_FR_B 10
#define ENC_RR_A 13
#define ENC_RR_B 12

// Lưu trữ các Handles của 4 bộ đếm
static pcnt_unit_handle_t pcnt_FL = NULL;
static pcnt_unit_handle_t pcnt_RL = NULL;
static pcnt_unit_handle_t pcnt_FR = NULL;
static pcnt_unit_handle_t pcnt_RR = NULL;

// Hàm nội bộ giúp setup 1 bộ Encoder Quadrature (Chế độ x4)
static pcnt_unit_handle_t setup_single_encoder(int pin_a, int pin_b) {
    pcnt_unit_handle_t unit;
    pcnt_unit_config_t unit_config = {
        .high_limit = 32767,
        .low_limit = -32768,
    };
    pcnt_new_unit(&unit_config, &unit);

    // Bộ lọc nhiễu (Glitch Filter)
    pcnt_glitch_filter_config_t filter_config = {.max_glitch_ns = 1000};
    pcnt_unit_set_glitch_filter(unit, &filter_config);

    pcnt_channel_handle_t chan_a, chan_b;
    pcnt_chan_config_t chan_a_cfg = {.edge_gpio_num = pin_a, .level_gpio_num = pin_b};
    pcnt_chan_config_t chan_b_cfg = {.edge_gpio_num = pin_b, .level_gpio_num = pin_a};
    pcnt_new_channel(unit, &chan_a_cfg, &chan_a);
    pcnt_new_channel(unit, &chan_b_cfg, &chan_b);

    // Thiết lập đọc Quadrature Phase để nhận biết chiều quay
    pcnt_channel_set_edge_action(chan_a, PCNT_CHANNEL_EDGE_ACTION_DECREASE, PCNT_CHANNEL_EDGE_ACTION_INCREASE);
    pcnt_channel_set_level_action(chan_a, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_INVERSE);
    pcnt_channel_set_edge_action(chan_b, PCNT_CHANNEL_EDGE_ACTION_INCREASE, PCNT_CHANNEL_EDGE_ACTION_DECREASE);
    pcnt_channel_set_level_action(chan_b, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_INVERSE);

    pcnt_unit_enable(unit);
    pcnt_unit_clear_count(unit);
    pcnt_unit_start(unit);
    
    return unit;
}

void encoder_init(void) {
    pcnt_FL = setup_single_encoder(ENC_FL_A, ENC_FL_B);
    pcnt_RL = setup_single_encoder(ENC_RL_A, ENC_RL_B);
    pcnt_FR = setup_single_encoder(ENC_FR_A, ENC_FR_B);
    pcnt_RR = setup_single_encoder(ENC_RR_A, ENC_RR_B);
    
    ESP_LOGI(TAG, "4 Encoders initialized (Quadrature mode).");
}

void encoder_get_counts(int *count_fl, int *count_rl, int *count_fr, int *count_rr) {
    if (pcnt_FL) pcnt_unit_get_count(pcnt_FL, count_fl);
    if (pcnt_RL) pcnt_unit_get_count(pcnt_RL, count_rl);
    if (pcnt_FR) pcnt_unit_get_count(pcnt_FR, count_fr);
    if (pcnt_RR) pcnt_unit_get_count(pcnt_RR, count_rr);
}

void encoder_clear_counts(void) {
    if (pcnt_FL) pcnt_unit_clear_count(pcnt_FL);
    if (pcnt_RL) pcnt_unit_clear_count(pcnt_RL);
    if (pcnt_FR) pcnt_unit_clear_count(pcnt_FR);
    if (pcnt_RR) pcnt_unit_clear_count(pcnt_RR);
}