/*
 * The user dictionary of the replacement DLL (REFERENCE 10.6): where it is
 * kept, and tts_UserDic's "TruVoice Pronunciation Manager" with its Word
 * List and Examples dialogs.  The dictionary itself (the file, the lexicon
 * entries) is port/userdic.c; the dialogs are the DLL's own resources,
 * copied at build time (tools/backend/gen_res.py).
 *
 * The first handle loads the dictionary: the file named by the registry
 * (HKCU\Software\Centigram\TextToSpeech\Dictionary, value ENGLISH), else
 * english.dic beside the DLL.  Closing the manager saves it if it changed;
 * a dictionary the registry does not name yet is offered for saving where
 * it was looked for, then registered.
 *
 * Each dialog runs in a thread of its own with its own message loop, as in
 * the DLL.  The Word, Like and Phone buttons speak through the handle's
 * wave device on the dialog's thread.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "engine.h"
#include "crt.h"
#include "tts.h"
#include <commdlg.h>

/* Dialogs, controls, bitmaps and strings of the DLL's resources */
#define DLG_MAIN      0x6a
#define DLG_WORDLIST  0x6e
#define DLG_EXAMPLES  0x74
#define ID_SAY_WORD   0x6b      /* owner-drawn "speak" buttons */
#define ID_SAY_LIKE   0x6c
#define ID_SAY_PHONE  0x6d
#define ID_WORDLIST   0x40d
#define ID_EXAMPLES   0x410
#define ID_HELP       0x417
#define ID_WORD       0x7d1     /* edit controls */
#define ID_LIKE       0x7d2
#define ID_PRON       0x7e3
#define ID_CLEAR      0x7d5
#define ID_ADD        0x7df
#define ID_CLOSE_ALT  0x9c52    /* closes each dialog, as IDOK / IDCANCEL */
#define ID_LIST       2000      /* the Word List's list box */
#define ID_DELETE     0x7e0
#define ID_EX1        0x41b     /* the Examples' three list boxes */
#define ID_EX2        0x418
#define ID_EX3        0x419
#define BMP_SAY       200
#define BMP_SAY_DOWN  201
#define STR_BAD_WORD      1
#define STR_BAD_PHONEMES  2
#define STR_NEED_BOTH     6
#define STR_ERROR_TITLE   7
#define STR_TOO_LONG      8
#define STR_CORRUPT       9
#define STR_ENGINE_BUSY   10
#define STR_AUDIO_BUSY    11
#define STR_LEXICON       12
#define STR_SAVE_IN       13
#define STR_CANNOT_SAVE   14
#define HELP_CONTEXT_MAIN 0x3f4

#define REG_KEY  "Software\\Centigram\\TextToSpeech\\Dictionary"
#define REG_VAL  "ENGLISH"

/* Set when the lexicon (g_lexicon, engine.h) changes */
/* @0x100bc67c */ extern int32_t g_lexicon_dirty;
/* @0x10025390 (engine/lexicon.c) */
void TV_CDECL UserLex_Add(const char *word, const char *pron);

static char g_dic_path[256];    /* 0x100ff128 */
static char g_help_path[256];   /* 0x100ff228 */
static int g_new_dic;           /* 0x101370e0: the registry names no dictionary */
static HWND g_dlg;              /* 0x100bc640: the pronunciation manager */
static HWND g_list_dlg;         /* 0x100bc644: Word List */
static HWND g_ex_dlg;           /* 0x100bc648: Examples */
static WNDPROC g_edit_proc;     /* 0x10136fe0 */

/* What UserDic_Thread gets (0x101370c8, one for all dialogs) */
typedef struct DlgArgs {
    HINSTANCE inst;
    UINT id;
    HWND parent;
    DLGPROC proc;
    TtsHandle *t;
    const char *title;
} DlgArgs;
static DlgArgs g_args;

/* The Examples' lines, as addresses of the DLL's data: each is a phoneme
 * letter, a tab and an example word.  Inserted at the top, so they show
 * in the opposite order. */
