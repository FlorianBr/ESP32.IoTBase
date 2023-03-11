/**
 ******************************************************************************
 *  file           : ntp.c
 *  brief          : NTP / network time protocol tunctions
 ******************************************************************************
 */

/****************************** Includes  */
#include <stdio.h>
#include <string.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "esp_log.h"
#include "esp_sntp.h"

/****************************** Configuration */
#define SYNC_INTERVAL ((uint32_t)600000) // NTP interval in ms

/****************************** Statics */
static const char *TAG = "NTP";

/****************************** Functions */

/**
 * @brief Callback when time was set
 *
 * @param tv
 */
void ntp_cb(struct timeval *tv) {
    time_t now;
    char cBuf[64];
    struct tm timeinfo;
    now = tv->tv_sec;
    localtime_r(&now, &timeinfo);
    strftime(cBuf, sizeof(cBuf), "%c", &timeinfo);
    ESP_LOGI(TAG, "Time updated from NTP: %llu.%lu sec", tv->tv_sec, tv->tv_usec );
    ESP_LOGI(TAG, "Time updated from NTP: %s", cBuf);
}

/**
 * @brief Init NTP
 *
 * @return esp_err_t
 */
esp_err_t NTP_Init(void) {
    ESP_LOGI(TAG, "Initializing SNTP");

    // For timezone strings see:
    // https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv
    setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1); // Europe/Berlin
    tzset();

    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "de.pool.ntp.org");
    sntp_setservername(1, "pool.ntp.org");
    sntp_set_time_sync_notification_cb(ntp_cb);
    sntp_set_sync_mode(SNTP_SYNC_MODE_IMMED);
    sntp_set_sync_interval(SYNC_INTERVAL);
    sntp_init();

    return (ESP_OK);
}  // NTP_Init
