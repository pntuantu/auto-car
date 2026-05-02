// bno055.h
#pragma once

#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C"
{
#endif

    extern volatile float g_bno_heading_deg;
    extern volatile float g_bno_roll_deg;
    extern volatile float g_bno_pitch_deg;

    extern volatile uint8_t g_bno_calib_sys;
    extern volatile uint8_t g_bno_calib_gyro;
    extern volatile uint8_t g_bno_calib_accel;
    extern volatile uint8_t g_bno_calib_mag;

    esp_err_t bno055_init(void);
    esp_err_t bno055_read_euler(float *heading_deg, float *roll_deg, float *pitch_deg);
    esp_err_t bno055_read_calibration(uint8_t *sys, uint8_t *gyro, uint8_t *accel, uint8_t *mag);
    void bno055_task(void *arg);

#ifdef __cplusplus
}
#endif