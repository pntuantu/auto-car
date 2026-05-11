#ifndef UART_LINK_H
#define UART_LINK_H

#include <stdint.h>
#include <stddef.h>

/**
 * UART Communication Module
 * 
 * Binary protocol for communication with Raspberry Pi
 * Uses UART1 for high-speed bidirectional communication
 * 
 * UART Configuration:
 *   - Port: UART1
 *   - Baudrate: 115200 bps
 *   - TX: GPIO17
 *   - RX: GPIO16
 *   - Data bits: 8
 *   - Stop bits: 1
 *   - Parity: None
 */

#define UART_LINK_PORT          UART_NUM_1
#define UART_LINK_TX_PIN        17
#define UART_LINK_RX_PIN        16
#define UART_LINK_BAUDRATE      115200
#define UART_LINK_BUFFER_SIZE   1024

// Protocol Frame Format
#define FRAME_START_BYTE        0xAA
#define FRAME_END_BYTE          0xCC
#define FRAME_MAX_LENGTH        64

// Message Types
#define MSG_TYPE_VELOCITY       0x01  // Rx: Set velocity command
#define MSG_TYPE_IMU_DATA       0x02  // Tx: Send IMU data
#define MSG_TYPE_ENCODER_DATA   0x03  // Tx: Send encoder data
#define MSG_TYPE_TELEMETRY      0x04  // Tx: Send telemetry
#define MSG_TYPE_HEARTBEAT      0x05  // Rx/Tx: Keep-alive
#define MSG_TYPE_EMERGENCY_STOP 0x06  // Rx: Emergency stop

/**
 * Velocity command structure
 */
typedef struct {
    float vx;      // Forward velocity (m/s)
    float vy;      // Lateral velocity (m/s)
    float omega;   // Angular velocity (rad/s)
} velocity_command_t;

/**
 * Telemetry data structure
 */
typedef struct {
    float battery_voltage;
    float current_draw;
    uint8_t system_status;
    uint8_t error_code;
} telemetry_t;

/**
 * Initialize UART communication
 */
void uart_link_init(void);

/**
 * Send velocity data to Pi
 * @param vx: Forward velocity (m/s)
 * @param vy: Lateral velocity (m/s)
 * @param omega: Angular velocity (rad/s)
 */
void uart_link_send_velocity(float vx, float vy, float omega);

/**
 * Send IMU data to Pi
 * @param qw, qx, qy, qz: Quaternion components
 * @param roll, pitch, yaw: Euler angles in radians
 */
void uart_link_send_imu(float qw, float qx, float qy, float qz,
                        float roll, float pitch, float yaw);

/**
 * Send encoder data to Pi
 * @param encoder_data: Array of 4 encoder velocity values (m/s)
 */
void uart_link_send_encoders(float encoder_data[4]);

/**
 * Send telemetry data to Pi
 * @param telemetry: Telemetry structure
 */
void uart_link_send_telemetry(const telemetry_t *telemetry);

/**
 * Receive command from Pi
 * @return: Message type received, 0 if no message
 */
uint8_t uart_link_receive(void);

/**
 * Get last velocity command
 * @return: Pointer to velocity command structure
 */
velocity_command_t* uart_link_get_velocity_command(void);

/**
 * Check connection status with Pi
 * @return: 1 if connected, 0 if timeout
 */
uint8_t uart_link_is_connected(void);

#endif // UART_LINK_H
