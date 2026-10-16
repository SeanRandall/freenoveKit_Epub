/* Harness-only bridge: BRIDGE3 runs a not-yet-ported function from the loaded DLL (thiscall) on the TextProc it is
   given. fe_bridge_init points the allocation hooks at the DLL's heap, which bridged code
   would need; tools/fediff replaces them with a private heap. Not part of a standalone build. */
#include "frontend.h"

static unsigned char *dll;
#define VA(a) ((void *)(dll + ((a) - 0x10000000u)))

typedef void (__attribute__((thiscall)) *Rule3)(TextProc *, int, short *);
typedef void (__attribute__((thiscall)) *Rule2)(TextProc *, int);
typedef void (__attribute__((thiscall)) *Rule2s)(TextProc *, int, int);
typedef void *(__cdecl *MallocF)(size_t);
typedef void (__cdecl *FreeF)(void *);

static void *dll_malloc(size_t n) { return ((MallocF)VA(0x10078020))(n); }
static void dll_free(void *p) { ((FreeF)VA(0x10077fb0))(p); }

void fe_bridge_init(unsigned char *dll_base)
{
    dll = dll_base;
    fe_malloc_hook = dll_malloc;
    fe_free_hook = dll_free;
}

#define BRIDGE3(name, va) void fe_##name(TextProc *t, short i, short *r) { ((Rule3)VA(va))(t, i, r); }


