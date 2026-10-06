#include "remsound_receiver.h"

#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <time.h>

#include "driver/i2s_std.h"
#include "esp_check.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "esp_random.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "lwip/inet.h"
#include "nvs.h"
#include "opus.h"
#include "psa/crypto.h"

#define CARD_ROOT "/sdcard"
#define REM_HEADER 12
#define REM_MAGIC UINT32_C(0x444e4d52)
#define REM_VERSION 1
#define REM_FORMAT 1
#define REM_AUDIO 2
#define REM_HEARTBEAT 4
#define REM_TICK_PROOF 11
#define DISCOVERY_PORT 47821
#define DEFAULT_AUDIO_PORT 47830
#define MAX_PACKET 1536
#define MAX_OPUS 1454
#define JITTER_SLOTS 64
#define MAX_FRAME_SAMPLES 2880
#define TARGET_LATENCY_MS 40
#define MAX_LATENCY_MS 100
#define BUTTON_LONG 0x100
#define KEY_CACHE_VERSION UINT32_C(1)

static const char *TAG = "remsound";

typedef struct {
    char host[16];
    uint16_t port;
    char password[128];
    int volume;
} rem_config_t;

typedef struct {
    bool valid;
    uint32_t sequence;
    uint16_t stream;
    size_t bytes;
    uint8_t opus[MAX_OPUS];
} jitter_packet_t;

typedef struct {
    int socket;
    struct sockaddr_in peer;
    uint8_t key[32];
    psa_key_id_t key_id;
    uint8_t fingerprint[8];
    uint8_t instance[16];
    char instance_text[37];
    uint32_t outgoing_sequence;
    uint16_t stream;
    int frame_samples;
    int capture_latency_tenths;
    int volume;
    bool enabled;
    bool locked;
    bool format_ready;
    bool playing;
    bool ever_connected;
    bool no_sender_announced;
    bool password_mismatch;
    bool unsupported_format;
    bool suppress_audio;
    bool stopping;
    int64_t started_us;
    int64_t last_packet_us;
    int64_t last_discovery_us;
    int64_t last_heartbeat_us;
    int64_t last_proof_us;
    int64_t last_loss_notice_us;
    int loss_count;
    uint32_t received_count;
    uint32_t network_loss_count;
    uint32_t local_overrun_count;
    uint32_t expected_sequence;
    bool have_expected;
    jitter_packet_t *jitter;
    OpusDecoder *decoder;
    i2s_chan_handle_t audio;
    SemaphoreHandle_t jitter_lock;
    TaskHandle_t receive_task;
} receiver_t;

extern const uint8_t remsound_config_error_wav_start[] asm("_binary_remsound_config_error_wav_start");
extern const uint8_t remsound_config_error_wav_end[] asm("_binary_remsound_config_error_wav_end");
extern const uint8_t remsound_wifi_error_wav_start[] asm("_binary_remsound_wifi_error_wav_start");
extern const uint8_t remsound_wifi_error_wav_end[] asm("_binary_remsound_wifi_error_wav_end");
extern const uint8_t remsound_no_sender_wav_start[] asm("_binary_remsound_no_sender_wav_start");
extern const uint8_t remsound_no_sender_wav_end[] asm("_binary_remsound_no_sender_wav_end");
extern const uint8_t remsound_password_error_wav_start[] asm("_binary_remsound_password_error_wav_start");
extern const uint8_t remsound_password_error_wav_end[] asm("_binary_remsound_password_error_wav_end");
extern const uint8_t remsound_format_error_wav_start[] asm("_binary_remsound_format_error_wav_start");
extern const uint8_t remsound_format_error_wav_end[] asm("_binary_remsound_format_error_wav_end");
extern const uint8_t remsound_decoder_error_wav_start[] asm("_binary_remsound_decoder_error_wav_start");
extern const uint8_t remsound_decoder_error_wav_end[] asm("_binary_remsound_decoder_error_wav_end");
extern const uint8_t remsound_packet_loss_wav_start[] asm("_binary_remsound_packet_loss_wav_start");
extern const uint8_t remsound_packet_loss_wav_end[] asm("_binary_remsound_packet_loss_wav_end");
extern const uint8_t remsound_ready_wav_start[] asm("_binary_remsound_ready_wav_start");
extern const uint8_t remsound_ready_wav_end[] asm("_binary_remsound_ready_wav_end");
extern const uint8_t remsound_connected_wav_start[] asm("_binary_remsound_connected_wav_start");
extern const uint8_t remsound_connected_wav_end[] asm("_binary_remsound_connected_wav_end");
extern const uint8_t remsound_disconnected_wav_start[] asm("_binary_remsound_disconnected_wav_start");
extern const uint8_t remsound_disconnected_wav_end[] asm("_binary_remsound_disconnected_wav_end");
extern const uint8_t remsound_status_wav_start[] asm("_binary_remsound_status_wav_start");
extern const uint8_t remsound_status_wav_end[] asm("_binary_remsound_status_wav_end");
extern const uint8_t remsound_volume_wav_start[] asm("_binary_remsound_volume_wav_start");
extern const uint8_t remsound_volume_wav_end[] asm("_binary_remsound_volume_wav_end");
extern const uint8_t remsound_percent_wav_start[] asm("_binary_remsound_percent_wav_start");
extern const uint8_t remsound_percent_wav_end[] asm("_binary_remsound_percent_wav_end");
extern const uint8_t remsound_latency_wav_start[] asm("_binary_remsound_latency_wav_start");
extern const uint8_t remsound_latency_wav_end[] asm("_binary_remsound_latency_wav_end");
extern const uint8_t remsound_milliseconds_wav_start[] asm("_binary_remsound_milliseconds_wav_start");
extern const uint8_t remsound_milliseconds_wav_end[] asm("_binary_remsound_milliseconds_wav_end");
extern const uint8_t remsound_locked_wav_start[] asm("_binary_remsound_locked_wav_start");
extern const uint8_t remsound_locked_wav_end[] asm("_binary_remsound_locked_wav_end");
extern const uint8_t remsound_unlocked_wav_start[] asm("_binary_remsound_unlocked_wav_start");
extern const uint8_t remsound_unlocked_wav_end[] asm("_binary_remsound_unlocked_wav_end");
extern const uint8_t keypress_wav_start[] asm("_binary_keypress_wav_start");
extern const uint8_t keypress_wav_end[] asm("_binary_keypress_wav_end");

