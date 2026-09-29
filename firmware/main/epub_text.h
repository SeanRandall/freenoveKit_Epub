#pragma once

#include <stddef.h>
#include "esp_err.h"

#define EPUB_MAX_SECTIONS 64

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
} epub_document_t;

/* Loads the readable spine of an unencrypted EPUB into PSRAM. */
esp_err_t epub_load_document(const char *path, epub_document_t *document);
void epub_free_document(epub_document_t *document);
