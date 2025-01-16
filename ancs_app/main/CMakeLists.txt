idf_component_register(SRCS "ancs_app.c" 
                       "nvs_manager.c" 
                       "ble_ancs.c" 
                       "battery_measurement.c" 
                       "main.c" 
                       "jd9613.c" 
                       "t_glass.c"
                       INCLUDE_DIRS "include"
                       PRIV_REQUIRES touch_element
                       REQUIRES nvs_flash bt)