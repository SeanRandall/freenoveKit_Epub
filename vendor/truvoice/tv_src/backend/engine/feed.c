/*
 * Feeding the text into the engine's input ring.
 *
 * TV_ENG32's Engine_Feed (REFERENCE 9.3): the same line-by-line tidying as
 * CGRM_EN's, with these differences:
 *  - up to 20 lines per call (CGRM 10);
 *  - "^[" in the text stands for ESC, and "^[[" starts an escape sequence
 *    like ESC [ does;
 *  - ESC[1I / ESC[0I switch a mode (feed_mode_i) under which no line counts as
 *    all capitals;
 *  - a short line gets " ." (CGRM "."), and not when it ends in a
 *    four-character escape sequence: so the settings line tts_SaveFile puts
 *    in front of the text makes no pause (REFERENCE 9.4);
 *  - an empty line gives "." only when a line with text came before it
 *    since the last one did (feed_stop).
 */
#include "engine.h"
#include "crt.h"

static int ends_sentence(char c)
{
    return c == '.' || c == '!' || c == '?' || c == ',' || c == ';';
}

static int is_symbol(uint8_t c)
{
    switch (c) {
    case 0xa2: case 0xa3: case 0xa5: case 0xa7: case 0xa9: case 0xae: case 0xb1:
    case 0xb6: case 0xbc: case 0xbd: case 0xbe:
        return 1;
    default:
        return 0;
    }
}

static void seg_start(InputSeg *seg, const char *p)
{
    seg->len = 0;
    seg->text = (char *)p;
    seg->no_words = 1;
    seg->caps_only = 1;
    seg->flag_c = 0;
}

static void seg_trim(InputSeg *seg, int nul)
{
    char c;

    while (seg->len > 0) {
        c = seg->text[seg->len - 1];
        if (c != ' ' && c != '\t' && !(nul && c == 0))
            break;
        seg->len--;
    }
}

/* Copy (part of) the text into the input ring; *ppos is the position
 * reached, kept across calls while the ring is full.
 *
 * With PreFormat on, the text is taken up to twenty lines at a time and each
 * line is tidied: trailing blanks are dropped, lines without final
 * punctuation that are shorter than 40 characters get " .", lines with no
 * words in a text longer than 130 characters become just a period, and in
 * texts longer than 40 characters ALL-CAPS lines are lower-cased so they are
 * read as words rather than spelled.  ESC [ ... <letter> sequences pass
 * through untouched.  The isalpha / tolower calls get the character sign
 * extended, islower / isalnum zero extended, as in the DLL. */
