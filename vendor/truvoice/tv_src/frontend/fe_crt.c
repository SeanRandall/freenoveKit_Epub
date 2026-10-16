/* Runtime for the front-end port: allocation hooks and the few CRT routines whose MSVC behaviour matters. */
#include <stdlib.h>
#include <string.h>
#include "frontend.h"

int fe_fixes = 1;

const unsigned char *fe_frame_bytes;
unsigned fe_frame_va;
const void *fe_frame_this;

const unsigned char *(*fe_frame_nested)(unsigned va);

const unsigned char *fe_frame_of(unsigned va)
{
    if (fe_frame_va == va) {
        fe_frame_va = 0;
        return fe_fixes ? NULL : fe_frame_bytes;
    }
    return !fe_fixes && fe_frame_nested ? fe_frame_nested(va) : NULL;
}

/* The email stage's one global (0x10137100, frontend.h). It is here so that harness builds have it before the stage is
   ported. */
short fe_hdr_key;

void *(*fe_malloc_hook)(size_t) = malloc;
void (*fe_free_hook)(void *) = free;

void *fe_malloc(size_t n) { return fe_malloc_hook(n); }
void fe_free(void *p) { if (p) fe_free_hook(p); }

char *fe_strdup(const char *s)
{
    char *d = fe_malloc(strlen(s) + 1);
    if (d) strcpy(d, s);
    return d;
}

/* @0x100446e0 */
short fe_strpos(const char *hay, const char *needle)
{
    const char *p = strstr(hay, needle);
    return p ? (short)(p - hay + 1) : 0;
}

char *fe_strlwr(char *s)
{
    for (char *p = s; *p; p++)
        if (*p >= 'A' && *p <= 'Z') *p += 'a' - 'A';
    return s;
}

char *fe_strupr(char *s)
{
    for (char *p = s; *p; p++)
        if (*p >= 'a' && *p <= 'z') *p -= 'a' - 'A';
    return s;
}

/* Drop trailing blanks. The original returns its own 2000-byte stack buffer (callers copy it at once); this
   one uses a static buffer. An empty string is returned as is. @0x10044460 */
char *fe_str_trim_spaces(char *s)
{
    static char buf[2000];
    char *sp;

    if (strlen(s) == 0) return s;
    strcpy(buf, s);
    for (;;) {
        sp = strrchr(buf, ' ');
        if ((unsigned short)strlen(buf) != (sp ? (unsigned short)(sp - buf + 1) : 0)) return buf;
        buf[strlen(buf) - 1] = 0;
        if (strlen(buf) == 0) return buf;
    }
}

/* Copy src to dst and replace every occurrence of find by repl in dst, stopping when the result would pass
   1000 characters. src may be dst. @0x10044560 */
void fe_str_replace_all(const char *src, const char *find, const char *repl, char *dst)
{
    char buf[1000];
    short p;

    if (dst != src) strcpy(dst, src);
    for (;;) {
        p = fe_strpos(dst, find);
        if (p < 1) return;
        if (strlen(repl) + 1 + (strlen(dst) + 1 - strlen(find)) > 1000) return;
        buf[0] = 0;
        strncat(buf, dst, p - 1);
        strcat(buf, repl);
        strncat(buf, dst + p - 1 + strlen(find), strlen(dst) + 1 - strlen(find) - p);
        strcpy(dst, buf);
    }
}
