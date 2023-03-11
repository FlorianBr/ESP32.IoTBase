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

esp_err_t   MQTT_Init(void);
esp_err_t   MQTT_Transmit(const char * SubTopic, const char * Payload);

#ifdef __cplusplus
}
#endif

#endif  // COMPONENTS_DRIVERS_MQTT_H_
