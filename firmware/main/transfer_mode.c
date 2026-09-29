#include "transfer_mode.h"

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "driver/sdmmc_host.h"
#include "esp_check.h"
#include "esp_event.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_netif.h"
#include "esp_sntp.h"
#include "esp_vfs_fat.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "mdns.h"
#include "sdmmc_cmd.h"

#define CARD_ROOT "/sdcard"
#define WIFI_READY BIT0
#define WIFI_FAILED BIT1
#define MAX_PATH_LEN 512

static const char *TAG = "transfer_mode";
static EventGroupHandle_t wifi_events;
static httpd_handle_t server;
static sdmmc_card_t *card;
static bool owns_card_mount;
static esp_netif_t *station_netif;
static int reconnect_attempts;

static void trim_line_end(char *value)
{
    size_t length = strlen(value);
    while (length && (value[length - 1] == '\r' || value[length - 1] == '\n'))
        value[--length] = 0;
}

static bool read_wifi_config_file(const char *path, char *ssid,
                                  size_t ssid_size, char *password,
                                  size_t password_size)
{
    FILE *file = fopen(path, "rb");
    if (!file)
        return false;
    char line[160];
    bool have_ssid = false, have_password = false;
    while (fgets(line, sizeof(line), file)) {
        trim_line_end(line);
        if (!strncmp(line, "ssid=", 5)) {
            strlcpy(ssid, line + 5, ssid_size);
            have_ssid = ssid[0] != 0;
        } else if (!strncmp(line, "password=", 9)) {
            strlcpy(password, line + 9, password_size);
            have_password = true;
        }
    }
    fclose(file);
    return have_ssid && have_password;
}

static bool read_wifi_config(char *ssid, size_t ssid_size,
                             char *password, size_t password_size)
{
    static const char *const paths[] = {
        CARD_ROOT "/.evv/WIFI.INI",
        CARD_ROOT "/.evv/wifi.ini",
        CARD_ROOT "/EVVZERO/WIFI.INI",
        CARD_ROOT "/EVVZERO/wifi.ini",
    };
    for (size_t i = 0; i < sizeof(paths) / sizeof(paths[0]); ++i) {
        ssid[0] = 0;
        password[0] = 0;
        if (read_wifi_config_file(paths[i], ssid, ssid_size,
                                  password, password_size)) {
            ESP_LOGI(TAG, "Wi-Fi configuration loaded from %s", paths[i]);
            return true;
        }
    }
    return false;
}

static bool read_remote_mode_marker(void)
{
    FILE *file = fopen(CARD_ROOT "/.evv/network.mode", "rb");
    if (!file) return false;
    char mode[16] = { 0 };
    bool remote = fgets(mode, sizeof(mode), file)
                  && !strncasecmp(mode, "nvda", 4);
    fclose(file);
    return remote;
}

static void make_hostname(char hostname[8])
{
    uint8_t mac[6];
    if (esp_read_mac(mac, ESP_MAC_WIFI_STA) != ESP_OK) {
        strlcpy(hostname, "EVV0000", 8);
        return;
    }
    snprintf(hostname, 8, "EVV%02X%02X", mac[4], mac[5]);
}

struct dav_name {
    struct dav_name *next;
    char text[];
};

static void discard_request_body(httpd_req_t *req)
{
    char buffer[256];
    size_t remaining = req->content_len;
    while (remaining) {
        size_t wanted = remaining < sizeof(buffer) ? remaining : sizeof(buffer);
        int received = httpd_req_recv(req, buffer, wanted);
        if (received <= 0)
            break;
        remaining -= (size_t)received;
    }
}

static void wifi_event(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    (void)arg;
    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
        if (++reconnect_attempts <= 8)
            esp_wifi_connect();
        else
            xEventGroupSetBits(wifi_events, WIFI_FAILED);
    } else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        reconnect_attempts = 0;
        xEventGroupSetBits(wifi_events, WIFI_READY);
    }
}