static const uint32_t g_ex1[] = {
    0x100bc954, 0x100bc948, 0x100bc940, 0x100bc938, 0x100bc930, 0x100bc928, 0x100bc920, 0x100bc918,
    0x100bc910, 0x100bc908, 0x100bc900, 0x100bc8f4, 0x100bc8ec, 0x100bc8e4, 0x100bc8d8, 0x100bc8d0,
    0x100bc8c8, 0x100bc8c0, 0x100bc8b8,
};
static const uint32_t g_ex2[] = {
    0x100bc8b0, 0x100bc8a8, 0x100bc89c, 0x100bc894, 0x100bc888, 0x100bc87c, 0x100bc874, 0x100bc86c,
    0x100bc860, 0x100bc858, 0x100bc850, 0x100bc848, 0x100bc83c, 0x100bc834, 0x100bc82c, 0x100bc820,
    0x100bc818, 0x100bc810, 0x100bc808,
};
static const uint32_t g_ex3[] = {
    0x100bc800, 0x100bc7f0, 0x100bc800, 0x100bc7e4, 0x100bc7d8, 0x100bc7d0, 0x100bc7c8, 0x100bc7c0,
    0x100bc7b8, 0x100bc7b0, 0x100bc7a4, 0x100bc79c, 0x100bc794, 0x100bc78c, 0x100bc784, 0x100bc77c,
    0x100bc774, 0x100bc76c, 0x100bc764, 0x100bc758,
};

/* ---- helpers ------------------------------------------------------------ */

static void message(HWND owner, UINT text_id, UINT title_id)
{
    char text[120], title[40];

    LoadStringA(tts_instance, text_id, text, sizeof text);
    LoadStringA(tts_instance, title_id, title, sizeof title);
    MessageBoxA(owner, text, title, MB_ICONEXCLAMATION);
}

/* The directory of the module tv_eng32 with its backslash, else the
 * Windows directory with one.  @0x100272e0 */
static void dll_directory(char *out)
{
    HMODULE m = GetModuleHandleA("tv_eng32");
    int i;

    out[0] = '\0';
    if (m == NULL) {
        GetWindowsDirectoryA(out, 256);
        strcat(out, "\\");
        return;
    }
    GetModuleFileNameA(m, out, 256);
    for (i = (int)strlen(out) - 1; i >= 0; i--)
        if (out[i] == '\\') {
            out[i + 1] = '\0';
            return;
        }
}

/* At the first invalid entry of a dictionary.  (UserDic_Load 100257c0) */
static void warn_corrupt(const char *path)
{
    char fmt[120];
    char *msg;

    msg = (char *)malloc(strlen(path) + 99);
    if (msg == NULL)
        return;
    if (LoadStringA(tts_instance, STR_CORRUPT, fmt, sizeof fmt) != 0) {
        snprintf(msg, strlen(path) + 99, fmt, path);
        MessageBoxA(NULL, msg, NULL, MB_ICONEXCLAMATION | MB_TASKMODAL);
    }
    free(msg);
}

/* Letters only.  @0x10026870 */
static int check_word(const char *s)
{
    int i, n = lstrlenA(s);

    for (i = 0; i < n; i++)
        if (!((s[i] >= 'A' && s[i] <= 'Z') || (s[i] >= 'a' && s[i] <= 'z'))) {
            message(NULL, STR_BAD_WORD, STR_ERROR_TITLE);
            return 0;
        }
    return 1;
}

/* Phoneme letters only.  @0x10026910 */
static int check_phonemes(const char *s)
{
    int i, n = lstrlenA(s);

    for (i = 0; i < n; i++)
        if (tv_strchr(tv_userdic_phoneme_chars(), s[i]) == NULL) {
            message(NULL, STR_BAD_PHONEMES, STR_ERROR_TITLE);
            return 0;
        }
    return 1;
}

/* Trailing blanks off */
static void trim_right(char *s)
{
    int i;

    for (i = (int)strlen(s) - 1; i >= 0 && s[i] == ' '; i--)
        s[i] = '\0';
}

/* ---- where the dictionary is kept --------------------------------------- */

/* Handle_Init 1005ea70: the dictionary for the first handle, and the help
 * file's path for every one. */
