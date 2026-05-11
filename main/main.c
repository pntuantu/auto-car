#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "motor.h"
#include "encoder.h"
#include "kinematics.h"
#include "bno055.h"
#include "uart_link.h"

static const char *TAG = "AUTO_CAR_MAIN";

// PID Control Parameters
typedef struct {
    float kp;
    float ki;
    float kd;
    float prev_error;
    float integral;
} pid_controller_t;

// Global PID controllers for each motor
pid_controller_t pid_motors[4] = {0};

// Task handles
TaskHandle_t imu_task_handle = NULL;
TaskHandle_t encoder_task_handle = NULL;
TaskHandle_t uart_task_handle = NULL;
TaskHandle_t control_task_handle = NULL;

/**
 * PID Controller Update
 */
float pid_update(pid_controller_t *pid, float error, float dt)
{
    pid->integral += error * dt;
    float derivative = (error - pid->prev_error) / dt;
    
    float output = pid->kp * error + pid->ki * pid->integral + pid->kd * derivative;
    
    pid->prev_error = error;
    
    return output;
}

/**
 * IMU Reading Task
 */
void imu_task(void *pvParameter)
{
    ESP_LOGI(TAG, "IMU Task started");
    bno055_init();
    
    while (1) {
        // Read BNO055 IMU data
        bno055_read_orientation();
        
        vTaskDelay(pdMS_TO_TICKS(50)); // 20 Hz
    }
}

/**
 * Encoder Reading Task
 */
void encoder_task(void *pvParameter)
{
    ESP_LOGI(TAG, "Encoder Task started");
    encoder_init();
    
    while (1) {
        // Read encoder values for all 4 wheels
        encoder_update();
        
        vTaskDelay(pdMS_TO_TICKS(20)); // 50 Hz
    }
}

/**
 * UART Communication Task
 */
void uart_task(void *pvParameter)
{
    ESP_LOGI(TAG, "UART Task started");
    uart_link_init();
    
    while (1) {
        // Receive commands from Raspberry Pi
        uart_link_receive();
        
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

/**
 * Main Control Loop (PID + Motor Control)
 */
void control_task(void *pvParameter)
{
    ESP_LOGI(TAG, "Control Task started");
    motor_init();
    
    // Initialize PID parameters
    for (int i = 0; i < 4; i++) {
        pid_motors[i].kp = 1.0f;
        pid_motors[i].ki = 0.1f;
        pid_motors[i].kd = 0.05f;
    }
    
    while (1) {
        // Get target velocities from UART
        float target_vx = 0.0f;
        float target_vy = 0.0f;
        float target_omega = 0.0f;
        
        // Get current velocities from kinematics
        float current_velocities[4] = {0};
        kinematics_update(target_vx, target_vy, target_omega);
        
        // PID Control Loop
        for (int i = 0; i < 4; i++) {
            float error = 0.0f; // target - current velocity
            float pwm_output = pid_update(&pid_motors[i], error, 0.01f);
            motor_set_pwm(i, pwm_output);
        }
        
        vTaskDelay(pdMS_TO_TICKS(10)); // 100 Hz
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "Auto-Car Firmware Starting...");
    
    // Create FreeRTOS tasks
    xTaskCreate(imu_task, "IMU_Task", 2048, NULL, 5, &imu_task_handle);
    xTaskCreate(encoder_task, "Encoder_Task", 2048, NULL, 5, &encoder_task_handle);
    xTaskCreate(uart_task, "UART_Task", 2048, NULL, 4, &uart_task_handle);
    xTaskCreate(control_task, "Control_Task", 4096, NULL, 6, &control_task_handle);
    
    ESP_LOGI(TAG, "All tasks created successfully");
}
