set(OPENEVV "${CMAKE_CURRENT_LIST_DIR}/../../vendor/openevv-perf")
set(EVV_APP_SOURCE "paragraph_reader_stable_main.c" CACHE STRING
    "Firmware entry point; use button_speech_main.c for the keypad demo")
set(EVV_LANGUAGE "enus" CACHE STRING
    "OpenEVV language module to compile (for example enus, engb or frfr)")
set(EVV_SPEECH_ENGINE "openevv" CACHE STRING
    "Speech backend to compile (openevv, pico, openbst or dectalk)")
set(EVV_PICO_VOICE "en-US" CACHE STRING
    "Pico voice variant (en-US, en-GB, de-DE, es-ES, fr-FR or it-IT)")
set(EVV_OPENBST_BUILD "1998ENG" CACHE STRING
    "OpenBST build name")

if(EVV_SPEECH_ENGINE STREQUAL "openevv" AND NOT EXISTS "${OPENEVV}/lang/${EVV_LANGUAGE}/delta_lang_${EVV_LANGUAGE}.c")
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
    )

if(EVV_SPEECH_ENGINE STREQUAL "openevv")
    list(APPEND APP_SOURCES
        "speech_engine_openevv.c"
        "${LANGUAGE_BINDING_SOURCE}"
        "port_esp.c"
        ${ENGINE_SOURCES}
        ${LANGUAGE_SOURCES}
        ${COMPILED_RULE_SOURCES})
    set(SPEECH_INCLUDE_DIRS
        "${OPENEVV}/src"
        "${OPENEVV}/lang/${EVV_LANGUAGE}")
