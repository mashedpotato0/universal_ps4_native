/* universal ps4 native - windows launcher ui */
#define UNICODE
#define _UNICODE

typedef unsigned short wchar_t;
typedef void *HANDLE;
typedef void *HWND;
typedef void *HDC;
typedef void *HINSTANCE;
typedef void *HICON;
typedef void *HCURSOR;
typedef void *HBRUSH;
typedef void *HPEN;
typedef void *HFONT;
typedef void *HMENU;
typedef unsigned long DWORD;
typedef int BOOL;
typedef unsigned int UINT;
typedef long long LONG_PTR;
typedef unsigned long long ULONG_PTR;
typedef LONG_PTR LRESULT;
typedef LONG_PTR LPARAM;
typedef ULONG_PTR WPARAM;

#define NULL ((void*)0)
#define FALSE 0
#define TRUE 1

#define WS_OVERLAPPED 0x00000000L
#define WS_CAPTION 0x00C00000L
#define WS_SYSMENU 0x00080000L
#define WS_THICKFRAME 0x00040000L
#define WS_MINIMIZEBOX 0x00020000L
#define WS_VISIBLE 0x10000000L
#define WS_CHILD 0x40000000L
#define WS_TABSTOP 0x00010000L
#define WS_VSCROLL 0x00200000L
#define WS_BORDER 0x00800000L

#define CBS_DROPDOWNLIST 0x0003L
#define BS_AUTOCHECKBOX 0x00000003L
#define BS_OWNERDRAW 0x0000000BL
#define BS_PUSHBUTTON 0x00000000L
#define ES_AUTOHSCROLL 0x0080L

#define CB_ADDSTRING 0x0143
#define CB_SETCURSEL 0x014E
#define CB_GETCURSEL 0x0147
#define CB_GETLBTEXT 0x0148
#define CB_RESETCONTENT 0x014B

#define BM_GETCHECK 0x00F0
#define BM_SETCHECK 0x00F1
#define BST_CHECKED 1
#define BST_UNCHECKED 0

#define WM_DESTROY 0x0002
#define WM_PAINT 0x000F
#define WM_COMMAND 0x0111
#define WM_DRAWITEM 0x002B
#define WM_CTLCOLORSTATIC 0x0138
#define WM_CTLCOLOREDIT 0x0133
#define WM_CTLCOLORBTN 0x0135
#define WM_CTLCOLORLISTBOX 0x0134
#define WM_SETFONT 0x0030

#define SW_SHOW 5
#define MB_OK 0x00000000L
#define MB_ICONERROR 0x00000010L
#define MB_ICONINFORMATION 0x00000040L

#define DT_CENTER 0x00000001
#define DT_VCENTER 0x00000004
#define DT_SINGLELINE 0x00000020
#define DT_LEFT 0x00000000

#define TRANSPARENT 1
#define FW_NORMAL 400
#define FW_BOLD 700
#define DEFAULT_CHARSET 1
#define OUT_DEFAULT_PRECIS 0
#define CLIP_DEFAULT_PRECIS 0
#define CLEARTYPE_QUALITY 5
#define DEFAULT_PITCH 0
#define FF_DONTCARE 0
#define IDC_ARROW ((const wchar_t*)(ULONG_PTR)32512)

#define ODS_SELECTED 0x0001

#define RGB(r,g,b) ((DWORD)(((unsigned char)(r)|((unsigned short)((unsigned char)(g))<<8))|(((DWORD)(unsigned char)(b))<<16)))

typedef struct tagRECT {
    long left;
    long top;
    long right;
    long bottom;
} RECT;

typedef struct tagPAINTSTRUCT {
    HDC hdc;
    BOOL fErase;
    RECT rcPaint;
    BOOL fRestore;
    BOOL fIncUpdate;
    unsigned char rgbReserved[32];
} PAINTSTRUCT;

typedef struct tagDRAWITEMSTRUCT {
    UINT CtlType;
    UINT CtlID;
    UINT itemID;
    UINT itemAction;
    UINT itemState;
    HWND hwndItem;
    HDC hDC;
    RECT rcItem;
    ULONG_PTR itemData;
} DRAWITEMSTRUCT;

typedef struct tagWNDCLASSEXW {
    UINT cbSize;
    UINT style;
    LRESULT (*lpfnWndProc)(HWND, UINT, WPARAM, LPARAM);
    int cbClsExtra;
    int cbWndExtra;
    HINSTANCE hInstance;
    HICON hIcon;
    HCURSOR hCursor;
    HBRUSH hbrBackground;
    const wchar_t *lpszMenuName;
    const wchar_t *lpszClassName;
    HICON hIconSm;
} WNDCLASSEXW;

typedef struct tagMSG {
    HWND hwnd;
    UINT message;
    WPARAM wParam;
    LPARAM lParam;
    DWORD time;
    long pt_x;
    long pt_y;
} MSG;

