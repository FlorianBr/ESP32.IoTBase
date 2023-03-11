/**
 ******************************************************************************
 *  file           : wifi.h
 *  brief          : WiFi Functions
 ******************************************************************************
 */

#ifndef COMPONENTS_DRIVERS_WIFI_H_
#define COMPONENTS_DRIVERS_WIFI_H_

#include "esp_wifi.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t       WiFi_Init(void);
esp_netif_t *   WiFi_GetNetIf();
bool            WiFi_isConnected();

#ifdef __cplusplus
}
#endif

#endif  // COMPONENTS_DRIVERS_WIFI_H_
