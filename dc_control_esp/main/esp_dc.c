#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_rom_sys.h"

#define TAG "MOTOR_LCD"

// =========================
// MOTOR PINS
// =========================
#define ENA 4
#define IN1 5
#define IN2 6

#define ENB 15
#define IN3 16
#define IN4 17

static uint32_t speed = 150;

// =========================
// PWM CONTROL
// =========================
static void pwm_set(uint32_t duty_left, uint32_t duty_right)
{
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty_left);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);

    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, duty_right);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1);
}

// =========================
// LCD I2C PINS
// =========================
#define I2C_MASTER_NUM I2C_NUM_0
#define I2C_MASTER_SDA_IO 18
#define I2C_MASTER_SCL_IO 9
#define I2C_MASTER_FREQ_HZ 100000
#define LCD_ADDR 0x27

// =========================
// PCF8574 BIT MAPPING (ĐÃ SỬA CHUẨN)
// =========================
#define LCD_RS 0x01
#define LCD_RW 0x02
#define LCD_EN 0x04
#define LCD_BL 0x08

static uint8_t lcd_backlight = LCD_BL;

// =========================
// LCD LOW LEVEL
// =========================
static esp_err_t lcd_write_byte(uint8_t data)
{
    return i2c_master_write_to_device(
        I2C_MASTER_NUM,
        LCD_ADDR,
        &data,
        1,
        pdMS_TO_TICKS(100));
}

// Sửa hàm cấp xung Enable dài hơn một chút để I2C PCF8574 kịp xử lý
static void lcd_pulse_enable(uint8_t data)
{
    // data lúc này đã chứa LCD_RS, LCD_BL và 4 bit data. LCD_EN đang bằng 0.
    lcd_write_byte(data | LCD_EN); // Kéo EN lên 1
    esp_rom_delay_us(2);           // Chờ 2 micro-giây (cũ là 1us hơi sát giới hạn)
    lcd_write_byte(data & ~LCD_EN); // Kéo EN xuống 0
    esp_rom_delay_us(50);          // Thời gian để chip LCD thực thi lệnh thường
}

static void lcd_write4bits(uint8_t data_nibble, uint8_t mode)
{
    uint8_t data = (data_nibble & 0xF0) | mode | lcd_backlight;
    lcd_write_byte(data);
    lcd_pulse_enable(data);
}

static void lcd_send(uint8_t value, uint8_t mode)
{
    lcd_write4bits(value & 0xF0, mode);           // Gửi 4 bit cao trước
    lcd_write4bits((value << 4) & 0xF0, mode);    // Dịch 4 bit thấp lên và gửi
}

static void lcd_command(uint8_t cmd)
{
    lcd_send(cmd, 0);
    esp_rom_delay_us(50);
}

static void lcd_data(uint8_t data)
{
    lcd_send(data, LCD_RS);
    esp_rom_delay_us(50);
}

// Thay vTaskDelay bằng esp_rom_delay_us để đảm bảo chắc chắn đợi đủ 2ms
static void lcd_clear(void)
{
    lcd_command(0x01);
    esp_rom_delay_us(2000); // Bắt buộc phải > 1.52ms
}

static void lcd_set_cursor(uint8_t col, uint8_t row)
{
    static const uint8_t row_offsets[] = {0x00, 0x40, 0x14, 0x54};
    lcd_command(0x80 | (col + row_offsets[row]));
    esp_rom_delay_us(50);
}

static void lcd_print(const char *str)
{
    while (*str)
    {
        lcd_data((uint8_t)*str++);
        esp_rom_delay_us(50);
    }
}

static void lcd_print_line(uint8_t row, const char *text)
{
    char buf[17];
    memset(buf, ' ', 16);
    buf[16] = '\0';

    size_t len = strlen(text);
    if (len > 16)
        len = 16;
    memcpy(buf, text, len);

    lcd_set_cursor(0, row);
    lcd_print(buf);
}

