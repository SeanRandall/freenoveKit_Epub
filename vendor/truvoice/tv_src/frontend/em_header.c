/* The email header / body analyser (patent US 5,634,084 item 134; REFERENCE 3.10, 8.4).

   FrontEnd_ProcessText 10076f30 cuts the input into lines of at most 220 characters (CR dropped, the '\n' kept) and
   copies each into para_buf; TextReaderControl 1002b270 calls EmailHeaderBodyAnalyzer first. The analyser tidies the
   line, finds its header keyword, decides whether a mail header starts, continues or ends here, rewrites the line
   (through Email_FromLine / Email_AddressList / Email_DateLine) and returns a code that tells TextReaderControl what to do
   with it (see EM_RET_* below).

   para_buf is 500 bytes; the analyser uses its second half, para_buf + 250 (+0x8cc), for the previous line and prev2_line
   (+0x9c6) for the line before that. At exit it shifts them: prev2_line = previous line, previous line = this line. */
#include <stdio.h>
#include <string.h>
#include "frontend.h"

/* What TextReaderControl 1002b270 does with each return code ("flush" = expand the paragraph collected so far in
   text, if it has more than one character; "emit" = FUN_1002e180, append text to the output unexpanded). A line of at
   most one character (a blank line) or a paragraph grown too long skips the switch: text is flushed and the line
   starts the next paragraph. */
enum {
    EM_RET_APPEND = 0,      /* append " " + the line to the paragraph (also the default of the switch) */
    EM_RET_EMIT_EMPTY = 1,  /* flush; emit ""; emit the line as it is (ccMail "Author:") */
    EM_RET_ANNOUNCE = 2,    /* flush; emit ""; emit "Start of Message."; expand the line (not returned) */
    EM_RET_EMIT = 3,        /* flush; emit the line as it is (Date line, separator lines) */
    EM_RET_EXPAND = 4,      /* flush; expand the line as a paragraph of its own (From:, To:, Subject: ...) */
    EM_RET_EXPAND_LAST = 6, /* = 4; returned for the ccMail "Subject:" line, which ends its header */
    /* 5 = 3, 7 = 1, 8: flush; emit the line; emit "End of Forwarded Message." (2, 5, 7 and 8 are never returned) */
    EM_RET_DROP = -1,       /* nothing: the line is dropped, the paragraph goes on */
    EM_RET_START = -2,      /* flush; emit ""; emit "Start of Message."; the line is dropped */
    EM_RET_NONE = -99       /* internal: no header rule took the line */
};

/* The header keywords, as the list at 0x10100434 (copied to [F+0x290] at entry); fe_hdr_key is the 1-based position
   of "#key#" in it. */
static const char hdr_keys[] =
    "#From#From:#>From:#Article#Author:#Subject:#TO:#To:#Date:#Organization:#CC:#Cc:#cc:#BCC:#Priority:#Received:"
    "#Return-Path:#Xref:#Path:#";
enum {
    HK_FROM = 1, HK_FROM_C = 6, HK_QFROM = 12, HK_ARTICLE = 19, HK_AUTHOR = 27, HK_SUBJECT = 35, HK_TO_UC = 44,
    HK_TO = 48, HK_DATE = 52, HK_ORG = 58, HK_CC_UC = 72, HK_CC = 76, HK_CC_LC = 80, HK_BCC = 84, HK_PRIORITY = 89,
    HK_RECEIVED = 99, HK_RETURN_PATH = 109, HK_XREF = 122, HK_PATH = 128
};

#define LINE 250            /* para_buf holds the line in its first 250 bytes and the previous line after them */
#define BUF 2048            /* the original's text buffers are 252 bytes ([F+0x18] and the key at [F+0x114]) */
#define AN_FRAME 0x388      /* the analyser's frame: SUB ESP,0x378 and 4 pushes */
static unsigned char an_frame[AN_FRAME + 2 * BUF];