void tts_userdic_open(int first)
{
    HKEY k;
    DWORD type, size = sizeof g_dic_path;

    tv_userdic_warn = warn_corrupt;
    if (first) {
        g_new_dic = 1;
        if (RegOpenKeyA(HKEY_CURRENT_USER, REG_KEY, &k) == ERROR_SUCCESS) {
            if (RegQueryValueExA(k, REG_VAL, NULL, &type, (BYTE *)g_dic_path, &size) == ERROR_SUCCESS &&
                tv_userdic_load(g_dic_path))
                g_new_dic = 0;
            RegCloseKey(k);
        }
        if (g_new_dic) {
            dll_directory(g_dic_path);
            strcat(g_dic_path, "english.dic");
            tv_userdic_load(g_dic_path);
        }
    }
    dll_directory(g_help_path);
    strcat(g_help_path, "cgrm_en.hlp");
}

/* Handle_Destruct 1005e9b0: a handle's manager is told to close (which
 * saves), and the last handle takes the dictionary with it. */
void tts_userdic_close(TtsHandle *t, int last)
{
    if (t->dic_window != NULL && IsWindow(t->dic_window))
        SendMessageA(t->dic_window, WM_DESTROY, 0, 0);
    if (last) {
        Lexicon_Lock();
        tv_userdic_clear();
        Lexicon_Unlock();
    }
}

/* The dictionary into path.  One the registry does not name yet is offered
 * for saving there (No: a file dialog), and then registered.
 * @0x100273a0 */
static void userdic_save(char *path)
{
    HCURSOR old = SetCursor(LoadCursorA(NULL, IDC_WAIT));
    char fmt[52], msg[256], title[20] = "", dir[256];
    HKEY k;
    int i;

    if (g_new_dic) {
        if (LoadStringA(tts_instance, STR_SAVE_IN, fmt, 0x32) == 0)
            strcpy(fmt, "Save Lexicon in: %s ?");
        wsprintfA(msg, fmt, path);
        LoadStringA(tts_instance, STR_LEXICON, title, sizeof title);
        if (MessageBoxA(g_dlg, msg, title, MB_YESNO | MB_ICONQUESTION) == IDNO) {
            OPENFILENAMEA ofn;

            memset(&ofn, 0, sizeof ofn);
            ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400A;
            ofn.hwndOwner = g_dlg;
            ofn.nFilterIndex = 1;
            strcpy(msg, "english.dic");
            ofn.lpstrFile = msg;
            ofn.nMaxFile = sizeof msg;
            strcpy(dir, path);
            for (i = (int)strlen(dir) - 1; i >= 0; i--)
                if (dir[i] == '\\') {
                    dir[i] = '\0';
                    break;
                }
            ofn.lpstrInitialDir = dir;
            ofn.lpstrDefExt = "dic";
            ofn.Flags = OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;
            if (!GetSaveFileNameA(&ofn)) {
                /* saved where it was looked for after all */
                if (LoadStringA(tts_instance, STR_SAVE_IN, fmt, 0x32) == 0)
                    strcpy(fmt, "Save Lexicon in: %s");
                wsprintfA(msg, fmt, path);
                MessageBoxA(g_dlg, msg, title, MB_ICONEXCLAMATION);
            } else {
                strcpy(path, ofn.lpstrFile);
            }
        }
    }
    if (tv_userdic_save(path) != 0) {
        if (LoadStringA(tts_instance, STR_CANNOT_SAVE, fmt, 0x32) == 0)
            strcpy(fmt, "Cannot save lexicon file: %s");
        wsprintfA(msg, fmt, path);
        MessageBoxA(g_dlg, msg, NULL, MB_ICONEXCLAMATION);
        return;
    }
    if (g_new_dic && RegCreateKeyA(HKEY_CURRENT_USER, REG_KEY, &k) == ERROR_SUCCESS) {
        RegSetValueExA(k, REG_VAL, 0, REG_SZ, (const BYTE *)path, (DWORD)strlen(path) + 1);
        RegCloseKey(k);
        g_new_dic = 0;
    }
    SetCursor(old);
}

/* ---- the dialogs' threads ------------------------------------------------ */