static bool decode_path(const char *uri, char *path, size_t capacity)
{
    const char *query = strchr(uri, '?');
    size_t n = query ? (size_t)(query - uri) : strlen(uri);
    size_t out = strlen(CARD_ROOT);
    if (out + n + 1 >= capacity)
        return false;
    memcpy(path, CARD_ROOT, out);
    for (size_t i = 0; i < n; ++i) {
        unsigned value;
        if (uri[i] == '%' && i + 2 < n
            && sscanf(uri + i + 1, "%2x", &value) == 1) {
            path[out++] = (char)value;
            i += 2;
        } else {
            path[out++] = uri[i];
        }
    }
    path[out] = 0;
    if (strstr(path + strlen(CARD_ROOT), "..") != NULL
        || strchr(path, '\\') != NULL)
        return false;
    while (out > strlen(CARD_ROOT) && path[out - 1] == '/')
        path[--out] = 0;
    return true;
}

static void encode_href(const char *plain, char *encoded, size_t capacity)
{
    static const char hex[] = "0123456789ABCDEF";
    size_t out = 0;
    for (size_t i = 0; plain[i] && out + 4 < capacity; ++i) {
        unsigned char c = (unsigned char)plain[i];
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')
            || (c >= '0' && c <= '9') || strchr("/-_.~", c)) {
            encoded[out++] = (char)c;
        } else {
            encoded[out++] = '%';
            encoded[out++] = hex[c >> 4];
            encoded[out++] = hex[c & 15];
        }
    }
    encoded[out] = 0;
}

static esp_err_t send_item(httpd_req_t *req, const char *disk_path,
                           const char *uri_path)
{
    struct stat st;
    if (stat(disk_path, &st) != 0)
        return ESP_FAIL;
    char href[MAX_PATH_LEN * 3];
    encode_href(uri_path, href, sizeof(href));
    char body[1024];
    const bool directory = S_ISDIR(st.st_mode);
    int length = snprintf(body, sizeof(body),
        "<D:response><D:href>%s%s</D:href><D:propstat><D:prop>"
        "<D:displayname>%s</D:displayname><D:resourcetype>%s</D:resourcetype>"
        "<D:getcontentlength>%lld</D:getcontentlength>"
        "<D:getlastmodified>Thu, 01 Jan 1970 00:00:00 GMT</D:getlastmodified>"
        "</D:prop><D:status>HTTP/1.1 200 OK</D:status></D:propstat></D:response>",
        href, directory && strcmp(uri_path, "/") ? "/" : "",
        strrchr(uri_path, '/') && strrchr(uri_path, '/')[1]
            ? strrchr(uri_path, '/') + 1 : "SD card",
        directory ? "<D:collection/>" : "", (long long)st.st_size);
    return length > 0 && length < (int)sizeof(body)
        ? httpd_resp_send_chunk(req, body, length) : ESP_FAIL;
}

