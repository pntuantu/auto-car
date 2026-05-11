#ifndef MOTOR_H
#define MOTOR_H

#include <stdint.h>

/**
 * Motor Control Module
 * 
 * Manages PWM signals and H-Bridge motor control for 4 wheels
 * Motor connections:
 *   - Motor 0: Front-Left
 *   - Motor 1: Front-Right  
 *   - Motor 2: Back-Left
 *   - Motor 3: Back-Right
 */

// Motor PWM pins (ESP32 GPIO)
#define MOTOR_FL_PWM_PIN    12
#define MOTOR_FR_PWM_PIN    13
#define MOTOR_BL_PWM_PIN    14
#define MOTOR_BR_PWM_PIN    15

// Motor Direction pins (H-Bridge control)
#define MOTOR_FL_DIR_PIN    16
#define MOTOR_FR_DIR_PIN    17
#define MOTOR_BL_DIR_PIN    18
#define MOTOR_BR_DIR_PIN    19

// Motor specifications
#define PWM_FREQUENCY       5000  // 5 kHz
#define PWM_RESOLUTION      10    // 10-bit (0-1023)
#define MAX_PWM_VALUE       1023

/**
 * Initialize motor control system
 * Configures GPIO pins and PWM channels
 */
void motor_init(void);

/**
 * Set motor PWM duty cycle
 * @param motor_id: Motor ID (0-3)
 * @param pwm_value: PWM duty cycle (-1023 to 1023, negative = reverse)
 */
void motor_set_pwm(uint8_t motor_id, int16_t pwm_value);

/**
 * Set motor velocity in m/s
 * @param motor_id: Motor ID (0-3)
 * @param velocity: Target velocity in m/s
 */
void motor_set_velocity(uint8_t motor_id, float velocity);

/**
 * Stop all motors
 */
void motor_stop_all(void);

/**
 * Stop specific motor
 * @param motor_id: Motor ID (0-3)
 */
void motor_stop(uint8_t motor_id);

#endif // MOTOR_H
