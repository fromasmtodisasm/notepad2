/******************************************************************************
*
*
* Notepad2
*
* Notepad2.c
*   Main application window functionality
*
* See Readme.txt for more information about this source code.
* Please send me your comments to this work.
*
* See License.txt for details about distribution and modification.
*
*                                              (c) Florian Balmer 1996-2011
*                                                  florian.balmer@gmail.com
*                                               http://www.flos-freeware.ch
*
*
******************************************************************************/

#include <windows.h>
#include <windowsx.h>
#include <shlwapi.h>
#include <shlobj.h>
#include <shellapi.h>
#include <commctrl.h>
#include <commdlg.h>
#include <uxtheme.h>
#include <stdio.h>
#include <time.h>
#include <stdint.h>
#include <inttypes.h>
#include "Notepad2.h"
#include "Edit.h"
#include "Styles.h"
#include "Helpers.h"
#include "Dialogs.h"
#include "SciCall.h"
#include "resource.h"

// show fold level
#define NP2_DEBUG_FOLD_LEVEL	0
// enable the .LOG feature
#define NP2_ENABLE_DOT_LOG_FEATURE	0
// enable call tips (currently not yet implemented)
#define NP2_ENABLE_SHOW_CALL_TIPS	0
// enable customize toolbar labels
#define NP2_ENABLE_CUSTOMIZE_TOOLBAR_LABELS		0

/******************************************************************************
*
* Local and global Variables for Notepad2.c
*
*/
HWND	hwndStatus;
static HWND hwndToolbar;
static HWND hwndReBar;
HWND	hwndEdit;
static HWND hwndEditFrame;
HWND	hwndMain;
static HWND hwndNextCBChain = NULL;
HWND	hDlgFindReplace = NULL;

#define NUMTOOLBITMAPS		25
#define MARGIN_LINE_NUMBER	0	// line number
#define MARGIN_SELECTION	1	// selection margin
#define MARGIN_FOLD_INDEX	2	// folding index

#define TOOLBAR_COMMAND_BASE	IDT_FILE_NEW
#define DefaultToolbarButtons	L"22 3 0 1 2 0 4 18 19 0 5 6 0 7 20 8 9 0 10 11 0 12 0 24 0 13 14 0 15 16 17 0"
static TBBUTTON tbbMainWnd[] = {
	{0, 	0, 					0, 				 TBSTYLE_SEP, {0}, 0, 0},
	{0, 	IDT_FILE_NEW, 		TBSTATE_ENABLED, TBSTYLE_BUTTON, {0}, 0, 0},
	{1, 	IDT_FILE_OPEN, 		TBSTATE_ENABLED, TBSTYLE_BUTTON, {0}, 0, 0},
	{2, 	IDT_FILE_BROWSE, 	TBSTATE_ENABLED, TBSTYLE_BUTTON, {0}, 0, 0},
	{3, 	IDT_FILE_SAVE, 		TBSTATE_ENABLED, TBSTYLE_BUTTON, {0}, 0, 0},
	{4, 	IDT_EDIT_UNDO, 		TBSTATE_ENABLED, TBSTYLE_BUTTON, {0}, 0, 0},
	{5, 	IDT_EDIT_REDO, 		TBSTATE_ENABLED, TBSTYLE_BUTTON, {0}, 0, 0},
	{6, 	IDT_EDIT_CUT, 		TBSTATE_ENABLED, TBSTYLE_BUTTON, {0}, 0, 0},
	{7, 	IDT_EDIT_COPY, 		TBSTATE_ENABLED, TBSTYLE_BUTTON, {0}, 0, 0},
	{8, 	IDT_EDIT_PASTE, 	TBSTATE_ENABLED, TBSTYLE_BUTTON, {0}, 0, 0},
	{9, 	IDT_EDIT_FIND, 		TBSTATE_ENABLED, TBSTYLE_BUTTON, {0}, 0, 0},
	{10, 	IDT_EDIT_REPLACE, 	TBSTATE_ENABLED, TBSTYLE_BUTTON, {0}, 0, 0},
	{11, 	IDT_VIEW_WORDWRAP, 	TBSTATE_ENABLED, TBSTYLE_BUTTON, {0}, 0, 0},
	{12, 	IDT_VIEW_ZOOMIN, 	TBSTATE_ENABLED, TBSTYLE_BUTTON, {0}, 0, 0},
	{13, 	IDT_VIEW_ZOOMOUT, 	TBSTATE_ENABLED, TBSTYLE_BUTTON, {0}, 0, 0},
	{14, 	IDT_VIEW_SCHEME, 	TBSTATE_ENABLED, TBSTYLE_BUTTON, {0}, 0, 0},
	{15, 	IDT_VIEW_SCHEMECONFIG, 	TBSTATE_ENABLED, TBSTYLE_BUTTON, {0}, 0, 0},
	{16, 	IDT_FILE_EXIT, 		TBSTATE_ENABLED, TBSTYLE_BUTTON, {0}, 0, 0},
	{17, 	IDT_FILE_SAVEAS, 	TBSTATE_ENABLED, TBSTYLE_BUTTON, {0}, 0, 0},
	{18, 	IDT_FILE_SAVECOPY, 	TBSTATE_ENABLED, TBSTYLE_BUTTON, {0}, 0, 0},
	{19, 	IDT_EDIT_CLEAR, 	TBSTATE_ENABLED, TBSTYLE_BUTTON, {0}, 0, 0},
	{20, 	IDT_FILE_PRINT, 	TBSTATE_ENABLED, TBSTYLE_BUTTON, {0}, 0, 0},
	{21, 	IDT_FILE_OPENFAV, 	TBSTATE_ENABLED, TBSTYLE_BUTTON, {0}, 0, 0},
	{22, 	IDT_FILE_ADDTOFAV, 	TBSTATE_ENABLED, TBSTYLE_BUTTON, {0}, 0, 0},
	{23, 	IDT_VIEW_TOGGLEFOLDS, 	TBSTATE_ENABLED, BTNS_WHOLEDROPDOWN, {0}, 0, 0},
	{24, 	IDT_FILE_LAUNCH, 	TBSTATE_ENABLED, TBSTYLE_BUTTON, {0}, 0, 0},
};

WCHAR	szIniFile[MAX_PATH] = L"";
static WCHAR szIniFile2[MAX_PATH] = L"";
static BOOL bSaveSettings;
BOOL	bSaveRecentFiles;
static BOOL bSaveFindReplace;
static WCHAR tchLastSaveCopyDir[MAX_PATH] = L"";
WCHAR	tchOpenWithDir[MAX_PATH];
WCHAR	tchFavoritesDir[MAX_PATH];
static WCHAR tchDefaultDir[MAX_PATH];
static WCHAR tchDefaultExtension[64];
LPWSTR	tchFileDlgFilters = NULL;
static WCHAR tchToolbarButtons[MAX_TOOLBAR_BUTTON_CONFIG_BUFFER_SIZE];
static LPWSTR tchToolbarBitmap = NULL;
static LPWSTR tchToolbarBitmapHot = NULL;
static LPWSTR tchToolbarBitmapDisabled = NULL;
static int iPathNameFormat;
BOOL	fWordWrap;
BOOL	fWordWrapG;
int		iWordWrapMode;
int		iWordWrapIndent;
int		iWordWrapSymbols;
BOOL	bShowWordWrapSymbols;
BOOL	bWordWrapSelectSubLine;
static BOOL bShowUnicodeControlCharacter;
static BOOL bMatchBraces;
static BOOL bShowIndentGuides;
BOOL	bHighlightCurrentSubLine;
INT		iHighlightCurrentLine;
BOOL	bTabsAsSpaces;
BOOL	bTabsAsSpacesG;
BOOL	bTabIndents;
BOOL	bTabIndentsG;
BOOL	bBackspaceUnindents;
static int iZoomLevel = 100;
int		iTabWidth;
int		iTabWidthG;
int		iIndentWidth;
int		iIndentWidthG;
static BOOL bMarkLongLines;
int		iLongLinesLimit;
int		iLongLinesLimitG;
int		iLongLineMode;
int		iWrapCol = 0;
static BOOL bShowSelectionMargin;
static BOOL bShowLineNumbers;
static int iMarkOccurrences;
static BOOL bMarkOccurrencesMatchCase;
static BOOL bMarkOccurrencesMatchWords;
struct EditAutoCompletionConfig autoCompletionConfig;
static BOOL bShowCodeFolding;
#if NP2_ENABLE_SHOW_CALL_TIPS
static BOOL bShowCallTips = FALSE;
static int iCallTipsWaitTime = 500; // 500 ms
#endif
static BOOL bViewWhiteSpace;
static BOOL bViewEOLs;
int		iDefaultEncoding;
BOOL	bSkipUnicodeDetection;
BOOL	bLoadANSIasUTF8;
BOOL	bLoadNFOasOEM;
BOOL	bNoEncodingTags;
int		iSrcEncoding = -1;
int		iWeakSrcEncoding = -1;
int		iDefaultEOLMode;
BOOL	bWarnLineEndings;
BOOL	bFixLineEndings;
BOOL	bAutoStripBlanks;
int		iPrintHeader;
int		iPrintFooter;
int		iPrintColor;
int		iPrintZoom = 100;
RECT	pageSetupMargin;
static BOOL bSaveBeforeRunningTools;
BOOL bOpenFolderWithMetapath;
int		iFileWatchingMode;
BOOL	bResetFileWatching;
static DWORD dwFileCheckInverval;
static DWORD dwAutoReloadTimeout;
DWORD	dwFileLoadWarningMB;
static int iEscFunction;
static BOOL bAlwaysOnTop;
static BOOL bMinimizeToTray;
static BOOL bTransparentMode;
BOOL	bFindReplaceTransparentMode;
static BOOL bEditLayoutRTL;
BOOL	bWindowLayoutRTL;
int		iRenderingTechnology;
BOOL	bUseInlineIME;
BOOL	bInlineIMEUseBlockCaret;
int		iBidirectional;
static BOOL bShowToolbar;
static BOOL bShowStatusbar;
static BOOL bInFullScreenMode;
static int iFullScreenMode;

typedef struct _wi {
	int x;
	int y;
	int cx;
	int cy;
	BOOL max;
} WININFO;

static WININFO wi;
static BOOL bStickyWinPos;
//BOOL	bIsAppThemed;

static int cyReBar;
static int cyReBarFrame;
static int cxEditFrame;
static int cyEditFrame;

int		cxRunDlg;
int		cxEncodingDlg;
int		cyEncodingDlg;
int		cxRecodeDlg;
int		cyRecodeDlg;
int		cxFileMRUDlg;
int		cyFileMRUDlg;
int		cxOpenWithDlg;
int		cyOpenWithDlg;
int		cxFavoritesDlg;
int		cyFavoritesDlg;
int		cxAddFavoritesDlg;
int		cxModifyLinesDlg;
int		cyModifyLinesDlg;
int		cxEncloseSelectionDlg;
int		cyEncloseSelectionDlg;
int		cxInsertTagDlg;
int		cyInsertTagDlg;
int		xFindReplaceDlg;
int		yFindReplaceDlg;
int		cxFindReplaceDlg;

static LPWSTR lpFileList[32];
static int cFileList = 0;
static int cchiFileList = 0;
static LPWSTR lpFileArg = NULL;
static LPWSTR lpSchemeArg = NULL;
static LPWSTR lpMatchArg = NULL;
static LPWSTR lpEncodingArg = NULL;
LPMRULIST	pFileMRU;
LPMRULIST	mruFind;
LPMRULIST	mruReplace;

DWORD	dwLastIOError;
WCHAR	szCurFile[MAX_PATH + 40];
FILEVARS	fvCurFile;
static BOOL bModified;
static BOOL bReadOnly = FALSE;
static int iOriginalEncoding;
static int iEOLMode;

int		iEncoding;
// DBCS code page
int		iDefaultCodePage;
int		iDefaultCharSet;

static int iInitialLine;
static int iInitialColumn;
static int iInitialLexer;

static BOOL bLastCopyFromMe = FALSE;
static DWORD dwLastCopyTime;

static UINT uidsAppTitle = IDS_APPTITLE;
static WCHAR szTitleExcerpt[128] = L"";
static int fKeepTitleExcerpt = 0;

static HANDLE hChangeHandle = NULL;
static BOOL bRunningWatch = FALSE;
static DWORD dwChangeNotifyTime = 0;

static UINT msgTaskbarCreated = 0;

static WIN32_FIND_DATA fdCurFile;
static EDITFINDREPLACE efrData;
UINT	cpLastFind = 0;
BOOL	bReplaceInitialized = FALSE;

const int iLineEndings[3] = {
	SC_EOL_CRLF,
	SC_EOL_LF,
	SC_EOL_CR
};

static int iSortOptions = 0;
static int iAlignMode	= 0;
int		iMatchesCount	= 0;
extern int iFontQuality;
extern int iCaretStyle;
extern int iOvrCaretStyle;
extern int iCaretBlinkPeriod;

static BOOL fIsElevated = FALSE;
static WCHAR wchWndClass[16] = WC_NOTEPAD2;

#define STATUS_BAR_UPDATE_MASK_LEXER	1
#define STATUS_BAR_UPDATE_MASK_CODEPAGE	2
#define STATUS_BAR_UPDATE_MASK_EOLMODE	4
#define STATUS_BAR_UPDATE_MASK_OVRMODE	8
#define STATUS_BAR_UPDATE_MASK_DOCZOOM	16
// rarely changed statusbar items
struct CachedStatusItem {
	UINT updateMask;
	BOOL overType;

	LPCWSTR pszLexerName;
	LPCWSTR pszEOLMode;
	LPCWSTR pszOvrMode;
	WCHAR tchZoom[8];

#if NP2_GET_LEXER_STYLE_NAME_FROM_RES
	WCHAR tchLexerName[MAX_EDITLEXER_NAME_SIZE];
#endif
	WCHAR tchDocPosFmt[96];
} cachedStatusItem;

HINSTANCE	g_hInstance;
HANDLE		g_hDefaultHeap;
HANDLE		g_hScintilla;
#if _WIN32_WINNT < _WIN32_WINNT_WIN10
DWORD		g_uWinVer;
#endif
#if _WIN32_WINNT < _WIN32_WINNT_WIN8
DWORD		kSystemLibraryLoadFlags = 0;
#endif
UINT		g_uCurrentDPI = USER_DEFAULT_SCREEN_DPI;
UINT		g_uDefaultDPI = USER_DEFAULT_SCREEN_DPI;
static WCHAR g_wchAppUserModelID[38] = L"";
static WCHAR g_wchWorkingDirectory[MAX_PATH] = L"";

#define	NP2_BookmarkLineForeColor	(0xff << 8)
#define NP2_BookmarkLineColorAlpha	40

//Graphics for bookmark indicator
/* XPM */
static const char* const bookmark_pixmap[] = {
	"11 11 44 1",
	" 	c #EBE9ED",
	".	c #E5E3E7",
	"+	c #767C6D",
	"@	c #2A3120",
	"#	c #1B2312",
	"$	c #333B28",
	"%	c #E3E1E5",
	"&	c #D8D6DA",
	"*	c #444D38",
	"=	c #3F5C19",
	"-	c #63AD00",
	";	c #73C900",
	">	c #64AF00",
	", 	c #3D5718",
	"'	c #3E4634",
	")	c #7B8172",
	"!	c #42601A",
	"~	c #74CB00",
	"{	c #71C600",
	"]	c #3A5317",
	"^	c #707668",
	"/	c #3F4931",
	"(	c #262C1D",
	"_	c #2F3A1E",
	":	c #72C700",
	"<	c #74CA00",
	"[	c #0E1109",
	"}	c #3C462F",
	"|	c #62AC00",
	"1	c #21271A",
	"2	c #7A8071",
	"3	c #405D19",
	"4	c #3D5A18",
	"5	c #D9D7DB",
	"6	c #4E5841",
	"7	c #72C800",
	"8	c #63AC00",
	"9	c #3F5B19",
	"0	c #3D4533",
	"a	c #DFDDE0",
	"b	c #353E29",
	"c	c #29331B",
	"d	c #7B8272",
	"e	c #DDDBDF",
	"           ",
	"  .+@#$+%  ",
	" &*=-;>, '  ",
	")!~~~~{]^ ",
	" /-~~~~~>(",
	" _:~~~~~<[ ",
	" }|~~~~~|1 ",
	" 23~~~~;4+ ",
	" 56=|7890  ",
	"  a2bc}de  ",
	"           "
};

//=============================================================================
//
// Flags
//
static int	flagNoReuseWindow		= 0;
static int	flagReuseWindow			= 0;
static BOOL bSingleFileInstance		= TRUE;
static BOOL bReuseWindow			= FALSE;
static int	flagMultiFileArg		= 0;
static int	flagSingleFileInstance	= 1;
static int	flagStartAsTrayIcon		= 0;
static int	flagAlwaysOnTop			= 0;
static int	flagRelativeFileMRU		= 0;
static int	flagPortableMyDocs		= 0;
int			flagNoFadeHidden		= 0;
static int	iOpacityLevel			= 75;
int			iFindReplaceOpacityLevel= 75;
static int	flagToolbarLook			= 0;
int			flagSimpleIndentGuides	= 0;
BOOL 		fNoHTMLGuess			= 0;
BOOL 		fNoCGIGuess				= 0;
BOOL 		fNoAutoDetection		= 0;
BOOL		fNoFileVariables		= 0;
static int	flagPosParam			= 0;
static int	flagDefaultPos			= 0;
static int	flagNewFromClipboard	= 0;
static int	flagPasteBoard			= 0;
static int	flagSetEncoding			= 0;
static int	flagSetEOLMode			= 0;
static int	flagJumpTo				= 0;
static int	flagMatchText			= 0;
static int	flagChangeNotify		= 0;
static int	flagLexerSpecified		= 0;
static int	flagQuietCreate			= 0;
static int	flagUseSystemMRU		= 0;
static int	flagRelaunchElevated	= 0;
static int	flagDisplayHelp			= 0;

static inline BOOL IsDocumentModified(void) {
	return bModified || iEncoding != iOriginalEncoding;
}

static inline BOOL IsTopMost(void) {
	return (bAlwaysOnTop || flagAlwaysOnTop == 2) && flagAlwaysOnTop != 1;
}

static inline void ToggleFullScreenModeConfig(int config) {
	if (iFullScreenMode & config) {
		iFullScreenMode &= ~config;
	} else {
		iFullScreenMode |= config;
	}
	if (bInFullScreenMode && config != FullScreenMode_OnStartup) {
		ToggleFullScreenMode();
	}
}

static inline void UpdateStatusBarCache_OVRMode(BOOL force) {
	const BOOL overType = (BOOL)SendMessage(hwndEdit, SCI_GETOVERTYPE, 0, 0);
	if (force || overType != cachedStatusItem.overType) {
		cachedStatusItem.overType = overType;
		cachedStatusItem.pszOvrMode = overType ? L"OVR" : L"INS";
		cachedStatusItem.updateMask |= STATUS_BAR_UPDATE_MASK_OVRMODE;
	}
}

//==============================================================================
//
// Folding Functions
//
//
#include "Edit_Fold.c"

//=============================================================================
//
// WinMain()
//
//
static void CleanUpResources(BOOL initialized) {
	if (tchFileDlgFilters != NULL) {
		LocalFree(tchFileDlgFilters);
	}
	if (tchToolbarBitmap != NULL) {
		LocalFree(tchToolbarBitmap);
	}
	if (tchToolbarBitmapHot != NULL) {
		LocalFree(tchToolbarBitmapHot);
	}
	if (tchToolbarBitmapDisabled != NULL) {
		LocalFree(tchToolbarBitmapDisabled);
	}

	Encoding_ReleaseResources();
	Style_ReleaseResources();
	Edit_ReleaseResources();
	Scintilla_ReleaseResources();

	if (initialized) {
		UnregisterClass(wchWndClass, g_hInstance);
	}
	OleUninitialize();
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nShowCmd) {
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	// Set global variable g_hInstance
	g_hInstance = hInstance;

#if _WIN32_WINNT < _WIN32_WINNT_WIN10
	// Set the Windows version global variable
	NP2_COMPILER_WARNING_PUSH
	NP2_IGNORE_WARNING_DEPRECATED_DECLARATIONS
	g_uWinVer = LOWORD(GetVersion());
	NP2_COMPILER_WARNING_POP
	g_uWinVer = MAKEWORD(HIBYTE(g_uWinVer), LOBYTE(g_uWinVer));
#endif

	g_hDefaultHeap = GetProcessHeap();
	// https://docs.microsoft.com/en-us/windows/desktop/Memory/low-fragmentation-heap
#if 0 // default enabled since Vista
	{
		// Enable the low-fragmenation heap (LFH).
		ULONG HeapInformation = /*HEAP_LFH*/2;
		HeapSetInformation(g_hDefaultHeap, HeapCompatibilityInformation, &HeapInformation, sizeof(HeapInformation));
		// Enable heap terminate-on-corruption.
		HeapSetInformation(NULL, HeapEnableTerminationOnCorruption, NULL, 0);
	}
#endif

	// Don't keep working directory locked
	WCHAR wchWorkingDirectory[MAX_PATH];
	GetCurrentDirectory(COUNTOF(g_wchWorkingDirectory), g_wchWorkingDirectory);
	GetModuleFileName(NULL, wchWorkingDirectory, COUNTOF(wchWorkingDirectory));
	PathRemoveFileSpec(wchWorkingDirectory);
	SetCurrentDirectory(wchWorkingDirectory);

	SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);

	// Check if running with elevated privileges
	fIsElevated = IsElevated();

	// Default Encodings (may already be used for command line parsing)
	Encoding_InitDefaults();

	// Command Line, Ini File and Flags
	ParseCommandLine();
	FindIniFile();
	TestIniFile();
	CreateIniFile();
	LoadFlags();

	// set AppUserModelID
	PrivateSetCurrentProcessExplicitAppUserModelID(g_wchAppUserModelID);

	// Command Line Help Dialog
	if (flagDisplayHelp) {
		DisplayCmdLineHelp(NULL);
		return 0;
	}

	// Adapt window class name
	if (fIsElevated) {
		lstrcat(wchWndClass, L"U");
	}
	if (flagPasteBoard) {
		lstrcat(wchWndClass, L"B");
	}

	// Relaunch with elevated privileges
	if (RelaunchElevated()) {
		return 0;
	}

	// Try to run multiple instances
	if (RelaunchMultiInst()) {
		return 0;
	}

	// Try to activate another window
	if (ActivatePrevInst()) {
		return 0;
	}

	// Init OLE and Common Controls
	OleInitialize(NULL);

	INITCOMMONCONTROLSEX icex;
	icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
	icex.dwICC	= ICC_WIN95_CLASSES | ICC_COOL_CLASSES | ICC_BAR_CLASSES | ICC_USEREX_CLASSES;
	InitCommonControlsEx(&icex);

	msgTaskbarCreated = RegisterWindowMessage(L"TaskbarCreated");

#if _WIN32_WINNT < _WIN32_WINNT_WIN8
	// see LoadD2D() in PlatWin.cxx
	kSystemLibraryLoadFlags = (IsWin8AndAbove() || GetProcAddress(GetModuleHandle(L"kernel32.dll"), "SetDefaultDllDirectories")) ? LOAD_LIBRARY_SEARCH_SYSTEM32 : 0;
#endif

	Scintilla_RegisterClasses(hInstance);

	// Load Settings
	LoadSettings();

	if (!InitApplication(hInstance)) {
		CleanUpResources(FALSE);
		return FALSE;
	}

	HWND hwnd;
	if ((hwnd = InitInstance(hInstance, nShowCmd)) == NULL) {
		CleanUpResources(TRUE);
		return FALSE;
	}

	HACCEL hAccMain = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDR_MAINWND));
	HACCEL hAccFindReplace = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDR_ACCFINDREPLACE));
	MSG msg;

	while (GetMessage(&msg, NULL, 0, 0)) {
		if (IsWindow(hDlgFindReplace) && (msg.hwnd == hDlgFindReplace || IsChild(hDlgFindReplace, msg.hwnd))) {
			if (TranslateAccelerator(hDlgFindReplace, hAccFindReplace, &msg) || IsDialogMessage(hDlgFindReplace, &msg)) {
				continue;
			}
		}

		if (!TranslateAccelerator(hwnd, hAccMain, &msg)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	CleanUpResources(TRUE);
	return (int)(msg.wParam);
}

//=============================================================================
//
// InitApplication()
//
//
BOOL InitApplication(HINSTANCE hInstance) {
	WNDCLASSEX wc;
	wc.cbSize        = sizeof(WNDCLASSEX);
	wc.style		 = CS_BYTEALIGNWINDOW | CS_DBLCLKS;
	wc.lpfnWndProc	 = (WNDPROC)MainWndProc;
	wc.cbClsExtra	 = 0;
	wc.cbWndExtra	 = 0;
	wc.hInstance	 = hInstance;
	wc.hIcon		 = LoadIcon(hInstance, MAKEINTRESOURCE(IDR_MAINWND));
	wc.hCursor		 = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_3DFACE + 1);
	wc.lpszMenuName	 = MAKEINTRESOURCE(IDR_MAINWND);
	wc.lpszClassName = wchWndClass;
	wc.hIconSm       = NULL;

	return RegisterClassEx(&wc);
}

//=============================================================================
//
// InitInstance()
//
//
HWND InitInstance(HINSTANCE hInstance, int nCmdShow) {
	RECT rc = { wi.x, wi.y, wi.x + wi.cx, wi.y + wi.cy };

	HMONITOR hMonitor = MonitorFromRect(&rc, MONITOR_DEFAULTTONEAREST);
	MONITORINFO mi;
	mi.cbSize = sizeof(mi);
	GetMonitorInfo(hMonitor, &mi);

	if (flagDefaultPos == 1) {
		wi.x = wi.y = wi.cx = wi.cy = CW_USEDEFAULT;
		wi.max = 0;
	} else if (flagDefaultPos >= 4) {
		SystemParametersInfo(SPI_GETWORKAREA, 0, &rc, 0);
		if (flagDefaultPos & 8) {
			wi.x = (rc.right - rc.left) / 2;
		} else {
			wi.x = rc.left;
		}
		wi.cx = rc.right - rc.left;
		if (flagDefaultPos & (4 | 8)) {
			wi.cx /= 2;
		}
		if (flagDefaultPos & 32) {
			wi.y = (rc.bottom - rc.top) / 2;
		} else {
			wi.y = rc.top;
		}
		wi.cy = rc.bottom - rc.top;
		if (flagDefaultPos & (16 | 32)) {
			wi.cy /= 2;
		}
		if (flagDefaultPos & 64) {
			wi.x = rc.left;
			wi.y = rc.top;
			wi.cx = rc.right - rc.left;
			wi.cy = rc.bottom - rc.top;
		}
		if (flagDefaultPos & 128) {
			wi.x += (flagDefaultPos & 8) ? 4 : 8;
			wi.cx -= (flagDefaultPos & (4 | 8)) ? 12 : 16;
			wi.y += (flagDefaultPos & 32) ? 4 : 8;
			wi.cy -= (flagDefaultPos & (16 | 32)) ? 12 : 16;
		}
	} else if (flagDefaultPos == 2 || flagDefaultPos == 3 ||
			   wi.x == CW_USEDEFAULT || wi.y == CW_USEDEFAULT ||
			   wi.cx == CW_USEDEFAULT || wi.cy == CW_USEDEFAULT) {

		// default window position
		SystemParametersInfo(SPI_GETWORKAREA, 0, &rc, 0);
		wi.y = rc.top + 16;
		wi.cy = rc.bottom - rc.top - 32;
		wi.cx = min_i(rc.right - rc.left - 32, wi.cy);
		wi.x = (flagDefaultPos == 3) ? rc.left + 16 : rc.right - wi.cx - 16;
	} else {

		// fit window into working area of current monitor
		wi.x += (mi.rcWork.left - mi.rcMonitor.left);
		wi.y += (mi.rcWork.top - mi.rcMonitor.top);
		if (wi.x < mi.rcWork.left) {
			wi.x = mi.rcWork.left;
		}
		if (wi.y < mi.rcWork.top) {
			wi.y = mi.rcWork.top;
		}
		if (wi.x + wi.cx > mi.rcWork.right) {
			wi.x -= (wi.x + wi.cx - mi.rcWork.right);
			if (wi.x < mi.rcWork.left) {
				wi.x = mi.rcWork.left;
			}
			if (wi.x + wi.cx > mi.rcWork.right) {
				wi.cx = mi.rcWork.right - wi.x;
			}
		}
		if (wi.y + wi.cy > mi.rcWork.bottom) {
			wi.y -= (wi.y + wi.cy - mi.rcWork.bottom);
			if (wi.y < mi.rcWork.top) {
				wi.y = mi.rcWork.top;
			}
			if (wi.y + wi.cy > mi.rcWork.bottom) {
				wi.cy = mi.rcWork.bottom - wi.y;
			}
		}
		SetRect(&rc, wi.x, wi.y, wi.x + wi.cx, wi.y + wi.cy);
		RECT rc2;
		if (!IntersectRect(&rc2, &rc, &mi.rcWork)) {
			wi.y = mi.rcWork.top + 16;
			wi.cy = mi.rcWork.bottom - mi.rcWork.top - 32;
			wi.cx = min_i(mi.rcWork.right - mi.rcWork.left - 32, wi.cy);
			wi.x = mi.rcWork.right - wi.cx - 16;
		}
	}

	hwndMain = CreateWindowEx(
				   0,
				   wchWndClass,
				   L"Notepad2",
				   WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
				   wi.x,
				   wi.y,
				   wi.cx,
				   wi.cy,
				   NULL,
				   NULL,
				   hInstance,
				   NULL);

	if (wi.max) {
		nCmdShow = SW_SHOWMAXIMIZED;
	}

	if (IsTopMost()) {
		SetWindowPos(hwndMain, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
	}

	if (bTransparentMode) {
		SetWindowTransparentMode(hwndMain, TRUE, iOpacityLevel);
	}

	// Current file information -- moved in front of ShowWindow()
	//FileLoad(TRUE, TRUE, FALSE, FALSE, L"");
	//
	if (!flagStartAsTrayIcon) {
		ShowWindow(hwndMain, nCmdShow);
		UpdateWindow(hwndMain);
	} else {
		ShowWindow(hwndMain, SW_HIDE); // trick ShowWindow()
		ShowNotifyIcon(hwndMain, TRUE);
	}

	// Source Encoding
	if (lpEncodingArg) {
		iSrcEncoding = Encoding_Match(lpEncodingArg);
	}

	BOOL bOpened = FALSE;
	BOOL bFileLoadCalled = FALSE;
	// Pathname parameter
	if (lpFileArg /*&& !flagNewFromClipboard*/) {

		// Open from Directory
		if (PathIsDirectory(lpFileArg)) {
			WCHAR tchFile[MAX_PATH];
			if (OpenFileDlg(hwndMain, tchFile, COUNTOF(tchFile), lpFileArg)) {
				bOpened = FileLoad(FALSE, FALSE, FALSE, FALSE, tchFile);
				bFileLoadCalled = TRUE;
			}
		} else {
			if ((bOpened = FileLoad(FALSE, FALSE, FALSE, FALSE, lpFileArg)) != FALSE) {
				bFileLoadCalled = TRUE;
				if (flagJumpTo) { // Jump to position
					EditJumpTo(hwndEdit, iInitialLine, iInitialColumn);
					EditEnsureSelectionVisible(hwndEdit);
				}
			}
		}
		NP2HeapFree(lpFileArg);

		if (bOpened) {
			if (flagChangeNotify == 1) {
				iFileWatchingMode = 0;
				bResetFileWatching = TRUE;
				InstallFileWatching(szCurFile);
			} else if (flagChangeNotify == 2) {
				iFileWatchingMode = 2;
				bResetFileWatching = TRUE;
				InstallFileWatching(szCurFile);
			}
		}
	} else {
		if (iSrcEncoding != -1) {
			iEncoding = iSrcEncoding;
			iOriginalEncoding = iSrcEncoding;
			SendMessage(hwndEdit, SCI_SETCODEPAGE, (iEncoding == CPI_DEFAULT) ? iDefaultCodePage : SC_CP_UTF8, 0);
		}
	}

	if (!bFileLoadCalled) {
		bOpened = FileLoad(TRUE, TRUE, FALSE, FALSE, L"");
	}

	// reset
	iSrcEncoding = -1;
	flagQuietCreate = 0;
	fKeepTitleExcerpt = 0;

	// Check for /c [if no file is specified] -- even if a file is specified
	/*else */
	if (flagNewFromClipboard) {
		if (SendMessage(hwndEdit, SCI_CANPASTE, 0, 0)) {
			const BOOL back = autoCompletionConfig.bIndentText;
			autoCompletionConfig.bIndentText = FALSE;
			EditJumpTo(hwndEdit, -1, 0);
			SendMessage(hwndEdit, SCI_BEGINUNDOACTION, 0, 0);
			if (SendMessage(hwndEdit, SCI_GETLENGTH, 0, 0) > 0) {
				SendMessage(hwndEdit, SCI_NEWLINE, 0, 0);
			}
			SendMessage(hwndEdit, SCI_PASTE, 0, 0);
			SendMessage(hwndEdit, SCI_NEWLINE, 0, 0);
			SendMessage(hwndEdit, SCI_ENDUNDOACTION, 0, 0);
			autoCompletionConfig.bIndentText = back;
			if (flagJumpTo) {
				EditJumpTo(hwndEdit, iInitialLine, iInitialColumn);
			}
			EditEnsureSelectionVisible(hwndEdit);
		}
	}

	// Encoding
	if (0 != flagSetEncoding) {
		SendWMCommand(hwndMain, IDM_ENCODING_ANSI + flagSetEncoding - 1);
		flagSetEncoding = 0;
	}

	// EOL mode
	if (0 != flagSetEOLMode) {
		SendWMCommand(hwndMain, IDM_LINEENDINGS_CRLF + flagSetEOLMode - 1);
		flagSetEOLMode = 0;
	}

	// Match Text
	if (flagMatchText && lpMatchArg) {
		if (StrNotEmpty(lpMatchArg) && SendMessage(hwndEdit, SCI_GETLENGTH, 0, 0)) {
			const UINT cpEdit = (UINT)SendMessage(hwndEdit, SCI_GETCODEPAGE, 0, 0);
			WideCharToMultiByte(cpEdit, 0, lpMatchArg, -1, efrData.szFind, COUNTOF(efrData.szFind), NULL, NULL);
			WideCharToMultiByte(CP_UTF8, 0, lpMatchArg, -1, efrData.szFindUTF8, COUNTOF(efrData.szFindUTF8), NULL, NULL);
			cpLastFind = cpEdit;

			if (flagMatchText & 4) {
				efrData.fuFlags |= SCFIND_REGEXP | SCFIND_POSIX;
			} else if (flagMatchText & 8) {
				efrData.bTransformBS = TRUE;
			}

			if (flagMatchText & 2) {
				if (!flagJumpTo) {
					EditJumpTo(hwndEdit, -1, 0);
				}
				EditFindPrev(hwndEdit, &efrData, FALSE);
				EditEnsureSelectionVisible(hwndEdit);
			} else {
				if (!flagJumpTo) {
					SendMessage(hwndEdit, SCI_DOCUMENTSTART, 0, 0);
				}
				EditFindNext(hwndEdit, &efrData, FALSE);
				EditEnsureSelectionVisible(hwndEdit);
			}
		}
		LocalFree(lpMatchArg);
	}

	// Check for Paste Board option -- after loading files
	if (flagPasteBoard) {
		bLastCopyFromMe = TRUE;
		hwndNextCBChain = SetClipboardViewer(hwndMain);
		uidsAppTitle = IDS_APPTITLE_PASTEBOARD;
		UpdateWindowTitle();
		bLastCopyFromMe = FALSE;
		dwLastCopyTime = 0;
		SetTimer(hwndMain, ID_PASTEBOARDTIMER, 100, PasteBoardTimer);
	}

	// check if a lexer was specified from the command line
	if (flagLexerSpecified) {
		if (lpSchemeArg) {
			Style_SetLexerFromName(hwndEdit, szCurFile, lpSchemeArg);
			LocalFree(lpSchemeArg);
		} else if (iInitialLexer >= 0 && iInitialLexer < NUMLEXERS) {
			Style_SetLexerFromID(hwndEdit, iInitialLexer);
		}
		flagLexerSpecified = 0;
	}

	// If start as tray icon, set current filename as tooltip
	if (flagStartAsTrayIcon) {
		SetNotifyIconTitle(hwndMain);
	}

	if (!bOpened) {
		UpdateStatusBarCache(STATUS_CODEPAGE);
		UpdateStatusBarCache(STATUS_EOLMODE);
	}
	UpdateStatusBarCache_OVRMode(TRUE);
	UpdateStatusBarCache(STATUS_DOCZOOM);
	UpdateToolbar();

	return hwndMain;
}

static inline void NP2MinimizeWind(HWND hwnd) {
	MinimizeWndToTray(hwnd);
	ShowNotifyIcon(hwnd, TRUE);
	SetNotifyIconTitle(hwnd);
}

static inline void NP2RestoreWind(HWND hwnd) {
	ShowNotifyIcon(hwnd, FALSE);
	RestoreWndFromTray(hwnd);
	ShowOwnedPopups(hwnd, TRUE);
}

static inline void NP2ExitWind(HWND hwnd) {
	if (FileSave(FALSE, TRUE, FALSE, FALSE)) {
		if (bInFullScreenMode) {
			bInFullScreenMode = FALSE;
			ToggleFullScreenMode();
		}
		DestroyWindow(hwnd);
	}
}

void OnDropOneFile(HWND hwnd, LPCWSTR szBuf) {
	// Reset Change Notify
	//bPendingChangeNotify = FALSE;
	if (IsIconic(hwnd)) {
		ShowWindow(hwnd, SW_RESTORE);
	}
	//SetForegroundWindow(hwnd);
	if (PathIsDirectory(szBuf)) {
		WCHAR tchFile[MAX_PATH];
		if (OpenFileDlg(hwndMain, tchFile, COUNTOF(tchFile), szBuf)) {
			FileLoad(FALSE, FALSE, FALSE, FALSE, tchFile);
		}
	} else {
		FileLoad(FALSE, FALSE, FALSE, FALSE, szBuf);
	}
}

//=============================================================================
//
// MainWndProc()
//
// Messages are distributed to the MsgXXX-handlers
//
//
LRESULT CALLBACK MainWndProc(HWND hwnd, UINT umsg, WPARAM wParam, LPARAM lParam) {
	static BOOL bShutdownOK;

	switch (umsg) {
	// Quickly handle painting and sizing messages, found in ScintillaWin.cxx
	// Cool idea, don't know if this has any effect... ;-)
	case WM_MOVE:
	case WM_MOUSEACTIVATE:
	case WM_NCHITTEST:
	case WM_NCCALCSIZE:
	case WM_NCPAINT:
	case WM_PAINT:
	case WM_ERASEBKGND:
	case WM_NCMOUSEMOVE:
	case WM_NCLBUTTONDOWN:
	case WM_WINDOWPOSCHANGING:
	case WM_WINDOWPOSCHANGED:
		return DefWindowProc(hwnd, umsg, wParam, lParam);

	case WM_CREATE:
		return MsgCreate(hwnd, wParam, lParam);

	case WM_DESTROY:
	case WM_ENDSESSION:
		if (!bShutdownOK) {
			WINDOWPLACEMENT wndpl;

			// Terminate file watching
			InstallFileWatching(NULL);

			// GetWindowPlacement
			wndpl.length = sizeof(WINDOWPLACEMENT);
			GetWindowPlacement(hwnd, &wndpl);

			wi.x = wndpl.rcNormalPosition.left;
			wi.y = wndpl.rcNormalPosition.top;
			wi.cx = wndpl.rcNormalPosition.right - wndpl.rcNormalPosition.left;
			wi.cy = wndpl.rcNormalPosition.bottom - wndpl.rcNormalPosition.top;
			wi.max = (IsZoomed(hwnd) || (wndpl.flags & WPF_RESTORETOMAXIMIZED));

			DragAcceptFiles(hwnd, FALSE);

			// Terminate clipboard watching
			if (flagPasteBoard) {
				KillTimer(hwnd, ID_PASTEBOARDTIMER);
				ChangeClipboardChain(hwnd, hwndNextCBChain);
			}

			// Destroy find / replace dialog
			if (IsWindow(hDlgFindReplace)) {
				DestroyWindow(hDlgFindReplace);
			}

			// call SaveSettings() when hwndToolbar is still valid
			SaveSettings(FALSE);

			if (StrNotEmpty(szIniFile)) {

				// Cleanup unwanted MRU's
				if (!bSaveRecentFiles) {
					MRU_Empty(pFileMRU);
					MRU_Save(pFileMRU);
				} else {
					MRU_MergeSave(pFileMRU, TRUE, flagRelativeFileMRU, flagPortableMyDocs);
				}
				MRU_Destroy(pFileMRU);

				if (!bSaveFindReplace) {
					MRU_Empty(mruFind);
					MRU_Empty(mruReplace);
					MRU_Save(mruFind);
					MRU_Save(mruReplace);
				} else {
					MRU_MergeSave(mruFind, FALSE, FALSE, FALSE);
					MRU_MergeSave(mruReplace, FALSE, FALSE, FALSE);
				}
				MRU_Destroy(mruFind);
				MRU_Destroy(mruReplace);
			}

			// Remove tray icon if necessary
			ShowNotifyIcon(hwnd, FALSE);

			bShutdownOK = TRUE;
		}
		if (umsg == WM_DESTROY) {
			PostQuitMessage(0);
		}
		break;

	case WM_CLOSE:
		if (bMinimizeToTray) {
			NP2MinimizeWind(hwnd);
		} else {
			NP2ExitWind(hwnd);
		}
		break;

	case WM_QUERYENDSESSION:
		if (FileSave(FALSE, TRUE, FALSE, FALSE)) {
			return TRUE;
		}
		return FALSE;

	// Reinitialize theme-dependent values and resize windows
	case WM_THEMECHANGED:
		MsgThemeChanged(hwnd, wParam, lParam);
		break;

	case WM_DPICHANGED:
		MsgDPIChanged(hwnd, wParam, lParam);
		break;

	// update Scintilla colors
	case WM_SYSCOLORCHANGE: {
		Style_SetLexer(hwndEdit, pLexCurrent);
		return DefWindowProc(hwnd, umsg, wParam, lParam);
	}

	//case WM_TIMER:
	//	break;

	case WM_SIZE:
		MsgSize(hwnd, wParam, lParam);
		break;

	case WM_GETMINMAXINFO:
		if (bInFullScreenMode) {
			HMONITOR hMonitor = MonitorFromWindow(hwndMain, MONITOR_DEFAULTTONEAREST);
			MONITORINFO mi;
			mi.cbSize = sizeof(mi);
			GetMonitorInfo(hMonitor, &mi);

			const int w = mi.rcMonitor.right - mi.rcMonitor.left;
			const int h = mi.rcMonitor.bottom - mi.rcMonitor.top;

			LPMINMAXINFO pmmi = (LPMINMAXINFO)lParam;
			pmmi->ptMaxSize.x = w + 2 * GetSystemMetricsEx(SM_CXSIZEFRAME);
			pmmi->ptMaxSize.y = h + GetSystemMetricsEx(SM_CYCAPTION) + GetSystemMetricsEx(SM_CYMENU) + 2 * GetSystemMetricsEx(SM_CYSIZEFRAME);
			pmmi->ptMaxTrackSize.x = pmmi->ptMaxSize.x;
			pmmi->ptMaxTrackSize.y = pmmi->ptMaxSize.y;
			return 0;
		}
		return DefWindowProc(hwnd, umsg, wParam, lParam);

	case WM_SETFOCUS:
		SetFocus(hwndEdit);

		UpdateToolbar();
		UpdateStatusbar();

		//if (bPendingChangeNotify)
		//	PostMessage(hwnd, APPM_CHANGENOTIFY, 0, 0);
		break;

	case WM_DROPFILES: {
		WCHAR szBuf[MAX_PATH + 40];
		HDROP hDrop = (HDROP)wParam;

		DragQueryFile(hDrop, 0, szBuf, COUNTOF(szBuf));
		OnDropOneFile(hwnd, szBuf);

		//if (DragQueryFile(hDrop, (UINT)(-1), NULL, 0) > 1) {
		//	MsgBox(MBWARN, IDS_ERR_DROP);
		//}

		DragFinish(hDrop);
	}
	break;

	case WM_COPYDATA: {
		PCOPYDATASTRUCT pcds = (PCOPYDATASTRUCT)lParam;

		// Reset Change Notify
		//bPendingChangeNotify = FALSE;

		SetDlgItemInt(hwnd, IDC_REUSELOCK, GetTickCount(), FALSE);

		if (pcds->dwData == DATA_NOTEPAD2_PARAMS) {
			LPNP2PARAMS params = (LPNP2PARAMS)NP2HeapAlloc(pcds->cbData);
			CopyMemory(params, pcds->lpData, pcds->cbData);

			if (params->flagLexerSpecified) {
				flagLexerSpecified = 1;
			}

			if (params->flagQuietCreate) {
				flagQuietCreate = 1;
			}

			if (params->flagFileSpecified) {
				BOOL bOpened = FALSE;
				iSrcEncoding = params->iSrcEncoding;

				if (PathIsDirectory(&params->wchData)) {
					WCHAR tchFile[MAX_PATH];
					if (OpenFileDlg(hwndMain, tchFile, COUNTOF(tchFile), &params->wchData)) {
						bOpened = FileLoad(FALSE, FALSE, FALSE, FALSE, tchFile);
					}
				} else {
					bOpened = FileLoad(FALSE, FALSE, FALSE, FALSE, &params->wchData);
				}

				if (bOpened) {
					if (params->flagChangeNotify == 1) {
						iFileWatchingMode = 0;
						bResetFileWatching = TRUE;
						InstallFileWatching(szCurFile);
					} else if (params->flagChangeNotify == 2) {
						iFileWatchingMode = 2;
						bResetFileWatching = TRUE;
						InstallFileWatching(szCurFile);
					}

					if (0 != params->flagSetEncoding) {
						flagSetEncoding = params->flagSetEncoding;
						SendWMCommand(hwnd, IDM_ENCODING_ANSI + flagSetEncoding - 1);
						flagSetEncoding = 0;
					}

					if (0 != params->flagSetEOLMode) {
						flagSetEOLMode = params->flagSetEOLMode;
						SendWMCommand(hwnd, IDM_LINEENDINGS_CRLF + flagSetEOLMode - 1);
						flagSetEOLMode = 0;
					}

					if (params->flagLexerSpecified) {
						if (params->iInitialLexer < 0) {
							WCHAR wchExt[32] = L".";
							lstrcpyn(CharNext(wchExt), StrEnd(&params->wchData) + 1, 30);
							Style_SetLexerFromName(hwndEdit, &params->wchData, wchExt);
						} else if (params->iInitialLexer >= 0 && params->iInitialLexer < NUMLEXERS) {
							Style_SetLexerFromID(hwndEdit, params->iInitialLexer);
						}
					}

					if (params->flagTitleExcerpt) {
						lstrcpyn(szTitleExcerpt, StrEnd(&params->wchData) + 1, COUNTOF(szTitleExcerpt));
						UpdateWindowTitle();
					}
				}
				// reset
				iSrcEncoding = -1;
			}

			if (params->flagJumpTo) {
				if (params->iInitialLine == 0) {
					params->iInitialLine = 1;
				}
				EditJumpTo(hwndEdit, params->iInitialLine, params->iInitialColumn);
				EditEnsureSelectionVisible(hwndEdit);
			}

			flagLexerSpecified = 0;
			flagQuietCreate = 0;

			NP2HeapFree(params);

			UpdateStatusbar();
		}
	}
	return TRUE;

	case WM_CONTEXTMENU: {
		const int nID = GetDlgCtrlID((HWND)wParam);

		if (!(nID == IDC_EDIT || nID == IDC_STATUSBAR || nID == IDC_REBAR || nID == IDC_TOOLBAR)) {
			return DefWindowProc(hwnd, umsg, wParam, lParam);
		}

		HMENU hmenu = LoadMenu(g_hInstance, MAKEINTRESOURCE(IDR_POPUPMENU));
		//SetMenuDefaultItem(GetSubMenu(hmenu, IDP_POPUP_SUBMENU_BAR), 0, FALSE);

		POINT pt;
		pt.x = GET_X_LPARAM(lParam);
		pt.y = GET_Y_LPARAM(lParam);

		int imenu = 0;
		switch (nID) {
		case IDC_EDIT: {
			const int iSelStart = (int)SendMessage(hwndEdit, SCI_GETSELECTIONSTART, 0, 0);
			const int iSelEnd = (int)SendMessage(hwndEdit, SCI_GETSELECTIONEND, 0, 0);

			if (iSelStart == iSelEnd && pt.x != -1 && pt.y != -1) {
				POINT ptc = { pt.x, pt.y };
				ScreenToClient(hwndEdit, &ptc);
				const int iNewPos = (int)SendMessage(hwndEdit, SCI_POSITIONFROMPOINT, ptc.x, ptc.y);
				SendMessage(hwndEdit, SCI_GOTOPOS, iNewPos, 0);
			}

			if (pt.x == -1 && pt.y == -1) {
				const int iCurrentPos = (int)SendMessage(hwndEdit, SCI_GETCURRENTPOS, 0, 0);
				pt.x = (int)SendMessage(hwndEdit, SCI_POINTXFROMPOSITION, 0, iCurrentPos);
				pt.y = (int)SendMessage(hwndEdit, SCI_POINTYFROMPOSITION, 0, iCurrentPos);
				ClientToScreen(hwndEdit, &pt);
			}
			imenu = IDP_POPUP_SUBMENU_EDIT;
		}
		break;

		case IDC_TOOLBAR:
		case IDC_STATUSBAR:
		case IDC_REBAR:
			if (pt.x == -1 && pt.y == -1) {
				GetCursorPos(&pt);
			}
			imenu = IDP_POPUP_SUBMENU_BAR;
			break;
		}

		TrackPopupMenuEx(GetSubMenu(hmenu, imenu),
						 TPM_LEFTBUTTON | TPM_RIGHTBUTTON, pt.x + 1, pt.y + 1, hwnd, NULL);

		DestroyMenu(hmenu);
	}
	break;

	case WM_INITMENU:
		MsgInitMenu(hwnd, wParam, lParam);
		break;

	case WM_NOTIFY:
		return MsgNotify(hwnd, wParam, lParam);

	case WM_COMMAND:
		return MsgCommand(hwnd, wParam, lParam);

	case WM_SYSCOMMAND:
		switch (wParam) {
		case SC_MINIMIZE:
			ShowOwnedPopups(hwnd, FALSE);
			if (bMinimizeToTray) {
				NP2MinimizeWind(hwnd);
				return 0;
			}
			return DefWindowProc(hwnd, umsg, wParam, lParam);

		case SC_RESTORE: {
			const LRESULT lrv = DefWindowProc(hwnd, umsg, wParam, lParam);
			ShowOwnedPopups(hwnd, TRUE);
			return lrv;
		}
		}
		return DefWindowProc(hwnd, umsg, wParam, lParam);

	case APPM_CHANGENOTIFY:
		if (iFileWatchingMode == 1 || IsDocumentModified()) {
			SetForegroundWindow(hwnd);
		}

		if (PathFileExists(szCurFile)) {
			if ((iFileWatchingMode == 2 && !IsDocumentModified()) || MsgBox(MBYESNO, IDS_FILECHANGENOTIFY) == IDYES) {
				const int iCurPos = (int)SendMessage(hwndEdit, SCI_GETCURRENTPOS, 0, 0);
				const int iAnchorPos = (int)SendMessage(hwndEdit, SCI_GETANCHOR, 0, 0);
#if NP2_ENABLE_DOT_LOG_FEATURE
				const int iVisTopLine = (int)SendMessage(hwndEdit, SCI_GETFIRSTVISIBLELINE, 0, 0);
				const int iDocTopLine = (int)SendMessage(hwndEdit, SCI_DOCLINEFROMVISIBLE, iVisTopLine, 0);
				const int iXOffset = (int)SendMessage(hwndEdit, SCI_GETXOFFSET, 0, 0);
#endif
				const BOOL bIsTail = (iCurPos == iAnchorPos) && (iCurPos == SendMessage(hwndEdit, SCI_GETLENGTH, 0, 0));

				iWeakSrcEncoding = iEncoding;
				if (FileLoad(TRUE, FALSE, TRUE, FALSE, szCurFile)) {

					if (bIsTail && iFileWatchingMode == 2) {
						EditJumpTo(hwndEdit, -1, 0);
						EditEnsureSelectionVisible(hwndEdit);
					}
#if NP2_ENABLE_DOT_LOG_FEATURE
					 else if (SendMessage(hwndEdit, SCI_GETLENGTH, 0, 0) >= 4) {
						char tch[5] = "";
						SendMessage(hwndEdit, SCI_GETTEXT, 5, (LPARAM)tch);
						if (StrEqual(tch, ".LOG")) {
							int iNewTopLine;
							SendMessage(hwndEdit, SCI_SETSEL, iAnchorPos, iCurPos);
							SendMessage(hwndEdit, SCI_ENSUREVISIBLE, iDocTopLine, 0);
							iNewTopLine = (int)SendMessage(hwndEdit, SCI_GETFIRSTVISIBLELINE, 0, 0);
							SendMessage(hwndEdit, SCI_LINESCROLL, 0, iVisTopLine - iNewTopLine);
							SendMessage(hwndEdit, SCI_SETXOFFSET, iXOffset, 0);
						}
					}
#endif
				}
			}
		} else {
			if (MsgBox(MBYESNO, IDS_FILECHANGENOTIFY2) == IDYES) {
				FileSave(TRUE, FALSE, FALSE, FALSE);
			}
		}

		if (!bRunningWatch) {
			InstallFileWatching(szCurFile);
		}
		break;

	//// This message is posted before Notepad2 reactivates itself
	//case APPM_CHANGENOTIFYCLEAR:
	//	bPendingChangeNotify = FALSE;
	//	break;

	case WM_DRAWCLIPBOARD:
		if (!bLastCopyFromMe) {
			dwLastCopyTime = GetTickCount();
		} else {
			bLastCopyFromMe = FALSE;
		}
		if (hwndNextCBChain) {
			SendMessage(hwndNextCBChain, WM_DRAWCLIPBOARD, wParam, lParam);
		}
		break;

	case WM_CHANGECBCHAIN:
		if ((HWND)wParam == hwndNextCBChain) {
			hwndNextCBChain = (HWND)lParam;
		}
		if (hwndNextCBChain) {
			SendMessage(hwndNextCBChain, WM_CHANGECBCHAIN, lParam, wParam);
		}
		break;

	case APPM_TRAYMESSAGE:
		switch (lParam) {
		case WM_RBUTTONUP: {
			HMENU hMenu = LoadMenu(g_hInstance, MAKEINTRESOURCE(IDR_POPUPMENU));
			HMENU hMenuPopup = GetSubMenu(hMenu, IDP_POPUP_SUBMENU_TRAY);

			SetForegroundWindow(hwnd);

			POINT pt;
			GetCursorPos(&pt);
			SetMenuDefaultItem(hMenuPopup, IDM_TRAY_RESTORE, FALSE);
			const int iCmd = TrackPopupMenu(hMenuPopup,
								  TPM_NONOTIFY | TPM_RETURNCMD | TPM_LEFTBUTTON | TPM_RIGHTBUTTON,
								  pt.x, pt.y, 0, hwnd, NULL);

			PostMessage(hwnd, WM_NULL, 0, 0);
			DestroyMenu(hMenu);

			if (iCmd == IDM_TRAY_RESTORE) {
				NP2RestoreWind(hwnd);
			} else if (iCmd == IDM_TRAY_EXIT) {
				NP2ExitWind(hwnd);
			}
		}
		return TRUE;

		case WM_LBUTTONUP:
			NP2RestoreWind(hwnd);
			return TRUE;
		}
		break;

	case APPM_CENTER_MESSAGE_BOX: {
		HWND box = FindWindow(L"#32770", NULL);
		HWND parent = GetParent(box);
		// MessageBox belongs to us.
		if (parent == (HWND)wParam || parent == hwndMain) {
			CenterDlgInParentEx(box, parent);
			SnapToDefaultButton(box);
		}
	}
	break;

	default:
		if (umsg == msgTaskbarCreated) {
			if (!IsWindowVisible(hwnd)) {
				ShowNotifyIcon(hwnd, TRUE);
			}
			SetNotifyIconTitle(hwnd);
			return 0;
		}

		return DefWindowProc(hwnd, umsg, wParam, lParam);

	}
	return 0;
}

void UpdateWindowTitle(void) {
	SetWindowTitle(hwndMain, uidsAppTitle, fIsElevated, IDS_UNTITLED, szCurFile,
				iPathNameFormat, IsDocumentModified(),
				IDS_READONLY, bReadOnly, szTitleExcerpt);
}

void UpdateSelectionMarginWidth(void) {
	// fixed width to put arrow cursor.
	const int width = bShowSelectionMargin ? (GetSystemMetrics(SM_CXCURSOR) / 2) : 0;
	SciCall_SetMarginWidth(MARGIN_SELECTION, width);
}

void UpdateFoldMarginWidth(void) {
	if (bShowCodeFolding) {
		const int iLineMarginWidthNow = SciCall_GetMarginWidth(MARGIN_FOLD_INDEX);
		const int iLineMarginWidthFit = SciCall_TextWidth(STYLE_LINENUMBER, "+_");

		if (iLineMarginWidthNow != iLineMarginWidthFit) {
			SciCall_SetMarginWidth(MARGIN_FOLD_INDEX, iLineMarginWidthFit);
		}
	} else {
		SciCall_SetMarginWidth(MARGIN_FOLD_INDEX, 0);
	}
}

#if NP2_ENABLE_SHOW_CALL_TIPS
#define SetCallTipsWaitTime() \
	SendMessage(hwndEdit, SCI_SETMOUSEDWELLTIME, bShowCallTips? iCallTipsWaitTime : SC_TIME_FOREVER, 0);
#endif

void SetWrapStartIndent(void) {
	int i = 0;
	switch (iWordWrapIndent) {
	case EditWrapIndentOneCharacter:
		i = 1;
		break;
	case EditWrapIndentTwoCharacter:
		i = 2;
		break;
	case EditWrapIndentOneLevel:
		i = (iIndentWidth) ? 1 * iIndentWidth : 1 * iTabWidth;
		break;
	case EditWrapIndentTwoLevel:
		i = (iIndentWidth) ? 2 * iIndentWidth : 2 * iTabWidth;
		break;
	}
	SendMessage(hwndEdit, SCI_SETWRAPSTARTINDENT, i, 0);
}

void SetWrapIndentMode(void) {
	if (iWordWrapIndent == EditWrapIndentSameAsSubline) {
		SendMessage(hwndEdit, SCI_SETWRAPINDENTMODE, SC_WRAPINDENT_SAME, 0);
	} else if (iWordWrapIndent == EditWrapIndentOneLevelThanSubline) {
		SendMessage(hwndEdit, SCI_SETWRAPINDENTMODE, SC_WRAPINDENT_INDENT, 0);
	} else if (iWordWrapIndent == EditWrapIndentTwoLevelThanSubline) {
		SendMessage(hwndEdit, SCI_SETWRAPINDENTMODE, SC_WRAPINDENT_DEEPINDENT, 0);
	} else {
		SetWrapStartIndent();
		SendMessage(hwndEdit, SCI_SETWRAPINDENTMODE, SC_WRAPINDENT_FIXED, 0);
	}
}

void SetWrapVisualFlags(void) {
	if (bShowWordWrapSymbols) {
		int wrapVisualFlags = 0;
		int wrapVisualFlagsLocation = 0;
		if (iWordWrapSymbols == 0) {
			iWordWrapSymbols = EditWrapSymbolDefaultValue;
		}
		switch (iWordWrapSymbols % 10) {
		case EditWrapSymbolBeforeNearText:
			wrapVisualFlags |= SC_WRAPVISUALFLAG_END;
			wrapVisualFlagsLocation |= SC_WRAPVISUALFLAGLOC_END_BY_TEXT;
			break;
		case EditWrapSymbolBeforeNearBorder:
			wrapVisualFlags |= SC_WRAPVISUALFLAG_END;
			break;
		}
		switch (iWordWrapSymbols / 10) {
		case EditWrapSymbolAfterNearText:
			wrapVisualFlags |= SC_WRAPVISUALFLAG_START;
			wrapVisualFlagsLocation |= SC_WRAPVISUALFLAGLOC_START_BY_TEXT;
			break;
		case EditWrapSymbolAfterNearBorder:
			wrapVisualFlags |= SC_WRAPVISUALFLAG_START;
			break;
		case EditWrapSymbolLineNumberMargin:
			wrapVisualFlags |= SC_WRAPVISUALFLAG_MARGIN;
			if (!bShowLineNumbers) {
				bShowLineNumbers = TRUE;
				UpdateLineNumberWidth();
			}
			break;
		}
		SendMessage(hwndEdit, SCI_SETWRAPVISUALFLAGSLOCATION, wrapVisualFlagsLocation, 0);
		SendMessage(hwndEdit, SCI_SETWRAPVISUALFLAGS, wrapVisualFlags, 0);
	} else {
		SendMessage(hwndEdit, SCI_SETWRAPVISUALFLAGS, 0, 0);
	}
}

static void EditFrameOnThemeChanged() {
	if (IsAppThemed()) {
		SetWindowLongPtr(hwndEdit, GWL_EXSTYLE, GetWindowLongPtr(hwndEdit, GWL_EXSTYLE) & ~WS_EX_CLIENTEDGE);
		SetWindowPos(hwndEdit, NULL, 0, 0, 0, 0, SWP_NOZORDER | SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE);

		if (IsVistaAndAbove()) {
			cxEditFrame = 0;
			cyEditFrame = 0;
		} else {
			RECT rc;
			RECT rc2;
			SetWindowPos(hwndEditFrame, NULL, 0, 0, 0, 0, SWP_NOZORDER | SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED);
			GetClientRect(hwndEditFrame, &rc);
			GetWindowRect(hwndEditFrame, &rc2);

			cxEditFrame = ((rc2.right - rc2.left) - (rc.right - rc.left)) / 2;
			cyEditFrame = ((rc2.bottom - rc2.top) - (rc.bottom - rc.top)) / 2;
		}
	} else {
		SetWindowLongPtr(hwndEdit, GWL_EXSTYLE, WS_EX_CLIENTEDGE | GetWindowLongPtr(hwndEdit, GWL_EXSTYLE));
		SetWindowPos(hwndEdit, NULL, 0, 0, 0, 0, SWP_NOZORDER | SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED);

		cxEditFrame = 0;
		cyEditFrame = 0;
	}
}

//=============================================================================
//
// MsgCreate() - Handles WM_CREATE
//
//
LRESULT MsgCreate(HWND hwnd, WPARAM wParam, LPARAM lParam) {
	UNREFERENCED_PARAMETER(wParam);

	HINSTANCE hInstance = ((LPCREATESTRUCT)lParam)->hInstance;
	g_uCurrentDPI = GetCurrentDPI(hwnd);
	g_uDefaultDPI = GetDefaultDPI(hwnd);

	// Setup edit control
	hwndEdit = EditCreate(hwnd);
	//SciInitThemes(hwndEdit);
	if (bEditLayoutRTL) {
		SetWindowLayoutRTL(hwndEdit, TRUE);
	}

	iRenderingTechnology = (int)SendMessage(hwndEdit, SCI_GETTECHNOLOGY, 0, 0);
	iBidirectional = (int)SendMessage(hwndEdit, SCI_GETBIDIRECTIONAL, 0, 0);

	SendMessage(hwndEdit, SCI_SETZOOM, iZoomLevel, 0);
	// Tabs
	SendMessage(hwndEdit, SCI_SETUSETABS, !bTabsAsSpaces, 0);
	SendMessage(hwndEdit, SCI_SETTABINDENTS, bTabIndents, 0);
	SendMessage(hwndEdit, SCI_SETBACKSPACEUNINDENTS, bBackspaceUnindents, 0);
	SendMessage(hwndEdit, SCI_SETTABWIDTH, iTabWidth, 0);
	SendMessage(hwndEdit, SCI_SETINDENT, iIndentWidth, 0);

	// Indent Guides
	Style_SetIndentGuides(hwndEdit, bShowIndentGuides);

	// Word wrap
	SendMessage(hwndEdit, SCI_SETWRAPMODE, (fWordWrap? iWordWrapMode : SC_WRAP_NONE), 0);
	SetWrapIndentMode();
	SetWrapVisualFlags();

	if (bShowUnicodeControlCharacter) {
		EditShowUnicodeControlCharacter(hwndEdit, TRUE);
	}

	// current line
	SendMessage(hwndEdit, SCI_SETCARETLINEVISIBLEALWAYS, TRUE, 0);

	// Long Lines
	if (bMarkLongLines) {
		SendMessage(hwndEdit, SCI_SETEDGEMODE, (iLongLineMode == EDGE_LINE) ? EDGE_LINE : EDGE_BACKGROUND, 0);
	} else {
		SendMessage(hwndEdit, SCI_SETEDGEMODE, EDGE_NONE, 0);
	}
	SendMessage(hwndEdit, SCI_SETEDGECOLUMN, iLongLinesLimit, 0);

	// Margins
	UpdateSelectionMarginWidth();

	// Margins
	SciCall_SetMarginType(MARGIN_FOLD_INDEX, SC_MARGIN_SYMBOL);
	SciCall_SetMarginMask(MARGIN_FOLD_INDEX, SC_MASK_FOLDERS);
	SciCall_SetMarginSensitive(MARGIN_FOLD_INDEX, TRUE);
	// only select sub line of wrapped line
	SendMessage(hwndEdit, SCI_SETMARGINOPTIONS, (bWordWrapSelectSubLine ? SC_MARGINOPTION_SUBLINESELECT : SC_MARGINOPTION_NONE), 0);
	// Code folding, Box tree
	SciCall_MarkerDefine(SC_MARKNUM_FOLDEROPEN, SC_MARK_BOXMINUS);
	SciCall_MarkerDefine(SC_MARKNUM_FOLDER, SC_MARK_BOXPLUS);
	SciCall_MarkerDefine(SC_MARKNUM_FOLDERSUB, SC_MARK_VLINE);
	SciCall_MarkerDefine(SC_MARKNUM_FOLDERTAIL, SC_MARK_LCORNER);
	SciCall_MarkerDefine(SC_MARKNUM_FOLDEREND, SC_MARK_BOXPLUSCONNECTED);
	SciCall_MarkerDefine(SC_MARKNUM_FOLDEROPENMID, SC_MARK_BOXMINUSCONNECTED);
	SciCall_MarkerDefine(SC_MARKNUM_FOLDERMIDTAIL, SC_MARK_TCORNER);
#if !NP2_DEBUG_FOLD_LEVEL
	// Draw below if not expanded
	SciCall_SetFoldFlags(SC_FOLDFLAG_LINEAFTER_CONTRACTED);
#else
	SciCall_SetFoldFlags(SC_FOLDFLAG_LEVELNUMBERS);
#endif
	// highlight for current folding block
	SciCall_MarkerEnableHighlight(TRUE);
#if NP2_ENABLE_SHOW_CALL_TIPS
	// CallTips
	SetCallTipsWaitTime();
#endif

	// Nonprinting characters
	SendMessage(hwndEdit, SCI_SETVIEWWS, (bViewWhiteSpace) ? SCWS_VISIBLEALWAYS : SCWS_INVISIBLE, 0);
	SendMessage(hwndEdit, SCI_SETVIEWEOL, bViewEOLs, 0);

	hwndEditFrame = CreateWindowEx(
						WS_EX_CLIENTEDGE,
						WC_LISTVIEW,
						NULL,
						WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
						0, 0, 100, 100,
						hwnd,
						(HMENU)IDC_EDITFRAME,
						hInstance,
						NULL);

	EditFrameOnThemeChanged();

	// Create Toolbar and Statusbar
	CreateBars(hwnd, hInstance);

	// Window Initialization

	(void)CreateWindow(
		WC_STATIC,
		NULL,
		WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
		0, 0, 10, 10,
		hwnd,
		(HMENU)IDC_FILENAME,
		hInstance,
		NULL);

	SetDlgItemText(hwnd, IDC_FILENAME, szCurFile);

	(void)CreateWindow(
		WC_STATIC,
		NULL,
		WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
		10, 10, 10, 10,
		hwnd,
		(HMENU)IDC_REUSELOCK,
		hInstance,
		NULL);

	SetDlgItemInt(hwnd, IDC_REUSELOCK, GetTickCount(), FALSE);

	// Menu
	//SetMenuDefaultItem(GetSubMenu(GetMenu(hwnd), 0), 0);

	// Drag & Drop
	DragAcceptFiles(hwnd, TRUE);

	// File MRU
	pFileMRU = MRU_Create(MRU_KEY_RECENT_FILES, MRU_NOCASE, MRU_MAX_RECENT_FILES);
	MRU_Load(pFileMRU);

	mruFind = MRU_Create(MRU_KEY_RECENT_FIND, MRU_UTF8, MRU_MAX_RECENT_FIND);
	MRU_Load(mruFind);

	mruReplace = MRU_Create(MRU_KEY_RECENT_REPLACE, MRU_UTF8, MRU_MAX_RECENT_REPLACE);
	MRU_Load(mruReplace);

	if (bInFullScreenMode) {
		bInFullScreenMode = FALSE;
		PostWMCommand(hwnd, IDM_VIEW_TOGGLE_FULLSCREEN);
	}
	return 0;
}

//=============================================================================
//
// CreateBars() - Create Toolbar and Statusbar
//
//
void CreateBars(HWND hwnd, HINSTANCE hInstance) {
	const BOOL bIsAppThemed = IsAppThemed();

	const DWORD dwToolbarStyle = WS_TOOLBAR;
	hwndToolbar = CreateWindowEx(0, TOOLBARCLASSNAME, NULL, dwToolbarStyle,
								 0, 0, 0, 0, hwnd, (HMENU)IDC_TOOLBAR, hInstance, NULL);

	SendMessage(hwndToolbar, TB_BUTTONSTRUCTSIZE, (WPARAM)sizeof(TBBUTTON), 0);

	BOOL bExternalBitmap = FALSE;
	// Add normal Toolbar Bitmap
	HBITMAP hbmp = NULL;
	if (tchToolbarBitmap != NULL) {
		hbmp = LoadBitmapFile(tchToolbarBitmap);
	}
	if (hbmp != NULL) {
		bExternalBitmap = TRUE;
	} else {
		hbmp = (HBITMAP)LoadImage(hInstance, MAKEINTRESOURCE(IDR_MAINWND), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
	}
	hbmp = ResizeImageForCurrentDPI(hbmp);
	HBITMAP hbmpCopy = NULL;
	if (!bExternalBitmap) {
		hbmpCopy = (HBITMAP)CopyImage(hbmp, IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
	}
	BITMAP bmp;
	GetObject(hbmp, sizeof(BITMAP), &bmp);

	HIMAGELIST himl = ImageList_Create(bmp.bmWidth / NUMTOOLBITMAPS, bmp.bmHeight, ILC_COLOR32 | ILC_MASK, 0, 0);
	ImageList_AddMasked(himl, hbmp, CLR_DEFAULT);
	DeleteObject(hbmp);
	SendMessage(hwndToolbar, TB_SETIMAGELIST, 0, (LPARAM)himl);

	// Optionally add hot Toolbar Bitmap
	if (tchToolbarBitmapHot != NULL) {
		hbmp = LoadBitmapFile(tchToolbarBitmapHot);
		if (hbmp != NULL) {
			hbmp = ResizeImageForCurrentDPI(hbmp);
			GetObject(hbmp, sizeof(BITMAP), &bmp);
			himl = ImageList_Create(bmp.bmWidth / NUMTOOLBITMAPS, bmp.bmHeight, ILC_COLOR32 | ILC_MASK, 0, 0);
			ImageList_AddMasked(himl, hbmp, CLR_DEFAULT);
			DeleteObject(hbmp);
			SendMessage(hwndToolbar, TB_SETHOTIMAGELIST, 0, (LPARAM)himl);
		}
	}

	// Optionally add disabled Toolbar Bitmap
	if (tchToolbarBitmapDisabled != NULL) {
		hbmp = LoadBitmapFile(tchToolbarBitmapDisabled);
		if (hbmp != NULL) {
			hbmp = ResizeImageForCurrentDPI(hbmp);
			GetObject(hbmp, sizeof(BITMAP), &bmp);
			himl = ImageList_Create(bmp.bmWidth / NUMTOOLBITMAPS, bmp.bmHeight, ILC_COLOR32 | ILC_MASK, 0, 0);
			ImageList_AddMasked(himl, hbmp, CLR_DEFAULT);
			DeleteObject(hbmp);
			SendMessage(hwndToolbar, TB_SETDISABLEDIMAGELIST, 0, (LPARAM)himl);
			bExternalBitmap = TRUE;
		}
	}

	if (!bExternalBitmap) {
		BOOL fProcessed = FALSE;
		if (flagToolbarLook == 1) {
			fProcessed = BitmapAlphaBlend(hbmpCopy, GetSysColor(COLOR_3DFACE), 0x60);
		} else if (flagToolbarLook == 2) {
			fProcessed = BitmapGrayScale(hbmpCopy);
		}
		if (fProcessed) {
			himl = ImageList_Create(bmp.bmWidth / NUMTOOLBITMAPS, bmp.bmHeight, ILC_COLOR32 | ILC_MASK, 0, 0);
			ImageList_AddMasked(himl, hbmpCopy, CLR_DEFAULT);
			SendMessage(hwndToolbar, TB_SETDISABLEDIMAGELIST, 0, (LPARAM)himl);
		}
	}
	if (hbmpCopy) {
		DeleteObject(hbmpCopy);
	}

#if NP2_ENABLE_CUSTOMIZE_TOOLBAR_LABELS
	// Load toolbar labels
	IniSection section;
	WCHAR *pIniSectionBuf = NP2HeapAlloc(sizeof(WCHAR) * MAX_INI_SECTION_size_tOOLBAR_LABELS);
	const int cchIniSection = (int)(NP2HeapSize(pIniSectionBuf) / sizeof(WCHAR));
	IniSection *pIniSection = &section;

	IniSectionInit(pIniSection, COUNTOF(tbbMainWnd));
	LoadIniSection(INI_SECTION_NAME_TOOLBAR_LABELS, pIniSectionBuf, cchIniSection);
	IniSectionParseArray(pIniSection, pIniSectionBuf);
	const int count = pIniSection->count;

	for (int i = 0; i < count; i++) {
		const IniKeyValueNode *node = &pIniSection->nodeList[i];
		const UINT n = (UINT)wcstol(node->key, NULL, 10);
		if (n == 0 || n >= COUNTOF(tbbMainWnd)) {
			continue;
		}

		LPCWSTR tchDesc = node->value;
		if (StrNotEmpty(tchDesc)) {
			tbbMainWnd[n].iString = SendMessage(hwndToolbar, TB_ADDSTRING, 0, (LPARAM)tchDesc);
			tbbMainWnd[n].fsStyle |= BTNS_AUTOSIZE | BTNS_SHOWTEXT;
		} else {
			tbbMainWnd[n].fsStyle &= ~(BTNS_AUTOSIZE | BTNS_SHOWTEXT);
		}
	}

	IniSectionFree(pIniSection);
	NP2HeapFree(pIniSectionBuf);
#endif

	SendMessage(hwndToolbar, TB_SETEXTENDEDSTYLE, 0,
				SendMessage(hwndToolbar, TB_GETEXTENDEDSTYLE, 0, 0) | TBSTYLE_EX_MIXEDBUTTONS);

	if (Toolbar_SetButtons(hwndToolbar, tchToolbarButtons, tbbMainWnd, COUNTOF(tbbMainWnd)) == 0) {
		Toolbar_SetButtons(hwndToolbar, DefaultToolbarButtons, tbbMainWnd, COUNTOF(tbbMainWnd));
	}

	RECT rc;
	SendMessage(hwndToolbar, TB_GETITEMRECT, 0, (LPARAM)&rc);
	//SendMessage(hwndToolbar, TB_SETINDENT, 2, 0);

	GetString(IDS_DOCPOS, cachedStatusItem.tchDocPosFmt, COUNTOF(cachedStatusItem.tchDocPosFmt));
	const DWORD dwStatusbarStyle = bShowStatusbar ? (WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE) : (WS_CHILD | WS_CLIPSIBLINGS);
	hwndStatus = CreateStatusWindow(dwStatusbarStyle, NULL, hwnd, IDC_STATUSBAR);

	// Create ReBar and add Toolbar
	const DWORD dwReBarStyle = bShowToolbar ? (WS_REBAR | WS_VISIBLE) : WS_REBAR;
	hwndReBar = CreateWindowEx(WS_EX_TOOLWINDOW, REBARCLASSNAME, NULL, dwReBarStyle,
							   0, 0, 0, 0, hwnd, (HMENU)IDC_REBAR, hInstance, NULL);

	REBARINFO rbi;
	rbi.cbSize = sizeof(REBARINFO);
	rbi.fMask = 0;
	rbi.himl = (HIMAGELIST)NULL;
	SendMessage(hwndReBar, RB_SETBARINFO, 0, (LPARAM)&rbi);

	REBARBANDINFO rbBand;
	rbBand.cbSize = sizeof(REBARBANDINFO);
	rbBand.fMask = /*RBBIM_COLORS | RBBIM_TEXT | RBBIM_BACKGROUND | */
		RBBIM_STYLE | RBBIM_CHILD | RBBIM_CHILDSIZE /*| RBBIM_SIZE*/;
	rbBand.fStyle	 = /*RBBS_CHILDEDGE |*//* RBBS_BREAK |*/ RBBS_FIXEDSIZE /*| RBBS_GRIPPERALWAYS*/;
	if (bIsAppThemed) {
		rbBand.fStyle |= RBBS_CHILDEDGE;
	}
	rbBand.hbmBack = NULL;
	rbBand.lpText = (LPWSTR)L"Toolbar";
	rbBand.hwndChild = hwndToolbar;
	rbBand.cxMinChild = (rc.right - rc.left) * COUNTOF(tbbMainWnd);
	rbBand.cyMinChild = (rc.bottom - rc.top) + 2 * rc.top;
	rbBand.cx		= 0;
	SendMessage(hwndReBar, RB_INSERTBAND, (WPARAM)(-1), (LPARAM)&rbBand);

	SetWindowPos(hwndReBar, NULL, 0, 0, 0, 0, SWP_NOZORDER);
	GetWindowRect(hwndReBar, &rc);
	cyReBar = rc.bottom - rc.top;

	cyReBarFrame = bIsAppThemed ? 0 : 2;
}

//=============================================================================
//
// MsgDPIChanged() - Handle WM_DPICHANGED
//
//
void MsgDPIChanged(HWND hwnd, WPARAM wParam, LPARAM lParam) {
	g_uCurrentDPI = HIWORD(wParam);
	const RECT* const rc = (RECT *)lParam;
	const Sci_Position pos = SciCall_GetCurrentPos();
#if 0
	char buf[128];
	sprintf(buf, "WM_DPICHANGED: dpi=%u, %u\n", g_uCurrentDPI, g_uDefaultDPI);
	SendMessage(hwndEdit, SCI_INSERTTEXT, 0, (LPARAM)buf);
#endif

	Style_OnDPIChanged(hwndEdit);
	UpdateSelectionMarginWidth();
	SciCall_GotoPos(pos);

	// recreate toolbar and statusbar
	Toolbar_GetButtons(hwndToolbar, TOOLBAR_COMMAND_BASE, tchToolbarButtons, COUNTOF(tchToolbarButtons));

	DestroyWindow(hwndToolbar);
	DestroyWindow(hwndReBar);
	DestroyWindow(hwndStatus);
	CreateBars(hwnd, g_hInstance);
	UpdateToolbar();
	SetWindowPos(hwnd, NULL, rc->left, rc->top, rc->right - rc->left, rc->bottom - rc->top, SWP_NOZORDER | SWP_NOACTIVATE);
	UpdateStatusbar();
}

//=============================================================================
//
// MsgThemeChanged() - Handle WM_THEMECHANGED
//
//
void MsgThemeChanged(HWND hwnd, WPARAM wParam, LPARAM lParam) {
	UNREFERENCED_PARAMETER(wParam);
	UNREFERENCED_PARAMETER(lParam);

	HINSTANCE hInstance = (HINSTANCE)(INT_PTR)GetWindowLongPtr(hwnd, GWLP_HINSTANCE);

	// reinitialize edit frame
	EditFrameOnThemeChanged();

	// recreate toolbar and statusbar
	Toolbar_GetButtons(hwndToolbar, TOOLBAR_COMMAND_BASE, tchToolbarButtons, COUNTOF(tchToolbarButtons));

	DestroyWindow(hwndToolbar);
	DestroyWindow(hwndReBar);
	DestroyWindow(hwndStatus);
	CreateBars(hwnd, hInstance);
	UpdateToolbar();

	RECT rc;
	GetClientRect(hwnd, &rc);
	SendMessage(hwnd, WM_SIZE, SIZE_RESTORED, MAKELPARAM(rc.right, rc.bottom));
	UpdateStatusbar();
}

//=============================================================================
//
// MsgSize() - Handles WM_SIZE
//
//
void MsgSize(HWND hwnd, WPARAM wParam, LPARAM lParam) {
	UNREFERENCED_PARAMETER(hwnd);

	if (wParam == SIZE_MINIMIZED) {
		return;
	}

	const int x = 0;
	int y = 0;

	const int cx = LOWORD(lParam);
	int cy = HIWORD(lParam);

	if (bShowToolbar) {
		/*
		SendMessage(hwndToolbar, WM_SIZE, 0, 0);
		GetWindowRect(hwndToolbar, &rc);
		y = (rc.bottom - rc.top);
		cy -= (rc.bottom - rc.top);
		*/

		//SendMessage(hwndToolbar, TB_GETITEMRECT, 0, (LPARAM)&rc);
		SetWindowPos(hwndReBar, NULL, 0, 0, LOWORD(lParam), cyReBar, SWP_NOZORDER);
		// the ReBar automatically sets the correct height
		// calling SetWindowPos() with the height of one toolbar button
		// causes the control not to temporarily use the whole client area
		// and prevents flickering

		//GetWindowRect(hwndReBar, &rc);
		y = cyReBar + cyReBarFrame;		 // define
		cy -= cyReBar + cyReBarFrame;	 // border
	}

	if (bShowStatusbar) {
		SendMessage(hwndStatus, WM_SIZE, 0, 0);
		RECT rc;
		GetWindowRect(hwndStatus, &rc);
		cy -= (rc.bottom - rc.top);
	}

	HDWP hdwp = BeginDeferWindowPos(2);

	DeferWindowPos(hdwp, hwndEditFrame, NULL, x, y, cx, cy, SWP_NOZORDER | SWP_NOACTIVATE);

	DeferWindowPos(hdwp, hwndEdit, NULL, x + cxEditFrame, y + cyEditFrame,
				   cx - 2 * cxEditFrame, cy - 2 * cyEditFrame, SWP_NOZORDER | SWP_NOACTIVATE);

	EndDeferWindowPos(hdwp);

	// Statusbar width
	UpdateStatusBarWidth();
}

void UpdateStatusBarCache(int item) {
	switch (item) {
	case STATUS_LEXER:
#if NP2_GET_LEXER_STYLE_NAME_FROM_RES
		cachedStatusItem.pszLexerName = Style_GetCurrentLexerDisplayName(cachedStatusItem.tchLexerName, MAX_EDITLEXER_NAME_SIZE);
#else
		cachedStatusItem.pszLexerName = Style_GetCurrentLexerName();
#endif
		cachedStatusItem.updateMask |= STATUS_BAR_UPDATE_MASK_LEXER;
		UpdateStatusBarWidth();
		break;

	case STATUS_CODEPAGE:
		Encoding_GetLabel(iEncoding);
		cachedStatusItem.updateMask |= STATUS_BAR_UPDATE_MASK_CODEPAGE;
		UpdateStatusBarWidth();
		break;

	case STATUS_EOLMODE:
		cachedStatusItem.pszEOLMode = (iEOLMode == SC_EOL_LF) ? L"LF" : ((iEOLMode == SC_EOL_CR) ? L"CR" : L"CR+LF");
		cachedStatusItem.updateMask |= STATUS_BAR_UPDATE_MASK_EOLMODE;
		break;

	case STATUS_DOCZOOM:
		wsprintf(cachedStatusItem.tchZoom, L"%i%%", iZoomLevel);
		cachedStatusItem.updateMask |= STATUS_BAR_UPDATE_MASK_DOCZOOM;
		break;
	}
}

void UpdateStatusBarWidth(void) {
	int aWidth[7];

	RECT rc;
	GetClientRect(hwndMain, &rc);
	const int cx = rc.right - rc.left;
	const int iBytes = SciCall_GetLength();

	aWidth[1] = StatusCalcPaneWidth(hwndStatus, cachedStatusItem.pszLexerName) + 4;
	aWidth[2] = StatusCalcPaneWidth(hwndStatus, mEncoding[iEncoding].wchLabel) + 4;
	aWidth[3] = StatusCalcPaneWidth(hwndStatus, L"CR+LF");
	aWidth[4] = StatusCalcPaneWidth(hwndStatus, L"OVR");
	aWidth[5] = StatusCalcPaneWidth(hwndStatus, ((iBytes < 1024)? L"1,023 Bytes" : L"99.9 MiB"));
	aWidth[6] = StatusCalcPaneWidth(hwndStatus, L"500%") + GetSystemMetricsEx(SM_CXHTHUMB);

	aWidth[0] = max_i(120, cx - (aWidth[1] + aWidth[2] + aWidth[3] + aWidth[4] + aWidth[5] + aWidth[6]));
	aWidth[1] += aWidth[0];
	aWidth[2] += aWidth[1];
	aWidth[3] += aWidth[2];
	aWidth[4] += aWidth[3];
	aWidth[5] += aWidth[4];
	aWidth[6] = -1;

	SendMessage(hwndStatus, SB_SETPARTS, COUNTOF(aWidth), (LPARAM)aWidth);
}

BOOL IsIMEInNativeMode(void) {
	BOOL result = FALSE;
	HIMC himc = ImmGetContext(hwndEdit);
	if (himc) {
		if (ImmGetOpenStatus(himc)) {
			DWORD dwConversion = IME_CMODE_ALPHANUMERIC;
			DWORD dwSentence = IME_SMODE_NONE;
			if (ImmGetConversionStatus(himc, &dwConversion, &dwSentence)) {
				result = dwConversion != IME_CMODE_ALPHANUMERIC;
			}
		}
		ImmReleaseContext(hwndEdit, himc);
	}
	return result;
}

void MsgNotifyZoom(void) {
	iZoomLevel = (int)SendMessage(hwndEdit, SCI_GETZOOM, 0, 0);

	UpdateStatusBarCache(STATUS_DOCZOOM);
	UpdateLineNumberWidth();
	UpdateFoldMarginWidth();
	UpdateStatusbar();
}

//=============================================================================
//
// MsgInitMenu() - Handles WM_INITMENU
//
//
void MsgInitMenu(HWND hwnd, WPARAM wParam, LPARAM lParam) {
	UNREFERENCED_PARAMETER(lParam);

	HMENU hmenu = (HMENU)wParam;

	int i = StrNotEmpty(szCurFile);
	EnableCmd(hmenu, IDM_FILE_REVERT, i);
	EnableCmd(hmenu, CMD_RELOADANSIASUTF8, i);
	EnableCmd(hmenu, CMD_RELOADANSI, i);
	EnableCmd(hmenu, CMD_RELOADOEM, i);
	EnableCmd(hmenu, CMD_RELOADNOFILEVARS, i);
	EnableCmd(hmenu, CMD_RECODEDEFAULT, i);
	EnableCmd(hmenu, IDM_FILE_LAUNCH, i);
	EnableCmd(hmenu, IDM_FILE_PROPERTIES, i);
	EnableCmd(hmenu, IDM_FILE_CREATELINK, i);
	EnableCmd(hmenu, IDM_FILE_ADDTOFAV, i);

	EnableCmd(hmenu, IDM_FILE_RELAUNCH_ELEVATED, IsVistaAndAbove() && !fIsElevated);
	EnableCmd(hmenu, IDM_FILE_OPEN_CONTAINING_FOLDER, i);
	EnableCmd(hmenu, IDM_FILE_READONLY, i);
	CheckCmd(hmenu, IDM_FILE_READONLY, bReadOnly);

	//EnableCmd(hmenu, IDM_ENCODING_UNICODEREV, !bReadOnly);
	//EnableCmd(hmenu, IDM_ENCODING_UNICODE, !bReadOnly);
	//EnableCmd(hmenu, IDM_ENCODING_UTF8SIGN, !bReadOnly);
	//EnableCmd(hmenu, IDM_ENCODING_UTF8, !bReadOnly);
	//EnableCmd(hmenu, IDM_ENCODING_ANSI, !bReadOnly);
	//EnableCmd(hmenu, IDM_LINEENDINGS_CRLF, !bReadOnly);
	//EnableCmd(hmenu, IDM_LINEENDINGS_LF, !bReadOnly);
	//EnableCmd(hmenu, IDM_LINEENDINGS_CR, !bReadOnly);

	EnableCmd(hmenu, IDM_RECODE_SELECT, i);

	if (mEncoding[iEncoding].uFlags & NCP_UNICODE_REVERSE) {
		i = IDM_ENCODING_UNICODEREV;
	} else if (mEncoding[iEncoding].uFlags & NCP_UNICODE) {
		i = IDM_ENCODING_UNICODE;
	} else if (mEncoding[iEncoding].uFlags & NCP_UTF8_SIGN) {
		i = IDM_ENCODING_UTF8SIGN;
	} else if (mEncoding[iEncoding].uFlags & NCP_UTF8) {
		i = IDM_ENCODING_UTF8;
	} else if (mEncoding[iEncoding].uFlags & NCP_DEFAULT) {
		i = IDM_ENCODING_ANSI;
	} else {
		i = -1;
	}
	CheckMenuRadioItem(hmenu, IDM_ENCODING_ANSI, IDM_ENCODING_UTF8SIGN, i, MF_BYCOMMAND);

	if (iEOLMode == SC_EOL_CRLF) {
		i = IDM_LINEENDINGS_CRLF;
	} else if (iEOLMode == SC_EOL_LF) {
		i = IDM_LINEENDINGS_LF;
	} else {
		i = IDM_LINEENDINGS_CR;
	}
	CheckMenuRadioItem(hmenu, IDM_LINEENDINGS_CRLF, IDM_LINEENDINGS_CR, i, MF_BYCOMMAND);

	EnableCmd(hmenu, IDM_FILE_RECENT, (MRU_GetCount(pFileMRU) > 0));

	EnableCmd(hmenu, IDM_EDIT_UNDO, SendMessage(hwndEdit, SCI_CANUNDO, 0, 0) /*&& !bReadOnly*/);
	EnableCmd(hmenu, IDM_EDIT_REDO, SendMessage(hwndEdit, SCI_CANREDO, 0, 0) /*&& !bReadOnly*/);

	i  = !EditIsEmptySelection();
	const int i2 = (int)SendMessage(hwndEdit, SCI_CANPASTE, 0, 0);
	const BOOL nonEmpty = SendMessage(hwndEdit, SCI_GETLENGTH, 0, 0) != 0;

	EnableCmd(hmenu, IDM_EDIT_CUT, i /*&& !bReadOnly*/);
	EnableCmd(hmenu, IDM_EDIT_CUT_BINARY, i /*&& !bReadOnly*/);
	EnableCmd(hmenu, IDM_EDIT_COPY, i /*&& !bReadOnly*/);
	EnableCmd(hmenu, IDM_EDIT_COPY_BINARY, i /*&& !bReadOnly*/);
	EnableCmd(hmenu, IDM_EDIT_COPYALL, nonEmpty /*&& !bReadOnly*/);
	EnableCmd(hmenu, IDM_EDIT_SELECTALL, nonEmpty);
	EnableCmd(hmenu, IDM_EDIT_COPYADD, i /*&& !bReadOnly*/);
	EnableCmd(hmenu, IDM_EDIT_PASTE, i2 /*&& !bReadOnly*/);
	EnableCmd(hmenu, IDM_EDIT_PASTE_BINARY, i2 /*&& !bReadOnly*/);
	EnableCmd(hmenu, IDM_EDIT_SWAP, i || i2 /*&& !bReadOnly*/);
	EnableCmd(hmenu, IDM_EDIT_CLEAR, i /*&& !bReadOnly*/);
	EnableCmd(hmenu, IDM_EDIT_COPYRTF, i /*&& !bReadOnly*/);

	OpenClipboard(hwnd);
	EnableCmd(hmenu, IDM_EDIT_CLEARCLIPBOARD, CountClipboardFormats());
	CloseClipboard();

	EnableCmd(hmenu, IDM_VIEW_FOLD_CURRENT, nonEmpty);
	EnableCmd(hmenu, CMD_OPEN_PATH_OR_LINK, nonEmpty);
	EnableCmd(hmenu, CMD_OPEN_CONTAINING_FOLDER, nonEmpty);
	EnableCmd(hmenu, CMD_ONLINE_SEARCH_GOOGLE, i);
	EnableCmd(hmenu, CMD_ONLINE_SEARCH_BING, i);
	EnableCmd(hmenu, CMD_ONLINE_SEARCH_WIKI, i);
	EnableCmd(hmenu, CMD_CUSTOM_ACTION1, i);
	EnableCmd(hmenu, CMD_CUSTOM_ACTION2, i);

	//EnableCmd(hmenu, IDM_EDIT_MOVELINEUP, !bReadOnly);
	//EnableCmd(hmenu, IDM_EDIT_MOVELINEDOWN, !bReadOnly);
	//EnableCmd(hmenu, IDM_EDIT_LINETRANSPOSE, !bReadOnly);
	//EnableCmd(hmenu, IDM_EDIT_DUPLICATELINE, !bReadOnly);
	//EnableCmd(hmenu, IDM_EDIT_CUTLINE, !bReadOnly);
	//EnableCmd(hmenu, IDM_EDIT_COPYLINE, !bReadOnly);
	//EnableCmd(hmenu, IDM_EDIT_DELETELINE, !bReadOnly);

	//EnableCmd(hmenu, IDM_EDIT_INDENT, !bReadOnly);
	//EnableCmd(hmenu, IDM_EDIT_UNINDENT, !bReadOnly);

	//EnableCmd(hmenu, IDM_EDIT_PADWITHSPACES, !bReadOnly);
	//EnableCmd(hmenu, IDM_EDIT_STRIP1STCHAR, !bReadOnly);
	//EnableCmd(hmenu, IDM_EDIT_STRIPLASTCHAR, !bReadOnly);
	//EnableCmd(hmenu, IDM_EDIT_TRIMLINES, !bReadOnly);
	//EnableCmd(hmenu, IDM_EDIT_TRIMLEAD, !bReadOnly);
	//EnableCmd(hmenu, IDM_EDIT_MERGEBLANKLINES, !bReadOnly);
	//EnableCmd(hmenu, IDM_EDIT_REMOVEBLANKLINES, !bReadOnly);

	EnableCmd(hmenu, IDM_EDIT_SORTLINES,
			  SendMessage(hwndEdit, SCI_LINEFROMPOSITION,
						  (WPARAM)SendMessage(hwndEdit, SCI_GETSELECTIONEND, 0, 0), 0) -
			  SendMessage(hwndEdit, SCI_LINEFROMPOSITION,
						  (WPARAM)SendMessage(hwndEdit, SCI_GETSELECTIONSTART, 0, 0), 0) >= 1);

	EnableCmd(hmenu, IDM_EDIT_COLUMNWRAP, i /*&& IsWindowsNT()*/);
	EnableCmd(hmenu, IDM_EDIT_SPLITLINES, i /*&& !bReadOnly*/);
	EnableCmd(hmenu, IDM_EDIT_JOINLINES, i /*&& !bReadOnly*/);
	EnableCmd(hmenu, IDM_EDIT_JOINLINESEX, i /*&& !bReadOnly*/);

	EnableCmd(hmenu, IDM_EDIT_CONVERTUPPERCASE, i /*&& !bReadOnly*/);
	EnableCmd(hmenu, IDM_EDIT_CONVERTLOWERCASE, i /*&& !bReadOnly*/);
	EnableCmd(hmenu, IDM_EDIT_INVERTCASE, i /*&& !bReadOnly*/ /*&& IsWindowsNT()*/);
	EnableCmd(hmenu, IDM_EDIT_TITLECASE, i /*&& !bReadOnly*/ /*&& IsWindowsNT()*/);
	EnableCmd(hmenu, IDM_EDIT_SENTENCECASE, i /*&& !bReadOnly*/ /*&& IsWindowsNT()*/);

	EnableCmd(hmenu, IDM_EDIT_CONVERTTABS, i /*&& !bReadOnly*/);
	EnableCmd(hmenu, IDM_EDIT_CONVERTSPACES, i /*&& !bReadOnly*/);
	EnableCmd(hmenu, IDM_EDIT_CONVERTTABS2, i /*&& !bReadOnly*/);
	EnableCmd(hmenu, IDM_EDIT_CONVERTSPACES2, i /*&& !bReadOnly*/);

	EnableCmd(hmenu, IDM_EDIT_URLENCODE, i /*&& !bReadOnly*/);
	EnableCmd(hmenu, IDM_EDIT_URLDECODE, i /*&& !bReadOnly*/);

	EnableCmd(hmenu, IDM_EDIT_XHTML_ESCAPE_CHAR, i /*&& !bReadOnly*/);
	EnableCmd(hmenu, IDM_EDIT_XHTML_UNESCAPE_CHAR, i /*&& !bReadOnly*/);

	EnableCmd(hmenu, IDM_EDIT_ESCAPECCHARS, i /*&& !bReadOnly*/);
	EnableCmd(hmenu, IDM_EDIT_UNESCAPECCHARS, i /*&& !bReadOnly*/);

	EnableCmd(hmenu, IDM_EDIT_CHAR2HEX, i /*&& !bReadOnly*/);
	EnableCmd(hmenu, IDM_EDIT_HEX2CHAR, i /*&& !bReadOnly*/);
	EnableCmd(hmenu, IDM_EDIT_SHOW_HEX, i /*&& !bReadOnly*/);

	EnableCmd(hmenu, IDM_EDIT_NUM2HEX, i /*&& !bReadOnly*/);
	EnableCmd(hmenu, IDM_EDIT_NUM2DEC, i /*&& !bReadOnly*/);
	EnableCmd(hmenu, IDM_EDIT_NUM2BIN, i /*&& !bReadOnly*/);
	EnableCmd(hmenu, IDM_EDIT_NUM2OCT, i /*&& !bReadOnly*/);

	//EnableCmd(hmenu, IDM_EDIT_INCREASENUM, i /*&& !bReadOnly*/);
	//EnableCmd(hmenu, IDM_EDIT_DECREASENUM, i /*&& !bReadOnly*/);

	EnableCmd(hmenu, IDM_VIEW_SHOWEXCERPT, i);

	EnableCmd(hmenu, IDM_EDIT_LINECOMMENT, bCurrentLexerHasLineComment);
	EnableCmd(hmenu, IDM_EDIT_STREAMCOMMENT, bCurrentLexerHasBlockComment);

	EnableCmd(hmenu, IDM_EDIT_INSERT_ENCODING, *mEncoding[iEncoding].pszParseNames);

	//EnableCmd(hmenu, IDM_EDIT_INSERT_SHORTDATE, !bReadOnly);
	//EnableCmd(hmenu, IDM_EDIT_INSERT_LONGDATE, !bReadOnly);
	//EnableCmd(hmenu, IDM_EDIT_INSERT_FILENAME, !bReadOnly);
	//EnableCmd(hmenu, IDM_EDIT_INSERT_PATHNAME, !bReadOnly);

	i = nonEmpty;
	EnableCmd(hmenu, IDM_EDIT_FIND, i);
	EnableCmd(hmenu, IDM_EDIT_SAVEFIND, i);
	EnableCmd(hmenu, IDM_EDIT_FINDNEXT, i);
	EnableCmd(hmenu, IDM_EDIT_FINDPREV, i && StrNotEmptyA(efrData.szFind));
	EnableCmd(hmenu, IDM_EDIT_REPLACE, i /*&& !bReadOnly*/);
	EnableCmd(hmenu, IDM_EDIT_REPLACENEXT, i);
	//EnableCmd(hmenu, IDM_EDIT_SELECTWORD, i);
	//EnableCmd(hmenu, IDM_EDIT_SELECTLINE, i);
	EnableCmd(hmenu, IDM_EDIT_SELTOEND, i);
	EnableCmd(hmenu, IDM_EDIT_SELTOBEGIN, i);
	EnableCmd(hmenu, IDM_EDIT_SELTONEXT, i && StrNotEmptyA(efrData.szFind));
	EnableCmd(hmenu, IDM_EDIT_SELTOPREV, i && StrNotEmptyA(efrData.szFind));
	EnableCmd(hmenu, IDM_EDIT_FINDMATCHINGBRACE, i);
	EnableCmd(hmenu, IDM_EDIT_SELTOMATCHINGBRACE, i);
	EnableCmd(hmenu, CMD_JUMP2SELSTART, i);
	EnableCmd(hmenu, CMD_JUMP2SELEND, i);
	EnableCmd(hmenu, BME_EDIT_BOOKMARKPREV, i);
	EnableCmd(hmenu, BME_EDIT_BOOKMARKNEXT, i);
	EnableCmd(hmenu, BME_EDIT_BOOKMARKTOGGLE, i);
	EnableCmd(hmenu, BME_EDIT_BOOKMARKCLEAR, i);
	EnableCmd(hmenu, IDM_EDIT_GOTOLINE, nonEmpty);
	EnableCmd(hmenu, IDM_EDIT_DELETELINELEFT, i);
	EnableCmd(hmenu, IDM_EDIT_DELETELINERIGHT, i);
	EnableCmd(hmenu, CMD_CTRLBACK, i);
	EnableCmd(hmenu, CMD_CTRLDEL, i);
	EnableCmd(hmenu, CMD_TIMESTAMPS, i);
	//EnableCmd(hmenu, IDM_VIEW_TOGGLEFOLDS, i && bShowCodeFolding);
	//EnableCmd(hmenu, IDM_VIEW_FOLD_ALL, i && bShowCodeFolding);
	//EnableCmd(hmenu, IDM_VIEW_FOLD_LEVEL1, i && bShowCodeFolding);
	//EnableCmd(hmenu, IDM_VIEW_FOLD_LEVEL2, i && bShowCodeFolding);
	//EnableCmd(hmenu, IDM_VIEW_FOLD_LEVEL3, i && bShowCodeFolding);
	//EnableCmd(hmenu, IDM_VIEW_FOLD_LEVEL4, i && bShowCodeFolding);
	//EnableCmd(hmenu, IDM_VIEW_FOLD_LEVEL5, i && bShowCodeFolding);
	//EnableCmd(hmenu, IDM_VIEW_FOLD_LEVEL6, i && bShowCodeFolding);
	//EnableCmd(hmenu, IDM_VIEW_FOLD_LEVEL7, i && bShowCodeFolding);
	//EnableCmd(hmenu, IDM_VIEW_FOLD_LEVEL8, i && bShowCodeFolding);
	//EnableCmd(hmenu, IDM_VIEW_FOLD_LEVEL9, i && bShowCodeFolding);
	//EnableCmd(hmenu, IDM_VIEW_FOLD_LEVEL10, i && bShowCodeFolding);
	CheckCmd(hmenu, IDM_VIEW_FOLDING, bShowCodeFolding);

	CheckCmd(hmenu, IDM_VIEW_USE2NDDEFAULT, bUse2ndDefaultStyle);

	CheckCmd(hmenu, IDM_VIEW_WORDWRAP, fWordWrap);
	i = IDM_VIEW_FONTQUALITY_DEFAULT + iFontQuality;
	CheckMenuRadioItem(hmenu, IDM_VIEW_FONTQUALITY_DEFAULT, IDM_VIEW_FONTQUALITY_CLEARTYPE, i, MF_BYCOMMAND);
	CheckCmd(hmenu, IDM_VIEW_CARET_STYLE_BLOCK_OVR, iOvrCaretStyle);
	i = IDM_VIEW_CARET_STYLE_BLOCK + iCaretStyle;
	CheckMenuRadioItem(hmenu, IDM_VIEW_CARET_STYLE_BLOCK, IDM_VIEW_CARET_STYLE_WIDTH3, i, MF_BYCOMMAND);
	CheckCmd(hmenu, IDM_VIEW_CARET_STYLE_NOBLINK, iCaretBlinkPeriod == 0);
	CheckCmd(hmenu, IDM_VIEW_LONGLINEMARKER, bMarkLongLines);
	CheckCmd(hmenu, IDM_VIEW_TABSASSPACES, bTabsAsSpaces);
	CheckCmd(hmenu, IDM_VIEW_SHOWINDENTGUIDES, bShowIndentGuides);
	CheckCmd(hmenu, IDM_VIEW_LINENUMBERS, bShowLineNumbers);
	CheckCmd(hmenu, IDM_VIEW_MARGIN, bShowSelectionMargin);
	EnableCmd(hmenu, IDM_EDIT_COMPLETEWORD, i);

	switch (iMarkOccurrences) {
	case 0:
		i = IDM_VIEW_MARKOCCURRENCES_OFF;
		break;
	case 3:
		i = IDM_VIEW_MARKOCCURRENCES_BLUE;
		break;
	case 2:
		i = IDM_VIEW_MARKOCCURRENCES_GREEN;
		break;
	case 1:
		i = IDM_VIEW_MARKOCCURRENCES_RED;
		break;
	}
	CheckMenuRadioItem(hmenu, IDM_VIEW_MARKOCCURRENCES_OFF, IDM_VIEW_MARKOCCURRENCES_RED, i, MF_BYCOMMAND);
	CheckCmd(hmenu, IDM_VIEW_MARKOCCURRENCES_CASE, bMarkOccurrencesMatchCase);
	CheckCmd(hmenu, IDM_VIEW_MARKOCCURRENCES_WORD, bMarkOccurrencesMatchWords);
	EnableCmd(hmenu, IDM_VIEW_MARKOCCURRENCES_CASE, iMarkOccurrences != 0);
	EnableCmd(hmenu, IDM_VIEW_MARKOCCURRENCES_WORD, iMarkOccurrences != 0);

	CheckCmd(hmenu, IDM_VIEW_SHOWWHITESPACE, bViewWhiteSpace);
	CheckCmd(hmenu, IDM_VIEW_SHOWEOLS, bViewEOLs);
	CheckCmd(hmenu, IDM_VIEW_WORDWRAPSYMBOLS, bShowWordWrapSymbols);
	CheckCmd(hmenu, IDM_VIEW_UNICODE_CONTROL_CHAR, bShowUnicodeControlCharacter);
#if NP2_ENABLE_SHOW_CALL_TIPS
	CheckCmd(hmenu, IDM_VIEW_SHOWCALLTIPS, bShowCallTips);
#endif
	CheckCmd(hmenu, IDM_VIEW_MATCHBRACES, bMatchBraces);
	CheckCmd(hmenu, IDM_VIEW_TOOLBAR, bShowToolbar);
	EnableCmd(hmenu, IDM_VIEW_CUSTOMIZETB, bShowToolbar);
	CheckCmd(hmenu, IDM_VIEW_STATUSBAR, bShowStatusbar);

	CheckCmd(hmenu, IDM_VIEW_FULLSCREEN_ON_START, iFullScreenMode & FullScreenMode_OnStartup);
	CheckCmd(hmenu, IDM_VIEW_FULLSCREEN_HIDE_TITLE, iFullScreenMode & FullScreenMode_HideCaption);
	CheckCmd(hmenu, IDM_VIEW_FULLSCREEN_HIDE_MENU, iFullScreenMode & FullScreenMode_HideMenu);

	i = IDM_VIEW_HIGHLIGHTCURRENTLINE_NONE + iHighlightCurrentLine;
	CheckMenuRadioItem(hmenu, IDM_VIEW_HIGHLIGHTCURRENTLINE_NONE, IDM_VIEW_HIGHLIGHTCURRENTLINE_FRAME, i, MF_BYCOMMAND);
	CheckCmd(hmenu, IDM_VIEW_HIGHLIGHTCURRENTLINE_SUBLINE, bHighlightCurrentSubLine);

	CheckCmd(hmenu, IDM_VIEW_REUSEWINDOW, bReuseWindow);
	CheckCmd(hmenu, IDM_VIEW_SINGLEFILEINSTANCE, bSingleFileInstance);
	CheckCmd(hmenu, IDM_VIEW_STICKYWINPOS, bStickyWinPos);
	CheckCmd(hmenu, IDM_VIEW_ALWAYSONTOP, IsTopMost());
	CheckCmd(hmenu, IDM_VIEW_MINTOTRAY, bMinimizeToTray);
	CheckCmd(hmenu, IDM_VIEW_TRANSPARENT, bTransparentMode);
	EnableCmd(hmenu, IDM_VIEW_TRANSPARENT, i);

	// Rendering Technology
	i = IsVistaAndAbove();
	EnableCmd(hmenu, IDM_SET_RENDER_TECH_D2D, i);
	EnableCmd(hmenu, IDM_SET_RENDER_TECH_D2DRETAIN, i);
	EnableCmd(hmenu, IDM_SET_RENDER_TECH_D2DDC, i);
	i = IDM_SET_RENDER_TECH_GDI + iRenderingTechnology;
	CheckMenuRadioItem(hmenu, IDM_SET_RENDER_TECH_GDI, IDM_SET_RENDER_TECH_D2DDC, i, MF_BYCOMMAND);
	// RTL Layout
	EnableCmd(hmenu, IDM_SET_RTL_LAYOUT_EDIT, iRenderingTechnology == SC_TECHNOLOGY_DEFAULT);
	CheckCmd(hmenu, IDM_SET_RTL_LAYOUT_EDIT, bEditLayoutRTL);
	CheckCmd(hmenu, IDM_SET_RTL_LAYOUT_OTHER, bWindowLayoutRTL);
	// Bidirectional
	i = iRenderingTechnology != SC_TECHNOLOGY_DEFAULT;
	EnableCmd(hmenu, IDM_SET_BIDIRECTIONAL_L2R, i);
	EnableCmd(hmenu, IDM_SET_BIDIRECTIONAL_R2L, i);
	i = IDM_SET_BIDIRECTIONAL_NONE + iBidirectional;
	CheckMenuRadioItem(hmenu, IDM_SET_BIDIRECTIONAL_NONE, IDM_SET_BIDIRECTIONAL_R2L, i, MF_BYCOMMAND);

	CheckCmd(hmenu, IDM_SET_USE_INLINE_IME, bUseInlineIME);
	CheckCmd(hmenu, IDM_SET_USE_BLOCK_CARET, bInlineIMEUseBlockCaret);

	CheckCmd(hmenu, IDM_VIEW_NOSAVERECENT, bSaveRecentFiles);
	CheckCmd(hmenu, IDM_VIEW_NOSAVEFINDREPL, bSaveFindReplace);
	CheckCmd(hmenu, IDM_VIEW_SAVEBEFORERUNNINGTOOLS, bSaveBeforeRunningTools);
	CheckCmd(hmenu, IDM_SET_OPEN_FOLDER_METAPATH, bOpenFolderWithMetapath);

	CheckCmd(hmenu, IDM_VIEW_CHANGENOTIFY, iFileWatchingMode);

	if (StrNotEmpty(szTitleExcerpt)) {
		i = IDM_VIEW_SHOWEXCERPT;
	} else if (iPathNameFormat == 0) {
		i = IDM_VIEW_SHOWFILENAMEONLY;
	} else if (iPathNameFormat == 1) {
		i = IDM_VIEW_SHOWFILENAMEFIRST;
	} else {
		i = IDM_VIEW_SHOWFULLPATH;
	}
	CheckMenuRadioItem(hmenu, IDM_VIEW_SHOWFILENAMEONLY, IDM_VIEW_SHOWEXCERPT, i, MF_BYCOMMAND);

	if (iEscFunction == 1) {
		i = IDM_VIEW_ESCMINIMIZE;
	} else if (iEscFunction == 2) {
		i = IDM_VIEW_ESCEXIT;
	} else {
		i = IDM_VIEW_NOESCFUNC;
	}
	CheckMenuRadioItem(hmenu, IDM_VIEW_NOESCFUNC, IDM_VIEW_ESCEXIT, i, MF_BYCOMMAND);

	i = StrNotEmpty(szIniFile);
	CheckCmd(hmenu, IDM_VIEW_SAVESETTINGS, bSaveSettings && i);
	EnableCmd(hmenu, CMD_OPENINIFILE, i);

	EnableCmd(hmenu, IDM_VIEW_REUSEWINDOW, i);
	EnableCmd(hmenu, IDM_VIEW_STICKYWINPOS, i);
	EnableCmd(hmenu, IDM_VIEW_CLEARWINPOS, i);
	EnableCmd(hmenu, IDM_VIEW_SINGLEFILEINSTANCE, i);
	EnableCmd(hmenu, IDM_VIEW_NOSAVERECENT, i);
	EnableCmd(hmenu, IDM_VIEW_NOSAVEFINDREPL, i);
	EnableCmd(hmenu, IDM_VIEW_SAVESETTINGS, i);

	i = i || StrNotEmpty(szIniFile2);
	EnableCmd(hmenu, IDM_VIEW_SAVESETTINGSNOW, i);

	Style_UpdateSchemeMenu(hmenu);
}

static void ConvertLineEndings(int iNewEOLMode) {
	iEOLMode = iNewEOLMode;
	SendMessage(hwndEdit, SCI_SETEOLMODE, iNewEOLMode, 0);
	SendMessage(hwndEdit, SCI_CONVERTEOLS, iNewEOLMode, 0);
	EditFixPositions(hwndEdit);
	UpdateStatusBarCache(STATUS_EOLMODE);
	UpdateToolbar();
	UpdateStatusbar();
	UpdateWindowTitle();
}

//=============================================================================
//
// MsgCommand() - Handles WM_COMMAND
//
//
LRESULT MsgCommand(HWND hwnd, WPARAM wParam, LPARAM lParam) {
	UNREFERENCED_PARAMETER(lParam);

	switch (LOWORD(wParam)) {
	case IDM_FILE_NEW:
		FileLoad(FALSE, TRUE, FALSE, FALSE, L"");
		break;

	case IDM_FILE_OPEN:
		FileLoad(FALSE, FALSE, FALSE, FALSE, L"");
		break;

	case IDM_FILE_REVERT: {
		if (StrNotEmpty(szCurFile)) {
			if (IsDocumentModified() && MsgBox(MBOKCANCEL, IDS_ASK_REVERT) != IDOK) {
				return 0;
			}

#if NP2_ENABLE_DOT_LOG_FEATURE
			const int iCurPos = (int)SendMessage(hwndEdit, SCI_GETCURRENTPOS, 0, 0);
			const int iAnchorPos = (int)SendMessage(hwndEdit, SCI_GETANCHOR, 0, 0);
			const int iVisTopLine = (int)SendMessage(hwndEdit, SCI_GETFIRSTVISIBLELINE, 0, 0);
			const int iDocTopLine = (int)SendMessage(hwndEdit, SCI_DOCLINEFROMVISIBLE, iVisTopLine, 0);
			const int iXOffset = (int)SendMessage(hwndEdit, SCI_GETXOFFSET, 0, 0);
#endif
			iWeakSrcEncoding = iEncoding;
			if (FileLoad(TRUE, FALSE, TRUE, FALSE, szCurFile)) {
#if NP2_ENABLE_DOT_LOG_FEATURE
				if (SendMessage(hwndEdit, SCI_GETLENGTH, 0, 0) >= 4) {
					char tch[5] = "";
					SendMessage(hwndEdit, SCI_GETTEXT, 5, (LPARAM)tch);
					if (StrEqual(tch, ".LOG")) {
						SendMessage(hwndEdit, SCI_SETSEL, iAnchorPos, iCurPos);
						SendMessage(hwndEdit, SCI_ENSUREVISIBLE, iDocTopLine, 0);
						const int iNewTopLine = (int)SendMessage(hwndEdit, SCI_GETFIRSTVISIBLELINE, 0, 0);
						SendMessage(hwndEdit, SCI_LINESCROLL, 0, iVisTopLine - iNewTopLine);
						SendMessage(hwndEdit, SCI_SETXOFFSET, iXOffset, 0);
					}
				}
#endif
			}
		}
	}
	break;

	case IDM_FILE_SAVE:
		FileSave(TRUE, FALSE, FALSE, FALSE);
		break;

	case IDM_FILE_SAVEAS:
		FileSave(TRUE, FALSE, TRUE, FALSE);
		break;

	case IDM_FILE_SAVECOPY:
		FileSave(TRUE, FALSE, TRUE, TRUE);
		break;

	case IDM_FILE_READONLY:
		//bReadOnly = !bReadOnly;
		//SendMessage(hwndEdit, SCI_SETREADONLY, bReadOnly, 0);
		//UpdateToolbar();
		if (StrNotEmpty(szCurFile)) {
			DWORD dwFileAttributes = GetFileAttributes(szCurFile);
			if (dwFileAttributes != INVALID_FILE_ATTRIBUTES) {
				if (bReadOnly) {
					dwFileAttributes = (dwFileAttributes & ~FILE_ATTRIBUTE_READONLY);
				} else {
					dwFileAttributes |= FILE_ATTRIBUTE_READONLY;
				}
				if (!SetFileAttributes(szCurFile, dwFileAttributes)) {
					MsgBox(MBWARN, IDS_READONLY_MODIFY, szCurFile);
				}
			} else {
				MsgBox(MBWARN, IDS_READONLY_MODIFY, szCurFile);
			}

			dwFileAttributes = GetFileAttributes(szCurFile);
			bReadOnly = (dwFileAttributes != INVALID_FILE_ATTRIBUTES) && (dwFileAttributes & FILE_ATTRIBUTE_READONLY);
			UpdateWindowTitle();
		}
		break;

	case IDM_FILE_BROWSE:
		TryBrowseFile(hwnd, szCurFile, TRUE);
		break;

	case IDM_FILE_NEWWINDOW:
	case IDM_FILE_NEWWINDOW2: {
		const BOOL emptyWind = LOWORD(wParam) == IDM_FILE_NEWWINDOW2;
		if (!emptyWind && bSaveBeforeRunningTools && !FileSave(FALSE, TRUE, FALSE, FALSE)) {
			break;
		}

		WCHAR szModuleName[MAX_PATH];
		GetModuleFileName(NULL, szModuleName, COUNTOF(szModuleName));
		LPWSTR szParameters = (LPWSTR)NP2HeapAlloc(sizeof(WCHAR) * 1024);
		GetRelaunchParameters(szParameters, szCurFile, TRUE, emptyWind);

		SHELLEXECUTEINFO sei;
		ZeroMemory(&sei, sizeof(SHELLEXECUTEINFO));
		sei.cbSize = sizeof(SHELLEXECUTEINFO);
		sei.fMask = SEE_MASK_NOZONECHECKS;
		sei.hwnd = hwnd;
		sei.lpVerb = NULL;
		sei.lpFile = szModuleName;
		sei.lpParameters = szParameters;
		sei.lpDirectory = g_wchWorkingDirectory;
		sei.nShow = SW_SHOWNORMAL;

		ShellExecuteEx(&sei);
		NP2HeapFree(szParameters);
	}
	break;

	case IDM_FILE_RELAUNCH_ELEVATED:
		flagRelaunchElevated = 2;
		if (RelaunchElevated()) {
			DestroyWindow(hwnd);
		}
		break;

	case IDM_FILE_OPEN_CONTAINING_FOLDER:
		OpenContainingFolder(hwnd, szCurFile, TRUE);
		break;

	case IDM_FILE_LAUNCH: {
		if (StrIsEmpty(szCurFile)) {
			break;
		}

		if (bSaveBeforeRunningTools && !FileSave(FALSE, TRUE, FALSE, FALSE)) {
			break;
		}

		WCHAR wchDirectory[MAX_PATH] = L"";
		if (StrNotEmpty(szCurFile)) {
			lstrcpy(wchDirectory, szCurFile);
			PathRemoveFileSpec(wchDirectory);
		}

		SHELLEXECUTEINFO sei;
		ZeroMemory(&sei, sizeof(SHELLEXECUTEINFO));

		sei.cbSize = sizeof(SHELLEXECUTEINFO);
		sei.fMask = 0;
		sei.hwnd = hwnd;
		sei.lpVerb = NULL;
		sei.lpFile = szCurFile;
		sei.lpParameters = NULL;
		sei.lpDirectory = wchDirectory;
		sei.nShow = SW_SHOWNORMAL;

		ShellExecuteEx(&sei);
	}
	break;

	case IDM_FILE_RUN: {
		if (bSaveBeforeRunningTools && !FileSave(FALSE, TRUE, FALSE, FALSE)) {
			break;
		}

		WCHAR tchCmdLine[MAX_PATH + 4];
		lstrcpy(tchCmdLine, szCurFile);
		PathQuoteSpaces(tchCmdLine);

		RunDlg(hwnd, tchCmdLine);
	}
	break;

	case IDM_FILE_OPENWITH:
		if (bSaveBeforeRunningTools && !FileSave(FALSE, TRUE, FALSE, FALSE)) {
			break;
		}
		OpenWithDlg(hwnd, szCurFile);
		break;

	case IDM_FILE_PAGESETUP:
		EditPrintSetup(hwndEdit);
		break;

	case IDM_FILE_PRINT: {
		SHFILEINFO shfi;
		WCHAR *pszTitle;
		WCHAR tchUntitled[32];

		if (StrNotEmpty(szCurFile)) {
			SHGetFileInfo2(szCurFile, 0, &shfi, sizeof(SHFILEINFO), SHGFI_DISPLAYNAME);
			pszTitle = shfi.szDisplayName;
		} else {
			GetString(IDS_UNTITLED, tchUntitled, COUNTOF(tchUntitled));
			pszTitle = tchUntitled;
		}

		WCHAR tchPageFmt[32];
		GetString(IDS_PRINT_PAGENUM, tchPageFmt, COUNTOF(tchPageFmt));

		if (!EditPrint(hwndEdit, pszTitle, tchPageFmt)) {
			MsgBox(MBWARN, IDS_PRINT_ERROR, pszTitle);
		}
	}
	break;

	case IDM_FILE_PROPERTIES: {
		if (StrIsEmpty(szCurFile)) {
			break;
		}

		SHELLEXECUTEINFO sei;
		ZeroMemory(&sei, sizeof(SHELLEXECUTEINFO));

		sei.cbSize = sizeof(SHELLEXECUTEINFO);
		sei.fMask = SEE_MASK_INVOKEIDLIST;
		sei.hwnd = hwnd;
		sei.lpVerb = L"properties";
		sei.lpFile = szCurFile;
		sei.nShow = SW_SHOWNORMAL;

		ShellExecuteEx(&sei);
	}
	break;

	case IDM_FILE_CREATELINK: {
		if (StrIsEmpty(szCurFile)) {
			break;
		}

		if (!PathCreateDeskLnk(szCurFile)) {
			MsgBox(MBWARN, IDS_ERR_CREATELINK);
		}
	}
	break;

	case IDM_FILE_OPENFAV:
		if (FileSave(FALSE, TRUE, FALSE, FALSE)) {
			WCHAR tchSelItem[MAX_PATH];

			if (FavoritesDlg(hwnd, tchSelItem)) {
				if (PathIsLnkToDirectory(tchSelItem, NULL, 0)) {
					PathGetLnkPath(tchSelItem, tchSelItem, COUNTOF(tchSelItem));
				}

				if (PathIsDirectory(tchSelItem)) {
					WCHAR tchFile[MAX_PATH];

					if (OpenFileDlg(hwnd, tchFile, COUNTOF(tchFile), tchSelItem)) {
						FileLoad(TRUE, FALSE, FALSE, FALSE, tchFile);
					}
				} else {
					FileLoad(TRUE, FALSE, FALSE, FALSE, tchSelItem);
				}
			}
		}
		break;

	case IDM_FILE_ADDTOFAV:
		if (StrNotEmpty(szCurFile)) {
			SHFILEINFO shfi;
			SHGetFileInfo2(szCurFile, 0, &shfi, sizeof(SHFILEINFO), SHGFI_DISPLAYNAME);
			AddToFavDlg(hwnd, shfi.szDisplayName, szCurFile);
		}
		break;

	case IDM_FILE_MANAGEFAV: {
		SHELLEXECUTEINFO sei;
		ZeroMemory(&sei, sizeof(SHELLEXECUTEINFO));

		sei.cbSize = sizeof(SHELLEXECUTEINFO);
		sei.fMask = 0;
		sei.hwnd = hwnd;
		sei.lpVerb = L"open";
		sei.lpFile = tchFavoritesDir;
		sei.lpParameters = NULL;
		sei.lpDirectory = NULL;
		sei.nShow = SW_SHOWNORMAL;

		// Run favorites directory
		ShellExecuteEx(&sei);
	}
	break;

	case IDM_FILE_RECENT:
		if (MRU_GetCount(pFileMRU) > 0) {
			if (FileSave(FALSE, TRUE, FALSE, FALSE)) {
				WCHAR tchFile[MAX_PATH];
				if (FileMRUDlg(hwnd, tchFile)) {
					FileLoad(TRUE, FALSE, FALSE, FALSE, tchFile);
				}
			}
		}
		break;

	case IDM_FILE_EXIT:
		NP2ExitWind(hwnd);
		break;

	case IDM_ENCODING_ANSI:
	case IDM_ENCODING_UNICODE:
	case IDM_ENCODING_UNICODEREV:
	case IDM_ENCODING_UTF8:
	case IDM_ENCODING_UTF8SIGN:
	case IDM_ENCODING_SELECT: {
		int iNewEncoding = iEncoding;
		if (LOWORD(wParam) == IDM_ENCODING_SELECT && !SelectEncodingDlg(hwnd, &iNewEncoding)) {
			break;
		}

		switch (LOWORD(wParam)) {
		case IDM_ENCODING_UNICODE:
			iNewEncoding = CPI_UNICODEBOM;
			break;
		case IDM_ENCODING_UNICODEREV:
			iNewEncoding = CPI_UNICODEBEBOM;
			break;
		case IDM_ENCODING_UTF8:
			iNewEncoding = CPI_UTF8;
			break;
		case IDM_ENCODING_UTF8SIGN:
			iNewEncoding = CPI_UTF8SIGN;
			break;
		case IDM_ENCODING_ANSI:
			iNewEncoding = CPI_DEFAULT;
			break;
		}

		if (EditSetNewEncoding(hwndEdit, iEncoding, iNewEncoding, (flagSetEncoding), StrIsEmpty(szCurFile))) {
			if (SendMessage(hwndEdit, SCI_GETLENGTH, 0, 0) == 0) {
				iEncoding = iNewEncoding;
				iOriginalEncoding = iNewEncoding;
			} else {
				if (iEncoding == CPI_DEFAULT || iNewEncoding == CPI_DEFAULT) {
					iOriginalEncoding = -1;
				}
				iEncoding = iNewEncoding;
			}

			UpdateStatusBarCache(STATUS_CODEPAGE);
			UpdateToolbar();
			UpdateStatusbar();
			UpdateWindowTitle();
		}
	}
	break;

	case IDM_RECODE_SELECT: {
		if (StrNotEmpty(szCurFile)) {
			int iNewEncoding;
			switch (iEncoding) {
			case CPI_DEFAULT:
				iNewEncoding = -1;
				break;
			case CPI_UTF8SIGN:
				iNewEncoding = CPI_UTF8;
				break;
			case CPI_UNICODEBOM:
				iNewEncoding = CPI_UNICODE;
				break;
			case CPI_UNICODEBEBOM:
				iNewEncoding = CPI_UNICODEBE;
				break;
			default:
				iNewEncoding = iEncoding;
				break;
			}

			if (IsDocumentModified() && MsgBox(MBOKCANCEL, IDS_ASK_RECODE) != IDOK) {
				return 0;
			}

			if (RecodeDlg(hwnd, &iNewEncoding)) {
				iSrcEncoding = iNewEncoding;
				FileLoad(TRUE, FALSE, TRUE, FALSE, szCurFile);
			}
		}
	}
	break;

	case IDM_ENCODING_SETDEFAULT:
		SelectDefEncodingDlg(hwnd, &iDefaultEncoding);
		break;

	case IDM_LINEENDINGS_CRLF:
	case IDM_LINEENDINGS_LF:
	case IDM_LINEENDINGS_CR: {
		const int iNewEOLMode = iLineEndings[LOWORD(wParam) - IDM_LINEENDINGS_CRLF];
		ConvertLineEndings(iNewEOLMode);
	}
	break;

	case IDM_LINEENDINGS_SETDEFAULT:
		SelectDefLineEndingDlg(hwnd, &iDefaultEOLMode);
		break;

	case IDM_EDIT_UNDO:
		SendMessage(hwndEdit, SCI_UNDO, 0, 0);
		break;

	case IDM_EDIT_REDO:
		SendMessage(hwndEdit, SCI_REDO, 0, 0);
		break;

	case IDM_EDIT_CUT:
	case IDM_EDIT_CUT_BINARY:
		if (flagPasteBoard) {
			bLastCopyFromMe = TRUE;
		}
		SendMessage(hwndEdit, SCI_CUT, (LOWORD(wParam) == IDM_EDIT_CUT_BINARY), 0);
		break;

	case IDM_EDIT_COPY:
	case IDM_EDIT_COPY_BINARY:
		if (flagPasteBoard) {
			bLastCopyFromMe = TRUE;
		}
		SendMessage(hwndEdit, SCI_COPY, (LOWORD(wParam) == IDM_EDIT_COPY_BINARY), 0);
		UpdateToolbar();
		break;

	case IDM_EDIT_COPYALL:
		if (flagPasteBoard) {
			bLastCopyFromMe = TRUE;
		}
		SendMessage(hwndEdit, SCI_COPYRANGE, 0, SendMessage(hwndEdit, SCI_GETLENGTH, 0, 0));
		UpdateToolbar();
		break;

	case IDM_EDIT_COPYADD:
		if (flagPasteBoard) {
			bLastCopyFromMe = TRUE;
		}
		EditCopyAppend(hwndEdit);
		UpdateToolbar();
		break;

	case IDM_EDIT_PASTE:
	case IDM_EDIT_PASTE_BINARY:
		SendMessage(hwndEdit, SCI_PASTE, (LOWORD(wParam) == IDM_EDIT_PASTE_BINARY), 0);
		break;

	case IDM_EDIT_SWAP:
		if (SendMessage(hwndEdit, SCI_GETSELECTIONEND, 0, 0) == SendMessage(hwndEdit, SCI_GETSELECTIONSTART, 0, 0)) {
			const int iPos = (int)SendMessage(hwndEdit, SCI_GETCURRENTPOS, 0, 0);
			SendMessage(hwndEdit, SCI_PASTE, 0, 0);
			const int iNewPos = (int)SendMessage(hwndEdit, SCI_GETCURRENTPOS, 0, 0);
			SendMessage(hwndEdit, SCI_SETSEL, iPos, iNewPos);
			SendWMCommand(hwnd, IDM_EDIT_CLEARCLIPBOARD);
		} else {
			const int iPos = (int)SendMessage(hwndEdit, SCI_GETCURRENTPOS, 0, 0);
			const int iAnchor = (int)SendMessage(hwndEdit, SCI_GETANCHOR, 0, 0);
			char *pClip = EditGetClipboardText(hwndEdit);
			if (flagPasteBoard) {
				bLastCopyFromMe = TRUE;
			}
			SendMessage(hwndEdit, SCI_BEGINUNDOACTION, 0, 0);
			SendMessage(hwndEdit, SCI_CUT, 0, 0);
			SendMessage(hwndEdit, SCI_REPLACESEL, 0, (LPARAM)pClip);
			if (iPos > iAnchor) {
				SendMessage(hwndEdit, SCI_SETSEL, iAnchor, iAnchor + strlen(pClip));
			} else {
				SendMessage(hwndEdit, SCI_SETSEL, iPos + strlen(pClip), iPos);
			}
			SendMessage(hwndEdit, SCI_ENDUNDOACTION, 0, 0);
			LocalFree(pClip);
		}
		break;

	case IDM_EDIT_CLEAR:
		SendMessage(hwndEdit, SCI_CLEAR, 0, 0);
		break;

	case IDM_EDIT_CLEARCLIPBOARD:
		if (OpenClipboard(hwnd)) {
			if (CountClipboardFormats() > 0) {
				EmptyClipboard();
				UpdateToolbar();
				UpdateStatusbar();
			}
			CloseClipboard();
		}
		break;

	case IDM_EDIT_SELECTALL:
		SendMessage(hwndEdit, SCI_SELECTALL, 0, 0);
		//SendMessage(hwndEdit, SCI_SETSEL, 0, -1);
		break;

	case IDM_EDIT_SELECTWORD:
		EditSelectWord(hwndEdit);
		break;

	case IDM_EDIT_SELECTLINE:
		EditSelectLine(hwndEdit);
		break;

	case IDM_EDIT_MOVELINEUP:
		EditMoveUp(hwndEdit);
		break;

	case IDM_EDIT_MOVELINEDOWN:
		EditMoveDown(hwndEdit);
		break;

	case IDM_EDIT_LINETRANSPOSE:
		SendMessage(hwndEdit, SCI_LINETRANSPOSE, 0, 0);
		break;

	case IDM_EDIT_DUPLICATELINE:
		SendMessage(hwndEdit, SCI_LINEDUPLICATE, 0, 0);
		break;

	case IDM_EDIT_CUTLINE:
		if (flagPasteBoard) {
			bLastCopyFromMe = TRUE;
		}
		SendMessage(hwndEdit, SCI_LINECUT, 0, 0);
		break;

	case IDM_EDIT_COPYLINE:
		if (flagPasteBoard) {
			bLastCopyFromMe = TRUE;
		}
		SendMessage(hwndEdit, SCI_LINECOPY, 0, 0);
		UpdateToolbar();
		break;

	case IDM_EDIT_DELETELINE:
		SendMessage(hwndEdit, SCI_LINEDELETE, 0, 0);
		break;

	case IDM_EDIT_DELETELINELEFT:
		SendMessage(hwndEdit, SCI_DELLINELEFT, 0, 0);
		break;

	case IDM_EDIT_DELETELINERIGHT:
		SendMessage(hwndEdit, SCI_DELLINERIGHT, 0, 0);
		break;

	case IDM_EDIT_INDENT: {
		const int iLineSelStart = (int)SendMessage(hwndEdit, SCI_LINEFROMPOSITION, SendMessage(hwndEdit, SCI_GETSELECTIONSTART, 0, 0), 0);
		const int iLineSelEnd = (int)SendMessage(hwndEdit, SCI_LINEFROMPOSITION, SendMessage(hwndEdit, SCI_GETSELECTIONEND, 0, 0), 0);

		SendMessage(hwndEdit, SCI_SETTABINDENTS, TRUE, 0);
		if (iLineSelStart == iLineSelEnd) {
			SendMessage(hwndEdit, SCI_VCHOME, 0, 0);
			SendMessage(hwndEdit, SCI_TAB, 0, 0);
		} else {
			SendMessage(hwndEdit, SCI_TAB, 0, 0);
		}
		SendMessage(hwndEdit, SCI_SETTABINDENTS, bTabIndents, 0);
	}
	break;

	case IDM_EDIT_UNINDENT: {
		const int iLineSelStart	= (int)SendMessage(hwndEdit, SCI_LINEFROMPOSITION, SendMessage(hwndEdit, SCI_GETSELECTIONSTART, 0, 0), 0);
		const int iLineSelEnd = (int)SendMessage(hwndEdit, SCI_LINEFROMPOSITION, SendMessage(hwndEdit, SCI_GETSELECTIONEND, 0, 0), 0);

		SendMessage(hwndEdit, SCI_SETTABINDENTS, TRUE, 0);
		if (iLineSelStart == iLineSelEnd) {
			SendMessage(hwndEdit, SCI_VCHOME, 0, 0);
			SendMessage(hwndEdit, SCI_BACKTAB, 0, 0);
		} else {
			SendMessage(hwndEdit, SCI_BACKTAB, 0, 0);
		}
		SendMessage(hwndEdit, SCI_SETTABINDENTS, bTabIndents, 0);
	}
	break;

	case IDM_EDIT_ENCLOSESELECTION:
		EditEncloseSelectionDlg(hwnd);
		break;

	case IDM_EDIT_SELECTIONDUPLICATE:
		SendMessage(hwndEdit, SCI_BEGINUNDOACTION, 0, 0);
		SendMessage(hwndEdit, SCI_SELECTIONDUPLICATE, 0, 0);
		SendMessage(hwndEdit, SCI_ENDUNDOACTION, 0, 0);
		break;

	case IDM_EDIT_PADWITHSPACES:
		BeginWaitCursor();
		EditPadWithSpaces(hwndEdit, FALSE, FALSE);
		EndWaitCursor();
		break;

	case IDM_EDIT_STRIP1STCHAR:
		BeginWaitCursor();
		EditStripFirstCharacter(hwndEdit);
		EndWaitCursor();
		break;

	case IDM_EDIT_STRIPLASTCHAR:
		BeginWaitCursor();
		EditStripLastCharacter(hwndEdit);
		EndWaitCursor();
		break;

	case IDM_EDIT_TRIMLINES:
		BeginWaitCursor();
		EditStripTrailingBlanks(hwndEdit, FALSE);
		EndWaitCursor();
		break;

	case IDM_EDIT_TRIMLEAD:
		BeginWaitCursor();
		EditStripLeadingBlanks(hwndEdit, FALSE);
		EndWaitCursor();
		break;

	case IDM_EDIT_COMPRESSWS:
		BeginWaitCursor();
		EditCompressSpaces(hwndEdit);
		EndWaitCursor();
		break;

	case IDM_EDIT_MERGEBLANKLINES:
		BeginWaitCursor();
		EditRemoveBlankLines(hwndEdit, TRUE);
		EndWaitCursor();
		break;

	case IDM_EDIT_REMOVEBLANKLINES:
		BeginWaitCursor();
		EditRemoveBlankLines(hwndEdit, FALSE);
		EndWaitCursor();
		break;

	case IDM_EDIT_MODIFYLINES:
		EditModifyLinesDlg(hwnd);
		break;

	case IDM_EDIT_ALIGN:
		if (EditAlignDlg(hwnd, &iAlignMode)) {
			BeginWaitCursor();
			EditAlignText(hwndEdit, iAlignMode);
			EndWaitCursor();
		}
		break;

	case IDM_EDIT_SORTLINES:
		if (EditSortDlg(hwnd, &iSortOptions)) {
			BeginWaitCursor();
			StatusSetText(hwndStatus, STATUS_HELP, L"...");
			StatusSetSimple(hwndStatus, TRUE);
			InvalidateRect(hwndStatus, NULL, TRUE);
			UpdateWindow(hwndStatus);
			EditSortLines(hwndEdit, iSortOptions);
			StatusSetSimple(hwndStatus, FALSE);
			EndWaitCursor();
		}
		break;

	case IDM_EDIT_COLUMNWRAP: {
		if (iWrapCol == 0) {
			iWrapCol = iLongLinesLimit;
		}

		if (ColumnWrapDlg(hwnd, &iWrapCol)) {
			iWrapCol = clamp_i(iWrapCol, 1, 512);
			BeginWaitCursor();
			EditWrapToColumn(hwndEdit, iWrapCol);
			EndWaitCursor();
		}
	}
	break;

	case IDM_EDIT_SPLITLINES:
		BeginWaitCursor();
		SendMessage(hwndEdit, SCI_TARGETFROMSELECTION, 0, 0);
		SendMessage(hwndEdit, SCI_LINESSPLIT, 0, 0);
		EndWaitCursor();
		break;

	case IDM_EDIT_JOINLINES:
		BeginWaitCursor();
		SendMessage(hwndEdit, SCI_TARGETFROMSELECTION, 0, 0);
		SendMessage(hwndEdit, SCI_LINESJOIN, 0, 0);
		EditJoinLinesEx(hwndEdit);
		EndWaitCursor();
		break;

	case IDM_EDIT_JOINLINESEX:
		BeginWaitCursor();
		EditJoinLinesEx(hwndEdit);
		EndWaitCursor();
		break;

	case IDM_EDIT_CONVERTUPPERCASE:
		BeginWaitCursor();
		SendMessage(hwndEdit, SCI_UPPERCASE, 0, 0);
		EndWaitCursor();
		break;

	case IDM_EDIT_CONVERTLOWERCASE:
		BeginWaitCursor();
		SendMessage(hwndEdit, SCI_LOWERCASE, 0, 0);
		EndWaitCursor();
		break;

	case IDM_EDIT_INVERTCASE:
		BeginWaitCursor();
		EditInvertCase(hwndEdit);
		EndWaitCursor();
		break;

	case IDM_EDIT_TITLECASE:
		BeginWaitCursor();
		EditTitleCase(hwndEdit);
		EndWaitCursor();
		break;

	case IDM_EDIT_SENTENCECASE:
		BeginWaitCursor();
		EditSentenceCase(hwndEdit);
		EndWaitCursor();
		break;

	case IDM_EDIT_CONVERTTABS:
		BeginWaitCursor();
		EditTabsToSpaces(hwndEdit, iTabWidth, FALSE);
		EndWaitCursor();
		break;

	case IDM_EDIT_CONVERTSPACES:
		BeginWaitCursor();
		EditSpacesToTabs(hwndEdit, iTabWidth, FALSE);
		EndWaitCursor();
		break;

	case IDM_EDIT_CONVERTTABS2:
		BeginWaitCursor();
		EditTabsToSpaces(hwndEdit, iTabWidth, TRUE);
		EndWaitCursor();
		break;

	case IDM_EDIT_CONVERTSPACES2:
		BeginWaitCursor();
		EditSpacesToTabs(hwndEdit, iTabWidth, TRUE);
		EndWaitCursor();
		break;

	case IDM_EDIT_INSERT_XMLTAG:
		EditInsertTagDlg(hwnd);
		break;

	case IDM_EDIT_INSERT_GUID: {
		GUID guid;
		if (S_OK == CoCreateGuid(&guid)) {
			char guidBuf[37] = {0};
			sprintf(guidBuf, "%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
					(unsigned int)(guid.Data1), guid.Data2, guid.Data3,
					guid.Data4[0], guid.Data4[1],
					guid.Data4[2], guid.Data4[3], guid.Data4[4],
					guid.Data4[5], guid.Data4[6], guid.Data4[7]
				   );
			SendMessage(hwndEdit, SCI_REPLACESEL, 0, (LPARAM)guidBuf);
		}
	}
	break;

	case IDM_INSERT_UNICODE_LRM:
	case IDM_INSERT_UNICODE_RLM:
	case IDM_INSERT_UNICODE_ZWJ:
	case IDM_INSERT_UNICODE_ZWNJ:
	case IDM_INSERT_UNICODE_LRE:
	case IDM_INSERT_UNICODE_RLE:
	case IDM_INSERT_UNICODE_LRO:
	case IDM_INSERT_UNICODE_RLO:
	case IDM_INSERT_UNICODE_PDF:
	case IDM_INSERT_UNICODE_NADS:
	case IDM_INSERT_UNICODE_NODS:
	case IDM_INSERT_UNICODE_ASS:
	case IDM_INSERT_UNICODE_ISS:
	case IDM_INSERT_UNICODE_AAFS:
	case IDM_INSERT_UNICODE_IAFS:
	case IDM_INSERT_UNICODE_RS:
	case IDM_INSERT_UNICODE_US:
	case IDM_INSERT_UNICODE_LS:
	case IDM_INSERT_UNICODE_PS:
		EditInsertUnicodeControlCharacter(hwndEdit, LOWORD(wParam));
		break;

	case IDM_EDIT_INSERT_ENCODING: {
		if (*mEncoding[iEncoding].pszParseNames) {
			char msz[64];
			BOOL done = FALSE;
			lstrcpynA(msz, mEncoding[iEncoding].pszParseNames, COUNTOF(msz));
			char *p;
			if ((p = strchr(msz, ',')) != NULL) {
				*p = '\0';
			}
			if (pLexCurrent->iLexer == SCLEX_PYTHON) {
				const Sci_Position iCurrentPos = SciCall_GetCurrentPos();
				const int iCurLine = SciCall_LineFromPosition(iCurrentPos);
				const Sci_Position iCurrentLinePos = iCurrentPos - SciCall_PositionFromLine(iCurLine);
				if (iCurLine < 2 && iCurrentLinePos == 0) {
					char cmsz[128];
					wsprintfA(cmsz, "#-*- coding: %s -*-", msz);
					SendMessage(hwndEdit, SCI_REPLACESEL, 0, (LPARAM)cmsz);
					done = TRUE;
				}
			}
			if (!done) {
				//int iSelStart;
				//iSelStart = SendMessage(hwndEdit, SCI_GETSELECTIONSTART, 0, 0);
				SendMessage(hwndEdit, SCI_REPLACESEL, 0, (LPARAM)msz);
				//SendMessage(hwndEdit, SCI_SETANCHOR, iSelStart, 0);
			}
		}
	}
	break;

	case IDM_EDIT_INSERT_SHEBANG:
		EditInsertScriptShebangLine(hwndEdit);
		break;

	case IDM_EDIT_INSERT_SHORTDATE:
	case IDM_EDIT_INSERT_LONGDATE: {
		WCHAR tchDateTime[256];
		WCHAR tchTemplate[256];

		SYSTEMTIME st;
		GetLocalTime(&st);

		if (IniGetString(INI_SECTION_NAME_FLAGS,
						 (LOWORD(wParam) == IDM_EDIT_INSERT_SHORTDATE) ? L"DateTimeShort" : L"DateTimeLong",
						 L"", tchTemplate, COUNTOF(tchTemplate))) {
			struct tm sst;
			sst.tm_isdst	= -1;
			sst.tm_sec		= (int)st.wSecond;
			sst.tm_min		= (int)st.wMinute;
			sst.tm_hour		= (int)st.wHour;
			sst.tm_mday		= (int)st.wDay;
			sst.tm_mon		= (int)st.wMonth - 1;
			sst.tm_year		= (int)st.wYear - 1900;
			sst.tm_wday		= (int)st.wDayOfWeek;
			mktime(&sst);
			wcsftime(tchDateTime, COUNTOF(tchDateTime), tchTemplate, &sst);
		} else {
			WCHAR tchDate[128];
			WCHAR tchTime[128];
			GetDateFormat(LOCALE_USER_DEFAULT, (
							  LOWORD(wParam) == IDM_EDIT_INSERT_SHORTDATE) ? DATE_SHORTDATE : DATE_LONGDATE,
						  &st, NULL, tchDate, COUNTOF(tchDate));
			GetTimeFormat(LOCALE_USER_DEFAULT, TIME_NOSECONDS, &st, NULL, tchTime, COUNTOF(tchTime));

			wsprintf(tchDateTime, L"%s %s", tchTime, tchDate);
		}

		const UINT cpEdit = (UINT)SendMessage(hwndEdit, SCI_GETCODEPAGE, 0, 0);
		char mszBuf[256 * kMaxMultiByteCount];
		WideCharToMultiByte(cpEdit, 0, tchDateTime, -1, mszBuf, COUNTOF(mszBuf), NULL, NULL);
		//const int iSelStart = SendMessage(hwndEdit, SCI_GETSELECTIONSTART, 0, 0);
		SendMessage(hwndEdit, SCI_REPLACESEL, 0, (LPARAM)mszBuf);
		//SendMessage(hwndEdit, SCI_SETANCHOR, iSelStart, 0);
	}
	break;

	case IDM_EDIT_INSERT_LOC_DATE:
	case IDM_EDIT_INSERT_LOC_DATETIME: {
		SYSTEMTIME lt;
		char mszBuf[38];
		// Local
		GetLocalTime(&lt);
		if (LOWORD(wParam) == IDM_EDIT_INSERT_LOC_DATE) {
			sprintf(mszBuf, "%04d-%02d-%02d", lt.wYear, lt.wMonth, lt.wDay);
		} else {
			sprintf(mszBuf, "%04d-%02d-%02d %02d:%02d:%02d", lt.wYear, lt.wMonth, lt.wDay, lt.wHour, lt.wMinute, lt.wSecond);
		}
		SendMessage(hwndEdit, SCI_REPLACESEL, 0, (LPARAM)mszBuf);
	}
	break;

	case IDM_EDIT_INSERT_UTC_DATETIME: {
		SYSTEMTIME lt;
		char mszBuf[38];
		// UTC
		GetSystemTime(&lt);
		sprintf(mszBuf, "%04d-%02d-%02dT%02d:%02d:%02dZ", lt.wYear, lt.wMonth, lt.wDay, lt.wHour, lt.wMinute, lt.wSecond);
		SendMessage(hwndEdit, SCI_REPLACESEL, 0, (LPARAM)mszBuf);
	}
	break;

	// https://www.frenk.com/2009/12/convert-filetime-to-unix-timestamp/
	case IDM_EDIT_INSERT_TIMESTAMP:		// second	1000 milli
	case IDM_EDIT_INSERT_TIMESTAMP_MS:	// milli	1000 micro
	case IDM_EDIT_INSERT_TIMESTAMP_US:	// micro 	1000 nano
	case IDM_EDIT_INSERT_TIMESTAMP_NS: {// nano
		char mszBuf[32];
		FILETIME ft;
		// Windows timestamp in 100-nanosecond
		GetSystemTimeAsFileTime(&ft);
		//GetSystemTimePreciseAsFileTime(&ft); // Win8 and above
		uint64_t timestamp = (((uint64_t)(ft.dwHighDateTime)) << 32) | ft.dwLowDateTime;
		// Between Jan 1, 1601 and Jan 1, 1970 there are 11644473600 seconds
		timestamp -= UINT64_C(11644473600) * 1000 * 1000 * 10;
		switch (LOWORD(wParam)) {
		case IDM_EDIT_INSERT_TIMESTAMP:		// second	1000 milli
			timestamp /= 1000U * 1000 * 10;
			break;
		case IDM_EDIT_INSERT_TIMESTAMP_MS:	// milli	1000 micro
			timestamp /= 1000U * 10;
			break;
		case IDM_EDIT_INSERT_TIMESTAMP_US:	// micro 	1000 nano
			timestamp /= 10U;
			break;
		case IDM_EDIT_INSERT_TIMESTAMP_NS:	// nano
			timestamp *= 100U;
			break;
		}
		sprintf(mszBuf, "%" PRIu64, timestamp);
		SendMessage(hwndEdit, SCI_REPLACESEL, 0, (LPARAM)mszBuf);
	}
	break;

	case CMD_INSERTFILENAME_NOEXT:
	case IDM_EDIT_INSERT_FILENAME:
	case IDM_EDIT_INSERT_PATHNAME:
	case CMD_COPYFILENAME_NOEXT:
	case CMD_COPYFILENAME:
	case CMD_COPYPATHNAME: {
		SHFILEINFO shfi;
		WCHAR *pszInsert;
		WCHAR tchUntitled[MAX_PATH];

		const int cmd = LOWORD(wParam);
		if (StrNotEmpty(szCurFile)) {
			if (cmd == IDM_EDIT_INSERT_FILENAME || cmd == CMD_COPYFILENAME || cmd == CMD_INSERTFILENAME_NOEXT || cmd == CMD_COPYFILENAME_NOEXT) {
				SHGetFileInfo2(szCurFile, 0, &shfi, sizeof(SHFILEINFO), SHGFI_DISPLAYNAME);
				pszInsert = shfi.szDisplayName;
			} else {
				pszInsert = szCurFile;
			}
		} else {
			GetString(IDS_UNTITLED, tchUntitled, COUNTOF(tchUntitled));
			pszInsert = tchUntitled;
		}
		if (cmd == CMD_INSERTFILENAME_NOEXT || cmd == CMD_COPYFILENAME_NOEXT) {
			PathRemoveExtension(pszInsert);
		}

		if (cmd == CMD_COPYFILENAME || cmd == CMD_COPYFILENAME_NOEXT || cmd == CMD_COPYPATHNAME) {
			SetClipData(hwnd, pszInsert);
		} else {
			//int iSelStart;
			char mszBuf[MAX_PATH * kMaxMultiByteCount];
			const UINT cpEdit = (UINT)SendMessage(hwndEdit, SCI_GETCODEPAGE, 0, 0);
			WideCharToMultiByte(cpEdit, 0, pszInsert, -1, mszBuf, COUNTOF(mszBuf), NULL, NULL);
			//iSelStart = SendMessage(hwndEdit, SCI_GETSELECTIONSTART, 0, 0);
			SendMessage(hwndEdit, SCI_REPLACESEL, 0, (LPARAM)mszBuf);
			//SendMessage(hwndEdit, SCI_SETANCHOR, iSelStart, 0);
		}
	}
	break;

	case IDM_EDIT_LINECOMMENT:
		EditToggleCommentLine(hwndEdit);
		break;

	case IDM_EDIT_STREAMCOMMENT:
		EditToggleCommentBlock(hwndEdit);
		break;

	case IDM_EDIT_URLENCODE:
		BeginWaitCursor();
		EditURLEncode(hwndEdit);
		EndWaitCursor();
		break;

	case IDM_EDIT_URLDECODE:
		BeginWaitCursor();
		EditURLDecode(hwndEdit);
		EndWaitCursor();
		break;

	case IDM_EDIT_ESCAPECCHARS:
		BeginWaitCursor();
		EditEscapeCChars(hwndEdit);
		EndWaitCursor();
		break;

	case IDM_EDIT_UNESCAPECCHARS:
		BeginWaitCursor();
		EditUnescapeCChars(hwndEdit);
		EndWaitCursor();
		break;

	case IDM_EDIT_XHTML_ESCAPE_CHAR:
		BeginWaitCursor();
		EditEscapeXHTMLChars(hwndEdit);
		EndWaitCursor();
		break;

	case IDM_EDIT_XHTML_UNESCAPE_CHAR:
		BeginWaitCursor();
		EditUnescapeXHTMLChars(hwndEdit);
		EndWaitCursor();
		break;

	case IDM_EDIT_CHAR2HEX:
		BeginWaitCursor();
		EditChar2Hex(hwndEdit);
		EndWaitCursor();
		break;

	case IDM_EDIT_HEX2CHAR:
		BeginWaitCursor();
		EditHex2Char(hwndEdit);
		EndWaitCursor();
		break;

	case IDM_EDIT_SHOW_HEX:
		BeginWaitCursor();
		EditShowHex(hwndEdit);
		EndWaitCursor();
		break;

	case IDM_EDIT_NUM2HEX:
		BeginWaitCursor();
		EditConvertNumRadix(hwndEdit, 16);
		EndWaitCursor();
		break;

	case IDM_EDIT_NUM2DEC:
		BeginWaitCursor();
		EditConvertNumRadix(hwndEdit, 10);
		EndWaitCursor();
		break;

	case IDM_EDIT_NUM2BIN:
		BeginWaitCursor();
		EditConvertNumRadix(hwndEdit, 2);
		EndWaitCursor();
		break;

	case IDM_EDIT_NUM2OCT:
		BeginWaitCursor();
		EditConvertNumRadix(hwndEdit, 8);
		EndWaitCursor();
		break;

	case IDM_EDIT_FINDMATCHINGBRACE: {
		int iBrace2 = -1;
		int iPos = (int)SendMessage(hwndEdit, SCI_GETCURRENTPOS, 0, 0);
		int ch = (int)SendMessage(hwndEdit, SCI_GETCHARAT, iPos, 0);
		if (ch < 0x80 && strchr("()[]{}<>", ch)) {
			iBrace2 = (int)SendMessage(hwndEdit, SCI_BRACEMATCH, iPos, 0);
		} else { // Try one before
			iPos = (int)SendMessage(hwndEdit, SCI_POSITIONBEFORE, iPos, 0);
			ch = (int)SendMessage(hwndEdit, SCI_GETCHARAT, iPos, 0);
			if (ch < 0x80 && strchr("()[]{}<>", ch)) {
				iBrace2 = (int)SendMessage(hwndEdit, SCI_BRACEMATCH, iPos, 0);
			}
		}
		if (iBrace2 != -1) {
			SendMessage(hwndEdit, SCI_GOTOPOS, iBrace2, 0);
		}
	}
	break;

	case IDM_EDIT_SELTOMATCHINGBRACE: {
		int iBrace2 = -1;
		int iPos = (int)SendMessage(hwndEdit, SCI_GETCURRENTPOS, 0, 0);
		int ch = (int)SendMessage(hwndEdit, SCI_GETCHARAT, iPos, 0);
		if (ch < 0x80 && strchr("()[]{}<>", ch)) {
			iBrace2 = (int)SendMessage(hwndEdit, SCI_BRACEMATCH, iPos, 0);
		} else { // Try one before
			iPos = (int)SendMessage(hwndEdit, SCI_POSITIONBEFORE, iPos, 0);
			ch = (int)SendMessage(hwndEdit, SCI_GETCHARAT, iPos, 0);
			if (ch < 0x80 && strchr("()[]{}<>", ch)) {
				iBrace2 = (int)SendMessage(hwndEdit, SCI_BRACEMATCH, iPos, 0);
			}
		}
		if (iBrace2 != -1) {
			if (iBrace2 > iPos) {
				SendMessage(hwndEdit, SCI_SETSEL, iPos, iBrace2 + 1);
			} else {
				SendMessage(hwndEdit, SCI_SETSEL, iPos + 1, iBrace2);
			}
		}
	}
	break;

	case IDM_EDIT_FIND:
		if (!IsWindow(hDlgFindReplace)) {
			hDlgFindReplace = EditFindReplaceDlg(hwndEdit, &efrData, FALSE);
		} else {
			if (GetDlgItem(hDlgFindReplace, IDC_REPLACE)) {
				SendWMCommand(hDlgFindReplace, IDMSG_SWITCHTOFIND);
				DestroyWindow(hDlgFindReplace);
				hDlgFindReplace = EditFindReplaceDlg(hwndEdit, &efrData, FALSE);
			} else {
				SetForegroundWindow(hDlgFindReplace);
				PostMessage(hDlgFindReplace, WM_NEXTDLGCTL, (WPARAM)(GetDlgItem(hDlgFindReplace, IDC_FINDTEXT)), 1);
			}
		}
		break;

	// Main Bookmark Functions
	case BME_EDIT_BOOKMARKNEXT: {
		const int iPos = (int)SendMessage(hwndEdit, SCI_GETCURRENTPOS, 0, 0);
		const int iLine = (int)SendMessage(hwndEdit, SCI_LINEFROMPOSITION, iPos, 0);

		const int bitmask = 1;
		int iNextLine = (int)SendMessage(hwndEdit, SCI_MARKERNEXT, iLine + 1, bitmask);
		if (iNextLine == -1) {
			iNextLine = (int)SendMessage(hwndEdit, SCI_MARKERNEXT, 0, bitmask);
		}

		if (iNextLine != -1) {
			SciCall_EnsureVisible(iNextLine);
			SendMessage(hwndEdit, SCI_GOTOLINE, iNextLine, 0);
			SciCall_SetYCaretPolicy(CARET_SLOP | CARET_STRICT | CARET_EVEN, 10);
			SciCall_ScrollCaret();
			SciCall_SetYCaretPolicy(CARET_EVEN, 0);
		}
	}
	break;

	case BME_EDIT_BOOKMARKPREV: {
		const int iPos = (int)SendMessage(hwndEdit, SCI_GETCURRENTPOS, 0, 0);
		const int iLine = (int)SendMessage(hwndEdit, SCI_LINEFROMPOSITION, iPos, 0);

		const int bitmask = 1;
		int iNextLine = (int)SendMessage(hwndEdit, SCI_MARKERPREVIOUS, iLine - 1, bitmask);
		if (iNextLine == -1) {
			const int nLines = (int)SendMessage(hwndEdit, SCI_GETLINECOUNT, 0, 0);
			iNextLine = (int)SendMessage(hwndEdit, SCI_MARKERPREVIOUS, nLines, bitmask);
		}

		if (iNextLine != -1) {
			SciCall_EnsureVisible(iNextLine);
			SendMessage(hwndEdit, SCI_GOTOLINE, iNextLine, 0);
			SciCall_SetYCaretPolicy(CARET_SLOP | CARET_STRICT | CARET_EVEN, 10);
			SciCall_ScrollCaret();
			SciCall_SetYCaretPolicy(CARET_EVEN, 0);
		}
	}
	break;

	case BME_EDIT_BOOKMARKTOGGLE: {
		const int iPos = (int)SendMessage(hwndEdit, SCI_GETCURRENTPOS, 0, 0);
		const int iLine = (int)SendMessage(hwndEdit, SCI_LINEFROMPOSITION, iPos, 0);

		const int bitmask = (int)SendMessage(hwndEdit, SCI_MARKERGET, iLine, 0);
		if (bitmask & 1) {
			// unset
			SendMessage(hwndEdit, SCI_MARKERDELETE, iLine, 0);
		} else {
			if (bShowSelectionMargin) {
				SendMessage(hwndEdit, SCI_MARKERDEFINEPIXMAP, 0, (LPARAM)bookmark_pixmap);
			} else {
				SendMessage(hwndEdit, SCI_MARKERSETBACK, 0, NP2_BookmarkLineForeColor);
				SendMessage(hwndEdit, SCI_MARKERSETALPHA, 0, NP2_BookmarkLineColorAlpha);
				SendMessage(hwndEdit, SCI_MARKERDEFINE, 0, SC_MARK_BACKGROUND);
			}

			// set
			SendMessage(hwndEdit, SCI_MARKERADD, iLine, 0);
		}
	}
	break;

	case BME_EDIT_BOOKMARKCLEAR:
		SendMessage(hwndEdit, SCI_MARKERDELETEALL, (WPARAM)(-1), 0);
		break;

	case IDM_EDIT_FINDNEXT:
	case IDM_EDIT_FINDPREV:
	case IDM_EDIT_REPLACENEXT:
	case IDM_EDIT_SELTONEXT:
	case IDM_EDIT_SELTOPREV:
		if (SendMessage(hwndEdit, SCI_GETLENGTH, 0, 0) == 0) {
			break;
		}

		if (StrIsEmptyA(efrData.szFind)) {
			if (LOWORD(wParam) != IDM_EDIT_REPLACENEXT) {
				SendWMCommand(hwnd, IDM_EDIT_FIND);
			} else {
				SendWMCommand(hwnd, IDM_EDIT_REPLACE);
			}
		} else {
			const UINT cpEdit = (UINT)SendMessage(hwndEdit, SCI_GETCODEPAGE, 0, 0);
			if (cpLastFind != cpEdit) {
				if (cpEdit != SC_CP_UTF8) {
					WCHAR wch[NP2_FIND_REPLACE_LIMIT];

					MultiByteToWideChar(CP_UTF8, 0, efrData.szFindUTF8, -1, wch, COUNTOF(wch));
					WideCharToMultiByte(cpEdit, 0, wch, -1, efrData.szFind, COUNTOF(efrData.szFind), NULL, NULL);

					MultiByteToWideChar(CP_UTF8, 0, efrData.szReplaceUTF8, -1, wch, COUNTOF(wch));
					WideCharToMultiByte(cpEdit, 0, wch, -1, efrData.szReplace, COUNTOF(efrData.szReplace), NULL, NULL);
				} else {
					strcpy(efrData.szFind, efrData.szFindUTF8);
					strcpy(efrData.szReplace, efrData.szReplaceUTF8);
				}
			}

			cpLastFind = cpEdit;
			switch (LOWORD(wParam)) {
			case IDM_EDIT_FINDNEXT:
				EditFindNext(hwndEdit, &efrData, FALSE);
				break;

			case IDM_EDIT_FINDPREV:
				EditFindPrev(hwndEdit, &efrData, FALSE);
				break;

			case IDM_EDIT_REPLACENEXT:
				if (bReplaceInitialized) {
					EditReplace(hwndEdit, &efrData);
				} else {
					SendWMCommand(hwnd, IDM_EDIT_REPLACE);
				}
				break;

			case IDM_EDIT_SELTONEXT:
				EditFindNext(hwndEdit, &efrData, TRUE);
				break;

			case IDM_EDIT_SELTOPREV:
				EditFindPrev(hwndEdit, &efrData, TRUE);
				break;
			}
		}
		break;

	case IDM_EDIT_SELTOEND:
	case IDM_EDIT_SELTOBEGIN: {
		int selStart;
		int selEnd;
		if (LOWORD(wParam) == IDM_EDIT_SELTOEND) {
			selStart = (int)SendMessage(hwndEdit, SCI_GETSELECTIONSTART, 0, 0);
			selEnd = (int)SendMessage(hwndEdit, SCI_GETLENGTH, 0, 0);
		} else {
			selStart = 0;
			selEnd = (int)SendMessage(hwndEdit, SCI_GETSELECTIONEND, 0, 0);
		}
		SendMessage(hwndEdit, SCI_SETSELECTIONSTART, selStart, 0);
		SendMessage(hwndEdit, SCI_SETSELECTIONEND, selEnd, 0);
	}
	break;

	case IDM_EDIT_COMPLETEWORD:
		EditCompleteWord(hwndEdit, TRUE);
		break;

	case IDM_EDIT_REPLACE:
		if (!IsWindow(hDlgFindReplace)) {
			hDlgFindReplace = EditFindReplaceDlg(hwndEdit, &efrData, TRUE);
		} else {
			if (!GetDlgItem(hDlgFindReplace, IDC_REPLACE)) {
				SendWMCommand(hDlgFindReplace, IDMSG_SWITCHTOREPLACE);
				DestroyWindow(hDlgFindReplace);
				hDlgFindReplace = EditFindReplaceDlg(hwndEdit, &efrData, TRUE);
			} else {
				SetForegroundWindow(hDlgFindReplace);
				PostMessage(hDlgFindReplace, WM_NEXTDLGCTL, (WPARAM)(GetDlgItem(hDlgFindReplace, IDC_FINDTEXT)), 1);
			}
		}
		break;

	case IDM_EDIT_GOTOLINE:
		EditLineNumDlg(hwndEdit);
		break;

	case IDM_VIEW_SCHEME:
		Style_SelectLexerDlg(hwndEdit);
		break;

	case IDM_VIEW_USE2NDDEFAULT:
		Style_ToggleUse2ndDefault(hwndEdit);
		break;

	case IDM_VIEW_SCHEMECONFIG:
		Style_ConfigDlg(hwndEdit);
		break;

	case IDM_VIEW_FONT:
		Style_SetDefaultFont(hwndEdit);
		break;

	case IDM_VIEW_WORDWRAP:
		fWordWrap = !fWordWrap;
		SendMessage(hwndEdit, SCI_SETWRAPMODE, (fWordWrap? iWordWrapMode : SC_WRAP_NONE), 0);
		fWordWrapG = fWordWrap;
		UpdateToolbar();
		break;

	case IDM_VIEW_WORDWRAPSETTINGS:
		if (WordWrapSettingsDlg(hwnd, &iWordWrapIndent)) {
			SendMessage(hwndEdit, SCI_SETWRAPMODE, (fWordWrap? iWordWrapMode : SC_WRAP_NONE), 0);
			SendMessage(hwndEdit, SCI_SETMARGINOPTIONS, (bWordWrapSelectSubLine ? SC_MARGINOPTION_SUBLINESELECT : SC_MARGINOPTION_NONE), 0);
			fWordWrapG = fWordWrap;
			UpdateToolbar();
			SetWrapIndentMode();
			SetWrapVisualFlags();
		}
		break;

	case IDM_VIEW_LONGLINEMARKER:
		bMarkLongLines = !bMarkLongLines;
		if (bMarkLongLines) {
			SendMessage(hwndEdit, SCI_SETEDGEMODE, (iLongLineMode == EDGE_LINE) ? EDGE_LINE : EDGE_BACKGROUND, 0);
			Style_SetLongLineColors(hwndEdit);
		} else {
			SendMessage(hwndEdit, SCI_SETEDGEMODE, EDGE_NONE, 0);
		}
		break;

	case IDM_VIEW_LONGLINESETTINGS:
		if (LongLineSettingsDlg(hwnd, &iLongLinesLimit)) {
			bMarkLongLines = TRUE;
			SendMessage(hwndEdit, SCI_SETEDGEMODE, (iLongLineMode == EDGE_LINE) ? EDGE_LINE : EDGE_BACKGROUND, 0);
			Style_SetLongLineColors(hwndEdit);
			iLongLinesLimit = clamp_i(iLongLinesLimit, 0, NP2_LONG_LINE_LIMIT);
			SendMessage(hwndEdit, SCI_SETEDGECOLUMN, iLongLinesLimit, 0);
			iLongLinesLimitG = iLongLinesLimit;
		}
		break;

	case IDM_VIEW_TABSASSPACES:
		bTabsAsSpaces = !bTabsAsSpaces;
		SendMessage(hwndEdit, SCI_SETUSETABS, !bTabsAsSpaces, 0);
		bTabsAsSpacesG = bTabsAsSpaces;
		break;

	case IDM_VIEW_TABSETTINGS:
		if (TabSettingsDlg(hwnd)) {
			SendMessage(hwndEdit, SCI_SETUSETABS, !bTabsAsSpaces, 0);
			SendMessage(hwndEdit, SCI_SETTABINDENTS, bTabIndents, 0);
			SendMessage(hwndEdit, SCI_SETBACKSPACEUNINDENTS, bBackspaceUnindents, 0);
			iTabWidth = clamp_i(iTabWidth, 1, 256);
			iIndentWidth = clamp_i(iIndentWidth, 0, 256);
			SendMessage(hwndEdit, SCI_SETTABWIDTH, iTabWidth, 0);
			SendMessage(hwndEdit, SCI_SETINDENT, iIndentWidth, 0);
			bTabsAsSpacesG = bTabsAsSpaces;
			bTabIndentsG = bTabIndents;
			iTabWidthG = iTabWidth;
			iIndentWidthG = iIndentWidth;
			if (SendMessage(hwndEdit, SCI_GETWRAPINDENTMODE, 0, 0) == SC_WRAPINDENT_FIXED) {
				SetWrapStartIndent();
			}
		}
		break;

	case IDM_VIEW_SHOWINDENTGUIDES:
		bShowIndentGuides = !bShowIndentGuides;
		Style_SetIndentGuides(hwndEdit, bShowIndentGuides);
		break;

	case IDM_VIEW_LINENUMBERS:
		bShowLineNumbers = !bShowLineNumbers;
		UpdateLineNumberWidth();
		break;

	case IDM_VIEW_MARGIN:
		bShowSelectionMargin = !bShowSelectionMargin;
		UpdateSelectionMarginWidth();

		//Depending on if the margin is visible or not, choose different bookmark indication
		if (bShowSelectionMargin) {
			SendMessage(hwndEdit, SCI_MARKERDEFINEPIXMAP, 0, (LPARAM)bookmark_pixmap);
		} else {
			SendMessage(hwndEdit, SCI_MARKERSETBACK, 0, NP2_BookmarkLineForeColor);
			SendMessage(hwndEdit, SCI_MARKERSETALPHA, 0, NP2_BookmarkLineColorAlpha);
			SendMessage(hwndEdit, SCI_MARKERDEFINE, 0, SC_MARK_BACKGROUND);
		}
		break;

	case IDM_VIEW_AUTOCOMPLETEWORDS:
		if (AutoCompletionSettingsDlg(hwnd)) {
			if (!autoCompletionConfig.bCompleteWord) {
				// close the autocompletion list
				SendMessage(hwndEdit, SCI_AUTOCCANCEL, 0, 0);
			}
		}
		break;

	case IDM_VIEW_MARKOCCURRENCES_OFF:
		iMarkOccurrences = 0;
		// clear all marks
		SendMessage(hwndEdit, SCI_SETINDICATORCURRENT, 1, 0);
		SendMessage(hwndEdit, SCI_INDICATORCLEARRANGE, 0, SendMessage(hwndEdit, SCI_GETLENGTH, 0, 0));
		break;

	case IDM_VIEW_MARKOCCURRENCES_RED:
		iMarkOccurrences = 1;
		EditMarkAll(hwndEdit, iMarkOccurrences, bMarkOccurrencesMatchCase, bMarkOccurrencesMatchWords);
		UpdateStatusbar();
		break;

	case IDM_VIEW_MARKOCCURRENCES_GREEN:
		iMarkOccurrences = 2;
		EditMarkAll(hwndEdit, iMarkOccurrences, bMarkOccurrencesMatchCase, bMarkOccurrencesMatchWords);
		UpdateStatusbar();
		break;

	case IDM_VIEW_MARKOCCURRENCES_BLUE:
		iMarkOccurrences = 3;
		EditMarkAll(hwndEdit, iMarkOccurrences, bMarkOccurrencesMatchCase, bMarkOccurrencesMatchWords);
		UpdateStatusbar();
		break;

	case IDM_VIEW_MARKOCCURRENCES_CASE:
		bMarkOccurrencesMatchCase = !bMarkOccurrencesMatchCase;
		EditMarkAll(hwndEdit, iMarkOccurrences, bMarkOccurrencesMatchCase, bMarkOccurrencesMatchWords);
		break;

	case IDM_VIEW_MARKOCCURRENCES_WORD:
		bMarkOccurrencesMatchWords = !bMarkOccurrencesMatchWords;
		EditMarkAll(hwndEdit, iMarkOccurrences, bMarkOccurrencesMatchCase, bMarkOccurrencesMatchWords);
		UpdateStatusbar();
		break;

	case IDM_VIEW_FOLDING:
		bShowCodeFolding = !bShowCodeFolding;
		UpdateFoldMarginWidth();
		UpdateToolbar();
		if (!bShowCodeFolding) {
			FoldToggleAll(FOLD_ACTION_EXPAND);
		}
		break;

	case IDM_VIEW_TOGGLEFOLDS:
		if (bShowCodeFolding) {
			FoldToggleDefault(FOLD_ACTION_SNIFF);
		}
		break;

	case IDM_VIEW_FOLD_ALL:
		if (bShowCodeFolding) {
			FoldToggleAll(FOLD_ACTION_SNIFF);
		}
		break;

	case IDM_VIEW_FOLD_CURRENT:
		if (bShowCodeFolding) {
			FoldToggleCurrent(FOLD_ACTION_SNIFF);
		}
		break;

	case IDM_VIEW_FOLD_LEVEL1:
	case IDM_VIEW_FOLD_LEVEL2:
	case IDM_VIEW_FOLD_LEVEL3:
	case IDM_VIEW_FOLD_LEVEL4:
	case IDM_VIEW_FOLD_LEVEL5:
	case IDM_VIEW_FOLD_LEVEL6:
	case IDM_VIEW_FOLD_LEVEL7:
	case IDM_VIEW_FOLD_LEVEL8:
	case IDM_VIEW_FOLD_LEVEL9:
	case IDM_VIEW_FOLD_LEVEL10:
		if (bShowCodeFolding) {
			FoldToggleLevel(LOWORD(wParam) - IDM_VIEW_FOLD_LEVEL1, FOLD_ACTION_SNIFF);
		}
		break;

	case IDM_VIEW_SHOWWHITESPACE:
		bViewWhiteSpace = !bViewWhiteSpace;
		SendMessage(hwndEdit, SCI_SETVIEWWS, (bViewWhiteSpace) ? SCWS_VISIBLEALWAYS : SCWS_INVISIBLE, 0);
		break;

	case IDM_VIEW_SHOWEOLS:
		bViewEOLs = !bViewEOLs;
		SendMessage(hwndEdit, SCI_SETVIEWEOL, bViewEOLs, 0);
		break;

	case IDM_VIEW_WORDWRAPSYMBOLS:
		bShowWordWrapSymbols = !bShowWordWrapSymbols;
		SetWrapVisualFlags();
		break;

	case IDM_VIEW_UNICODE_CONTROL_CHAR:
		bShowUnicodeControlCharacter = !bShowUnicodeControlCharacter;
		EditShowUnicodeControlCharacter(hwndEdit, bShowUnicodeControlCharacter);
		// temporary fix for issue #77: force InvalidateStyleRedraw().
		SendMessage(hwndEdit, SCI_SETVIEWEOL, bViewEOLs, 0);
		break;

#if NP2_ENABLE_SHOW_CALL_TIPS
	case IDM_VIEW_SHOWCALLTIPS:
		bShowCallTips = !bShowCallTips;
		SetCallTipsWaitTime();
		break;
#endif

	case IDM_VIEW_MATCHBRACES:
		bMatchBraces = !bMatchBraces;
		if (bMatchBraces) {
			struct SCNotification scn;
			scn.nmhdr.hwndFrom = hwndEdit;
			scn.nmhdr.idFrom = IDC_EDIT;
			scn.nmhdr.code = SCN_UPDATEUI;
			scn.updated = SC_UPDATE_CONTENT;
			SendMessage(hwnd, WM_NOTIFY, IDC_EDIT, (LPARAM)&scn);
		} else {
			SendMessage(hwndEdit, SCI_BRACEHIGHLIGHT, (WPARAM)(-1), -1);
		}
		break;

	case IDM_VIEW_HIGHLIGHTCURRENTLINE_NONE:
	case IDM_VIEW_HIGHLIGHTCURRENTLINE_BACK:
	case IDM_VIEW_HIGHLIGHTCURRENTLINE_FRAME:
		iHighlightCurrentLine = LOWORD(wParam) - IDM_VIEW_HIGHLIGHTCURRENTLINE_NONE;
		Style_HighlightCurrentLine(hwndEdit);
		break;

	case IDM_VIEW_HIGHLIGHTCURRENTLINE_SUBLINE:
		bHighlightCurrentSubLine = !bHighlightCurrentSubLine;
		break;

	case IDM_VIEW_ZOOMIN:
		SendMessage(hwndEdit, SCI_ZOOMIN, 0, 0);
		break;

	case IDM_VIEW_ZOOMOUT:
		SendMessage(hwndEdit, SCI_ZOOMOUT, 0, 0);
		break;

	case IDM_VIEW_ZOOM_LEVEL:
		ZoomLevelDlg(hwnd, FALSE);
		break;

	case IDM_VIEW_RESETZOOM:
		SendMessage(hwndEdit, SCI_SETZOOM, 100, 0);
		break;

	case IDM_VIEW_TOOLBAR:
		bShowToolbar = !bShowToolbar;
		if (bShowToolbar) {
			UpdateToolbar();
			ShowWindow(hwndReBar, SW_SHOW);
		} else {
			ShowWindow(hwndReBar, SW_HIDE);
		}
		SendWMSize(hwnd);
		break;

	case IDM_VIEW_CUSTOMIZETB:
		SendMessage(hwndToolbar, TB_CUSTOMIZE, 0, 0);
		break;

	case IDM_VIEW_STATUSBAR:
		bShowStatusbar = !bShowStatusbar;
		if (bShowStatusbar) {
			UpdateStatusbar();
			ShowWindow(hwndStatus, SW_SHOW);
		} else {
			ShowWindow(hwndStatus, SW_HIDE);
		}
		SendWMSize(hwnd);
		break;

	case IDM_VIEW_STICKYWINPOS:
		bStickyWinPos = !bStickyWinPos;
		IniSetBool(INI_SECTION_NAME_FLAGS, L"StickyWindowPosition", bStickyWinPos);
		if (bStickyWinPos) {
			WINDOWPLACEMENT wndpl;
			wndpl.length = sizeof(WINDOWPLACEMENT);
			GetWindowPlacement(hwndMain, &wndpl);

			wi.x = wndpl.rcNormalPosition.left;
			wi.y = wndpl.rcNormalPosition.top;
			wi.cx = wndpl.rcNormalPosition.right - wndpl.rcNormalPosition.left;
			wi.cy = wndpl.rcNormalPosition.bottom - wndpl.rcNormalPosition.top;
			wi.max = (IsZoomed(hwndMain) || (wndpl.flags & WPF_RESTORETOMAXIMIZED));

			SaveWindowPosition();
			InfoBox(0, L"MsgStickyWinPos", IDS_STICKYWINPOS);
		}
		break;

	case IDM_VIEW_CLEARWINPOS:
		IniClearSection(INI_SECTION_NAME_WINDOW_POSITION);
		break;

	case IDM_VIEW_REUSEWINDOW:
		bReuseWindow = !bReuseWindow;
		IniSetBool(INI_SECTION_NAME_FLAGS, L"ReuseWindow", bReuseWindow);
		break;

	case IDM_VIEW_SINGLEFILEINSTANCE:
		bSingleFileInstance = !bSingleFileInstance;
		IniSetBool(INI_SECTION_NAME_FLAGS, L"SingleFileInstance", bSingleFileInstance);
		break;

	case IDM_VIEW_ALWAYSONTOP:
		if (IsTopMost()) {
			bAlwaysOnTop = 0;
			flagAlwaysOnTop = 0;
			SetWindowPos(hwnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
		} else {
			bAlwaysOnTop = 1;
			flagAlwaysOnTop = 0;
			SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
		}
		break;

	case IDM_VIEW_MINTOTRAY:
		bMinimizeToTray = !bMinimizeToTray;
		break;

	case IDM_VIEW_TRANSPARENT:
		bTransparentMode = !bTransparentMode;
		SetWindowTransparentMode(hwnd, bTransparentMode, iOpacityLevel);
		break;

	case IDM_SET_RENDER_TECH_GDI:
	case IDM_SET_RENDER_TECH_D2D:
	case IDM_SET_RENDER_TECH_D2DRETAIN:
	case IDM_SET_RENDER_TECH_D2DDC:
		iRenderingTechnology = LOWORD(wParam) - IDM_SET_RENDER_TECH_GDI;
		SendMessage(hwndEdit, SCI_SETBUFFEREDDRAW, (iRenderingTechnology == SC_TECHNOLOGY_DEFAULT), 0);
		SendMessage(hwndEdit, SCI_SETTECHNOLOGY, iRenderingTechnology, 0);
		iRenderingTechnology = (int)SendMessage(hwndEdit, SCI_GETTECHNOLOGY, 0, 0);
		iBidirectional = (int)SendMessage(hwndEdit, SCI_GETBIDIRECTIONAL, 0, 0);
		break;

	case IDM_SET_RTL_LAYOUT_EDIT:
		bEditLayoutRTL = !bEditLayoutRTL;
		SetWindowLayoutRTL(hwndEdit, bEditLayoutRTL);
		// force InvalidateStyleRedraw().
		SendMessage(hwndEdit, SCI_SETVIEWEOL, bViewEOLs, 0);
		break;

	case IDM_SET_RTL_LAYOUT_OTHER:
		bWindowLayoutRTL = !bWindowLayoutRTL;
		break;

	case IDM_SET_BIDIRECTIONAL_NONE:
	case IDM_SET_BIDIRECTIONAL_L2R:
	case IDM_SET_BIDIRECTIONAL_R2L:
		SendMessage(hwndEdit, SCI_SETBIDIRECTIONAL, LOWORD(wParam) - IDM_SET_BIDIRECTIONAL_NONE, 0);
		iBidirectional = (int)SendMessage(hwndEdit, SCI_GETBIDIRECTIONAL, 0, 0);
		break;

	case IDM_SET_USE_INLINE_IME:
		bUseInlineIME = bUseInlineIME? SC_IME_WINDOWED : SC_IME_INLINE;
		SendMessage(hwndEdit, SCI_SETIMEINTERACTION, bUseInlineIME, 0);
		break;

	case IDM_SET_USE_BLOCK_CARET:
		bInlineIMEUseBlockCaret = !bInlineIMEUseBlockCaret;
		SendMessage(hwndEdit, SCI_SETINLINEIMEUSEBLOCKCARET, bInlineIMEUseBlockCaret, 0);
		break;

	case IDM_VIEW_FONTQUALITY_DEFAULT:
	case IDM_VIEW_FONTQUALITY_NONE:
	case IDM_VIEW_FONTQUALITY_STANDARD:
	case IDM_VIEW_FONTQUALITY_CLEARTYPE:
		iFontQuality = LOWORD(wParam) - IDM_VIEW_FONTQUALITY_DEFAULT;
		SendMessage(hwndEdit, SCI_SETFONTQUALITY, iFontQuality, 0);
		break;

	case IDM_VIEW_CARET_STYLE_BLOCK_OVR:
		iOvrCaretStyle = !iOvrCaretStyle;
		Style_UpdateCaret(hwndEdit);
		break;
	case IDM_VIEW_CARET_STYLE_BLOCK:
	case IDM_VIEW_CARET_STYLE_WIDTH1:
	case IDM_VIEW_CARET_STYLE_WIDTH2:
	case IDM_VIEW_CARET_STYLE_WIDTH3:
		iCaretStyle = LOWORD(wParam) -  IDM_VIEW_CARET_STYLE_BLOCK;
		Style_UpdateCaret(hwndEdit);
		break;

	case IDM_VIEW_CARET_STYLE_NOBLINK:
		iCaretBlinkPeriod = (iCaretBlinkPeriod == 0)? -1 : 0;
		Style_UpdateCaret(hwndEdit);
		break;

	case IDM_VIEW_SHOWFILENAMEONLY:
	case IDM_VIEW_SHOWFILENAMEFIRST:
	case IDM_VIEW_SHOWFULLPATH:
		iPathNameFormat = LOWORD(wParam) - IDM_VIEW_SHOWFILENAMEONLY;
		lstrcpy(szTitleExcerpt, L"");
		UpdateWindowTitle();
		break;

	case IDM_VIEW_SHOWEXCERPT:
		EditGetExcerpt(hwndEdit, szTitleExcerpt, COUNTOF(szTitleExcerpt));
		UpdateWindowTitle();
		break;

	case IDM_VIEW_NOSAVERECENT:
		bSaveRecentFiles = !bSaveRecentFiles;
		break;

	case IDM_VIEW_NOSAVEFINDREPL:
		bSaveFindReplace = !bSaveFindReplace;
		break;

	case IDM_VIEW_SAVEBEFORERUNNINGTOOLS:
		bSaveBeforeRunningTools = !bSaveBeforeRunningTools;
		break;

	case IDM_SET_OPEN_FOLDER_METAPATH:
		bOpenFolderWithMetapath = !bOpenFolderWithMetapath;
		break;

	case IDM_VIEW_CHANGENOTIFY:
		if (ChangeNotifyDlg(hwnd)) {
			InstallFileWatching(szCurFile);
		}
		break;

	case IDM_VIEW_NOESCFUNC:
		iEscFunction = 0;
		break;

	case IDM_VIEW_ESCMINIMIZE:
		iEscFunction = 1;
		break;

	case IDM_VIEW_ESCEXIT:
		iEscFunction = 2;
		break;

	case IDM_VIEW_SAVESETTINGS:
		bSaveSettings = !bSaveSettings;
		break;

	case IDM_VIEW_SAVESETTINGSNOW: {
		BOOL bCreateFailure = FALSE;

		if (StrIsEmpty(szIniFile)) {
			if (StrNotEmpty(szIniFile2)) {
				if (CreateIniFileEx(szIniFile2)) {
					lstrcpy(szIniFile, szIniFile2);
					lstrcpy(szIniFile2, L"");
				} else {
					bCreateFailure = TRUE;
				}
			} else {
				break;
			}
		}

		if (!bCreateFailure) {
			if (WritePrivateProfileString(INI_SECTION_NAME_SETTINGS, L"WriteTest", L"ok", szIniFile)) {
				BeginWaitCursor();
				StatusSetTextID(hwndStatus, STATUS_HELP, IDS_SAVINGSETTINGS);
				StatusSetSimple(hwndStatus, TRUE);
				InvalidateRect(hwndStatus, NULL, TRUE);
				UpdateWindow(hwndStatus);
				SaveSettings(TRUE);
				StatusSetSimple(hwndStatus, FALSE);
				EndWaitCursor();
				MsgBox(MBINFO, IDS_SAVEDSETTINGS);
			} else {
				dwLastIOError = GetLastError();
				MsgBox(MBWARN, IDS_WRITEINI_FAIL);
			}
		} else {
			MsgBox(MBWARN, IDS_CREATEINI_FAIL);
		}
	}
	break;

	case IDM_HELP_ABOUT:
		ThemedDialogBox(g_hInstance, MAKEINTRESOURCE(IDD_ABOUT), hwnd, AboutDlgProc);
		break;

	case IDM_CMDLINE_HELP:
		DisplayCmdLineHelp(hwnd);
		break;

	case IDM_HELP_PROJECT_HOME:
	case IDM_HELP_LATEST_RELEASE:
	case IDM_HELP_LATEST_BUILD:
	case IDM_HELP_REPORT_ISSUE:
	case IDM_HELP_FEATURE_REQUEST:
	case IDM_HELP_ONLINE_WIKI:
		OpenHelpLink(hwnd, LOWORD(wParam));
		break;

	case IDM_VIEW_TOGGLE_FULLSCREEN:
		bInFullScreenMode = !bInFullScreenMode;
		ToggleFullScreenMode();
		break;

	case IDM_VIEW_FULLSCREEN_ON_START:
		ToggleFullScreenModeConfig(FullScreenMode_OnStartup);
		break;

	case IDM_VIEW_FULLSCREEN_HIDE_TITLE:
		ToggleFullScreenModeConfig(FullScreenMode_HideCaption);
		break;

	case IDM_VIEW_FULLSCREEN_HIDE_MENU:
		ToggleFullScreenModeConfig(FullScreenMode_HideMenu);
		break;

	case CMD_ESCAPE:
		if (SendMessage(hwndEdit, SCI_AUTOCACTIVE, 0, 0)) {
			//close the AutoComplete box
			SendMessage(hwndEdit, SCI_AUTOCCANCEL, 0, 0);
		} else if (bInFullScreenMode) {
			bInFullScreenMode = FALSE;
			ToggleFullScreenMode();
		} else if (iEscFunction == 1) {
			SendMessage(hwnd, WM_SYSCOMMAND, SC_MINIMIZE, 0);
		} else if (iEscFunction == 2) {
			NP2ExitWind(hwnd);
		}
		break;

	case CMD_SHIFTESC:
		NP2ExitWind(hwnd);
		break;

	// Newline with toggled auto indent setting
	case CMD_CTRLENTER:
		autoCompletionConfig.bIndentText = !autoCompletionConfig.bIndentText;
		SendMessage(hwndEdit, SCI_NEWLINE, 0, 0);
		autoCompletionConfig.bIndentText = !autoCompletionConfig.bIndentText;
		break;

	case CMD_CTRLBACK: {
		const int iPos = (int)SendMessage(hwndEdit, SCI_GETCURRENTPOS, 0, 0);
		const int iAnchor = (int)SendMessage(hwndEdit, SCI_GETANCHOR, 0, 0);

		if (iPos != iAnchor) {
			SendMessage(hwndEdit, SCI_SETSEL, iPos, iPos);
		} else {
			const int iLine = (int)SendMessage(hwndEdit, SCI_LINEFROMPOSITION, iPos, 0);
			const int iStartPos = (int)SendMessage(hwndEdit, SCI_POSITIONFROMLINE, iLine, 0);
			const int iIndentPos = (int)SendMessage(hwndEdit, SCI_GETLINEINDENTPOSITION, iLine, 0);
			if (iPos == iStartPos) {
				SendMessage(hwndEdit, SCI_DELETEBACK, 0, 0);
			} else if (iPos <= iIndentPos) {
				SendMessage(hwndEdit, SCI_DELLINELEFT, 0, 0);
			} else {
				SendMessage(hwndEdit, SCI_DELWORDLEFT, 0, 0);
			}
		}
	}
	break;

	case CMD_CTRLDEL: {
		const int iPos = (int)SendMessage(hwndEdit, SCI_GETCURRENTPOS, 0, 0);
		const int iAnchor = (int)SendMessage(hwndEdit, SCI_GETANCHOR, 0, 0);

		if (iPos != iAnchor) {
			SendMessage(hwndEdit, SCI_SETSEL, iPos, iPos);
		} else {
			const int iLine = (int)SendMessage(hwndEdit, SCI_LINEFROMPOSITION, iPos, 0);
			const int iStartPos = (int)SendMessage(hwndEdit, SCI_POSITIONFROMLINE, iLine, 0);
			const int iEndPos = (int)SendMessage(hwndEdit, SCI_GETLINEENDPOSITION, iLine, 0);
			if (iStartPos != iEndPos) {
				SendMessage(hwndEdit, SCI_DELWORDRIGHT, 0, 0);
			} else { // iStartPos == iEndPos
				SendMessage(hwndEdit, SCI_LINEDELETE, 0, 0);
			}
		}
	}
	break;

	case CMD_CTRLTAB:
		SendMessage(hwndEdit, SCI_SETTABINDENTS, FALSE, 0);
		SendMessage(hwndEdit, SCI_SETUSETABS, TRUE, 0);
		SendMessage(hwndEdit, SCI_TAB, 0, 0);
		SendMessage(hwndEdit, SCI_SETUSETABS, !bTabsAsSpaces, 0);
		SendMessage(hwndEdit, SCI_SETTABINDENTS, bTabIndents, 0);
		break;

	case CMD_RECODEDEFAULT: {
		if (StrNotEmpty(szCurFile)) {
			if (iDefaultEncoding == CPI_UNICODEBOM) {
				iSrcEncoding = CPI_UNICODE;
			} else if (iDefaultEncoding == CPI_UNICODEBEBOM) {
				iSrcEncoding = CPI_UNICODEBE;
			} else if (iDefaultEncoding == CPI_UTF8SIGN) {
				iSrcEncoding = CPI_UTF8;
			} else {
				iSrcEncoding = iDefaultEncoding;
			}
			FileLoad(FALSE, FALSE, TRUE, FALSE, szCurFile);
		}
	}
	break;

	case CMD_RELOADANSI: {
		if (StrNotEmpty(szCurFile)) {
			iSrcEncoding = CPI_DEFAULT;
			FileLoad(FALSE, FALSE, TRUE, FALSE, szCurFile);
		}
	}
	break;

	case CMD_RELOADOEM: {
		if (StrNotEmpty(szCurFile)) {
			iSrcEncoding = CPI_OEM;
			FileLoad(FALSE, FALSE, TRUE, FALSE, szCurFile);
		}
	}
	break;

	case CMD_RELOADANSIASUTF8: {
		if (StrNotEmpty(szCurFile)) {
			const BOOL back = bLoadANSIasUTF8;
			bLoadANSIasUTF8 = TRUE;
			FileLoad(FALSE, FALSE, TRUE, FALSE, szCurFile);
			bLoadANSIasUTF8 = back;
		}
	}
	break;

	case CMD_RELOADNOFILEVARS: {
		if (StrNotEmpty(szCurFile)) {
			const int _fNoFileVariables = fNoFileVariables;
			const BOOL _bNoEncodingTags = bNoEncodingTags;
			fNoFileVariables = 1;
			bNoEncodingTags = 1;
			FileLoad(FALSE, FALSE, TRUE, FALSE, szCurFile);
			fNoFileVariables = _fNoFileVariables;
			bNoEncodingTags = _bNoEncodingTags;
		}
	}
	break;

	case IDM_LANG_DEFAULT:
	case IDM_LANG_APACHE:
	case IDM_LANG_WEB:
	case IDM_LANG_PHP:
	case IDM_LANG_JSP:
	case IDM_LANG_ASPX_CS:
	case IDM_LANG_ASPX_VB:
	case IDM_LANG_ASP_VBS:
	case IDM_LANG_ASP_JS:
	case IDM_LANG_XML:
	case IDM_LANG_XSD:
	case IDM_LANG_XSLT:
	case IDM_LANG_DTD:

	case IDM_LANG_ANT_BUILD:
	case IDM_LANG_MAVEN_POM:
	case IDM_LANG_MAVEN_SETTINGS:
	case IDM_LANG_IVY_MODULE:
	case IDM_LANG_IVY_SETTINGS:
	case IDM_LANG_PMD_RULESET:
	case IDM_LANG_CHECKSTYLE:

	case IDM_LANG_TOMCAT:
	case IDM_LANG_WEB_JAVA:
	case IDM_LANG_STRUTS:
	case IDM_LANG_HIB_CFG:
	case IDM_LANG_HIB_MAP:
	case IDM_LANG_SPRING_BEANS:
	case IDM_LANG_JBOSS:

	case IDM_LANG_WEB_NET:
	case IDM_LANG_RESX:
	case IDM_LANG_XAML:

	case IDM_LANG_PROPERTY_LIST:
	case IDM_LANG_ANDROID_MANIFEST:
	case IDM_LANG_ANDROID_LAYOUT:
	case IDM_LANG_SVG:

	case IDM_LANG_BASH:
	case IDM_LANG_CSHELL:
	case IDM_LANG_M4:

	case IDM_LANG_MATLAB:
	case IDM_LANG_OCTAVE:
	case IDM_LANG_SCILAB:

	case IDM_LANG_CSS:
	case IDM_LANG_SCSS:
	case IDM_LANG_LESS:
	case IDM_LANG_HSS:
		Style_SetLexerByLangIndex(hwndEdit, LOWORD(wParam));
		break;

	case CMD_TIMESTAMPS: {
		WCHAR wchFind[256] = {0};
		IniGetString(INI_SECTION_NAME_FLAGS, L"TimeStamp", L"\\$Date:[^\\$]+\\$ | $Date: %Y/%m/%d %H:%M:%S $", wchFind, COUNTOF(wchFind));

		WCHAR wchTemplate[256] = {0};
		WCHAR *pwchSep;
		if ((pwchSep = StrChr(wchFind, L'|')) != NULL) {
			lstrcpy(wchTemplate, pwchSep + 1);
			*pwchSep = 0;
		}

		StrTrim(wchFind, L" ");
		StrTrim(wchTemplate, L" ");

		if (StrIsEmpty(wchFind) || StrIsEmpty(wchTemplate)) {
			break;
		}

		SYSTEMTIME st;
		struct tm sst;
		GetLocalTime(&st);
		sst.tm_isdst = -1;
		sst.tm_sec	 = (int)st.wSecond;
		sst.tm_min	 = (int)st.wMinute;
		sst.tm_hour	 = (int)st.wHour;
		sst.tm_mday	 = (int)st.wDay;
		sst.tm_mon	 = (int)st.wMonth - 1;
		sst.tm_year	 = (int)st.wYear - 1900;
		sst.tm_wday	 = (int)st.wDayOfWeek;
		mktime(&sst);

		WCHAR wchReplace[256];
		wcsftime(wchReplace, COUNTOF(wchReplace), wchTemplate, &sst);

		const UINT cpEdit = (UINT)SendMessage(hwndEdit, SCI_GETCODEPAGE, 0, 0);
#if NP2_USE_DESIGNATED_INITIALIZER
		EDITFINDREPLACE efrTS = {
			.hwnd = hwndEdit,
			.fuFlags = SCFIND_REGEXP,
		};
#else
		EDITFINDREPLACE efrTS = { "", "", "", "", hwnd, SCFIND_REGEXP };
#endif

		WideCharToMultiByte(cpEdit, 0, wchFind, -1, efrTS.szFind, COUNTOF(efrTS.szFind), NULL, NULL);
		WideCharToMultiByte(cpEdit, 0, wchReplace, -1, efrTS.szReplace, COUNTOF(efrTS.szReplace), NULL, NULL);

		if (!EditIsEmptySelection()) {
			EditReplaceAllInSelection(hwndEdit, &efrTS, TRUE);
		} else {
			EditReplaceAll(hwndEdit, &efrTS, TRUE);
		}
	}
	break;

	case CMD_OPEN_PATH_OR_LINK:
		EditOpenSelection(hwndEdit, 0);
		break;
	case CMD_OPEN_CONTAINING_FOLDER:
		EditOpenSelection(hwndEdit, 4);
		break;

	case CMD_ONLINE_SEARCH_GOOGLE:
	case CMD_ONLINE_SEARCH_BING:
	case CMD_ONLINE_SEARCH_WIKI:
	case CMD_CUSTOM_ACTION1:
	case CMD_CUSTOM_ACTION2:
		EditSelectionAction(hwndEdit, LOWORD(wParam));
		break;

	case CMD_FINDNEXTSEL:
	case CMD_FINDPREVSEL:
	case IDM_EDIT_SAVEFIND: {
		int cchSelection = (int)SendMessage(hwndEdit, SCI_GETSELECTIONEND, 0, 0) -
						   (int)SendMessage(hwndEdit, SCI_GETSELECTIONSTART, 0, 0);

		if (cchSelection == 0) {
			SendWMCommand(hwnd, IDM_EDIT_SELECTWORD);
			cchSelection = (int)SendMessage(hwndEdit, SCI_GETSELECTIONEND, 0, 0) -
						   (int)SendMessage(hwndEdit, SCI_GETSELECTIONSTART, 0, 0);
		}

		if (cchSelection > 0 && cchSelection < NP2_FIND_REPLACE_LIMIT && SendMessage(hwndEdit, SCI_GETSELTEXT, 0, 0) < NP2_FIND_REPLACE_LIMIT) {
			char mszSelection[NP2_FIND_REPLACE_LIMIT];

			SendMessage(hwndEdit, SCI_GETSELTEXT, 0, (LPARAM)mszSelection);
			mszSelection[cchSelection] = 0; // zero terminate

			// Check lpszSelection and truncate newlines
			char *lpsz = strpbrk(mszSelection, "\r\n");
			if (lpsz) {
				*lpsz = '\0';
			}

			cpLastFind = (UINT)SendMessage(hwndEdit, SCI_GETCODEPAGE, 0, 0);
			strcpy(efrData.szFind, mszSelection);

			if (cpLastFind != SC_CP_UTF8) {
				WCHAR wszBuf[NP2_FIND_REPLACE_LIMIT];
				MultiByteToWideChar(cpLastFind, 0, mszSelection, -1, wszBuf, COUNTOF(wszBuf));
				WideCharToMultiByte(CP_UTF8, 0, wszBuf, -1, efrData.szFindUTF8, COUNTOF(efrData.szFindUTF8), NULL, NULL);
			} else {
				strcpy(efrData.szFindUTF8, mszSelection);
			}

			efrData.fuFlags &= (~(SCFIND_REGEXP | SCFIND_POSIX));
			efrData.bTransformBS = FALSE;

			switch (LOWORD(wParam)) {
			case IDM_EDIT_SAVEFIND:
				break;

			case CMD_FINDNEXTSEL:
				EditFindNext(hwndEdit, &efrData, FALSE);
				break;

			case CMD_FINDPREVSEL:
				EditFindPrev(hwndEdit, &efrData, FALSE);
				break;
			}
		}
	}
	break;

	case CMD_INCLINELIMIT:
	case CMD_DECLINELIMIT:
		if (!bMarkLongLines) {
			SendWMCommand(hwnd, IDM_VIEW_LONGLINEMARKER);
		} else {
			if (LOWORD(wParam) == CMD_INCLINELIMIT) {
				iLongLinesLimit++;
			} else {
				iLongLinesLimit--;
			}
			iLongLinesLimit = clamp_i(iLongLinesLimit, 0, NP2_LONG_LINE_LIMIT);
			SendMessage(hwndEdit, SCI_SETEDGECOLUMN, iLongLinesLimit, 0);
			UpdateStatusbar();
			iLongLinesLimitG = iLongLinesLimit;
		}
		break;

	case CMD_EMBRACKETR:	// Ctrl+4
		EditEncloseSelection(hwndEdit, L"(", L")");
		break;
	case CMD_EMBRACKETC:	// Ctrl+5
		EditEncloseSelection(hwndEdit, L"{", L"}");
		break;
	case CMD_EMBRACKETA:	// Ctrl+6
		EditEncloseSelection(hwndEdit, L"<", L">");
		break;
	case CMD_EMBRACKETS:	// Ctrl+7
		EditEncloseSelection(hwndEdit, L"[", L"]");
		break;
	case CMD_STRINGIFYS:	// Ctrl+1
		EditEncloseSelection(hwndEdit, L"'", L"'");
		break;
	case CMD_STRINGIFYD:	// Ctrl+2
		EditEncloseSelection(hwndEdit, L"\"", L"\"");
		break;
	case CMD_STRINGIFYT2:	// Ctrl+3
		EditEncloseSelection(hwndEdit, L"\"\"\"", L"\"\"\"");
		break;
	case CMD_STRINGIFYT1:	// Ctrl+9
		EditEncloseSelection(hwndEdit, L"'''", L"'''");
		break;
	case CMD_STRINGIFYB:	// Ctrl+8
		EditEncloseSelection(hwndEdit, L"`", L"`");
		break;

	case CMD_INCREASENUM:	// Ctrl++
		EditModifyNumber(hwndEdit, TRUE);
		break;

	case CMD_DECREASENUM:	// Ctrl+-
		EditModifyNumber(hwndEdit, FALSE);
		break;

	case CMD_JUMP2SELSTART:	// Ctrl+'
		if (SC_SEL_RECTANGLE != SendMessage(hwnd, SCI_GETSELECTIONMODE, 0, 0)) {
			const int iAnchorPos = (int)SendMessage(hwndEdit, SCI_GETANCHOR, 0, 0);
			const int iCursorPos = (int)SendMessage(hwndEdit, SCI_GETCURRENTPOS, 0, 0);
			if (iCursorPos > iAnchorPos) {
				SendMessage(hwndEdit, SCI_SETSEL, iCursorPos, iAnchorPos);
				SendMessage(hwndEdit, SCI_CHOOSECARETX, 0, 0);
			}
		}
		break;

	case CMD_JUMP2SELEND:	// Ctrl+.
		if (SC_SEL_RECTANGLE != SendMessage(hwnd, SCI_GETSELECTIONMODE, 0, 0)) {
			const int iAnchorPos = (int)SendMessage(hwndEdit, SCI_GETANCHOR, 0, 0);
			const int iCursorPos = (int)SendMessage(hwndEdit, SCI_GETCURRENTPOS, 0, 0);
			if (iCursorPos < iAnchorPos) {
				SendMessage(hwndEdit, SCI_SETSEL, iCursorPos, iAnchorPos);
				SendMessage(hwndEdit, SCI_CHOOSECARETX, 0, 0);
			}
		}
		break;

	case CMD_COPYWINPOS: {
		WINDOWPLACEMENT wndpl;
		wndpl.length = sizeof(WINDOWPLACEMENT);
		GetWindowPlacement(hwndMain, &wndpl);

		const int x = wndpl.rcNormalPosition.left;
		const int y = wndpl.rcNormalPosition.top;
		const int cx = wndpl.rcNormalPosition.right - wndpl.rcNormalPosition.left;
		const int cy = wndpl.rcNormalPosition.bottom - wndpl.rcNormalPosition.top;
		const BOOL max = IsZoomed(hwndMain) || (wndpl.flags & WPF_RESTORETOMAXIMIZED);

		WCHAR wszWinPos[64];
		wsprintf(wszWinPos, L"/pos %i,%i,%i,%i,%i", x, y, cx, cy, max);

		SetClipData(hwnd, wszWinPos);
		UpdateToolbar();
	}
	break;

	case CMD_DEFAULTWINPOS:
		SnapToDefaultPos(hwnd);
		break;

	case CMD_OPENINIFILE:
		if (StrNotEmpty(szIniFile)) {
			CreateIniFile();
			FileLoad(FALSE, FALSE, FALSE, FALSE, szIniFile);
		}
		break;

	case IDT_FILE_NEW:
		SendWMCommandOrBeep(hwnd, IDM_FILE_NEW);
		break;

	case IDT_FILE_OPEN:
		SendWMCommandOrBeep(hwnd, IDM_FILE_OPEN);
		break;

	case IDT_FILE_BROWSE:
		SendWMCommandOrBeep(hwnd, IDM_FILE_BROWSE);
		break;

	case IDT_FILE_SAVE:
		SendWMCommandOrBeep(hwnd, IDM_FILE_SAVE);
		break;

	case IDT_EDIT_UNDO:
		SendWMCommandOrBeep(hwnd, IDM_EDIT_UNDO);
		break;

	case IDT_EDIT_REDO:
		SendWMCommandOrBeep(hwnd, IDM_EDIT_REDO);
		break;

	case IDT_EDIT_CUT:
		SendWMCommandOrBeep(hwnd, IDM_EDIT_CUT);
		break;

	case IDT_EDIT_COPY:
		if (IsCmdEnabled(hwnd, IDM_EDIT_COPY)) {
			SendWMCommand(hwnd, IDM_EDIT_COPY);
		} else {
			SendWMCommand(hwnd, IDM_EDIT_COPYALL);
		}
		break;

	case IDT_EDIT_PASTE:
		SendWMCommandOrBeep(hwnd, IDM_EDIT_PASTE);
		break;

	case IDT_EDIT_FIND:
		SendWMCommandOrBeep(hwnd, IDM_EDIT_FIND);
		break;

	case IDT_EDIT_REPLACE:
		SendWMCommandOrBeep(hwnd, IDM_EDIT_REPLACE);
		break;

	case IDT_VIEW_WORDWRAP:
		SendWMCommandOrBeep(hwnd, IDM_VIEW_WORDWRAP);
		break;

	case IDT_VIEW_ZOOMIN:
		SendWMCommandOrBeep(hwnd, IDM_VIEW_ZOOMIN);
		break;

	case IDT_VIEW_ZOOMOUT:
		SendWMCommandOrBeep(hwnd, IDM_VIEW_ZOOMOUT);
		break;

	case IDT_VIEW_SCHEME:
		SendWMCommandOrBeep(hwnd, IDM_VIEW_SCHEME);
		break;

	case IDT_VIEW_SCHEMECONFIG:
		SendWMCommandOrBeep(hwnd, IDM_VIEW_SCHEMECONFIG);
		break;

	case IDT_FILE_EXIT:
		NP2ExitWind(hwnd);
		break;

	case IDT_FILE_SAVEAS:
		SendWMCommandOrBeep(hwnd, IDM_FILE_SAVEAS);
		break;

	case IDT_FILE_SAVECOPY:
		SendWMCommandOrBeep(hwnd, IDM_FILE_SAVECOPY);
		break;

	case IDT_EDIT_CLEAR:
		if (IsCmdEnabled(hwnd, IDM_EDIT_CLEAR)) {
			SendWMCommand(hwnd, IDM_EDIT_CLEAR);
		} else {
			SendMessage(hwndEdit, SCI_CLEARALL, 0, 0);
		}
		break;

	case IDT_FILE_PRINT:
		SendWMCommandOrBeep(hwnd, IDM_FILE_PRINT);
		break;

	case IDT_FILE_OPENFAV:
		SendWMCommandOrBeep(hwnd, IDM_FILE_OPENFAV);
		break;

	case IDT_FILE_ADDTOFAV:
		SendWMCommandOrBeep(hwnd, IDM_FILE_ADDTOFAV);
		break;

	case IDT_VIEW_TOGGLEFOLDS:
		SendWMCommandOrBeep(hwnd, IDM_VIEW_TOGGLEFOLDS);
		break;

	case IDT_FILE_LAUNCH:
		SendWMCommandOrBeep(hwnd, IDM_FILE_LAUNCH);
		break;

	}

	return 0;
}

//=============================================================================
//
// MsgNotify() - Handles WM_NOTIFY
//
//
LRESULT MsgNotify(HWND hwnd, WPARAM wParam, LPARAM lParam) {
	UNREFERENCED_PARAMETER(wParam);

	LPNMHDR pnmh = (LPNMHDR)lParam;
	const struct SCNotification *scn = (struct SCNotification *)lParam;

	switch (pnmh->idFrom) {
	case IDC_EDIT:
		switch (pnmh->code) {
		case SCN_UPDATEUI:
			if (scn->updated & ~(SC_UPDATE_V_SCROLL | SC_UPDATE_H_SCROLL)) {
				UpdateToolbar();

				if (scn->updated & (SC_UPDATE_SELECTION)) {
					// mark occurrences of text currently selected
					EditMarkAll(hwndEdit, iMarkOccurrences, bMarkOccurrencesMatchCase, bMarkOccurrencesMatchWords);
					UpdateStatusBarCache_OVRMode(FALSE);
				}
				UpdateStatusbar();

				// Brace Match
				if (bMatchBraces) {
					int iEndStyled = (int)SendMessage(hwndEdit, SCI_GETENDSTYLED, 0, 0);
					if (iEndStyled < (int)SendMessage(hwndEdit, SCI_GETLENGTH, 0, 0)) {
						const int iLine = (int)SendMessage(hwndEdit, SCI_LINEFROMPOSITION, iEndStyled, 0);
						iEndStyled = (int)SendMessage(hwndEdit, SCI_POSITIONFROMLINE, iLine, 0);
						SendMessage(hwndEdit, SCI_COLOURISE, iEndStyled, -1);
					}

					int iPos = (int)SendMessage(hwndEdit, SCI_GETCURRENTPOS, 0, 0);
					int ch = (int)SendMessage(hwndEdit, SCI_GETCHARAT, iPos, 0);
					if (ch < 0x80 && strchr("()[]{}<>", ch)) {
						const int iBrace2 = (int)SendMessage(hwndEdit, SCI_BRACEMATCH, iPos, 0);
						if (iBrace2 != -1) {
							const int col1 = (int)SendMessage(hwndEdit, SCI_GETCOLUMN, iPos, 0);
							const int col2 = (int)SendMessage(hwndEdit, SCI_GETCOLUMN, iBrace2, 0);
							SendMessage(hwndEdit, SCI_BRACEHIGHLIGHT, iPos, iBrace2);
							SendMessage(hwndEdit, SCI_SETHIGHLIGHTGUIDE, min_i(col1, col2), 0);
						} else {
							SendMessage(hwndEdit, SCI_BRACEBADLIGHT, iPos, 0);
							SendMessage(hwndEdit, SCI_SETHIGHLIGHTGUIDE, 0, 0);
						}
					} else { // Try one before
						iPos = (int)SendMessage(hwndEdit, SCI_POSITIONBEFORE, iPos, 0);
						ch = (int)SendMessage(hwndEdit, SCI_GETCHARAT, iPos, 0);
						if (ch < 0x80 && strchr("()[]{}<>", ch)) {
							const int iBrace2 = (int)SendMessage(hwndEdit, SCI_BRACEMATCH, iPos, 0);
							if (iBrace2 != -1) {
								const int col1 = (int)SendMessage(hwndEdit, SCI_GETCOLUMN, iPos, 0);
								const int col2 = (int)SendMessage(hwndEdit, SCI_GETCOLUMN, iBrace2, 0);
								SendMessage(hwndEdit, SCI_BRACEHIGHLIGHT, iPos, iBrace2);
								SendMessage(hwndEdit, SCI_SETHIGHLIGHTGUIDE, min_i(col1, col2), 0);
							} else {
								SendMessage(hwndEdit, SCI_BRACEBADLIGHT, iPos, 0);
								SendMessage(hwndEdit, SCI_SETHIGHLIGHTGUIDE, 0, 0);
							}
						} else {
							SendMessage(hwndEdit, SCI_BRACEHIGHLIGHT, (WPARAM)(-1), -1);
							SendMessage(hwndEdit, SCI_SETHIGHLIGHTGUIDE, 0, 0);
						}
					}
				}
			}
			break;

		case SCN_CHARADDED: {
			const int ch = scn->ch;
			if (ch < 0x80) {
				// Auto indent
				if (ch == '\r' || ch == '\n') {
					// in CRLF mode handle LF only...
					if (autoCompletionConfig.bIndentText && ((SC_EOL_CRLF == iEOLMode && ch != '\n') || SC_EOL_CRLF != iEOLMode)) {
						EditAutoIndent(hwndEdit);
					}
					return 0;
				}
				// Auto close tags
				if (ch == '>') {
					if (autoCompletionConfig.bCloseTags || autoCompletionConfig.bCompleteWord) {
						EditAutoCloseXMLTag(hwndEdit);
					}
					return 0;
				}
				// Auto close braces/quotes
				if (strchr("([{<\"\'`,", ch)) {
					if (autoCompletionConfig.fAutoInsertMask) {
						EditAutoCloseBraceQuote(hwndEdit, ch);
					}
					return 0;
				}
			}

			// auto complete word
			if (!autoCompletionConfig.bCompleteWord
				// ignore IME input
				|| (scn->modifiers && (ch >= 0x80 || autoCompletionConfig.bEnglistIMEModeOnly))
				|| !IsAutoCompletionWordCharacter(ch)
			) {
				return 0;
			}
			if (SendMessage(hwndEdit, SCI_AUTOCACTIVE, 0, 0)) {
				// many items in auto-completion list (> autoCompletionConfig.iVisibleItemCount), recreate it
				if (autoCompletionConfig.iPreviousItemCount < autoCompletionConfig.iVisibleItemCount) {
					return 0;
				}
				SendMessage(hwnd, SCI_AUTOCCANCEL, 0, 0);
			}
			EditCompleteWord(hwndEdit, FALSE);
		}
		break;

		case SCN_AUTOCSELECTION:
		case SCN_USERLISTSELECTION: {
			LPCSTR text = scn->text;
			// function/array/template/generic
			LPSTR braces = (LPSTR)strpbrk(text, "([{<");
			const Sci_Position iCurPos = SciCall_GetCurrentPos();
			const int ch = SciCall_GetCharAt(iCurPos);
			if (braces != NULL && *braces == ch) {
				*braces = L'\0'; // unsafe
			}
			const Sci_Position iNewPos = scn->position + ((braces == NULL) ? ((Sci_Position)strlen(text)) : (braces - text + 1));

			SendMessage(hwndEdit, SCI_BEGINUNDOACTION, 0, 0);
			SendMessage(hwndEdit, SCI_SETSEL, scn->position, iCurPos);
			SendMessage(hwndEdit, SCI_REPLACESEL, 0, (LPARAM)text);
			SendMessage(hwndEdit, SCI_SETSEL, iNewPos, iNewPos);
			SendMessage(hwndEdit, SCI_ENDUNDOACTION, 0, 0);
			SendMessage(hwndEdit, SCI_AUTOCCANCEL, 0, 0);
		}
		break;

		case SCN_AUTOCCHARDELETED:
			EditCompleteWord(hwndEdit, FALSE);
			break;

#if NP2_ENABLE_SHOW_CALL_TIPS
		// CallTips
		case SCN_DWELLSTART:
			if (bShowCallTips && scn->position >= 0) {
				EditShowCallTips(hwndEdit, scn->position);
			}
			break;

		case SCN_DWELLEND:
			SendMessage(hwndEdit, SCI_CALLTIPCANCEL, 0, 0);
			break;
#endif

		case SCN_MODIFIED:
			if (scn->linesAdded) {
				UpdateLineNumberWidth();
			}
			break;

		case SCN_ZOOM:
			MsgNotifyZoom();
			break;

		case SCN_SAVEPOINTREACHED:
			bModified = FALSE;
			UpdateWindowTitle();
			break;

		case SCN_MARGINCLICK:
			if (scn->margin == MARGIN_FOLD_INDEX) {
				FoldClick(SciCall_LineFromPosition(scn->position), scn->modifiers);
			}
			break;

		case SCN_KEY:
			// Also see the corresponding patch in scintilla\src\Editor.cxx
			FoldAltArrow(scn->ch, scn->modifiers);
			break;

		case SCN_SAVEPOINTLEFT:
			bModified = TRUE;
			UpdateWindowTitle();
			break;

		case SCN_URIDROPPED: {
			WCHAR szBuf[MAX_PATH + 40];
			if (MultiByteToWideChar(CP_UTF8, 0, scn->text, -1, szBuf, COUNTOF(szBuf)) > 0) {
				OnDropOneFile(hwnd, szBuf);
			}
		} break;

		case SCN_CODEPAGECHANGED:
			EditOnCodePageChanged(hwndEdit, scn->ch);
			break;
		}
		break;

	case IDC_TOOLBAR:
		switch (pnmh->code) {
		case TBN_ENDADJUST:
			UpdateToolbar();
			break;

		case TBN_QUERYDELETE:
		case TBN_QUERYINSERT:
			return TRUE;

		case TBN_GETBUTTONINFO: {
			LPTBNOTIFY lpTbNotify = (LPTBNOTIFY)lParam;
			if (lpTbNotify->iItem < (int)COUNTOF(tbbMainWnd)) {
				WCHAR tch[256];
				GetString(tbbMainWnd[lpTbNotify->iItem].idCommand, tch, COUNTOF(tch));
				lstrcpyn(lpTbNotify->pszText, tch, lpTbNotify->cchText);
				CopyMemory(&lpTbNotify->tbButton, &tbbMainWnd[lpTbNotify->iItem], sizeof(TBBUTTON));
				return TRUE;
			}
		}
		return FALSE;

		case TBN_RESET:
			Toolbar_SetButtons(hwndToolbar, DefaultToolbarButtons, tbbMainWnd, COUNTOF(tbbMainWnd));
			return FALSE;

		case TBN_DROPDOWN: {
			LPTBNOTIFY lpTbNotify = (LPTBNOTIFY)lParam;
			RECT rc;
			SendMessage(hwndToolbar, TB_GETRECT, lpTbNotify->iItem, (LPARAM)&rc);
			MapWindowPoints(hwndToolbar, HWND_DESKTOP, (LPPOINT)&rc, 2);
			HMENU hmenu = LoadMenu(g_hInstance, MAKEINTRESOURCE(IDR_POPUPMENU));
			TPMPARAMS tpm;
			tpm.cbSize = sizeof(TPMPARAMS);
			tpm.rcExclude = rc;
			TrackPopupMenuEx(GetSubMenu(hmenu, IDP_POPUP_SUBMENU_FOLD),
							TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_VERTICAL,
							rc.left, rc.bottom, hwndMain, &tpm);
			DestroyMenu(hmenu);
		}
		return FALSE;
		}
		break;

	case IDC_STATUSBAR:
		switch (pnmh->code) {
		case NM_CLICK: {
			LPNMMOUSE pnmm = (LPNMMOUSE)lParam;
			switch (pnmm->dwItemSpec) {
			case STATUS_EOLMODE:
				EditEnsureConsistentLineEndings(hwndEdit);
				return TRUE;

			case STATUS_DOCZOOM:
				ZoomLevelDlg(hwnd, TRUE);
				return TRUE;

			default:
				return FALSE;
			}
		}

		case NM_DBLCLK: {
			LPNMMOUSE pnmm = (LPNMMOUSE)lParam;
			switch (pnmm->dwItemSpec) {
			case STATUS_CODEPAGE:
				SendWMCommand(hwnd, IDM_ENCODING_SELECT);
				return TRUE;

			case STATUS_EOLMODE:
				if (iEOLMode == SC_EOL_CRLF) {
					iEOLMode = SC_EOL_LF;
				} else if (iEOLMode == SC_EOL_LF) {
					iEOLMode = SC_EOL_CR;
				} else {
					iEOLMode = SC_EOL_CRLF;
				}
				ConvertLineEndings(iEOLMode);
				return TRUE;

			case STATUS_LEXER:
				SendWMCommand(hwnd, IDM_VIEW_SCHEME);
				return TRUE;

			case STATUS_OVRMODE:
				SendMessage(hwndEdit, SCI_EDITTOGGLEOVERTYPE, 0, 0);
				UpdateStatusBarCache_OVRMode(TRUE);
				return TRUE;

			default:
				return FALSE;
			}
		}
		break;

		}
		break;

	default:
		switch (pnmh->code) {
		case TTN_NEEDTEXT: {
			LPTOOLTIPTEXT pTTT = (LPTOOLTIPTEXT)lParam;
			if (pTTT->uFlags & TTF_IDISHWND) {
				//nop;
			} else {
				WCHAR tch[256];
				GetString((UINT)pnmh->idFrom, tch, COUNTOF(tch));
				lstrcpyn(pTTT->szText, /*StrChr(tch, L'\n')+1*/tch, 80);
			}
		}
		break;

		}
		break;
	}

	return 0;
}

//=============================================================================
//
// LoadSettings()
//
//
void LoadSettings(void) {
	IniSection section;
	WCHAR *pIniSectionBuf = (WCHAR *)NP2HeapAlloc(sizeof(WCHAR) * MAX_INI_SECTION_SIZE_SETTINGS);
	const int cchIniSection = (int)(NP2HeapSize(pIniSectionBuf) / sizeof(WCHAR));
	IniSection *pIniSection = &section;
	IniSectionInit(pIniSection, 128);

	LoadIniSection(INI_SECTION_NAME_SETTINGS, pIniSectionBuf, cchIniSection);
	IniSectionParse(pIniSection, pIniSectionBuf);

	const int iSettingsVersion = IniSectionGetInt(pIniSection, L"SettingsVersion", NP2SettingsVersion_None);
	bSaveSettings = IniSectionGetBool(pIniSection, L"SaveSettings", 1);
	bSaveRecentFiles = IniSectionGetBool(pIniSection, L"SaveRecentFiles", 0);
	bSaveFindReplace = IniSectionGetBool(pIniSection, L"SaveFindReplace", 0);

	efrData.bFindClose = IniSectionGetBool(pIniSection, L"CloseFind", 0);
	efrData.bReplaceClose = IniSectionGetBool(pIniSection, L"CloseReplace", 0);
	efrData.bNoFindWrap = IniSectionGetBool(pIniSection, L"NoFindWrap", 0);

	LPCWSTR strValue = IniSectionGetValue(pIniSection, L"OpenWithDir");
	if (StrIsEmpty(strValue)) {
		SHGetSpecialFolderPath(NULL, tchOpenWithDir, CSIDL_DESKTOPDIRECTORY, TRUE);
	} else {
		PathAbsoluteFromApp(strValue, tchOpenWithDir, COUNTOF(tchOpenWithDir), TRUE);
	}

	strValue = IniSectionGetValue(pIniSection, L"Favorites");
	if (StrIsEmpty(strValue)) {
		SHGetFolderPath(NULL, CSIDL_PERSONAL, NULL, SHGFP_TYPE_CURRENT, tchFavoritesDir);
	} else {
		PathAbsoluteFromApp(strValue, tchFavoritesDir, COUNTOF(tchFavoritesDir), TRUE);
	}

	int iValue = IniSectionGetInt(pIniSection, L"PathNameFormat", 1);
	iPathNameFormat = clamp_i(iValue, 0, 2);

	fWordWrap = IniSectionGetBool(pIniSection, L"WordWrap", 1);
	fWordWrapG = fWordWrap;

	iValue = IniSectionGetInt(pIniSection, L"WordWrapMode", SC_WRAP_WORD);
	iWordWrapMode = clamp_i(iValue, SC_WRAP_WORD, SC_WRAP_WHITESPACE);

	iValue = IniSectionGetInt(pIniSection, L"WordWrapIndent", EditWrapIndentDefaultValue);
	iWordWrapIndent = clamp_i(iValue, EditWrapIndentNone, EditWrapIndentMaxValue);

	iValue = IniSectionGetInt(pIniSection, L"WordWrapSymbols", EditWrapSymbolDefaultValue);
	iValue = clamp_i(iValue, 0, EditWrapSymbolMaxValue);
	iWordWrapSymbols = clamp_i(iValue % 10, EditWrapSymbolBeforeNone, EditWrapSymbolBeforeMaxValue) + (iValue / 10) * 10;

	bShowWordWrapSymbols = IniSectionGetBool(pIniSection, L"ShowWordWrapSymbols", 0);
	bWordWrapSelectSubLine = IniSectionGetBool(pIniSection, L"WordWrapSelectSubLine", 0);
	bShowUnicodeControlCharacter = IniSectionGetBool(pIniSection, L"ShowUnicodeControlCharacter", 0);

	bMatchBraces = IniSectionGetBool(pIniSection, L"MatchBraces", 1);
	iValue = IniSectionGetInt(pIniSection, L"HighlightCurrentLine", 12);
	iValue = (iSettingsVersion < NP2SettingsVersion_V1) ? 12 : iValue;
	bHighlightCurrentSubLine = iValue > 10;
	iHighlightCurrentLine = clamp_i(iValue % 10, 0, 2);
	bShowIndentGuides = IniSectionGetBool(pIniSection, L"ShowIndentGuides", 0);

	autoCompletionConfig.bIndentText = IniSectionGetBool(pIniSection, L"AutoIndent", 1);
	autoCompletionConfig.bCloseTags = IniSectionGetBool(pIniSection, L"AutoCloseTags", 1);
	autoCompletionConfig.bCompleteWord = IniSectionGetBool(pIniSection, L"AutoCompleteWords", 1);
	autoCompletionConfig.bScanWordsInDocument = IniSectionGetBool(pIniSection, L"AutoCScanWordsInDocument", 1);
	autoCompletionConfig.bEnglistIMEModeOnly = IniSectionGetBool(pIniSection, L"AutoCEnglishIMEModeOnly", 0);
	iValue = IniSectionGetInt(pIniSection, L"AutoCVisibleItemCount", 16);
	autoCompletionConfig.iVisibleItemCount = max_i(iValue, MIN_AUTO_COMPLETION_VISIBLE_ITEM_COUNT);
	iValue = IniSectionGetInt(pIniSection, L"AutoCMinWordLength", 1);
	autoCompletionConfig.iMinWordLength = max_i(iValue, MIN_AUTO_COMPLETION_WORD_LENGTH);
	iValue = IniSectionGetInt(pIniSection, L"AutoCMinNumberLength", 3);
	autoCompletionConfig.iMinNumberLength = max_i(iValue, MIN_AUTO_COMPLETION_NUMBER_LENGTH);
	autoCompletionConfig.fAutoCompleteFillUpMask = IniSectionGetInt(pIniSection, L"AutoCFillUpMask", AutoCompleteFillUpDefault);
	autoCompletionConfig.fAutoInsertMask = IniSectionGetInt(pIniSection, L"AutoInsertMask", AutoInsertDefaultMask);
	iValue = IniSectionGetInt(pIniSection, L"AsmLineCommentChar", AsmLineCommentCharSemicolon);
	autoCompletionConfig.iAsmLineCommentChar = clamp_i(iValue, AsmLineCommentCharSemicolon, AsmLineCommentCharAt);
	strValue = IniSectionGetValue(pIniSection, L"AutoCFillUpPunctuation");
	if (StrIsEmpty(strValue)) {
		lstrcpy(autoCompletionConfig.wszAutoCompleteFillUp, AUTO_COMPLETION_FILLUP_DEFAULT);
	} else {
		lstrcpyn(autoCompletionConfig.wszAutoCompleteFillUp, strValue, COUNTOF(autoCompletionConfig.wszAutoCompleteFillUp));
	}
	EditCompleteUpdateConfig();

#if NP2_ENABLE_SHOW_CALL_TIPS
	bShowCallTips = IniSectionGetBool(pIniSection, L"ShowCallTips", 0);
	iValue = IniSectionGetInt(pIniSection, L"CallTipsWaitTime", 500);
	iCallTipsWaitTime = max_i(iValue, 100);
#endif

	bTabsAsSpaces = IniSectionGetBool(pIniSection, L"TabsAsSpaces", 0);
	bTabsAsSpacesG = bTabsAsSpaces;
	bTabIndents = IniSectionGetBool(pIniSection, L"TabIndents", 1);
	bTabIndentsG = bTabIndents;

	bBackspaceUnindents = IniSectionGetBool(pIniSection, L"BackspaceUnindents", 0);

	iValue = IniSectionGetInt(pIniSection, L"ZoomLevel", 100);
	// Added in v4.2.25.1172, stored as a relative font size in point, in range [-10, 20].
	iValue = (iSettingsVersion < NP2SettingsVersion_V1)? 100 : iValue;
	iZoomLevel = clamp_i(iValue, SC_MIN_ZOOM_LEVEL, SC_MAX_ZOOM_LEVEL);

	iValue = IniSectionGetInt(pIniSection, L"TabWidth", 4);
	iValue = clamp_i(iValue, 1, 256);
	iTabWidth = iValue;
	iTabWidthG = iValue;

	iValue = IniSectionGetInt(pIniSection, L"IndentWidth", 4);
	iValue = clamp_i(iValue, 0, 256);
	iIndentWidth = iValue;
	iIndentWidthG = iValue;

	bMarkLongLines = IniSectionGetBool(pIniSection, L"MarkLongLines", 1);

	iValue = IniSectionGetInt(pIniSection, L"LongLinesLimit", 80);
	iValue = clamp_i(iValue, 0, NP2_LONG_LINE_LIMIT);
	iLongLinesLimit = iValue;
	iLongLinesLimitG = iValue;

	iValue = IniSectionGetInt(pIniSection, L"LongLineMode", EDGE_LINE);
	iLongLineMode = clamp_i(iValue, EDGE_LINE, EDGE_BACKGROUND);

	bShowSelectionMargin = IniSectionGetBool(pIniSection, L"ShowSelectionMargin", 0);
	bShowLineNumbers = IniSectionGetBool(pIniSection, L"ShowLineNumbers", 1);
	bShowCodeFolding = IniSectionGetBool(pIniSection, L"ShowCodeFolding", 1);

	iValue = IniSectionGetInt(pIniSection, L"MarkOccurrences", 3);
	iMarkOccurrences = clamp_i(iValue, 0, 3);
	bMarkOccurrencesMatchCase = IniSectionGetBool(pIniSection, L"MarkOccurrencesMatchCase", 1);
	bMarkOccurrencesMatchWords = IniSectionGetBool(pIniSection, L"MarkOccurrencesMatchWholeWords", 0);

	bViewWhiteSpace = IniSectionGetBool(pIniSection, L"ViewWhiteSpace", 0);
	bViewEOLs = IniSectionGetBool(pIniSection, L"ViewEOLs", 0);

	iValue = Encoding_MapIniSetting(FALSE, CPI_UTF8);
	iValue = IniSectionGetInt(pIniSection, L"DefaultEncoding", iValue);
	iValue = Encoding_MapIniSetting(TRUE, iValue);
	if (!Encoding_IsValid(iValue)) {
		iValue = CPI_UTF8;
	}
	iDefaultEncoding = iValue;

	bSkipUnicodeDetection = IniSectionGetBool(pIniSection, L"SkipUnicodeDetection", 1);
	bLoadANSIasUTF8 = IniSectionGetBool(pIniSection, L"LoadANSIasUTF8", 0);
	bLoadNFOasOEM = IniSectionGetBool(pIniSection, L"LoadNFOasOEM", 1);
	bNoEncodingTags = IniSectionGetBool(pIniSection, L"NoEncodingTags", 0);

	iValue = IniSectionGetInt(pIniSection, L"DefaultEOLMode", 0);
	iDefaultEOLMode = clamp_i(iValue, SC_EOL_CRLF, SC_EOL_LF);

	bWarnLineEndings = IniSectionGetBool(pIniSection, L"WarnLineEndings", 1);
	bFixLineEndings = IniSectionGetBool(pIniSection, L"FixLineEndings", 1);
	bAutoStripBlanks = IniSectionGetBool(pIniSection, L"FixTrailingBlanks", 0);

	iValue = IniSectionGetInt(pIniSection, L"PrintHeader", 1);
	iPrintHeader = clamp_i(iValue, 0, 3);

	iValue = IniSectionGetInt(pIniSection, L"PrintFooter", 0);
	iPrintFooter = clamp_i(iValue, 0, 1);

	iValue = IniSectionGetInt(pIniSection, L"PrintColorMode", SC_PRINT_COLOURONWHITE);
	iPrintColor = clamp_i(iValue, SC_PRINT_NORMAL, SC_PRINT_SCREENCOLOURS);

	iValue = IniSectionGetInt(pIniSection, L"PrintZoom", 100);
	// previously stored as a relative font size in point plus 10, in range [-10, 20] + 10.
	iValue = (iSettingsVersion < NP2SettingsVersion_V1)? 100 : iValue;
	iPrintZoom = clamp_i(iValue, SC_MIN_ZOOM_LEVEL, SC_MAX_ZOOM_LEVEL);

	iValue = IniSectionGetInt(pIniSection, L"PrintMarginLeft", -1);
	pageSetupMargin.left = max_i(iValue, -1);

	iValue = IniSectionGetInt(pIniSection, L"PrintMarginTop", -1);
	pageSetupMargin.top = max_i(iValue, -1);

	iValue = IniSectionGetInt(pIniSection, L"PrintMarginRight", -1);
	pageSetupMargin.right = max_i(iValue, -1);

	iValue = IniSectionGetInt(pIniSection, L"PrintMarginBottom", -1);
	pageSetupMargin.bottom = max_i(iValue, -1);

	bSaveBeforeRunningTools = IniSectionGetBool(pIniSection, L"SaveBeforeRunningTools", 0);
	bOpenFolderWithMetapath = IniSectionGetBool(pIniSection, L"OpenFolderWithMetapath", 1);

	iValue = IniSectionGetInt(pIniSection, L"FileWatchingMode", 2);
	iFileWatchingMode = clamp_i(iValue, 0, 2);

	bResetFileWatching = IniSectionGetBool(pIniSection, L"ResetFileWatching", 0);

	iValue = IniSectionGetInt(pIniSection, L"EscFunction", 0);
	iEscFunction = clamp_i(iValue, 0, 2);

	bAlwaysOnTop = IniSectionGetBool(pIniSection, L"AlwaysOnTop", 0);
	bMinimizeToTray = IniSectionGetBool(pIniSection, L"MinimizeToTray", 0);
	bTransparentMode = IniSectionGetBool(pIniSection, L"TransparentMode", 0);
	bFindReplaceTransparentMode = IniSectionGetBool(pIniSection, L"FindReplaceTransparentMode", 1);
	bEditLayoutRTL = IniSectionGetBool(pIniSection, L"EditLayoutRTL", 0);
	bWindowLayoutRTL = IniSectionGetBool(pIniSection, L"WindowLayoutRTL", 0);

	iValue = IniSectionGetInt(pIniSection, L"RenderingTechnology", (IsVistaAndAbove()? SC_TECHNOLOGY_DIRECTWRITE : SC_TECHNOLOGY_DEFAULT));
	iValue = clamp_i(iValue, SC_TECHNOLOGY_DEFAULT, SC_TECHNOLOGY_DIRECTWRITEDC);
	iRenderingTechnology = iValue;
	bEditLayoutRTL = bEditLayoutRTL && iValue == SC_TECHNOLOGY_DEFAULT;

	iValue = IniSectionGetInt(pIniSection, L"Bidirectional", SC_BIDIRECTIONAL_DISABLED);
	iBidirectional = clamp_i(iValue, SC_BIDIRECTIONAL_DISABLED, SC_BIDIRECTIONAL_R2L);

	iValue = IniSectionGetInt(pIniSection, L"FontQuality", SC_EFF_QUALITY_LCD_OPTIMIZED);
	iFontQuality = clamp_i(iValue, SC_EFF_QUALITY_DEFAULT, SC_EFF_QUALITY_LCD_OPTIMIZED);

	iValue = IniSectionGetInt(pIniSection, L"CaretStyle", 1);
	iOvrCaretStyle = clamp_i(iValue / 10, 0, 1);
	iCaretStyle = clamp_i(iValue % 10, 0, 3);
	iCaretBlinkPeriod = IniSectionGetInt(pIniSection, L"CaretBlinkPeriod", -1);

	// Korean IME use inline mode (and block caret in inline mode) by default
	bUseInlineIME = IniSectionGetBool(pIniSection, L"UseInlineIME", -1);
	bInlineIMEUseBlockCaret = IniSectionGetBool(pIniSection, L"InlineIMEUseBlockCaret", 0);
	if (bUseInlineIME == -1) { // auto detection once
		// ScintillaWin::KoreanIME()
		const int codePage = Scintilla_InputCodePage();
		if (codePage == 949 || codePage == 1361) {
			bUseInlineIME = TRUE;
			bInlineIMEUseBlockCaret = TRUE;
		} else {
			bUseInlineIME = FALSE;
		}
	}

	strValue = IniSectionGetValue(pIniSection, L"ToolbarButtons");
	if (StrIsEmpty(strValue)) {
		CopyMemory(tchToolbarButtons, DefaultToolbarButtons, sizeof(DefaultToolbarButtons));
	} else {
		lstrcpyn(tchToolbarButtons, strValue, COUNTOF(tchToolbarButtons));
	}

	bShowToolbar = IniSectionGetBool(pIniSection, L"ShowToolbar", 1);
	bShowStatusbar = IniSectionGetBool(pIniSection, L"ShowStatusbar", 1);

	iValue = IniSectionGetInt(pIniSection, L"FullScreenMode", FullScreenMode_Default);
	iFullScreenMode = iValue;
	bInFullScreenMode = iValue & FullScreenMode_OnStartup;

	cxRunDlg = IniSectionGetInt(pIniSection, L"RunDlgSizeX", 0);
	cxEncodingDlg = IniSectionGetInt(pIniSection, L"EncodingDlgSizeX", 0);
	cyEncodingDlg = IniSectionGetInt(pIniSection, L"EncodingDlgSizeY", 0);
	cxRecodeDlg = IniSectionGetInt(pIniSection, L"RecodeDlgSizeX", 0);
	cyRecodeDlg = IniSectionGetInt(pIniSection, L"RecodeDlgSizeY", 0);
	cxFileMRUDlg = IniSectionGetInt(pIniSection, L"FileMRUDlgSizeX", 0);
	cyFileMRUDlg = IniSectionGetInt(pIniSection, L"FileMRUDlgSizeY", 0);
	cxOpenWithDlg = IniSectionGetInt(pIniSection, L"OpenWithDlgSizeX", 0);
	cyOpenWithDlg = IniSectionGetInt(pIniSection, L"OpenWithDlgSizeY", 0);
	cxFavoritesDlg = IniSectionGetInt(pIniSection, L"FavoritesDlgSizeX", 0);
	cyFavoritesDlg = IniSectionGetInt(pIniSection, L"FavoritesDlgSizeY", 0);
	cxAddFavoritesDlg = IniSectionGetInt(pIniSection, L"AddFavoritesDlgSizeX", 0);
	cxModifyLinesDlg = IniSectionGetInt(pIniSection, L"ModifyLinesDlgSizeX", 0);
	cyModifyLinesDlg = IniSectionGetInt(pIniSection, L"ModifyLinesDlgSizeY", 0);
	cxEncloseSelectionDlg = IniSectionGetInt(pIniSection, L"EncloseSelectionDlgSizeX", 0);
	cyEncloseSelectionDlg = IniSectionGetInt(pIniSection, L"EncloseSelectionDlgSizeY", 0);
	cxInsertTagDlg = IniSectionGetInt(pIniSection, L"InsertTagDlgSizeX", 0);
	cyInsertTagDlg = IniSectionGetInt(pIniSection, L"InsertTagDlgSizeY", 0);
	xFindReplaceDlg = IniSectionGetInt(pIniSection, L"FindReplaceDlgPosX", 0);
	yFindReplaceDlg = IniSectionGetInt(pIniSection, L"FindReplaceDlgPosY", 0);
	cxFindReplaceDlg = IniSectionGetInt(pIniSection, L"FindReplaceDlgSizeX", 0);

	if (bSaveFindReplace) {
		efrData.fuFlags = 0;
		if (IniSectionGetBool(pIniSection, L"FindReplaceMatchCase", 0)) {
			efrData.fuFlags |= SCFIND_MATCHCASE;
		}
		if (IniSectionGetBool(pIniSection, L"FindReplaceMatchWholeWorldOnly", 0)) {
			efrData.fuFlags |= SCFIND_WHOLEWORD;
		}
		if (IniSectionGetBool(pIniSection, L"FindReplaceMatchBeginingWordOnly", 0)) {
			efrData.fuFlags |= SCFIND_WORDSTART;
		}
		if (IniSectionGetBool(pIniSection, L"FindReplaceRegExpSearch", 0)) {
			efrData.fuFlags |= SCFIND_REGEXP | SCFIND_POSIX;
		}
		efrData.bTransformBS = IniSectionGetBool(pIniSection, L"FindReplaceTransformBackslash", 0);
		efrData.bWildcardSearch = IniSectionGetBool(pIniSection, L"FindReplaceWildcardSearch", 0);
	}

	LoadIniSection(INI_SECTION_NAME_TOOLBAR_IMAGES, pIniSectionBuf, cchIniSection);
	IniSectionParse(pIniSection, pIniSectionBuf);

	strValue = IniSectionGetValue(pIniSection, L"BitmapDefault");
	if (StrNotEmpty(strValue)) {
		tchToolbarBitmap = StrDup(strValue);
	}
	strValue = IniSectionGetValue(pIniSection, L"BitmapHot");
	if (StrNotEmpty(strValue)) {
		tchToolbarBitmapHot = StrDup(strValue);
	}
	strValue = IniSectionGetValue(pIniSection, L"BitmapDisabled");
	if (StrNotEmpty(strValue)) {
		tchToolbarBitmapDisabled = StrDup(strValue);
	}

	if (!flagPosParam /*|| bStickyWinPos*/) { // ignore window position if /p was specified
		WCHAR tchPosX[32];
		WCHAR tchPosY[32];
		WCHAR tchSizeX[32];
		WCHAR tchSizeY[32];
		WCHAR tchMaximized[32];
		const int ResX = GetSystemMetrics(SM_CXSCREEN);
		const int ResY = GetSystemMetrics(SM_CYSCREEN);

		const int len = wsprintf(tchPosX, L"%ix%i ", ResX, ResY);
		lstrcpy(tchPosY, tchPosX);
		lstrcpy(tchSizeX, tchPosX);
		lstrcpy(tchSizeY, tchPosX);
		lstrcpy(tchMaximized, tchPosX);
		lstrcpy(tchPosX + len, L"PosX");
		lstrcpy(tchPosY + len, L"PosY");
		lstrcpy(tchSizeX + len, L"SizeX");
		lstrcpy(tchSizeY + len, L"SizeY");
		lstrcpy(tchMaximized + len, L"Maximized");

		LoadIniSection(INI_SECTION_NAME_WINDOW_POSITION, pIniSectionBuf, cchIniSection);
		IniSectionParse(pIniSection, pIniSectionBuf);

		wi.x	= IniSectionGetIntEx(pIniSection, tchPosX, CW_USEDEFAULT);
		wi.y	= IniSectionGetIntEx(pIniSection, tchPosY, CW_USEDEFAULT);
		wi.cx	= IniSectionGetIntEx(pIniSection, tchSizeX, CW_USEDEFAULT);
		wi.cy	= IniSectionGetIntEx(pIniSection, tchSizeY, CW_USEDEFAULT);
		wi.max	= IniSectionGetBoolEx(pIniSection, tchMaximized, 0);
	}

	IniSectionFree(pIniSection);
	NP2HeapFree(pIniSectionBuf);

	iDefaultCodePage = 0;
	{
		const UINT acp = GetACP();
		if (IsDBCSCodePage(acp) || acp == CP_UTF8) {
			iDefaultCodePage = acp;
		}
	}

	{
		CHARSETINFO ci;
		if (TranslateCharsetInfo((DWORD *)(UINT_PTR)iDefaultCodePage, &ci, TCI_SRCCODEPAGE)) {
			iDefaultCharSet = ci.ciCharset;
		} else {
			iDefaultCharSet = ANSI_CHARSET;
		}
	}

	// Scintilla Styles
	Style_Load();
}

//=============================================================================
//
// SaveSettings()
//
//
void SaveSettings(BOOL bSaveSettingsNow) {
	if (StrIsEmpty(szIniFile)) {
		return;
	}

	CreateIniFile();

	if (!bSaveSettings && !bSaveSettingsNow) {
		IniSetBool(INI_SECTION_NAME_SETTINGS, L"SaveSettings", bSaveSettings);
		return;
	}

	WCHAR wchTmp[MAX_PATH];
	IniSectionOnSave section;
	WCHAR *pIniSectionBuf = (WCHAR *)NP2HeapAlloc(sizeof(WCHAR) * MAX_INI_SECTION_SIZE_SETTINGS);
	IniSectionOnSave *pIniSection = &section;
	pIniSection->next = pIniSectionBuf;

	IniSectionSetInt(pIniSection, L"SettingsVersion", NP2SettingsVersion_Current);
	IniSectionSetBool(pIniSection, L"SaveSettings", bSaveSettings);
	IniSectionSetBoolEx(pIniSection, L"SaveRecentFiles", bSaveRecentFiles, 0);
	IniSectionSetBoolEx(pIniSection, L"SaveFindReplace", bSaveFindReplace, 0);
	IniSectionSetBoolEx(pIniSection, L"CloseFind", efrData.bFindClose, 0);
	IniSectionSetBoolEx(pIniSection, L"CloseReplace", efrData.bReplaceClose, 0);
	IniSectionSetBoolEx(pIniSection, L"NoFindWrap", efrData.bNoFindWrap, 0);
	PathRelativeToApp(tchOpenWithDir, wchTmp, COUNTOF(wchTmp), FALSE, TRUE, flagPortableMyDocs);
	IniSectionSetString(pIniSection, L"OpenWithDir", wchTmp);
	PathRelativeToApp(tchFavoritesDir, wchTmp, COUNTOF(wchTmp), FALSE, TRUE, flagPortableMyDocs);
	IniSectionSetString(pIniSection, L"Favorites", wchTmp);
	IniSectionSetIntEx(pIniSection, L"PathNameFormat", iPathNameFormat, 1);
	IniSectionSetBoolEx(pIniSection, L"WordWrap", fWordWrapG, 1);
	IniSectionSetIntEx(pIniSection, L"WordWrapMode", iWordWrapMode, SC_WRAP_WORD);
	IniSectionSetIntEx(pIniSection, L"WordWrapIndent", iWordWrapIndent, EditWrapIndentNone);
	IniSectionSetIntEx(pIniSection, L"WordWrapSymbols", iWordWrapSymbols, EditWrapSymbolDefaultValue);
	IniSectionSetBoolEx(pIniSection, L"ShowWordWrapSymbols", bShowWordWrapSymbols, 0);
	IniSectionSetBoolEx(pIniSection, L"WordWrapSelectSubLine", bWordWrapSelectSubLine, 0);
	IniSectionSetBoolEx(pIniSection, L"ShowUnicodeControlCharacter", bShowUnicodeControlCharacter, 0);
	IniSectionSetBoolEx(pIniSection, L"MatchBraces", bMatchBraces, 1);
	IniSectionSetIntEx(pIniSection, L"HighlightCurrentLine", iHighlightCurrentLine + (bHighlightCurrentSubLine ? 10 : 0), 12);
	IniSectionSetBoolEx(pIniSection, L"ShowIndentGuides", bShowIndentGuides, 0);

	IniSectionSetBoolEx(pIniSection, L"AutoIndent", autoCompletionConfig.bIndentText, 1);
	IniSectionSetBoolEx(pIniSection, L"AutoCloseTags", autoCompletionConfig.bCloseTags, 1);
	IniSectionSetBoolEx(pIniSection, L"AutoCompleteWords", autoCompletionConfig.bCompleteWord, 1);
	IniSectionSetBoolEx(pIniSection, L"AutoCScanWordsInDocument", autoCompletionConfig.bScanWordsInDocument, 1);
	IniSectionSetBoolEx(pIniSection, L"AutoCEnglishIMEModeOnly", autoCompletionConfig.bEnglistIMEModeOnly, 0);
	IniSectionSetIntEx(pIniSection, L"AutoCVisibleItemCount", autoCompletionConfig.iVisibleItemCount, 16);
	IniSectionSetIntEx(pIniSection, L"AutoCMinWordLength", autoCompletionConfig.iMinWordLength, 1);
	IniSectionSetIntEx(pIniSection, L"AutoCMinNumberLength", autoCompletionConfig.iMinNumberLength, 3);
	IniSectionSetIntEx(pIniSection, L"AutoCFillUpMask", autoCompletionConfig.fAutoCompleteFillUpMask, AutoCompleteFillUpDefault);
	IniSectionSetIntEx(pIniSection, L"AutoInsertMask", autoCompletionConfig.fAutoInsertMask, AutoInsertDefaultMask);
	IniSectionSetIntEx(pIniSection, L"AsmLineCommentChar", autoCompletionConfig.iAsmLineCommentChar, AsmLineCommentCharSemicolon);
	IniSectionSetStringEx(pIniSection, L"AutoCFillUpPunctuation", autoCompletionConfig.wszAutoCompleteFillUp, AUTO_COMPLETION_FILLUP_DEFAULT);
#if NP2_ENABLE_SHOW_CALL_TIPS
	IniSectionSetBoolEx(pIniSection, L"ShowCallTips", bShowCallTips, 1);
	IniSectionSetIntEx(pIniSection, L"CallTipsWaitTime", iCallTipsWaitTime, 500);
#endif
	IniSectionSetBoolEx(pIniSection, L"TabsAsSpaces", bTabsAsSpacesG, 0);
	IniSectionSetBoolEx(pIniSection, L"TabIndents", bTabIndentsG, 1);
	IniSectionSetBoolEx(pIniSection, L"BackspaceUnindents", bBackspaceUnindents, 0);
	IniSectionSetIntEx(pIniSection, L"ZoomLevel", iZoomLevel, 100);
	IniSectionSetIntEx(pIniSection, L"TabWidth", iTabWidthG, 4);
	IniSectionSetIntEx(pIniSection, L"IndentWidth", iIndentWidthG, 4);
	IniSectionSetBoolEx(pIniSection, L"MarkLongLines", bMarkLongLines, 1);
	IniSectionSetIntEx(pIniSection, L"LongLinesLimit", iLongLinesLimitG, 80);
	IniSectionSetIntEx(pIniSection, L"LongLineMode", iLongLineMode, EDGE_LINE);
	IniSectionSetBoolEx(pIniSection, L"ShowSelectionMargin", bShowSelectionMargin, 0);
	IniSectionSetBoolEx(pIniSection, L"ShowLineNumbers", bShowLineNumbers, 1);
	IniSectionSetBoolEx(pIniSection, L"ShowCodeFolding", bShowCodeFolding, 1);
	IniSectionSetIntEx(pIniSection, L"MarkOccurrences", iMarkOccurrences, 3);
	IniSectionSetBoolEx(pIniSection, L"MarkOccurrencesMatchCase", bMarkOccurrencesMatchCase, 1);
	IniSectionSetBoolEx(pIniSection, L"MarkOccurrencesMatchWholeWords", bMarkOccurrencesMatchWords, 0);
	IniSectionSetBoolEx(pIniSection, L"ViewWhiteSpace", bViewWhiteSpace, 0);
	IniSectionSetBoolEx(pIniSection, L"ViewEOLs", bViewEOLs, 0);
	IniSectionSetIntEx(pIniSection, L"DefaultEncoding", Encoding_MapIniSetting(FALSE, iDefaultEncoding), Encoding_MapIniSetting(FALSE, CPI_UTF8));
	IniSectionSetBoolEx(pIniSection, L"SkipUnicodeDetection", bSkipUnicodeDetection, 1);
	IniSectionSetBoolEx(pIniSection, L"LoadANSIasUTF8", bLoadANSIasUTF8, 0);
	IniSectionSetBoolEx(pIniSection, L"LoadNFOasOEM", bLoadNFOasOEM, 1);
	IniSectionSetBoolEx(pIniSection, L"NoEncodingTags", bNoEncodingTags, 0);
	IniSectionSetIntEx(pIniSection, L"DefaultEOLMode", iDefaultEOLMode, 0);
	IniSectionSetIntEx(pIniSection, L"WarnLineEndings", bWarnLineEndings, 1);
	IniSectionSetBoolEx(pIniSection, L"FixLineEndings", bFixLineEndings, 1);
	IniSectionSetBoolEx(pIniSection, L"FixTrailingBlanks", bAutoStripBlanks, 0);

	IniSectionSetIntEx(pIniSection, L"PrintHeader", iPrintHeader, 1);
	IniSectionSetIntEx(pIniSection, L"PrintFooter", iPrintFooter, 0);
	IniSectionSetIntEx(pIniSection, L"PrintColorMode", iPrintColor, SC_PRINT_COLOURONWHITE);
	IniSectionSetIntEx(pIniSection, L"PrintZoom", iPrintZoom, 100);
	IniSectionSetIntEx(pIniSection, L"PrintMarginLeft", pageSetupMargin.left, -1);
	IniSectionSetIntEx(pIniSection, L"PrintMarginTop", pageSetupMargin.top, -1);
	IniSectionSetIntEx(pIniSection, L"PrintMarginRight", pageSetupMargin.right, -1);
	IniSectionSetIntEx(pIniSection, L"PrintMarginBottom", pageSetupMargin.bottom, -1);
	IniSectionSetBoolEx(pIniSection, L"SaveBeforeRunningTools", bSaveBeforeRunningTools, 0);
	IniSectionSetBoolEx(pIniSection, L"OpenFolderWithMetapath", bOpenFolderWithMetapath, 1);
	IniSectionSetIntEx(pIniSection, L"FileWatchingMode", iFileWatchingMode, 2);
	IniSectionSetBoolEx(pIniSection, L"ResetFileWatching", bResetFileWatching, 0);
	IniSectionSetIntEx(pIniSection, L"EscFunction", iEscFunction, 0);
	IniSectionSetBoolEx(pIniSection, L"AlwaysOnTop", bAlwaysOnTop, 0);
	IniSectionSetBoolEx(pIniSection, L"MinimizeToTray", bMinimizeToTray, 0);
	IniSectionSetBoolEx(pIniSection, L"TransparentMode", bTransparentMode, 0);
	IniSectionSetBoolEx(pIniSection, L"FindReplaceTransparentMode", bFindReplaceTransparentMode, 1);
	IniSectionSetBoolEx(pIniSection, L"EditLayoutRTL", bEditLayoutRTL, 0);
	IniSectionSetBoolEx(pIniSection, L"WindowLayoutRTL", bWindowLayoutRTL, 0);
	IniSectionSetIntEx(pIniSection, L"RenderingTechnology", iRenderingTechnology, (IsVistaAndAbove()? SC_TECHNOLOGY_DIRECTWRITE : SC_TECHNOLOGY_DEFAULT));
	IniSectionSetIntEx(pIniSection, L"Bidirectional", iBidirectional, SC_BIDIRECTIONAL_DISABLED);
	IniSectionSetIntEx(pIniSection, L"FontQuality", iFontQuality, SC_EFF_QUALITY_LCD_OPTIMIZED);
	IniSectionSetIntEx(pIniSection, L"CaretStyle", iCaretStyle + iOvrCaretStyle*10, 1);
	IniSectionSetIntEx(pIniSection, L"CaretBlinkPeriod", iCaretBlinkPeriod, -1);
	IniSectionSetBool(pIniSection, L"UseInlineIME", bUseInlineIME); // keep result of auto detection
	IniSectionSetBoolEx(pIniSection, L"InlineIMEUseBlockCaret", bInlineIMEUseBlockCaret, 0);
	Toolbar_GetButtons(hwndToolbar, TOOLBAR_COMMAND_BASE, tchToolbarButtons, COUNTOF(tchToolbarButtons));
	IniSectionSetStringEx(pIniSection, L"ToolbarButtons", tchToolbarButtons, DefaultToolbarButtons);
	IniSectionSetBoolEx(pIniSection, L"ShowToolbar", bShowToolbar, 1);
	IniSectionSetBoolEx(pIniSection, L"ShowStatusbar", bShowStatusbar, 1);
	IniSectionSetIntEx(pIniSection, L"FullScreenMode", iFullScreenMode, FullScreenMode_Default);
	IniSectionSetInt(pIniSection, L"RunDlgSizeX", cxRunDlg);
	IniSectionSetInt(pIniSection, L"EncodingDlgSizeX", cxEncodingDlg);
	IniSectionSetInt(pIniSection, L"EncodingDlgSizeY", cyEncodingDlg);
	IniSectionSetInt(pIniSection, L"RecodeDlgSizeX", cxRecodeDlg);
	IniSectionSetInt(pIniSection, L"RecodeDlgSizeY", cyRecodeDlg);
	IniSectionSetInt(pIniSection, L"FileMRUDlgSizeX", cxFileMRUDlg);
	IniSectionSetInt(pIniSection, L"FileMRUDlgSizeY", cyFileMRUDlg);
	IniSectionSetInt(pIniSection, L"OpenWithDlgSizeX", cxOpenWithDlg);
	IniSectionSetInt(pIniSection, L"OpenWithDlgSizeY", cyOpenWithDlg);
	IniSectionSetInt(pIniSection, L"FavoritesDlgSizeX", cxFavoritesDlg);
	IniSectionSetInt(pIniSection, L"FavoritesDlgSizeY", cyFavoritesDlg);
	IniSectionSetInt(pIniSection, L"AddFavoritesDlgSizeX", cxAddFavoritesDlg);
	IniSectionSetInt(pIniSection, L"ModifyLinesDlgSizeX", cxModifyLinesDlg);
	IniSectionSetInt(pIniSection, L"ModifyLinesDlgSizeY", cyModifyLinesDlg);
	IniSectionSetInt(pIniSection, L"EncloseSelectionDlgSizeX", cxEncloseSelectionDlg);
	IniSectionSetInt(pIniSection, L"EncloseSelectionDlgSizeY", cyEncloseSelectionDlg);
	IniSectionSetInt(pIniSection, L"InsertTagDlgSizeX", cxInsertTagDlg);
	IniSectionSetInt(pIniSection, L"InsertTagDlgSizeY", cyInsertTagDlg);
	IniSectionSetInt(pIniSection, L"FindReplaceDlgPosX", xFindReplaceDlg);
	IniSectionSetInt(pIniSection, L"FindReplaceDlgPosY", yFindReplaceDlg);
	IniSectionSetInt(pIniSection, L"FindReplaceDlgSizeX", cxFindReplaceDlg);

	if (bSaveFindReplace) {
		IniSectionSetBoolEx(pIniSection, L"FindReplaceMatchCase", (efrData.fuFlags & SCFIND_MATCHCASE), 0);
		IniSectionSetBoolEx(pIniSection, L"FindReplaceMatchWholeWorldOnly", (efrData.fuFlags & SCFIND_WHOLEWORD), 0);
		IniSectionSetBoolEx(pIniSection, L"FindReplaceMatchBeginingWordOnly", (efrData.fuFlags & SCFIND_WORDSTART), 0);
		IniSectionSetBoolEx(pIniSection, L"FindReplaceRegExpSearch", (efrData.fuFlags & (SCFIND_REGEXP | SCFIND_POSIX)), 0);
		IniSectionSetBoolEx(pIniSection, L"FindReplaceTransformBackslash", efrData.bTransformBS, 0);
		IniSectionSetBoolEx(pIniSection, L"FindReplaceWildcardSearch", efrData.bWildcardSearch, 0);
	}

	SaveIniSection(INI_SECTION_NAME_SETTINGS, pIniSectionBuf);
	NP2HeapFree(pIniSectionBuf);

	// SaveSettingsNow(): query Window Dimensions
	if (bSaveSettingsNow) {
		WINDOWPLACEMENT wndpl;
		wndpl.length = sizeof(WINDOWPLACEMENT);
		GetWindowPlacement(hwndMain, &wndpl);

		wi.x = wndpl.rcNormalPosition.left;
		wi.y = wndpl.rcNormalPosition.top;
		wi.cx = wndpl.rcNormalPosition.right - wndpl.rcNormalPosition.left;
		wi.cy = wndpl.rcNormalPosition.bottom - wndpl.rcNormalPosition.top;
		wi.max = (IsZoomed(hwndMain) || (wndpl.flags & WPF_RESTORETOMAXIMIZED));
	}

	if (!bStickyWinPos) {
		SaveWindowPosition();
	}

	// Scintilla Styles
	Style_Save();
}

void SaveWindowPosition(void) {
	WCHAR tchPosX[32];
	WCHAR tchPosY[32];
	WCHAR tchSizeX[32];
	WCHAR tchSizeY[32];
	WCHAR tchMaximized[32];
	const int ResX = GetSystemMetrics(SM_CXSCREEN);
	const int ResY = GetSystemMetrics(SM_CYSCREEN);

	const int len = wsprintf(tchPosX, L"%ix%i ", ResX, ResY);
	lstrcpy(tchPosY, tchPosX);
	lstrcpy(tchSizeX, tchPosX);
	lstrcpy(tchSizeY, tchPosX);
	lstrcpy(tchMaximized, tchPosX);
	lstrcpy(tchPosX + len, L"PosX");
	lstrcpy(tchPosY + len, L"PosY");
	lstrcpy(tchSizeX + len, L"SizeX");
	lstrcpy(tchSizeY + len, L"SizeY");
	lstrcpy(tchMaximized + len, L"Maximized");

	IniSetInt(INI_SECTION_NAME_WINDOW_POSITION, tchPosX, wi.x);
	IniSetInt(INI_SECTION_NAME_WINDOW_POSITION, tchPosY, wi.y);
	IniSetInt(INI_SECTION_NAME_WINDOW_POSITION, tchSizeX, wi.cx);
	IniSetInt(INI_SECTION_NAME_WINDOW_POSITION, tchSizeY, wi.cy);
	IniSetBool(INI_SECTION_NAME_WINDOW_POSITION, tchMaximized, wi.max);
}

//=============================================================================
//
// ParseCommandLine()
//
//
int ParseCommandLineEncoding(LPCWSTR opt, int idmLE, int idmBE) {
	int flag = idmLE;
	if (*opt == '-') {
		++opt;
	}
	if (StrNCaseEqual(opt, L"LE", CSTRLEN(L"LE"))){
		flag = idmLE;
		opt += CSTRLEN(L"LE");
		if (*opt == '-') {
			++opt;
		}
	} else if (StrNCaseEqual(opt, L"BE", CSTRLEN(L"BE"))) {
		flag = idmBE;
		opt += CSTRLEN(L"BE");
		if (*opt == '-') {
			++opt;
		}
	}
	if (*opt == 0 || StrCaseEqual(opt, L"BOM") || StrCaseEqual(opt, L"SIG") || StrCaseEqual(opt, L"SIGNATURE")) {
		flagSetEncoding = flag - IDM_ENCODING_ANSI + 1;
		return 1;
	}
	return 0;
}

int ParseCommandLineOption(LPWSTR lp1, LPWSTR lp2, BOOL *bIsNotepadReplacement) {
	LPWSTR opt = lp1 + 1;
	// only accept /opt, -opt, --opt
	if (*opt == L'-') {
		++opt;
	}
	if (*opt == 0) {
		return 0;
	}

	int state = 0;
	if (opt[1] == 0) {
		switch (*CharUpper(opt)) {
		case L'A':
			flagSetEncoding = IDM_ENCODING_ANSI - IDM_ENCODING_ANSI + 1;
			state = 1;
			break;

		case L'B':
			flagPasteBoard = 1;
			state = 1;
			break;

		case L'C':
			flagNewFromClipboard = 1;
			state = 1;
			break;

		case L'D':
			if (lpSchemeArg) {
				LocalFree(lpSchemeArg);
				lpSchemeArg = NULL;
			}
			iInitialLexer = Style_GetEditLexerId(EditLexer_Default);
			flagLexerSpecified = 1;
			state = 1;
			break;

		case L'E':
			state = 2;
			if (ExtractFirstArgument(lp2, lp1, lp2)) {
				if (lpEncodingArg) {
					LocalFree(lpEncodingArg);
				}
				lpEncodingArg = StrDup(lp1);
				state = 1;
			}
			break;

		case L'F':
			state = 2;
			if (ExtractFirstArgument(lp2, lp1, lp2)) {
				lstrcpyn(szIniFile, lp1, COUNTOF(szIniFile));
				TrimString(szIniFile);
				PathUnquoteSpaces(szIniFile);
				state = 1;
			}
			break;

		case L'G':
			state = 2;
			if (ExtractFirstArgument(lp2, lp1, lp2)) {
				int cord[2] = { 0 };
				const int itok = ParseCommaList(lp1, cord, COUNTOF(cord));
				if (itok != 0) {
					flagJumpTo = 1;
					state = 1;
					iInitialLine = cord[0];
					iInitialColumn = cord[1];
				}
			}
			break;

		case L'H':
			if (lpSchemeArg) {
				LocalFree(lpSchemeArg);
				lpSchemeArg = NULL;
			}
			iInitialLexer = Style_GetEditLexerId(EditLexer_HTML);
			flagLexerSpecified = 1;
			state = 1;
			break;

		case L'I':
			flagStartAsTrayIcon = 1;
			state = 1;
			break;

		case L'L':
			flagChangeNotify = 2;
			state = 1;
			break;

		case L'N':
			flagReuseWindow = 0;
			flagNoReuseWindow = 1;
			flagSingleFileInstance = 0;
			state = 1;
			break;

		case L'O':
			flagAlwaysOnTop = 2;
			state = 1;
			break;

		case L'Q':
			flagQuietCreate = 1;
			state = 1;
			break;

		case L'R':
			flagReuseWindow = 1;
			flagNoReuseWindow = 0;
			flagSingleFileInstance = 0;
			state = 1;
			break;

		case L'S':
			state = 2;
			if (ExtractFirstArgument(lp2, lp1, lp2)) {
				if (lpSchemeArg) {
					LocalFree(lpSchemeArg);
				}
				lpSchemeArg = StrDup(lp1);
				flagLexerSpecified = 1;
				state = 1;
			}
			break;

		case L'T':
			state = 2;
			if (ExtractFirstArgument(lp2, lp1, lp2)) {
				lstrcpyn(szTitleExcerpt, lp1, COUNTOF(szTitleExcerpt));
				fKeepTitleExcerpt = 1;
				state = 1;
			}
			break;

		case L'U':
			flagRelaunchElevated = 1;
			state = 1;
			break;

		case L'W':
			flagSetEncoding = IDM_ENCODING_UNICODE - IDM_ENCODING_ANSI + 1;
			state = 1;
			break;

		case L'X':
			if (lpSchemeArg) {
				LocalFree(lpSchemeArg);
				lpSchemeArg = NULL;
			}
			iInitialLexer = Style_GetEditLexerId(EditLexer_XML);
			flagLexerSpecified = 1;
			state = 1;
			break;

		case L'Z':
			ExtractFirstArgument(lp2, lp1, lp2);
			flagMultiFileArg = 1;
			*bIsNotepadReplacement = TRUE;
			state = 1;
			break;

		case L'?':
			flagDisplayHelp = 1;
			state = 1;
			break;

		default:
			state = 3;
			break;
		}
	} else if (opt[2] == 0) {
		switch (*CharUpper(opt)) {
		case L'C':
			if (opt[1] == L'R') {
				flagSetEOLMode = IDM_LINEENDINGS_CR - IDM_LINEENDINGS_CRLF + 1;
				state = 1;
			}
			break;

		case L'F':
			if (opt[1] == L'0' || *CharUpper(opt + 1) == L'O') {
				lstrcpy(szIniFile, L"*?");
				state = 1;
			}
			break;

		case L'L':
			if (opt[1] == L'F') {
				flagSetEOLMode = IDM_LINEENDINGS_LF - IDM_LINEENDINGS_CRLF + 1;
				state = 1;
			} else if (opt[1] == L'0' || opt[1] == L'-' || *CharUpper(opt + 1) == L'O') {
				flagChangeNotify = 1;
				state = 1;
			}
			break;

		case L'N':
			if (*CharUpper(opt + 1) == L'S') {
				flagReuseWindow = 0;
				flagNoReuseWindow = 1;
				flagSingleFileInstance = 1;
				state = 1;
			}
			break;

		case L'O':
			if (opt[1] == L'0' || opt[1] == L'-' || *CharUpper(opt + 1) == L'O') {
				flagAlwaysOnTop = 1;
				state = 1;
			}
			break;

		case L'R':
			if (*CharUpper(opt + 1) == L'S') {
				flagReuseWindow = 1;
				flagNoReuseWindow = 0;
				flagSingleFileInstance = 1;
				state = 1;
			}
			break;

		default:
			state = 3;
			break;
		}
	} else {
		state = 3;
	}

	if (state != 3) {
		return state;
	}

	state = 0;
	switch (*CharUpper(opt)) {
	case L'A':
		if (StrCaseEqual(opt, L"ANSI")) {
			flagSetEncoding = IDM_ENCODING_ANSI - IDM_ENCODING_ANSI + 1;
			state = 1;
		} else if (StrNCaseEqual(opt, L"appid=", CSTRLEN(L"appid="))) {
			// Shell integration
			opt += CSTRLEN(L"appid=");
			lstrcpyn(g_wchAppUserModelID, opt, COUNTOF(g_wchAppUserModelID));
			StrTrim(g_wchAppUserModelID, L"\" ");
			if (StrIsEmpty(g_wchAppUserModelID)) {
				lstrcpy(g_wchAppUserModelID, L"(default)");
			}
			state = 1;
		}
		break;

	case L'C':
		if (opt[1] == L'R') {
			opt += 2;
			if (*opt == L'-') {
				++opt;
			}
			if (*opt == L'L' && opt[1] == L'F' && opt[2] == 0) {
				flagSetEOLMode = IDM_LINEENDINGS_CRLF - IDM_LINEENDINGS_CRLF + 1;
				state = 1;
			}
		}
		break;

	case L'M':
		if (StrCaseEqual(opt, L"MBCS")) {
			flagSetEncoding = IDM_ENCODING_ANSI - IDM_ENCODING_ANSI + 1;
			state = 1;
		} else {
			BOOL bFindUp = FALSE;
			BOOL bRegex = FALSE;
			BOOL bTransBS = FALSE;

			++opt;
			switch (*CharUpper(opt)) {
			case L'R':
				bRegex = TRUE;
				++opt;
				break;

			case L'B':
				bTransBS = TRUE;
				++opt;
				break;
			}

			if (*opt == L'-') {
				bFindUp = TRUE;
				++opt;
			}
			if (*opt != 0) {
				break;
			}

			state = 2;
			if (ExtractFirstArgument(lp2, lp1, lp2)) {
				if (lpMatchArg) {
					LocalFree(lpMatchArg);
				}

				lpMatchArg = StrDup(lp1);
				flagMatchText = 1;
				state = 1;

				if (bFindUp) {
					flagMatchText |= 2;
				}
				if (bRegex) {
					flagMatchText &= ~8;
					flagMatchText |= 4;
				}
				if (bTransBS) {
					flagMatchText &= ~4;
					flagMatchText |= 8;
				}
			}
		}
		break;

	case L'P': {
		if (*bIsNotepadReplacement) {
			if (*CharUpper(opt + 1) == L'T') {
				ExtractFirstArgument(lp2, lp1, lp2);
			}
			state = 1;
			break;
		}

		if (StrNCaseEqual(opt, L"POS", CSTRLEN(L"POS"))) {
			opt += CSTRLEN(L"POS");
		} else {
			++opt;
		}
		if (*opt == L':') {
			++opt;
		}

		switch (*CharUpper(opt)) {
		case L'0':
		case L'O':
			if (opt[1] == 0) {
				flagPosParam = 1;
				flagDefaultPos = 1;
				state = 1;
			}
			break;

		case L'D':
		case L'S':
			if (opt[1] == 0 || (opt[2] == 0 && *CharUpper(opt + 1) == L'L')) {
				flagPosParam = 1;
				flagDefaultPos = (opt[1] == 0)? 2 : 3;;
				state = 1;
			}
			break;

		case L'F':
		case L'L':
		case L'R':
		case L'T':
		case L'B':
		case L'M': {
			WCHAR *p = opt;
			flagPosParam = 1;
			flagDefaultPos = 0;
			state = 1;
			while (*p && state == 1) {
				switch (*CharUpper(p)) {
				case L'F':
					flagDefaultPos &= ~(4 | 8 | 16 | 32);
					flagDefaultPos |= 64;
					break;

				case L'L':
					flagDefaultPos &= ~(8 | 64);
					flagDefaultPos |= 4;
					break;

				case L'R':
					flagDefaultPos &= ~(4 | 64);
					flagDefaultPos |= 8;
					break;

				case L'T':
					flagDefaultPos &= ~(32 | 64);
					flagDefaultPos |= 16;
					break;

				case L'B':
					flagDefaultPos &= ~(16 | 64);
					flagDefaultPos |= 32;
					break;

				case L'M':
					if (flagDefaultPos == 0) {
						flagDefaultPos |= 64;
					}
					flagDefaultPos |= 128;
					break;

				default:
					state = 0;
					break;
				}
				p = CharNext(p);
			}
		}
		break;

		default:
			state = 2;
			if (ExtractFirstArgument(lp2, lp1, lp2)) {
				int cord[5] = { 0 };
				const int itok = ParseCommaList(lp1, cord, COUNTOF(cord));
				if (itok >= 4) {
					flagPosParam = 1;
					flagDefaultPos = 0;
					state = 1;
					wi.x = cord[0];
					wi.y = cord[1];
					wi.cx = cord[2];
					wi.cy = cord[3];
					wi.max = cord[4] != 0;
					if (wi.cx < 1) {
						wi.cx = CW_USEDEFAULT;
					}
					if (wi.cy < 1) {
						wi.cy = CW_USEDEFAULT;
					}
					if (itok == 4) {
						wi.max = 0;
					}
				}
			}
			break;
		}
	}
	break;

	case L'S':
		// Shell integration
		if (StrNCaseEqual(opt, L"sysmru=", CSTRLEN(L"sysmru="))) {
			opt += CSTRLEN(L"sysmru=");
			if (opt[1] == 0) {
				switch (*opt) {
				case L'0':
					flagUseSystemMRU = 1;
					state = 1;
					break;

				case L'1':
					flagUseSystemMRU = 2;
					state = 1;
					break;
				}
			}
		}
		break;

	case L'U':
		if (StrNCaseEqual(opt, L"UTF", CSTRLEN(L"UTF"))) {
			opt += CSTRLEN(L"UTF");
			if (*opt == '-') {
				++opt;
			}
			if (*opt == L'8') {
				++opt;
				if (*opt == '-') {
					++opt;
				}
				if (*opt == 0) {
					flagSetEncoding = IDM_ENCODING_UTF8 - IDM_ENCODING_ANSI + 1;
					state = 1;
				} else if (StrCaseEqual(opt, L"BOM") || StrCaseEqual(opt, L"SIG") || StrCaseEqual(opt, L"SIGNATURE")) {
					flagSetEncoding = IDM_ENCODING_UTF8SIGN - IDM_ENCODING_ANSI + 1;
					state = 1;
				}
			} else if (*opt == L'1' && opt[1] == L'6') {
				opt += 2;
				state = ParseCommandLineEncoding(opt, IDM_ENCODING_UNICODE, IDM_ENCODING_UNICODEREV);
			}
		} else if (StrNCaseEqual(opt, L"UNICODE", CSTRLEN(L"UNICODE"))) {
			opt += CSTRLEN(L"UNICODE");
			state = ParseCommandLineEncoding(opt, IDM_ENCODING_UNICODE, IDM_ENCODING_UNICODEREV);
		}
		break;
	}

	return state;
}

void ParseCommandLine(void) {
	LPWSTR lpCmdLine = GetCommandLine();
	const size_t cmdSize = sizeof(WCHAR) * (lstrlen(lpCmdLine) + 1);

	if (cmdSize == sizeof(WCHAR)) {
		return;
	}

	// Good old console can also send args separated by Tabs
	StrTab2Space(lpCmdLine);

	LPWSTR lp1 = (LPWSTR)NP2HeapAlloc(cmdSize);
	LPWSTR lp3 = (LPWSTR)NP2HeapAlloc(cmdSize);

	// Start with 2nd argument
	if (!(ExtractFirstArgument(lpCmdLine, lp1, lp3) && *lp3)) {
		NP2HeapFree(lp1);
		NP2HeapFree(lp3);
		return;
	}

	BOOL bIsFileArg = FALSE;
	BOOL bIsNotepadReplacement = FALSE;
	LPWSTR lp2 = (LPWSTR)NP2HeapAlloc(cmdSize);
	while (ExtractFirstArgument(lp3, lp1, lp2)) {
		// options
		if (!bIsFileArg) {
			int state = 0;
			if (lp1[1] == 0) {
				switch (*lp1) {
				case L'+':
					flagMultiFileArg = 2;
					bIsFileArg = TRUE;
					state = 1;
					break;

				case L'-':
					flagMultiFileArg = 1;
					bIsFileArg = TRUE;
					state = 1;
					break;
				}
			} else if (*lp1 == L'/' || *lp1 == L'-') {
				state = ParseCommandLineOption(lp1, lp2, &bIsNotepadReplacement);
			}

			if (state == 1) {
				lstrcpy(lp3, lp2);
				continue;
			}
			if (state == 2 && flagMultiFileArg == 2) {
				ExtractFirstArgument(lp3, lp1, lp2);
			}
		}

		// pathname
		{
			LPWSTR lpFileBuf = (LPWSTR)NP2HeapAlloc(cmdSize);

			if (lpFileArg) {
				NP2HeapFree(lpFileArg);
			}

			lpFileArg = (LPWSTR)NP2HeapAlloc(sizeof(WCHAR) * (MAX_PATH + 2)); // changed for ActivatePrevInst() needs
			if (flagMultiFileArg == 2) {
				// multiple file arguments with quoted spaces
				lstrcpyn(lpFileArg, lp1, MAX_PATH);
			} else {
				lstrcpyn(lpFileArg, lp3, MAX_PATH);
			}

			PathFixBackslashes(lpFileArg);
			StrTrim(lpFileArg, L" \"");

			if (!PathIsRelative(lpFileArg) && !PathIsUNC(lpFileArg) &&
					PathGetDriveNumber(lpFileArg) == -1 /*&& PathGetDriveNumber(g_wchWorkingDirectory) != -1*/) {

				WCHAR wchPath[MAX_PATH];
				lstrcpy(wchPath, g_wchWorkingDirectory);
				PathStripToRoot(wchPath);
				PathAppend(wchPath, lpFileArg);
				lstrcpy(lpFileArg, wchPath);
			}

			if (flagMultiFileArg == 2) {
				cchiFileList = lstrlen(lpCmdLine) - lstrlen(lp3);

				while (cFileList < 32 && ExtractFirstArgument(lp3, lpFileBuf, lp3)) {
					PathQuoteSpaces(lpFileBuf);
					lpFileList[cFileList++] = StrDup(lpFileBuf);
				}
			}

			NP2HeapFree(lpFileBuf);
			break;
		}
	}

	NP2HeapFree(lp2);
	NP2HeapFree(lp1);
	NP2HeapFree(lp3);
}

//=============================================================================
//
// LoadFlags()
//
//
void LoadFlags(void) {
	IniSection section;
	WCHAR *pIniSectionBuf = (WCHAR *)NP2HeapAlloc(sizeof(WCHAR) * MAX_INI_SECTION_SIZE_FLAGS);
	const int cchIniSection = (int)(NP2HeapSize(pIniSectionBuf) / sizeof(WCHAR));
	IniSection *pIniSection = &section;
	IniSectionInit(pIniSection, 64);

	LoadIniSection(INI_SECTION_NAME_FLAGS, pIniSectionBuf, cchIniSection);
	IniSectionParse(pIniSection, pIniSectionBuf);

	bSingleFileInstance = IniSectionGetBool(pIniSection, L"SingleFileInstance", 1);
	bReuseWindow = IniSectionGetBool(pIniSection, L"ReuseWindow", 0);
	bStickyWinPos = IniSectionGetBool(pIniSection, L"StickyWindowPosition", 0);

	if (!flagReuseWindow && !flagNoReuseWindow) {
		flagNoReuseWindow = !bReuseWindow;
		flagSingleFileInstance = bSingleFileInstance;
	}

	if (flagMultiFileArg == 0) {
		if (IniSectionGetBool(pIniSection, L"MultiFileArg", 0)) {
			flagMultiFileArg = 2;
		}
	}

	flagRelativeFileMRU = IniSectionGetBool(pIniSection, L"RelativeFileMRU", 1);
	flagPortableMyDocs = IniSectionGetBool(pIniSection, L"PortableMyDocs", flagRelativeFileMRU);

	IniSectionGetString(pIniSection, L"DefaultExtension", L"txt", tchDefaultExtension, COUNTOF(tchDefaultExtension));
	StrTrim(tchDefaultExtension, L" \t.\"");

	IniSectionGetString(pIniSection, L"DefaultDirectory", L"", tchDefaultDir, COUNTOF(tchDefaultDir));

	LPCWSTR strValue = IniSectionGetValue(pIniSection, L"FileDlgFilters");
	if (StrNotEmpty(strValue)) {
		tchFileDlgFilters = StrDup(strValue);
	}

	dwFileCheckInverval = IniSectionGetInt(pIniSection, L"FileCheckInverval", 1000);
	dwAutoReloadTimeout = IniSectionGetInt(pIniSection, L"AutoReloadTimeout", 1000);
	dwFileLoadWarningMB = IniSectionGetInt(pIniSection, L"FileLoadWarningMB", 64);

	flagNoFadeHidden = IniSectionGetBool(pIniSection, L"NoFadeHidden", 0);

	int iValue = IniSectionGetInt(pIniSection, L"OpacityLevel", 75);
	iOpacityLevel = validate_i(iValue, 0, 100, 75);

	iValue = IniSectionGetInt(pIniSection, L"FindReplaceOpacityLevel", 75);
	iFindReplaceOpacityLevel = validate_i(iValue, 0, 100, 75);

	iValue = IniSectionGetInt(pIniSection, L"ToolbarLook", 1);
	flagToolbarLook = clamp_i(iValue, 0, 2);

	flagSimpleIndentGuides = IniSectionGetBool(pIniSection, L"SimpleIndentGuides", 0);
	fNoHTMLGuess = IniSectionGetBool(pIniSection, L"NoHTMLGuess", 0);
	fNoCGIGuess = IniSectionGetBool(pIniSection, L"NoCGIGuess", 0);
	fNoAutoDetection = IniSectionGetBool(pIniSection, L"NoAutoDetection", 0);
	fNoFileVariables = IniSectionGetBool(pIniSection, L"NoFileVariables", 0);

	if (StrIsEmpty(g_wchAppUserModelID)) {
		strValue = IniSectionGetValue(pIniSection, L"ShellAppUserModelID");
		if (StrNotEmpty(strValue)) {
			lstrcpyn(g_wchAppUserModelID, strValue, COUNTOF(g_wchAppUserModelID));
		} else {
			lstrcpy(g_wchAppUserModelID, L"(default)");
		}
	}

	if (flagUseSystemMRU == 0) {
		if (IniSectionGetBool(pIniSection, L"ShellUseSystemMRU", 1)) {
			flagUseSystemMRU = 2;
		}
	}

	IniSectionFree(pIniSection);
	NP2HeapFree(pIniSectionBuf);
}

//=============================================================================
//
// FindIniFile()
//
//
int CheckIniFile(LPWSTR lpszFile, LPCWSTR lpszModule) {
	WCHAR tchFileExpanded[MAX_PATH];
	ExpandEnvironmentStrings(lpszFile, tchFileExpanded, COUNTOF(tchFileExpanded));

	if (PathIsRelative(tchFileExpanded)) {
		WCHAR tchBuild[MAX_PATH];
		// program directory
		lstrcpy(tchBuild, lpszModule);
		lstrcpy(PathFindFileName(tchBuild), tchFileExpanded);
		if (PathFileExists(tchBuild)) {
			lstrcpy(lpszFile, tchBuild);
			return 1;
		}
		// Application Data
		if (S_OK == SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, SHGFP_TYPE_CURRENT, tchBuild)) {
			PathAppend(tchBuild, tchFileExpanded);
			if (PathFileExists(tchBuild)) {
				lstrcpy(lpszFile, tchBuild);
				return 1;
			}
		}
		// Home
		if (S_OK == SHGetFolderPath(NULL, CSIDL_PROFILE, NULL, SHGFP_TYPE_CURRENT, tchBuild)) {
			PathAppend(tchBuild, tchFileExpanded);
			if (PathFileExists(tchBuild)) {
				lstrcpy(lpszFile, tchBuild);
				return 1;
			}
		}
	} else if (PathFileExists(tchFileExpanded)) {
		lstrcpy(lpszFile, tchFileExpanded);
		return 1;
	}

	return 0;
}

int CheckIniFileRedirect(LPWSTR lpszFile, LPCWSTR lpszModule) {
	WCHAR tch[MAX_PATH];
	if (GetPrivateProfileString(INI_SECTION_NAME_NOTEPAD2, L"Notepad2.ini", L"", tch, COUNTOF(tch), lpszFile)) {
		if (CheckIniFile(tch, lpszModule)) {
			lstrcpy(lpszFile, tch);
		} else {
			WCHAR tchFileExpanded[MAX_PATH];
			ExpandEnvironmentStrings(tch, tchFileExpanded, COUNTOF(tchFileExpanded));
			if (PathIsRelative(tchFileExpanded)) {
				lstrcpy(lpszFile, lpszModule);
				lstrcpy(PathFindFileName(lpszFile), tchFileExpanded);
			} else {
				lstrcpy(lpszFile, tchFileExpanded);
			}
		}
		return 1;
	}
	return 0;
}

int FindIniFile(void) {
	WCHAR tchTest[MAX_PATH];
	WCHAR tchModule[MAX_PATH];
	GetModuleFileName(NULL, tchModule, COUNTOF(tchModule));

	if (StrNotEmpty(szIniFile)) {
		if (StrEqual(szIniFile, L"*?")) {
			return 0;
		}
		if (!CheckIniFile(szIniFile, tchModule)) {
			ExpandEnvironmentStringsEx(szIniFile, COUNTOF(szIniFile));
			if (PathIsRelative(szIniFile)) {
				lstrcpy(tchTest, tchModule);
				PathRemoveFileSpec(tchTest);
				PathAppend(tchTest, szIniFile);
				lstrcpy(szIniFile, tchTest);
			}
		}
		return 1;
	}

	lstrcpy(tchTest, PathFindFileName(tchModule));
	PathRenameExtension(tchTest, L".ini");
	int bFound = CheckIniFile(tchTest, tchModule);

	if (!bFound) {
		lstrcpy(tchTest, L"Notepad2.ini");
		bFound = CheckIniFile(tchTest, tchModule);
	}

	if (bFound) {
		// allow two redirections: administrator -> user -> custom
		if (CheckIniFileRedirect(tchTest, tchModule)) {
			CheckIniFileRedirect(tchTest, tchModule);
		}
		lstrcpy(szIniFile, tchTest);
	} else {
		lstrcpy(szIniFile, tchModule);
		PathRenameExtension(szIniFile, L".ini");
	}

	return 1;
}

int TestIniFile(void) {
	if (StrEqual(szIniFile, L"*?")) {
		lstrcpy(szIniFile2, L"");
		lstrcpy(szIniFile, L"");
		return 0;
	}

	if (PathIsDirectory(szIniFile) || *CharPrev(szIniFile, StrEnd(szIniFile)) == L'\\') {
		WCHAR wchModule[MAX_PATH];
		GetModuleFileName(NULL, wchModule, COUNTOF(wchModule));
		PathAppend(szIniFile, PathFindFileName(wchModule));
		PathRenameExtension(szIniFile, L".ini");
		if (!PathFileExists(szIniFile)) {
			lstrcpy(PathFindFileName(szIniFile), L"Notepad2.ini");
			if (!PathFileExists(szIniFile)) {
				lstrcpy(PathFindFileName(szIniFile), PathFindFileName(wchModule));
				PathRenameExtension(szIniFile, L".ini");
			}
		}
	}

	if (!PathFileExists(szIniFile) || PathIsDirectory(szIniFile)) {
		lstrcpy(szIniFile2, szIniFile);
		lstrcpy(szIniFile, L"");
		return 0;
	}

	return 1;
}

int CreateIniFile(void) {
	return CreateIniFileEx(szIniFile);
}

int CreateIniFileEx(LPCWSTR lpszIniFile) {
	if (StrNotEmpty(lpszIniFile)) {
		WCHAR *pwchTail;

		if ((pwchTail = StrRChr(lpszIniFile, NULL, L'\\')) != NULL) {
			*pwchTail = 0;
			SHCreateDirectoryEx(NULL, lpszIniFile, NULL);
			*pwchTail = L'\\';
		}

		HANDLE hFile = CreateFile(lpszIniFile,
						   GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
						   NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		dwLastIOError = GetLastError();
		if (hFile != INVALID_HANDLE_VALUE) {
			if (GetFileSize(hFile, NULL) == 0) {
				DWORD dw;
				WriteFile(hFile, (LPCVOID)L"\xFEFF[Notepad2]\r\n", 26, &dw, NULL);
			}
			CloseHandle(hFile);
			return 1;
		}
	}
	return 0;
}

//=============================================================================
//
// UpdateToolbar()
//
//
#define EnableTool(id, b)		SendMessage(hwndToolbar, TB_ENABLEBUTTON, id, MAKELPARAM(((b) ? 1 : 0), 0))
#define CheckTool(id, b)		SendMessage(hwndToolbar, TB_CHECKBUTTON, id, MAKELPARAM(b, 0))

void UpdateToolbar(void) {
	if (!bShowToolbar) {
		return;
	}

	EnableTool(IDT_FILE_ADDTOFAV, StrNotEmpty(szCurFile));

	EnableTool(IDT_EDIT_UNDO, SendMessage(hwndEdit, SCI_CANUNDO, 0, 0) /*&& !bReadOnly*/);
	EnableTool(IDT_EDIT_REDO, SendMessage(hwndEdit, SCI_CANREDO, 0, 0) /*&& !bReadOnly*/);

	int i = !EditIsEmptySelection();
	EnableTool(IDT_EDIT_CUT, i /*&& !bReadOnly*/);
	i = (int)SendMessage(hwndEdit, SCI_GETLENGTH, 0, 0);
	EnableTool(IDT_EDIT_COPY, i);
	EnableTool(IDT_EDIT_PASTE, SendMessage(hwndEdit, SCI_CANPASTE, 0, 0) /*&& !bReadOnly*/);

	EnableTool(IDT_EDIT_FIND, i);
	//EnableTool(IDT_EDIT_FINDNEXT, i);
	//EnableTool(IDT_EDIT_FINDPREV, i && StrNotEmptyA(efrData.szFind));
	EnableTool(IDT_EDIT_REPLACE, i /*&& !bReadOnly*/);
	EnableTool(IDT_EDIT_CLEAR, i /*&& !bReadOnly*/);

	EnableTool(IDT_VIEW_TOGGLEFOLDS, i && bShowCodeFolding);
	EnableTool(IDT_FILE_LAUNCH, i);

	CheckTool(IDT_VIEW_WORDWRAP, fWordWrap);
}

//=============================================================================
//
// UpdateStatusbar()
//
//
void UpdateStatusbar(void) {
	if (!bShowStatusbar) {
		return;
	}

	WCHAR tchLn[32];
	WCHAR tchLines[32];
	WCHAR tchCol[32];
	WCHAR tchCols[32];
	WCHAR tchCh[32];
	WCHAR tchChs[32];
	WCHAR tchSel[32];
	WCHAR tchSelCh[32];
	WCHAR tchDocPos[256];
	WCHAR tchDocSize[32];
	WCHAR tchLinesSelected[32];
	WCHAR tchMatchesCount[32];

	const Sci_Position iPos =  SciCall_GetCurrentPos();

	const int iLn = SciCall_LineFromPosition(iPos) + 1;
	wsprintf(tchLn, L"%i", iLn);
	FormatNumberStr(tchLn);

	const int iLines = SciCall_GetLineCount();
	wsprintf(tchLines, L"%i", iLines);
	FormatNumberStr(tchLines);

	int iCol = SciCall_GetColumn(iPos) + 1;
	wsprintf(tchCol, L"%i", iCol);
	FormatNumberStr(tchCol);

	const Sci_Position iLineStart = SciCall_PositionFromLine(iLn - 1);
	const Sci_Position iLineEnd = SciCall_GetLineEndPosition(iLn - 1);
	iCol = SciCall_CountCharacters(iLineStart, iPos) + 1;
	const int iLineChar = SciCall_CountCharacters(iLineStart, iLineEnd);
	wsprintf(tchCh, L"%i", iCol);
	wsprintf(tchChs, L"%i", iLineChar);
	FormatNumberStr(tchCh);
	FormatNumberStr(tchChs);

	iCol = SciCall_GetColumn(iLineEnd);
	wsprintf(tchCols, L"%i", iCol);
	FormatNumberStr(tchCols);

	const Sci_Position iSelStart = SciCall_GetSelectionStart();
	const Sci_Position iSelEnd = SciCall_GetSelectionEnd();
	if (iSelStart == iSelEnd) {
		lstrcpy(tchSel, L"0");
		lstrcpy(tchSelCh, L"0");
	} else if (SC_SEL_RECTANGLE != SciCall_GetSelectionMode()) {
		int iSel = SciCall_GetSelText(NULL) - 1;
		wsprintf(tchSel, L"%i", iSel);
		FormatNumberStr(tchSel);
		iSel = SciCall_CountCharacters(iSelStart, iSelEnd);
		wsprintf(tchSelCh, L"%i", iSel);
		FormatNumberStr(tchSelCh);
	} else {
		lstrcpy(tchSel, L"--");
		lstrcpy(tchSelCh, L"--");
	}

	// Print number of lines selected lines in statusbar
	if (iSelStart == iSelEnd) {
		lstrcpy(tchLinesSelected, L"0");
		lstrcpy(tchMatchesCount, L"0");
	} else {
		const int iStartLine = SciCall_LineFromPosition(iSelStart);
		const int iEndLine = SciCall_LineFromPosition(iSelEnd);
		const Sci_Position iStartOfLinePos = SciCall_PositionFromLine(iEndLine);
		int iLinesSelected = iEndLine - iStartLine;
		if (iSelStart != iSelEnd && iStartOfLinePos != iSelEnd) {
			iLinesSelected += 1;
		}
		wsprintf(tchLinesSelected, L"%i", iLinesSelected);
		FormatNumberStr(tchLinesSelected);
		wsprintf(tchMatchesCount, L"%i", iMatchesCount);
		FormatNumberStr(tchMatchesCount);
	}

	wsprintf(tchDocPos, cachedStatusItem.tchDocPosFmt, tchLn, tchLines,
				 tchCol, tchCols, tchCh, tchChs, tchSelCh, tchSel, tchLinesSelected, tchMatchesCount);

	const int iBytes = SciCall_GetLength();
	StrFormatByteSize(iBytes, tchDocSize, COUNTOF(tchDocSize));

	StatusSetText(hwndStatus, STATUS_DOCPOS, tchDocPos);
	const UINT updateMask = cachedStatusItem.updateMask;
	if (updateMask & STATUS_BAR_UPDATE_MASK_LEXER) {
		StatusSetText(hwndStatus, STATUS_LEXER, cachedStatusItem.pszLexerName);
	}
	if (updateMask & STATUS_BAR_UPDATE_MASK_CODEPAGE) {
		StatusSetText(hwndStatus, STATUS_CODEPAGE, mEncoding[iEncoding].wchLabel);
	}
	if (updateMask & STATUS_BAR_UPDATE_MASK_EOLMODE) {
		StatusSetText(hwndStatus, STATUS_EOLMODE, cachedStatusItem.pszEOLMode);
	}
	if (updateMask & STATUS_BAR_UPDATE_MASK_OVRMODE) {
		StatusSetText(hwndStatus, STATUS_OVRMODE, cachedStatusItem.pszOvrMode);
	}
	StatusSetText(hwndStatus, STATUS_DOCSIZE, tchDocSize);
	if (updateMask & STATUS_BAR_UPDATE_MASK_DOCZOOM) {
		StatusSetText(hwndStatus, STATUS_DOCZOOM, cachedStatusItem.tchZoom);
	}
	if (updateMask != 0) {
		cachedStatusItem.updateMask = 0;
	}
}

//=============================================================================
//
// UpdateLineNumberWidth()
//
//
void UpdateLineNumberWidth(void) {
	if (bShowLineNumbers) {
		char tchLines[32];

		const int iLines = SciCall_GetLineCount();
		wsprintfA(tchLines, "_%i_", iLines);

		const int iLineMarginWidthNow = SciCall_GetMarginWidth(MARGIN_LINE_NUMBER);
		const int iLineMarginWidthFit = SciCall_TextWidth(STYLE_LINENUMBER, tchLines);

		if (iLineMarginWidthNow != iLineMarginWidthFit) {
#if !NP2_DEBUG_FOLD_LEVEL
			SciCall_SetMarginWidth(MARGIN_LINE_NUMBER, iLineMarginWidthFit);

#else
			SciCall_SetMarginWidth(MARGIN_LINE_NUMBER, RoundToCurrentDPI(100));
#endif
		}
	} else {
		SciCall_SetMarginWidth(MARGIN_LINE_NUMBER, 0);
	}
}

// based on SciTEWin::FullScreenToggle()
void ToggleFullScreenMode(void) {
	static BOOL bSaved;
	static WINDOWPLACEMENT wndpl;
	static RECT rcWorkArea;
	static LONG_PTR exStyle;

	HWND wTaskBar = FindWindow(L"Shell_TrayWnd", L"");
	HWND wStartButton = FindWindow(L"Button", NULL);

	if (bInFullScreenMode) {
		if (!bSaved) {
			bSaved = TRUE;
			SystemParametersInfo(SPI_GETWORKAREA, 0, &rcWorkArea, 0);
			wndpl.length = sizeof(WINDOWPLACEMENT);
			GetWindowPlacement(hwndMain, &wndpl);
			exStyle = GetWindowLongPtr(hwndEdit, GWL_EXSTYLE);
		}

		HMONITOR hMonitor = MonitorFromWindow(hwndMain, MONITOR_DEFAULTTONEAREST);
		MONITORINFO mi;
		mi.cbSize = sizeof(mi);
		GetMonitorInfo(hMonitor, &mi);

		const int x = mi.rcMonitor.left;
		const int y = mi.rcMonitor.top;
		const int w = mi.rcMonitor.right - x;
		const int h = mi.rcMonitor.bottom - y;
		const int cx = GetSystemMetricsEx(SM_CXSIZEFRAME);
		const int cy = GetSystemMetricsEx(SM_CYSIZEFRAME);

		int top = cy;
		if (iFullScreenMode & (FullScreenMode_HideCaption | FullScreenMode_HideMenu)) {
			top += GetSystemMetricsEx(SM_CYCAPTION);
		}
		if (iFullScreenMode & FullScreenMode_HideMenu) {
			top += GetSystemMetricsEx(SM_CYMENU);
		}

		SystemParametersInfo(SPI_SETWORKAREA, 0, NULL, SPIF_SENDCHANGE);

		if (wStartButton) {
			ShowWindow(wStartButton, SW_HIDE);
		}
		ShowWindow(wTaskBar, SW_HIDE);

		SetWindowLongPtr(hwndEdit, GWL_EXSTYLE, 0);
		SetWindowPos(hwndMain, (IsTopMost() ? HWND_TOPMOST : HWND_TOP), x - cx, y - top, x + w + 2 * cx , y + h + top + cy, 0);
	} else {
		bSaved = FALSE;
		ShowWindow(wTaskBar, SW_SHOW);
		if (wStartButton) {
			ShowWindow(wStartButton, SW_SHOW);
		}
		SetWindowLongPtr(hwndEdit, GWL_EXSTYLE, exStyle);
		if (!IsTopMost()) {
			SetWindowPos(hwndMain, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
		}
		if (wndpl.length) {
			SystemParametersInfo(SPI_SETWORKAREA, 0, &rcWorkArea, 0);
			if (wndpl.showCmd == SW_SHOWMAXIMIZED) {
				ShowWindow(hwndMain, SW_RESTORE);
				ShowWindow(hwndMain, SW_SHOWMAXIMIZED);
			} else {
				SetWindowPlacement(hwndMain, &wndpl);
			}
		}
	}
	SetForegroundWindow(hwndMain);
}

//=============================================================================
//
//	FileIO()
//
//
BOOL FileIO(BOOL fLoad, LPWSTR pszFile, BOOL bFlag, EditFileIOStatus *status) {
	BeginWaitCursor();

	WCHAR tch[MAX_PATH + 40];
	WCHAR fmt[64];
	FormatString(tch, fmt, (fLoad ? IDS_LOADFILE : IDS_SAVEFILE), PathFindFileName(pszFile));

	StatusSetText(hwndStatus, STATUS_HELP, tch);
	StatusSetSimple(hwndStatus, TRUE);

	InvalidateRect(hwndStatus, NULL, TRUE);
	UpdateWindow(hwndStatus);

	const BOOL fSuccess = fLoad ? EditLoadFile(hwndEdit, pszFile, bFlag, status) : EditSaveFile(hwndEdit, pszFile, bFlag, status);
	const DWORD dwFileAttributes = GetFileAttributes(pszFile);
	bReadOnly = (dwFileAttributes != INVALID_FILE_ATTRIBUTES) && (dwFileAttributes & FILE_ATTRIBUTE_READONLY);

	StatusSetSimple(hwndStatus, FALSE);
	EndWaitCursor();

	return fSuccess;
}

//=============================================================================
//
// FileLoad()
//
//
BOOL FileLoad(BOOL bDontSave, BOOL bNew, BOOL bReload, BOOL bNoEncDetect, LPCWSTR lpszFile) {
	WCHAR tch[MAX_PATH] = L"";
	BOOL fSuccess = FALSE;
	int line = 0;
	int col = 0;
	int keepTitleExcerpt = fKeepTitleExcerpt;
	int lexerSpecified = flagLexerSpecified;

	if (!bNew && StrNotEmpty(lpszFile)) {
		lstrcpy(tch, lpszFile);
		if (lpszFile == szCurFile || StrCaseEqual(lpszFile, szCurFile)) {
			const Sci_Position pos = SciCall_GetCurrentPos();
			line = SciCall_LineFromPosition(pos) + 1;
			col = SciCall_GetColumn(pos) + 1;
			keepTitleExcerpt = 1;
			lexerSpecified = 1;
		}
		fSuccess = TRUE;
	}
	if (!bDontSave) {
		if (!FileSave(FALSE, TRUE, FALSE, FALSE)) {
			return FALSE;
		}
	}

	if (bNew) {
		lstrcpy(szCurFile, L"");
		SetDlgItemText(hwndMain, IDC_FILENAME, szCurFile);
		SetDlgItemInt(hwndMain, IDC_REUSELOCK, GetTickCount(), FALSE);
		if (!keepTitleExcerpt) {
			lstrcpy(szTitleExcerpt, L"");
		}
		FileVars_Init(NULL, 0, &fvCurFile);
		EditSetNewText(hwndEdit, "", 0);
		Style_SetLexer(hwndEdit, NULL);
		bModified = FALSE;
		bReadOnly = FALSE;
		iEOLMode = iLineEndings[iDefaultEOLMode];
		SendMessage(hwndEdit, SCI_SETEOLMODE, iLineEndings[iDefaultEOLMode], 0);
		iEncoding = iDefaultEncoding;
		iOriginalEncoding = iDefaultEncoding;
		SendMessage(hwndEdit, SCI_SETCODEPAGE, (iDefaultEncoding == CPI_DEFAULT) ? iDefaultCodePage : SC_CP_UTF8, 0);
		UpdateStatusBarCache(STATUS_CODEPAGE);
		UpdateStatusBarCache(STATUS_EOLMODE);
		UpdateWindowTitle();

		// Terminate file watching
		if (bResetFileWatching) {
			iFileWatchingMode = 0;
		}
		InstallFileWatching(NULL);

		return TRUE;
	}

	if (!fSuccess) {
		if (!OpenFileDlg(hwndMain, tch, COUNTOF(tch), NULL)) {
			return FALSE;
		}
	}
	fSuccess = FALSE;

	WCHAR szFileName[MAX_PATH] = L"";
	ExpandEnvironmentStringsEx(tch, COUNTOF(tch));

	if (PathIsRelative(tch)) {
		lstrcpyn(szFileName, g_wchWorkingDirectory, COUNTOF(szFileName));
		PathAppend(szFileName, tch);
	} else {
		lstrcpy(szFileName, tch);
	}

	PathCanonicalizeEx(szFileName);
	GetLongPathNameEx(szFileName, COUNTOF(szFileName));

	if (PathIsLnkFile(szFileName)) {
		PathGetLnkPath(szFileName, szFileName, COUNTOF(szFileName));
	}

#if NP2_USE_DESIGNATED_INITIALIZER
	EditFileIOStatus status = {
		.iEncoding = iEncoding,
		.iEOLMode = iEOLMode,
	};
#else
	EditFileIOStatus status = { iEncoding, iEOLMode };
#endif

	// Ask to create a new file...
	if (!bReload && !PathFileExists(szFileName)) {
		UINT result = IDCANCEL;
		if (flagQuietCreate || (result = MsgBox(MBYESNOCANCEL, IDS_ASK_CREATE, szFileName)) == IDYES) {
			HANDLE hFile = CreateFile(szFileName,
									  GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
									  NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
			dwLastIOError = GetLastError();
			if (hFile != INVALID_HANDLE_VALUE) {
				fSuccess = TRUE;
				CloseHandle(hFile);
				FileVars_Init(NULL, 0, &fvCurFile);
				EditSetNewText(hwndEdit, "", 0);
				Style_SetLexer(hwndEdit, NULL);
				iEOLMode = iLineEndings[iDefaultEOLMode];
				SendMessage(hwndEdit, SCI_SETEOLMODE, iLineEndings[iDefaultEOLMode], 0);
				if (iSrcEncoding != -1) {
					iEncoding = iSrcEncoding;
					iOriginalEncoding = iSrcEncoding;
				} else {
					iEncoding = iDefaultEncoding;
					iOriginalEncoding = iDefaultEncoding;
				}
				SendMessage(hwndEdit, SCI_SETCODEPAGE, (iEncoding == CPI_DEFAULT) ? iDefaultCodePage : SC_CP_UTF8, 0);
				bReadOnly = FALSE;
			}
		} else if (result == IDCANCEL) {
			NP2ExitWind(hwndMain);
			return FALSE;
		} else {
			return FALSE;
		}
	} else {
		fSuccess = FileIO(TRUE, szFileName, bNoEncDetect, &status);
		if (fSuccess) {
			iEncoding = status.iEncoding;
			iEOLMode = status.iEOLMode;
		}
	}

	if (fSuccess) {
		lstrcpy(szCurFile, szFileName);
		SetDlgItemText(hwndMain, IDC_FILENAME, szCurFile);
		SetDlgItemInt(hwndMain, IDC_REUSELOCK, GetTickCount(), FALSE);
		if (!keepTitleExcerpt) {
			lstrcpy(szTitleExcerpt, L"");
		}
		iOriginalEncoding = iEncoding;
		bModified = FALSE;
		UpdateStatusBarCache(STATUS_CODEPAGE);
		UpdateStatusBarCache(STATUS_EOLMODE);
		if (!lexerSpecified) { // flagLexerSpecified will be cleared
			np2LexLangIndex = 0;
			Style_SetLexerFromFile(hwndEdit, szCurFile);
		}
		if (!lexerSpecified) {
			UpdateLineNumberWidth();
		}
		//bReadOnly = FALSE;
		SendMessage(hwndEdit, SCI_SETEOLMODE, iEOLMode, 0);
		MRU_AddFile(pFileMRU, szFileName, flagRelativeFileMRU, flagPortableMyDocs);
		if (flagUseSystemMRU == 2) {
			SHAddToRecentDocs(SHARD_PATHW, szFileName);
		}

		// Install watching of the current file
		if (!bReload && bResetFileWatching) {
			iFileWatchingMode = 0;
		}
		InstallFileWatching(szCurFile);

		if (line > 1 || col > 1) {
			EditJumpTo(hwndEdit, line, col);
			EditEnsureSelectionVisible(hwndEdit);
		}
#if NP2_ENABLE_DOT_LOG_FEATURE
		// the .LOG feature ...
		if (SendMessage(hwndEdit, SCI_GETLENGTH, 0, 0) >= 4) {
			char tchLog[5] = "";
			SendMessage(hwndEdit, SCI_GETTEXT, 5, (LPARAM)tchLog);
			if (StrEqual(tchLog, ".LOG")) {
				EditJumpTo(hwndEdit, -1, 0);
				SendMessage(hwndEdit, SCI_BEGINUNDOACTION, 0, 0);
				SendMessage(hwndEdit, SCI_NEWLINE, 0, 0);
				SendWMCommand(hwndMain, IDM_EDIT_INSERT_SHORTDATE);
				EditJumpTo(hwndEdit, -1, 0);
				SendMessage(hwndEdit, SCI_NEWLINE, 0, 0);
				SendMessage(hwndEdit, SCI_ENDUNDOACTION, 0, 0);
				EditJumpTo(hwndEdit, -1, 0);
				EditEnsureSelectionVisible(hwndEdit);
			}
		}
#endif

		UpdateStatusbar();
		UpdateWindowTitle();
		// Show warning: Unicode file loaded as ANSI
		if (status.bUnicodeErr) {
			MsgBox(MBWARN, IDS_ERR_UNICODE);
		}
		// Show inconsistent line endings warning
		if (status.bInconsistent && bWarnLineEndings) {
			if (WarnLineEndingDlg(hwndMain, &status)) {
				const int iNewEOLMode = iLineEndings[status.iEOLMode];
				ConvertLineEndings(iNewEOLMode);
			}
		}
	} else if (!status.bFileTooBig) {
		MsgBox(MBWARN, IDS_ERR_LOADFILE, szFileName);
	}

	return fSuccess;
}

//=============================================================================
//
// FileSave()
//
//
BOOL FileSave(BOOL bSaveAlways, BOOL bAsk, BOOL bSaveAs, BOOL bSaveCopy) {
	const BOOL Untitled = StrIsEmpty(szCurFile);
	BOOL bIsEmptyNewFile = FALSE;

	if (Untitled) {
		const int cchText = (int)SendMessage(hwndEdit, SCI_GETLENGTH, 0, 0);
		if (cchText == 0) {
			bIsEmptyNewFile = TRUE;
		} else if (cchText < 2047) {
			char tchText[2048];
			SendMessage(hwndEdit, SCI_GETTEXT, 2047, (LPARAM)tchText);
			StrTrimA(tchText, " \t\n\r");
			if (StrIsEmptyA(tchText)) {
				bIsEmptyNewFile = TRUE;
			}
		}
	}

	if (!bSaveAlways && (!IsDocumentModified() || bIsEmptyNewFile) && !bSaveAs) {
		return TRUE;
	}

	if (bAsk) {
		// File or "Untitled" ...
		WCHAR tch[MAX_PATH];
		if (!Untitled) {
			lstrcpy(tch, szCurFile);
		} else {
			GetString(IDS_UNTITLED, tch, COUNTOF(tch));
		}

		switch (MsgBox(MBYESNOCANCEL, IDS_ASK_SAVE, tch)) {
		case IDCANCEL:
			return FALSE;
		case IDNO:
			return TRUE;
		}
	}

	BOOL fSuccess = FALSE;
	WCHAR tchFile[MAX_PATH];
#if NP2_USE_DESIGNATED_INITIALIZER
	EditFileIOStatus status = {
		.iEncoding = iEncoding,
		.iEOLMode = iEOLMode,
	};
#else
	EditFileIOStatus status = { iEncoding, iEOLMode };
#endif

	// Read only...
	if (!bSaveAs && !bSaveCopy && !Untitled) {
		const DWORD dwFileAttributes = GetFileAttributes(szCurFile);
		bReadOnly = (dwFileAttributes != INVALID_FILE_ATTRIBUTES) && (dwFileAttributes & FILE_ATTRIBUTE_READONLY);
		if (bReadOnly) {
			UpdateWindowTitle();
			if (MsgBox(MBYESNOWARN, IDS_READONLY_SAVE, szCurFile) == IDYES) {
				bSaveAs = TRUE;
			} else {
				return FALSE;
			}
		}
		if (!bSaveAs) {
			fSuccess = FileIO(FALSE, szCurFile, FALSE, &status);
			if (!fSuccess) {
				bSaveAs = TRUE;
			}
		}
	}

	// Save As...
	if (bSaveAs || bSaveCopy || Untitled) {
		WCHAR tchInitialDir[MAX_PATH] = L"";
		if (bSaveCopy && StrNotEmpty(tchLastSaveCopyDir)) {
			lstrcpy(tchInitialDir, tchLastSaveCopyDir);
			lstrcpy(tchFile, tchLastSaveCopyDir);
			PathAppend(tchFile, PathFindFileName(szCurFile));
		} else {
			lstrcpy(tchFile, szCurFile);
		}

		if (SaveFileDlg(hwndMain, tchFile, COUNTOF(tchFile), tchInitialDir)) {
			fSuccess = FileIO(FALSE, tchFile, bSaveCopy, &status);
			if (fSuccess) {
				if (!bSaveCopy) {
					lstrcpy(szCurFile, tchFile);
					SetDlgItemText(hwndMain, IDC_FILENAME, szCurFile);
					SetDlgItemInt(hwndMain, IDC_REUSELOCK, GetTickCount(), FALSE);
					if (!fKeepTitleExcerpt) {
						lstrcpy(szTitleExcerpt, L"");
					}
					Style_SetLexerFromFile(hwndEdit, szCurFile);
				} else {
					lstrcpy(tchLastSaveCopyDir, tchFile);
					PathRemoveFileSpec(tchLastSaveCopyDir);
				}
			}
		} else {
			return FALSE;
		}
	} else if (!fSuccess) {
		fSuccess = FileIO(FALSE, szCurFile, FALSE, &status);
	}

	if (fSuccess) {
		if (!bSaveCopy) {
			bModified = FALSE;
			iOriginalEncoding = iEncoding;
			MRU_AddFile(pFileMRU, szCurFile, flagRelativeFileMRU, flagPortableMyDocs);
			if (flagUseSystemMRU == 2) {
				SHAddToRecentDocs(SHARD_PATHW, szCurFile);
			}
			if (flagRelaunchElevated == 2 && bSaveAs && iPathNameFormat == 0) {
				iPathNameFormat = 1;
			}
			UpdateWindowTitle();

			// Install watching of the current file
			if (bSaveAs && bResetFileWatching) {
				iFileWatchingMode = 0;
			}
			InstallFileWatching(szCurFile);
		}
	} else if (!status.bCancelDataLoss) {
		if (StrNotEmpty(szCurFile) != 0) {
			lstrcpy(tchFile, szCurFile);
		}

		UpdateWindowTitle();
		MsgBox(MBWARN, IDS_ERR_SAVEFILE, tchFile);
	}

	return fSuccess;
}

//=============================================================================
//
// OpenFileDlg()
//
//
BOOL OpenFileDlg(HWND hwnd, LPWSTR lpstrFile, int cchFile, LPCWSTR lpstrInitialDir) {
	WCHAR szFile[MAX_PATH];
	LPWSTR szFilter = (LPWSTR)NP2HeapAlloc(MAX_OPEN_SAVE_FILE_DIALOG_FILTER_SIZE * sizeof(WCHAR));
	WCHAR tchInitialDir[MAX_PATH] = L"";

	lstrcpy(szFile, L"");
	Style_GetOpenDlgFilterStr(szFilter, MAX_OPEN_SAVE_FILE_DIALOG_FILTER_SIZE);

	if (!lpstrInitialDir) {
		if (StrNotEmpty(szCurFile)) {
			lstrcpy(tchInitialDir, szCurFile);
			PathRemoveFileSpec(tchInitialDir);
		} else if (StrNotEmpty(tchDefaultDir)) {
			ExpandEnvironmentStrings(tchDefaultDir, tchInitialDir, COUNTOF(tchInitialDir));
			if (PathIsRelative(tchInitialDir)) {
				WCHAR tchModule[MAX_PATH];
				GetModuleFileName(NULL, tchModule, COUNTOF(tchModule));
				PathRemoveFileSpec(tchModule);
				PathAppend(tchModule, tchInitialDir);
				PathCanonicalize(tchInitialDir, tchModule);
			}
		} else {
			lstrcpy(tchInitialDir, g_wchWorkingDirectory);
		}
	}

	OPENFILENAME ofn;
	ZeroMemory(&ofn, sizeof(OPENFILENAME));
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = hwnd;
	ofn.lpstrFilter = szFilter;
	ofn.lpstrFile = szFile;
	ofn.lpstrInitialDir = (lpstrInitialDir) ? lpstrInitialDir : tchInitialDir;
	ofn.nMaxFile = COUNTOF(szFile);
	ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | /* OFN_NOCHANGEDIR |*/
				OFN_DONTADDTORECENT | OFN_PATHMUSTEXIST |
				OFN_SHAREAWARE /*| OFN_NODEREFERENCELINKS*/ | OFN_NOVALIDATE;
	ofn.lpstrDefExt = StrNotEmpty(tchDefaultExtension) ? tchDefaultExtension : NULL;

	const BOOL success = GetOpenFileName(&ofn);
	if (success) {
		lstrcpyn(lpstrFile, szFile, cchFile);
	}
	NP2HeapFree(szFilter);
	return success;
}

//=============================================================================
//
// SaveFileDlg()
//
//
BOOL SaveFileDlg(HWND hwnd, LPWSTR lpstrFile, int cchFile, LPCWSTR lpstrInitialDir) {
	WCHAR szNewFile[MAX_PATH];
	LPWSTR szFilter = (LPWSTR)NP2HeapAlloc(MAX_OPEN_SAVE_FILE_DIALOG_FILTER_SIZE * sizeof(WCHAR));
	WCHAR tchInitialDir[MAX_PATH] = L"";

	lstrcpy(szNewFile, lpstrFile);
	Style_GetOpenDlgFilterStr(szFilter, MAX_OPEN_SAVE_FILE_DIALOG_FILTER_SIZE);

	if (StrNotEmpty(lpstrInitialDir)) {
		lstrcpy(tchInitialDir, lpstrInitialDir);
	} else if (StrNotEmpty(szCurFile)) {
		lstrcpy(tchInitialDir, szCurFile);
		PathRemoveFileSpec(tchInitialDir);
	} else if (StrNotEmpty(tchDefaultDir)) {
		ExpandEnvironmentStrings(tchDefaultDir, tchInitialDir, COUNTOF(tchInitialDir));
		if (PathIsRelative(tchInitialDir)) {
			WCHAR tchModule[MAX_PATH];
			GetModuleFileName(NULL, tchModule, COUNTOF(tchModule));
			PathRemoveFileSpec(tchModule);
			PathAppend(tchModule, tchInitialDir);
			PathCanonicalize(tchInitialDir, tchModule);
		}
	} else {
		lstrcpy(tchInitialDir, g_wchWorkingDirectory);
	}

	OPENFILENAME ofn;
	ZeroMemory(&ofn, sizeof(OPENFILENAME));
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = hwnd;
	ofn.lpstrFilter = szFilter;
	ofn.lpstrFile = szNewFile;
	ofn.lpstrInitialDir = tchInitialDir;
	ofn.nMaxFile = MAX_PATH;
	ofn.Flags = OFN_HIDEREADONLY /*| OFN_NOCHANGEDIR*/ |
				/*OFN_NODEREFERENCELINKS |*/ OFN_OVERWRITEPROMPT |
				OFN_DONTADDTORECENT | OFN_PATHMUSTEXIST;
	ofn.lpstrDefExt = StrNotEmpty(tchDefaultExtension) ? tchDefaultExtension : NULL;

	const BOOL success = GetSaveFileName(&ofn);
	if (success) {
		lstrcpyn(lpstrFile, szNewFile, cchFile);
	}
	NP2HeapFree(szFilter);
	return success;
}

/******************************************************************************
*
* ActivatePrevInst()
*
* Tries to find and activate an already open Notepad2 Window
*
*
******************************************************************************/
BOOL CALLBACK EnumWndProc(HWND hwnd, LPARAM lParam) {
	BOOL bContinue = TRUE;
	WCHAR szClassName[64];

	if (GetClassName(hwnd, szClassName, COUNTOF(szClassName))) {
		if (StrCaseEqual(szClassName, wchWndClass)) {
			const DWORD dwReuseLock = GetDlgItemInt(hwnd, IDC_REUSELOCK, NULL, FALSE);
			if (GetTickCount() - dwReuseLock >= REUSEWINDOWLOCKTIMEOUT) {
				*(HWND *)lParam = hwnd;
				if (IsWindowEnabled(hwnd)) {
					bContinue = FALSE;
				}
			}
		}
	}
	return bContinue;
}

BOOL CALLBACK EnumWndProc2(HWND hwnd, LPARAM lParam) {
	BOOL bContinue = TRUE;
	WCHAR szClassName[64];

	if (GetClassName(hwnd, szClassName, COUNTOF(szClassName))) {
		if (StrCaseEqual(szClassName, wchWndClass)) {
			const DWORD dwReuseLock = GetDlgItemInt(hwnd, IDC_REUSELOCK, NULL, FALSE);
			if (GetTickCount() - dwReuseLock >= REUSEWINDOWLOCKTIMEOUT) {
				if (IsWindowEnabled(hwnd)) {
					bContinue = FALSE;
				}

				WCHAR tchFileName[MAX_PATH] = L"";
				GetDlgItemText(hwnd, IDC_FILENAME, tchFileName, COUNTOF(tchFileName));
				if (StrCaseEqual(tchFileName, lpFileArg)) {
					*(HWND *)lParam = hwnd;
				} else {
					bContinue = TRUE;
				}
			}
		}
	}
	return bContinue;
}

BOOL ActivatePrevInst(void) {
	if ((flagNoReuseWindow && !flagSingleFileInstance) || flagStartAsTrayIcon || flagNewFromClipboard || flagPasteBoard) {
		return FALSE;
	}

	if (flagSingleFileInstance && lpFileArg) {
		ExpandEnvironmentStringsEx(lpFileArg, (DWORD)(NP2HeapSize(lpFileArg) / sizeof(WCHAR)));

		if (PathIsRelative(lpFileArg)) {
			WCHAR tchTmp[MAX_PATH];
			lstrcpyn(tchTmp, g_wchWorkingDirectory, COUNTOF(tchTmp));
			PathAppend(tchTmp, lpFileArg);
			lstrcpy(lpFileArg, tchTmp);
		}

		GetLongPathNameEx(lpFileArg, MAX_PATH);

		HWND hwnd = NULL;
		EnumWindows(EnumWndProc2, (LPARAM)&hwnd);

		if (hwnd != NULL) {
			// Enabled
			if (IsWindowEnabled(hwnd)) {
				// Make sure the previous window won't pop up a change notification message
				//SendMessage(hwnd, APPM_CHANGENOTIFYCLEAR, 0, 0);

				if (IsIconic(hwnd)) {
					ShowWindowAsync(hwnd, SW_RESTORE);
				}

				if (!IsWindowVisible(hwnd)) {
					SendMessage(hwnd, APPM_TRAYMESSAGE, 0, WM_LBUTTONDBLCLK);
					SendMessage(hwnd, APPM_TRAYMESSAGE, 0, WM_LBUTTONUP);
				}

				SetForegroundWindow(hwnd);

				DWORD cb = sizeof(NP2PARAMS);
				if (lpSchemeArg) {
					cb += (lstrlen(lpSchemeArg) + 1) * sizeof(WCHAR);
				}

				LPNP2PARAMS params = (LPNP2PARAMS)GlobalAlloc(GPTR, cb);
				params->flagFileSpecified = FALSE;
				params->flagChangeNotify = 0;
				params->flagQuietCreate = FALSE;
				params->flagLexerSpecified = flagLexerSpecified;
				if (flagLexerSpecified && lpSchemeArg) {
					lstrcpy(StrEnd(&params->wchData) + 1, lpSchemeArg);
					params->iInitialLexer = -1;
				} else {
					params->iInitialLexer = iInitialLexer;
				}
				params->flagJumpTo = flagJumpTo;
				params->iInitialLine = iInitialLine;
				params->iInitialColumn = iInitialColumn;

				params->iSrcEncoding = (lpEncodingArg) ? Encoding_Match(lpEncodingArg) : -1;
				params->flagSetEncoding = flagSetEncoding;
				params->flagSetEOLMode = flagSetEOLMode;
				params->flagTitleExcerpt = 0;

				COPYDATASTRUCT cds;
				cds.dwData = DATA_NOTEPAD2_PARAMS;
				cds.cbData = (DWORD)GlobalSize(params);
				cds.lpData = params;

				SendMessage(hwnd, WM_COPYDATA, 0, (LPARAM)&cds);
				GlobalFree(params);

				return TRUE;
			}

			// Ask...
			if (IDYES == MsgBox(MBYESNO, IDS_ERR_PREVWINDISABLED)) {
				return FALSE;
			}
			return TRUE;
		}
	}

	if (flagNoReuseWindow) {
		return FALSE;
	}

	HWND hwnd = NULL;
	EnumWindows(EnumWndProc, (LPARAM)&hwnd);

	// Found a window
	if (hwnd != NULL) {
		// Enabled
		if (IsWindowEnabled(hwnd)) {
			// Make sure the previous window won't pop up a change notification message
			//SendMessage(hwnd, APPM_CHANGENOTIFYCLEAR, 0, 0);

			if (IsIconic(hwnd)) {
				ShowWindowAsync(hwnd, SW_RESTORE);
			}

			if (!IsWindowVisible(hwnd)) {
				SendMessage(hwnd, APPM_TRAYMESSAGE, 0, WM_LBUTTONDBLCLK);
				SendMessage(hwnd, APPM_TRAYMESSAGE, 0, WM_LBUTTONUP);
			}

			SetForegroundWindow(hwnd);

			if (lpFileArg) {
				ExpandEnvironmentStringsEx(lpFileArg, (DWORD)(NP2HeapSize(lpFileArg) / sizeof(WCHAR)));

				if (PathIsRelative(lpFileArg)) {
					WCHAR tchTmp[MAX_PATH];
					lstrcpyn(tchTmp, g_wchWorkingDirectory, COUNTOF(tchTmp));
					PathAppend(tchTmp, lpFileArg);
					lstrcpy(lpFileArg, tchTmp);
				}

				DWORD cb = sizeof(NP2PARAMS);
				cb += (lstrlen(lpFileArg) + 1) * sizeof(WCHAR);

				if (lpSchemeArg) {
					cb += (lstrlen(lpSchemeArg) + 1) * sizeof(WCHAR);
				}

				const int cchTitleExcerpt = lstrlen(szTitleExcerpt);
				if (cchTitleExcerpt) {
					cb += (cchTitleExcerpt + 1) * sizeof(WCHAR);
				}

				LPNP2PARAMS params = (LPNP2PARAMS)GlobalAlloc(GPTR, cb);
				params->flagFileSpecified = TRUE;
				lstrcpy(&params->wchData, lpFileArg);
				params->flagChangeNotify = flagChangeNotify;
				params->flagQuietCreate = flagQuietCreate;
				params->flagLexerSpecified = flagLexerSpecified;
				if (flagLexerSpecified && lpSchemeArg) {
					lstrcpy(StrEnd(&params->wchData) + 1, lpSchemeArg);
					params->iInitialLexer = -1;
				} else {
					params->iInitialLexer = iInitialLexer;
				}
				params->flagJumpTo = flagJumpTo;
				params->iInitialLine = iInitialLine;
				params->iInitialColumn = iInitialColumn;

				params->iSrcEncoding = (lpEncodingArg) ? Encoding_Match(lpEncodingArg) : -1;
				params->flagSetEncoding = flagSetEncoding;
				params->flagSetEOLMode = flagSetEOLMode;

				if (cchTitleExcerpt) {
					lstrcpy(StrEnd(&params->wchData) + 1, szTitleExcerpt);
					params->flagTitleExcerpt = 1;
				} else {
					params->flagTitleExcerpt = 0;
				}

				COPYDATASTRUCT cds;
				cds.dwData = DATA_NOTEPAD2_PARAMS;
				cds.cbData = (DWORD)GlobalSize(params);
				cds.lpData = params;

				SendMessage(hwnd, WM_COPYDATA, 0, (LPARAM)&cds);
				GlobalFree(params);
				NP2HeapFree(lpFileArg);
			}
			return TRUE;
		}

		// Ask...
		if (IDYES == MsgBox(MBYESNO, IDS_ERR_PREVWINDISABLED)) {
			return FALSE;
		}
		return TRUE;
	}
	return FALSE;
}

//=============================================================================
//
// RelaunchMultiInst()
//
//
BOOL RelaunchMultiInst(void) {
	if (flagMultiFileArg == 2 && cFileList > 1) {
		LPWSTR lpCmdLineNew = StrDup(GetCommandLine());
		const size_t cmdSize = sizeof(WCHAR) * (lstrlen(lpCmdLineNew) + 1);
		LPWSTR lp1 = (LPWSTR)NP2HeapAlloc(cmdSize);
		LPWSTR lp2 = (LPWSTR)NP2HeapAlloc(cmdSize);

		StrTab2Space(lpCmdLineNew);
		lstrcpy(lpCmdLineNew + cchiFileList, L"");

		LPWSTR pwch = CharPrev(lpCmdLineNew, StrEnd(lpCmdLineNew));
		int i = 0;
		while (*pwch == L' ' || *pwch == L'-' || *pwch == L'+') {
			*pwch = L' ';
			pwch = CharPrev(lpCmdLineNew, pwch);
			if (i++ > 1) {
				cchiFileList--;
			}
		}

		for (i = 0; i < cFileList; i++) {
			lstrcpy(lpCmdLineNew + cchiFileList, L" /n - ");
			lstrcat(lpCmdLineNew, lpFileList[i]);
			LocalFree(lpFileList[i]);

			STARTUPINFO si;
			ZeroMemory(&si, sizeof(STARTUPINFO));
			si.cb = sizeof(STARTUPINFO);
			PROCESS_INFORMATION pi;
			ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));

			if (CreateProcess(NULL, lpCmdLineNew, NULL, NULL, FALSE, 0, NULL, g_wchWorkingDirectory, &si, &pi)) {
				CloseHandle(pi.hProcess);
				CloseHandle(pi.hThread);
			}
		}

		LocalFree(lpCmdLineNew);
		NP2HeapFree(lp1);
		NP2HeapFree(lp2);
		NP2HeapFree(lpFileArg);

		return TRUE;
	}

	for (int i = 0; i < cFileList; i++) {
		LocalFree(lpFileList[i]);
	}
	return FALSE;
}

void GetRelaunchParameters(LPWSTR szParameters, LPCWSTR lpszFile, BOOL newWind, BOOL emptyWind) {
	WCHAR tch[64];
	wsprintf(tch, L"-appid=\"%s\"", g_wchAppUserModelID);
	lstrcpy(szParameters, tch);

	wsprintf(tch, L" -sysmru=%i", (flagUseSystemMRU == 2));
	lstrcat(szParameters, tch);

	lstrcat(szParameters, L" -f");
	if (StrNotEmpty(szIniFile)) {
		lstrcat(szParameters, L" \"");
		lstrcat(szParameters, szIniFile);
		lstrcat(szParameters, L"\"");
	} else {
		lstrcat(szParameters, L"0");
	}

	if (newWind) {
		lstrcat(szParameters, L" -n");
	}

	WINDOWPLACEMENT wndpl;
	wndpl.length = sizeof(WINDOWPLACEMENT);
	GetWindowPlacement(hwndMain, &wndpl);

	HMONITOR hMonitor = MonitorFromRect(&wndpl.rcNormalPosition, MONITOR_DEFAULTTONEAREST);
	MONITORINFO mi;
	mi.cbSize = sizeof(mi);
	GetMonitorInfo(hMonitor, &mi);

	// offset new window position +10/+10
	int x = wndpl.rcNormalPosition.left + (newWind? 10 : 0);
	int y = wndpl.rcNormalPosition.top	+ (newWind? 10 : 0);
	const int cx = wndpl.rcNormalPosition.right - wndpl.rcNormalPosition.left;
	const int cy = wndpl.rcNormalPosition.bottom - wndpl.rcNormalPosition.top;

	// check if window fits monitor
	if ((x + cx) > mi.rcWork.right || (y + cy) > mi.rcWork.bottom) {
		x = mi.rcMonitor.left;
		y = mi.rcMonitor.top;
	}

	const BOOL imax = IsZoomed(hwndMain);
	wsprintf(tch, L" -pos %i,%i,%i,%i,%i", x, y, cx, cy, imax);
	lstrcat(szParameters, tch);

	if (!emptyWind && StrNotEmpty(lpszFile)) {
		// file encoding
		switch (iEncoding) {
		case CPI_DEFAULT:
			lstrcat(szParameters, L" -ansi");
			break;

		case CPI_UNICODE:
		case CPI_UNICODEBOM:
			lstrcat(szParameters, L" -utf16le");
			break;

		case CPI_UNICODEBE:
		case CPI_UNICODEBEBOM:
			lstrcat(szParameters, L" -utf16be");
			break;

		case CPI_UTF8:
			lstrcat(szParameters, L" -utf8");
			break;

		case CPI_UTF8SIGN:
			lstrcat(szParameters, L" -utf8sig");
			break;

		default: {
			const char *enc = mEncoding[iEncoding].pszParseNames;
			const char *sep = strchr(enc, ',');
			ZeroMemory(tch, sizeof(tch));
			MultiByteToWideChar(CP_UTF8, 0, enc, (int)(sep - enc), tch, COUNTOF(tch));
			lstrcat(szParameters, L" -e \"");
			lstrcat(szParameters, tch);
			lstrcat(szParameters, L"\"");
		}
		}

		// scheme
		switch (pLexCurrent->rid) {
		case NP2LEX_DEFAULT:
			lstrcat(szParameters, L" -d");
			break;

		case NP2LEX_HTML:
			lstrcat(szParameters, L" -h");
			break;

		case NP2LEX_XML:
			lstrcat(szParameters, L" -x");
			break;

		default:
			lstrcat(szParameters, L" -s \"");
			lstrcat(szParameters, pLexCurrent->pszName);
			lstrcat(szParameters, L"\"");
			break;
		}

		// position
		const Sci_Position pos = SciCall_GetCurrentPos();
		if (pos > 0) {
			x = SciCall_LineFromPosition(pos) + 1;
			y = SciCall_GetColumn(pos) + 1;
			wsprintf(tch, L" -g %i,%i", x, y);
			lstrcat(szParameters, tch);
		}

		WCHAR szFileName[MAX_PATH + 4];
		lstrcpy(szFileName, lpszFile);
		PathQuoteSpaces(szFileName);
		lstrcat(szParameters, L" ");
		lstrcat(szParameters, szFileName);
	}
}

//=============================================================================
//
// RelaunchElevated()
//
//
BOOL RelaunchElevated(void) {
	if (!IsVistaAndAbove() || fIsElevated || !flagRelaunchElevated || flagDisplayHelp) {
		return FALSE;
	}
	{
		LPWSTR lpArg1;
		LPWSTR lpArg2;
		BOOL exit = TRUE;

		if (flagRelaunchElevated == 2) {
			WCHAR tchFile[MAX_PATH];
			lstrcpy(tchFile, szCurFile);
			if (!FileSave(FALSE, TRUE, FALSE, FALSE)) {
				return FALSE;
			}

			exit = StrCaseEqual(tchFile, szCurFile);
			lpArg1 = (LPWSTR)NP2HeapAlloc(sizeof(WCHAR) * MAX_PATH);
			GetModuleFileName(NULL, lpArg1, MAX_PATH);
			lpArg2 = (LPWSTR)NP2HeapAlloc(sizeof(WCHAR) * 1024);
			GetRelaunchParameters(lpArg2, tchFile, !exit, FALSE);
			exit = !IsDocumentModified();
		} else {
			const LPCWSTR lpCmdLine = GetCommandLine();
			const size_t cmdSize = sizeof(WCHAR) * (lstrlen(lpCmdLine) + 1);
			lpArg1 = (LPWSTR)NP2HeapAlloc(cmdSize);
			lpArg2 = (LPWSTR)NP2HeapAlloc(cmdSize);
			ExtractFirstArgument(lpCmdLine, lpArg1, lpArg2);
		}

		if (StrNotEmpty(lpArg1)) {
			SHELLEXECUTEINFO sei;
			ZeroMemory(&sei, sizeof(SHELLEXECUTEINFO));
			sei.cbSize = sizeof(SHELLEXECUTEINFO);
			sei.fMask = SEE_MASK_FLAG_NO_UI | SEE_MASK_NOASYNC | SEE_MASK_NOZONECHECKS;
			sei.hwnd = GetForegroundWindow();
			sei.lpVerb = L"runas";
			sei.lpFile = lpArg1;
			sei.lpParameters = lpArg2;
			sei.lpDirectory = g_wchWorkingDirectory;
			sei.nShow = SW_SHOWNORMAL;

			ShellExecuteEx(&sei);
		} else {
			exit = FALSE;
		}

		NP2HeapFree(lpArg1);
		NP2HeapFree(lpArg2);
		return exit;
	}
}

//=============================================================================
//
// SnapToDefaultPos()
//
// Aligns Notepad2 to the default window position on the current screen
//
//
void SnapToDefaultPos(HWND hwnd) {
	RECT rcOld;
	GetWindowRect(hwnd, &rcOld);

	HMONITOR hMonitor = MonitorFromRect(&rcOld, MONITOR_DEFAULTTONEAREST);
	MONITORINFO mi;
	mi.cbSize = sizeof(mi);
	GetMonitorInfo(hMonitor, &mi);

	const int y = mi.rcWork.top + 16;
	const int cy = mi.rcWork.bottom - mi.rcWork.top - 32;
	const int cx = min_i(mi.rcWork.right - mi.rcWork.left - 32, cy);
	int x = mi.rcWork.right - cx - 16;

	WINDOWPLACEMENT wndpl;
	wndpl.length = sizeof(WINDOWPLACEMENT);
	wndpl.flags = WPF_ASYNCWINDOWPLACEMENT;
	wndpl.showCmd = SW_RESTORE;

	wndpl.rcNormalPosition.left = x;
	wndpl.rcNormalPosition.top = y;
	wndpl.rcNormalPosition.right = x + cx;
	wndpl.rcNormalPosition.bottom = y + cy;

	if (EqualRect(&rcOld, &wndpl.rcNormalPosition)) {
		x = mi.rcWork.left + 16;
		wndpl.rcNormalPosition.left = x;
		wndpl.rcNormalPosition.right = x + cx;
	}

	if (GetDoAnimateMinimize()) {
		DrawAnimatedRects(hwnd, IDANI_CAPTION, &rcOld, &wndpl.rcNormalPosition);
		OffsetRect(&wndpl.rcNormalPosition, mi.rcMonitor.left - mi.rcWork.left, mi.rcMonitor.top - mi.rcWork.top);
	}

	SetWindowPlacement(hwnd, &wndpl);
}

//=============================================================================
//
// ShowNotifyIcon()
//
//
void ShowNotifyIcon(HWND hwnd, BOOL bAdd) {
	static HICON hIcon;

	if (!hIcon) {
		hIcon = (HICON)LoadImage(g_hInstance, MAKEINTRESOURCE(IDR_MAINWND), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
	}

	NOTIFYICONDATA nid;
	ZeroMemory(&nid, sizeof(NOTIFYICONDATA));
	nid.cbSize = sizeof(NOTIFYICONDATA);
	nid.hWnd = hwnd;
	nid.uID = 0;
	nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
	nid.uCallbackMessage = APPM_TRAYMESSAGE;
	nid.hIcon = hIcon;
	lstrcpy(nid.szTip, L"Notepad2");

	if (bAdd) {
		Shell_NotifyIcon(NIM_ADD, &nid);
	} else {
		Shell_NotifyIcon(NIM_DELETE, &nid);
	}
}

//=============================================================================
//
// SetNotifyIconTitle()
//
//
void SetNotifyIconTitle(HWND hwnd) {
	NOTIFYICONDATA nid;
	ZeroMemory(&nid, sizeof(NOTIFYICONDATA));
	nid.cbSize = sizeof(NOTIFYICONDATA);
	nid.hWnd = hwnd;
	nid.uID = 0;
	nid.uFlags = NIF_TIP;

	WCHAR tchTitle[128];
	if (StrNotEmpty(szTitleExcerpt)) {
		WCHAR tchFormat[32];
		FormatString(tchTitle, tchFormat, IDS_TITLEEXCERPT, szTitleExcerpt);
	} else if (StrNotEmpty(szCurFile)) {
		SHFILEINFO shfi;
		SHGetFileInfo2(szCurFile, 0, &shfi, sizeof(SHFILEINFO), SHGFI_DISPLAYNAME);
		PathCompactPathEx(tchTitle, shfi.szDisplayName, COUNTOF(tchTitle) - 4, 0);
	} else {
		GetString(IDS_UNTITLED, tchTitle, COUNTOF(tchTitle) - 4);
	}

	if (IsDocumentModified()) {
		lstrcpy(nid.szTip, L"* ");
	} else {
		lstrcpy(nid.szTip, L"");
	}
	lstrcat(nid.szTip, tchTitle);

	Shell_NotifyIcon(NIM_MODIFY, &nid);
}

//=============================================================================
//
// InstallFileWatching()
//
//
void InstallFileWatching(LPCWSTR lpszFile) {
	// Terminate
	if (!iFileWatchingMode || StrIsEmpty(lpszFile)) {
		if (bRunningWatch) {
			if (hChangeHandle) {
				FindCloseChangeNotification(hChangeHandle);
				hChangeHandle = NULL;
			}
			KillTimer(NULL, ID_WATCHTIMER);
			bRunningWatch = FALSE;
			dwChangeNotifyTime = 0;
		}
	} else { // Install
		// Terminate previous watching
		if (bRunningWatch) {
			if (hChangeHandle) {
				FindCloseChangeNotification(hChangeHandle);
				hChangeHandle = NULL;
			}
			dwChangeNotifyTime = 0;
		} else {
			// No previous watching installed, so launch the timer first
			SetTimer(NULL, ID_WATCHTIMER, dwFileCheckInverval, WatchTimerProc);
		}

		WCHAR tchDirectory[MAX_PATH];
		lstrcpy(tchDirectory, lpszFile);
		PathRemoveFileSpec(tchDirectory);

		// Save data of current file
		HANDLE hFind = FindFirstFile(szCurFile, &fdCurFile);
		if (hFind != INVALID_HANDLE_VALUE) {
			FindClose(hFind);
		} else {
			ZeroMemory(&fdCurFile, sizeof(WIN32_FIND_DATA));
		}

		hChangeHandle = FindFirstChangeNotification(tchDirectory, FALSE,
						FILE_NOTIFY_CHANGE_FILE_NAME	| \
						FILE_NOTIFY_CHANGE_DIR_NAME		| \
						FILE_NOTIFY_CHANGE_ATTRIBUTES	| \
						FILE_NOTIFY_CHANGE_SIZE			| \
						FILE_NOTIFY_CHANGE_LAST_WRITE);

		bRunningWatch = TRUE;
		dwChangeNotifyTime = 0;
	}
}

//=============================================================================
//
// WatchTimerProc()
//
//
void CALLBACK WatchTimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime) {
	UNREFERENCED_PARAMETER(hwnd);
	UNREFERENCED_PARAMETER(uMsg);
	UNREFERENCED_PARAMETER(idEvent);
	UNREFERENCED_PARAMETER(dwTime);

	if (bRunningWatch) {
		if (dwChangeNotifyTime > 0 && GetTickCount() - dwChangeNotifyTime > dwAutoReloadTimeout) {
			if (hChangeHandle) {
				FindCloseChangeNotification(hChangeHandle);
				hChangeHandle = NULL;
			}
			KillTimer(NULL, ID_WATCHTIMER);
			bRunningWatch = FALSE;
			dwChangeNotifyTime = 0;
			SendMessage(hwndMain, APPM_CHANGENOTIFY, 0, 0);
		}

		// Check Change Notification Handle
		else if (WAIT_OBJECT_0 == WaitForSingleObject(hChangeHandle, 0)) {
			// Check if the changes affect the current file
			WIN32_FIND_DATA fdUpdated;
			HANDLE hFind = FindFirstFile(szCurFile, &fdUpdated);
			if (INVALID_HANDLE_VALUE != hFind) {
				FindClose(hFind);
			} else { // The current file has been removed
				ZeroMemory(&fdUpdated, sizeof(WIN32_FIND_DATA));
			}

			// Check if the file has been changed
			if (CompareFileTime(&fdCurFile.ftLastWriteTime, &fdUpdated.ftLastWriteTime) != 0 ||
					fdCurFile.nFileSizeLow != fdUpdated.nFileSizeLow ||
					fdCurFile.nFileSizeHigh != fdUpdated.nFileSizeHigh) {
				// Shutdown current watching and give control to main window
				if (hChangeHandle) {
					FindCloseChangeNotification(hChangeHandle);
					hChangeHandle = NULL;
				}
				if (iFileWatchingMode == 2) {
					bRunningWatch = TRUE; /* ! */
					dwChangeNotifyTime = GetTickCount();
				} else {
					KillTimer(NULL, ID_WATCHTIMER);
					bRunningWatch = FALSE;
					dwChangeNotifyTime = 0;
					SendMessage(hwndMain, APPM_CHANGENOTIFY, 0, 0);
				}
			} else {
				FindNextChangeNotification(hChangeHandle);
			}
		}
	}
}

//=============================================================================
//
// PasteBoardTimer()
//
//
void CALLBACK PasteBoardTimer(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime) {
	UNREFERENCED_PARAMETER(hwnd);
	UNREFERENCED_PARAMETER(uMsg);
	UNREFERENCED_PARAMETER(idEvent);
	UNREFERENCED_PARAMETER(dwTime);

	if (dwLastCopyTime > 0 && GetTickCount() - dwLastCopyTime > 200) {
		if (SendMessage(hwndEdit, SCI_CANPASTE, 0, 0)) {
			const BOOL back = autoCompletionConfig.bIndentText;
			autoCompletionConfig.bIndentText = FALSE;
			EditJumpTo(hwndEdit, -1, 0);
			SendMessage(hwndEdit, SCI_BEGINUNDOACTION, 0, 0);
			if (SendMessage(hwndEdit, SCI_GETLENGTH, 0, 0) > 0) {
				SendMessage(hwndEdit, SCI_NEWLINE, 0, 0);
			}
			SendMessage(hwndEdit, SCI_PASTE, 0, 0);
			SendMessage(hwndEdit, SCI_NEWLINE, 0, 0);
			SendMessage(hwndEdit, SCI_ENDUNDOACTION, 0, 0);
			EditEnsureSelectionVisible(hwndEdit);
			autoCompletionConfig.bIndentText = back;
		}

		dwLastCopyTime = 0;
	}
}

// End of Notepad2.c
