idf_component_register(
    SRCS "main.c" 
         "motor.c" 
         "encoder.c" 
         "kinematics.c" 
         "bno055.c" 
         "uart_link.c"
    INCLUDE_DIRS "."
    REQUIRES nvs_flash driver esp_driver_gptimer
)