/* MSVC strncat: a negative count copies the whole string. */
static void ncat(char *d, const char *s, int n)
{
    if (n < 0) strcat(d, s);
    else strncat(d, s, (size_t)n);
}

/* The inlined strcpy: the length is taken first, then the bytes are copied forwards, so a destination inside the
   source (the previous line at para_buf + 250 when the line is longer than 249 characters) repeats the source's head
   and loses its terminator. */
static void copy_fwd(char *d, const char *s)
{
    size_t n = strlen(s) + 1;
    for (size_t i = 0; i < n; i++) d[i] = s[i];
}

/* s ends in '.', but not in "m." (a.m., p.m. get a second period) */
static int ends_in_period(const char *s)
{
    return strcmp(fe_tail(s, 1), ".") == 0 && strncmp(fe_tail(s, 2), "m.", 2) != 0;
}

/* A header starts: count it, mark the header open and name the mail system. */
static void header_start(TextProc *t, const char *system)
{
    t->n_headers++;
    t->in_header = 1;
    strcpy(t->mail_system, system);
}

/* The header ends at this line, which follows an address line ending in ',': ".  " + previous + ".  " + line, or
   ".  " + line when they would not fit in 250. */
static void join_previous(TextProc *t, char *buf)
{
    char *pb = t->para_buf, *prev = t->para_buf + LINE;

    strcpy(buf, ".  ");
    if ((short)strlen(prev) + (short)strlen(pb) + 6 < 250) {
        strcat(buf, prev);
        strcat(buf, ".  ");
    }
    strcat(buf, pb);
    strcpy(pb, buf);
}

/* Drop leading blanks (spaces only). Returns s itself when it is empty, else a static copy (the original: its own
   2000-byte stack buffer, [F+0x10] of its frame; callers copy it at once). No stale reads. @0x100442e0 */
char *fe_str_trim_leading(char *s)
{
    static char buf[4096];

    if ((unsigned short)strlen(s) == 0) return s;
    strcpy(buf, s);
    /* the original rebuilds the string as strlen - 1 blanks overwritten by strncpy(tmp, buf + 1, strlen - 1) */
    while (buf[0] == ' ') memmove(buf, buf + 1, strlen(buf));
    return buf;
}

/* Called for "Subject:" lines and Subject continuations; the body is a bare RET. @0x10072940 */
void fe_Email_SubjectLine(TextProc *t)
{
    (void)t;
}

/* Rewrite a separator line: any line containing "Message Contents", "Reply Separator", "Forward Header" or
   "Forwarded Message" becomes just that phrase with a period ("End of Forwarded Message." when it also contains
   "End"). "Forward Header" and "Forwarded Message" set new_msg_ok (a new message may start). Returns 3 (emit the line as
   it is) when the line changed, else a (the analyser passes 0: append to the paragraph). @0x10074aa0 */
short fe_Email_Separators(TextProc *t, short a)
{
    char *pb = t->para_buf;
    char old[BUF];

    /* The original copies the line into a 252-byte buffer at [F+0x14]: from 252 characters on it overwrites its own
       return address (and from 256 the argument at [F+0x114]), and the DLL crashes on return. Lines that long come
       from the analyser's ":" -> ": " rewrite (REFERENCE 3.4c); the port has room for them. */
    copy_fwd(old, pb);
    if (fe_strpos(pb, "Message Contents")) strcpy(pb, "Message Contents.");
    if (fe_strpos(pb, "Reply Separator")) strcpy(pb, "Reply Separator.");
    if (fe_strpos(pb, "Forward Header")) {
        strcpy(pb, "Forward Header.");
        t->new_msg_ok = 1;
    }
    if (fe_strpos(pb, "Forwarded Message")) {
        if (fe_strpos(pb, "End") == 0) {
            strcpy(pb, "Forwarded Message.");
            t->new_msg_ok = 1;
        } else
            strcpy(pb, "End of Forwarded Message.");
    }
    return strcmp(old, pb) != 0 ? EM_RET_EMIT : a;
}

