// ************************************************************************
// **   FILENAME:          TEXTTALKLIB.CPP
// **
// **   CREATED:           12/8/96
// **   LAST MODIFIED:     12/8/96
// **
// **   VERSION:           1.0
// **   MODULE:            TEXTTALKLIB.CPP
// **
// **   DESCRIPTION:       Dialogic TextTalk Library Compatability Module
// **
// **   Copyright (c) 1996 Dialogic Corp.
// ************************************************************************

#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <io.h>
#include <sys\stat.h>
#include <srllib.h>
#include <dxxxlib.h>
#include "texttalk.h"

// move to textalk.h
#define TTS_NOTIMP         -650

// prototypes for 'not implemented' functions
static int tts_int_notimp ();
static char *tts_null_notimp ();

// typedefs and #defines
typedef void (*func_ptr)(void);

struct func_table
{
  char *fname;
  func_ptr fptr;
};

typedef enum _FUNCPTR_IDS
{
  tts_close_ID = 0,
  tts_geterror_ID,
  tts_getpitch_ID,
  tts_getspeechrate_ID,
  tts_getvoice_ID,
  tts_getvolume_ID,
  tts_init_ID,
  tts_jump_ID,
  tts_open_ID,
  tts_pause_ID,
  tts_phoneme_ID,
  tts_pitch_ID,
  tts_resume_ID,
  tts_reset_ID,
  tts_setcontext_ID,
  tts_speakbuffer_ID,
  tts_speakemail_ID,
  tts_speakfile_ID,
  tts_speechrate_ID,
  tts_stop_ID,
  tts_tracelevel_ID,
  tts_voice_ID,
  tts_volume_ID
} FUNCPTR_IDS;


typedef int       (*ptr_tts_close) (int chdev);
typedef char*     (*ptr_tts_geterror) (int errorCode);
typedef int       (*ptr_tts_getpitch) (int chdev, int *pPitch);
typedef int       (*ptr_tts_getspeechrate) (int chdev, int *pRate);
typedef int       (*ptr_tts_getvoice) (int chdev, int *pVoice);
typedef int       (*ptr_tts_getvolume) (int chdev, int *pVolume);
typedef int       (*ptr_tts_init) (void);
typedef int       (*ptr_tts_jump) (int chdev, int jumpType, int jumpSize);
typedef int       (*ptr_tts_open) (int chdev);
typedef int       (*ptr_tts_pause) (int chdev);
typedef int       (*ptr_tts_phoneme) (int chdev, LPSTR p1, LPSTR p2, long l1, long* pl2);
typedef int       (*ptr_tts_pitch) (int chdev, int pitch);
typedef int       (*ptr_tts_resume) (int chdev);
typedef int       (*ptr_tts_reset) (int chdev);
typedef int       (*ptr_tts_setcontext) (int chdev , int context);
typedef int       (*ptr_tts_speakbuffer) (int chdev, LPSTR pBuf, int mode, DV_TPT* tpt);
typedef int       (*ptr_tts_speakemail)  (int chdev, LPSTR pBuf, int mode, DV_TPT* tpt);
typedef int       (*ptr_tts_speakfile)  (int chdev, LPSTR pBuf, int mode, DV_TPT* tpt);
typedef int       (*ptr_tts_speechrate)  (int chdev, int speechRate);
typedef int       (*ptr_tts_stop) (int chdev);
typedef int       (*ptr_tts_tracelevel) (int chdev, int traceLevel);
typedef int       (*ptr_tts_voice) (int chdev, int voice);
typedef int       (*ptr_tts_volume) (int chdev, int volume);