/* A dialog and its message loop.  @0x1005e880 */
static DWORD WINAPI dialog_thread(void *arg)
{
    DlgArgs *a = (DlgArgs *)arg;
    TtsHandle *t = a->t;
    HWND w;
    MSG m;

    ResetEvent(t->dic_event);
    w = CreateDialogParamA(a->inst, MAKEINTRESOURCEA(a->id), a->parent, a->proc, (LPARAM)t);
    if (a->id == DLG_MAIN) {
        g_dlg = w;
        t->dic_window = w;
    } else if (a->id == DLG_WORDLIST) {
        g_list_dlg = w;
    } else if (a->id == DLG_EXAMPLES) {
        g_ex_dlg = w;
    }
    if (a->title != NULL)
        SetWindowTextA(w, a->title);
    SetEvent(t->dic_event);
    while (GetMessageA(&m, NULL, 0, 0) > 0)
        if (!IsDialogMessageA(w, &m)) {
            TranslateMessage(&m);
            DispatchMessageA(&m);
        }
    return 0;
}

static int start_dialog(UINT id, HWND parent, DLGPROC proc, TtsHandle *t)
{
    HANDLE th;
    DWORD tid;

    g_args.inst = tts_instance;
    g_args.id = id;
    g_args.parent = parent;
    g_args.proc = proc;
    g_args.t = t;
    g_args.title = NULL;
    th = CreateThread(NULL, 4000, dialog_thread, &g_args, 0, &tid);
    if (th == NULL)
        return 0;
    CloseHandle(th);
    Sleep(0);
    return 1;
}

/* To the front where it is.  @0x10025e30 */
static void bring_up(HWND w)
{
    RECT r;

    GetWindowRect(w, &r);
    SetWindowPos(w, NULL, r.left, r.top, 0, 0, SWP_NOSIZE | SWP_SHOWWINDOW);
}

/* ---- Word List ---------------------------------------------------------- */

/* @0x10025e70 */
static void wordlist_fill(HWND dlg)
{
    uint32_t i;

    SendDlgItemMessageA(dlg, ID_LIST, LB_RESETCONTENT, 0, 0);
    for (i = 0; i < g_lexicon_count; i++)
        SendDlgItemMessageA(dlg, ID_LIST, LB_ADDSTRING, 0, (LPARAM)g_lexicon[i].word);
    EnableWindow(GetDlgItem(dlg, ID_DELETE), g_lexicon_count != 0);
}

/* The selected word's entry into the manager.  @0x100271f0 */
static void wordlist_select(HWND dlg)
{
    char word[100];
    LRESULT i;
    int k;

    memset(word, 0, sizeof word);
    i = SendDlgItemMessageA(dlg, ID_LIST, LB_GETCURSEL, 0, 0);
    SendDlgItemMessageA(dlg, ID_LIST, LB_GETTEXT, (WPARAM)i, (LPARAM)word);
    k = tv_userdic_find(word);
    if (k >= 0) {
        SetDlgItemTextA(g_dlg, ID_WORD, g_lexicon[k].word);
        SetDlgItemTextA(g_dlg, ID_LIKE, "");
        SetDlgItemTextA(g_dlg, ID_PRON, g_lexicon[k].pron);
    }
}

/* @0x10026ba0 */
static void wordlist_delete(HWND dlg)
{
    char word[100];
    LRESULT i;

    i = SendDlgItemMessageA(dlg, ID_LIST, LB_GETCURSEL, 0, 0);
    SendDlgItemMessageA(dlg, ID_LIST, LB_GETTEXT, (WPARAM)i, (LPARAM)word);
    tv_userdic_delete(word);
    if (g_list_dlg != NULL)
        wordlist_fill(g_list_dlg);
}