elseif(EVV_SPEECH_ENGINE STREQUAL "pico")
    set(PICO_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../vendor/picotts/pico")
    if(EVV_PICO_VOICE STREQUAL "en-US")
        set(PICO_TA "en-US_ta.bin")
        set(PICO_SG "en-US_lh0_sg.bin")
        set(PICO_TA_SYMBOL "_binary_en_US_ta_bin")
        set(PICO_SG_SYMBOL "_binary_en_US_lh0_sg_bin")
    elseif(EVV_PICO_VOICE STREQUAL "en-GB")
        set(PICO_TA "en-GB_ta.bin")
        set(PICO_SG "en-GB_kh0_sg.bin")
        set(PICO_TA_SYMBOL "_binary_en_GB_ta_bin")
        set(PICO_SG_SYMBOL "_binary_en_GB_kh0_sg_bin")
    elseif(EVV_PICO_VOICE STREQUAL "de-DE")
        set(PICO_TA "de-DE_ta.bin")
        set(PICO_SG "de-DE_gl0_sg.bin")
        set(PICO_TA_SYMBOL "_binary_de_DE_ta_bin")
        set(PICO_SG_SYMBOL "_binary_de_DE_gl0_sg_bin")
    elseif(EVV_PICO_VOICE STREQUAL "es-ES")
        set(PICO_TA "es-ES_ta.bin")
        set(PICO_SG "es-ES_zl0_sg.bin")
        set(PICO_TA_SYMBOL "_binary_es_ES_ta_bin")
        set(PICO_SG_SYMBOL "_binary_es_ES_zl0_sg_bin")
    elseif(EVV_PICO_VOICE STREQUAL "fr-FR")
        set(PICO_TA "fr-FR_ta.bin")
        set(PICO_SG "fr-FR_nk0_sg.bin")
        set(PICO_TA_SYMBOL "_binary_fr_FR_ta_bin")
        set(PICO_SG_SYMBOL "_binary_fr_FR_nk0_sg_bin")
    elseif(EVV_PICO_VOICE STREQUAL "it-IT")
        set(PICO_TA "it-IT_ta.bin")
        set(PICO_SG "it-IT_cm0_sg.bin")
        set(PICO_TA_SYMBOL "_binary_it_IT_ta_bin")
        set(PICO_SG_SYMBOL "_binary_it_IT_cm0_sg_bin")
    else()
        message(FATAL_ERROR "Unknown Pico voice '${EVV_PICO_VOICE}'")
    endif()
    file(GLOB PICO_SOURCES "${PICO_ROOT}/lib/*.c")
    list(APPEND APP_SOURCES "speech_engine_pico.c" ${PICO_SOURCES})
    set(SPEECH_INCLUDE_DIRS "${PICO_ROOT}/lib")
elseif(EVV_SPEECH_ENGINE STREQUAL "openbst")
    set(OPENBST_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../vendor/openbst")
    if(NOT EXISTS "${OPENBST_ROOT}/src/data/${EVV_OPENBST_BUILD}.c")
        message(FATAL_ERROR "Unknown OpenBST build '${EVV_OPENBST_BUILD}'")
    endif()
    set(OPENBST_MODULES
        text dict lts cursors wordstream stress assemble tokenise translit
        except numbers phrules accents pairs contour diphone frames interp
        synth speak)
    foreach(MODULE IN LISTS OPENBST_MODULES)
        list(APPEND OPENBST_SOURCES "${OPENBST_ROOT}/src/${MODULE}.c")
    endforeach()
    list(APPEND APP_SOURCES
        "speech_engine_openbst.c"
        "openbst_2006eng_index.c"
        "${OPENBST_ROOT}/src/data/${EVV_OPENBST_BUILD}.c"
        ${OPENBST_SOURCES})
    set(SPEECH_INCLUDE_DIRS "${OPENBST_ROOT}/include")
elseif(EVV_SPEECH_ENGINE STREQUAL "dectalk")
    set(DECTALK_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../vendor/dectalk-dtc01-reborn")
    set(DECTALK_SOURCES
        ${DECTALK_ROOT}/src/api/ttsapi.c
        ${DECTALK_ROOT}/src/api/tts_dict.c
        ${DECTALK_ROOT}/src/api/tts_stubs.c
        ${DECTALK_ROOT}/src/speech/engine.c
        ${DECTALK_ROOT}/src/speech/tx_scan.c
        ${DECTALK_ROOT}/src/speech/tx_num.c
        ${DECTALK_ROOT}/src/speech/tx_word.c
        ${DECTALK_ROOT}/src/speech/tx_dict.c
        ${DECTALK_ROOT}/src/speech/tx_lts.c
        ${DECTALK_ROOT}/src/speech/tx_phon.c
        ${DECTALK_ROOT}/src/speech/tx_clause.c
        ${DECTALK_ROOT}/src/speech/tx_rom.c
        ${DECTALK_ROOT}/src/speech/tx_rom_lts.c
        ${DECTALK_ROOT}/src/speech/tx_rom_dict.c
        ${DECTALK_ROOT}/src/speech/ph_frame.c
        ${DECTALK_ROOT}/src/speech/ph_timing.c
        ${DECTALK_ROOT}/src/speech/ph_alloph.c
        ${DECTALK_ROOT}/src/speech/ph_clause.c
        ${DECTALK_ROOT}/src/speech/ph_command.c
        ${DECTALK_ROOT}/src/speech/ph_task.c
        ${DECTALK_ROOT}/src/speech/ph_rom.c
        ${DECTALK_ROOT}/src/speech/dsp_synth.c
        ${DECTALK_ROOT}/src/speech/dsp_rom.c
        ${DECTALK_ROOT}/src/speech/dsp_link.c
        ${DECTALK_ROOT}/src/kernel/kernel.c
        ${DECTALK_ROOT}/src/kernel/console.c)
    list(APPEND APP_SOURCES
        "speech_engine_dectalk.c"
        "dectalk_audio_esp.c"
        ${DECTALK_SOURCES})
    set(SPEECH_INCLUDE_DIRS
        "${DECTALK_ROOT}/src/api"
        "${DECTALK_ROOT}/src/speech"
        "${DECTALK_ROOT}/src/kernel")
    list(APPEND APP_REQUIREMENTS pthread)
else()
    message(FATAL_ERROR "Unknown speech backend '${EVV_SPEECH_ENGINE}'")
endif()
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

if(EVV_SPEECH_ENGINE STREQUAL "pico")
    list(APPEND APP_EMBED_FILES
        "../../vendor/picotts/pico/lang/${PICO_TA}"
        "../../vendor/picotts/pico/lang/${PICO_SG}")
endif()

if("${EVV_APP_SOURCE}" STREQUAL "webdav_transfer_main.c")
    list(APPEND APP_SOURCES "remsound_receiver.c" "remsound_numbers.c")
    file(GLOB REMSOUND_WAVS RELATIVE "${CMAKE_CURRENT_LIST_DIR}"
         "${CMAKE_CURRENT_LIST_DIR}/../tones/remsound/*.wav")
    list(APPEND APP_EMBED_FILES ${REMSOUND_WAVS})
endif()

idf_component_register(
    SRCS ${APP_SOURCES}
    INCLUDE_DIRS
        ${SPEECH_INCLUDE_DIRS}
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
    -Wno-error=uninitialized
    -Wno-error=maybe-uninitialized
    -Wno-error=stringop-truncation
)


target_compile_definitions(${COMPONENT_LIB} PRIVATE
    EVV_NO_BYTECODE=1
    EVV_FRAME_STACK=65536)

if(EVV_SPEECH_ENGINE STREQUAL "pico")
    target_compile_definitions(${COMPONENT_LIB} PRIVATE
        EVV_ENGINE_SAMPLE_RATE=16000
        __BYTE_ORDER=__BYTE_ORDER__
        __BIG_ENDIAN=__ORDER_BIG_ENDIAN__)
    set_source_files_properties("speech_engine_pico.c" PROPERTIES
      COMPILE_DEFINITIONS
        "PICO_VOICE_TAG=\"${EVV_PICO_VOICE}\";PICO_TA_FILENAME=\"${PICO_TA}\";PICO_SG_FILENAME=\"${PICO_SG}\";PICO_TA_SYMBOL=${PICO_TA_SYMBOL};PICO_SG_SYMBOL=${PICO_SG_SYMBOL}")
endif()

if(EVV_SPEECH_ENGINE STREQUAL "openbst")
    if(EVV_OPENBST_BUILD STREQUAL "1995"
       OR EVV_OPENBST_BUILD MATCHES "^1998")
        set(OPENBST_SAMPLE_RATE 11025)
    elseif(EVV_OPENBST_BUILD STREQUAL "2006RUS")
        set(OPENBST_SAMPLE_RATE 10800)
    else()
        set(OPENBST_SAMPLE_RATE 10000)
    endif()
    target_compile_definitions(${COMPONENT_LIB} PRIVATE
        EVV_ENGINE_SAMPLE_RATE=${OPENBST_SAMPLE_RATE})
    set_source_files_properties(
      "speech_engine_openbst.c" "openbst_2006eng_index.c" PROPERTIES
      COMPILE_DEFINITIONS
        "OPENBST_BUILD_NAME=\"${EVV_OPENBST_BUILD}\";OPENBST_DATA_SYMBOL=BST_DATA_${EVV_OPENBST_BUILD}")
endif()

if(EVV_SPEECH_ENGINE STREQUAL "dectalk")
    target_compile_definitions(${COMPONENT_LIB} PRIVATE
        EVV_ENGINE_SAMPLE_RATE=10000)
    set_source_files_properties(
        "${DECTALK_ROOT}/src/kernel/kernel.c"
        "${DECTALK_ROOT}/src/speech/engine.c" PROPERTIES
        COMPILE_DEFINITIONS "sem_wait=dtc01_sem_wait")
endif()

if("${EVV_APP_SOURCE}" STREQUAL "webdav_transfer_main.c")
    if(DEFINED TRANSFER_WIFI_SSID AND DEFINED TRANSFER_WIFI_PASSWORD)
        target_compile_definitions(${COMPONENT_LIB} PRIVATE
            TRANSFER_WIFI_SSID="${TRANSFER_WIFI_SSID}"
            TRANSFER_WIFI_PASSWORD="${TRANSFER_WIFI_PASSWORD}")
    endif()
endif()
