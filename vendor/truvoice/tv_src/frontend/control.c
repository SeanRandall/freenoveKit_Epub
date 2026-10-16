/*
 * The control procedure (patent 132; REFERENCE 3.12). FrontEnd_ProcessText cuts the input into lines and hands each
 * one to TextReaderControl in para_buf. TextReaderControl asks the email stage what the line is
 * (EmailHeaderBodyAnalyzer) and then adds the line to the paragraph in text, expands the paragraph
 * (TextExpander_Paragraph), or writes lines to the output as they are (Output_AppendText).
 */
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "frontend.h"

#define LINE 250   /* para_buf: the current line, then the previous one */

/* FrontEnd_ProcessText's globals. A line of more than 220 characters is kept whole in line_buf and handed out in
   pieces; line_pos is where the next piece starts (0: none pending). */
static short line_pos;     /* 0x10137108 */
static short line_len;     /* 0x1013710c: the length of the piece being measured */
static char *line_buf;     /* 0x10137110 */
static short line_k;       /* 0x10137114 */

/* Only blanks and line ends */
static int blank_piece(const char *s)
{
    for (; *s; s++)
        if (*s != ' ' && *s != '\r' && *s != '\n') return 0;
    return 1;
}

/* Copy the next input line, up to and including its '\n', to line without its CRs, and move *in past it. A line of
   more than 220 characters is copied whole to line_buf, and only its first piece goes to line: up to the last ". "
   within 220 characters, else up to the last blank, else 220 characters. FrontEnd_ProcessText hands out the rest.
   Returns the length of line, 0 at the end of the input, -1 when out of memory. @0x10077300 */
static int read_line(const char **in, char *line)
{
    char buf[250];
    const char *nl;
    short n, i, j = 0;

    memset(buf, 0, sizeof buf);
    *line = 0;
    nl = strchr(*in, '\n');
    n = nl ? (short)(nl - *in + 1) : (short)strlen(*in);
    if (n > 220) {
        line_buf = fe_malloc(n + 1);
        if (!line_buf) return -1;
        *line_buf = 0;
        strncat(line_buf, *in, n);
        line_len = 0;
        /* (+ 2: the search goes on one character after the blank) */
        while ((line_k = fe_strpos(line_buf + line_len, ". ")) != 0 && line_len + line_k <= 220)
            line_len += line_k + 2;
        if (line_len == 0)
            while ((line_k = fe_strpos(line_buf + line_len, " ")) != 0 && line_len + line_k <= 220)
                line_len += line_k + 1;
        n = line_len ? line_len - 1 : 220;
        *line = 0;
        strncat(line, line_buf, n);
        line_pos = n;
    } else if (!strchr(*in, '\n')) {
        strcpy(line, *in);
    } else {
        *line = 0;
        strncat(line, *in, n);
    }
    nl = strchr(*in, '\n');
    if (!nl) {
        *in += strlen(*in);
    } else {
        line[n] = 0;
        *in = nl + 1;
    }
    for (i = 0; i < n; i++)
        if (line[i] != '\r') buf[j++] = line[i];
    strcpy(line, buf);
    return (int)strlen(line);
}

/* The next piece of a long line (read_line): up to the last ". " (and one more blank) within 220 characters, else
   up to the last blank, else the rest, at most 220 characters. Returns its length. */
static short next_piece(TextProc *t)
{
    short n;

    line_len = 0;
    while ((line_k = fe_strpos(line_buf + line_pos + line_len, ". ")) != 0 && line_len + line_k <= 220) {
        line_len += line_k + 1;
        if (line_buf[line_pos + line_len] == ' ') line_len++;
    }
    if (line_len == 0)
        while ((line_k = fe_strpos(line_buf + line_pos + line_len, " ")) != 0 && line_len + line_k <= 220)
            line_len += line_k;
    if (line_len == 0) {
        n = (short)strlen(line_buf) - line_pos;
        if (n > 220) n = 220;
    } else {
        n = line_len;
    }
    t->para_buf[0] = 0;
    strncat(t->para_buf, line_buf + line_pos, n);   /* (read_line drops the CRs of the first piece, not these) */
    line_pos += n;
    if ((short)strlen(line_buf) <= line_pos) {
        line_pos = 0;
        fe_free(line_buf);
    }
    return n;
}

static short run_mode(TextProc *t, short ret)
{
    switch (t->fe_mode) {
    case 0:   /* the text as it is */
        strcpy(t->text, t->para_buf);
        fe_Output_AppendText(t);
        break;
    case 1: case 3: case 4:
        ret = fe_TextReaderControl(t);
        break;
    case 2:
        fe_NumLine_Read(t);
        break;
    }
    return ret;
}

/* Expand the input (in) and append it to out (out_size bytes; the callers pass it zeroed). mode is stored in fe_mode:
   0 copies every line to the output, 2 reads lines as numbers (numline.c), 1, 3 and 4 run the email front end
   (tts_Speak and tts_SaveFile pass 1). Returns -1 when out of memory, else the last TextReaderControl result.
   @0x10076f30 */