typedef struct _STARTUPINFOW {
    DWORD cb;
    wchar_t *lpReserved;
    wchar_t *lpDesktop;
    wchar_t *lpTitle;
    DWORD dwX, dwY, dwXSize, dwYSize;
    DWORD dwXCountChars, dwYCountChars;
    DWORD dwFillAttribute;
    DWORD dwFlags;
    unsigned short wShowWindow, cbReserved2;
    unsigned char *lpReserved2;
    HANDLE hStdInput, hStdOutput, hStdError;
} STARTUPINFOW;

typedef struct _PROCESS_INFORMATION {
    HANDLE hProcess;
    HANDLE hThread;
    DWORD dwProcessId;
    DWORD dwThreadId;
} PROCESS_INFORMATION;

typedef struct _WIN32_FIND_DATAW {
    DWORD dwFileAttributes;
    DWORD ftCreationTime_low, ftCreationTime_high;
    DWORD ftLastAccessTime_low, ftLastAccessTime_high;
    DWORD ftLastWriteTime_low, ftLastWriteTime_high;
    DWORD nFileSizeHigh, nFileSizeLow;
    DWORD dwReserved0, dwReserved1;
    wchar_t cFileName[260];
    wchar_t cAlternateFileName[14];
} WIN32_FIND_DATAW;

typedef struct tagOFNW {
    DWORD lStructSize;
    HWND hwndOwner;
    HINSTANCE hInstance;
    const wchar_t *lpstrFilter;
    wchar_t *lpstrCustomFilter;
    DWORD nMaxCustFilter;
    DWORD nFilterIndex;
    wchar_t *lpstrFile;
    DWORD nMaxFile;
    wchar_t *lpstrFileTitle;
    DWORD nMaxFileTitle;
    const wchar_t *lpstrInitialDir;
    const wchar_t *lpstrTitle;
    DWORD Flags;
    unsigned short nFileOffset;
    unsigned short nFileExtension;
    const wchar_t *lpstrDefExt;
    LPARAM lCustData;
    void *lpfnHook;
    const wchar_t *lpTemplateName;
    void *pvReserved;
    DWORD dwReserved;
    DWORD FlagsEx;
} OPENFILENAMEW;

/* win32 imports */
__declspec(dllimport) void __stdcall ExitProcess(DWORD);
__declspec(dllimport) DWORD __stdcall GetModuleFileNameW(HANDLE, wchar_t*, DWORD);
__declspec(dllimport) BOOL __stdcall CreateProcessW(const wchar_t*, wchar_t*, void*, void*, BOOL, DWORD, void*, const wchar_t*, STARTUPINFOW*, PROCESS_INFORMATION*);
__declspec(dllimport) BOOL __stdcall CloseHandle(HANDLE);
__declspec(dllimport) HANDLE __stdcall FindFirstFileW(const wchar_t*, WIN32_FIND_DATAW*);
__declspec(dllimport) BOOL __stdcall FindNextFileW(HANDLE, WIN32_FIND_DATAW*);
__declspec(dllimport) BOOL __stdcall FindClose(HANDLE);
__declspec(dllimport) DWORD __stdcall GetFileAttributesW(const wchar_t*);


__declspec(dllimport) UINT __stdcall RegisterClassExW(const WNDCLASSEXW*);
__declspec(dllimport) HWND __stdcall CreateWindowExW(DWORD, const wchar_t*, const wchar_t*, DWORD, int, int, int, int, HWND, HMENU, HINSTANCE, void*);
__declspec(dllimport) BOOL __stdcall ShowWindow(HWND, int);
__declspec(dllimport) BOOL __stdcall UpdateWindow(HWND);
__declspec(dllimport) BOOL __stdcall GetMessageW(MSG*, HWND, UINT, UINT);
__declspec(dllimport) BOOL __stdcall TranslateMessage(const MSG*);
__declspec(dllimport) LRESULT __stdcall DispatchMessageW(const MSG*);
__declspec(dllimport) LRESULT __stdcall DefWindowProcW(HWND, UINT, WPARAM, LPARAM);
__declspec(dllimport) void __stdcall PostQuitMessage(int);
__declspec(dllimport) LRESULT __stdcall SendMessageW(HWND, UINT, WPARAM, LPARAM);
__declspec(dllimport) BOOL __stdcall SetWindowTextW(HWND, const wchar_t*);
__declspec(dllimport) int __stdcall GetWindowTextW(HWND, wchar_t*, int);
__declspec(dllimport) int __stdcall MessageBoxW(HWND, const wchar_t*, const wchar_t*, UINT);
__declspec(dllimport) BOOL __stdcall GetClientRect(HWND, RECT*);
__declspec(dllimport) HDC __stdcall BeginPaint(HWND, PAINTSTRUCT*);
__declspec(dllimport) BOOL __stdcall EndPaint(HWND, const PAINTSTRUCT*);
__declspec(dllimport) int __stdcall FillRect(HDC, const RECT*, HBRUSH);
__declspec(dllimport) int __stdcall DrawTextW(HDC, const wchar_t*, int, RECT*, UINT);
__declspec(dllimport) HCURSOR __stdcall LoadCursorW(HINSTANCE, const wchar_t*);

