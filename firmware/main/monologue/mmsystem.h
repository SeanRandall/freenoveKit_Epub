#ifndef RETROREADER_MONOLOGUE_MMSYSTEM_H
#define RETROREADER_MONOLOGUE_MMSYSTEM_H

#include "windows.h"

typedef struct {
    WORD wFormatTag;
    WORD nChannels;
    DWORD nSamplesPerSec;
    DWORD nAvgBytesPerSec;
    WORD nBlockAlign;
    WORD wBitsPerSample;
    WORD cbSize;
} WAVEFORMATEX;

/* Kept only because the recovered engine's shared globals declare it.  The
 * ESP port never opens a WinMM output device, so the exact capability strings
 * are immaterial, but the object must be a complete type. */
typedef struct tagWAVEOUTCAPSA {
    WORD wMid;
    WORD wPid;
    UINT vDriverVersion;
    char szPname[32];
    DWORD dwFormats;
    WORD wChannels;
    WORD wReserved1;
    DWORD dwSupport;
} WAVEOUTCAPSA, *LPWAVEOUTCAPSA;

typedef void *HWAVEOUT;
typedef struct { int unused; } WAVEHDR;
typedef uintptr_t DWORD_PTR;

#endif