static uint32_t le32(const uint8_t *p)
{
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16
           | (uint32_t)p[3] << 24;
}

static uint16_t le16(const uint8_t *p)
{
    return (uint16_t)(p[0] | p[1] << 8);
}

static void put_le16(uint8_t *p, uint16_t value)
{
    p[0] = value;
    p[1] = value >> 8;
}

static void put_le32(uint8_t *p, uint32_t value)
{
    for (int i = 0; i < 4; ++i)
        p[i] = value >> (8 * i);
}

static void put_le64(uint8_t *p, uint64_t value)
{
    for (int i = 0; i < 8; ++i)
        p[i] = value >> (8 * i);
}

static void write_header(uint8_t *packet, uint8_t type, uint16_t stream,
                         uint32_t sequence)
{
    put_le32(packet, REM_MAGIC);
    packet[4] = REM_VERSION;
    packet[5] = type;
    put_le16(packet + 6, stream ? stream : 1);
    put_le32(packet + 8, sequence);
}

static char *trim(char *text)
{
    while (isspace((unsigned char)*text))
        ++text;
    char *end = text + strlen(text);
    while (end > text && isspace((unsigned char)end[-1]))
        *--end = 0;
    return text;
}

static bool read_config(rem_config_t *config)
{
    memset(config, 0, sizeof(*config));
    config->port = DEFAULT_AUDIO_PORT;
    config->volume = 70;
    FILE *file = fopen(CARD_ROOT "/.evv/remsound.ini", "rb");
    if (!file)
        file = fopen(CARD_ROOT "/.evv/REMSOUND.INI", "rb");
    if (!file) {
        ESP_LOGE(TAG, "configuration open failed: errno=%d", errno);
        return false;
    }
    char line[256];
    while (fgets(line, sizeof(line), file)) {
        char *item = trim(line);
        if (!*item || *item == '#')
            continue;
        char *equals = strchr(item, '=');
        if (!equals)
            continue;
        *equals++ = 0;
        char *value = trim(equals);
        item = trim(item);
        if (!strcasecmp(item, "host"))
            strlcpy(config->host, value, sizeof(config->host));
        else if (!strcasecmp(item, "password"))
            strlcpy(config->password, value, sizeof(config->password));
        else if (!strcasecmp(item, "port")) {
            long port = strtol(value, NULL, 10);
            if (port > 0 && port <= 65535)
                config->port = (uint16_t)port;
            else
                config->port = 0;
        } else if (!strcasecmp(item, "volume")) {
            config->volume = (int)strtol(value, NULL, 10);
        }
    }
    fclose(file);
    struct in_addr address;
    bool host_valid = inet_pton(AF_INET, config->host, &address) == 1;
    if (!host_valid || !config->password[0] || !config->port) {
        ESP_LOGE(TAG,
                 "configuration invalid: host_present=%u host_valid=%u password_present=%u port=%u",
                 config->host[0] != 0, host_valid, config->password[0] != 0,
                 (unsigned)config->port);
        return false;
    }
    if (config->volume < 0 || config->volume > 100) {
        ESP_LOGE(TAG, "configuration invalid: volume=%d", config->volume);
        return false;
    }
    ESP_LOGI(TAG, "configuration loaded: host=%s port=%u volume=%d password_present=1",
             config->host, (unsigned)config->port, config->volume);
    return true;
}

static bool derive(const char *password, const char *salt,
                   uint8_t *output, size_t output_bytes)
{
    psa_key_derivation_operation_t operation =
        PSA_KEY_DERIVATION_OPERATION_INIT;
    psa_status_t status = psa_key_derivation_setup(
        &operation, PSA_ALG_PBKDF2_HMAC(PSA_ALG_SHA_256));
    if (status == PSA_SUCCESS)
        status = psa_key_derivation_input_integer(
            &operation, PSA_KEY_DERIVATION_INPUT_COST, 100000);
    if (status == PSA_SUCCESS)
        status = psa_key_derivation_input_bytes(
            &operation, PSA_KEY_DERIVATION_INPUT_SALT,
            (const uint8_t *)salt, strlen(salt));
    if (status == PSA_SUCCESS)
        status = psa_key_derivation_input_bytes(
            &operation, PSA_KEY_DERIVATION_INPUT_PASSWORD,
            (const uint8_t *)password, strlen(password));
    if (status == PSA_SUCCESS)
        status = psa_key_derivation_output_bytes(
            &operation, output, output_bytes);
    psa_key_derivation_abort(&operation);
    return status == PSA_SUCCESS;
}