__declspec(dllimport) HBRUSH __stdcall CreateSolidBrush(DWORD);
__declspec(dllimport) HPEN __stdcall CreatePen(int, int, DWORD);
__declspec(dllimport) BOOL __stdcall DeleteObject(void*);
__declspec(dllimport) DWORD __stdcall SetBkColor(HDC, DWORD);
__declspec(dllimport) int __stdcall SetBkMode(HDC, int);
__declspec(dllimport) DWORD __stdcall SetTextColor(HDC, DWORD);
__declspec(dllimport) HFONT __stdcall CreateFontW(int, int, int, int, int, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, const wchar_t*);
__declspec(dllimport) void* __stdcall SelectObject(HDC, void*);
__declspec(dllimport) BOOL __stdcall RoundRect(HDC, int, int, int, int, int, int);

__declspec(dllimport) long __stdcall DwmSetWindowAttribute(HWND, DWORD, const void*, DWORD);
__declspec(dllimport) BOOL __stdcall GetOpenFileNameW(OPENFILENAMEW*);

/* helpers */
void *memcpy(void *dst, const void *src, unsigned long long n) {
    char *d = (char*)dst; const char *s = (const char*)src;
    while (n--) *d++ = *s++; return dst;
}
void *memset(void *dst, int c, unsigned long long n) {
    char *d = (char*)dst; while (n--) *d++ = (char)c; return dst;
}
void __chkstk(void) {}

static unsigned int wlen(const wchar_t *s) {
    unsigned int n = 0; while (s && s[n]) n++; return n;
}
static void wcpy(wchar_t *dst, const wchar_t *src, unsigned int max) {
    unsigned int i = 0;
    while (src && src[i] && (i + 1 < max)) { dst[i] = src[i]; i++; }
    dst[i] = 0;
}
static void wcat(wchar_t *dst, const wchar_t *src, unsigned int max) {
    unsigned int d = wlen(dst); unsigned int i = 0;
    while (src && src[i] && (d + i + 1 < max)) { dst[d + i] = src[i]; i++; }
    dst[d + i] = 0;
}

/* color palette - sleek dark theme, zero purple */
#define COL_BG       RGB(18, 20, 24)      /* #121418 */
#define COL_CARD     RGB(26, 29, 36)      /* #1a1d24 */
#define COL_INPUT    RGB(34, 38, 48)      /* #222630 */
#define COL_BORDER   RGB(48, 54, 68)      /* #303644 */
#define COL_TEXT     RGB(235, 240, 245)   /* #ebf0f5 */
#define COL_MUTED    RGB(140, 148, 160)   /* #8c94a0 */
#define COL_BLUE     RGB(0, 112, 209)     /* #0070d1 playstation blue */
#define COL_BTN_SEC  RGB(38, 42, 52)      /* #262a34 secondary button */

/* control ids */
#define ID_GAME_COMBO    101
#define ID_BROWSE_BTN    102
#define ID_PATH_EDIT     103
#define ID_CHK_LOWSPEC   104
#define ID_CHK_UNCAP     105
#define ID_CHK_PERMISSIVE 106
#define ID_CHK_CPUONLY   107
#define ID_CHK_REBUILD   108
#define ID_FPS_COMBO     109
#define ID_GPU_COMBO     110
#define ID_BTN_LAUNCH    111
#define ID_BTN_INSPECT   112
#define ID_BTN_EXTRACT   113
#define ID_BTN_SYNC      114

static HBRUSH hbrBg = NULL;
static HBRUSH hbrCard = NULL;
static HBRUSH hbrInput = NULL;
static HBRUSH hbrBlue = NULL;
static HBRUSH hbrBtnSec = NULL;
static HPEN   hpenBorder = NULL;
static HFONT  hFontMain = NULL;
static HFONT  hFontTitle = NULL;
static HFONT  hFontSub = NULL;
static HFONT  hFontBold = NULL;

static HWND hGameCombo = NULL;
static HWND hPathEdit = NULL;
static HWND hChkLowSpec = NULL;
static HWND hChkUncap = NULL;
static HWND hChkPermissive = NULL;
static HWND hChkCpuOnly = NULL;
static HWND hChkRebuild = NULL;
static HWND hFpsCombo = NULL;
static HWND hGpuCombo = NULL;
static HWND hBtnLaunch = NULL;
static HWND hBtnInspect = NULL;
static HWND hBtnExtract = NULL;
static HWND hBtnSync = NULL;

static wchar_t appDir[1024];

static void initPaths(void) {
    GetModuleFileNameW(NULL, appDir, 1024);
    int last = -1;
    for (int i = 0; appDir[i]; i++) {
        if (appDir[i] == L'\\' || appDir[i] == L'/') last = i;
    }
    if (last >= 0) appDir[last] = 0; else { appDir[0] = L'.'; appDir[1] = 0; }
}

