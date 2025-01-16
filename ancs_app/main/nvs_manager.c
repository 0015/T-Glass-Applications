#include "nvs_manager.h"
#include <esp_log.h>
#include <string.h>

#define TAG "[NVS Manager]"

esp_err_t nvs_manager_init(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_LOGW(TAG, "NVS partition truncated, erasing...");
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }

    if (err == ESP_OK)
    {
        ESP_LOGI(TAG, "NVS initialized successfully.");
    }
    else
    {
        ESP_LOGE(TAG, "Failed to initialize NVS: %s", esp_err_to_name(err));
    }
    return err;
}

esp_err_t nvs_manager_store_string(const char *key, const char *value)
{
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err == ESP_OK)
    {
        err = nvs_set_str(nvs_handle, key, value);
        if (err == ESP_OK)
        {
            nvs_commit(nvs_handle);
            ESP_LOGI(TAG, "String stored successfully under key '%s'.", key);
        }
        else
        {
            ESP_LOGE(TAG, "Failed to store string under key '%s': %s", key, esp_err_to_name(err));
        }
        nvs_close(nvs_handle);
    }
    else
    {
        ESP_LOGE(TAG, "Failed to open NVS: %s", esp_err_to_name(err));
    }
    return err;
}

esp_err_t nvs_manager_get_string(const char *key, char *buffer, size_t buffer_size)
{
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &nvs_handle);
    if (err == ESP_OK)
    {
        err = nvs_get_str(nvs_handle, key, buffer, &buffer_size);
        if (err == ESP_OK)
        {
            ESP_LOGI(TAG, "String retrieved from NVS using key '%s'.", key);
        }
        else
        {
            ESP_LOGW(TAG, "String not found in NVS under key '%s': %s", key, esp_err_to_name(err));
        }
        nvs_close(nvs_handle);
    }
    else
    {
        ESP_LOGE(TAG, "Failed to open NVS: %s", esp_err_to_name(err));
    }
    return err;
}

esp_err_t nvs_manager_erase_key(const char *key)
{
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err == ESP_OK)
    {
        err = nvs_erase_key(nvs_handle, key);
        if (err == ESP_OK)
        {
            nvs_commit(nvs_handle);
            ESP_LOGI(TAG, "Key '%s' erased successfully.", key);
        }
        else
        {
            ESP_LOGE(TAG, "Failed to erase key '%s': %s", key, esp_err_to_name(err));
        }
        nvs_close(nvs_handle);
    }
    else
    {
        ESP_LOGE(TAG, "Failed to open NVS: %s", esp_err_to_name(err));
    }
    return err;
}

esp_err_t nvs_manager_erase_all(void)
{
    esp_err_t err = nvs_flash_erase();
    if (err == ESP_OK)
    {
        ESP_LOGI(TAG, "NVS partition erased successfully.");
    }
    else
    {
        ESP_LOGE(TAG, "Failed to erase NVS partition: %s", esp_err_to_name(err));
    }
    return err;
}