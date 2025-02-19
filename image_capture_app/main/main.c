#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "nvs_manager.h"
#include "t_glass.h"
#include "ble_server.h"

#define TAG "[Glass Main]"

#define IMAGE_MAX_SIZE      31752       // 126x126x2 (Width x Height x 2 bytes)
#define DESIRED_MTU_SIZE    215         // MTU is determined by the Flutter BT Package

static uint8_t *image_buffer = NULL;
static size_t received_bytes = 0;

typedef struct {
    uint8_t data[DESIRED_MTU_SIZE]; 
    size_t length;
} ble_data_t;

static QueueHandle_t ble_queue;

void ble_disconnected(){
    received_bytes = 0;
}

void ble_receive_image_chunk(uint8_t *data, size_t length) {
    if (ble_queue) {
        ble_data_t ble_data;
        memcpy(ble_data.data, data, length);
        ble_data.length = length;
        xQueueSend(ble_queue, &ble_data, portMAX_DELAY);
    }
}

// Function to process BLE image chunks in a separate task
void ble_process_task(void *arg) {
    ble_data_t received_data;
    while (1) {
        if (xQueueReceive(ble_queue, &received_data, portMAX_DELAY)) {
            if (image_buffer == NULL) {
                ESP_LOGE("BLE", "Image buffer is NULL! PSRAM allocation failed?");
                continue;
            }

            if ((received_bytes + received_data.length) <= IMAGE_MAX_SIZE) {
                memcpy(&image_buffer[received_bytes], received_data.data, received_data.length);
                received_bytes += received_data.length;
                ESP_LOGI("BLE", "Received %d/%d bytes", received_bytes, IMAGE_MAX_SIZE);
            } else {
                ESP_LOGE("BLE", "Buffer overflow! Resetting.");
                received_bytes = 0;
            }

            if (received_bytes >= IMAGE_MAX_SIZE) {
                ESP_LOGI("BLE", "Image complete, updating LVGL.");
                update_canvas_with_rgb565(image_buffer, IMAGE_MAX_SIZE);
                received_bytes = 0;
            }
        }
    }
}

// Initialize BLE queue
void ble_receive_init() {
    ble_queue = xQueueCreate(10, sizeof(ble_data_t));

    // Allocate in PSRAM
    image_buffer = (uint8_t *)heap_caps_malloc(IMAGE_MAX_SIZE, MALLOC_CAP_SPIRAM);
    if (!image_buffer) {
        ESP_LOGE("BLE", "Failed to allocate image buffer in PSRAM!");
    } else {
        ESP_LOGI("BLE", "Image buffer allocated in PSRAM.");
    }

    xTaskCreatePinnedToCore(ble_process_task, "BLE_Proc_Task", 4096, NULL, 2, NULL, 1);
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

    ble_receive_init();
    ble_server_init();
}
