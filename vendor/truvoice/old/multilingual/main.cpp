/************************************************************************
main.cpp - Minimal command-line TTS driver for the Centigram TruVoice
   SAPI 4.0a engines (CGRM_EN/DE/FR/ES/IT.DLL), used without SAPI/COM
   registration and without touching the Windows registry.

   Mechanism:
     1. LoadLibrary() the engine DLL directly.
     2. Call its exported DllGetClassObject() with the engine's known
        CLSID (taken from the vendor's own .INI install scripts) to get
        an IClassFactory - no CoGetClassObject / registry lookup.
     3. CreateInstance() the engine, passing our own in-process audio
        destination object (CAOut, adapted from Microsoft's AudioSD SDK
        sample) as the aggregation/context pointer - this mirrors what
        ITTSEnum::Select() does internally, minus the registry-backed
        enumerator.
     4. Drive the resulting ITTSCentralW::TextData() to speak text.
************************************************************************/

#include <windows.h>
#include <dbt.h>
#include <mmsystem.h>
#include <ocidl.h>
#include <stdio.h>
#include <initguid.h>
#include <speech.h>
#include "audio.h"

HINSTANCE ghInstance = NULL;
LONG gEnumObjectCount = 0;

/* ---- minimal ITTSBufNotifySink so we know when speaking is done ---- */

class CBufNotify : public ITTSBufNotifySink {
   private:
      ULONG m_cRef;
   public:
      BOOL m_fDone;

      CBufNotify(void) : m_cRef(1), m_fDone(FALSE) {}
      ~CBufNotify(void) {}

      STDMETHODIMP QueryInterface(REFIID riid, LPVOID FAR *ppv)
      {
         if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_ITTSBufNotifySinkW)) {
            *ppv = (LPVOID) this;
            AddRef();
            return NOERROR;
         }
         *ppv = NULL;
         return ResultFromScode(E_NOINTERFACE);
      }

      STDMETHODIMP_(ULONG) AddRef(void) { return ++m_cRef; }
      STDMETHODIMP_(ULONG) Release(void)
      {
         if (--m_cRef == 0) { delete this; return 0; }
         return m_cRef;
      }

      STDMETHODIMP TextDataDone(QWORD qwTime, DWORD dwFlags)
      {
         (void) qwTime; (void) dwFlags;
         m_fDone = TRUE;
         return NOERROR;
      }
      STDMETHODIMP TextDataStarted(QWORD qwTime) { (void) qwTime; return NOERROR; }
      STDMETHODIMP BookMark(QWORD qwTime, DWORD dwMarkID) { (void) qwTime; (void) dwMarkID; return NOERROR; }
      STDMETHODIMP WordPosition(QWORD qwTime, DWORD dwPos) { (void) qwTime; (void) dwPos; return NOERROR; }
   };


/* ---- optional WAV-file audio destination (-o <file.wav>), used instead
   of live playback. Implements the same trio of interfaces CAOut does
   (IAudio/IAudioDest/IAudioMultiMediaDevice) - Select() can't tell the
   difference - but just appends whatever PCM bytes the engine hands us
   to a file, using the WAVEFORMATEX the engine itself supplies via
   WaveFormatSet() to fill in the WAV header at the end. */

class CWavOut;

class CWavIAudio : public IAudio {
   public:
      CWavOut *m_pParent;
      CWavIAudio(CWavOut *pParent) : m_pParent(pParent) {}
      STDMETHODIMP QueryInterface(REFIID riid, LPVOID FAR *ppv);
      STDMETHODIMP_(ULONG) AddRef(void);
      STDMETHODIMP_(ULONG) Release(void);
      STDMETHODIMP Flush(void) { return NOERROR; }
      STDMETHODIMP LevelGet(DWORD *pdw) { if (!pdw) return ResultFromScode(E_INVALIDARG); *pdw = 0xFFFF; return NOERROR; }
      STDMETHODIMP LevelSet(DWORD dw) { (void) dw; return NOERROR; }
      STDMETHODIMP PassNotify(PVOID p, IID iid) { (void) p; (void) iid; return NOERROR; }
      STDMETHODIMP PosnGet(PQWORD pq);
      STDMETHODIMP Claim(void) { return NOERROR; }
      STDMETHODIMP UnClaim(void) { return NOERROR; }
      STDMETHODIMP Start(void) { return NOERROR; }
      STDMETHODIMP Stop(void) { return NOERROR; }
      STDMETHODIMP TotalGet(PQWORD pq);
      STDMETHODIMP ToFileTime(PQWORD pq, FILETIME *pft);
      STDMETHODIMP WaveFormatGet(PSDATA pd);
      STDMETHODIMP WaveFormatSet(SDATA d);
   };

