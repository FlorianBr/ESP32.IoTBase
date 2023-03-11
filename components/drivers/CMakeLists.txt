idf_component_register(SRCS "wifi.c" "ntp.c" "mqtt.c"
                    INCLUDE_DIRS "."
                   REQUIRES nvs_flash esp_wifi mqtt
                    )