// Thay toàn bộ quy trình Init bằng delay cứng để HD44780 không bị ngợp
static void lcd_init_display(void)
{
    vTaskDelay(pdMS_TO_TICKS(50)); // Chờ nguồn ổn định khi vừa bật (dùng vTaskDelay ở đây là OK vì 50ms đủ lớn)

    // Quy trình "đánh thức" chuẩn của Datasheet
    lcd_write4bits(0x30, 0);
    esp_rom_delay_us(5000); // Bắt buộc đợi > 4.1ms
    
    lcd_write4bits(0x30, 0);
    esp_rom_delay_us(200);  // Bắt buộc đợi > 100us
    
    lcd_write4bits(0x30, 0);
    esp_rom_delay_us(200);
    
    // Bắt đầu ép sang chế độ giao tiếp 4-bit
    lcd_write4bits(0x20, 0);
    esp_rom_delay_us(200);

    // Cấu hình các thông số
    lcd_command(0x28); // 4-bit, 2 dòng, font 5x8
    lcd_command(0x0C); // Bật màn hình, tắt con trỏ nhấp nháy
    lcd_command(0x06); // Tự động dịch con trỏ từ trái sang phải
    lcd_clear();       // Dọn sạch rác trên màn hình

    ESP_LOGI(TAG, "LCD 16x2 initialized with backlight ON");
}

// =========================
// LCD UI
// =========================
static void lcd_show_status(const char *mode)
{
    lcd_clear();
    vTaskDelay(pdMS_TO_TICKS(1));  // Giảm từ 2ms

    lcd_print_line(0, "ESP32-S3 Robot");
    lcd_print_line(1, mode);
}

// =========================
// MOTOR STATE LOG
// =========================
static void print_state(const char *state)
{
    int a1 = gpio_get_level(IN1);
    int a2 = gpio_get_level(IN2);
    int b1 = gpio_get_level(IN3);
    int b2 = gpio_get_level(IN4);

    ESP_LOGI(TAG, "[%s] IN1=%d IN2=%d IN3=%d IN4=%d", state, a1, a2, b1, b2);

    if (a1 == 1 && a2 == 0)
        ESP_LOGI(TAG, "LEFT  (OUT1/OUT2): FORWARD");
    else if (a1 == 0 && a2 == 1)
        ESP_LOGI(TAG, "LEFT  (OUT1/OUT2): BACKWARD");
    else if (a1 == 0 && a2 == 0)
        ESP_LOGI(TAG, "LEFT  (OUT1/OUT2): STOP");
    else
        ESP_LOGI(TAG, "LEFT  (OUT1/OUT2): BRAKE");

    if (b1 == 1 && b2 == 0)
        ESP_LOGI(TAG, "RIGHT (OUT3/OUT4): FORWARD");
    else if (b1 == 0 && b2 == 1)
        ESP_LOGI(TAG, "RIGHT (OUT3/OUT4): BACKWARD");
    else if (b1 == 0 && b2 == 0)
        ESP_LOGI(TAG, "RIGHT (OUT3/OUT4): STOP");
    else
        ESP_LOGI(TAG, "RIGHT (OUT3/OUT4): BRAKE");
}

// =========================
// MOTOR CONTROL
// =========================
static void move_forward(void)
{
    gpio_set_level(IN1, 1);
    gpio_set_level(IN2, 0);
    gpio_set_level(IN3, 1);
    gpio_set_level(IN4, 0);
    pwm_set(speed, speed);

    ESP_LOGI(TAG, "LCD -> FORWARD");
    lcd_show_status("FORWARD");
    print_state("FORWARD");
}

static void move_backward(void)
{
    gpio_set_level(IN1, 0);
    gpio_set_level(IN2, 1);
    gpio_set_level(IN3, 0);
    gpio_set_level(IN4, 1);
    pwm_set(speed, speed);

    ESP_LOGI(TAG, "LCD -> BACKWARD");
    lcd_show_status("BACKWARD");
    print_state("BACKWARD");
}