typedef struct {
    uint32_t version;
    uint8_t password_hash[32];
    uint8_t audio_key[32];
    uint8_t fingerprint[8];
} key_cache_t;

static bool prepare_keys(receiver_t *receiver, const char *password)
{
    key_cache_t cache = { .version = KEY_CACHE_VERSION };
    size_t hash_bytes = 0;
    if (psa_hash_compute(PSA_ALG_SHA_256, (const uint8_t *)password,
                         strlen(password), cache.password_hash,
                         sizeof(cache.password_hash), &hash_bytes) != PSA_SUCCESS
        || hash_bytes != sizeof(cache.password_hash))
        return false;

    nvs_handle_t handle = 0;
    bool have_handle = nvs_open("remsound", NVS_READWRITE, &handle) == ESP_OK;
    if (have_handle) {
        key_cache_t saved;
        size_t saved_bytes = sizeof(saved);
        if (nvs_get_blob(handle, "keys", &saved, &saved_bytes) == ESP_OK
            && saved_bytes == sizeof(saved)
            && saved.version == KEY_CACHE_VERSION
            && !memcmp(saved.password_hash, cache.password_hash,
                       sizeof(cache.password_hash))) {
            memcpy(receiver->key, saved.audio_key, sizeof(receiver->key));
            memcpy(receiver->fingerprint, saved.fingerprint,
                   sizeof(receiver->fingerprint));
            nvs_close(handle);
            ESP_LOGI(TAG, "startup stage key cache loaded");
            return true;
        }
    }

    int64_t started = esp_timer_get_time();
    bool okay = derive(password, "RemSound.v1.audio-key", cache.audio_key,
                       sizeof(cache.audio_key))
        && derive(password, "RemSound.v1.fingerprint", cache.fingerprint,
                  sizeof(cache.fingerprint));
    if (okay) {
        memcpy(receiver->key, cache.audio_key, sizeof(receiver->key));
        memcpy(receiver->fingerprint, cache.fingerprint,
               sizeof(receiver->fingerprint));
        if (have_handle
            && nvs_set_blob(handle, "keys", &cache, sizeof(cache)) == ESP_OK)
            (void)nvs_commit(handle);
    }
    if (have_handle) nvs_close(handle);
    ESP_LOGI(TAG, "startup stage optimized key derivation elapsed_ms=%lld",
             (long long)((esp_timer_get_time() - started) / 1000));
    return okay;
}

static bool open_packet(receiver_t *receiver, const uint8_t *sealed,
                        size_t sealed_bytes, uint8_t *plain,
                        size_t *plain_bytes)
{
    if (sealed_bytes < 28 || *plain_bytes < sealed_bytes - 28)
        return false;
    size_t cipher_bytes = sealed_bytes - 12;
    uint8_t cipher[MAX_PACKET];
    size_t payload_bytes = sealed_bytes - 28;
    memcpy(cipher, sealed + 28, payload_bytes);
    memcpy(cipher + payload_bytes, sealed + 12, 16);
    size_t output_bytes = 0;
    psa_status_t status = psa_aead_decrypt(receiver->key_id, PSA_ALG_GCM,
        sealed, 12, NULL, 0, cipher, cipher_bytes,
        plain, *plain_bytes, &output_bytes);
    if (status != PSA_SUCCESS)
        return false;
    *plain_bytes = output_bytes;
    return true;
}

static bool seal_packet(receiver_t *receiver, const uint8_t *plain,
                        size_t plain_bytes, uint8_t *sealed)
{
    esp_fill_random(sealed, 12);
    uint8_t cipher[MAX_PACKET];
    size_t cipher_bytes = 0;
    psa_status_t status = psa_aead_encrypt(receiver->key_id, PSA_ALG_GCM,
        sealed, 12, NULL, 0, plain, plain_bytes,
        cipher, sizeof(cipher), &cipher_bytes);
    if (status != PSA_SUCCESS || cipher_bytes != plain_bytes + 16)
        return false;
    memcpy(sealed + 12, cipher + plain_bytes, 16);
    memcpy(sealed + 28, cipher, plain_bytes);
    return true;
}

static bool wav_info(const uint8_t *start, const uint8_t *end,
                     const int16_t **samples, size_t *frames,
                     unsigned *rate, unsigned *channels)
{
    if (end - start < 44 || memcmp(start, "RIFF", 4)
        || memcmp(start + 8, "WAVE", 4))
        return false;
    const uint8_t *p = start + 12;
    unsigned bits = 0;
    while (p + 8 <= end) {
        uint32_t size = le32(p + 4);
        const uint8_t *data = p + 8;
        if (data + size > end)
            return false;
        if (!memcmp(p, "fmt ", 4) && size >= 16) {
            *channels = le16(data + 2);
            *rate = le32(data + 4);
            bits = le16(data + 14);
        } else if (!memcmp(p, "data", 4) && bits == 16
                   && (*channels == 1 || *channels == 2) && *rate) {
            *samples = (const int16_t *)data;
            *frames = size / (2 * *channels);
            return true;
        }
        p = data + ((size + 1) & ~1U);
    }
    return false;
}

