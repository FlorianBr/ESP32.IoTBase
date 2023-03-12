/**
 ******************************************************************************
 *  file           : mqtt.h
 *  brief          : mqtt functions
 ******************************************************************************
 */

#ifndef COMPONENTS_DRIVERS_MQTT_H_
#define COMPONENTS_DRIVERS_MQTT_H_

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_TOPIC_LEN 250               // Max length of full topic
#define MAX_BASE_LENGTH 128             // Max length base topic
#define MAX_PAYLOAD 128                 // Max size of payload

typedef struct MQTT_RXMessage {
    char SubTopic[MAX_TOPIC_LEN-MAX_BASE_LENGTH];
    char Payload[MAX_PAYLOAD];
} MQTT_RXMessage;

esp_err_t       MQTT_Init(void);
esp_err_t       MQTT_Transmit(const char * SubTopic, const char * Payload);
esp_err_t       MQTT_Subscribe(const char * SubTopic);
esp_err_t       MQTT_Unsubscribe(const char * SubTopic);
QueueHandle_t * MQTT_GetRxQueue();

#ifdef __cplusplus
}
#endif

#endif  // COMPONENTS_DRIVERS_MQTT_H_
