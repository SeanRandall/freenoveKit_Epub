#pragma once

#include <stddef.h>

/* BeST garbles some raw digits ("Arc 2" came out as "arc one" among strange
 * noises), so the OpenBST adapter spells numbers out in English words before
 * BeST reads them.
 *
 * Four digits are said in pairs ("2026" twenty twenty six, "1905" nineteen oh
 * five, "1900" nineteen hundred), other numbers as words, and a run with a
 * leading zero or of more than nine digits one digit at a time. "5.5" is
 * five point five, "-5" at a word start minus five, and a dot inside a word
 * ("notes.txt") is said as "dot", since BeST says nothing at all for an
 * utterance containing one. Writes at most out_len bytes, terminator
 * included. */
void openbst_spell_numbers(const char *in, char *out, size_t out_len);
