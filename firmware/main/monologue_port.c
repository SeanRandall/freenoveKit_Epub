#include "monologue_port.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "fbv.h"

extern const unsigned char monologue_pack_start[]
    asm("_binary_monologue_xen11k8_pack_start");
extern const unsigned char monologue_pack_end[]
    asm("_binary_monologue_xen11k8_pack_end");

#define PACK_TYPE_BYTES 24
#define PACK_NAME_BYTES 40

#pragma pack(push, 1)
typedef struct {
    char magic[8];
    uint32_t count;
    uint32_t entry_size;
} pack_header_t;

typedef struct {
    uint8_t type_numeric;
    uint8_t name_numeric;
    uint16_t reserved;
    uint32_t type_id;
    uint32_t name_id;
    char type_name[PACK_TYPE_BYTES];
    char name_name[PACK_NAME_BYTES];
    uint32_t offset;
    uint32_t size;
} pack_entry_t;
#pragma pack(pop)

struct monologue_voice {
    BYTE *scb;
};

static int ascii_equal(const char *left, const char *right)
{
    while (*left && *right) {
        unsigned char a = (unsigned char)*left++;
        unsigned char b = (unsigned char)*right++;
        if (a >= 'a' && a <= 'z') a -= 'a' - 'A';
        if (b >= 'a' && b <= 'z') b -= 'a' - 'A';
        if (a != b) return 0;
    }
    return *left == *right;
}

static const pack_header_t *pack_header(void)
{
    const pack_header_t *header = (const pack_header_t *)monologue_pack_start;
    size_t bytes = (size_t)(monologue_pack_end - monologue_pack_start);
    if (bytes < sizeof(*header)
        || memcmp(header->magic, "MONOVC1\0", 8) != 0
        || header->entry_size != sizeof(pack_entry_t)
        || sizeof(*header) + (size_t)header->count * sizeof(pack_entry_t) > bytes)
        return NULL;
    return header;
}

static const pack_entry_t *pack_entries(const pack_header_t *header)
{
    return (const pack_entry_t *)(header + 1);
}

static const void *entry_data(const pack_entry_t *entry)
{
    size_t bytes = (size_t)(monologue_pack_end - monologue_pack_start);
    if ((size_t)entry->offset + entry->size > bytes) return NULL;
    return monologue_pack_start + entry->offset;
}

HRSRC FindResourceA(HMODULE module, LPCSTR name, LPCSTR type)
{
    (void)module;
    const pack_header_t *header = pack_header();
    if (!header) return NULL;
    const pack_entry_t *entries = pack_entries(header);
    int type_numeric = IS_INTRESOURCE(type);
    int name_numeric = IS_INTRESOURCE(name);
    for (uint32_t i = 0; i < header->count; i++) {
        const pack_entry_t *entry = &entries[i];
        if (entry->type_numeric != type_numeric
            || entry->name_numeric != name_numeric)
            continue;
        if (type_numeric) {
            if (entry->type_id != (uint32_t)(uintptr_t)type) continue;
        } else if (!ascii_equal(entry->type_name, type)) {
            continue;
        }
        if (name_numeric) {
            if (entry->name_id != (uint32_t)(uintptr_t)name) continue;
        } else if (!ascii_equal(entry->name_name, name)) {
            continue;
        }
        return (HRSRC)entry;
    }
    return NULL;
}

HGLOBAL LoadResource(HMODULE module, HRSRC resource)
{
    (void)module;
    return resource ? (HGLOBAL)entry_data((const pack_entry_t *)resource) : NULL;
}

void *LockResource(HGLOBAL resource) { return resource; }
int FreeResource(HGLOBAL resource) { (void)resource; return 0; }

static HGLOBAL resource(const char *type, WORD id)
{
    return LoadResource((HMODULE)1,
                        FindResourceA((HMODULE)1, MAKEINTRESOURCE(id), type));
}

static HGLOBAL named_resource(WORD type, const char *name)
{
    return LoadResource((HMODULE)1,
                        FindResourceA((HMODULE)1, name, MAKEINTRESOURCE(type)));
}

static uint32_t resource_dword(WORD type, const char *name, uint32_t fallback)
{
    const uint32_t *value = named_resource(type, name);
    return value ? *value : fallback;
}

static int load_dictionary(BYTE *scb)
{
    HGLOBAL data = resource("MONO_DICTIONARY", 1);
    if (!data) return -1;
    const BYTE *base = (const BYTE *)data;
    uint32_t count = (uint32_t)(base[0] | base[1] << 8);
    DICTSLOT *slot = calloc(1, sizeof(*slot));
    if (!slot) return -1;
    slot->entries = (BYTE *)base + 4;
    slot->pool = (char *)base + 4 + count * 10;
    slot->count = count;
    const pack_entry_t *entry = (const pack_entry_t *)
        FindResourceA((HMODULE)1, MAKEINTRESOURCE(1), "MONO_DICTIONARY");
    slot->poolsize = entry && entry->size >= 4 + count * 10
        ? entry->size - 4 - count * 10 : 0;
    SCBF(scb, DICTSLOT *, SCB_D_SLOTS) = slot;
    SCBF(scb, int, SCB_D_NSLOTS) = 1;
    SCBF(scb, BYTE *, SCB_D_ENTRIES) = slot->entries;
    SCBF(scb, char *, SCB_D_POOL) = slot->pool;
    SCBF(scb, DWORD, SCB_D_COUNT) = slot->count;
    SCBF(scb, DWORD, SCB_D_POOLSIZE) = slot->poolsize;
    SCBF(scb, int, SCB_D_KERNEL) = 0;
    SCBF(scb, int, 0x270) = 1;
    SCBF(scb, int, 0x274) = 1;
    SCBF(scb, int, 0x278) = 1;
    return 0;
}