static void play_wav(receiver_t *r, const uint8_t *start, const uint8_t *end)
{
    const int16_t *source;
    size_t frames;
    unsigned rate, channels;
    if (!wav_info(start, end, &source, &frames, &rate, &channels))
        return;
    int16_t out[256 * 2];
    uint64_t phase = 0;
    uint64_t step = ((uint64_t)rate << 32) / 48000;
    while ((phase >> 32) < frames) {
        size_t count = 0;
        while (count < 256 && (phase >> 32) < frames) {
            size_t index = phase >> 32;
            int16_t left = source[index * channels];
            int16_t right = channels == 2 ? source[index * 2 + 1] : left;
            out[count * 2] = left;
            out[count * 2 + 1] = right;
            ++count;
            phase += step;
        }
        size_t written;
        i2s_channel_write(r->audio, out, count * 4, &written, portMAX_DELAY);
    }
}

static void reset_audio_to_silence(receiver_t *r)
{
    /* Fill every DMA descriptor with zero.  Leave the channel running: an
       empty restart can make the codec retain its previous non-zero sample. */
    static const int16_t silence[256 * 2] = { 0 };
    for (int i = 0; i < 2; ++i) {
        size_t written = 0;
        (void)i2s_channel_write(r->audio, silence, sizeof(silence), &written,
                                portMAX_DELAY);
    }
    vTaskDelay(pdMS_TO_TICKS(20));
}

static void clear_jitter(receiver_t *r)
{
    for (int i = 0; i < JITTER_SLOTS; ++i)
        r->jitter[i].valid = false;
    r->have_expected = false;
    r->playing = false;
    if (r->decoder)
        opus_decoder_ctl(r->decoder, OPUS_RESET_STATE);
}

static int jitter_count(receiver_t *r)
{
    int count = 0;
    for (int i = 0; i < JITTER_SLOTS; ++i)
        count += r->jitter[i].valid;
    return count;
}

static jitter_packet_t *find_packet(receiver_t *r, uint32_t sequence)
{
    jitter_packet_t *packet = &r->jitter[sequence % JITTER_SLOTS];
    return packet->valid && packet->sequence == sequence ? packet : NULL;
}

static void write_pcm(receiver_t *r, int16_t *pcm, int frames, bool fade)
{
    for (int i = 0; i < frames; ++i) {
        int gain = r->volume;
        if (fade && i < 96)
            gain = gain * i / 96;
        pcm[i * 2] = (int16_t)((int32_t)pcm[i * 2] * gain / 100);
        pcm[i * 2 + 1] = (int16_t)((int32_t)pcm[i * 2 + 1] * gain / 100);
    }
    size_t written;
    i2s_channel_write(r->audio, pcm, frames * 4, &written, portMAX_DELAY);
}

static void playback_one(receiver_t *r)
{
    uint8_t encoded[MAX_OPUS];
    size_t encoded_bytes = 0;
    int frame_samples = 0;
    bool decode_fec = false;
    bool decode_normal = false;
    bool decode_plc = false;
    bool fade = false;

    xSemaphoreTake(r->jitter_lock, portMAX_DELAY);
    int buffered = jitter_count(r);
    int target_frames = (TARGET_LATENCY_MS * 48000 + r->frame_samples - 1)
                        / (r->frame_samples * 1000);
    if (!r->playing) {
        if (buffered < target_frames) {
            xSemaphoreGive(r->jitter_lock);
            return;
        }
        uint32_t oldest = UINT32_MAX;
        for (int i = 0; i < JITTER_SLOTS; ++i)
            if (r->jitter[i].valid && r->jitter[i].sequence < oldest)
                oldest = r->jitter[i].sequence;
        r->expected_sequence = oldest;
        r->have_expected = true;
        r->playing = true;
    }
    /* The packet currently selected for rendering is not queueing latency.
       This also prevents a legal 60 ms Opus frame from being discarded merely
       because its own duration is greater than the latency target. */
    int buffered_ms = (buffered > 0 ? buffered - 1 : 0)
                      * r->frame_samples * 1000 / 48000;
    while (buffered_ms > MAX_LATENCY_MS) {
        jitter_packet_t *drop = find_packet(r, r->expected_sequence++);
        if (drop)
            drop->valid = false;
        buffered_ms -= r->frame_samples * 1000 / 48000;
        fade = true;
    }
    jitter_packet_t *packet = find_packet(r, r->expected_sequence);
    jitter_packet_t *next = find_packet(r, r->expected_sequence + 1);
    if (!packet && next) {
        memcpy(encoded, next->opus, next->bytes);
        encoded_bytes = next->bytes;
        next->valid = false;
        decode_fec = decode_normal = true;
        ++r->loss_count;
        ++r->network_loss_count;
        r->expected_sequence += 2;
    } else if (!packet) {
        if (buffered >= target_frames) {
            frame_samples = r->frame_samples;
            decode_plc = true;
            ++r->loss_count;
            ++r->network_loss_count;
            ++r->expected_sequence;
        }
    } else {
        memcpy(encoded, packet->opus, packet->bytes);
        encoded_bytes = packet->bytes;
        packet->valid = false;
        decode_normal = true;
        ++r->expected_sequence;
    }
    xSemaphoreGive(r->jitter_lock);

    if (!decode_fec && !decode_normal && !decode_plc)
        return;
    int16_t *pcm = malloc(MAX_FRAME_SAMPLES * 2 * sizeof(*pcm));
    if (!pcm)
        return;
    if (decode_fec) {
        int frames = opus_decode(r->decoder, encoded, encoded_bytes, pcm,
                                 MAX_FRAME_SAMPLES, 1);
        if (frames > 0)
            write_pcm(r, pcm, frames, fade);
    }
    if (decode_normal) {
        int frames = opus_decode(r->decoder, encoded, encoded_bytes, pcm,
                                 MAX_FRAME_SAMPLES, 0);
        if (frames > 0)
            write_pcm(r, pcm, frames, fade);
    } else if (decode_plc) {
        int frames = opus_decode(r->decoder, NULL, 0, pcm, frame_samples, 0);
        if (frames > 0)
            write_pcm(r, pcm, frames, fade);
    }
    free(pcm);
}

