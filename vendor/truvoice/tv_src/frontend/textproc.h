/*
 * TextProc: the per-paragraph state of the email front end (patent US 5,634,084 text processor).
 * Layout matches TV_ENG32.DLL byte for byte (0x3008 bytes, `new`ed by tts_Speak; REFERENCE 3.2, 8.3), so the
 * differential harness (tools/fediff) can run this C on a copy of the DLL's own object.
 * Fields named u_XXXX are not understood yet; they keep their offset.
 */
#ifndef FE_TEXTPROC_H
#define FE_TEXTPROC_H

#include <stddef.h>
#include <stdint.h>

#define FE_MAXWORDS 250

#pragma pack(push, 1)
typedef struct TextProc {
    union {
        char text[2000];          /* +0x0000 the paragraph text; the tokenizer rewrites it in place */
        char *u_0000[500];        /* (the harness compares this region as 500 raw dwords) */
    };
    int16_t line_no;              /* +0x07d0 input line counter (FrontEnd_ProcessText 10076f30) */
    char para_buf[500];           /* +0x07d2 [0..249] the current input line, [250..499] the previous one (the email stage
                                     shifts them per line; a line of 250+ characters runs into the second half) */
    char prev2_line[250];         /* +0x09c6 the line before the previous one */
    char mail_system[10];         /* +0x0ac0 "internet", "netnews", "ccmail", "msmail", "lnmail" (header style) */
    int16_t in_header;            /* +0x0aca a mail header is open */
    int16_t in_quote;             /* +0x0acc inside a quoted (">") block */
    int16_t new_msg_ok;           /* +0x0ace a new message may start here (set at the start and by forward separators) */
    int16_t fper[FE_MAXWORDS];    /* +0x0ad0 FPER  first word of paragraph / sentence */
    int16_t ilc[FE_MAXWORDS];     /* +0x0cc4 ILC   initial lower case */
    int16_t iuc[FE_MAXWORDS];     /* +0x0eb8 IUC   initial upper case */
    int16_t auc[FE_MAXWORDS];     /* +0x10ac AUC   all upper case */
    int16_t num[FE_MAXWORDS];     /* +0x12a0 NUM   number */
    int16_t stprov[FE_MAXWORDS];  /* +0x1494 STPROV state / province */
    int16_t cur_word;             /* +0x1688 index of the last word of the paragraph */
    int16_t lc_period;            /* +0x168a word_lc ended in '.' (Tokenize_Dots 1003fe90) */
    int16_t lc_initcap;           /* +0x168c word_lc had an initial capital only (Tokenize_Dots) */
    char u_168e[2];               /* +0x168e */
    char *word[FE_MAXWORDS];      /* +0x1690 */
    char *qual[FE_MAXWORDS];      /* +0x1a78 QUAL */
    char *punc[FE_MAXWORDS];      /* +0x1e60 PUNC  punctuation after the word */
    char *sepr[FE_MAXWORDS];      /* +0x2248 SEPR  separator after the word */
    char *prepunc[FE_MAXWORDS];   /* +0x2630 PREPUNC */
    char word_lc[60];             /* +0x2a18 current word, lower case */
    char out_text[60];            /* +0x2a54 expansion being built */
    char rule_type[25];           /* +0x2a90 rule name of the record */
    char word_as_typed[60];       /* +0x2aa9 */
    char rec_keyword[60];         /* +0x2ae5 */
    char rec_expansion[60];       /* +0x2b21 */
    char accepted_text[60];       /* +0x2b5d */
    char measure_sfx[60];         /* +0x2b99 */
    char u_2bd5[61];              /* +0x2bd5 text */
    int16_t psnum[FE_MAXWORDS];   /* +0x2c12 PSNUM (likely) */
    int16_t sep_dash[FE_MAXWORDS];/* +0x2e06 the separator after the word was "- " (TextExpander_Paragraph) */
    int16_t fe_mode;              /* +0x2ffa FrontEnd_ProcessText's 5th argument; both callers pass 1 */
    int16_t hdr_cont_key;         /* +0x2ffc header keyword continued by the next line (the line ended in a comma) */
    int16_t n_headers;            /* +0x2ffe mail headers so far (>1: not the first message) */
    char *out_buf;                /* +0x3000 the output text (FrontEnd_ProcessText's buffer; Output_AppendText) */
    uint32_t out_size;            /* +0x3004 its size */
} TextProc;
#pragma pack(pop)

#define FE_CHECK(f, off) _Static_assert(offsetof(TextProc, f) == (off), #f)
FE_CHECK(line_no, 0x7d0);
FE_CHECK(para_buf, 0x7d2);
FE_CHECK(prev2_line, 0x9c6);
FE_CHECK(mail_system, 0xac0);
FE_CHECK(in_header, 0xaca);
FE_CHECK(fper, 0xad0);
FE_CHECK(ilc, 0xcc4);
FE_CHECK(iuc, 0xeb8);
FE_CHECK(auc, 0x10ac);
FE_CHECK(num, 0x12a0);
FE_CHECK(stprov, 0x1494);
FE_CHECK(cur_word, 0x1688);
FE_CHECK(lc_period, 0x168a);
FE_CHECK(lc_initcap, 0x168c);
FE_CHECK(word, 0x1690);
FE_CHECK(qual, 0x1a78);
FE_CHECK(punc, 0x1e60);
FE_CHECK(sepr, 0x2248);
FE_CHECK(prepunc, 0x2630);
FE_CHECK(word_lc, 0x2a18);
FE_CHECK(out_text, 0x2a54);
FE_CHECK(rule_type, 0x2a90);
FE_CHECK(word_as_typed, 0x2aa9);
FE_CHECK(rec_keyword, 0x2ae5);
FE_CHECK(rec_expansion, 0x2b21);
FE_CHECK(accepted_text, 0x2b5d);
FE_CHECK(measure_sfx, 0x2b99);
FE_CHECK(u_2bd5, 0x2bd5);
FE_CHECK(psnum, 0x2c12);
FE_CHECK(sep_dash, 0x2e06);
FE_CHECK(fe_mode, 0x2ffa);
FE_CHECK(out_buf, 0x3000);
FE_CHECK(out_size, 0x3004);
_Static_assert(sizeof(TextProc) == 0x3008, "TextProc size");

#endif