static void populateGames(void) {
    SendMessageW(hGameCombo, CB_RESETCONTENT, 0, 0);

    /* scan extracted directory */
    wchar_t pattern[1024];
    pattern[0] = 0;
    wcat(pattern, appDir, 1024);
    wcat(pattern, L"\\extracted\\*", 1024);

    WIN32_FIND_DATAW fd;
    HANDLE hFind = FindFirstFileW(pattern, &fd);
    int found = 0;

    if (hFind != (HANDLE)-1) {
        do {
            if (fd.cFileName[0] != L'.') {
                wchar_t entry[512];
                entry[0] = 0;
                if (fd.cFileName[0] == L'C' && fd.cFileName[1] == L'U' && fd.cFileName[2] == L'S' && fd.cFileName[3] == L'A') {
                    if (wlen(fd.cFileName) >= 9 && fd.cFileName[4] == L'0' && fd.cFileName[5] == L'3' && fd.cFileName[6] == L'1') {
                        wcat(entry, L"Bloodborne (CUSA03173) [Extracted]", 512);
                    } else if (wlen(fd.cFileName) >= 9 && fd.cFileName[4] == L'1' && fd.cFileName[5] == L'3' && fd.cFileName[6] == L'8') {
                        wcat(entry, L"Sekiro: Shadows Die Twice (CUSA13801) [Extracted]", 512);
                    } else if (wlen(fd.cFileName) >= 9 && fd.cFileName[4] == L'0' && fd.cFileName[5] == L'7' && fd.cFileName[6] == L'9') {
                        wcat(entry, L"Street Fighter 30th Anniversary (CUSA07997) [Extracted]", 512);
                    } else {
                        wcat(entry, fd.cFileName, 512);
                        wcat(entry, L" [Extracted]", 512);
                    }
                } else {
                    wcat(entry, fd.cFileName, 512);
                }
                SendMessageW(hGameCombo, CB_ADDSTRING, 0, (LPARAM)entry);
                found++;
            }
        } while (FindNextFileW(hFind, &fd));
        FindClose(hFind);
    }

    /* add default known titles */
    SendMessageW(hGameCombo, CB_ADDSTRING, 0, (LPARAM)L"Bloodborne (Auto-detect)");
    SendMessageW(hGameCombo, CB_ADDSTRING, 0, (LPARAM)L"Sekiro: Shadows Die Twice (Auto-detect)");
    SendMessageW(hGameCombo, CB_ADDSTRING, 0, (LPARAM)L"Street Fighter Collection (Auto-detect)");
    SendMessageW(hGameCombo, CB_ADDSTRING, 0, (LPARAM)L"<Custom Path / Package>");

    SendMessageW(hGameCombo, CB_SETCURSEL, 0, 0);
}

