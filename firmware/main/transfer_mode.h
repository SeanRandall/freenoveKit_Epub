#pragma once

#include <stdbool.h>

#include "esp_err.h"

typedef struct {
    char ip_address[16];
    char hostname[8];
    bool credentials_from_sd;
    bool remote_mode;
} transfer_mode_status_t;

/* Mounts the SD card, joins Wi-Fi, and exposes the complete card through
 * unauthenticated WebDAV.  The caller must ensure the reader has no open card
 * files while this service is running. */
/* If .evv/WIFI.INI (or legacy EVVZERO/WIFI.INI) contains valid ssid= and
 * password= entries, those
 * override the optional fallback values supplied by the firmware. */
esp_err_t transfer_mode_start(const char *fallback_ssid,
                              const char *fallback_password,
                              transfer_mode_status_t *status);

/* Mounts the card and joins Wi-Fi without starting WebDAV.  Used by network
 * services such as the listening-only NVDA Remote client. */
esp_err_t transfer_network_start(const char *fallback_ssid,
                                 const char *fallback_password,
                                 transfer_mode_status_t *status);

/* Starts DAV on a card already mounted at /sdcard by the reader. */
esp_err_t transfer_mode_start_mounted(const char *fallback_ssid,
                                      const char *fallback_password,
                                      transfer_mode_status_t *status);

void transfer_mode_stop(void);