/* @0x10025f00 */
static INT_PTR CALLBACK wordlist_proc(HWND dlg, UINT msg, WPARAM wp, LPARAM lp)
{
    (void)lp;
    switch (msg) {
    case WM_DESTROY:
        g_list_dlg = NULL;
        PostQuitMessage(0);
        return 0;
    case WM_INITDIALOG:
        wordlist_fill(dlg);
        return 1;
    case WM_COMMAND:
        if (LOWORD(wp) == ID_LIST && HIWORD(wp) == LBN_SELCHANGE) {
            wordlist_select(dlg);
            return 1;
        }
        if (wp != IDCANCEL) {
            if (wp == ID_DELETE) {
                wordlist_delete(dlg);
                return 1;
            }
            if (wp != ID_CLOSE_ALT)
                return 0;
        }
        g_list_dlg = NULL;
        DestroyWindow(dlg);
        return 1;
    }
    return 0;
}

/* ---- Examples ----------------------------------------------------------- */

static void examples_add(HWND dlg, int id, const uint32_t *va, size_t n)
{
    HWND lb = GetDlgItem(dlg, id);
    int tab = 0x12;
    size_t i;

    SendMessageA(lb, LB_SETTABSTOPS, 1, (LPARAM)&tab);
    for (i = 0; i < n; i++)
        SendMessageA(lb, LB_INSERTSTRING, 0, (LPARAM)tv_userdic_example(va[i]));
}

/* @0x10025a50 */
static void examples_fill(HWND dlg)
{
    examples_add(dlg, ID_EX1, g_ex1, sizeof g_ex1 / sizeof *g_ex1);
    examples_add(dlg, ID_EX2, g_ex2, sizeof g_ex2 / sizeof *g_ex2);
    examples_add(dlg, ID_EX3, g_ex3, sizeof g_ex3 / sizeof *g_ex3);
}

/* One selection across the three lists.  @0x100277c0 */
static void examples_select(HWND dlg, int id)
{
    if (id != ID_EX1)
        SendDlgItemMessageA(dlg, ID_EX1, LB_SETCURSEL, (WPARAM)-1, 0);
    if (id != ID_EX2)
        SendDlgItemMessageA(dlg, ID_EX2, LB_SETCURSEL, (WPARAM)-1, 0);
    if (id != ID_EX3)
        SendDlgItemMessageA(dlg, ID_EX3, LB_SETCURSEL, (WPARAM)-1, 0);
}

/* A double click adds the line's phoneme letter to the pronunciation.
 * (The DLL can also add it to the word, which nothing asks for.)
 * @0x10027700 */
static void examples_copy(HWND dlg, int id)
{
    char line[80], text[72];
    LRESULT i;
    size_t n;

    memset(line, 0, sizeof line);
    i = SendDlgItemMessageA(dlg, id, LB_GETCURSEL, 0, 0);
    SendDlgItemMessageA(dlg, id, LB_GETTEXT, (WPARAM)i, (LPARAM)line);
    GetDlgItemTextA(g_dlg, ID_PRON, text, 0x46);
    n = strlen(text);
    if (n < 0x45 && line[1] != ' ') {
        text[n] = line[1];
        text[n + 1] = '\0';
        SetDlgItemTextA(g_dlg, ID_PRON, text);
    }
}

/* @0x10027830 */
static INT_PTR CALLBACK examples_proc(HWND dlg, UINT msg, WPARAM wp, LPARAM lp)
{
    WORD code = HIWORD(wp), id = LOWORD(wp);

    (void)lp;
    switch (msg) {
    case WM_DESTROY:
        g_ex_dlg = NULL;
        PostQuitMessage(0);
        return 0;
    case WM_INITDIALOG:
        examples_fill(dlg);
        return 1;
    case WM_COMMAND:
        if (code == LBN_DBLCLK || code == LBN_SELCHANGE) {
            if (id != ID_EX1 && id != ID_EX2 && id != ID_EX3)
                return 0;
            examples_select(dlg, id);
            if (code == LBN_DBLCLK)
                examples_copy(dlg, id);
        }
        if (wp != 0 && wp < 3) {
            DestroyWindow(dlg);
            g_ex_dlg = NULL;
            return 1;
        }
        return 0;
    }
    return 0;
}

/* ---- the pronunciation manager ------------------------------------------ */

/* The Word and Like buttons: the field spoken, and its phoneme spelling
 * into Pronunciation.  The Phone button: Pronunciation spoken as phonemes.
 * @0x10026c00 (Word), 0x10026e00 (Like), 0x10027000 (Phone) */