class CWavIAudioDest : public IAudioDest {
   public:
      CWavOut *m_pParent;
      CWavIAudioDest(CWavOut *pParent) : m_pParent(pParent) {}
      STDMETHODIMP QueryInterface(REFIID riid, LPVOID FAR *ppv);
      STDMETHODIMP_(ULONG) AddRef(void);
      STDMETHODIMP_(ULONG) Release(void);
      STDMETHODIMP FreeSpace(DWORD *pdw, BOOL *pf) { if (pdw) *pdw = 0x7FFFFFFF; if (pf) *pf = FALSE; return NOERROR; }
      STDMETHODIMP DataSet(PVOID pBuf, DWORD dwSize);
      STDMETHODIMP BookMark(DWORD dw) { (void) dw; return NOERROR; }
   };

class CWavIAudioMultiMediaDevice : public IAudioMultiMediaDevice {
   public:
      CWavOut *m_pParent;
      CWavIAudioMultiMediaDevice(CWavOut *pParent) : m_pParent(pParent) {}
      STDMETHODIMP QueryInterface(REFIID riid, LPVOID FAR *ppv);
      STDMETHODIMP_(ULONG) AddRef(void);
      STDMETHODIMP_(ULONG) Release(void);
      STDMETHODIMP CustomMessage(UINT u, SDATA d) { (void) u; (void) d; return NOERROR; }
      STDMETHODIMP DeviceNumGet(DWORD *pdw) { if (pdw) *pdw = 0; return NOERROR; }
      STDMETHODIMP DeviceNumSet(DWORD dw) { (void) dw; return NOERROR; }
   };

class CWavOut : public IUnknown {
   public:
      ULONG    m_cRef;
      FILE     *m_pFile;
      QWORD    m_qwTotal;      // bytes written so far (== "position", since writes are synchronous)
      BYTE     m_abFmt[64];    // raw WAVEFORMATEX bytes as given to WaveFormatSet()
      DWORD    m_dwFmtSize;

      CWavIAudio                  m_iAudio;
      CWavIAudioDest              m_iAudioDest;
      CWavIAudioMultiMediaDevice  m_iAudioMMDevice;

      CWavOut(void)
         : m_cRef(0), m_pFile(NULL), m_qwTotal(0), m_dwFmtSize(0),
           m_iAudio(this), m_iAudioDest(this), m_iAudioMMDevice(this)
      {
         memset(m_abFmt, 0, sizeof(m_abFmt));
      }
      ~CWavOut(void) { Close(); }

      BOOL Open(const char *pszPath)
      {
         m_pFile = fopen(pszPath, "wb");
         if (!m_pFile) return FALSE;
         BYTE hdr[44];
         memset(hdr, 0, sizeof(hdr));   // placeholder, patched by Close()
         fwrite(hdr, 1, sizeof(hdr), m_pFile);
         return TRUE;
      }

