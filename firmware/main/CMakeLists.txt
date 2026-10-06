set(OPENEVV "${CMAKE_CURRENT_LIST_DIR}/../../vendor/openevv-perf")
set(EVV_APP_SOURCE "paragraph_reader_stable_main.c" CACHE STRING
    "Firmware entry point; use button_speech_main.c for the keypad demo")
set(EVV_LANGUAGE "enus" CACHE STRING
    "OpenEVV language module to compile (for example enus, engb or frfr)")

if(NOT EXISTS "${OPENEVV}/lang/${EVV_LANGUAGE}/delta_lang_${EVV_LANGUAGE}.c")
    message(FATAL_ERROR "Unknown OpenEVV language '${EVV_LANGUAGE}'")
endif()

if(MINIMAL_BOOT)
    idf_component_register(SRCS "minimal_main.c")
    return()
endif()

file(GLOB ENGINE_SOURCES "${OPENEVV}/src/*.c")
list(REMOVE_ITEM ENGINE_SOURCES "${OPENEVV}/src/port_win32.c")
list(REMOVE_ITEM ENGINE_SOURCES "${OPENEVV}/src/port_posix.c")

file(GLOB LANGUAGE_SOURCES "${OPENEVV}/lang/${EVV_LANGUAGE}/*.c")
file(GLOB COMPILED_RULE_SOURCES
    "${OPENEVV}/lang/${EVV_LANGUAGE}/delta_rules_c[0-9][0-9]_${EVV_LANGUAGE}.c")
list(REMOVE_ITEM LANGUAGE_SOURCES
    "${OPENEVV}/lang/${EVV_LANGUAGE}/delta_rules_none_${EVV_LANGUAGE}.c")
list(REMOVE_ITEM LANGUAGE_SOURCES ${COMPILED_RULE_SOURCES})

if(NOT COMPILED_RULE_SOURCES)
    message(FATAL_ERROR
        "Compiled rules for '${EVV_LANGUAGE}' are missing. Run "
        "make EVVLANG=lang/${EVV_LANGUAGE} rules in vendor/openevv-perf first.")
endif()

set(LANGUAGE_BINDING_SOURCE
    "${CMAKE_CURRENT_BINARY_DIR}/delta_langs_${EVV_LANGUAGE}.c")
file(WRITE "${LANGUAGE_BINDING_SOURCE}"
"#include \"delta_lang.h\"\n"
"extern delta_language delta_lang_${EVV_LANGUAGE};\n"
"void delta_lang_bind_${EVV_LANGUAGE}(void);\n"
"const delta_language *const delta_languages[] = { &delta_lang_${EVV_LANGUAGE}, 0 };\n"
"void delta_lang_bind_all(void) { delta_lang_bind_${EVV_LANGUAGE}(); }\n")

set(APP_REQUIREMENTS
    esp_timer esp_psram esp_driver_i2s esp_adc nvs_flash esp_wifi
    esp_event esp_netif esp_http_server fatfs sdmmc mdns app_update
    esp-tls mbedtls lwip esp_driver_tsens esp_audio_codec)
message(STATUS "Firmware source ${EVV_APP_SOURCE}; requirements ${APP_REQUIREMENTS}")

# Dropbox may leave the historic network entry point as an inaccessible
# placeholder.  Keep the public build argument stable while compiling the
# locally maintained network-services entry point.
set(APP_ENTRY_SOURCE "${EVV_APP_SOURCE}")
if("${EVV_APP_SOURCE}" STREQUAL "webdav_transfer_main.c")
    set(APP_ENTRY_SOURCE "network_services_main.c")
endif()

set(APP_SOURCES
    "${APP_ENTRY_SOURCE}"
    "transfer_mode.c"
    "epub_text.c"
    "${LANGUAGE_BINDING_SOURCE}"
    "port_esp.c"
    ${ENGINE_SOURCES}
    ${LANGUAGE_SOURCES}
    ${COMPILED_RULE_SOURCES})
set(APP_EMBED_FILES
    "../tones/UI.wav"
    "../tones/keypress.wav"
    "../tones/opening book.wav"
    "../tones/locked.wav"
    "../tones/unlocked.wav"
    "../tones/volume-minimum.wav"
    "../tones/volume-maximum.wav"
    "../tones/recording-started.wav"
    "../tones/recording-stopped.wav")

if("${EVV_APP_SOURCE}" STREQUAL "webdav_transfer_main.c")
    list(APPEND APP_SOURCES "remsound_receiver.c" "remsound_numbers.c")
    file(GLOB REMSOUND_WAVS RELATIVE "${CMAKE_CURRENT_LIST_DIR}"
         "${CMAKE_CURRENT_LIST_DIR}/../tones/remsound/*.wav")
    list(APPEND APP_EMBED_FILES ${REMSOUND_WAVS})
endif()

idf_component_register(
    SRCS ${APP_SOURCES}
    INCLUDE_DIRS
        "${OPENEVV}/src"
        "${OPENEVV}/lang/${EVV_LANGUAGE}"
    REQUIRES ${APP_REQUIREMENTS}
    EMBED_FILES ${APP_EMBED_FILES}
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
