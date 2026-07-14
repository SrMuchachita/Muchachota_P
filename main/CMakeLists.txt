file(GLOB UI_SOURCES "ui/*.c")

idf_component_register(SRCS "main.c"
                     ${UI_SOURCES}
                    INCLUDE_DIRS "."
                    REQUIRES esp_wifi esp_netif esp_event ota_http esp_hosted
                             esp_driver_ledc esp_driver_i2c esp_driver_uart
                             nvs_flash esp_timer)
