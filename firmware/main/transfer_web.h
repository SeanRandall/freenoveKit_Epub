#pragma once

#include <stdbool.h>

#include "esp_err.h"
#include "esp_http_server.h"

esp_err_t transfer_web_handler(httpd_req_t *req);
esp_err_t transfer_substitutions_handler(httpd_req_t *req);
esp_err_t transfer_settings_handler(httpd_req_t *req);

typedef bool (*transfer_web_preview_fn)(const char *text, void *context);
void transfer_web_configure(const char *engine_storage_id,
                            transfer_web_preview_fn preview,
                            void *context);

