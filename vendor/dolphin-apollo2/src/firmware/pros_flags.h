#ifndef FW_PROS_FLAGS_H
#define FW_PROS_FLAGS_H
/* The bits of pros_sym_flags ($1D00, one byte per tn_phonetic position) that pros_mark_stress and pros_mark_phrases
   set and pros_phrase_gen reads (M4.3b spec 2.6). At a space or punctuation the byte is not bits but the number of
   phonetic symbols of the word after it (pros_mark_phrases $CF74; at position 0 its exit block, $CEF2). */
#define PSF_NUCLEUS     0x07   /* bits 0-2: 7 on each phonetic symbol from its phrase's last stressed symbol to the
                                  phrase's end (mark_phrases $D0E7 ORs in $1C1E; $1C1E is set back to 7 only at
                                  ? ! , : . ; ($CFBF, $CFE9, $D022, $D048), so other punctuation such as - keeps
                                  it) */
#define PSF_FUNC_WORD   0x08   /* the whole byte at a '~', the function-word marker (mark_stress $CBC9); no pass reads
                                  it (pros_phrase_gen tests the symbol) */
#define PSF_UNRELEASED  0x10   /* a stop whose next symbol is a stop (mark_stress $CCEB ORA, $CCED STA): only its first
                                  triple, the closure */
#define PSF_ACCENT      0x20   /* the accent, set with PSF_STRESS as $60 (mark_phrases $D0DC) */
#define PSF_STRESS      0x40   /* a vowel or long symbol in a stressed syllable (mark_stress $CC77); $60 = the accent
                                  (mark_phrases $D0DC) */

#endif