static void say_field(TtsHandle *t, int id)
{
    char text[80], buf[80];
    char *ph = NULL;
    int phone = id == ID_PRON;
    size_t n;

    memset(text, 0, sizeof text);
    GetDlgItemTextA(g_dlg, id, text, 0x46);
    n = strlen(text);
    if (n == 0)
        return;
    if (text[n - 1] == ' ') {
        trim_right(text);
        SetDlgItemTextA(g_dlg, id, text);
        if (text[0] == '\0')
            return;
    }
    if (!(phone ? check_phonemes(text) : check_word(text)))
        return;
    if (t->speaking) {
        LoadStringA(tts_instance, STR_ENGINE_BUSY, buf, sizeof buf);
        MessageBoxA(g_dlg, buf, NULL, MB_ICONEXCLAMATION);
        return;
    }
    if (tts_audio_open(t) != 0) {
        LoadStringA(tts_instance, STR_AUDIO_BUSY, buf, sizeof buf);
        MessageBoxA(g_dlg, buf, NULL, MB_ICONEXCLAMATION);
        return;
    }
    strcat(text, ".");
    if (phone) {
        /* ESC[1I: phoneme input, "&" the start of a word */
        strcpy(buf, "\x1b[1I&");
        strcat(buf, text);
        tts_speak_transcribe(t, buf, &ph);
        free(ph);
        return;
    }
    tv_strlwr(text);
    tts_speak_transcribe(t, text, &ph);
    if (ph != NULL) {
        char *p = ph;

        n = strlen(ph);
        if (n >= 1 && ph[n - 1] == '.')
            ph[n - 1] = '\0';
        if (n >= 2 && ph[n - 2] == 'p')
            ph[n - 2] = '\0';
        if (*p == '&' || *p == '%')
            p++;
        SetDlgItemTextA(g_dlg, ID_PRON, p);
        free(ph);
    }
}

/* @0x10027290 */
static void clear_fields(HWND dlg)
{
    SetDlgItemTextA(dlg, ID_WORD, "");
    SetDlgItemTextA(dlg, ID_LIKE, "");
    SetDlgItemTextA(dlg, ID_PRON, "");
    SetFocus(GetDlgItem(dlg, ID_WORD));
}

/* The word and its pronunciation into the lexicon.  @0x100269b0 */
static void add_entry(HWND dlg)
{
    char word[80], pron[80];

    memset(word, 0, sizeof word);
    memset(pron, 0, sizeof pron);
    GetDlgItemTextA(dlg, ID_WORD, word, sizeof word);
    GetDlgItemTextA(dlg, ID_PRON, pron, sizeof pron);
    trim_right(word);
    trim_right(pron);
    if (word[0] == '\0' || pron[0] == '\0') {
        message(NULL, STR_NEED_BOTH, STR_ERROR_TITLE);
        return;
    }
    if (strlen(word) >= 40 || strlen(pron) >= 40) {
        message(NULL, STR_TOO_LONG, STR_ERROR_TITLE);
        return;
    }
    if (!check_word(word) || !check_phonemes(pron))
        return;
    UserLex_Add(word, pron);
    if (g_list_dlg != NULL)
        wordlist_fill(g_list_dlg);
    clear_fields(dlg);
}

/* The speak buttons' faces.  @0x10025fc0 */
static void draw_button(const DRAWITEMSTRUCT *d)
{
    HBITMAP b;
    HDC dc;
    HGDIOBJ old;

    if (d->CtlType != ODT_BUTTON)
        return;
    if (d->CtlID != ID_SAY_WORD && d->CtlID != ID_SAY_LIKE && d->CtlID != ID_SAY_PHONE)
        return;
    b = LoadBitmapA(tts_instance, MAKEINTRESOURCEA(d->itemState & ODS_SELECTED ? BMP_SAY_DOWN : BMP_SAY));
    if (b == NULL)
        return;
    if (d->itemAction & (ODA_DRAWENTIRE | ODA_SELECT)) {
        dc = CreateCompatibleDC(d->hDC);
        old = SelectObject(dc, b);
        if (old != NULL) {
            BitBlt(d->hDC, d->rcItem.left, d->rcItem.top, d->rcItem.right - d->rcItem.left,
                   d->rcItem.bottom - d->rcItem.top, dc, 0, 0, SRCCOPY);
            SelectObject(dc, old);
        }
        DeleteDC(dc);
    }
    DeleteObject(b);
}

