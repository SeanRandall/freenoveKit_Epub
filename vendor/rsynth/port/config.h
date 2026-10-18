/*
    config.h for the CMake build of rsynth-nvda.

    The original sources expect the config.h that autoconf generates from
    configure.ac. The library build does not run configure, so this file
    stands in for it and defines only what the library sources look at.
    Nothing here selects an audio driver or a dictionary database: the
    library has neither (see docs/02-library-build.md).

    Copyright (c) 2026 the rsynth-nvda contributors.
    Licensed under the GNU Library General Public License, version 2 or
    (at your option) any later version, like the sources it configures.
*/
#ifndef RSYNTH_NVDA_CONFIG_H
#define RSYNTH_NVDA_CONFIG_H

/* useconfig.h: take <stddef.h>, <string.h> and <stdlib.h>. */
#define STDC_HEADERS 1
#define HAVE_LIMITS_H 1

/* Built as a library, not as `say`: the sources' progress and diagnostic
   messages on stdout and stderr are compiled out (patches P2 and P4,
   docs/02-library-build.md). */
#define RSYNTH_LIBRARY 1

#if !defined(_WIN32)
#define HAVE_UNISTD_H 1
#endif

#endif /* RSYNTH_NVDA_CONFIG_H */
