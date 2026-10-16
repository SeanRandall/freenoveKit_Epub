/************************* 
*   TTS limit defines    * 
**************************/

#ifndef __TEXTTALK_H__
#define __TEXTTALK_H__

#define TTS_MAX_SPEECH_RATE      250
#define TTS_MIN_SPEECH_RATE       50

#define TTS_MAX_PITCH            400
#define TTS_MIN_PITCH             50

#define TTS_MAX_VOLUME            16
#define TTS_MIN_VOLUME             0

// English Character Voices
#define PETER_VOICE    0
#define SIDNEY_VOICE   1
#define EDDIE_VOICE    2
#define DOUGLAS_VOICE  3
#define BIFF_VOICE     4
#define AMOS_VOICE     5
#define MELVIN_VOICE   6
#define ALEX_VOICE     7
#define WANDA_VOICE    8
#define JULIA_VOICE    9

// Defines for tts_SetCtx()/tts_GetCtx()
#define TTS_DEFAULT_CTX      0
#define TTS_EMAIL_CTX        1

// Default settings
#define TTS_DEFAULT_SPEECH_RATE  150
#define TTS_DEFAULT_PITCH        85
#define TTS_DEFAULT_VOLUME       5
#define TTS_DEFAULT_VOICE        0

// Jump defines
#define TTS_JUMP_CHARACTER		0
#define TTS_JUMP_WORD			1
#define TTS_JUMP_SENTENCE		2
#define TTS_JUMP_BEGINNING		10
#define TTS_JUMP_END			11

#define TTS_EV_FILE_END      0x01053000

// API function prototypes

#ifdef __cplusplus
    #define DllImport    extern "C"
#else
    #define DllImport    extern
#endif
DECLARE_HANDLE(HINST);


DllImport int tts_close( int );
DllImport char* tts_geterror( int );
DllImport int tts_getspeechrate( int, int* );
DllImport int tts_getpitch( int, int* );
DllImport int tts_getvoice( int, int* );
DllImport int tts_getvolume( int, int* );
DllImport int tts_init ( void );
DllImport int tts_jump(int, int, int);
DllImport int tts_open( int );
DllImport int tts_pause(int);
DllImport int tts_phoneme(int , LPSTR , LPSTR , long , long *);
DllImport int tts_pitch(int, int);
DllImport int tts_resume(int);
DllImport int tts_reset( int );
DllImport int tts_setcontext(int, int);
DllImport int tts_speakbuffer ( int, LPSTR, int, DV_TPT* tptPtr);
DllImport int tts_speakemail( int, LPSTR, int, DV_TPT* tptPtr);
DllImport int tts_speakfile( int, char*, int, DV_TPT* tptPtr);
DllImport int tts_speechrate(int, int);
DllImport int tts_stop( int );
DllImport int tts_tracelevel(int, int );
DllImport int tts_volume(int, int);
DllImport int tts_voice(int, int);

/*
 *     This file contains all the error code #defines and
 *     their string outputs.
 *
 */

#define    TTS_ERROR_START       -600
#define    TTS_NOT_SPEAKING      -601
#define    TTS_INVALID_RCU       -602
#define    TTS_WPM_RANGE         -603
#define    TTS_PITCH_RANGE       -604
#define    TTS_VOLUME_RANGE      -605
#define    TTS_NOT_IDLE          -606
#define    TTS_BAD_FILENAME      -607
#define    TTS_NULL_STRING       -608
#define    TTS_NOT_PAUSED        -609
#define    TTS_PAUSED            -610
#define    TTS_NO_RESOURCES      -611
#define    TTS_ENGINE_ERROR      -612
#define    TTS_DONGLE_ERROR      -613
#define    TTS_LICEN_ERROR       -614
#define    TTS_EMAIL_MALLOC      -615
#define    TTS_TRANS_EMAIL       -616
#define    TTS_HI_PRI_HDLR       -617
#define    TTS_SPEAKING          -618
#define    TTS_NOT_INIT          -619
#define    TTS_NOT_ENABLED       -620
#define    TTS_DX_HANDLE         -621
#define    TTS_DX_BUSY           -622
#define    TTS_ERROR_END         -623


// Internal Engine Error codes

#define TTS_INVALIDHANDLE        -1000
#define TTS_WAVEOUTBUSY          -1001
#define TTS_MAXNUMOPENED         -1002
#define TTS_WAVEOUTBUFFAILED     -1003
#define TTS_WAVEOUTOPENFAILED    -1004
#define TTS_INVALIDSRATE         -1005
#define TTS_INVALIDSFORMAT       -1006
#define TTS_WAVEOUTPROBLEM       -1007
#define TTS_INVALIDFILEHANDLE    -1008
#define TTS_INVALIDHEADERFORMAT  -1009
#define TTS_WRITEERROR           -1010
#define TTS_INVALIDSETTING       -1011
#define TTS_INVALIDCALLBACKTYPE  -1012
#define TTS_INVALIDPARAM	     -1013
#define TTS_OUTOFMEMORY		     -1014
#define TTS_NOTWAVEOUTDEVICE	 -1015
#define TTS_CREATETHREADFAILED	 -1016
#define TTS_NOTENOUGHSPACEINBUFFER -1017
#define TTS_NOTSAVEBUFFERMODE	 -1018
#define TTS_BUFFERTOOSMALL	     -1019
#define TTS_EVENBUFFERSIZENEEDED -1020
#define TTS_NOTBUSY		         -1021
#define TTS_GETDATABUSY		     -1022
#define TTS_LASTDATABUFFER	     -1023
#define TTS_JUMPINPROGRESS	     -1024
#define TTS_FILEOPENERROR		 -1025
#define TTS_FILETOOBIG			 -1026


#endif
