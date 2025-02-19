idf_component_register(SRCS "ble_server.c" "nvs_manager.c" "rtc_pcf85063.c" "battery_measurement.c" "main.c" "jd9613.c" "t_glass.c" "battery_measurement.c"
                        INCLUDE_DIRS "include"
                        PRIV_REQUIRES touch_element
                        REQUIRES nvs_flash bt)