      /* Idempotent: safe to call more than once (e.g. explicitly, then
         again from the destructor if it ever runs). */
      void Close(void)
      {
         if (!m_pFile) return;

         WAVEFORMATEX wfx;
         memset(&wfx, 0, sizeof(wfx));
         if (m_dwFmtSize >= 16) {
            DWORD dwCopy = (m_dwFmtSize < sizeof(wfx)) ? m_dwFmtSize : sizeof(wfx);
            memcpy(&wfx, m_abFmt, dwCopy);
         } else {
            /* engine never called WaveFormatSet - shouldn't happen in
               practice, but fall back to something playable */
            wfx.wFormatTag = WAVE_FORMAT_PCM;
            wfx.nChannels = 1;
            wfx.nSamplesPerSec = 11025;
            wfx.wBitsPerSample = 8;
            wfx.nBlockAlign = 1;
            wfx.nAvgBytesPerSec = 11025;
         }

         DWORD dwDataSize = (DWORD) m_qwTotal;
         DWORD dwRiffSize = 36 + dwDataSize;
         DWORD dw16 = 16;

         fseek(m_pFile, 0, SEEK_SET);
         fwrite("RIFF", 1, 4, m_pFile);
         fwrite(&dwRiffSize, 4, 1, m_pFile);
         fwrite("WAVE", 1, 4, m_pFile);
         fwrite("fmt ", 1, 4, m_pFile);
         fwrite(&dw16, 4, 1, m_pFile);
         fwrite(&wfx.wFormatTag, 2, 1, m_pFile);
         fwrite(&wfx.nChannels, 2, 1, m_pFile);
         fwrite(&wfx.nSamplesPerSec, 4, 1, m_pFile);
         fwrite(&wfx.nAvgBytesPerSec, 4, 1, m_pFile);
         fwrite(&wfx.nBlockAlign, 2, 1, m_pFile);
         fwrite(&wfx.wBitsPerSample, 2, 1, m_pFile);
         fwrite("data", 1, 4, m_pFile);
         fwrite(&dwDataSize, 4, 1, m_pFile);

         fclose(m_pFile);
         m_pFile = NULL;
      }

      STDMETHODIMP QueryInterface(REFIID riid, LPVOID FAR *ppv)
      {
         if (IsEqualIID(riid, IID_IUnknown))
            *ppv = (LPVOID) (IUnknown*) this;
         else if (IsEqualIID(riid, IID_IAudio))
            *ppv = (LPVOID) (IAudio*) &m_iAudio;
         else if (IsEqualIID(riid, IID_IAudioDest))
            *ppv = (LPVOID) (IAudioDest*) &m_iAudioDest;
         else if (IsEqualIID(riid, IID_IAudioMultiMediaDevice))
            *ppv = (LPVOID) (IAudioMultiMediaDevice*) &m_iAudioMMDevice;
         else {
            *ppv = NULL;
            return ResultFromScode(E_NOINTERFACE);
         }
         ((LPUNKNOWN) *ppv)->AddRef();
         return NOERROR;
      }
      STDMETHODIMP_(ULONG) AddRef(void) { return ++m_cRef; }
      STDMETHODIMP_(ULONG) Release(void)
      {
         if (--m_cRef == 0) { delete this; return 0; }
         return m_cRef;
      }
   };

STDMETHODIMP CWavIAudio::QueryInterface(REFIID riid, LPVOID FAR *ppv) { return m_pParent->QueryInterface(riid, ppv); }
STDMETHODIMP_(ULONG) CWavIAudio::AddRef(void) { return m_pParent->AddRef(); }
STDMETHODIMP_(ULONG) CWavIAudio::Release(void) { return m_pParent->Release(); }

