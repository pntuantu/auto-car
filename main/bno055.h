#ifndef BNO055_H
#define BNO055_H

#include <stdint.h>

/**
 * BNO055 IMU Module
 * 
 * 9-axis IMU (Accelerometer, Gyroscope, Magnetometer)
 * Communicates via I2C interface
 * 
 * I2C Configuration:
 *   - Address: 0x28 or 0x29 (configurable via COM3)
 *   - Speed: 400 kHz
 *   - SDA: GPIO21
 *   - SCL: GPIO22
 */

// BNO055 I2C Configuration
#define BNO055_I2C_ADDR     0x28
#define BNO055_I2C_PORT     I2C_NUM_0
#define BNO055_I2C_SDA_PIN  21
#define BNO055_I2C_SCL_PIN  22
#define BNO055_I2C_FREQ     400000

// BNO055 Operation Modes
#define BNO055_MODE_IMU     0x08
#define BNO055_MODE_NDOF    0x0C
#define BNO055_MODE_NDOF_FMC_OFF 0x0B

/**
 * Quaternion data structure
 */
typedef struct {
    float w;
    float x;
    float y;
    float z;
} quaternion_t;

/**
 * Euler angles data structure
 */
typedef struct {
    float roll;
    float pitch;
    float yaw;
} euler_t;

/**
 * Vector data structure
 */
typedef struct {
    float x;
    float y;
    float z;
} vector_t;

/**
 * IMU data structure
 */
typedef struct {
    quaternion_t quaternion;
    euler_t euler;
    vector_t acceleration;
    vector_t gyroscope;
    vector_t magnetometer;
    float temperature;
    uint8_t calibration_status;
} imu_data_t;

/**
 * Initialize BNO055 IMU
 * Configures I2C and sets operation mode to NDOF
 */
void bno055_init(void);

/**
 * Read orientation (Quaternion and Euler angles)
 * @return: Pointer to IMU data structure
 */
imu_data_t* bno055_read_orientation(void);

/**
 * Read acceleration data
 * @return: Pointer to acceleration vector
 */
vector_t* bno055_read_acceleration(void);

/**
 * Read gyroscope data
 * @return: Pointer to angular velocity vector
 */
vector_t* bno055_read_gyroscope(void);

/**
 * Read magnetometer data
 * @return: Pointer to magnetic field vector
 */
vector_t* bno055_read_magnetometer(void);

/**
 * Get current IMU data
 * @return: Pointer to IMU data structure
 */
imu_data_t* bno055_get_data(void);

/**
 * Get calibration status
 * @return: Calibration status (0-255)
 */
uint8_t bno055_get_calibration_status(void);

/**
 * Reset IMU
 */
void bno055_reset(void);

#endif // BNO055_H
