#ifndef FW_TN_CONVERT_H
#define FW_TN_CONVERT_H
/* The clause-to-phonetic-text driver of text normalisation (ROM $B1E6), decompiled (M4.1 spec section 2.2). */
#include "fw.h"

#define TN_PHONETIC_FLUSH       0xFA     /* tn_phon_len at which tn_phonetic is spoken mid-clause */

/* $B1E6: convert cmd_text (boot_clause_len characters) to phonetic text in tn_phonetic: per character the
   deferred commands due, then a verbatim copy (@X), the rules (letters and digits; a word with no vowel before
   its end is spelled), or a copy or spoken name (space and punctuation). A nearly full tn_phonetic is spoken
   mid-clause and the conversion goes on from tn_resume_pos. */
void tn_convert_clause(fw *f);

#endif
