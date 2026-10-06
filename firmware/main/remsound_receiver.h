#pragma once

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

/* Runs until the user requests reading mode or an unrecoverable configuration /
 * authentication error occurs.  OpenEVV must not be initialised before this is
 * called: the receiver deliberately owns the network application's audio path. */
esp_err_t remsound_receiver_run(QueueHandle_t button_events,
                                const char *local_ip_address);
void remsound_receiver_announce_wifi_error(void);

