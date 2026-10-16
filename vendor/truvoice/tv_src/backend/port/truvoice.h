/*
 * TruVoice's own driver for the engine: what TV_ENG32.DLL does between
 * tts_SaveFile and the engine (REFERENCE 9.3).  It replaces OpenTV's SAPI
 * thread and flat API (tvtts.c), which describe CGRM_EN.DLL.
 */
#ifndef TV_TRUVOICE_H
#define TV_TRUVOICE_H

#include <stdint.h>
#include <stdio.h>

/* The fields of the tts handle (0xbcb8 bytes in the DLL) that the file
 * driver reads, with their handle offsets.  tv_settings_default gives what
 * tts_Open 1005ef70 and Handle_Init 1005ea70 set. */
typedef struct TvSettings {
    uint32_t voice;         /* +0x44   ESC[..V */
    uint32_t sample_rate;   /* +0x48   11025, or 8000 (tts_Open option 1) */
    uint16_t pitch;         /* +0x64   ESC[..p gets half of it */
    uint16_t rate;          /* +0x6a   ESC[..r, words per minute */
    uint16_t volume;        /* +0x7c   0..16; ESC[..a gets 16 - volume */
    uint32_t format;        /* +0x84   0 16-bit, 1 8-bit, 2 mu-law, 3 A-law */
    uint32_t cmode;         /* +0xbc98 ESC[..c */
    uint32_t ctx;           /* +0xbca0 ESC[..X; 4 = email (tts_SetCtx 1) */
} TvSettings;

/* The file types of tts_SaveFile. */
#define TV_FILE_WAVE 0
#define TV_FILE_RAW  1
#define TV_FILE_AU   2

/* Errors, as in cgrm.h. */
#define TV_ERR_NOMEMORY  (-1014)
#define TV_ERR_WRITE     (-1010)
#define TV_ERR_FILETYPE  (-1009)

/* Where tts_SaveFile writes: an HFILE in the DLL (_lwrite / _llseek), a
 * FILE in tvsay (tv_out_file).  write returns the bytes written or -1. */
#define TV_SEEK_SET 0
#define TV_SEEK_CUR 1
typedef struct TvOut {
    int32_t (*write)(void *ctx, const void *p, uint32_t n);
    void (*seek)(void *ctx, int32_t off, int whence);
    void *ctx;
} TvOut;
void tv_out_file(TvOut *o, FILE *f);

/* tts_Open's options: low word 1 for 8000 Hz (else 11025), high word the
 * format. */
void tv_settings_default(TvSettings *s, uint32_t options);

/* BuildEscapePrefix 1005f240: the settings as escape sequences, a line
 * feed, then the text.  out needs strlen(text) + 0x28 bytes. */
void tv_build_prefix(const char *text, char *out, const TvSettings *s);

/* SaveFile_Synthesize 10001990: speak text into o, one second of audio
 * (out_limit) at a time through buf.  Returns the bytes written, or -1. */
int32_t tv_synthesize(const TvOut *o, const char *text, uint8_t *buf, const TvSettings *s);

/* One second of audio at the settings' rate and format: the engine's
 * out_limit in SaveFile_Synthesize and Speak_Thread. */
uint32_t tv_second_bytes(const TvSettings *s);

/* The two drivers' engine helpers (Speak_Thread in src/dll uses them). */
struct Engine;
/* Output_TrimTail 10001d70: drop the trailing silence of the last block. */
void Output_TrimTail(struct Engine *e);
/* Engine_Abort 10024fa0: tts_Stop's end of the input. */
void Engine_Abort(struct Engine *e);

/* Stage 2's working state, which TV_ENG32 keeps in globals that every
 * engine in the process shares (REFERENCE 10.7): an engine's own copy while
 * another engine's is in the globals.  The drivers keep one beside each
 * engine, zeroed, step the engine only through tv_engine_step and call
 * tv_engine_done when they are finished with it. */
typedef struct TvStage2Save {
    int32_t valid;
    int32_t count, run, state, b15, saved, total;
    uint8_t voiced, value, hold;
    uint32_t rnd;
    void *last, *node;
} TvStage2Save;
int32_t tv_engine_step(struct Engine *e, TvStage2Save *s2);
void tv_engine_done(TvStage2Save *s2);

/* Process-wide locks: the engine's steps (tv_engine_step) and the email
 * front end (port/email.c), whose C keeps its buffers in static storage. */
enum { TV_LOCK_ENGINE, TV_LOCK_FRONTEND, TV_LOCKS };
void tv_lock(int which);
void tv_unlock(int which);

/* The email context (port/email.c): full (the prefix and the text) with
 * bytes above 0x7f masked, through the front end (FrontEnd_ProcessText
 * 10076f30, mode 1), between ESC[7F ESC[8F and ESC[7N ESC[8N.  full is
 * modified; the result is malloc'd, NULL when out of memory. */
char *tv_email_text(char *full);

/* What tts_SaveFile and tts_Speak hand to the engine: the prefix and the
 * text, through the front end in the email context.  malloc'd; NULL when
 * out of memory. */
char *tv_prepare_text(const TvSettings *s, const char *text);

/* tts_SaveFile 1005fb50: the header of the file type, the prefix, the
 * synthesis, then the header again with the sizes.  0 or an error. */
int tv_save_file(const TvSettings *s, const TvOut *o, const char *text, int type);

/* Phoneme_Transcribe 10001760 (tts_Phoneme): the phoneme spelling of text,
 * malloc'd in *out.  0, or -1 when out of memory. */
int tv_phonemes(const char *text, char **out);

/* 10024410: text straight into the engine's PreFormat stage, then
 * ESC[0i ESC[C and two blanks (the pronunciation manager's input). */
void Engine_PutText(struct Engine *e, const char *text);

/* The user dictionary (port/userdic.c), the engine's user lexicon.  The
 * DLL loads it from the file the registry names, else english.dic beside
 * the DLL. */
int tv_userdic_load(const char *path);     /* 0 if it cannot be opened */
void tv_userdic_clear(void);
void tv_userdic_reload(const char *path);  /* clear, then load */
int tv_userdic_find(const char *word);     /* index, -1, or -2 (memory) */
void tv_userdic_delete(const char *word);
int tv_userdic_save(const char *path);     /* 0, or -1 */
/* Called once per load, at the first invalid entry, if set. */
extern void (*tv_userdic_warn)(const char *path);
/* The DLL's data for the pronunciation manager: the spelling letters, the
 * help-context pairs and the example string at va. */
const char *tv_userdic_phoneme_chars(void);
const uint32_t *tv_userdic_help(void);
const char *tv_userdic_example(uint32_t va);

#endif