static esp_err_t propfind_handler(httpd_req_t *req, const char *path)
{
    struct stat st;
    char depth[8] = "0";
    httpd_req_get_hdr_value_str(req, "Depth", depth, sizeof(depth));
    if (stat(path, &st) != 0) {
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Not found");
        return ESP_OK;
    }
    discard_request_body(req);
    httpd_resp_set_status(req, "207 Multi-Status");
    httpd_resp_set_type(req, "application/xml; charset=utf-8");
    httpd_resp_send_chunk(req,
        "<?xml version=\"1.0\" encoding=\"utf-8\"?>"
        "<D:multistatus xmlns:D=\"DAV:\">", HTTPD_RESP_USE_STRLEN);
    send_item(req, path, req->uri);

    if (S_ISDIR(st.st_mode) && strcmp(depth, "0")) {
        DIR *dir = opendir(path);
        if (dir) {
            struct dav_name *first = NULL;
            struct dav_name **tail = &first;
            struct dirent *entry;
            while ((entry = readdir(dir)) != NULL) {
                if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, "..")
                    || strstr(entry->d_name, ".upload"))
                    continue;
                size_t name_len = strlen(entry->d_name);
                struct dav_name *item = malloc(sizeof(*item) + name_len + 1);
                if (!item)
                    break;
                item->next = NULL;
                memcpy(item->text, entry->d_name, name_len + 1);
                *tail = item;
                tail = &item->next;
            }
            closedir(dir);

            /* FatFS directory iteration and stat calls must not overlap on
               this configuration.  Collect names, close the directory, then
               inspect each entry. */
            while (first) {
                struct dav_name *item = first;
                first = item->next;
                char child[MAX_PATH_LEN];
                char child_uri[MAX_PATH_LEN];
                size_t path_len = strlen(path);
                size_t uri_len = strlen(req->uri);
                size_t name_len = strlen(item->text);
                bool path_slash = path_len && path[path_len - 1] != '/';
                bool uri_slash = uri_len && req->uri[uri_len - 1] != '/';
                if (path_len + (path_slash ? 1 : 0) + name_len >= sizeof(child)
                    || uri_len + (uri_slash ? 1 : 0) + name_len
                           >= sizeof(child_uri)) {
                    free(item);
                    continue;
                }
                memcpy(child, path, path_len);
                if (path_slash) child[path_len++] = '/';
                memcpy(child + path_len, item->text, name_len + 1);
                memcpy(child_uri, req->uri, uri_len);
                if (uri_slash) child_uri[uri_len++] = '/';
                memcpy(child_uri + uri_len, item->text, name_len + 1);
                send_item(req, child, child_uri);
                free(item);
            }
        }
    }
    httpd_resp_send_chunk(req, "</D:multistatus>", HTTPD_RESP_USE_STRLEN);
    return httpd_resp_send_chunk(req, NULL, 0);
}

static esp_err_t get_handler(httpd_req_t *req, const char *path, bool head)
{
    struct stat st;
    if (stat(path, &st) != 0 || S_ISDIR(st.st_mode)) {
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Not a file");
        return ESP_OK;
    }
    char length[24];
    snprintf(length, sizeof(length), "%lld", (long long)st.st_size);
    if (head) {
        char response[160];
        int response_len = snprintf(response, sizeof(response),
            "HTTP/1.1 200 OK\r\nContent-Type: application/octet-stream\r\n"
            "Content-Length: %s\r\n\r\n", length);
        return response_len > 0 && response_len < (int)sizeof(response)
            && httpd_send(req, response, response_len) == response_len
            ? ESP_OK : ESP_FAIL;
    }
    httpd_resp_set_type(req, "application/octet-stream");
    FILE *file = fopen(path, "rb");
    if (!file)
        return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR,
                                   "Cannot open file");
    char *buffer = malloc(8192);
    if (!buffer) {
        fclose(file);
        return ESP_ERR_NO_MEM;
    }
    size_t count;
    esp_err_t result = ESP_OK;
    while ((count = fread(buffer, 1, 8192, file)) > 0) {
        if (httpd_resp_send_chunk(req, buffer, count) != ESP_OK) {
            result = ESP_FAIL;
            break;
        }
    }
    free(buffer);
    fclose(file);
    if (result == ESP_OK)
        result = httpd_resp_send_chunk(req, NULL, 0);
    return result;
}

static esp_err_t put_handler(httpd_req_t *req, const char *path)
{
    struct stat old;
    bool replacing = stat(path, &old) == 0;
    char temporary[MAX_PATH_LEN + 16];
    snprintf(temporary, sizeof(temporary), "%s.upload", path);
    FILE *file = fopen(temporary, "wb");
    if (!file)
        return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR,
                                   "Cannot create file");
    char *buffer = malloc(8192);
    if (!buffer) {
        fclose(file);
        unlink(temporary);
        return ESP_ERR_NO_MEM;
    }
    int remaining = req->content_len;
    bool okay = true;
    while (remaining > 0) {
        int wanted = remaining > 8192 ? 8192 : remaining;
        int received = httpd_req_recv(req, buffer, wanted);
        if (received <= 0 || fwrite(buffer, 1, received, file) != (size_t)received) {
            okay = false;
            break;
        }
        remaining -= received;
    }
    free(buffer);
    fflush(file);
    fsync(fileno(file));
    fclose(file);
    if (!okay) {
        unlink(temporary);
        return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR,
                                   "Upload interrupted");
    }
    unlink(path);
    if (rename(temporary, path) != 0) {
        unlink(temporary);
        return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR,
                                   "Cannot finish upload");
    }
    httpd_resp_set_status(req, replacing ? "204 No Content" : "201 Created");
    return httpd_resp_send(req, NULL, 0);
}

