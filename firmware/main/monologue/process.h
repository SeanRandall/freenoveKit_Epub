#ifndef RETROREADER_MONOLOGUE_PROCESS_H
#define RETROREADER_MONOLOGUE_PROCESS_H

#include <stdint.h>

static inline uintptr_t _beginthread(void (*function)(void *), unsigned stack,
                                     void *argument)
{
    (void)function;
    (void)stack;
    (void)argument;
    return 0;
}

static inline void _endthread(void) {}

#endif