STDMETHODIMP CWavIAudio::PosnGet(PQWORD pq)
{
   if (!pq) return ResultFromScode(E_INVALIDARG);
   *pq = m_pParent->m_qwTotal;
   return NOERROR;
}
STDMETHODIMP CWavIAudio::TotalGet(PQWORD pq)
{
   if (!pq) return ResultFromScode(E_INVALIDARG);
   *pq = m_pParent->m_qwTotal;
   return NOERROR;
}
STDMETHODIMP CWavIAudio::ToFileTime(PQWORD pq, FILETIME *pft)
{
   if (!pq || !pft) return ResultFromScode(E_INVALIDARG);
   DWORD dwABPS = 11025;
   if (m_pParent->m_dwFmtSize >= 16) {
      WAVEFORMATEX *pwfx = (WAVEFORMATEX*) m_pParent->m_abFmt;
      if (pwfx->nAvgBytesPerSec) dwABPS = pwfx->nAvgBytesPerSec;
   }
   *((QWORD*) pft) = (*pq * (QWORD) 10000000) / dwABPS;
   return NOERROR;
}
STDMETHODIMP CWavIAudio::WaveFormatGet(PSDATA pd)
{
   if (!pd) return ResultFromScode(E_INVALIDARG);
   if (!m_pParent->m_dwFmtSize) return ResultFromScode(AUDERR_NEEDWAVEFORMAT);
   LPMALLOC pMalloc;
   if (FAILED(CoGetMalloc(MEMCTX_TASK, &pMalloc))) return ResultFromScode(E_OUTOFMEMORY);
   pd->pData = pMalloc->Alloc(m_pParent->m_dwFmtSize);
   pMalloc->Release();
   if (!pd->pData) return ResultFromScode(E_OUTOFMEMORY);
   memcpy(pd->pData, m_pParent->m_abFmt, m_pParent->m_dwFmtSize);
   pd->dwSize = m_pParent->m_dwFmtSize;
   return NOERROR;
}
STDMETHODIMP CWavIAudio::WaveFormatSet(SDATA d)
{
   if (!d.pData) return ResultFromScode(E_INVALIDARG);
   DWORD dwCopy = (d.dwSize < sizeof(m_pParent->m_abFmt)) ? d.dwSize : sizeof(m_pParent->m_abFmt);
   memcpy(m_pParent->m_abFmt, d.pData, dwCopy);
   m_pParent->m_dwFmtSize = dwCopy;
   return NOERROR;
}

STDMETHODIMP CWavIAudioDest::QueryInterface(REFIID riid, LPVOID FAR *ppv) { return m_pParent->QueryInterface(riid, ppv); }
STDMETHODIMP_(ULONG) CWavIAudioDest::AddRef(void) { return m_pParent->AddRef(); }
STDMETHODIMP_(ULONG) CWavIAudioDest::Release(void) { return m_pParent->Release(); }
STDMETHODIMP CWavIAudioDest::DataSet(PVOID pBuf, DWORD dwSize)
{
   if (!pBuf) return ResultFromScode(E_INVALIDARG);
   if (!m_pParent->m_pFile) return ResultFromScode(E_UNEXPECTED);
   fwrite(pBuf, 1, dwSize, m_pParent->m_pFile);
   m_pParent->m_qwTotal += dwSize;
   return NOERROR;
}

STDMETHODIMP CWavIAudioMultiMediaDevice::QueryInterface(REFIID riid, LPVOID FAR *ppv) { return m_pParent->QueryInterface(riid, ppv); }
STDMETHODIMP_(ULONG) CWavIAudioMultiMediaDevice::AddRef(void) { return m_pParent->AddRef(); }
STDMETHODIMP_(ULONG) CWavIAudioMultiMediaDevice::Release(void) { return m_pParent->Release(); }


/* ---- table of the five TruVoice engines, keyed off the CLSIDs and
   DLL names recorded in the product's own *.INI install scripts ---- */

typedef struct {
   const char *pszLangCode;
   const char *pszDllName;
   const char *pszClsid;
   const char *pszDisplayName;
   } ENGINEINFO;

static const ENGINEINFO g_Engines[] = {
   { "en", "CGRM_EN.DLL", "{1B6BF820-9299-101B-8A19-265D428C60FF}", "Centigram English"  },
   { "de", "CGRM_DE.DLL", "{F2486E80-F687-11cd-8239-0000C0A293FF}", "German TruVoice"     },
   { "fr", "CGRM_FR.DLL", "{A22B8380-9299-101B-8A19-265D428C60FF}", "French TruVoice"     },
   { "es", "CGRM_ES.DLL", "{C2521AC0-9299-101B-8A19-265D428C60FF}", "Spanish TruVoice"    },
   { "it", "CGRM_IT.DLL", "{E20AD500-9299-101B-8A19-265D428C60FF}", "Italian TruVoice"    },
   };