static void browseGame(HWND hWnd) {
    wchar_t szFile[1024];
    szFile[0] = 0;

    OPENFILENAMEW ofn;
    memset(&ofn, 0, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hWnd;
    ofn.lpstrFilter = L"PS4 Packages (*.pkg)\0*.pkg\0All Files (*.*)\0*.*\0";
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = 1024;
    ofn.Flags = 0x00000800 | 0x00001000; /* OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST */

    if (GetOpenFileNameW(&ofn)) {
        SetWindowTextW(hPathEdit, szFile);
        /* set combo to custom */
        int count = (int)SendMessageW(hGameCombo, 0x0146 /* CB_GETCOUNT */, 0, 0);
        if (count > 0) SendMessageW(hGameCombo, CB_SETCURSEL, count - 1, 0);
    }
}

static void executeAction(const wchar_t *subcmd) {
    /* resolve selected target */
    int sel = (int)SendMessageW(hGameCombo, CB_GETCURSEL, 0, 0);
    wchar_t text[512];
    text[0] = 0;
    SendMessageW(hGameCombo, CB_GETLBTEXT, sel, (LPARAM)text);

    wchar_t targetPath[1024];
    targetPath[0] = 0;

    wchar_t customEdit[1024];
    customEdit[0] = 0;
    GetWindowTextW(hPathEdit, customEdit, 1024);

    if (customEdit[0]) {
        wcat(targetPath, L"\"", 1024);
        wcat(targetPath, customEdit, 1024);
        wcat(targetPath, L"\"", 1024);
    } else if (text[0] == L'B' && text[1] == L'l') {
        wcat(targetPath, L"./extracted/CUSA03173", 1024);
    } else if (text[0] == L'S' && text[1] == L'e') {
        wcat(targetPath, L"./extracted/CUSA13801", 1024);
    } else if (text[0] == L'S' && text[1] == L't') {
        wcat(targetPath, L"./extracted/CUSA07997", 1024);
    }

    /* collect flags */
    wchar_t flags[1024];
    flags[0] = 0;

    if (SendMessageW(hChkLowSpec, BM_GETCHECK, 0, 0) == BST_CHECKED) {
        wcat(flags, L" --low-spec", 1024);
    }
    if (SendMessageW(hChkUncap, BM_GETCHECK, 0, 0) == BST_CHECKED) {
        wcat(flags, L" --no-cap-fps", 1024);
    }
    if (SendMessageW(hChkPermissive, BM_GETCHECK, 0, 0) == BST_CHECKED) {
        wcat(flags, L" --permissive", 1024);
    }
    if (SendMessageW(hChkCpuOnly, BM_GETCHECK, 0, 0) == BST_CHECKED) {
        wcat(flags, L" --cpu-only", 1024);
    }
    if (SendMessageW(hChkRebuild, BM_GETCHECK, 0, 0) == BST_CHECKED) {
        wcat(flags, L" --rebuild", 1024);
    }

    int fpsSel = (int)SendMessageW(hFpsCombo, CB_GETCURSEL, 0, 0);
    if (fpsSel == 1) wcat(flags, L" --fps 60", 1024);
    else if (fpsSel == 2) wcat(flags, L" --fps 30", 1024);
    else if (fpsSel == 3) wcat(flags, L" --fps 90", 1024);

    int gpuSel = (int)SendMessageW(hGpuCombo, CB_GETCURSEL, 0, 0);
    if (gpuSel == 1) wcat(flags, L" --discrete", 1024);
    else if (gpuSel == 2) wcat(flags, L" --integrated", 1024);

    /* check if native windows runner exists */
    wchar_t winRunner[1024];
    winRunner[0] = 0;
    wcat(winRunner, appDir, 1024);
    wcat(winRunner, L"\\bin\\windows\\shadPS4.exe", 1024);

    DWORD runnerAttr = GetFileAttributesW(winRunner);
    BOOL hasNativeWin = (runnerAttr != (DWORD)-1 && !(runnerAttr & 0x10 /* FILE_ATTRIBUTE_DIRECTORY */));

    /* build run command */
    wchar_t cmdLine[4096];
    cmdLine[0] = 0;

    if (hasNativeWin && (!subcmd || !subcmd[0] || (subcmd[0] == L'r' && subcmd[1] == L'u' && subcmd[2] == L'n'))) {
        /* launch direct native windows runner */
        wcat(cmdLine, L"\"", 4096);
        wcat(cmdLine, winRunner, 4096);
        wcat(cmdLine, L"\"", 4096);
        if (targetPath[0]) {
            wcat(cmdLine, L" -g ", 4096);
            wcat(cmdLine, targetPath, 4096);
        }
        if (SendMessageW(hChkUncap, BM_GETCHECK, 0, 0) == BST_CHECKED) {
            wcat(cmdLine, L" --show-fps", 4096);
        }
    } else {
        wcat(cmdLine, L"cmd.exe /c run.bat", 4096);
        if (subcmd && subcmd[0]) {
            wcat(cmdLine, L" ", 4096);
            wcat(cmdLine, subcmd, 4096);
        }
        if (targetPath[0]) {
            wcat(cmdLine, L" ", 4096);
            wcat(cmdLine, targetPath, 4096);
        }
        if (flags[0]) {
            wcat(cmdLine, flags, 4096);
        }
    }

    STARTUPINFOW si;
    memset(&si, 0, sizeof(si));
    si.cb = sizeof(si);

    PROCESS_INFORMATION pi;
    memset(&pi, 0, sizeof(pi));

    BOOL ok = CreateProcessW(NULL, cmdLine, NULL, NULL, FALSE, 0, NULL, appDir, &si, &pi);
    if (!ok) {
        /* fallback direct wsl command */
        wchar_t wslCmd[4096];
        wslCmd[0] = 0;
        wcat(wslCmd, L"wsl.exe -e bash -c \"cd \\\"$(wslpath '", 4096);
        wcat(wslCmd, appDir, 4096);
        wcat(wslCmd, L"')\\\" && ./run.sh", 4096);
        if (targetPath[0]) { wcat(wslCmd, L" ", 4096); wcat(wslCmd, targetPath, 4096); }
        if (flags[0]) { wcat(wslCmd, flags, 4096); }
        wcat(wslCmd, L"\"", 4096);

        ok = CreateProcessW(NULL, wslCmd, NULL, NULL, FALSE, 0, NULL, appDir, &si, &pi);
    }

    if (!ok) {
        MessageBoxW(NULL,
            L"Could not launch game runner.\n\nMake sure run.bat or bin/windows/shadPS4.exe is present.",
            L"Universal PS4 Native",
            MB_OK | MB_ICONERROR);
    } else {
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
}


static LRESULT LauncherWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CTLCOLORSTATIC:
        case WM_CTLCOLORBTN: {
            HDC hdc = (HDC)wParam;
            SetBkColor(hdc, COL_BG);
            SetTextColor(hdc, COL_TEXT);
            return (LRESULT)hbrBg;
        }
        case WM_CTLCOLOREDIT:
        case WM_CTLCOLORLISTBOX: {
            HDC hdc = (HDC)wParam;
            SetBkColor(hdc, COL_INPUT);
            SetTextColor(hdc, COL_TEXT);
            return (LRESULT)hbrInput;
        }
        case WM_DRAWITEM: {
            DRAWITEMSTRUCT *dis = (DRAWITEMSTRUCT*)lParam;
            if (dis->CtlID == ID_BTN_LAUNCH) {
                HBRUSH btnBr = hbrBlue;
                FillRect(dis->hDC, &dis->rcItem, btnBr);
                SetBkMode(dis->hDC, TRANSPARENT);
                SetTextColor(dis->hDC, RGB(255, 255, 255));
                SelectObject(dis->hDC, hFontBold);
                DrawTextW(dis->hDC, L"LAUNCH GAME", -1, &dis->rcItem, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
                return TRUE;
            } else if (dis->CtlID == ID_BTN_INSPECT || dis->CtlID == ID_BTN_EXTRACT || dis->CtlID == ID_BTN_SYNC || dis->CtlID == ID_BROWSE_BTN) {
                FillRect(dis->hDC, &dis->rcItem, hbrBtnSec);
                SetBkMode(dis->hDC, TRANSPARENT);
                SetTextColor(dis->hDC, COL_TEXT);
                SelectObject(dis->hDC, hFontMain);
                wchar_t txt[64]; txt[0] = 0;
                if (dis->CtlID == ID_BTN_INSPECT) wcat(txt, L"Inspect Game", 64);
                else if (dis->CtlID == ID_BTN_EXTRACT) wcat(txt, L"Extract PKG", 64);
                else if (dis->CtlID == ID_BTN_SYNC) wcat(txt, L"Sync Launchers", 64);
                else if (dis->CtlID == ID_BROWSE_BTN) wcat(txt, L"Browse...", 64);
                DrawTextW(dis->hDC, txt, -1, &dis->rcItem, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
                return TRUE;
            }
            break;
        }
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            RECT rc;
            GetClientRect(hWnd, &rc);
            FillRect(hdc, &rc, hbrBg);

            /* header title */
            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, COL_TEXT);
            SelectObject(hdc, hFontTitle);
            RECT rTitle = { 24, 18, 600, 44 };
            DrawTextW(hdc, L"UNIVERSAL PS4 NATIVE", -1, &rTitle, DT_LEFT | DT_SINGLELINE);

            /* header subtitle */
            SetTextColor(hdc, COL_MUTED);
            SelectObject(hdc, hFontSub);
            RECT rSub = { 24, 46, 600, 66 };
            DrawTextW(hdc, L"Direct PlayStation 4 Runtime & Execution Toolchain", -1, &rSub, DT_LEFT | DT_SINGLELINE);

            /* section header 1 */
            SetTextColor(hdc, COL_TEXT);
            SelectObject(hdc, hFontBold);
            RECT rSec1 = { 24, 76, 600, 96 };
            DrawTextW(hdc, L"INSTALLED GAME / PACKAGE", -1, &rSec1, DT_LEFT | DT_SINGLELINE);

            /* section header 2 */
            RECT rSec2 = { 24, 172, 600, 192 };
            DrawTextW(hdc, L"RUNTIME && PERFORMANCE OPTIONS", -1, &rSec2, DT_LEFT | DT_SINGLELINE);

            /* section header 3 */
            RECT rSec3 = { 330, 172, 600, 192 };
            DrawTextW(hdc, L"HARDWARE && LIMITS", -1, &rSec3, DT_LEFT | DT_SINGLELINE);

            EndPaint(hWnd, &ps);
            return 0;
        }
        case WM_COMMAND: {
            int id = (int)(wParam & 0xFFFF);
            int code = (int)(wParam >> 16);
            if (id == ID_GAME_COMBO && code == 1 /* CBN_SELCHANGE */) {
                int sel = (int)SendMessageW(hGameCombo, CB_GETCURSEL, 0, 0);
                wchar_t txt[512]; txt[0] = 0;
                SendMessageW(hGameCombo, CB_GETLBTEXT, sel, (LPARAM)txt);
                if (txt[0] == L'B' && txt[1] == L'l') {
                    SetWindowTextW(hPathEdit, L"./extracted/CUSA03173");
                } else if (txt[0] == L'S' && txt[1] == L'e') {
                    SetWindowTextW(hPathEdit, L"./extracted/CUSA13801");
                } else if (txt[0] == L'S' && txt[1] == L't') {
                    SetWindowTextW(hPathEdit, L"./extracted/CUSA07997");
                } else if (txt[0] == L'<') {
                    SetWindowTextW(hPathEdit, L"");
                }
            } else if (id == ID_BTN_LAUNCH) {
                executeAction(NULL);
            } else if (id == ID_BTN_INSPECT) {
                executeAction(L"inspect");
            } else if (id == ID_BTN_EXTRACT) {
                executeAction(L"extract");
            } else if (id == ID_BTN_SYNC) {
                executeAction(L"--sync-launchers");
            } else if (id == ID_BROWSE_BTN) {
                browseGame(hWnd);
            }
            break;
        }
        case WM_DESTROY: {
            PostQuitMessage(0);
            return 0;
        }
    }
    return DefWindowProcW(hWnd, uMsg, wParam, lParam);
}