static esp_err_t webdav_handler(httpd_req_t *req)
{
    char path[MAX_PATH_LEN];
    if (!decode_path(req->uri, path, sizeof(path)))
        return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Bad path");

    switch (req->method) {
    case HTTP_OPTIONS:
        httpd_resp_set_hdr(req, "DAV", "1, 2");
        httpd_resp_set_hdr(req, "Allow",
            "OPTIONS, GET, HEAD, PUT, DELETE, MKCOL, MOVE, PROPFIND, PROPPATCH, LOCK, UNLOCK");
        return httpd_resp_send(req, NULL, 0);
    case HTTP_PROPFIND:
        return propfind_handler(req, path);
    case HTTP_PROPPATCH:
        discard_request_body(req);
        httpd_resp_set_status(req, "207 Multi-Status");
        return httpd_resp_sendstr(req,
            "<?xml version=\"1.0\"?><D:multistatus xmlns:D=\"DAV:\"/>");
    case HTTP_GET:
        return get_handler(req, path, false);
    case HTTP_HEAD:
        return get_handler(req, path, true);
    case HTTP_PUT:
        return put_handler(req, path);
    case HTTP_MKCOL:
        if (mkdir(path, 0775) == 0) {
            httpd_resp_set_status(req, "201 Created");
            return httpd_resp_send(req, NULL, 0);
        }
        return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR,
                                   "Cannot create folder");
    case HTTP_DELETE:
        if (unlink(path) == 0 || rmdir(path) == 0) {
            httpd_resp_set_status(req, "204 No Content");
            return httpd_resp_send(req, NULL, 0);
        }
        return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR,
                                   "Cannot delete");
    case HTTP_MOVE: {
        char destination[MAX_PATH_LEN * 2];
        if (httpd_req_get_hdr_value_str(req, "Destination", destination,
                                        sizeof(destination)) != ESP_OK)
            return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST,
                                       "Destination required");
        char *dest_uri = strstr(destination, "://");
        if (dest_uri) {
            dest_uri = strchr(dest_uri + 3, '/');
            if (!dest_uri) dest_uri = "/";
        } else dest_uri = destination;
        char dest_path[MAX_PATH_LEN];
        if (!decode_path(dest_uri, dest_path, sizeof(dest_path)))
            return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST,
                                       "Bad destination");
        if (rename(path, dest_path) != 0)
            return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR,
                                       "Cannot move");
        httpd_resp_set_status(req, "201 Created");
        return httpd_resp_send(req, NULL, 0);
    }
    case HTTP_LOCK:
        discard_request_body(req);
        httpd_resp_set_hdr(req, "Lock-Token", "<opaquelocktoken:evvzero>");
        httpd_resp_set_type(req, "application/xml");
        return httpd_resp_sendstr(req,
            "<?xml version=\"1.0\"?><D:prop xmlns:D=\"DAV:\"><D:lockdiscovery>"
            "<D:activelock><D:locktype><D:write/></D:locktype><D:lockscope>"
            "<D:exclusive/></D:lockscope><D:depth>infinity</D:depth>"
            "<D:locktoken><D:href>opaquelocktoken:evvzero</D:href></D:locktoken>"
            "</D:activelock></D:lockdiscovery></D:prop>");
    case HTTP_UNLOCK:
        httpd_resp_set_status(req, "204 No Content");
        return httpd_resp_send(req, NULL, 0);
    default:
        return httpd_resp_send_err(req, HTTPD_405_METHOD_NOT_ALLOWED,
                                   "Unsupported WebDAV operation");
    }
}

