#include "bno055.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include <math.h>

static const char *TAG = "BNO055_IMU";

// BNO055 Register Map
#define BNO055_REG_ID               0x00
#define BNO055_REG_DID              0x00
#define BNO055_REG_STAT             0x39
#define BNO055_REG_OPR_MODE         0x3D
#define BNO055_REG_PWR_MODE         0x3E
#define BNO055_REG_SYS_TRIGGER      0x3F

#define BNO055_REG_QUAT_W           0x20
#define BNO055_REG_QUAT_X           0x22
#define BNO055_REG_QUAT_Y           0x24
#define BNO055_REG_QUAT_Z           0x26

#define BNO055_REG_EULER_H          0x1A
#define BNO055_REG_EULER_R          0x1C
#define BNO055_REG_EULER_P          0x1E

#define BNO055_REG_ACC_X            0x08
#define BNO055_REG_GYR_X            0x14
#define BNO055_REG_MAG_X            0x0E

static imu_data_t imu_data = {0};

static esp_err_t bno055_write_byte(uint8_t reg, uint8_t value)
{
    uint8_t data[2] = {reg, value};
    return i2c_master_write_to_device(BNO055_I2C_PORT, BNO055_I2C_ADDR, data, 2, pdMS_TO_TICKS(100));
}

static esp_err_t bno055_read_bytes(uint8_t reg, uint8_t *data, size_t len)
{
    return i2c_master_write_read_device(BNO055_I2C_PORT, BNO055_I2C_ADDR, 
                                        &reg, 1, data, len, pdMS_TO_TICKS(100));
}

static int16_t bno055_read_int16(uint8_t reg)
{
    uint8_t data[2];
    bno055_read_bytes(reg, data, 2);
    return (int16_t)((data[1] << 8) | data[0]);
}

void bno055_init(void)
{
    ESP_LOGI(TAG, "Initializing BNO055 IMU");
    
    // Configure I2C
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = BNO055_I2C_SDA_PIN,
        .scl_io_num = BNO055_I2C_SCL_PIN,
        .master.clk_speed = BNO055_I2C_FREQ,
        .clk_flags = I2C_SCLK_SRC_FLAG_FOR_NOMAL,
    };
    
    i2c_param_config(BNO055_I2C_PORT, &conf);
    i2c_driver_install(BNO055_I2C_PORT, conf.mode, 0, 0, 0);
    
    // Reset BNO055
    bno055_write_byte(BNO055_REG_SYS_TRIGGER, 0x20);
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    // Check device ID
    uint8_t chip_id;
    bno055_read_bytes(BNO055_REG_ID, &chip_id, 1);
    if (chip_id != 0xA0) {
        ESP_LOGE(TAG, "BNO055 not found! Chip ID: 0x%02X", chip_id);
        return;
    }
    
    // Set operation mode to NDOF
    bno055_write_byte(BNO055_REG_OPR_MODE, BNO055_MODE_NDOF);
    vTaskDelay(pdMS_TO_TICKS(100));
    
    ESP_LOGI(TAG, "BNO055 initialized successfully");
}

imu_data_t* bno055_read_orientation(void)
{
    // Read Quaternion (Q=1/16384)
    int16_t qw = bno055_read_int16(BNO055_REG_QUAT_W);
    int16_t qx = bno055_read_int16(BNO055_REG_QUAT_X);
    int16_t qy = bno055_read_int16(BNO055_REG_QUAT_Y);
    int16_t qz = bno055_read_int16(BNO055_REG_QUAT_Z);
    
    imu_data.quaternion.w = qw / 16384.0f;
    imu_data.quaternion.x = qx / 16384.0f;
    imu_data.quaternion.y = qy / 16384.0f;
    imu_data.quaternion.z = qz / 16384.0f;
    
    // Read Euler angles (in 1/16 degree)
    int16_t h = bno055_read_int16(BNO055_REG_EULER_H);
    int16_t r = bno055_read_int16(BNO055_REG_EULER_R);
    int16_t p = bno055_read_int16(BNO055_REG_EULER_P);
    
    imu_data.euler.yaw = h / 16.0f;
    imu_data.euler.roll = r / 16.0f;
    imu_data.euler.pitch = p / 16.0f;
    
    // Convert to radians
    imu_data.euler.yaw *= M_PI / 180.0f;
    imu_data.euler.roll *= M_PI / 180.0f;
    imu_data.euler.pitch *= M_PI / 180.0f;
    
    return &imu_data;
}

vector_t* bno055_read_acceleration(void)
{
    imu_data.acceleration.x = (float)bno055_read_int16(BNO055_REG_ACC_X) / 100.0f;
    imu_data.acceleration.y = (float)bno055_read_int16(BNO055_REG_ACC_X + 2) / 100.0f;
    imu_data.acceleration.z = (float)bno055_read_int16(BNO055_REG_ACC_X + 4) / 100.0f;
    
    return &imu_data.acceleration;
}

vector_t* bno055_read_gyroscope(void)
{
    imu_data.gyroscope.x = (float)bno055_read_int16(BNO055_REG_GYR_X) / 900.0f;
    imu_data.gyroscope.y = (float)bno055_read_int16(BNO055_REG_GYR_X + 2) / 900.0f;
    imu_data.gyroscope.z = (float)bno055_read_int16(BNO055_REG_GYR_X + 4) / 900.0f;
    
    return &imu_data.gyroscope;
}

vector_t* bno055_read_magnetometer(void)
{
    imu_data.magnetometer.x = (float)bno055_read_int16(BNO055_REG_MAG_X) / 16.0f;
    imu_data.magnetometer.y = (float)bno055_read_int16(BNO055_REG_MAG_X + 2) / 16.0f;
    imu_data.magnetometer.z = (float)bno055_read_int16(BNO055_REG_MAG_X + 4) / 16.0f;
    
    return &imu_data.magnetometer;
}

imu_data_t* bno055_get_data(void)
{
    return &imu_data;
}

uint8_t bno055_get_calibration_status(void)
{
    uint8_t status;
    bno055_read_bytes(BNO055_REG_STAT, &status, 1);
    return status;
}

void bno055_reset(void)
{
    bno055_write_byte(BNO055_REG_SYS_TRIGGER, 0x20);
    vTaskDelay(pdMS_TO_TICKS(1000));
    bno055_init();
}
