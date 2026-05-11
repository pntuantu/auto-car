#ifndef KINEMATICS_H
#define KINEMATICS_H

#include <stdint.h>

/**
 * Kinematics Module
 * 
 * Implements Mecanum wheel kinematics for omni-directional robot
 * 
 * Mecanum wheel configuration:
 *   FL(45°)    FR(135°)
 *      *-------*
 *      |       |
 *      |       |
 *      *-------*
 *   BL(315°)   BR(225°)
 * 
 * Where angles indicate wheel roller orientations
 */

// Robot parameters
#define ROBOT_LENGTH    0.24f    // Distance between front and back (meters)
#define ROBOT_WIDTH     0.20f    // Distance between left and right (meters)
#define WHEEL_RADIUS    0.05f    // Wheel radius (meters)

// Mecanum wheel lever arms
#define LX (ROBOT_LENGTH / 2.0f)
#define LY (ROBOT_WIDTH / 2.0f)

/**
 * Velocity target structure
 */
typedef struct {
    float vx;      // Forward velocity (m/s)
    float vy;      // Lateral velocity (m/s)
    float omega;   // Angular velocity (rad/s)
} velocity_t;

/**
 * Initialize kinematics module
 */
void kinematics_init(void);

/**
 * Calculate wheel velocities from robot velocities
 * @param vx: Forward velocity (m/s)
 * @param vy: Lateral velocity (m/s)
 * @param omega: Angular velocity (rad/s)
 * @param wheel_velocities: Output array of 4 wheel velocities (m/s)
 */
void kinematics_forward(float vx, float vy, float omega, float wheel_velocities[4]);

/**
 * Calculate robot velocity from wheel encoder feedback
 * @param wheel_velocities: Input array of 4 wheel velocities (m/s)
 * @param velocity: Output velocity structure (vx, vy, omega)
 */
void kinematics_inverse(float wheel_velocities[4], velocity_t *velocity);

/**
 * Update kinematics with target velocities
 * @param vx: Target forward velocity (m/s)
 * @param vy: Target lateral velocity (m/s)
 * @param omega: Target angular velocity (rad/s)
 */
void kinematics_update(float vx, float vy, float omega);

/**
 * Get current robot velocity estimate
 * @return: Pointer to current velocity structure
 */
velocity_t* kinematics_get_velocity(void);

/**
 * Stop robot (zero all velocities)
 */
void kinematics_stop(void);

#endif // KINEMATICS_H