/* The header / body analyser. State between lines (TextProc; FrontEnd_ProcessText resets it per request):
   - in_header: a mail header is open;
   - new_msg_ok: a new message may start here (the start of the text, or after a forward separator); cleared when an mbox
     "From " or netnews "Article" line starts a header;
   - in_quote: inside a quoted block ('>' lines);
   - mail_system: the mail system ("internet", "netnews", "ccmail"; Email_DateLine sets others);
   - n_headers: headers found so far;
   - hdr_cont_key: the keyword of an address line ending in ',' (set by Email_AddressList): the next line continues it;
   - fe_mode: the mode FrontEnd_ProcessText was called with (always 1; 4 selects a variant that is never used);
   - fe_hdr_key (0x10137100): the keyword of this line (position in hdr_keys), or hdr_cont_key for a continuation line.
   @0x1006fe00 */
short fe_EmailHeaderBodyAnalyzer(TextProc *t)
{
    const unsigned char *fr = fe_frame_of(0x1006fe00);
    char *buf, *key;
    char *pb = t->para_buf, *prev = t->para_buf + LINE;
    const char *r;
    short ret, many, sp, co, si;

    /* The frame is modelled byte for byte: the text buffer [F+0x18] runs on into the key [F+0x114] (harmlessly: the
       key is not used after that), its period test reads the two bytes in front of it when it is empty (stack padding
       the original never writes: the result is at [F+0x12], the "more than one header" flag at [F+0x14]), and the
       key is handed to Email_FromLine with the stack bytes after it. With fe_fixes the two buffers are apart. */
    memset(an_frame, 0, sizeof an_frame);
    if (fr) memcpy(an_frame, fr, AN_FRAME);
    memcpy(an_frame + 0x228, "JANFEBMARAPRMAYJUNJULAUGSEPOCTNOVDEC", 37);
    memcpy(an_frame + 0x318, "January  February March    April    May      June     July     August   September"
                             "October  November December ", 109);
    memcpy(an_frame + 0x210, "SUNMONTUEWEDTHUFRISAT", 22);
    memcpy(an_frame + 0x250, "Sunday   Monday   Tuesday  WednesdayThursday Friday   Saturday ", 64);
    memcpy(an_frame + 0x290, hdr_keys, sizeof hdr_keys);
    if (fe_fixes) {
        buf = (char *)an_frame + AN_FRAME;
        key = buf + BUF;
    } else {
        buf = (char *)an_frame + 0x18;
        key = (char *)an_frame + 0x114;
    }

    many = t->n_headers > 1;

    /* trailing, then leading blanks */
    r = fe_str_trim_spaces(pb);
    if (r != pb) strcpy(pb, r);
    r = fe_str_trim_leading(pb);
    if (r != pb) strcpy(pb, r);

    /* Quoted text: the first '>' line gets ESC[1e in front, the first line after the block ESC[0e (the engine ignores
       both; REFERENCE 8.7). A blank line does not end the block. One '>' is dropped. */
    buf[0] = 0;
    if (pb[0] == '>') {
        if (t->in_quote == 0) {
            t->in_quote = 1;
            strcat(buf, "\n\n \x1b[1e \n\n");
        }
        if (250 - (short)strlen(buf) >= (short)strlen(pb)) {
            strcat(buf, pb + 1);
            strcpy(pb, buf);
        }
    } else if (t->in_quote == 1 && pb[0] != '\n') {
        t->in_quote = 0;
        strcat(buf, "\n\n \x1b[0e \n\n");
        if (250 - (short)strlen(buf) >= (short)strlen(pb)) {
            strcat(buf, pb);
            strcpy(pb, buf);
        }
    }

    fe_str_replace_all(pb, "\t", " ", pb);
    fe_str_replace_all(pb, ".  ", ".. ", pb);
    fe_str_replace_all(pb, "  ", " ", pb);

    /* The key: the first word, cut after its colon when the colon comes at least two characters before the first
       blank (or, with no blank, at positions 2-12); a line without a blank gives its first 13 characters (all but the
       last when shorter). When the key was cut at a colon, every ':' of the line becomes ": ". */
    key[0] = 0;
    sp = fe_strpos(pb, " ");
    co = fe_strpos(pb, ":");
    if (sp == 0 ? co > 1 && co < 13 : co > 0 && sp - 1 > co) {
        ncat(key, pb, co);
        /* The original means to put a blank after the keyword's colon but rewrites every colon of the line:
           "Date:Mon, 16 Jan 95 10:15:00 PST" loses its time ("10: 15: Pacific Standard Time"), and a line full of
           colons grows past 250 (REFERENCE 3.4c); a body line starting with a time ("8:40 a.m.: ...") gets
           "8: 40 a.m.: ". Fixed: only the colon of a header keyword gets a blank. */
        if (fe_fixes) {
            char k[LINE + 8];
            snprintf(k, sizeof k, "#%s#", key);
            if (fe_strpos(hdr_keys, k)) {
                memmove(pb + co + 1, pb + co, strlen(pb + co) + 1);
                pb[co] = ' ';
            }
        } else {
            fe_str_replace_all(pb, ":", "~~", pb);
            fe_str_replace_all(pb, "~~", ": ", pb);
        }
    } else if (sp == 0)
        ncat(key, pb, strlen(pb) < 13 ? (int)strlen(pb) - 1 : 13);
    else
        ncat(key, pb, sp - 1);

    ret = EM_RET_NONE;
    /* The original takes a blank line after "To: a@b.c," for a continuation too. From the second message on the
       blank line is then spoken as "." (Email_AddressList and the period below), so the next line does not follow a
       blank line, the header stays open and the body is dropped up to the next blank line (REFERENCE 3.4c). Fixed:
       a blank line continues nothing. */
    if (t->hdr_cont_key > 0 && fe_strpos(key, ":") < 3 && !(fe_fixes && strlen(pb) <= 1)) {
        /* continuation of an address list: the line takes the keyword of the line before */
        fe_hdr_key = t->hdr_cont_key;
    } else {
        fe_hdr_key = 0;
        t->hdr_cont_key = 0;
        if (strcmp(key, "") != 0) {
            strcpy(buf, "#");
            strcat(buf, key);
            strcat(buf, "#");
            fe_hdr_key = fe_strpos(hdr_keys, buf);
        }
        if (t->fe_mode == 4) {
            /* Mode 4: never used (FrontEnd_ProcessText's two callers, tts_Speak and tts_SaveFile, pass 1). Every
               keyword line opens the header, except a Date line outside one (expanded, without " at"); a line
               without one closes it after a blank line, or after an address line ending in ',' when neither line has
               a colon at 3+ and the line before that was no "cc:" line. The line is emitted as it is. */
            ret = EM_RET_EMIT;
            if (fe_hdr_key > 0) {
                if (fe_hdr_key == HK_DATE && t->in_header == 0) {
                    fe_Email_DateLine(t);
                    fe_str_replace_all(pb, " at", "", pb);
                } else
                    t->in_header = 1;
            } else if (t->in_header == 0)
                t->in_header = 0;
            else if (strlen(prev) <= 1) {
                t->in_header = 0;
                strcpy(buf, ".  ");
                strcat(buf, pb);
                strcpy(pb, buf);
            } else if ((strcmp(fe_tail(prev, 2), ",\n") == 0 || strcmp(fe_tail(prev, 3), ", \n") == 0) &&
                       fe_strpos(prev, ":") < 3 && fe_strpos(pb, ":") < 3 &&
                       fe_strpos(fe_strlwr(t->prev2_line), "cc:") == 0) {
                t->in_header = 0;
                join_previous(t, buf);
            }
            if (t->in_header == 0) ret = fe_Email_Separators(t, 0);
            goto done;
        }
    }

    switch (fe_hdr_key) {
    case HK_FROM:
    case HK_RECEIVED:
    case HK_RETURN_PATH:
        if (strcmp(key, "From") == 0) {
            /* The mbox "From user@host date" line opens a message at the start of the text or after a forward
               separator, after a blank line: it needs an '@' or a time (a second ':' 3 characters after the
               first). */
            si = fe_strpos(pb, ":");
            if (t->new_msg_ok == 0 || t->in_header != 0 || strlen(prev) > 1) break;
            if (fe_strpos(pb, "@") == 0 && (si == 0 || fe_strpos(pb + si, ":") != 3)) break;
            header_start(t, "internet");
            ret = EM_RET_START;
            t->new_msg_ok = 0;
        } else if (t->in_header == 0) {
            header_start(t, "internet");
            ret = EM_RET_START;
        }
        break;
    case HK_FROM_C:
        if (t->in_header == 0) {
            t->n_headers++;
            t->in_header = 1;
        }
        fe_Email_FromLine(t, key);
        ret = EM_RET_EXPAND;
        break;
    case HK_QFROM:
        /* ">From:" never gets here: the quote handling above drops the '>' of any line short enough to fit */
        if (fe_strpos(key, ":") != 0) {
            if (t->in_header != 0) {
                fe_Email_FromLine(t, key);
                ret = EM_RET_EXPAND;
            }
        } else if (t->in_header == 0 && strlen(prev) <= 1) {
            t->in_header = 1;
            ret = EM_RET_START;
            t->n_headers++;
        }
        break;
    case HK_ARTICLE:
        /* A netnews "Article N of group:" line opens a header like "From " does, but the line is then kept as
           text: the -2 is overwritten by 0 */
        if (t->new_msg_ok != 0 && t->in_header == 0 && strlen(prev) <= 1) {
            header_start(t, "netnews");
            ret = EM_RET_START;
            t->new_msg_ok = 0;
        }
        if (t->in_header != 0) ret = EM_RET_APPEND;
        break;
    case HK_AUTHOR:
        /* ccMail: "Author:" opens the header unless it follows a "Subject:" line */
        buf[0] = 0;
        strncat(buf, prev, 8);
        if (strcmp(buf, "Subject:") != 0) {
            header_start(t, "ccmail");
            ret = EM_RET_EMIT_EMPTY;
            fe_Email_AddressList(t);
        } else {
            fe_Email_AddressList(t);
            ret = EM_RET_EMIT;
        }
        break;
    case HK_SUBJECT:
        /* spoken on its own; in ccMail it is the last header line */
        fe_Email_SubjectLine(t);
        ret = EM_RET_EXPAND;
        if (t->in_header != 0 && strcmp(t->mail_system, "ccmail") == 0) {
            t->in_header = 0;
            ret = EM_RET_EXPAND_LAST;
            strcat(pb, ".");
        }
        break;
    case HK_TO_UC:
    case HK_TO:
        /* In the first header the line is dropped; from the second header on (n_headers > 1 at entry) it is spoken.
           Outside a header it opens one (and is dropped) when none was counted yet, else it is spoken. */
        if (!many) {
            if (t->in_header != 0) {
                fe_Email_AddressList(t);
                ret = EM_RET_DROP;
            } else if (t->n_headers >= 1) {
                fe_Email_AddressList(t);
                ret = EM_RET_EXPAND;
            } else {
                t->n_headers++;
                t->in_header = 1;
                fe_Email_AddressList(t);
                ret = EM_RET_DROP;
            }
        } else {
            fe_Email_AddressList(t);
            ret = EM_RET_EXPAND;
        }
        break;
    case HK_DATE:
        /* Outside a header a Date line counts only with a time: two colons, or one colon and AM / PM after it */
        if (t->in_header == 0) {
            short c = fe_strpos(pb + 5, ":");
            const char *q;
            if (c == 0) break;
            q = pb + 5 + c;
            if (fe_strpos(q, ":") == 0 && fe_strpos(q, "AM") == 0 && fe_strpos(q, "PM") == 0) break;
        }
        /* The original compares the first 8 characters of the line before the previous one with the 10-character
           "----------", which never matches, so this "internet" is never set (fixed: 10 characters). */
        buf[0] = 0;
        strncat(buf, t->prev2_line, fe_fixes ? 10 : 8);
        if (t->in_header == 0 && strlen(prev) <= 1 && strcmp(buf, "----------") == 0) header_start(t, "internet");
        if (t->in_header == 0) {
            t->n_headers++;
            t->in_header = 1;
        }
        fe_Email_DateLine(t);
        ret = EM_RET_EMIT;
        break;
    case HK_ORG:
        if (t->in_header != 0) ret = EM_RET_EXPAND;
        break;
    case HK_CC_UC:
    case HK_CC:
    case HK_CC_LC:
    case HK_BCC:
    case HK_PRIORITY:
        if (many) {
            fe_Email_AddressList(t);
            ret = EM_RET_EXPAND;
        } else
            ret = EM_RET_DROP;
        break;
    case HK_XREF:
    case HK_PATH:
        if (t->in_header != 0)
            ret = EM_RET_DROP;
        else {
            t->in_header = 1;
            ret = EM_RET_START;
            t->n_headers++;
        }
        break;
    default:
        break;
    }

    /* A line spoken on its own ends with a period: before its '\n', after its trailing blanks are dropped. "a.m." and
       "p.m." get a second one. */
    if (ret >= 1 && ret <= 4) {
        if (strcmp(fe_tail(pb, 1), "\n") == 0) {
            buf[0] = 0;
            ncat(buf, pb, (int)strlen(pb) - 1);
            r = fe_str_trim_spaces(buf);
            if (r != buf) strcpy(buf, r);
            if (!ends_in_period(buf)) strcat(buf, ".");
            strcat(buf, fe_tail(pb, 1));
            strcpy(pb, buf);
        } else {
            r = fe_str_trim_spaces(pb);
            if (r != pb) strcpy(pb, r);
            if (!ends_in_period(pb)) strcat(pb, ".");
        }
    }

    if (ret == EM_RET_NONE) {
        if (t->in_header != 0) {
            if (strncmp(prev, "Subject:", 8) == 0 && fe_strpos(pb, ": ") == 0) {
                /* a Subject continuation line */
                fe_Email_SubjectLine(t);
                ret = EM_RET_EXPAND;
            } else {
                /* Any other header line is dropped. After a blank line the header has ended: the line is body text
                   (with ".  " in front if no header was counted, which cannot happen). So it has after an address
                   line ending in ",\n" when neither line has a colon at 3 or later: the two lines are joined. */
                ret = EM_RET_DROP;
                if (strlen(prev) <= 1) {
                    t->in_header = 0;
                    if (t->n_headers == 0) {
                        strcpy(buf, ".  ");
                        strcat(buf, pb);
                        strcpy(pb, buf);
                    }
                } else if (strcmp(fe_tail(prev, 2), ",\n") == 0 && fe_strpos(prev, ":") < 3 &&
                           fe_strpos(pb, ":") < 3) {
                    t->in_header = 0;
                    join_previous(t, buf);
                }
            }
        }
        /* body text */
        if (t->in_header == 0) ret = fe_Email_Separators(t, 0);
    }

done:
    /* A line longer than 249 characters (the ": " rewrite can take a 220-character line to 330) runs into the
       previous-line half of para_buf: the copy below then overwrites its own tail with its head and leaves it
       unterminated, so the line spoken and the next two lines' state are garbage (REFERENCE 3.4c). Fixed: cut the
       line at 249. */
    if (fe_fixes && strlen(pb) >= LINE) pb[LINE - 1] = 0;
    copy_fwd(t->prev2_line, prev);
    copy_fwd(prev, pb);
    return ret;
}
