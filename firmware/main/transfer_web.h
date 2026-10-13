#pragma once

#include "esp_err.h"
#include "esp_http_server.h"

esp_err_t transfer_web_handler(httpd_req_t *req);