static void send_discovery(receiver_t *r, const char *local_ip)
{
    char json[192];
    int length = snprintf(json, sizeof(json),
        "{\"InstanceId\":\"%s\",\"Name\":\"%s\",\"AudioPort\":%u,"
        "\"CanSend\":false,\"CanReceive\":true}",
        r->instance_text, local_ip, ntohs(r->peer.sin_port));
    struct sockaddr_in destination = r->peer;
    destination.sin_port = htons(DISCOVERY_PORT);
    sendto(r->socket, json, length, 0, (struct sockaddr *)&destination,
           sizeof(destination));
    destination.sin_addr.s_addr = inet_addr("192.168.2.255");
    sendto(r->socket, json, length, 0, (struct sockaddr *)&destination,
           sizeof(destination));
}

static void send_heartbeat(receiver_t *r, bool pong, const uint8_t *timestamp)
{
    uint8_t packet[REM_HEADER + 9];
    write_header(packet, REM_HEARTBEAT, 0xffff, ++r->outgoing_sequence);
    packet[REM_HEADER] = pong ? 1 : 0;
    if (timestamp)
        memcpy(packet + REM_HEADER + 1, timestamp, 8);
    else
        put_le64(packet + REM_HEADER + 1, esp_timer_get_time() / 1000);
    sendto(r->socket, packet, sizeof(packet), 0,
           (struct sockaddr *)&r->peer, sizeof(r->peer));
}

static void send_tick_proof(receiver_t *r)
{
    uint8_t plain[25], packet[REM_HEADER + 53];
    plain[0] = 1;
    put_le64(plain + 1, (uint64_t)time(NULL));
    memcpy(plain + 9, r->instance, 16);
    write_header(packet, REM_TICK_PROOF, 0xffff, ++r->outgoing_sequence);
    if (seal_packet(r, plain, sizeof(plain), packet + REM_HEADER))
        sendto(r->socket, packet, sizeof(packet), 0,
               (struct sockaddr *)&r->peer, sizeof(r->peer));
}

static bool legal_opus_frame(int samples)
{
    return samples == 120 || samples == 240 || samples == 480
        || samples == 960 || samples == 1920 || samples == 2880;
}

static void handle_packet(receiver_t *r, uint8_t *packet, size_t bytes,
                          const struct sockaddr_in *from)
{
    if (bytes < REM_HEADER || le32(packet) != REM_MAGIC
        || from->sin_addr.s_addr != r->peer.sin_addr.s_addr)
        return;
    if (packet[4] != REM_VERSION) {
        r->unsupported_format = true;
        return;
    }
    uint8_t type = packet[5];
    uint16_t stream = le16(packet + 6);
    uint32_t sequence = le32(packet + 8);
    uint8_t *payload = packet + REM_HEADER;
    size_t payload_bytes = bytes - REM_HEADER;
    if (type == REM_HEARTBEAT && payload_bytes >= 9) {
        if (payload[0] == 0)
            send_heartbeat(r, true, payload + 1);
        return;
    }
    if (!r->enabled)
        return;
    if (type == REM_FORMAT) {
        if (payload_bytes < 44) {
            r->unsupported_format = true;
            return;
        }
        if (memcmp(payload + 36, r->fingerprint, 8)) {
            r->password_mismatch = true;
            return;
        }
        int sample_rate = (int)le32(payload);
        int channels = (int)le32(payload + 4);
        int bits = (int)le32(payload + 8);
        int codec = (int)le32(payload + 24);
        int frame = (int)le32(payload + 28);
        if (sample_rate != 48000 || channels != 2 || bits != 16
            || codec != 2 || !legal_opus_frame(frame)) {
            r->unsupported_format = true;
            return;
        }
        xSemaphoreTake(r->jitter_lock, portMAX_DELAY);
        if (!r->format_ready || r->stream != stream || r->frame_samples != frame) {
            clear_jitter(r);
            r->stream = stream;
            r->frame_samples = frame;
            r->format_ready = true;
        }
        r->capture_latency_tenths = payload_bytes >= 46 ? le16(payload + 44) : 0;
        r->last_packet_us = esp_timer_get_time();
        xSemaphoreGive(r->jitter_lock);
        return;
    }
    if (type != REM_AUDIO || !r->format_ready || stream != r->stream)
        return;
    uint8_t plain[MAX_OPUS];
    size_t plain_bytes = sizeof(plain);
    if (!open_packet(r, payload, payload_bytes, plain, &plain_bytes))
        return;
    xSemaphoreTake(r->jitter_lock, portMAX_DELAY);
    ++r->received_count;
    r->last_packet_us = esp_timer_get_time();
    if (r->suppress_audio) {
        xSemaphoreGive(r->jitter_lock);
        return;
    }
    jitter_packet_t *slot = &r->jitter[sequence % JITTER_SLOTS];
    if (slot->valid && slot->sequence != sequence) {
        ++r->local_overrun_count;
        ESP_LOGW(TAG, "local jitter overrun: total=%lu received=%lu",
                 (unsigned long)r->local_overrun_count,
                 (unsigned long)r->received_count);
    }
    slot->valid = true;
    slot->sequence = sequence;
    slot->stream = stream;
    slot->bytes = plain_bytes;
    memcpy(slot->opus, plain, plain_bytes);
    xSemaphoreGive(r->jitter_lock);
}

