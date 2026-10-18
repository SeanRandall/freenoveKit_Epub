#include <stdlib.h>

/* Initial embedded port: fall through to the original NRL letter-to-sound
   rules.  The desktop build's 3.75 MB CMU source dictionary is not compiled
   into this firmware; an SD-backed dictionary is being prepared separately. */
unsigned char *rs_dict_lookup(const char *s, unsigned n)
{
    (void)s;
    (void)n;
    return NULL;
}

unsigned rs_dict_size(void) { return 0; }