void mainEntry(void) {
    initPaths();

    hbrBg = CreateSolidBrush(COL_BG);
    hbrCard = CreateSolidBrush(COL_CARD);
    hbrInput = CreateSolidBrush(COL_INPUT);
    hbrBlue = CreateSolidBrush(COL_BLUE);
    hbrBtnSec = CreateSolidBrush(COL_BTN_SEC);
    hpenBorder = CreatePen(0, 1, COL_BORDER);

    hFontMain = CreateFontW(-13, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    hFontTitle = CreateFontW(-19, 0, 0, 0, FW_BOLD, 0, 0, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    hFontSub = CreateFontW(-12, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    hFontBold = CreateFontW(-13, 0, 0, 0, FW_BOLD, 0, 0, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");

    WNDCLASSEXW wc;
    memset(&wc, 0, sizeof(wc));
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = LauncherWndProc;
    wc.hInstance = NULL;
    wc.hCursor = LoadCursorW(NULL, IDC_ARROW);
    wc.hbrBackground = hbrBg;
    wc.lpszClassName = L"UniversalPS4LauncherClass";
    RegisterClassExW(&wc);

    HWND hWnd = CreateWindowExW(
        0,
        L"UniversalPS4LauncherClass",
        L"Universal PS4 Native - Game Launcher",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        150, 150, 640, 520,
        NULL, NULL, NULL, NULL
    );

    /* enable modern immersive dark titlebar on windows 10/11 */
    DWORD dark = 1;
    DwmSetWindowAttribute(hWnd, 20 /* DWMWA_USE_IMMERSIVE_DARK_MODE */, &dark, sizeof(dark));
    DwmSetWindowAttribute(hWnd, 19 /* older dark mode attribute */, &dark, sizeof(dark));

    /* game combo dropdown */
    hGameCombo = CreateWindowExW(0, L"COMBOBOX", L"",
        WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_TABSTOP | WS_VSCROLL,
        24, 100, 460, 240, hWnd, (HMENU)ID_GAME_COMBO, NULL, NULL);
    SendMessageW(hGameCombo, WM_SETFONT, (WPARAM)hFontMain, TRUE);

    /* browse button */
    HWND hBrowse = CreateWindowExW(0, L"BUTTON", L"Browse...",
        WS_CHILD | WS_VISIBLE | BS_OWNERDRAW | WS_TABSTOP,
        494, 100, 110, 26, hWnd, (HMENU)ID_BROWSE_BTN, NULL, NULL);
    SendMessageW(hBrowse, WM_SETFONT, (WPARAM)hFontMain, TRUE);

    /* path edit */
    hPathEdit = CreateWindowExW(0, L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL | WS_TABSTOP,
        24, 134, 580, 24, hWnd, (HMENU)ID_PATH_EDIT, NULL, NULL);
    SendMessageW(hPathEdit, WM_SETFONT, (WPARAM)hFontMain, TRUE);

    /* left checkboxes */
    hChkLowSpec = CreateWindowExW(0, L"BUTTON", L"Low-Spec / Laptop iGPU Profile",
        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX | WS_TABSTOP,
        24, 198, 280, 24, hWnd, (HMENU)ID_CHK_LOWSPEC, NULL, NULL);
    SendMessageW(hChkLowSpec, WM_SETFONT, (WPARAM)hFontMain, TRUE);
    SendMessageW(hChkLowSpec, BM_SETCHECK, BST_CHECKED, 0);

    hChkUncap = CreateWindowExW(0, L"BUTTON", L"Uncap Frame Rate (Display VSync)",
        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX | WS_TABSTOP,
        24, 226, 280, 24, hWnd, (HMENU)ID_CHK_UNCAP, NULL, NULL);
    SendMessageW(hChkUncap, WM_SETFONT, (WPARAM)hFontMain, TRUE);

    hChkPermissive = CreateWindowExW(0, L"BUTTON", L"Permissive Mode (Stub Unmapped OS)",
        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX | WS_TABSTOP,
        24, 254, 280, 24, hWnd, (HMENU)ID_CHK_PERMISSIVE, NULL, NULL);
    SendMessageW(hChkPermissive, WM_SETFONT, (WPARAM)hFontMain, TRUE);

    hChkCpuOnly = CreateWindowExW(0, L"BUTTON", L"Headless / CPU-Only Mode",
        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX | WS_TABSTOP,
        24, 282, 280, 24, hWnd, (HMENU)ID_CHK_CPUONLY, NULL, NULL);
    SendMessageW(hChkCpuOnly, WM_SETFONT, (WPARAM)hFontMain, TRUE);

    hChkRebuild = CreateWindowExW(0, L"BUTTON", L"Force Rebuild / Recompile Native Image",
        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX | WS_TABSTOP,
        24, 310, 280, 24, hWnd, (HMENU)ID_CHK_REBUILD, NULL, NULL);
    SendMessageW(hChkRebuild, WM_SETFONT, (WPARAM)hFontMain, TRUE);

    /* right dropdowns */
    HWND hFpsLabel = CreateWindowExW(0, L"STATIC", L"Target Frame Rate:",
        WS_CHILD | WS_VISIBLE,
        330, 198, 260, 18, hWnd, NULL, NULL, NULL);
    SendMessageW(hFpsLabel, WM_SETFONT, (WPARAM)hFontMain, TRUE);

    hFpsCombo = CreateWindowExW(0, L"COMBOBOX", L"",
        WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_TABSTOP,
        330, 220, 274, 180, hWnd, (HMENU)ID_FPS_COMBO, NULL, NULL);
    SendMessageW(hFpsCombo, WM_SETFONT, (WPARAM)hFontMain, TRUE);
    SendMessageW(hFpsCombo, CB_ADDSTRING, 0, (LPARAM)L"Uncapped (Delta-Time Patch)");
    SendMessageW(hFpsCombo, CB_ADDSTRING, 0, (LPARAM)L"60 FPS Cap (Fixed Timestep)");
    SendMessageW(hFpsCombo, CB_ADDSTRING, 0, (LPARAM)L"30 FPS Cap (Standard)");
    SendMessageW(hFpsCombo, CB_ADDSTRING, 0, (LPARAM)L"90 FPS Cap (High Refresh)");
    SendMessageW(hFpsCombo, CB_SETCURSEL, 0, 0);

    HWND hGpuLabel = CreateWindowExW(0, L"STATIC", L"Graphics Device:",
        WS_CHILD | WS_VISIBLE,
        330, 260, 260, 18, hWnd, NULL, NULL, NULL);
    SendMessageW(hGpuLabel, WM_SETFONT, (WPARAM)hFontMain, TRUE);

    hGpuCombo = CreateWindowExW(0, L"COMBOBOX", L"",
        WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_TABSTOP,
        330, 282, 274, 180, hWnd, (HMENU)ID_GPU_COMBO, NULL, NULL);
    SendMessageW(hGpuCombo, WM_SETFONT, (WPARAM)hFontMain, TRUE);
    SendMessageW(hGpuCombo, CB_ADDSTRING, 0, (LPARAM)L"Auto-Detect (Best Available)");
    SendMessageW(hGpuCombo, CB_ADDSTRING, 0, (LPARAM)L"Force Discrete GPU");
    SendMessageW(hGpuCombo, CB_ADDSTRING, 0, (LPARAM)L"Force Integrated GPU");
    SendMessageW(hGpuCombo, CB_SETCURSEL, 0, 0);

    /* primary action button */
    hBtnLaunch = CreateWindowExW(0, L"BUTTON", L"LAUNCH GAME",
        WS_CHILD | WS_VISIBLE | BS_OWNERDRAW | WS_TABSTOP,
        24, 356, 580, 46, hWnd, (HMENU)ID_BTN_LAUNCH, NULL, NULL);
    SendMessageW(hBtnLaunch, WM_SETFONT, (WPARAM)hFontBold, TRUE);

    /* secondary buttons */
    hBtnInspect = CreateWindowExW(0, L"BUTTON", L"Inspect Game",
        WS_CHILD | WS_VISIBLE | BS_OWNERDRAW | WS_TABSTOP,
        24, 414, 180, 34, hWnd, (HMENU)ID_BTN_INSPECT, NULL, NULL);
    SendMessageW(hBtnInspect, WM_SETFONT, (WPARAM)hFontMain, TRUE);

    hBtnExtract = CreateWindowExW(0, L"BUTTON", L"Extract PKG",
        WS_CHILD | WS_VISIBLE | BS_OWNERDRAW | WS_TABSTOP,
        224, 414, 180, 34, hWnd, (HMENU)ID_BTN_EXTRACT, NULL, NULL);
    SendMessageW(hBtnExtract, WM_SETFONT, (WPARAM)hFontMain, TRUE);

    hBtnSync = CreateWindowExW(0, L"BUTTON", L"Sync Launchers",
        WS_CHILD | WS_VISIBLE | BS_OWNERDRAW | WS_TABSTOP,
        424, 414, 180, 34, hWnd, (HMENU)ID_BTN_SYNC, NULL, NULL);
    SendMessageW(hBtnSync, WM_SETFONT, (WPARAM)hFontMain, TRUE);

    populateGames();

    ShowWindow(hWnd, SW_SHOW);
    UpdateWindow(hWnd);

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    ExitProcess((DWORD)msg.wParam);
}