// Global variables
static struct func_table TTSFunc[] =
{
  {"tts_close", (func_ptr)tts_int_notimp},
  {"tts_geterror", (func_ptr)tts_null_notimp},
  {"tts_getpitch", (func_ptr)tts_int_notimp},
  {"tts_getspeechrate", (func_ptr)tts_int_notimp},
  {"tts_getvoice", (func_ptr)tts_int_notimp},
  {"tts_getvolume", (func_ptr)tts_int_notimp},
  {"tts_init", (func_ptr)tts_int_notimp},
  {"tts_jump", (func_ptr)tts_int_notimp},
  {"tts_open", (func_ptr)tts_int_notimp},
  {"tts_pause", (func_ptr)tts_int_notimp},
  {"tts_phoneme", (func_ptr)tts_int_notimp},
  {"tts_pitch", (func_ptr)tts_int_notimp},
  {"tts_resume", (func_ptr)tts_int_notimp},
  {"tts_reset", (func_ptr)tts_int_notimp},
  {"tts_setcontext", (func_ptr)tts_int_notimp},
  {"tts_speakbuffer", (func_ptr)tts_int_notimp},
  {"tts_speakemail", (func_ptr)tts_int_notimp},
  {"tts_speakfile", (func_ptr)tts_int_notimp},
  {"tts_speechrate", (func_ptr)tts_int_notimp},
  {"tts_stop", (func_ptr)tts_int_notimp},
  {"tts_tracelevel", (func_ptr)tts_int_notimp},
  {"tts_voice", (func_ptr)tts_int_notimp},
  {"tts_volume", (func_ptr)tts_int_notimp}
};

char caMTLibName[] = "TEXTTALK.DLL";
char *pcLibName;

long tts_libinit ()
{
  struct func_table *pTTS = TTSFunc;
  func_ptr pProcAddress;
  static HINSTANCE hLibrary = NULL;
  DWORD dwNumberOfFunctions;

  // Check if it's already open
  if (hLibrary != (HINSTANCE)NULL)
  {
    return -1;
  }

  // Determine which library to load
    pcLibName = caMTLibName;

  dwNumberOfFunctions = sizeof (TTSFunc) / sizeof (TTSFunc[0]);

  // Try to load the library
  hLibrary = LoadLibrary (pcLibName);
  if (hLibrary == (HINSTANCE)NULL)
  {
    return 0;
  }

  // Set up each entry to a default not implemented
  // function and then overwrite the true address if the 
  // function is actually present in the DLL
  while (dwNumberOfFunctions--)
  {
    pProcAddress = (func_ptr)GetProcAddress (hLibrary, pTTS->fname);
    if (pProcAddress)
    {
      pTTS->fptr = pProcAddress;
    }
    else
    {
	// char caTemp[200];
      //sprintf (caTemp, "Function %s not implemented, error %X.", pTTS->fname, GetLastError());
      //MessageBox (HWND_DESKTOP, caTemp, "CCLIB", MB_ICONINFORMATION | MB_OK);
    }
    pTTS++;
  }

  return 0;
}

int tts_int_notimp ()
{
  return TTS_NOTIMP;
}

char *tts_null_notimp ()
{
  return (char *) ("TTS Function has not been implemented");
}



int tts_close (int iChannel)
{
  ptr_tts_close p;

  p = (ptr_tts_close) *TTSFunc[tts_close_ID].fptr;
  return ( (*p) (iChannel));
}

char* tts_geterror (int errorCode)
{
  ptr_tts_geterror p;

  p = (ptr_tts_geterror) *TTSFunc[tts_geterror_ID].fptr;
  return ( (*p) (errorCode));
}

int tts_getspeechrate (int iChannel, int *pSpeechrate)
{
  ptr_tts_getspeechrate p;

  p = (ptr_tts_getspeechrate) *TTSFunc[tts_getspeechrate_ID].fptr;
  return ( (*p) (iChannel, pSpeechrate));
}

int tts_getpitch (int iChannel, int *pPitch)
{
  ptr_tts_getpitch p;

  p = (ptr_tts_getpitch) *TTSFunc[tts_getpitch_ID].fptr;
  return ( (*p) (iChannel, pPitch));
}

int tts_getvoice (int iChannel, int *pVoice)
{
  ptr_tts_getvoice p;

  p = (ptr_tts_getvoice) *TTSFunc[tts_getvoice_ID].fptr;
  return ( (*p) (iChannel, pVoice));
}

int tts_getvolume (int iChannel, int *pVolume)
{
  ptr_tts_getvolume p;

  p = (ptr_tts_getvolume) *TTSFunc[tts_getvolume_ID].fptr;
  return ( (*p) (iChannel, pVolume));
}

