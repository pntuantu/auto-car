// bno055.c
#include "bno055.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/i2c_master.h"
#include "driver/gpio.h"

#include "esp_log.h"

static const char *TAG = "BNO055";

#define BNO_I2C_PORT I2C_NUM_0
#define BNO_I2C_SDA GPIO_NUM_8
#define BNO_I2C_SCL GPIO_NUM_9
#define BNO_I2C_FREQ_HZ 100000
#define BNO_I2C_TIMEOUT_MS 100

#define BNO055_ADDR 0x29

#define BNO055_CHIP_ID_REG 0x00
#define BNO055_PAGE_ID_REG 0x07
#define BNO055_EULER_H_LSB 0x1A
#define BNO055_CALIB_STAT 0x35
#define BNO055_UNIT_SEL 0x3B
#define BNO055_OPR_MODE 0x3D
#define BNO055_PWR_MODE 0x3E
#define BNO055_SYS_TRIGGER 0x3F

#define BNO055_CHIP_ID 0xA0
#define BNO055_MODE_CONFIG 0x00
#define BNO055_MODE_NDOF 0x0C
#define BNO055_PWR_NORMAL 0x00

volatile float g_bno_heading_deg = 0.0f;
volatile float g_bno_roll_deg = 0.0f;
volatile float g_bno_pitch_deg = 0.0f;

volatile uint8_t g_bno_calib_sys = 0;
volatile uint8_t g_bno_calib_gyro = 0;
volatile uint8_t g_bno_calib_accel = 0;
volatile uint8_t g_bno_calib_mag = 0;

static i2c_master_bus_handle_t s_i2c_bus = NULL;
static i2c_master_dev_handle_t s_bno_dev = NULL;

static esp_err_t bno055_write8(uint8_t reg, uint8_t value)
{
    uint8_t data[2] = {reg, value};
    return i2c_master_transmit(s_bno_dev, data, sizeof(data), BNO_I2C_TIMEOUT_MS);
}

static esp_err_t bno055_read(uint8_t reg, uint8_t *data, size_t len)
{
    return i2c_master_transmit_receive(
        s_bno_dev,
        &reg,
        1,
        data,
        len,
        BNO_I2C_TIMEOUT_MS);
}

static int16_t bno055_to_s16(uint8_t lsb, uint8_t msb)
{
    return (int16_t)((uint16_t)lsb | ((uint16_t)msb << 8));
}

esp_err_t bno055_init(void)
{
    if (s_bno_dev != NULL)
    {
        return ESP_OK;
    }

    i2c_master_bus_config_t bus_cfg = {
        .i2c_port = BNO_I2C_PORT,
        .sda_io_num = BNO_I2C_SDA,
        .scl_io_num = BNO_I2C_SCL,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };

    esp_err_t ret = i2c_new_master_bus(&bus_cfg, &s_i2c_bus);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to create I2C bus: %s", esp_err_to_name(ret));
        return ret;
    }

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = BNO055_ADDR,
        .scl_speed_hz = BNO_I2C_FREQ_HZ,
    };

    ret = i2c_master_bus_add_device(s_i2c_bus, &dev_cfg, &s_bno_dev);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to add BNO055 device: %s", esp_err_to_name(ret));
        return ret;
    }

    vTaskDelay(pdMS_TO_TICKS(700));

    uint8_t chip_id = 0;
    ret = bno055_read(BNO055_CHIP_ID_REG, &chip_id, 1);

    if (ret != ESP_OK || chip_id != BNO055_CHIP_ID)
    {
        ESP_LOGW(TAG, "First read failed: ret=%s id=0x%02X",
                 esp_err_to_name(ret), chip_id);

        vTaskDelay(pdMS_TO_TICKS(500));
        ret = bno055_read(BNO055_CHIP_ID_REG, &chip_id, 1);

        if (ret != ESP_OK || chip_id != BNO055_CHIP_ID)
        {
            ESP_LOGE(TAG, "BNO055 not found: ret=%s id=0x%02X",
                     esp_err_to_name(ret), chip_id);
            return ESP_FAIL;
        }
    }

    ESP_LOGI(TAG, "BNO055 detected, CHIP_ID=0x%02X", chip_id);

    bno055_write8(BNO055_OPR_MODE, BNO055_MODE_CONFIG);
    vTaskDelay(pdMS_TO_TICKS(25));

    bno055_write8(BNO055_PAGE_ID_REG, 0x00);
    bno055_write8(BNO055_PWR_MODE, BNO055_PWR_NORMAL);
    vTaskDelay(pdMS_TO_TICKS(10));

    bno055_write8(BNO055_UNIT_SEL, 0x00);
    vTaskDelay(pdMS_TO_TICKS(10));

    bno055_write8(BNO055_SYS_TRIGGER, 0x80);
    vTaskDelay(pdMS_TO_TICKS(10));

    bno055_write8(BNO055_OPR_MODE, BNO055_MODE_NDOF);
    vTaskDelay(pdMS_TO_TICKS(25));

    ESP_LOGI(TAG, "BNO055 initialized in NDOF mode");
    return ESP_OK;
}