static esp_err_t transfer_mode_start_internal(const char *fallback_ssid,
                                              const char *fallback_password,
                                              transfer_mode_status_t *status,
                                              bool mount_card,
                                              bool start_webdav)
{
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    sdmmc_slot_config_t slot = SDMMC_SLOT_CONFIG_DEFAULT();
    slot.width = 1;
    slot.clk = GPIO_NUM_39;
    slot.cmd = GPIO_NUM_38;
    slot.d0 = GPIO_NUM_40;
    slot.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;
    esp_vfs_fat_sdmmc_mount_config_t mount = {
        .format_if_mount_failed = false,
        .max_files = 8,
        .allocation_unit_size = 16 * 1024,
    };
    owns_card_mount = false;
    if (mount_card) {
        ESP_RETURN_ON_ERROR(esp_vfs_fat_sdmmc_mount(CARD_ROOT, &host, &slot,
                                                     &mount, &card), TAG,
                            "mount SD card");
        owns_card_mount = true;
        /* The OTA handoff takes two software resets; RTC scratch registers
           are not reliable across both on every board.  The reader therefore
           leaves the intended service on the card. */
        status->remote_mode = read_remote_mode_marker();
        start_webdav = !status->remote_mode;
    }

    char ssid[33] = { 0 };
    char password[65] = { 0 };
    status->credentials_from_sd = read_wifi_config(
        ssid, sizeof(ssid), password, sizeof(password));
    if (!status->credentials_from_sd) {
        strlcpy(ssid, fallback_ssid ? fallback_ssid : "", sizeof(ssid));
        strlcpy(password, fallback_password ? fallback_password : "",
                sizeof(password));
    }
    ESP_RETURN_ON_FALSE(ssid[0], ESP_ERR_INVALID_STATE, TAG,
                        "no Wi-Fi credentials; add .evv/WIFI.INI");
    make_hostname(status->hostname);

    ESP_RETURN_ON_ERROR(esp_netif_init(), TAG, "network stack");
    esp_err_t event_result = esp_event_loop_create_default();
    if (event_result != ESP_OK && event_result != ESP_ERR_INVALID_STATE)
        return event_result;
    station_netif = esp_netif_create_default_wifi_sta();
    ESP_RETURN_ON_FALSE(station_netif, ESP_ERR_NO_MEM, TAG, "station netif");
    ESP_RETURN_ON_ERROR(esp_netif_set_hostname(station_netif,
                                                status->hostname), TAG,
                        "set hostname");
    wifi_events = xEventGroupCreate();
    ESP_RETURN_ON_FALSE(wifi_events, ESP_ERR_NO_MEM, TAG, "Wi-Fi events");
    wifi_init_config_t wifi_init = WIFI_INIT_CONFIG_DEFAULT();
    ESP_RETURN_ON_ERROR(esp_wifi_init(&wifi_init), TAG, "Wi-Fi init");
    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event, NULL);
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, wifi_event, NULL);
    wifi_config_t config = { 0 };
    strlcpy((char *)config.sta.ssid, ssid, sizeof(config.sta.ssid));
    strlcpy((char *)config.sta.password, password, sizeof(config.sta.password));
    config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    ESP_RETURN_ON_ERROR(esp_wifi_set_mode(WIFI_MODE_STA), TAG, "station mode");
    ESP_RETURN_ON_ERROR(esp_wifi_set_config(WIFI_IF_STA, &config), TAG,
                        "station config");
    ESP_RETURN_ON_ERROR(esp_wifi_start(), TAG, "Wi-Fi start");
    EventBits_t bits = xEventGroupWaitBits(wifi_events,
        WIFI_READY | WIFI_FAILED, pdFALSE, pdFALSE, pdMS_TO_TICKS(30000));
    ESP_RETURN_ON_FALSE(bits & WIFI_READY, ESP_ERR_TIMEOUT, TAG,
                        "join Wi-Fi");
    esp_netif_ip_info_t ip;
    ESP_RETURN_ON_ERROR(esp_netif_get_ip_info(station_netif, &ip), TAG,
                        "read IP");
    snprintf(status->ip_address, sizeof(status->ip_address), IPSTR,
             IP2STR(&ip.ip));

    /* Keep the RTC useful after Wi-Fi is shut down.  Both WebDAV and NVDA
       Remote pass through here, so either online mode refreshes the clock. */
    setenv("TZ", "GMT0BST,M3.5.0/1,M10.5.0/2", 1);
    tzset();
    if (time(NULL) < 1704067200 && !esp_sntp_enabled()) {
        esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
        esp_sntp_setservername(0, "pool.ntp.org");
        esp_sntp_init();
    }
    for (int i = 0; i < 30 && time(NULL) < 1704067200; ++i)
        vTaskDelay(pdMS_TO_TICKS(100));
    ESP_LOGI(TAG, "Clock %s", time(NULL) >= 1704067200
             ? "synchronised" : "not yet synchronised");

    ESP_RETURN_ON_ERROR(mdns_init(), TAG, "mDNS init");
    ESP_RETURN_ON_ERROR(mdns_hostname_set(status->hostname), TAG,
                        "mDNS hostname");
    char instance[32];
    snprintf(instance, sizeof(instance), "EVVzero %s", status->hostname);
    ESP_RETURN_ON_ERROR(mdns_instance_name_set(instance), TAG,
                        "mDNS instance");
    if (!start_webdav) {
        ESP_LOGI(TAG, "Network ready at %s (%s; credentials: %s)",
                 status->hostname, status->ip_address,
                 status->credentials_from_sd ? "SD card" : "firmware fallback");
        return ESP_OK;
    }

    ESP_RETURN_ON_ERROR(mdns_service_add(NULL, "_webdav", "_tcp", 80,
                                          NULL, 0), TAG, "mDNS WebDAV");

    httpd_config_t http = HTTPD_DEFAULT_CONFIG();
    http.uri_match_fn = httpd_uri_match_wildcard;
    http.max_uri_handlers = 4;
    /* Explorer's WebClient sends substantially larger DAV request headers
       than curl.  URI decoding, Depth: 1 enumeration and ESP-IDF's header
       parser can otherwise exhaust the default/8 KiB httpd task stack. */
    http.stack_size = 16384;
    ESP_RETURN_ON_ERROR(httpd_start(&server, &http), TAG, "HTTP server");
    const httpd_uri_t root = {
        .uri = "/", .method = HTTP_ANY, .handler = webdav_handler,
    };
    const httpd_uri_t wildcard = {
        .uri = "/*", .method = HTTP_ANY, .handler = webdav_handler,
    };
    ESP_RETURN_ON_ERROR(httpd_register_uri_handler(server, &root), TAG,
                        "root WebDAV handler");
    ESP_RETURN_ON_ERROR(httpd_register_uri_handler(server, &wildcard), TAG,
                        "WebDAV handler");
    ESP_LOGI(TAG, "WebDAV ready at http://%s.local/ (%s; credentials: %s)",
             status->hostname, status->ip_address,
             status->credentials_from_sd ? "SD card" : "firmware fallback");
    return ESP_OK;
}

