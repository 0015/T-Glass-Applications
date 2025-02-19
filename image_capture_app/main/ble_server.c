#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_bt.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_bt_defs.h"
#include "esp_bt_main.h"
#include "t_glass.h"

#define TAG "[BLE_SERVER]"

#define DEVICE_NAME     "T-Glass"
#define SERVICE_UUID    0x00FF
#define CHAR_UUID       0xFF01

// External function defined in main.c
extern void ble_receive_image_chunk(uint8_t *data, size_t length);
extern void ble_disconnected();

static uint8_t char_value = 0;
static esp_gatt_char_prop_t char_property = ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_WRITE_NR;
static esp_attr_value_t gatts_char_val = {
    .attr_max_len = 20,
    .attr_len = sizeof(char_value),
    .attr_value = &char_value,
};

static uint8_t adv_service_uuid128[16] = {
    0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80,
    0x00, 0x10, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00};

static esp_ble_adv_data_t adv_data = {
    .set_scan_rsp = false,
    .include_name = true,
    .include_txpower = true,
    .min_interval = 0x20,
    .max_interval = 0x40,
    .appearance = 0x00,
    .manufacturer_len = 0,
    .p_manufacturer_data = NULL,
    .service_data_len = 0,
    .p_service_data = NULL,
    .service_uuid_len = sizeof(adv_service_uuid128),
    .p_service_uuid = adv_service_uuid128,
    .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
};

static esp_ble_adv_data_t scan_rsp_data = {
    .set_scan_rsp = true,
    .include_name = true,
    .include_txpower = false,
    .service_uuid_len = sizeof(adv_service_uuid128),
    .p_service_uuid = adv_service_uuid128,
};

static esp_ble_adv_params_t adv_params = {
    .adv_int_min = 0x20,
    .adv_int_max = 0x40,
    .adv_type = ADV_TYPE_IND,
    .own_addr_type = BLE_ADDR_TYPE_PUBLIC,
    .channel_map = ADV_CHNL_ALL,
    .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};

static uint16_t service_handle;
static esp_gatt_srvc_id_t service_id = {
    .id = {
        .uuid = {.len = ESP_UUID_LEN_16, .uuid = {.uuid16 = SERVICE_UUID}},
        .inst_id = 0,
    },
    .is_primary = true,
};

static esp_gatt_id_t char_id = {
    .uuid = {.len = ESP_UUID_LEN_16, .uuid = {.uuid16 = CHAR_UUID}},
    .inst_id = 0,
};

static void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if,
                                esp_ble_gatts_cb_param_t *param)
{
    switch (event)
    {
    case ESP_GATTS_REG_EVT:
        ESP_LOGI(TAG, "Registering service...");
        esp_ble_gatts_create_service(gatts_if, &service_id, 4);
        break;
    case ESP_GATTS_CREATE_EVT:
        service_handle = param->create.service_handle;
        esp_ble_gatts_start_service(service_handle);
        esp_ble_gatts_add_char(service_handle, &char_id.uuid, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
                               char_property, &gatts_char_val, NULL);
        break;
    case ESP_GATTS_ADD_CHAR_EVT:
        ESP_LOGI(TAG, "Characteristic added.");
        break;
    case ESP_GATTS_WRITE_EVT:
        // ESP_LOGI(TAG, "Data received: %.*s", param->write.len, param->write.value);
        ESP_LOGI(TAG, "Data received: %d", param->write.len);
        ble_receive_image_chunk(param->write.value, param->write.len);
        break;
    case ESP_GATTS_CONNECT_EVT:
        ESP_LOGI(TAG, "Device connected");
        lv_gui_ble_status(true);
        break;
    case ESP_GATTS_DISCONNECT_EVT:
        ESP_LOGI(TAG, "Device disconnected");
        ble_disconnected();
        lv_gui_ble_status(false);
        esp_ble_gap_start_advertising(&adv_params);
        break;
    case ESP_GATTS_MTU_EVT:
        // MTU exchange completed
        ESP_LOGI("GATT", "MTU updated to %d", param->mtu.mtu);
        break;
    default:
        break;
    }
}

static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    switch (event)
    {
    case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
        ESP_LOGI(TAG, "Advertising data set, starting advertising...");
        esp_ble_gap_start_advertising(&adv_params);
        break;
    case ESP_GAP_BLE_SCAN_RSP_DATA_SET_COMPLETE_EVT:
        ESP_LOGI(TAG, "Scan response data set.");
        break;
    default:
        break;
    }
}

void ble_server_init()
{
    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_bt_controller_init(&bt_cfg));
    ESP_ERROR_CHECK(esp_bt_controller_enable(ESP_BT_MODE_BLE));

    ESP_ERROR_CHECK(esp_bluedroid_init());
    ESP_ERROR_CHECK(esp_bluedroid_enable());

    esp_ble_gap_set_device_name(DEVICE_NAME);
    esp_ble_gap_register_callback(gap_event_handler);

    esp_ble_gatts_register_callback(gatts_event_handler);
    esp_ble_gatts_app_register(0);
    esp_ble_gap_config_adv_data(&adv_data);
    esp_ble_gap_config_adv_data(&scan_rsp_data);
}