esp_err_t bno055_read_euler(float *heading_deg, float *roll_deg, float *pitch_deg)
{
    uint8_t buf[6] = {0};

    esp_err_t ret = bno055_read(BNO055_EULER_H_LSB, buf, sizeof(buf));
    if (ret != ESP_OK)
    {
        return ret;
    }

    int16_t heading_raw = bno055_to_s16(buf[0], buf[1]);
    int16_t roll_raw = bno055_to_s16(buf[2], buf[3]);
    int16_t pitch_raw = bno055_to_s16(buf[4], buf[5]);

    if (heading_deg)
    {
        *heading_deg = heading_raw / 16.0f;
    }

    if (roll_deg)
    {
        *roll_deg = roll_raw / 16.0f;
    }

    if (pitch_deg)
    {
        *pitch_deg = pitch_raw / 16.0f;
    }

    return ESP_OK;
}

esp_err_t bno055_read_calibration(uint8_t *sys, uint8_t *gyro, uint8_t *accel, uint8_t *mag)
{
    uint8_t calib = 0;

    esp_err_t ret = bno055_read(BNO055_CALIB_STAT, &calib, 1);
    if (ret != ESP_OK)
    {
        return ret;
    }

    if (sys)
    {
        *sys = (calib >> 6) & 0x03;
    }

    if (gyro)
    {
        *gyro = (calib >> 4) & 0x03;
    }

    if (accel)
    {
        *accel = (calib >> 2) & 0x03;
    }

    if (mag)
    {
        *mag = calib & 0x03;
    }

    return ESP_OK;
}

void bno055_task(void *arg)
{
    if (bno055_init() != ESP_OK)
    {
        ESP_LOGE(TAG, "BNO055 init failed");
        vTaskDelete(NULL);
        return;
    }

    int log_count = 0;

    while (1)
    {
        float heading = 0.0f;
        float roll = 0.0f;
        float pitch = 0.0f;

        uint8_t sys = 0;
        uint8_t gyro = 0;
        uint8_t accel = 0;
        uint8_t mag = 0;

        if (bno055_read_euler(&heading, &roll, &pitch) == ESP_OK)
        {
            g_bno_heading_deg = heading;
            g_bno_roll_deg = roll;
            g_bno_pitch_deg = pitch;
        }

        if (bno055_read_calibration(&sys, &gyro, &accel, &mag) == ESP_OK)
        {
            g_bno_calib_sys = sys;
            g_bno_calib_gyro = gyro;
            g_bno_calib_accel = accel;
            g_bno_calib_mag = mag;
        }

        log_count++;
        if (log_count >= 20)
        {
            log_count = 0;

            ESP_LOGI(TAG,
                     "Heading: %.2f Roll: %.2f Pitch: %.2f | Calib SYS:%u G:%u A:%u M:%u",
                     g_bno_heading_deg,
                     g_bno_roll_deg,
                     g_bno_pitch_deg,
                     g_bno_calib_sys,
                     g_bno_calib_gyro,
                     g_bno_calib_accel,
                     g_bno_calib_mag);
        }

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}