static void turn_left(void)
{
    gpio_set_level(IN1, 0);
    gpio_set_level(IN2, 1);
    gpio_set_level(IN3, 1);
    gpio_set_level(IN4, 0);
    pwm_set(speed, speed);

    ESP_LOGI(TAG, "LCD -> LEFT");
    lcd_show_status("TURN LEFT");
    print_state("LEFT");
}

static void turn_right(void)
{
    gpio_set_level(IN1, 1);
    gpio_set_level(IN2, 0);
    gpio_set_level(IN3, 0);
    gpio_set_level(IN4, 1);
    pwm_set(speed, speed);

    ESP_LOGI(TAG, "LCD -> RIGHT");
    lcd_show_status("TURN RIGHT");
    print_state("RIGHT");
}

static void stop_motor(void)
{
    gpio_set_level(IN1, 0);
    gpio_set_level(IN2, 0);
    gpio_set_level(IN3, 0);
    gpio_set_level(IN4, 0);
    pwm_set(0, 0);

    ESP_LOGI(TAG, "LCD -> STOP");
    lcd_show_status("STOP");
    print_state("STOP");
}

// =========================
// INIT
// =========================
static void motor_gpio_init(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << IN1) | (1ULL << IN2) | (1ULL << IN3) | (1ULL << IN4),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = 0,
        .pull_down_en = 0,
        .intr_type = GPIO_INTR_DISABLE};
    gpio_config(&io_conf);
}

static void motor_pwm_init(void)
{
    ledc_timer_config_t timer_conf = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_num = LEDC_TIMER_0,
        .duty_resolution = LEDC_TIMER_8_BIT,
        .freq_hz = 1000,
        .clk_cfg = LEDC_AUTO_CLK};
    ledc_timer_config(&timer_conf);

    ledc_channel_config_t left_ch = {
        .gpio_num = ENA,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = LEDC_TIMER_0,
        .duty = 0,
        .hpoint = 0};
    ledc_channel_config(&left_ch);

    ledc_channel_config_t right_ch = {
        .gpio_num = ENB,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_1,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = LEDC_TIMER_0,
        .duty = 0,
        .hpoint = 0};
    ledc_channel_config(&right_ch);
}

static void i2c_master_init(void)
{
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ};

    i2c_param_config(I2C_MASTER_NUM, &conf);
    i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0);
}

// =========================
// MAIN
// =========================
void app_main(void)
{
    // Setup UART stdio (enable stdin/stdout over serial)
    esp_log_level_set("*", ESP_LOG_INFO);
    
    motor_gpio_init();
    motor_pwm_init();
    i2c_master_init();
    lcd_init_display();

    // Unbuffered I/O
    setvbuf(stdin, NULL, _IONBF, 0);
    setvbuf(stdout, NULL, _IONBF, 0);
    
    ESP_LOGI(TAG, "System ready for serial input");

    vTaskDelay(pdMS_TO_TICKS(2000));
    stop_motor();

    lcd_print_line(0, "ESP32-S3 Robot");
    lcd_print_line(1, "READY F/B/L/R/S");

    ESP_LOGI(TAG, "READY: F B L R S");
    ESP_LOGI(TAG, "Type command then press Enter");

    while (1)
    {
        int ch = getchar();

        if (ch == EOF)
        {
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }

        ESP_LOGI(TAG, "Received char: %c (%d)", ch, ch);

        switch (ch)
        {
        case 'F':
        case 'f':
            move_forward();
            break;
        case 'B':
        case 'b':
            move_backward();
            break;
        case 'L':
        case 'l':
            turn_left();
            break;
        case 'R':
        case 'r':
            turn_right();
            break;
        case 'S':
        case 's':
            stop_motor();
            break;
        case '\n':
        case '\r':
            // Skip newline without LCD update
            break;
        default:
            ESP_LOGW(TAG, "Invalid command: %c", ch);
            break;
        }
        
        // Small yield to other tasks
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}