static void receive_packets(void *argument)
{
    receiver_t *r = argument;
    while (!r->stopping) {
        fd_set readable;
        FD_ZERO(&readable);
        FD_SET(r->socket, &readable);
        struct timeval timeout = { .tv_sec = 0, .tv_usec = 100000 };
        if (select(r->socket + 1, &readable, NULL, NULL, &timeout) <= 0)
            continue;
        do {
            uint8_t packet[MAX_PACKET];
            struct sockaddr_in from;
            socklen_t from_size = sizeof(from);
            ssize_t bytes = recvfrom(r->socket, packet, sizeof(packet),
                                     MSG_DONTWAIT,
                                     (struct sockaddr *)&from, &from_size);
            if (bytes <= 0)
                break;
            handle_packet(r, packet, bytes, &from);
        } while (!r->stopping);
    }
    r->receive_task = NULL;
    vTaskDelete(NULL);
}

static void begin_notice(receiver_t *r)
{
    xSemaphoreTake(r->jitter_lock, portMAX_DELAY);
    r->suppress_audio = true;
    clear_jitter(r);
    xSemaphoreGive(r->jitter_lock);
}

static void end_notice(receiver_t *r)
{
    xSemaphoreTake(r->jitter_lock, portMAX_DELAY);
    clear_jitter(r);
    r->suppress_audio = false;
    xSemaphoreGive(r->jitter_lock);
}

static void play_notice(receiver_t *r, const uint8_t *start,
                        const uint8_t *end)
{
    begin_notice(r);
    play_wav(r, start, end);
    reset_audio_to_silence(r);
    end_notice(r);
}

static void announce_status(receiver_t *r)
{
    /* Snapshot status before begin_notice() clears the live jitter queue. */
    xSemaphoreTake(r->jitter_lock, portMAX_DELAY);
    int buffered = jitter_count(r);
    int frame_samples = r->frame_samples;
    int capture_latency_tenths = r->capture_latency_tenths;
    int volume = r->volume;
    bool connected = r->ever_connected;
    xSemaphoreGive(r->jitter_lock);
    int queue_latency = frame_samples > 0
        ? buffered * frame_samples * 1000 / 48000 : 0;
    int latency = (capture_latency_tenths + 5) / 10 + queue_latency;
    ESP_LOGI(TAG,
             "status: connected=%u latency_ms=%d capture_tenths=%d queue_ms=%d buffered=%d frame_samples=%d volume=%d",
             connected, latency, capture_latency_tenths, queue_latency,
             buffered, frame_samples, volume);
    begin_notice(r);
    play_wav(r, remsound_status_wav_start, remsound_status_wav_end);
    play_wav(r, connected ? remsound_connected_wav_start
                          : remsound_disconnected_wav_start,
             connected ? remsound_connected_wav_end
                       : remsound_disconnected_wav_end);
    play_wav(r, remsound_latency_wav_start, remsound_latency_wav_end);
    /* Values are deliberately rounded to five so only 21 tiny fixed number
       recordings are needed.  Linker symbols are contiguous by filename; the
       generated table below is supplied by remsound_numbers.c. */
    extern void remsound_play_number(i2s_chan_handle_t, int);
    remsound_play_number(r->audio, ((latency + 2) / 5) * 5);
    play_wav(r, remsound_milliseconds_wav_start,
             remsound_milliseconds_wav_end);
    play_wav(r, remsound_volume_wav_start, remsound_volume_wav_end);
    remsound_play_number(r->audio, volume);
    play_wav(r, remsound_percent_wav_start, remsound_percent_wav_end);
    reset_audio_to_silence(r);
    end_notice(r);
}

static void make_identity(receiver_t *r)
{
    esp_fill_random(r->instance, sizeof(r->instance));
    r->instance[6] = (r->instance[6] & 0x0f) | 0x40;
    r->instance[8] = (r->instance[8] & 0x3f) | 0x80;
    snprintf(r->instance_text, sizeof(r->instance_text),
        "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
        r->instance[0], r->instance[1], r->instance[2], r->instance[3],
        r->instance[4], r->instance[5], r->instance[6], r->instance[7],
        r->instance[8], r->instance[9], r->instance[10], r->instance[11],
        r->instance[12], r->instance[13], r->instance[14], r->instance[15]);
}

