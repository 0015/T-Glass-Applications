#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "nvs_manager.h"
#include "t_glass.h"
#include "ancs_app.h"

#define TAG "[Glass Main]"

void notification_received_callback(NotificationAttributes *notification)
{
    ESP_LOGI(TAG, "New Notification Received:");
    ESP_LOGI(TAG, "Index: %d", (int)notification_index);
    ESP_LOGI(TAG, "UID: %d", (int)notification->NotificationUID);
    ESP_LOGI(TAG, "Identifier: %s", notification->Identifier);
    ESP_LOGI(TAG, "Title: %s", notification->Title);
    ESP_LOGI(TAG, "Subtitle: %s", notification->Subtitle);
    ESP_LOGI(TAG, "Message: %s", notification->Message);

    add_tile_view(notification_index, notification);
}

void app_main(void)
{
    ESP_LOGE(TAG, "App Started!");

    // Initialize NVS
    if (nvs_manager_init() != ESP_OK)
    {
        ESP_LOGE(TAG, "[Err] Failed to initialize NVS.");
        return;
    }

    if (init_tglass() != ESP_OK)
    {
        ESP_LOGE(TAG, "[Err] T-Glass Init Fail");
        abort();
    }

    ESP_LOGI(TAG, "[Pass] T-Glass Init");

    ancs_app(notification_received_callback);
}
