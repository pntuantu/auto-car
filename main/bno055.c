#include "bno055.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c_master.h"
#include "esp_log.h"

static const char *TAG = "BNO055";

#define BNO_I2C_PORT       I2C_NUM_0
#define BNO_I2C_SDA        8
#define BNO_I2C_SCL        9
#define BNO_I2C_FREQ_HZ    100000
#define BNO_I2C_TIMEOUT_MS 100
#define BNO055_ADDR        0x29

// Thanh ghi
#define BNO055_CHIP_ID_REG 0x00
#define BNO055_PAGE_ID_REG 0x07
#define BNO055_QUAT_W_LSB  0x20  // Bắt đầu vùng dữ liệu Quaternion (8 bytes)
#define BNO055_CALIB_STAT  0x35
#define BNO055_OPR_MODE    0x3D
#define BNO055_PWR_MODE    0x3E

// Cấu hình
#define BNO055_CHIP_ID     0xA0
#define BNO055_MODE_CONFIG 0x00
#define BNO055_MODE_NDOF   0x0C
#define BNO055_PWR_NORMAL  0x00
#define QUAT_SCALE_FACTOR  16384.0f // Theo datasheet BNO055 (2^14)

volatile float g_bno_quat_w = 1.0f;
volatile float g_bno_quat_x = 0.0f;
volatile float g_bno_quat_y = 0.0f;
volatile float g_bno_quat_z = 0.0f;

volatile uint8_t g_bno_calib_sys = 0;
volatile uint8_t g_bno_calib_gyro = 0;
volatile uint8_t g_bno_calib_accel = 0;
volatile uint8_t g_bno_calib_mag = 0;

static i2c_master_bus_handle_t s_i2c_bus = NULL;
static i2c_master_dev_handle_t s_bno_dev = NULL;

static esp_err_t bno055_write8(uint8_t reg, uint8_t value) {
    uint8_t data[2] = {reg, value};
    return i2c_master_transmit(s_bno_dev, data, sizeof(data), BNO_I2C_TIMEOUT_MS);
}

static esp_err_t bno055_read(uint8_t reg, uint8_t *data, size_t len) {
    return i2c_master_transmit_receive(s_bno_dev, &reg, 1, data, len, BNO_I2C_TIMEOUT_MS);
}

esp_err_t bno055_init(void) {
    if (s_bno_dev != NULL) return ESP_OK;

    i2c_master_bus_config_t bus_cfg = {
        .i2c_port = BNO_I2C_PORT,
        .sda_io_num = BNO_I2C_SDA,
        .scl_io_num = BNO_I2C_SCL,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_cfg, &s_i2c_bus));

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = BNO055_ADDR,
        .scl_speed_hz = BNO_I2C_FREQ_HZ,
    };
    ESP_ERROR_CHECK(i2c_master_bus_add_device(s_i2c_bus, &dev_cfg, &s_bno_dev));

    vTaskDelay(pdMS_TO_TICKS(500)); // Chờ IC khởi động

    uint8_t chip_id = 0;
    bno055_read(BNO055_CHIP_ID_REG, &chip_id, 1);
    if (chip_id != BNO055_CHIP_ID) {
        ESP_LOGE(TAG, "BNO055 not found! ID: 0x%02X", chip_id);
        return ESP_FAIL;
    }

    // Config Mode -> NDOF
    bno055_write8(BNO055_OPR_MODE, BNO055_MODE_CONFIG);
    vTaskDelay(pdMS_TO_TICKS(25));
    bno055_write8(BNO055_PAGE_ID_REG, 0x00);
    bno055_write8(BNO055_PWR_MODE, BNO055_PWR_NORMAL);
    vTaskDelay(pdMS_TO_TICKS(10));
    bno055_write8(BNO055_OPR_MODE, BNO055_MODE_NDOF);
    vTaskDelay(pdMS_TO_TICKS(25));

    ESP_LOGI(TAG, "BNO055 initialized in NDOF mode.");
    return ESP_OK;
}

void bno055_task(void *arg) {
    if (bno055_init() != ESP_OK) {
        vTaskDelete(NULL);
        return;
    }

    uint8_t buf[8];
    uint8_t calib;

    while (1) {
        // Đọc 8 bytes Quaternion (W, X, Y, Z mỗi trục 2 byte LSB/MSB)
        if (bno055_read(BNO055_QUAT_W_LSB, buf, 8) == ESP_OK) {
            int16_t rw = (int16_t)(buf[0] | (buf[1] << 8));
            int16_t rx = (int16_t)(buf[2] | (buf[3] << 8));
            int16_t ry = (int16_t)(buf[4] | (buf[5] << 8));
            int16_t rz = (int16_t)(buf[6] | (buf[7] << 8));

            g_bno_quat_w = (float)rw / QUAT_SCALE_FACTOR;
            g_bno_quat_x = (float)rx / QUAT_SCALE_FACTOR;
            g_bno_quat_y = (float)ry / QUAT_SCALE_FACTOR;
            g_bno_quat_z = (float)rz / QUAT_SCALE_FACTOR;
        }

        // Đọc trạng thái Calibration
        if (bno055_read(BNO055_CALIB_STAT, &calib, 1) == ESP_OK) {
            g_bno_calib_sys   = (calib >> 6) & 0x03;
            g_bno_calib_gyro  = (calib >> 4) & 0x03;
            g_bno_calib_accel = (calib >> 2) & 0x03;
            g_bno_calib_mag   = calib & 0x03;
        }

        vTaskDelay(pdMS_TO_TICKS(20)); // Đọc ở tần số 50Hz
    }
}