int tts_init (void)
{
  ptr_tts_init p;

  p = (ptr_tts_init) *TTSFunc[tts_init_ID].fptr;
  return ( (*p) ());
}

int tts_jump (int iChannel, int jumpType, int jumpSize)
{
  ptr_tts_jump p;

  p = (ptr_tts_jump) *TTSFunc[tts_jump_ID].fptr;
  return ( (*p) (iChannel, jumpType, jumpSize));
}

int tts_open (int iChannel)
{
  ptr_tts_open p;

  p = (ptr_tts_open) *TTSFunc[tts_open_ID].fptr;
  return ( (*p) (iChannel));
}

int tts_pause (int iChannel)
{
  ptr_tts_pause p;

  p = (ptr_tts_pause) *TTSFunc[tts_pause_ID].fptr;
  return ( (*p) (iChannel));
}

int tts_phoneme (int iChannel, LPSTR p1, LPSTR p2, long l1, long *pl2)
{
  ptr_tts_phoneme p;

  p = (ptr_tts_phoneme) *TTSFunc[tts_phoneme_ID].fptr;
  return ( (*p) (iChannel, p1, p2, l1, pl2));
}

int tts_pitch (int iBoard, int pitch)
{
  ptr_tts_pitch p;

  p = (ptr_tts_pitch) *TTSFunc[tts_pitch_ID].fptr;
  return ((*p) (iBoard, pitch));
}

int tts_resume (int iChannel)
{
  ptr_tts_resume p;

  p = (ptr_tts_resume) *TTSFunc[tts_resume_ID].fptr;
  return ((*p) (iChannel));
}

int tts_reset (int iChannel)
{
  ptr_tts_reset p;

  p = (ptr_tts_reset) *TTSFunc[tts_reset_ID].fptr;
  return ((*p) (iChannel));
}

int tts_setcontext (int iChannel, int context)
{
  ptr_tts_setcontext p;

  p = (ptr_tts_setcontext) *TTSFunc[tts_setcontext_ID].fptr;
  return ((*p) (iChannel, context));
}

int tts_speakbuffer (int iChannel, LPSTR pBuffer, int mode, DV_TPT *tpt)
{
  ptr_tts_speakbuffer p;

  p = (ptr_tts_speakbuffer) *TTSFunc[tts_speakbuffer_ID].fptr;
  return ((*p) (iChannel, pBuffer, mode, tpt));
}

int tts_speakemail (int iChannel, LPSTR pBuffer, int mode, DV_TPT *tpt)
{
  ptr_tts_speakemail p;

  p = (ptr_tts_speakemail) *TTSFunc[tts_speakemail_ID].fptr;
  return ((*p) (iChannel, pBuffer, mode, tpt));
}

int tts_speakfile (int iChannel, LPSTR pBuffer, int mode, DV_TPT *tpt)
{
  ptr_tts_speakfile p;

  p = (ptr_tts_speakfile) *TTSFunc[tts_speakfile_ID].fptr;
  return ((*p) (iChannel, pBuffer, mode, tpt));
}

int tts_speechrate (int iChannel, int rate)
{
  ptr_tts_speechrate p;

  p = (ptr_tts_speechrate) *TTSFunc[tts_speechrate_ID].fptr;
  return ((*p) (iChannel, rate));
}

int tts_stop (int iChannel)
{
  ptr_tts_stop p;

  p = (ptr_tts_stop) *TTSFunc[tts_stop_ID].fptr;
  return ((*p) (iChannel));
}

int tts_tracelevel (int iChannel, int traceLevel)
{
  ptr_tts_tracelevel p;

  p = (ptr_tts_tracelevel) *TTSFunc[tts_tracelevel_ID].fptr;
  return ((*p) (iChannel, traceLevel));
}

int tts_voice (int iChannel, int voice)
{
  ptr_tts_voice p;

  p = (ptr_tts_voice) *TTSFunc[tts_voice_ID].fptr;
  return ((*p) (iChannel, voice));
}

int tts_volume (int iChannel, int volume)
{
  ptr_tts_volume p;

  p = (ptr_tts_volume) *TTSFunc[tts_volume_ID].fptr;
  return ((*p) (iChannel, volume));
}