monologue_voice_t *monologue_voice_open(void)
{
    const pack_header_t *header = pack_header();
    if (!header) return NULL;
    monologue_voice_t *voice = calloc(1, sizeof(*voice));
    BYTE *scb = calloc(1, SCB_SIZE);
    if (!voice || !scb) {
        free(voice);
        free(scb);
        return NULL;
    }
    voice->scb = scb;
    g_err_scb = scb;
    SCBF(scb, int, 0) = 5;
    SCBF(scb, int, 4) = 5;
    SCBF(scb, int, 8) = 9;
    SCBF(scb, DWORD, SCB_FLAGS) = 8;
    SCBF(scb, int, SCB_SENT_PAUSE) = 0;
    SCBF(scb, HMODULE, 0x15c) = (HMODULE)1;
    SCBF(scb, int, 0x230) = (int)resource_dword(10, "NUMRULEPASSES", 0);
    if (SCBF(scb, int, 0x230) <= 0 || SCBF(scb, int, 0x230) > 26)
        goto fail;
    for (int i = 0; i < SCBF(scb, int, 0x230); i++) {
        HGLOBAL pass = resource("RULE", (WORD)(300 + i));
        if (!pass) goto fail;
        ((HGLOBAL *)(scb + 0x160))[i] = pass;
    }
    SCBF(scb, HGLOBAL, 0x22c) = resource("RULE", 350);
    SCBF(scb, HGLOBAL, 0x228) = resource("RULE", 354);
    SCBF(scb, HGLOBAL, SCB_H_DEMI) = resource("DEMI", 258);
    SCBF(scb, HGLOBAL, 0x234) = resource("SPEECH_HDR", 256);
    SCBF(scb, HGLOBAL, SCB_H_REC238) = resource("MOUTH_SHAPES", 257);
    HGLOBAL *inst = calloc(1, sizeof(*inst));
    HGLOBAL *pcmd = calloc(1, sizeof(*pcmd));
    if (!inst || !pcmd || !SCBF(scb, HGLOBAL, SCB_H_DEMI)
        || !SCBF(scb, HGLOBAL, 0x234)) {
        free(inst);
        free(pcmd);
        goto fail;
    }
    *inst = resource("INST", 400);
    *pcmd = resource("PCMD", 500);
    if (!*inst || !*pcmd) {
        free(inst);
        free(pcmd);
        goto fail;
    }
    SCBF(scb, HGLOBAL *, SCB_P_H_INST) = inst;
    SCBF(scb, HGLOBAL *, SCB_P_H_PCMD) = pcmd;
    SCBF(scb, int, 0x248) = 1;
    SCBF(scb, int, 0x250) = 1;
    const SPEECHHDR *speech = SCBF(scb, const SPEECHHDR *, 0x234);
    SCBF(scb, const SPEECHHDR *, SCB_HDR) = speech;
    SCBF(scb, DWORD, 0x3b8) = speech->rate / speech->f4;
    g_prim_pitch = (int)resource_dword(10, "PRIMSTRESSPITCHINCR", 60);
    g_sec_pitch = (int)resource_dword(10, "SECSTRESSPITCHINCR", 30);
    g_de_pitch = (int)resource_dword(10, "DESTRESSPITCHINCR", -30);
    g_prim_speed = (int)resource_dword(10, "PRIMSTRESSSPEEDINCR", -60);
    g_sec_speed = (int)resource_dword(10, "SECSTRESSSPEEDINCR", -40);
    g_de_speed = (int)resource_dword(10, "DESTRESSSPEEDINCR", 40);
    if (load_dictionary(scb)) goto fail;
    return voice;

fail:
    monologue_voice_close(voice);
    return NULL;
}

void monologue_voice_close(monologue_voice_t *voice)
{
    if (!voice) return;
    BYTE *scb = voice->scb;
    if (scb) {
        if (SCBF(scb, int, 0x3b0)) CloseBackend(scb);
        free(SCBF(scb, HGLOBAL *, SCB_P_H_INST));
        free(SCBF(scb, HGLOBAL *, SCB_P_H_PCMD));
        free(SCBF(scb, DICTSLOT *, SCB_D_SLOTS));
        free(scb);
    }
    free(voice);
}

unsigned monologue_voice_sample_rate(const monologue_voice_t *voice)
{
    return voice && voice->scb
        ? SCBF(voice->scb, const SPEECHHDR *, SCB_HDR)->rate : 0;
}

unsigned monologue_voice_bits(const monologue_voice_t *voice)
{
    return voice && voice->scb
        ? SCBF(voice->scb, const SPEECHHDR *, SCB_HDR)->bits : 0;
}

void *monologue_voice_scb(monologue_voice_t *voice)
{
    return voice ? voice->scb : NULL;
}