esp_err_t transfer_mode_start(const char *fallback_ssid,
                              const char *fallback_password,
                              transfer_mode_status_t *status)
{
    return transfer_mode_start_internal(fallback_ssid, fallback_password,
                                        status, true, true);
}

esp_err_t transfer_network_start(const char *fallback_ssid,
                                 const char *fallback_password,
                                 transfer_mode_status_t *status)
{
    return transfer_mode_start_internal(fallback_ssid, fallback_password,
                                        status, true, false);
}

esp_err_t transfer_mode_start_mounted(const char *fallback_ssid,
                                      const char *fallback_password,
                                      transfer_mode_status_t *status)
{
    return transfer_mode_start_internal(fallback_ssid, fallback_password,
                                        status, false, true);
}

void transfer_mode_stop(void)
{
    if (server) {
        httpd_stop(server);
        server = NULL;
    }
    (void)esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                       wifi_event);
    (void)esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                       wifi_event);
    (void)esp_wifi_stop();
    (void)esp_wifi_deinit();
    if (wifi_events) {
        vEventGroupDelete(wifi_events);
        wifi_events = NULL;
    }
    if (station_netif) {
        esp_netif_destroy_default_wifi(station_netif);
        station_netif = NULL;
    }
    mdns_free();
    if (card && owns_card_mount) {
        esp_vfs_fat_sdcard_unmount(CARD_ROOT, card);
        card = NULL;
    }
    owns_card_mount = false;
}
