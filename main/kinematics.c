#include "kinematics.h"
#include "encoder.h"
#include "esp_log.h"
#include <math.h>

static const char *TAG = "KINEMATICS_MODULE";

static velocity_t current_velocity = {0, 0, 0};
static velocity_t target_velocity = {0, 0, 0};

void kinematics_init(void)
{
    ESP_LOGI(TAG, "Initializing kinematics module");
    current_velocity.vx = 0.0f;
    current_velocity.vy = 0.0f;
    current_velocity.omega = 0.0f;
}

void kinematics_forward(float vx, float vy, float omega, float wheel_velocities[4])
{
    /**
     * Mecanum wheel kinematic equation:
     * 
     * [v_fl]   [ 1  1  -(Lx+Ly)]   [vx    ]
     * [v_fr] = [ 1 -1   (Lx+Ly)] * [vy    ]
     * [v_bl]   [ 1  1   (Lx+Ly)]   [omega ]
     * [v_br]   [ 1 -1  -(Lx+Ly)]
     * 
     */
    
    float k = Lx + Ly;
    
    wheel_velocities[0] = vx + vy - omega * k;  // FL
    wheel_velocities[1] = vx - vy + omega * k;  // FR
    wheel_velocities[2] = vx + vy + omega * k;  // BL
    wheel_velocities[3] = vx - vy - omega * k;  // BR
}

void kinematics_inverse(float wheel_velocities[4], velocity_t *velocity)
{
    /**
     * Inverse kinematics (least squares solution)
     * [vx, vy, omega]^T = (M^T*M)^-1 * M^T * [v_fl, v_fr, v_bl, v_br]^T
     */
    
    float k = Lx + Ly;
    float inv4 = 0.25f;
    
    velocity->vx = (wheel_velocities[0] + wheel_velocities[1] + 
                    wheel_velocities[2] + wheel_velocities[3]) * inv4;
    
    velocity->vy = (wheel_velocities[0] - wheel_velocities[1] + 
                    wheel_velocities[2] - wheel_velocities[3]) * inv4;
    
    velocity->omega = (-wheel_velocities[0] + wheel_velocities[1] + 
                       wheel_velocities[2] - wheel_velocities[3]) / (4.0f * k);
}

void kinematics_update(float vx, float vy, float omega)
{
    // Store target velocities
    target_velocity.vx = vx;
    target_velocity.vy = vy;
    target_velocity.omega = omega;
    
    // Calculate wheel velocities from target
    float wheel_velocities[4];
    kinematics_forward(vx, vy, omega, wheel_velocities);
    
    // Update current velocities from encoders
    float encoder_velocities[4];
    for (int i = 0; i < 4; i++) {
        encoder_velocities[i] = encoder_get_velocity(i);
    }
    
    // Estimate robot velocity
    kinematics_inverse(encoder_velocities, &current_velocity);
    
    ESP_LOGD(TAG, "Target: vx=%.2f, vy=%.2f, w=%.2f", vx, vy, omega);
    ESP_LOGD(TAG, "Current: vx=%.2f, vy=%.2f, w=%.2f", 
             current_velocity.vx, current_velocity.vy, current_velocity.omega);
}

velocity_t* kinematics_get_velocity(void)
{
    return &current_velocity;
}

void kinematics_stop(void)
{
    kinematics_update(0.0f, 0.0f, 0.0f);
}