/* The three edit controls: Tab moves between them and the buttons, Enter
 * speaks the field, and the dialog's default button follows the focus.
 * @0x10026070 */
static LRESULT CALLBACK edit_proc(HWND w, UINT msg, WPARAM wp, LPARAM lp)
{
    HWND dlg;
    int id, shift;

    if (msg == WM_GETDLGCODE)
        return DLGC_WANTALLKEYS;
    if (msg == WM_SETFOCUS)
        PostMessageA(GetParent(w), DM_SETDEFID, IDOK, 0);
    if (msg == WM_CHAR && wp == '\t') {
        id = GetDlgCtrlID(w);
        dlg = GetParent(w);
        shift = GetKeyState(VK_SHIFT) < 0;
        if (id == ID_WORD) {
            if (!shift) {
                SetFocus(GetDlgItem(dlg, ID_LIKE));
            } else {
                SetFocus(GetDlgItem(dlg, IDOK));
                PostMessageA(dlg, DM_SETDEFID, IDOK, 0);
            }
        } else if (id == ID_LIKE) {
            SetFocus(GetDlgItem(dlg, shift ? ID_WORD : ID_PRON));
        } else if (id == ID_PRON) {
            if (shift) {
                SetFocus(GetDlgItem(dlg, ID_LIKE));
            } else {
                SetFocus(GetDlgItem(dlg, ID_EXAMPLES));
                PostMessageA(dlg, DM_SETDEFID, ID_EXAMPLES, 0);
            }
        }
        return 0;
    }
    if (msg == WM_CHAR && wp == '\r') {
        id = GetDlgCtrlID(w);
        if (id == ID_WORD)
            PostMessageA(GetParent(w), WM_COMMAND, ID_SAY_WORD, (LPARAM)w);
        else if (id == ID_LIKE)
            PostMessageA(GetParent(w), WM_COMMAND, ID_SAY_LIKE, (LPARAM)w);
        else if (id == ID_PRON)
            PostMessageA(GetParent(w), WM_COMMAND, ID_SAY_PHONE, (LPARAM)w);
        return 0;
    }
    return CallWindowProcA(g_edit_proc, w, msg, wp, lp);
}

static void save_if_changed(void)
{
    if (g_lexicon_dirty) {
        userdic_save(g_dic_path);
        g_lexicon_dirty = 0;
    }
}

