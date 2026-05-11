#ifndef ENCODER_H
#define ENCODER_H

#include <stdint.h>

/**
 * Encoder Module
 * 
 * Reads wheel encoder data using PCNT (Pulse Count) peripheral
 * Provides RPM, velocity, and position feedback for 4 wheels
 * 
 * Encoder connections (ESP32 GPIO):
 *   - Encoder 0 (Front-Left):  GPIO21, GPIO22
 *   - Encoder 1 (Front-Right): GPIO23, GPIO19
 *   - Encoder 2 (Back-Left):   GPIO25, GPIO26
 *   - Encoder 3 (Back-Right):  GPIO27, GPIO14
 */

// Encoder pin definitions
#define ENCODER_FL_A_PIN    21
#define ENCODER_FL_B_PIN    22
#define ENCODER_FR_A_PIN    23
#define ENCODER_FR_B_PIN    19
#define ENCODER_BL_A_PIN    25
#define ENCODER_BL_B_PIN    26
#define ENCODER_BR_A_PIN    27
#define ENCODER_BR_B_PIN    14

// Encoder specifications
#define ENCODER_PPR         16      // Pulses per revolution
#define WHEEL_RADIUS        0.05f   // Wheel radius in meters
#define WHEEL_CIRCUMFERENCE (2 * 3.14159f * WHEEL_RADIUS)

/**
 * Initialize encoder system
 * Configures PCNT units for 4 wheels
 */
void encoder_init(void);

/**
 * Update encoder readings
 * Call this periodically to refresh encoder values
 */
void encoder_update(void);

/**
 * Get pulse count for specific encoder
 * @param encoder_id: Encoder ID (0-3)
 * @return: Current pulse count
 */
int32_t encoder_get_count(uint8_t encoder_id);

/**
 * Get velocity in m/s
 * @param encoder_id: Encoder ID (0-3)
 * @return: Velocity in m/s
 */
float encoder_get_velocity(uint8_t encoder_id);

/**
 * Get RPM for specific encoder
 * @param encoder_id: Encoder ID (0-3)
 * @return: RPM value
 */
float encoder_get_rpm(uint8_t encoder_id);

/**
 * Reset encoder count for specific encoder
 * @param encoder_id: Encoder ID (0-3)
 */
void encoder_reset(uint8_t encoder_id);

/**
 * Reset all encoders
 */
void encoder_reset_all(void);

#endif // ENCODER_H
