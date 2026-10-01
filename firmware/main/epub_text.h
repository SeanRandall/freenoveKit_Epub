#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "esp_err.h"

#define EPUB_MAX_SECTIONS 512
#define EPUB_MAX_MARKERS 1024

typedef enum {
    EPUB_MARKER_HEADING,
} epub_marker_type_t;

typedef struct {
    epub_marker_type_t type;
    uint8_t level;
    char name[96];
    size_t text_offset;
} epub_marker_t;

typedef struct {
    char name[96];
    size_t text_offset;
    size_t text_length;
} epub_section_t;

typedef struct {
    char title[192];
    char *text;
    size_t text_length;
    epub_section_t *sections;
    size_t section_count;
    epub_marker_t *markers;
    size_t marker_count;
    bool truncated;
} epub_document_t;

/* Loads the readable spine of an unencrypted EPUB into PSRAM. */
esp_err_t epub_load_document(const char *path, epub_document_t *document);
void epub_free_document(epub_document_t *document);