#define NUM_ENGINES (sizeof(g_Engines)/sizeof(g_Engines[0]))


typedef HRESULT (STDAPICALLTYPE *PFNDLLGETCLASSOBJECT)(REFCLSID, REFIID, LPVOID*);


int PrintHR(const char *pszWhat, HRESULT hr)
{
   fprintf(stderr, "%s failed: hr=0x%08X\n", pszWhat, (unsigned) hr);
   return 1;
}


int main(int argc, char *argv[])
{
   setvbuf(stdout, NULL, _IONBF, 0);
   int iArg = 1;
   const ENGINEINFO *pInfo = &g_Engines[0];   // default: English
   const char *pszWavPath = NULL;             // -o <file.wav>: write to file instead of playing
   char szDllPath[MAX_PATH];
   char szExeDir[MAX_PATH];

   /* -l <code> and -o <file.wav> may appear in either order, before the text */
   while (iArg < argc) {
      if (!strcmp(argv[iArg], "-l") && iArg + 1 < argc) {
         DWORD i;
         BOOL fFound = FALSE;
         for (i = 0; i < NUM_ENGINES; i++) {
            if (!_stricmp(argv[iArg + 1], g_Engines[i].pszLangCode)) {
               pInfo = &g_Engines[i];
               fFound = TRUE;
               break;
            }
         }
         if (!fFound) {
            fprintf(stderr, "Unknown language '%s'\n", argv[iArg + 1]);
            return 1;
         }
         iArg += 2;
      } else if (!strcmp(argv[iArg], "-o") && iArg + 1 < argc) {
         pszWavPath = argv[iArg + 1];
         iArg += 2;
      } else {
         break;
      }
   }

   if (iArg >= argc) {
      fprintf(stderr, "usage: speak.exe [-l en|de|fr|es|it] [-o out.wav] \"text to speak\"\n");
      fprintf(stderr, "  -o writes the audio to a WAV file instead of playing it live.\n");
      return 1;
   }

   /* Build the DLL path: same directory as this EXE */
   GetModuleFileNameA(NULL, szExeDir, MAX_PATH);
   {
      char *pSlash = strrchr(szExeDir, '\\');
      if (pSlash) *(pSlash + 1) = '\0'; else szExeDir[0] = '\0';
   }
   wsprintfA(szDllPath, "%s%s", szExeDir, pInfo->pszDllName);

   CLSID clsidEngine;
   {
      WCHAR wszClsid[64];
      MultiByteToWideChar(CP_ACP, 0, pInfo->pszClsid, -1, wszClsid, 64);
      if (FAILED(CLSIDFromString(wszClsid, &clsidEngine))) {
         fprintf(stderr, "Bad CLSID string\n");
         return 1;
      }
   }

   /* Build the wide-character text to speak */
   WCHAR wszText[16000];
   if (argc - iArg >= 1) {
      /* Join remaining argv[] with spaces (mirrors the "speak.exe hello world" sample) */
      char szJoined[8000];
      szJoined[0] = '\0';
      for (int i = iArg; i < argc; i++) {
         if (i > iArg) strcat(szJoined, " ");
         strcat(szJoined, argv[i]);
      }
      MultiByteToWideChar(CP_ACP, 0, szJoined, -1, wszText, 16000);
   }

   printf("Engine : %s\n", pInfo->pszDisplayName);
   printf("DLL    : %s\n", szDllPath);
   printf("CLSID  : %s\n", pInfo->pszClsid);
   printf("Text   : %S\n", wszText);
   if (pszWavPath)
      printf("Output : %s (file, no live playback)\n", pszWavPath);

   if (FAILED(CoInitialize(NULL))) {
      fprintf(stderr, "CoInitialize failed\n");
      return 1;
   }
   ghInstance = GetModuleHandle(NULL);

   /* ---- Step 1: load the engine DLL directly (no registry) ---- */
   HMODULE hEngine = LoadLibraryA(szDllPath);
   if (!hEngine) {
      fprintf(stderr, "LoadLibrary('%s') failed, err=%lu\n", szDllPath, GetLastError());
      CoUninitialize();
      return 1;
   }

   PFNDLLGETCLASSOBJECT pDllGetClassObject =
      (PFNDLLGETCLASSOBJECT) GetProcAddress(hEngine, "DllGetClassObject");
   if (!pDllGetClassObject) {
      fprintf(stderr, "GetProcAddress(DllGetClassObject) failed\n");
      FreeLibrary(hEngine);
      CoUninitialize();
      return 1;
   }

   /* ---- Step 2: get the class factory directly from the DLL ---- */
   IClassFactory *pFactory = NULL;
   HRESULT hr = pDllGetClassObject(clsidEngine, IID_IClassFactory, (LPVOID*) &pFactory);
   if (FAILED(hr) || !pFactory) {
      PrintHR("DllGetClassObject", hr);
      FreeLibrary(hEngine);
      CoUninitialize();
      return 1;
   }
   printf("DllGetClassObject: OK, got IClassFactory\n");

   /* ---- Step 3: build our own audio destination object - either the
      waveOut-based CAOut (live playback; no CoCreateInstance(
      CLSID_MMAudioDest,...), so no dependency on any registered SAPI
      base runtime), or the file-based CWavOut when -o was given.
      Either way Select() just sees an IUnknown and can't tell which. */
   PCAOut  pAOut  = NULL;
   CWavOut *pWavOut = NULL;
   LPUNKNOWN pAudioUnknown = NULL;

   if (pszWavPath) {
      pWavOut = new CWavOut();
      if (!pWavOut || !pWavOut->Open(pszWavPath)) {
         fprintf(stderr, "Could not open '%s' for writing\n", pszWavPath);
         pFactory->Release();
         FreeLibrary(hEngine);
         CoUninitialize();
         return 1;
      }
      pWavOut->AddRef();
      pAudioUnknown = (LPUNKNOWN) pWavOut;
   } else {
      pAOut = new CAOut(NULL, NULL);
      if (!pAOut || !pAOut->FInit(WAVE_MAPPER, NULL, 0)) {
         fprintf(stderr, "CAOut::FInit failed\n");
         pFactory->Release();
         FreeLibrary(hEngine);
         CoUninitialize();
         return 1;
      }
      pAOut->AddRef();
      pAudioUnknown = (LPUNKNOWN) pAOut;
   }

   /* ---- Step 4: the CLSID is actually a per-DLL TTS *enumerator*
      (confirmed by disassembly: its QueryInterface only answers
      IID_IUnknown / IID_ITTSEnumW / IID_ITTSEnumA - CreateInstance()
      never builds the real engine directly, and does not support
      aggregation at all). ITTSEnumW itself carries Next/Skip/Reset/
      Clone/Select (confirmed via its vtable) - no separate ITTSFindW
      needed here. Select(gModeID, &pCentral, pAudio) is the real
      audio hookup point. */
   PITTSENUMW pEnum = NULL;
   hr = pFactory->CreateInstance(NULL, IID_ITTSEnumW, (LPVOID*) &pEnum);
   if (FAILED(hr) || !pEnum) {
      PrintHR("CreateInstance(NULL, IID_ITTSEnumW)", hr);
      pFactory->Release();
      FreeLibrary(hEngine);
      CoUninitialize();
      return 1;
   }
   printf("CreateInstance(IID_ITTSEnumW): OK, got the enumerator\n");

   pFactory->Release();
   pFactory = NULL;

   /* Discover the mode GUID via the enumerator (don't assume it equals
      the outer CLSID) */
   GUID gModeID = clsidEngine;   // fallback if enumeration comes up empty
   {
      TTSMODEINFOW mi;
      memset(&mi, 0, sizeof(mi));
      ULONG uFound = 0;
      HRESULT hrNext = pEnum->Next(1, &mi, &uFound);
      if (SUCCEEDED(hrNext) && uFound) {
         gModeID = mi.gModeID;
         printf("Enumerated mode: mfg=%S product=%S\n", mi.szMfgName, mi.szProductName);
      } else {
         printf("Enum::Next found no modes (hr=0x%08X); using outer CLSID as mode\n", (unsigned) hrNext);
      }
   }

   /* ---- Step 5: the real audio hookup, via Select() ---- */
   PITTSCENTRALW pCentral = NULL;
   hr = pEnum->Select(gModeID, &pCentral, pAudioUnknown);
   if (FAILED(hr) || !pCentral) {
      PrintHR("ITTSEnumW::Select", hr);
      pEnum->Release();
      FreeLibrary(hEngine);
      CoUninitialize();
      return 1;
   }
   printf("Select: OK, got ITTSCentralW with audio wired up\n");

   pEnum->Release();

   /* ---- Step 5: speak the text ---- */
   CBufNotify *pNotify = new CBufNotify();

   SDATA d;
   d.pData = (PVOID) wszText;
   d.dwSize = (DWORD) ((wcslen(wszText) + 1) * sizeof(WCHAR));

   hr = pCentral->TextData(CHARSET_TEXT, 0, d, (PVOID) pNotify, IID_ITTSBufNotifySinkW);
   if (FAILED(hr)) {
      PrintHR("ITTSCentralW::TextData", hr);
   } else {
      printf("TextData: OK, speaking...\n");

      /* Pump messages (CAOut's hidden window needs MM_WOM_* / WM_TIMER)
         until the notify sink tells us we're done, or we time out. */
      DWORD dwStart = GetTickCount();
      while (!pNotify->m_fDone) {
         MSG msg;
         if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
         } else {
            Sleep(10);
         }
         if (GetTickCount() - dwStart > 60000) {
            fprintf(stderr, "(timed out waiting for TextDataDone)\n");
            break;
         }
      }

      /* TextDataDone only means the engine finished handing us audio
         data - CAOut may still have several buffers queued/playing on
         the wave device. Wait for actual playback position to catch
         up to the total bytes written before tearing anything down,
         instead of guessing with a fixed Sleep(). */
      IAudio *pAudioIface = NULL;
      pAudioUnknown->QueryInterface(IID_IAudio, (LPVOID*) &pAudioIface);
      if (pAudioIface) {
         DWORD dwDrainStart = GetTickCount();
         for (;;) {
            MSG msg;
            while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
               TranslateMessage(&msg);
               DispatchMessage(&msg);
            }
            QWORD qwTotal = 0, qwPosn = 0;
            pAudioIface->TotalGet(&qwTotal);
            pAudioIface->PosnGet(&qwPosn);
            if (qwPosn >= qwTotal) break;
            if (GetTickCount() - dwDrainStart > 30000) {
               fprintf(stderr, "(timed out waiting for audio to drain: posn=%I64u total=%I64u)\n",
                  qwPosn, qwTotal);
               break;
            }
            Sleep(20);
         }
         pAudioIface->Release();
      } else {
         Sleep(300);
      }
   }

   pNotify->Release();
   pCentral->Release();

   if (pWavOut) {
      /* Finalize the WAV header now, independent of COM refcounting -
         see the note below on why we don't call Release() here. */
      pWavOut->Close();
      printf("Wrote %I64u bytes of audio to %s\n", pWavOut->m_qwTotal, pszWavPath);
   }

   /* Deliberately not releasing pAOut/pWavOut: some engines (observed
      with the German build) over-release the shared audio object's
      refcount internally, which would make a final Release() here a
      use-after-free. The process is exiting right after anyway, so the
      OS reclaims the wave device/file handle regardless - not worth
      chasing a third-party refcounting bug for a value that's about to
      be torn down. */
   FreeLibrary(hEngine);
   CoUninitialize();

   printf("Done.\n");
   return 0;
}