/* @0x10026260 */
static INT_PTR CALLBACK main_proc(HWND dlg, UINT msg, WPARAM wp, LPARAM lp)
{
    TtsHandle *t = (TtsHandle *)GetWindowLongPtrA(dlg, DWLP_USER);

    switch (msg) {
    case WM_DRAWITEM:
        draw_button((const DRAWITEMSTRUCT *)lp);
        return 1;
    case WM_DESTROY:
        /* (not in the DLL: this also comes when the thread ends after
         * tts_Close sent it, and the handle may be gone by then) */
        if (t == NULL)
            return 0;
        if (IsWindow(g_ex_dlg))
            PostMessageA(g_ex_dlg, WM_DESTROY, 0, 0);
        if (IsWindow(g_list_dlg))
            PostMessageA(g_list_dlg, WM_DESTROY, 0, 0);
        SetWindowLongPtrA(GetDlgItem(dlg, ID_WORD), GWLP_WNDPROC, (LONG_PTR)g_edit_proc);
        SetWindowLongPtrA(GetDlgItem(dlg, ID_LIKE), GWLP_WNDPROC, (LONG_PTR)g_edit_proc);
        SetWindowLongPtrA(GetDlgItem(dlg, ID_PRON), GWLP_WNDPROC, (LONG_PTR)g_edit_proc);
        g_dlg = NULL;
        t->dic_window = NULL;
        save_if_changed();
        SetWindowLongPtrA(dlg, DWLP_USER, 0);
        PostQuitMessage(0);
        return 0;
    case WM_HELP:
        WinHelpA((HWND)((const HELPINFO *)lp)->hItemHandle, g_help_path, HELP_WM_HELP,
                 (ULONG_PTR)tv_userdic_help());
        return 1;
    case WM_CONTEXTMENU:
        WinHelpA((HWND)wp, g_help_path, HELP_CONTEXTMENU, (ULONG_PTR)tv_userdic_help());
        return 1;
    case WM_INITDIALOG:
        SetWindowLongPtrA(dlg, DWLP_USER, lp);
        SetWindowLongA(dlg, GWL_EXSTYLE, GetWindowLongA(dlg, GWL_EXSTYLE) | WS_EX_CONTEXTHELP);
        SetWindowPos(GetDlgItem(dlg, ID_SAY_WORD), NULL, 0, 0, 0x2b, 0x15, SWP_NOMOVE);
        SetWindowPos(GetDlgItem(dlg, ID_SAY_LIKE), NULL, 0, 0, 0x2b, 0x15, SWP_NOMOVE);
        SetWindowPos(GetDlgItem(dlg, ID_SAY_PHONE), NULL, 0, 0, 0x2b, 0x15, SWP_NOMOVE);
        g_list_dlg = NULL;
        g_ex_dlg = NULL;
        g_edit_proc = (WNDPROC)SetWindowLongPtrA(GetDlgItem(dlg, ID_WORD), GWLP_WNDPROC, (LONG_PTR)edit_proc);
        SetWindowLongPtrA(GetDlgItem(dlg, ID_LIKE), GWLP_WNDPROC, (LONG_PTR)edit_proc);
        SetWindowLongPtrA(GetDlgItem(dlg, ID_PRON), GWLP_WNDPROC, (LONG_PTR)edit_proc);
        clear_fields(dlg);
        tv_userdic_reload(g_dic_path);
        return 1;
    case WM_COMMAND:
        if (HIWORD(wp) == 0) {
            switch (LOWORD(wp)) {
            case ID_SAY_WORD:
                say_field(t, ID_WORD);
                return 1;
            case ID_SAY_LIKE:
                say_field(t, ID_LIKE);
                return 1;
            case ID_SAY_PHONE:
                say_field(t, ID_PRON);
                return 1;
            }
        }
        switch (wp) {
        case ID_WORDLIST:
            WaitForSingleObject(t->dic_event, 5000);
            if (g_list_dlg != NULL)
                bring_up(g_list_dlg);
            else
                start_dialog(DLG_WORDLIST, dlg, wordlist_proc, t);
            return 1;
        case ID_EXAMPLES:
            WaitForSingleObject(t->dic_event, 5000);
            if (g_ex_dlg != NULL)
                bring_up(g_ex_dlg);
            else
                start_dialog(DLG_EXAMPLES, dlg, examples_proc, t);
            return 1;
        case ID_HELP:
            WinHelpA(dlg, g_help_path, HELP_CONTEXT, HELP_CONTEXT_MAIN);
            return 1;
        case ID_CLEAR:
            clear_fields(dlg);
            return 1;
        case ID_ADD:
            add_entry(dlg);
            return 1;
        case IDOK:
        case IDCANCEL:
        case ID_CLOSE_ALT:
            g_dlg = NULL;
            t->dic_window = NULL;
            save_if_changed();
            DestroyWindow(dlg);
            return 1;
        }
        return 0;
    }
    return 0;
}

/* Opens the pronunciation manager in a thread of its own, or brings it to
 * the top.  parent may be NULL.  @0x100605f0 */
TTS_EXPORT tts_UserDic(void *h, HWND parent)
{
    TtsHandle *t = (TtsHandle *)h;

    if (tts_validate(h) < 0)
        return TTS_INVALIDHANDLE;
    if (parent != NULL && !IsWindow(parent))
        return TTS_BADPARAM;
    WaitForSingleObject(t->dic_event, 5000);
    if (g_dlg == NULL)
        return start_dialog(DLG_MAIN, parent, main_proc, t) ? 0 : TTS_NOTHREAD;
    SetWindowPos(g_dlg, NULL, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
    return 0;
}
