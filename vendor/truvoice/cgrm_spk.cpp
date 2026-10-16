// Centigram TruVoice TTS Speak App
//
// TruVoice is the successor to the Speech Plus Prose 2000
// since Centigram did acquire Speech Plus in the early 90s
//
// author: Anthony C. Bartman (@rommix0)
// date:   7/8/2026, 7/9/2026

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <stdio.h>
#include "argparse.h"
#include "parse.h"
#include "cgrm.h"

bool is_done = false;
void make_callback() {
	// very simple callback
	// truvoice will call this
	// when speech is done
	is_done = true;
}

int main(int argc, const char* argv[]) {
	// variables to consider
	LPCSTR filename = nullptr;
	bool tmode  = false;
	bool verbo  = false;
	int smprate = TTS_11K;
	int acodec  = TTS_PCM;
	int aformat = WAVE_FILE;

	// parse arguments using argparse.c
	const char* const usages[] = {"cgrm_spk [options] \"text\"", nullptr};
	argparse_option options[] = {
		OPT_HELP(),
		OPT_STRING('f', "filename", &filename, "output to audio file", nullptr, 0, 0),
		OPT_INTEGER('a', "format", &aformat, "audio file format (0: wav, 1: raw, 2: au)", nullptr, 0, 0),
		OPT_INTEGER('c', "codec", &acodec, "audio codec (0: pcm, 1: a-law)", nullptr, 0, 0),
		OPT_INTEGER('s', "samplerate", &smprate, "audio sampling rate (0: 11khz, 1: 8khz)", nullptr, 0, 0),
		OPT_BOOLEAN('m', "test", &tmode, "turn on test mode. overrides text input", nullptr, 0, 0),
		OPT_BOOLEAN('v', "verbose", &verbo, "turn on verbose mode. display text and phoneme stream to standard output", nullptr, 0, 0),
		OPT_END()
	};
	argparse argparse;
	argparse_init(&argparse, options, usages, 0);
	argparse_describe(&argparse, "\nSpeak to audio file or to speaker using Centigram's TruVoice TTS synthesizer.", "");
	if (argc > 1) {
		argc = argparse_parse(&argparse, argc, argv);
	} else {
		argparse_usage(&argparse);
		return 1;
	}

	// get text as final argument
	const char *text = argv[argc-1];

	// make sure codec is set correctly if not PCM
	if (acodec != 0) { acodec = TTS_ALAW; }

	// check if library is loaded.
	// if not, the library is not present.
	if (!cgrmLib) {
		printf("TruVoice not found!\n");
		return 1;
	}

	// Load up functions for TruVoice
	_tts_Open     = (tts_OpenFunc)GetProcAddress(cgrmLib, "tts_Open");
	_tts_Close    = (tts_CloseFunc)GetProcAddress(cgrmLib, "tts_Close");
	_tts_Speak    = (tts_SpeakFunc)GetProcAddress(cgrmLib, "tts_Speak");
	_tts_Phoneme  = (tts_PhonemeFunc)GetProcAddress(cgrmLib, "tts_Phoneme");
	_tts_CallBack = (tts_CallBackFunc)GetProcAddress(cgrmLib, "tts_CallBack");
	_tts_SaveFile = (tts_SaveFileFunc)GetProcAddress(cgrmLib, "tts_SaveFile");

	// Create new TTS handle
	_tts_Open(-1, acodec | smprate, tts_handle);
	_tts_CallBack(tts_handle, TTS_CALLBACK, make_callback, 0);

	// Do parsing stuff here
	char text_to_parse[strlen(text)+1] = {0};
	strcpy(text_to_parse, text);
	parse(text_to_parse);
	
	// do phoneme output stuff in verbose mode
	if (!tmode && verbo) {
		char phoneme_out[strlen(text)*8+1] = {0};
		int phoneme_len = 0;
		err = _tts_Phoneme(tts_handle, text_to_parse, phoneme_out, strlen(text_to_parse)*8+1, &phoneme_len);
		//printf("%i\n", err);
		printf("\ntext:         %s\n", text);
		printf("converted:    %s\n", text_to_parse);
		printf("phonemes:     %s\n\n", phoneme_out);
	}

	// Do synthesis routine stuff here
	if (!filename || !filename[0]) {
		// speak text to speaker (default)
		if (!tmode) {
			_tts_Speak(tts_handle, text_to_parse);
		} else {
			_tts_Speak(tts_handle, test);
		}
		while(!is_done) { ; }
	} else {
		// write to audio file
		OFSTRUCT buffer = {0};
		HFILE aFile = OpenFile(filename, &buffer, OF_CREATE);
		if (!tmode) {
			_tts_SaveFile(tts_handle, aFile, text_to_parse, aformat);
		} else {
			_tts_SaveFile(tts_handle, aFile, test, aformat);
		}
		CloseHandle(aFile);
	}

	// Close TTS handle
	_tts_Close(tts_handle);

	return 0;
}
