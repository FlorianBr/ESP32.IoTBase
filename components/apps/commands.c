/**
 ******************************************************************************
 *  file           : commands.c
 *  brief          : Command interpreter functions
 ******************************************************************************
 */

/****************************** Includes  */
#include <stdio.h>
#include <string.h>
#include <cJSON.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "freertos/queue.h"

#include "sdkconfig.h"

#include "esp_system.h"
#include "esp_event.h"
#include "esp_ota_ops.h"
#include "esp_app_format.h"
#include "esp_flash_partitions.h"
#include "esp_log.h"
#include "esp_http_client.h"
#include "esp_partition.h"

#include "../drivers/mqtt.h"

/****************************** Configuration */
#define CMD_SUBTOPIC "cmd"          // Subtopic for commands
#define CMD_FWUP     "fwupdate"     // JSON Command for a FW update
#define CMD_RESTART  "restart"      // JSON Command for restart
#define BUF_SIZE     1024           // Size of buffer for transfers

/****************************** Statics */
static const char *TAG = "CMD";
static QueueHandle_t * pRxQueue;
char ota_data[BUF_SIZE + 1] = { 0 };

/****************************** Functions */

/**
 * @brief Starts a FW update from URL (OTA update)
 *
 * @param url
 */
void cmd_fwupdate(char * url) {
    esp_err_t err;
    int file_length = 0; // length of bin file
    bool was_checked = false;
    esp_ota_handle_t update_hdl = 0;

    ESP_LOGI(TAG, "FW Update: Starting with URL '%s'", url);

    // Get partition data, print some info and check the prereqs
    const esp_partition_t *part_run = esp_ota_get_running_partition();
    const esp_partition_t *part_next = esp_ota_get_next_update_partition(NULL);

    ESP_LOGI(TAG, "Partition Infos:");
    ESP_LOGI(TAG, "Running: type %d subtype %d (offset 0x%08lx, label '%s')", part_run->type, part_run->subtype, part_run->address, part_run->label);
    ESP_LOGI(TAG, "Next:    type %d subtype %d (offset 0x%08lx, label '%s')", part_next->type, part_next->subtype, part_next->address, part_next->label);

    if (NULL == part_next) {
        ESP_LOGE(TAG, "FW Update: Partition to write to is NULL! Aborting");
        return;
    }

    // Set up http(s) transfer of the fw file
    esp_http_client_config_t config = {
        .url = url,
        .timeout_ms = 2000,     // 2s connection timeout
        .keep_alive_enable = true,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == NULL) {
        ESP_LOGE(TAG, "FW Update: Cannot init HTTP connection!");
        return;
    }
    err = esp_http_client_open(client, 0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "FW Update: Cannot open HTTP connection, err %s", esp_err_to_name(err));
        esp_http_client_cleanup(client);
        return;
    }
    esp_http_client_fetch_headers(client);

    file_length = 0;
    was_checked = false;

    // The loop for transfer and programming
    while (1) {
        int bytes_read = esp_http_client_read(client, ota_data, BUF_SIZE);
        if (bytes_read == 0) {
            // Connection closed due to error
            if (errno == ECONNRESET || errno == ENOTCONN) {
                ESP_LOGE(TAG, "FW Update: Connection closed, errno = %d", errno);
                break;
            }
            // Connection closed due to complete
            if (esp_http_client_is_complete_data_received(client) == true) {
                ESP_LOGI(TAG, "FW Update: Connection closed");
                break;
            }
        } else if (bytes_read < 0) { // Error
            ESP_LOGE(TAG, "FW Update: SSL data read error");
            esp_http_client_close(client);
            esp_http_client_cleanup(client);
            return;
        } else if (bytes_read > 0) { // Data received

            // Check the firmware header
            if (was_checked == false) {
                esp_app_desc_t new_fw_info;
                if (bytes_read > sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t) + sizeof(esp_app_desc_t)) {

                    // The current version
                    esp_app_desc_t running_fw_info;
                    if (esp_ota_get_partition_description(part_run, &running_fw_info) == ESP_OK) {
                        ESP_LOGI(TAG, "FW Update: Running version: %s", running_fw_info.version);
                    }

                    // The new version
                    memcpy(&new_fw_info, &ota_data[sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t)], sizeof(esp_app_desc_t));
                    ESP_LOGI(TAG, "FW Update: New version: %s", new_fw_info.version);

                    // Last invalid version
                    const esp_partition_t* part_last_inv = esp_ota_get_last_invalid_partition();
                    esp_app_desc_t invalid_app_info;
                    if (esp_ota_get_partition_description(part_last_inv, &invalid_app_info) == ESP_OK) {
                        ESP_LOGI(TAG, "FW Update: Invalid version: %s", invalid_app_info.version);
                    }

                    // Check before flashing: Invalid firmware
                    if (part_last_inv != NULL) {
                        if (memcmp(invalid_app_info.version, new_fw_info.version, sizeof(new_fw_info.version)) == 0) {
                            ESP_LOGE(TAG, "FW Update: Trying to reflash a invalid FW. Aborting");
                            esp_http_client_close(client);
                            esp_http_client_cleanup(client);
                            return;
                        }
                    }
#if 0
                    // Check before flashing: Same version
                    if (memcmp(new_fw_info.version, running_fw_info.version, sizeof(new_fw_info.version)) == 0) {
                        ESP_LOGE(TAG, "FW Update: Reflashing same FW. Aborting");
                        esp_http_client_close(client);
                        esp_http_client_cleanup(client);
                        return;
                    }
#endif
                    was_checked = true;

                    err = esp_ota_begin(part_next, OTA_WITH_SEQUENTIAL_WRITES, &update_hdl);
                    if (err != ESP_OK) {
                        ESP_LOGE(TAG, "FW Update: esp_ota_begin failed (%s)", esp_err_to_name(err));
                        esp_http_client_close(client);
                        esp_http_client_cleanup(client);
                        esp_ota_abort(update_hdl);
                        return;
                    }
                } else {
                    ESP_LOGE(TAG, "FW Update: Received package length error");
                    esp_http_client_close(client);
                    esp_http_client_cleanup(client);
                    esp_ota_abort(update_hdl);
                    return;
                }
            }  // was_checked

            // Write the data
            err = esp_ota_write(update_hdl, (const void *)ota_data, bytes_read);
            if (err != ESP_OK) {
                ESP_LOGE(TAG, "FW Update: Failed to write data");
                esp_http_client_close(client);
                esp_http_client_cleanup(client);
                esp_ota_abort(update_hdl);
                return;
            }
            file_length += bytes_read;
            ESP_LOGI(TAG, "FW Update: Written image length = %d", file_length);


        }  // bytes_read
    }  // while 1

    ESP_LOGI(TAG, "FW Update: Total FW size = %d", file_length);

    // Check for complete transfer
    if (esp_http_client_is_complete_data_received(client) != true) {
        ESP_LOGE(TAG, "FW Update: Error, file incomplete");
        esp_http_client_close(client);
        esp_http_client_cleanup(client);
        esp_ota_abort(update_hdl);
        return;
    }

    // Finalize and verify
    err = esp_ota_end(update_hdl);
    if (err != ESP_OK) {
        if (err == ESP_ERR_OTA_VALIDATE_FAILED) {
            ESP_LOGE(TAG, "FW Update: Error, fw corrupt");
        }
        ESP_LOGE(TAG, "FW Update: Error, esp_ota_end failed (%s)!", esp_err_to_name(err));
        esp_http_client_close(client);
        esp_http_client_cleanup(client);
        return;
    }

    // Set new partition
    err = esp_ota_set_boot_partition(part_next);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "FW Update: Setting new boot partition failed (%s)!", esp_err_to_name(err));
        esp_http_client_close(client);
        esp_http_client_cleanup(client);
        return;
    }

    // Final delay (probably not necessary) ans reboot into new FW
    vTaskDelay(250 / portTICK_PERIOD_MS);
    esp_restart();

    return;
}
/**
 * @brief Receive and handle incoming commands
 *
 * @param pvParameters
 */
