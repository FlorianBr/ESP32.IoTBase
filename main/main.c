/*
 * ESP32 Base for MQTT controlled IoT devices
 *
 * 2023 Florian Brandner
 */

/****************************** Includes  */
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_chip_info.h"
#include "esp_ota_ops.h"
#include "esp_partition.h"

#include "../components/drivers/wifi.h"
#include "../components/drivers/ntp.h"

/****************************** Statics */

static const char *TAG = "MAIN";

/****************************** Functions */

/**
 * @brief App Main / entry point
 */
void app_main(void) {
    esp_err_t ret = ESP_OK;

#if 1
    // Print some system statistics
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);

    // Core info
    ESP_LOGW(TAG, "-------------------------------------");
    ESP_LOGW(TAG, "System Info:");
    ESP_LOGW(TAG, "%s chip with %d CPU cores, WiFi%s%s, ",
           CONFIG_IDF_TARGET,
           chip_info.cores,
           (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
           (chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "");
    ESP_LOGW(TAG, "Heap: %lu", esp_get_free_heap_size());
    ESP_LOGW(TAG, "Reset reason: %d", esp_reset_reason());
    ESP_LOGW(TAG, "-------------------------------------");

    // Partition info
    const esp_partition_t * part_info;
    esp_ota_img_states_t ota_state;
    part_info = esp_ota_get_boot_partition();
    esp_ota_get_state_partition(part_info, &ota_state);
    if (NULL != part_info) {
        ESP_LOGW(TAG, "Current partition:");
        ESP_LOGW(TAG, "    Label = %s, state = %d", part_info->label, ota_state);
        ESP_LOGW(TAG, "    Address=0x%lx, size=0x%lx", part_info->address, part_info->size);
    }
    ESP_LOGW(TAG, "-------------------------------------");
#endif

    // Initialize NVS, format it if necessary
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "Erasing NVS!");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    ESP_LOGI(TAG, "NVS init returned %d", ret);

#if 1
    // Print out NVS statistics
    nvs_stats_t nvs_stats;
    nvs_get_stats("nvs", &nvs_stats);
    ESP_LOGW(TAG, "-------------------------------------");
    ESP_LOGW(TAG, "NVS Statistics:");
    ESP_LOGW(TAG, "NVS Used = %d", nvs_stats.used_entries);
    ESP_LOGW(TAG, "NVS Free = %d", nvs_stats.free_entries);
    ESP_LOGW(TAG, "NVS All = %d", nvs_stats.total_entries);

    nvs_iterator_t iter = NULL;
    esp_err_t res = nvs_entry_find("nvs", NULL, NVS_TYPE_ANY, &iter);
    while(res == ESP_OK) {
        nvs_entry_info_t info;
        nvs_entry_info(iter, &info);
        ESP_LOGW(TAG, "Key '%s', Type '%d'", info.key, info.type);
        res = nvs_entry_next(&iter);
    }
    nvs_release_iterator(iter);
    ESP_LOGW(TAG, "-------------------------------------");
#endif

#if 0
    // Write initial settings to NVS
    nvs_handle_t handle;
    nvs_open("SETTINGS", NVS_READWRITE, &handle);
    nvs_set_str(handle, "WIFI_SSID", "My cool SSID");
    nvs_set_str(handle, "WIFI_PASS", "SupaSecret");
    nvs_set_str(handle, "MQTT_URL", "mqtt://IP:Address");
    nvs_close(handle);
#endif

    // Initialize WiFi
    ret = WiFi_Init();
    ESP_ERROR_CHECK(ret);

    // Initialize NTP
    ret = NTP_Init();
    ESP_ERROR_CHECK(ret);

    // Idle loop
    ESP_LOGW(TAG, "Starting idling");
    while (1) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
