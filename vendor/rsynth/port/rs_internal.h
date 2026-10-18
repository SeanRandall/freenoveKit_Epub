/*
    rs_internal.h - shared between the files in port/. Not installed.

    Copyright (c) 2026 the rsynth-nvda contributors.
    Licensed under the GNU Library General Public License, version 2 or
    (at your option) any later version.
*/
#ifndef RS_INTERNAL_H
#define RS_INTERNAL_H

#include <stddef.h>
#include "darray.h"

/* dict_builtin.c: the compiled-in CMU dictionary, in the shape of
   dict.c's dict_lookup(): phone codes from phones.def, NUL-terminated,
   malloc'd (xlate_word frees it), or NULL when the word is not there. */
unsigned char *rs_dict_lookup(const char *s, unsigned n);
unsigned rs_dict_size(void);

/* frontend.c: UTF-8 in, the ASCII text the original engine is handed.
   Returns a malloc'd NUL-terminated string, or NULL when out of memory.
   `spell` keeps brackets (spelling names them) and leaves numbers alone. */
char *rs_frontend(const char *utf8, int spell);

#endif /* RS_INTERNAL_H */