static esp_err_t init_audio(receiver_t *r)
{
    i2s_chan_config_t channel = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO,
                                                            I2S_ROLE_MASTER);
    channel.dma_desc_num = 4;
    channel.dma_frame_num = 120;
    ESP_RETURN_ON_ERROR(i2s_new_channel(&channel, &r->audio, NULL), TAG,
                        "create I2S channel");
    i2s_std_config_t standard = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(48000),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(
            I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO),
        .gpio_cfg = { .mclk = I2S_GPIO_UNUSED, .bclk = GPIO_NUM_42,
                      .ws = GPIO_NUM_41, .dout = GPIO_NUM_1,
                      .din = I2S_GPIO_UNUSED },
    };
    ESP_RETURN_ON_ERROR(i2s_channel_init_std_mode(r->audio, &standard), TAG,
                        "configure I2S");
    return i2s_channel_enable(r->audio);
}

void remsound_receiver_announce_wifi_error(void)
{
    receiver_t receiver = { 0 };
    if (init_audio(&receiver) == ESP_OK) {
        play_wav(&receiver, remsound_wifi_error_wav_start,
                 remsound_wifi_error_wav_end);
        i2s_channel_disable(receiver.audio);
        i2s_del_channel(receiver.audio);
    }
}

esp_err_t remsound_receiver_run(QueueHandle_t buttons,
                                const char *local_ip_address)
{
    rem_config_t config;
    receiver_t r = { .socket = -1, .enabled = true };
    ESP_LOGI(TAG,
             "startup: local_ip=%s internal_free=%u spiram_free=%u spiram_largest=%u",
             local_ip_address ? local_ip_address : "(null)",
             (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
             (unsigned)heap_caps_get_free_size(MALLOC_CAP_SPIRAM),
             (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM));
    esp_err_t result = init_audio(&r);
    if (result != ESP_OK) {
        ESP_LOGE(TAG, "startup failed at audio: %s", esp_err_to_name(result));
        return result;
    }
    ESP_LOGI(TAG, "startup stage audio ready");
    /* Replace any sample retained by the codec across the partition restart
       before configuration and key preparation can introduce a pause. */
    reset_audio_to_silence(&r);
    r.jitter = heap_caps_calloc(JITTER_SLOTS, sizeof(*r.jitter),
                                MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!r.jitter) {
        ESP_LOGE(TAG,
                 "startup failed at jitter allocation: requested=%u spiram_free=%u spiram_largest=%u",
                 (unsigned)(JITTER_SLOTS * sizeof(*r.jitter)),
                 (unsigned)heap_caps_get_free_size(MALLOC_CAP_SPIRAM),
                 (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM));
        return ESP_ERR_NO_MEM;
    }
    ESP_LOGI(TAG, "startup stage jitter ready: bytes=%u",
             (unsigned)(JITTER_SLOTS * sizeof(*r.jitter)));
    r.jitter_lock = xSemaphoreCreateMutex();
    if (!r.jitter_lock) {
        ESP_LOGE(TAG, "startup failed creating jitter lock");
        return ESP_ERR_NO_MEM;
    }
    if (!read_config(&config)) {
        play_wav(&r, remsound_config_error_wav_start,
                 remsound_config_error_wav_end);
        ESP_LOGE(TAG, "startup failed at configuration");
        return ESP_ERR_INVALID_ARG;
    }
    r.volume = config.volume;
    ESP_LOGI(TAG, "startup stage key preparation beginning");
    if (psa_crypto_init() != PSA_SUCCESS
        || !prepare_keys(&r, config.password)) {
        play_wav(&r, remsound_decoder_error_wav_start,
                 remsound_decoder_error_wav_end);
        ESP_LOGE(TAG, "startup failed at key preparation");
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "startup stage key preparation complete");
    psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
    psa_set_key_type(&attributes, PSA_KEY_TYPE_AES);
    psa_set_key_bits(&attributes, 256);
    psa_set_key_usage_flags(&attributes,
                            PSA_KEY_USAGE_ENCRYPT | PSA_KEY_USAGE_DECRYPT);
    psa_set_key_algorithm(&attributes, PSA_ALG_GCM);
    if (psa_import_key(&attributes, r.key, sizeof(r.key), &r.key_id)
        != PSA_SUCCESS) {
        play_wav(&r, remsound_decoder_error_wav_start,
                 remsound_decoder_error_wav_end);
        ESP_LOGE(TAG, "startup failed at AES key import");
        return ESP_FAIL;
    }
    psa_reset_key_attributes(&attributes);
    int opus_error;
    r.decoder = opus_decoder_create(48000, 2, &opus_error);
    if (!r.decoder || opus_error != OPUS_OK) {
        play_wav(&r, remsound_decoder_error_wav_start,
                 remsound_decoder_error_wav_end);
        ESP_LOGE(TAG, "startup failed at Opus decoder: opus_error=%d",
                 opus_error);
        return ESP_ERR_NO_MEM;
    }
    ESP_LOGI(TAG, "startup stage Opus decoder ready");
    make_identity(&r);
    r.peer.sin_family = AF_INET;
    r.peer.sin_port = htons(config.port);
    inet_pton(AF_INET, config.host, &r.peer.sin_addr);
    r.socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (r.socket < 0) {
        ESP_LOGE(TAG, "startup failed creating UDP socket: errno=%d", errno);
        return ESP_FAIL;
    }
    int yes = 1;
    setsockopt(r.socket, SOL_SOCKET, SO_BROADCAST, &yes, sizeof(yes));
    struct sockaddr_in local = {
        .sin_family = AF_INET, .sin_port = htons(config.port),
        .sin_addr.s_addr = htonl(INADDR_ANY),
    };
    if (bind(r.socket, (struct sockaddr *)&local, sizeof(local)) != 0) {
        ESP_LOGE(TAG, "startup failed binding UDP port %u: errno=%d",
                 (unsigned)config.port, errno);
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "receiver ready: sender=%s:%u local_port=%u",
             config.host, (unsigned)config.port, (unsigned)config.port);
    play_wav(&r, remsound_ready_wav_start, remsound_ready_wav_end);
    reset_audio_to_silence(&r);
    /* Packet receive, AES-GCM and PSA each use temporary stack buffers.  The
       combined peak is above 6 KiB when the first encrypted audio datagram is
       opened, so retain enough headroom for the crypto implementation too. */
    if (xTaskCreatePinnedToCore(receive_packets, "remsound_rx", 16384, &r,
                                18, &r.receive_task, 1) != pdPASS) {
        ESP_LOGE(TAG, "startup failed creating receive task");
        return ESP_ERR_NO_MEM;
    }
    r.started_us = r.last_packet_us = esp_timer_get_time();
    for (;;) {
        int64_t now = esp_timer_get_time();
        if (now - r.last_discovery_us >= 1500000) {
            send_discovery(&r, local_ip_address);
            r.last_discovery_us = now;
        }
        if (now - r.last_heartbeat_us >= 1000000) {
            send_heartbeat(&r, false, NULL);
            r.last_heartbeat_us = now;
        }
        if (now - r.last_proof_us >= 5000000) {
            send_tick_proof(&r);
            r.last_proof_us = now;
        }
        if (r.password_mismatch) {
            play_notice(&r, remsound_password_error_wav_start,
                        remsound_password_error_wav_end);
            break;
        }
        if (r.unsupported_format) {
            play_notice(&r, remsound_format_error_wav_start,
                        remsound_format_error_wav_end);
            break;
        }
        xSemaphoreTake(r.jitter_lock, portMAX_DELAY);
        int buffered = jitter_count(&r);
        xSemaphoreGive(r.jitter_lock);
        if (buffered && !r.ever_connected) {
            r.ever_connected = true;
            r.no_sender_announced = true;
            ESP_LOGI(TAG, "sender connected; buffered_packets=%d", buffered);
        } else if (r.ever_connected && now - r.last_packet_us > 3000000) {
            r.ever_connected = false;
            ESP_LOGI(TAG, "sender disconnected");
            play_notice(&r, remsound_disconnected_wav_start,
                        remsound_disconnected_wav_end);
        }
        if (!r.no_sender_announced && !r.ever_connected
            && now - r.started_us >= 15000000) {
            r.no_sender_announced = true;
            play_notice(&r, remsound_no_sender_wav_start,
                        remsound_no_sender_wav_end);
        }
        if (r.loss_count >= 20 && now - r.last_loss_notice_us > 5000000) {
            r.loss_count = 0;
            r.last_loss_notice_us = now;
            ESP_LOGW(TAG,
                     "network loss notice: gaps=%lu overruns=%lu received=%lu",
                     (unsigned long)r.network_loss_count,
                     (unsigned long)r.local_overrun_count,
                     (unsigned long)r.received_count);
            play_notice(&r, remsound_packet_loss_wav_start,
                        remsound_packet_loss_wav_end);
        }
        if (r.enabled && r.format_ready)
            playback_one(&r);
        int button;
        while (xQueueReceive(buttons, &button, 0) == pdTRUE) {
            bool is_long = (button & BUTTON_LONG) != 0;
            int key = button & 0xff;
            if (is_long && key == 2)
                goto done;
            if (is_long && key == 1) {
                r.locked = !r.locked;
                play_notice(&r, r.locked ? remsound_locked_wav_start
                                         : remsound_unlocked_wav_start,
                            r.locked ? remsound_locked_wav_end
                                     : remsound_unlocked_wav_end);
            } else if (!is_long && !r.locked && key == 1) {
                announce_status(&r);
            } else if (!is_long && !r.locked && key == 3) {
                r.volume = r.volume >= 5 ? r.volume - 5 : 0;
            } else if (!is_long && !r.locked && key == 5) {
                r.volume = r.volume <= 95 ? r.volume + 5 : 100;
            } else if (!is_long && !r.locked && key == 4) {
                r.enabled = !r.enabled;
                r.ever_connected = false;
                r.started_us = now;
                r.no_sender_announced = false;
                xSemaphoreTake(r.jitter_lock, portMAX_DELAY);
                clear_jitter(&r);
                xSemaphoreGive(r.jitter_lock);
                if (!r.enabled)
                    play_notice(&r, keypress_wav_start, keypress_wav_end);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(1));
    }
done:
    r.stopping = true;
    shutdown(r.socket, SHUT_RDWR);
    while (r.receive_task)
        vTaskDelay(pdMS_TO_TICKS(10));
    if (r.socket >= 0)
        close(r.socket);
    if (r.decoder)
        opus_decoder_destroy(r.decoder);
    if (r.audio) {
        i2s_channel_disable(r.audio);
        i2s_del_channel(r.audio);
    }
    if (r.jitter_lock)
        vSemaphoreDelete(r.jitter_lock);
    free(r.jitter);
    return ESP_OK;
}
