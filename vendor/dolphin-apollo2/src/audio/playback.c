#define WIN32_LEAN_AND_MEAN
#pragma warning(push, 3)
#include <windows.h>
#include <mmsystem.h>
#pragma warning(pop)
#include <stdio.h>
#include <string.h>
#include "audio/playback.h"

static void pcm_format(WAVEFORMATEX *wf, uint32_t rate)
{
    memset(wf, 0, sizeof *wf);
    wf->wFormatTag = WAVE_FORMAT_PCM;
    wf->nChannels = 1;
    wf->nSamplesPerSec = rate;
    wf->wBitsPerSample = 16;
    wf->nBlockAlign = 2;
    wf->nAvgBytesPerSec = rate * 2;
}

bool playback_supported(uint32_t rate)
{
    WAVEFORMATEX wf;
    pcm_format(&wf, rate);
    return waveOutGetNumDevs() > 0 &&
           waveOutOpen(NULL, WAVE_MAPPER, &wf, 0, 0, WAVE_FORMAT_QUERY) == MMSYSERR_NOERROR;
}

bool playback_pcm16(const int16_t *samples, size_t count, uint32_t rate, char *err, size_t errlen)
{
    WAVEFORMATEX wf;
    HWAVEOUT h;
    WAVEHDR hdr;
    MMRESULT r;
    pcm_format(&wf, rate);
    r = waveOutOpen(&h, WAVE_MAPPER, &wf, 0, 0, CALLBACK_NULL);
    if (r != MMSYSERR_NOERROR) {
        snprintf(err, errlen, "waveOutOpen failed (MMRESULT %u)", (unsigned)r);
        return false;
    }
    memset(&hdr, 0, sizeof hdr);
    hdr.lpData = (LPSTR)samples;
    hdr.dwBufferLength = (DWORD)(count * sizeof samples[0]);
    r = waveOutPrepareHeader(h, &hdr, sizeof hdr);
    if (r == MMSYSERR_NOERROR)
        r = waveOutWrite(h, &hdr, sizeof hdr);
    if (r != MMSYSERR_NOERROR) {
        snprintf(err, errlen, "waveOutWrite failed (MMRESULT %u)", (unsigned)r);
        waveOutUnprepareHeader(h, &hdr, sizeof hdr);
        waveOutClose(h);
        return false;
    }
    while (!(hdr.dwFlags & WHDR_DONE))
        Sleep(10);
    waveOutUnprepareHeader(h, &hdr, sizeof hdr);
    waveOutClose(h);
    return true;
}
