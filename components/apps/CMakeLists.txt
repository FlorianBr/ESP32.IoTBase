idf_component_register(SRCS "commands.c"
                    INCLUDE_DIRS "."
                    REQUIRES mqtt json app_update esp_http_client
                    )
