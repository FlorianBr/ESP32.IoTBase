/**
 ******************************************************************************
 *  file           : mqtt.c
 *  brief          : mqtt functions, based on espressifs tcp example
 ******************************************************************************
 */

/****************************** Includes  */
#include <stdio.h>
#include <string.h>
#include "sdkconfig.h"
#include "esp_system.h"
#include "mqtt_client.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_mac.h"

/****************************** Configuration */
#define MAX_URLLEN  64                  // Max length of broker URL
#define MAX_BASE_LENGTH 128             // Max length base topic
#define MAX_TOPIC_LEN 250               // Max length of full topic
#define NVS_NAMESPACE "SETTINGS"        // Namespace for the Settings
#define MQTT_ID "IoT"                   // Start of the base ID

/****************************** Statics */
static const char *TAG = "MQTT";
static esp_mqtt_client_handle_t client = NULL;
static bool isConnected = false;
static char BaseTopic[MAX_BASE_LENGTH];

/****************************** Functions */

/**
 * @brief Event handler registered to receive MQTT events
 *
 * @param handler_args user data registered to the event.
 * @param base Event base for the handler(always MQTT Base in this example).
 * @param event_id The id for the received event.
 * @param event_data The data for the event, esp_mqtt_event_handle_t.
 */
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%ld", base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
            // TODO subscribe to topic
            // msg_id = esp_mqtt_client_subscribe(client, "/topic/qos0", 0);
            // ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);
            isConnected = true;
            break;
        case MQTT_EVENT_DISCONNECTED:
            isConnected = false;
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
            break;
        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
            msg_id = esp_mqtt_client_publish(client, "/topic/qos0", "data", 0, 0, 0);
            ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
            break;
        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "MQTT_EVENT_DATA");
            // TODO put data into queue
            printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
            printf("DATA=%.*s\r\n", event->data_len, event->data);
            break;
        case MQTT_EVENT_BEFORE_CONNECT:
            ESP_LOGI(TAG, "MQTT_EVENT_BEFORE_CONNECT");
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
            break;
        default:
            ESP_LOGI(TAG, "Other event id:%d", event->event_id);
            break;
    }
} // mqtt_event_handler

/**
 * @brief Init MQTT
 *
 * @return esp_err_t
 */
esp_err_t MQTT_Init(void) {
    size_t          url_length = MAX_URLLEN;
    esp_err_t       ret = ESP_OK;
    nvs_handle_t    handle;
    char            cBuffer[MAX_URLLEN];
    uint8_t         Mac[6];

    // Read in broker URL
    ret = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    ESP_ERROR_CHECK(ret);
    ret = nvs_get_str(handle, "MQTT_URL", &cBuffer[0], &url_length);
    ESP_ERROR_CHECK(ret);
    nvs_close(handle);
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = cBuffer,
    };
    ESP_LOGI(TAG, "Broker address is: %s", mqtt_cfg.broker.address.uri);

    // Setup MQTT client
    client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);

    // Generate base topic with id and mac address
    ESP_ERROR_CHECK(esp_efuse_mac_get_default(&Mac[0]));
    snprintf(&BaseTopic[0], MAX_BASE_LENGTH, "%s_%02x%02x%02x%02x%02x%02x", MQTT_ID, Mac[0], Mac[1], Mac[2], Mac[3], Mac[4], Mac[5]);

    return (ESP_OK);
}  // MQTT_Init


/**
 * @brief Transmit Data to MQTT
 *
 * @param SubTopic The subtopic to send to
 * @param Payload The payload to send
 * @return esp_err_t
 */
esp_err_t MQTT_Transmit(const char * SubTopic, const char * Payload) {
    char FullTopic[MAX_TOPIC_LEN];

    if (!isConnected) {
        ESP_LOGW(TAG, "Cannot transmit: Not connected");
        return(ESP_FAIL);
    }

    // generate full topic: Bae Topic / Subtopic(s)
    snprintf(&FullTopic[0], MAX_TOPIC_LEN, "%s/%s", BaseTopic, SubTopic );

    // Transmit, QoS always 1 and no retaining
    int msg_id = esp_mqtt_client_publish(client, FullTopic, Payload, 0, 1, 0);

    if (0 > msg_id) {
        ESP_LOGW(TAG, "Cannot transmit: Code %d", msg_id);
        return(ESP_FAIL);
    }
    return (ESP_OK);
}
