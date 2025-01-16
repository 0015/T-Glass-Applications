#pragma once

#include <esp_err.h>
#include <nvs_flash.h>

#define NVS_NAMESPACE "storage"

// Function declarations
esp_err_t nvs_manager_init(void);
esp_err_t nvs_manager_store_string(const char *key, const char *value);
esp_err_t nvs_manager_get_string(const char *key, char *buffer, size_t buffer_size);
esp_err_t nvs_manager_erase_key(const char *key);
esp_err_t nvs_manager_erase_all(void);