short fe_FrontEnd_ProcessText(TextProc *t, const char *in, char *out, uint32_t out_size, short mode)
{
    const char *p = in;
    short ret = 0;
    int r;

    line_pos = 0;
    t->fe_mode = mode;
    t->out_buf = out;
    t->text[0] = 0;
    t->line_no = 0;
    t->out_size = out_size;
    t->hdr_cont_key = 0;
    t->para_buf[LINE] = 0;
    t->in_header = 0;
    t->prev2_line[0] = 0;
    t->n_headers = 0;
    t->mail_system[0] = 0;
    t->new_msg_ok = 1;
    t->in_quote = 0;
    for (;;) {
        if (line_pos == 0) {
            r = read_line(&p, t->para_buf);
            if (r == -1) ret = -1;
            if (r <= 0) break;
        } else if (next_piece(t) == 0) {
            break;
        } else if (fe_fixes && blank_piece(t->para_buf)) {
            /* A long line cut at a blank leaves its line end as the last piece. "\n" alone is a blank line to the
               email stage and TextReaderControl, so a long LF line ends the paragraph ("\r\n" happens not to). */
            continue;
        }
        t->line_no++;
        ret = run_mode(t, ret);
        if (ret == -1) {
            /* the original keeps the rest of a long line allocated */
            if (fe_fixes && line_pos > 0) {
                fe_free(line_buf);
                line_pos = 0;
            }
            return -1;
        }
    }
    if (ret == -1) return -1;
    if (line_pos > 0) fe_free(line_buf);   /* (never: the loop only ends between long lines) */
    if (strlen(t->text) > 1) ret = run_mode(t, ret);   /* the last paragraph (para_buf is empty) */
    return ret;
}

/* A line of nothing but escape sequences (ESC '[', digits, a letter) and blanks, such as the settings line
   tts_Speak and tts_SaveFile put in front of the text */
static int settings_line(const char *s)
{
    int any = 0;

    while (*s) {
        if (*s == ' ' || *s == '\n') {
            s++;
            continue;
        }
        if (s[0] != 0x1b || s[1] != '[') return 0;
        s += 2;
        while (*s >= '0' && *s <= '9') s++;
        if (!isalpha((unsigned char)*s)) return 0;
        s++;
        any = 1;
    }
    return any;
}

/* Expand the paragraph collected so far, if any. Returns -1 when out of memory, else ret or the expander's result. */
#define FLUSH(set)                                                   \
    do {                                                             \
        if (strlen(text) > 1) {                                      \
            short r_ = fe_TextExpander_Paragraph(t);                 \
            if (set) ret = r_;                                       \
            if (r_ == -1) return -1;                                 \
        }                                                            \
    } while (0)

/* One input line (para_buf). The email stage's code says what to do with it and with the paragraph in text (flush:
   expand the paragraph; emit: write text to the output unexpanded; REFERENCE 3.10):
     0 and others  add " " + the line to the paragraph
     -2            flush, emit "" and "Start of Message.", drop the line
     -1            drop the line
     1, 7          flush, emit "" and the line
     2             flush, emit "" and "Start of Message.", expand the line
     3, 5          flush, emit the line
     4, 6          flush, expand the line on its own
     8             flush, emit the line and "End of Forwarded Message."
   A blank line (at most one character) or a paragraph too long to take the line flushes the paragraph whatever the
   code; the line is then expanded on its own (emitted when there was no paragraph). Returns -1 when out of memory,
   else the last expander result (0). @0x1002b270 */
short fe_TextReaderControl(TextProc *t)
{
    char *text = t->text, *line = t->para_buf;
    short ret = 0, code;

    /* (the original has an unused local "P" buffer here) */
    code = fe_EmailHeaderBodyAnalyzer(t);
    /* The settings line is line 1, so the first real line never follows a blank line and a mail that starts with
       an mbox "From " or "Article" line is not recognised. With the fix it does not count as the previous line. */
    if (fe_fixes && settings_line(line)) line[LINE] = 0;
    if (strlen(line) <= 1 || (uint32_t)(500 - strlen(line)) * 2 <= strlen(text)) {
        if (strlen(text) == 0) {
            if (strlen(line) <= 1) {
                strcpy(text, line);
                fe_Output_AppendText(t);
            }
            return ret;
        }
        if (fe_TextExpander_Paragraph(t) == -1) return -1;
        /* A paragraph too long to take the line: the original expands the line on its own whatever the code, so a
           header line the email stage drops (-1) is read, and "Start of Message." and the like are lost. With the
           fix the line goes through the switch below. */
        if (!fe_fixes || strlen(line) <= 1) {
            strcpy(text, line);
            return fe_TextExpander_Paragraph(t);
        }
        text[0] = 0;
    }
    switch (code) {
    case -2:
        FLUSH(1);
        text[0] = 0;
        fe_Output_AppendText(t);
        strcpy(text, "Start of Message.");
        fe_Output_AppendText(t);
        return ret;
    case -1:
        return ret;
    case 1: case 7:
        FLUSH(1);
        text[0] = 0;
        fe_Output_AppendText(t);
        strcpy(text, line);
        fe_Output_AppendText(t);
        return ret;
    case 2:
        FLUSH(0);
        text[0] = 0;
        fe_Output_AppendText(t);
        strcpy(text, "Start of Message.");
        fe_Output_AppendText(t);
        strcpy(text, line);
        ret = fe_TextExpander_Paragraph(t);
        return ret;
    case 3: case 5:
        FLUSH(1);
        strcpy(text, line);
        fe_Output_AppendText(t);
        return ret;
    case 4: case 6:
        FLUSH(0);
        strcpy(text, line);
        ret = fe_TextExpander_Paragraph(t);
        return ret;
    case 8:
        FLUSH(1);
        strcpy(text, line);
        fe_Output_AppendText(t);
        strcpy(text, "End of Forwarded Message.");
        fe_Output_AppendText(t);
        return ret;
    default:
        strcat(text, " ");
        strcat(text, line);
        return ret;
    }
}