void TaskCommand(void* pvParameters) {
    while (1) {
        MQTT_RXMessage RxMessage;

        // Wait blocking for a message
        // TODO Check with peek to avoid removing other apps topics
        if (pdTRUE == xQueueReceive(*pRxQueue, &RxMessage, portMAX_DELAY)) {

            // Check for correct subtopic
            if (0 == strcmp(RxMessage.SubTopic, CMD_SUBTOPIC)) {
                cJSON *jsondata = cJSON_Parse(RxMessage.Payload);

                if (NULL == jsondata) {
                    ESP_LOGW(TAG, "Error parsing JSON payload");
                } else {
                    cJSON *cmd = cJSON_GetObjectItemCaseSensitive(jsondata, "cmd");
                    cJSON *payload = cJSON_GetObjectItemCaseSensitive(jsondata, "payload");

                    if (cJSON_IsString(cmd) && (cmd->valuestring != NULL)
                     && cJSON_IsString(payload) && (payload->valuestring != NULL)) {

                        // Command: FW Update
                        if (0 == strcmp(cmd->valuestring, CMD_FWUP)) {
                            cmd_fwupdate(payload->valuestring);
                        }
                        // Command: FW Update
                        else if (0 == strcmp(cmd->valuestring, CMD_RESTART)) {
                            ESP_LOGW(TAG, "Restart!");
                            vTaskDelay(250 / portTICK_PERIOD_MS); // 250ms delay
                            esp_restart();
                        }
                        // Command: Unknown command
                        else {
                            ESP_LOGW(TAG, "Unknown command '%s'", cmd->valuestring);
                        }
                    } else {
                        ESP_LOGW(TAG, "Error slicing JSON payload");
                    }
                }
                cJSON_Delete(jsondata);
            } else {
                ESP_LOGW(TAG, "Unknown subtopic '%s'!", RxMessage.SubTopic);

            }
        }
        vTaskDelay(50 / portTICK_PERIOD_MS); // 50ms delay
    }
}

/**
 * @brief Init Command interpreter
 *
 * @return esp_err_t
 */
esp_err_t Comm_Init(void) {

    MQTT_Subscribe(CMD_SUBTOPIC);
    pRxQueue = MQTT_GetRxQueue();

    xTaskCreate(TaskCommand, "Command Task", 4096, NULL, tskIDLE_PRIORITY, NULL);

    return (ESP_OK);
}  // MQTT_Init
