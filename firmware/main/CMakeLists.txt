set(OPENEVV "${CMAKE_CURRENT_LIST_DIR}/../../vendor/openevv-perf")
set(EVV_APP_SOURCE "paragraph_reader_stable_main.c" CACHE STRING
    "Firmware entry point; use button_speech_main.c for the keypad demo")

if(MINIMAL_BOOT)
    idf_component_register(SRCS "minimal_main.c")
    return()
endif()

file(GLOB ENGINE_SOURCES "${OPENEVV}/src/*.c")
list(REMOVE_ITEM ENGINE_SOURCES "${OPENEVV}/src/port_win32.c")
list(REMOVE_ITEM ENGINE_SOURCES "${OPENEVV}/src/port_posix.c")

file(GLOB LANGUAGE_SOURCES "${OPENEVV}/lang/enus/*.c")
file(GLOB COMPILED_RULE_SOURCES
    "${OPENEVV}/lang/enus/delta_rules_c[0-9][0-9]_enus.c")
list(REMOVE_ITEM LANGUAGE_SOURCES
    "${OPENEVV}/lang/enus/delta_rules_none_enus.c")
list(REMOVE_ITEM LANGUAGE_SOURCES ${COMPILED_RULE_SOURCES})

set(APP_REQUIREMENTS
    esp_timer esp_psram esp_driver_i2s esp_adc nvs_flash esp_wifi
    esp_event esp_netif esp_http_server fatfs sdmmc mdns app_update
    esp-tls mbedtls lwip esp_driver_tsens)
message(STATUS "Firmware source ${EVV_APP_SOURCE}; requirements ${APP_REQUIREMENTS}")

idf_component_register(
    SRCS
        "${EVV_APP_SOURCE}"
        "transfer_mode.c"
        "epub_text.c"
        "delta_langs_enus.c"
        "port_esp.c"
        ${ENGINE_SOURCES}
        ${LANGUAGE_SOURCES}
        ${COMPILED_RULE_SOURCES}
    INCLUDE_DIRS
        "${OPENEVV}/src"
        "${OPENEVV}/lang/enus"
    REQUIRES ${APP_REQUIREMENTS}
    EMBED_FILES
        "../tones/UI.wav"
        "../tones/opening book.wav"
        "../tones/locked.wav"
        "../tones/unlocked.wav"
        "../tones/volume-minimum.wav"
        "../tones/volume-maximum.wav"
        "../tones/recording-started.wav"
        "../tones/recording-stopped.wav"
)

target_compile_options(${COMPONENT_LIB} PRIVATE
    -O3
    -fno-strict-aliasing
    -Wno-error
    -Wno-attributes
    -Wno-unused-function
    -Wno-unused-variable
    -Wno-error=format
    -Wno-error=maybe-uninitialized
    -Wno-error=stringop-truncation
)


target_compile_definitions(${COMPONENT_LIB} PRIVATE
    EVV_NO_BYTECODE=1
    EVV_FRAME_STACK=65536)

if("${EVV_APP_SOURCE}" STREQUAL "webdav_transfer_main.c")
    if(DEFINED TRANSFER_WIFI_SSID AND DEFINED TRANSFER_WIFI_PASSWORD)
        target_compile_definitions(${COMPONENT_LIB} PRIVATE
            TRANSFER_WIFI_SSID="${TRANSFER_WIFI_SSID}"
            TRANSFER_WIFI_PASSWORD="${TRANSFER_WIFI_PASSWORD}")
    endif()
endif()
