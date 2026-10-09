#ifndef RETROREADER_MONOLOGUE_WINDOWS_H
#define RETROREADER_MONOLOGUE_WINDOWS_H

#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define __stdcall
#define __cdecl
#define WINAPI

typedef uint8_t BYTE;
typedef uint16_t WORD;
typedef uint32_t DWORD;
typedef int32_t LONG;
typedef unsigned int UINT;
typedef int BOOL;
typedef void *HANDLE;
typedef void *HGLOBAL;
typedef void *HMODULE;
typedef void *HRSRC;
typedef void *HWND;
typedef void *HINSTANCE;
typedef void *LPVOID;
typedef const char *LPCSTR;
typedef intptr_t INT_PTR;
typedef uintptr_t WPARAM;
typedef intptr_t LPARAM;
typedef int HFILE;

typedef struct {
    BYTE data[136];
} OFSTRUCT;

#define TRUE 1
#define FALSE 0
#define GHND 0
#define GMEM_DDESHARE 0
#define GMEM_MOVEABLE 0
#define MB_OK 0
#define MB_ICONHAND 0
#define MB_TASKMODAL 0
#define MAKEINTRESOURCE(i) ((LPCSTR)(uintptr_t)(WORD)(i))
#define IS_INTRESOURCE(p) ((uintptr_t)(p) <= 0xffffu)

HRSRC FindResourceA(HMODULE module, LPCSTR name, LPCSTR type);
HGLOBAL LoadResource(HMODULE module, HRSRC resource);
void *LockResource(HGLOBAL resource);
int FreeResource(HGLOBAL resource);

static inline HGLOBAL GlobalAlloc(UINT flags, size_t size)
{
    (void)flags;
    return calloc(1, size);
}

static inline void *GlobalLock(HGLOBAL handle) { return handle; }
static inline HGLOBAL GlobalHandle(const void *pointer) { return (HGLOBAL)pointer; }
static inline BOOL GlobalUnlock(HGLOBAL handle) { (void)handle; return TRUE; }
static inline HGLOBAL GlobalFree(HGLOBAL handle) { free(handle); return NULL; }
static inline HGLOBAL GlobalReAlloc(HGLOBAL handle, size_t size, UINT flags)
{
    (void)flags;
    return realloc(handle, size);
}

static inline HWND GetFocus(void) { return NULL; }
static inline int MessageBoxA(HWND window, const char *text,
                              const char *title, UINT flags)
{
    (void)window;
    (void)flags;
    fprintf(stderr, "%s: %s\n", title ? title : "Monologue",
            text ? text : "error");
    return 0;
}

static inline char *lstrcpyA(char *destination, const char *source)
{
    return strcpy(destination, source);
}

#endif