/* @0x10024960 */
void TV_THISCALL Engine_Feed(Engine *self, const char *text, uint32_t len, uint32_t *ppos)
{
    uint32_t pos = *ppos;
    int16_t nseg, k, j, slen;
    InputSeg *seg;
    const char *q;
    uint8_t c;
    char ch;

    if (!self->preformat) {
        if (pos < len) {
            do {
                if (text[pos] == '^' && text[pos + 1] == '[') {
                    pos += 2;
                    Engine_InPut(self, 0x1b);
                }
                if (!Engine_InPut(self, (uint8_t)text[pos]))
                    break;
            } while (++pos < len);
            if (pos < len)
                goto done;
        }
        goto end;
    }

    if (pos == 0)
        self->feed_pos = 0;
    nseg = 0;
    seg = &self->in_seg[0];
    seg_start(seg, text + self->feed_pos);
    while (self->feed_pos < len) {
        uint32_t p = self->feed_pos;
        c = (uint8_t)text[p];
        if (c == '\n' || c == '\r') {
            self->feed_pos = p + 1;
            if (text[p + 1] == (c == '\n' ? '\r' : '\n'))
                self->feed_pos = p + 2;
            seg_trim(seg, 0);
            if (self->feed_mode_i)
                seg->caps_only = 0;
            nseg++;
            if (nseg > 19)
                break;
            seg = &self->in_seg[nseg];
            seg_start(seg, text + self->feed_pos);
            continue;
        }
        if ((c == 0x1b && text[p + 1] == '[') ||
            (c == '^' && text[p + 1] == '[' && text[p + 2] == '[')) {
            if (c == 0x1b) {
                self->feed_pos = p + 2;
                seg->len += 2;
            } else {
                self->feed_pos = p + 3;
                seg->len += 3;
            }
            seg->no_words = 0;
            q = text + self->feed_pos;
            if (q[0] == '1' && q[1] == 'I') {
                seg->caps_only = 0;
                self->feed_mode_i = 1;
            } else if (q[0] == '0' && q[1] == 'I') {
                seg->caps_only = 0;
                self->feed_mode_i = 0;
            }
            while (self->feed_pos < len) {
                int alpha = tv_isalpha((signed char)text[self->feed_pos]);
                self->feed_pos++;
                seg->len++;
                if (alpha)
                    break;
            }
            continue;
        }
        seg->len++;
        if (c > 0x7f || tv_islower(c))
            seg->caps_only = 0;
        if (c > 0xbf || tv_isalnum(c) || is_symbol(c))
            seg->no_words = 0;
        self->feed_pos++;
    }
    if (nseg != 20) {
        seg_trim(seg, 1);
        if (self->feed_mode_i)
            seg->caps_only = 0;
        if (seg->len > 0)
            nseg++;
    }

    for (k = 0; k < nseg; k++) {
        seg = &self->in_seg[k];
        if (seg->no_words && seg->len != 0 && len > 0x82) {
            Engine_InPut(self, '.');
            Engine_InPut(self, '\n');
            continue;
        }
        slen = seg->len;
        if (slen < 1) {
            if (self->feed_stop) {
                Engine_InPut(self, '.');
                self->feed_stop = 0;
            }
            Engine_InPut(self, '\n');
            continue;
        }
        self->feed_stop = 1;
        j = 0;
        do {
            q = seg->text + j;
            ch = q[0];
            if (!seg->caps_only || len < 0x29) {
                if (ch == '^' && q[1] == '[') {
                    j++;
                    Engine_InPut(self, 0x1b);
                } else {
                    Engine_InPut(self, (uint8_t)ch);
                }
            } else if ((ch == 0x1b && q[1] == '[') || (ch == '^' && q[1] == '[' && q[2] == '[')) {
                /* copy the escape sequence up to its final letter */
                if (ch == '^') {
                    j += 2;
                    Engine_InPut(self, 0x1b);
                }
                for (; j < slen; j++) {
                    if (tv_isalpha((signed char)seg->text[j])) {
                        Engine_InPut(self, (uint8_t)seg->text[j]);
                        break;
                    }
                    Engine_InPut(self, (uint8_t)seg->text[j]);
                }
            } else {
                Engine_InPut(self, (uint8_t)tv_tolower((signed char)ch));
            }
            j++;
        } while (j < slen);
        if (slen < 0x28) {
            q = seg->text + slen;
            if (!ends_sentence(q[-1]) && (slen < 4 || q[-4] != 0x1b)) {
                Engine_InPut(self, ' ');
                Engine_InPut(self, '.');
            }
        }
        Engine_InPut(self, '\n');
    }
    pos = self->feed_pos;
    if (pos < len)
        goto done;
end:
    Engine_InPutEnd(self);
    self->item_done = 1;
done:
    if (*ppos < pos)
        self->st_input_empty = 0;
    *ppos = pos;
}

/* Move pending input on towards the pipeline: through TextIn when it is
 * enabled, otherwise straight into the preformatter (at most 400
 * characters per call).  new_item is set for the first flush of a new
 * TextData item. */
/* @0x10024f20 */
void TV_THISCALL Engine_Flush(Engine *self, int32_t new_item)
{
    TextIn *ti = self->textin;
    int32_t n, c;

    if (self->textin_on && ti != NULL) {
        ti->input_done = self->st_input_empty;
        ti->item_done = self->item_done;
        if ((uint8_t)new_item)
            TextIn_Reset(ti);
        TextIn_Flush(ti, 0);
        return;
    }
    for (n = 0;;) {
        c = Engine_InGet(self);
        if (c < 0) {
            self->st_input_empty = 1;
            break;
        }
        n++;
        Preformat_PutChar(self, (uint8_t)c);
        if (n >= 400)
            break;
    }
    if (n > 0)
        self->st_idle = 0;
}
