/******************************************************************************
*
*
* Notepad2
*
* Edit.c
*   Text File Editing Helper Stuff
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
#include <shlwapi.h>
#include <commctrl.h>
#include <commdlg.h>
#if 0
#include <uxtheme.h>
#include <vssym32.h>
#endif
#include <stdio.h>
#include <limits.h>
#include <stdint.h>
#include <inttypes.h>
#include "Notepad2.h"
#include "Edit.h"
#include "Styles.h"
#include "Helpers.h"
#include "Dialogs.h"
#include "SciCall.h"
#include "resource.h"

extern HWND hwndMain;
extern DWORD dwLastIOError;
extern HWND hDlgFindReplace;
extern UINT cpLastFind;
extern BOOL bReplaceInitialized;

extern int xFindReplaceDlg;
extern int yFindReplaceDlg;
extern int cxFindReplaceDlg;

extern int iDefaultEncoding;
extern int iDefaultEOLMode;
extern BOOL bFixLineEndings;
extern BOOL bAutoStripBlanks;

extern int iRenderingTechnology;
extern int iBidirectional;
extern BOOL bUseInlineIME;
extern BOOL bInlineIMEUseBlockCaret;
// Default Codepage and Character Set
extern int iDefaultCodePage;
//extern int iDefaultCharSet;
extern BOOL bLoadANSIasUTF8;
extern BOOL bLoadNFOasOEM;
extern int iSrcEncoding;
extern int iWeakSrcEncoding;

extern int g_DOSEncoding;

extern LPMRULIST mruFind;
extern LPMRULIST mruReplace;
extern WCHAR szCurFile[MAX_PATH + 40];

static DString wchPrefixSelection;
static DString wchAppendSelection;
static DString wchPrefixLines;
static DString wchAppendLines;

void Edit_ReleaseResources(void) {
	DString_Free(&wchPrefixSelection);
	DString_Free(&wchAppendSelection);
	DString_Free(&wchPrefixLines);
	DString_Free(&wchAppendLines);
}

//=============================================================================
//
// EditCreate()
//
HWND EditCreate(HWND hwndParent) {
	HWND hwnd = CreateWindowEx(WS_EX_CLIENTEDGE,
						  L"Scintilla",
						  NULL,
						  WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS,
						  0, 0, 0, 0,
						  hwndParent,
						  (HMENU)IDC_EDIT,
						  g_hInstance,
						  NULL);

	InitScintillaHandle(hwnd);
	SendMessage(hwnd, SCI_SETBUFFEREDDRAW, (iRenderingTechnology == SC_TECHNOLOGY_DEFAULT), 0);
	SendMessage(hwnd, SCI_SETTECHNOLOGY, iRenderingTechnology, 0);
	SendMessage(hwnd, SCI_SETBIDIRECTIONAL, iBidirectional, 0);
	SendMessage(hwnd, SCI_SETIMEINTERACTION, bUseInlineIME, 0);
	SendMessage(hwnd, SCI_SETINLINEIMEUSEBLOCKCARET, bInlineIMEUseBlockCaret, 0);
	SendMessage(hwnd, SCI_SETPASTECONVERTENDINGS, 1, 0);
	SendMessage(hwnd, SCI_SETMODEVENTMASK, /*SC_MODEVENTMASKALL*/SC_MOD_INSERTTEXT | SC_MOD_DELETETEXT, 0);
	SendMessage(hwnd, SCI_SETCOMMANDEVENTS, FALSE, 0);
	SendMessage(hwnd, SCI_USEPOPUP, FALSE, 0);
	SendMessage(hwnd, SCI_SETSCROLLWIDTH, 2048, 0);
	SendMessage(hwnd, SCI_SETSCROLLWIDTHTRACKING, TRUE, 0);
	SendMessage(hwnd, SCI_SETENDATLASTLINE, TRUE, 0);
	SendMessage(hwnd, SCI_SETCARETSTICKY, SC_CARETSTICKY_OFF, 0);
	SendMessage(hwnd, SCI_SETXCARETPOLICY, CARET_SLOP | CARET_EVEN, 50);
	SendMessage(hwnd, SCI_SETYCARETPOLICY, CARET_EVEN, 0);
	SendMessage(hwnd, SCI_SETMULTIPLESELECTION, FALSE, 0);
	SendMessage(hwnd, SCI_SETADDITIONALSELECTIONTYPING, FALSE, 0);
	SendMessage(hwnd, SCI_SETVIRTUALSPACEOPTIONS, SCVS_NONE, 0);
	SendMessage(hwnd, SCI_SETADDITIONALCARETSBLINK, FALSE, 0);
	SendMessage(hwnd, SCI_SETADDITIONALCARETSVISIBLE, FALSE, 0);
	// style both before and after the visible text in the background
	SendMessage(hwnd, SCI_SETIDLESTYLING, SC_IDLESTYLING_ALL, 0);
	// cache layout for visible lines
	SendMessage(hwnd, SCI_SETLAYOUTCACHE, SC_CACHE_PAGE, 0);

	SendMessage(hwnd, SCI_ASSIGNCMDKEY, (SCK_NEXT + (SCMOD_CTRL << 16)), SCI_PARADOWN);
	SendMessage(hwnd, SCI_ASSIGNCMDKEY, (SCK_PRIOR + (SCMOD_CTRL << 16)), SCI_PARAUP);
	SendMessage(hwnd, SCI_ASSIGNCMDKEY, (SCK_NEXT + ((SCMOD_CTRL | SCMOD_SHIFT) << 16)), SCI_PARADOWNEXTEND);
	SendMessage(hwnd, SCI_ASSIGNCMDKEY, (SCK_PRIOR + ((SCMOD_CTRL | SCMOD_SHIFT) << 16)), SCI_PARAUPEXTEND);
	SendMessage(hwnd, SCI_ASSIGNCMDKEY, (SCK_HOME + (0 << 16)), SCI_VCHOMEWRAP);
	SendMessage(hwnd, SCI_ASSIGNCMDKEY, (SCK_END + (0 << 16)), SCI_LINEENDWRAP);
	SendMessage(hwnd, SCI_ASSIGNCMDKEY, (SCK_HOME + (SCMOD_SHIFT << 16)), SCI_VCHOMEWRAPEXTEND);
	SendMessage(hwnd, SCI_ASSIGNCMDKEY, (SCK_END + (SCMOD_SHIFT << 16)), SCI_LINEENDWRAPEXTEND);

	return hwnd;
}

//=============================================================================
//
// EditSetNewText()
//
extern BOOL bFreezeAppTitle;
extern FILEVARS fvCurFile;

void EditSetNewText(HWND hwnd, LPCSTR lpstrText, DWORD cbText) {
	bFreezeAppTitle = TRUE;

	if (SendMessage(hwnd, SCI_GETREADONLY, 0, 0)) {
		SendMessage(hwnd, SCI_SETREADONLY, FALSE, 0);
	}

	SendMessage(hwnd, SCI_CANCEL, 0, 0);
	SendMessage(hwnd, SCI_SETUNDOCOLLECTION, 0, 0);
	SendMessage(hwnd, SCI_EMPTYUNDOBUFFER, 0, 0);
	SendMessage(hwnd, SCI_CLEARALL, 0, 0);
	SendMessage(hwnd, SCI_MARKERDELETEALL, (WPARAM)(-1), 0);
	SendMessage(hwnd, SCI_SETSCROLLWIDTH, 2048, 0);
	SendMessage(hwnd, SCI_SETXOFFSET, 0, 0);

	FileVars_Apply(hwnd, &fvCurFile);

	if (cbText > 0) {
		SendMessage(hwnd, SCI_ADDTEXT, cbText, (LPARAM)lpstrText);
	}

	SendMessage(hwnd, SCI_SETUNDOCOLLECTION, 1, 0);
	SendMessage(hwnd, EM_EMPTYUNDOBUFFER, 0, 0);
	SendMessage(hwnd, SCI_SETSAVEPOINT, 0, 0);
	SendMessage(hwnd, SCI_GOTOPOS, 0, 0);
	SendMessage(hwnd, SCI_CHOOSECARETX, 0, 0);

	bFreezeAppTitle = FALSE;
}

//=============================================================================
//
// EditConvertText()
//
BOOL EditConvertText(HWND hwnd, UINT cpSource, UINT cpDest, BOOL bSetSavePoint) {
	if (cpSource == cpDest) {
		return TRUE;
	}

	const int length = (int)SendMessage(hwnd, SCI_GETLENGTH, 0, 0);
	if (length == 0) {
		SendMessage(hwnd, SCI_CANCEL, 0, 0);
		SendMessage(hwnd, SCI_SETUNDOCOLLECTION, 0, 0);
		SendMessage(hwnd, SCI_EMPTYUNDOBUFFER, 0, 0);
		SendMessage(hwnd, SCI_CLEARALL, 0, 0);
		SendMessage(hwnd, SCI_MARKERDELETEALL, (WPARAM)(-1), 0);
		SendMessage(hwnd, SCI_SETCODEPAGE, cpDest, 0);
		SendMessage(hwnd, SCI_SETUNDOCOLLECTION, 1, 0);
		SendMessage(hwnd, EM_EMPTYUNDOBUFFER, 0, 0);
		SendMessage(hwnd, SCI_GOTOPOS, 0, 0);
		SendMessage(hwnd, SCI_CHOOSECARETX, 0, 0);

		if (bSetSavePoint) {
			SendMessage(hwnd, SCI_SETSAVEPOINT, 0, 0);
		}
	} else {
		// DBCS: length -> WCHAR: sizeof(WCHAR) * (length / 2) -> UTF-8: kMaxMultiByteCount * (length / 2)
		struct Sci_TextRange tr = { { 0, -1 }, NULL };
		char *pchText = (char *)NP2HeapAlloc((length + 1) * sizeof(WCHAR));
		tr.lpstrText = pchText;
		SendMessage(hwnd, SCI_GETTEXTRANGE, 0, (LPARAM)&tr);

		WCHAR *pwchText = (WCHAR *)NP2HeapAlloc((length + 1) * sizeof(WCHAR));
		const int cbwText = MultiByteToWideChar(cpSource, 0, pchText, length, pwchText, length);
		const int cbText = WideCharToMultiByte(cpDest, 0, pwchText, cbwText, pchText, (int)(length * sizeof(WCHAR)), NULL, NULL);

		SendMessage(hwnd, SCI_CANCEL, 0, 0);
		SendMessage(hwnd, SCI_SETUNDOCOLLECTION, 0, 0);
		SendMessage(hwnd, SCI_EMPTYUNDOBUFFER, 0, 0);
		SendMessage(hwnd, SCI_CLEARALL, 0, 0);
		SendMessage(hwnd, SCI_MARKERDELETEALL, (WPARAM)(-1), 0);
		SendMessage(hwnd, SCI_SETCODEPAGE, cpDest, 0);
		SendMessage(hwnd, SCI_ADDTEXT, cbText, (LPARAM)pchText);
		SendMessage(hwnd, SCI_EMPTYUNDOBUFFER, 0, 0);
		SendMessage(hwnd, SCI_SETUNDOCOLLECTION, 1, 0);
		SendMessage(hwnd, SCI_GOTOPOS, 0, 0);
		SendMessage(hwnd, SCI_CHOOSECARETX, 0, 0);

		NP2HeapFree(pchText);
		NP2HeapFree(pwchText);
	}

	return TRUE;
}

//=============================================================================
//
// EditGetClipboardText()
//
char* EditGetClipboardText(HWND hwnd) {
	if (!IsClipboardFormatAvailable(CF_UNICODETEXT) || !OpenClipboard(GetParent(hwnd))) {
		return NULL;
	}

	HANDLE hmem = GetClipboardData(CF_UNICODETEXT);
	LPCWSTR pwch = (LPCWSTR)GlobalLock(hmem);
	const int wlen = lstrlen(pwch);

	const UINT cpEdit = (UINT)SendMessage(hwnd, SCI_GETCODEPAGE, 0, 0);
	const int iEOLMode = (int)SendMessage(hwnd, SCI_GETEOLMODE, 0, 0);

	const int mlen = WideCharToMultiByte(cpEdit, 0, pwch, wlen + 1, NULL, 0, NULL, NULL) - 1;
	char *pmch = (char *)LocalAlloc(LPTR, mlen + 1);
	char *ptmp = (char *)NP2HeapAlloc(mlen * 4 + 1);

	if (pmch && ptmp) {
		const char *s = pmch;
		char *d = ptmp;

		WideCharToMultiByte(cpEdit, 0, pwch, wlen + 1, pmch, mlen + 1, NULL, NULL);

		for (int i = 0; (i < mlen) && (*s != 0); i++) {
			if (*s == '\n' || *s == '\r') {
				if (iEOLMode == SC_EOL_CR) {
					*d++ = '\r';
				} else if (iEOLMode == SC_EOL_LF) {
					*d++ = '\n';
				} else { // iEOLMode == SC_EOL_CRLF
					*d++ = '\r';
					*d++ = '\n';
				}
				if ((*s == '\r') && (i + 1 < mlen) && (*(s + 1) == '\n')) {
					i++;
					s++;
				}
				s++;
			} else {
				*d++ = *s++;
			}
		}

		*d++ = 0;
		LocalFree(pmch);
		pmch = (char *)LocalAlloc(LPTR, (d - ptmp));
		strcpy(pmch, ptmp);
		NP2HeapFree(ptmp);
	}

	GlobalUnlock(hmem);
	CloseClipboard();

	return pmch;
}

//=============================================================================
//
// EditCopyAppend()
//
BOOL EditCopyAppend(HWND hwnd) {
	if (!IsClipboardFormatAvailable(CF_UNICODETEXT)) {
		SendMessage(hwnd, SCI_COPY, 0, 0);
		return TRUE;
	}

	const int iCurPos = (int)SendMessage(hwnd, SCI_GETCURRENTPOS, 0, 0);
	const int iAnchorPos = (int)SendMessage(hwnd, SCI_GETANCHOR, 0, 0);
	char *pszText;

	if (iCurPos != iAnchorPos) {
		if (SC_SEL_RECTANGLE == SendMessage(hwnd, SCI_GETSELECTIONMODE, 0, 0)) {
			MsgBox(MBWARN, IDS_SELRECT);
			return FALSE;
		}

		const int iSelCount = (int)SendMessage(hwnd, SCI_GETSELTEXT, 0, 0);
		pszText = (char *)NP2HeapAlloc(iSelCount);
		SendMessage(hwnd, SCI_GETSELTEXT, 0, (LPARAM)pszText);
	} else {
		const int cchText = (int)SendMessage(hwnd, SCI_GETLENGTH, 0, 0);
		pszText = (char *)NP2HeapAlloc(cchText + 1);
		SendMessage(hwnd, SCI_GETTEXT, NP2HeapSize(pszText), (LPARAM)pszText);
	}

	const UINT cpEdit = (UINT)SendMessage(hwnd, SCI_GETCODEPAGE, 0, 0);
	const int cchTextW = MultiByteToWideChar(cpEdit, 0, pszText, -1, NULL, 0);

	WCHAR *pszTextW = NULL;
	if (cchTextW > 0) {
		const WCHAR *pszSep = L"\r\n\r\n";
		pszTextW = (WCHAR *)NP2HeapAlloc(sizeof(WCHAR) * (CSTRLEN(L"\r\n\r\n") + cchTextW + 1));
		lstrcpy(pszTextW, pszSep);
		MultiByteToWideChar(cpEdit, 0, pszText, -1, StrEnd(pszTextW), (int)(NP2HeapSize(pszTextW) / sizeof(WCHAR)));
	}

	NP2HeapFree(pszText);

	BOOL succ = FALSE;
	if (OpenClipboard(GetParent(hwnd))) {
		HANDLE hOld = GetClipboardData(CF_UNICODETEXT);
		LPCWSTR pszOld = (LPCWSTR)GlobalLock(hOld);

		HANDLE hNew = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT,
						   sizeof(WCHAR) * (lstrlen(pszOld) + lstrlen(pszTextW) + 1));
		WCHAR *pszNew = (WCHAR *)GlobalLock(hNew);

		lstrcpy(pszNew, pszOld);
		lstrcat(pszNew, pszTextW);

		GlobalUnlock(hOld);
		GlobalUnlock(hNew);

		EmptyClipboard();
		SetClipboardData(CF_UNICODETEXT, hNew);
		CloseClipboard();

		succ = TRUE;
	}

	if (pszTextW != NULL) {
		NP2HeapFree(pszTextW);
	}
	return succ;
}

//=============================================================================
//
// EditDetectEOLMode() - moved here to handle Unicode files correctly
//
void EditDetectEOLMode(LPCSTR lpData, DWORD cbData, EditFileIOStatus *status) {
	int iEOLMode = iLineEndings[iDefaultEOLMode];

	if (cbData == 0) {
		status->iEOLMode = iEOLMode;
		return;
	}

	UINT linesCount[3] = { 0, 0, 0 };
#if 0
	StopWatch watch;
	StopWatch_Start(watch);
#endif

#if 0
	LPCSTR cp = lpData;
	LPCSTR const end = cp + cbData;
	while (cp < end) {
		if (*cp == '\r') {
			if (*(cp + 1) == '\n') {
				++cp;
				++linesCount[SC_EOL_CRLF];
			} else {
				++linesCount[SC_EOL_CR];
			}
		} else if (*cp == '\n') {
			++linesCount[SC_EOL_LF];
		}
		++cp;
	}
#endif

#if 0
	LPCSTR cp = lpData;
	LPCSTR const end = cp + cbData;
	while (cp < end) {
		switch (*cp++) {
		case '\n':
			++linesCount[SC_EOL_LF];
			break;
		case '\r':
			if (*cp == '\n') {
				++cp;
				++linesCount[SC_EOL_CRLF];
			} else {
				++linesCount[SC_EOL_CR];
			}
			break;
		}
	}
#endif

#if 0
	// tools/GenerateTable.py
	static const UINT8 eol_table[256] = {
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 2, 0, 0, // 00 - 0F
	};

	const UINT8 *ptr = (const UINT8 *)lpData;
	const UINT8 * const end = ptr + cbData;
	UINT type = 0;
	do {
		// skip to line end
		while (ptr < end && (type = eol_table[*ptr++]) == 0) {
			// nop
		}
		switch (type) {
		case 1: // '\n'
			++linesCount[SC_EOL_LF];
			break;
		case 2: // '\r'
			if (*ptr == '\n') {
				++ptr;
				++linesCount[SC_EOL_CRLF];
			} else {
				++linesCount[SC_EOL_CR];
			}
			break;
		}
	} while (ptr < end);
#endif

#if 1
	// tools/GenerateTable.py
	static const UINT8 eol_table[16] = {
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 2, 0, 0, // 00 - 0F
	};

	const UINT8 *ptr = (const UINT8 *)lpData;
	const UINT8 * const end = ptr + cbData;
	do {
		// skip to line end
		UINT ch;
		UINT type = 0;
		while (ptr < end && ((ch = *ptr++) > '\r' || (type = eol_table[ch]) == 0)) {
			// nop
		}
		switch (type) {
		case 1: //'\n'
			++linesCount[SC_EOL_LF];
			break;
		case 2: //'\r'
			if (*ptr == '\n') {
				++ptr;
				++linesCount[SC_EOL_CRLF];
			} else {
				++linesCount[SC_EOL_CR];
			}
			break;
		}
	} while (ptr < end);
#endif

	const UINT linesMax = max_u(max_u(linesCount[0], linesCount[1]), linesCount[2]);
	if (linesMax != linesCount[iEOLMode]) {
		if (linesMax == linesCount[SC_EOL_CRLF]) {
			iEOLMode = SC_EOL_CRLF;
		} else if (linesMax == linesCount[SC_EOL_LF]) {
			iEOLMode = SC_EOL_LF;
		} else {
			iEOLMode = SC_EOL_CR;
		}
	}

#if 0
	StopWatch_Stop(watch);
	StopWatch_Show(&watch, L"EOL time");
#endif

	status->iEOLMode = iEOLMode;
	status->bInconsistent = ((!!linesCount[0]) + (!!linesCount[1]) + (!!linesCount[2])) > 1;
	status->linesCount[0] = linesCount[SC_EOL_CRLF];
	status->linesCount[1] = linesCount[SC_EOL_LF];
	status->linesCount[2] = linesCount[SC_EOL_CR];
}

//=============================================================================
//
// EditLoadFile()
//
extern DWORD dwFileLoadWarningMB;
BOOL EditLoadFile(HWND hwnd, LPWSTR pszFile, BOOL bSkipEncodingDetection, EditFileIOStatus *status) {
	HANDLE hFile = CreateFile(pszFile,
					   GENERIC_READ,
					   FILE_SHARE_READ | FILE_SHARE_WRITE,
					   NULL, OPEN_EXISTING,
					   FILE_ATTRIBUTE_NORMAL,
					   NULL);
	dwLastIOError = GetLastError();

	if (hFile == INVALID_HANDLE_VALUE) {
		iSrcEncoding = -1;
		iWeakSrcEncoding = -1;
		return FALSE;
	}

	// calculate buffer limit
	const DWORD dwFileSize = GetFileSize(hFile, NULL);

	// Check if a warning message should be displayed for large files
	if (dwFileLoadWarningMB != 0 && dwFileLoadWarningMB * 1024 * 1024 < dwFileSize) {
		if (InfoBox(MBYESNO, L"MsgFileSizeWarning", IDS_WARNLOADBIGFILE) != IDYES) {
			CloseHandle(hFile);
			status->bFileTooBig = TRUE;
			iSrcEncoding = -1;
			iWeakSrcEncoding = -1;
			return FALSE;
		}
	}

	// display real path name
	if (IsVistaAndAbove()) {
		// since since Windows Vista
		typedef DWORD (WINAPI *GetFinalPathNameByHandleSig)(HANDLE hFile, LPWSTR lpszFilePath, DWORD cchFilePath, DWORD dwFlags);
		WCHAR path[MAX_PATH] = L"";
#if _WIN32_WINNT >= _WIN32_WINNT_VISTA
		GetFinalPathNameByHandleSig pfnGetFinalPathNameByHandle = GetFinalPathNameByHandleW;
#else
		GetFinalPathNameByHandleSig pfnGetFinalPathNameByHandle = (GetFinalPathNameByHandleSig)GetProcAddress(GetModuleHandle(L"kernel32.dll"), "GetFinalPathNameByHandleW");
#endif
		if (pfnGetFinalPathNameByHandle && pfnGetFinalPathNameByHandle(hFile, path, MAX_PATH, FILE_NAME_OPENED)) {
			if (StrNEqual(path, L"\\\\?\\", CSTRLEN(L"\\\\?\\"))) {
				WCHAR *p = path + 4;
				if (StrNEqual(p, L"UNC\\", CSTRLEN(L"UNC\\"))) {
					p += 2;
					*p = L'\\';
				}
				lstrcpy(pszFile, p);
			}
		}
	}

	char *lpData = (char *)NP2HeapAlloc(dwFileSize + 16);
	DWORD cbData = 0;
	const BOOL bReadSuccess = ReadFile(hFile, lpData, (DWORD)NP2HeapSize(lpData) - 2, &cbData, NULL);
	dwLastIOError = GetLastError();
	CloseHandle(hFile);

	if (!bReadSuccess) {
		NP2HeapFree(lpData);
		iSrcEncoding = -1;
		iWeakSrcEncoding = -1;
		return FALSE;
	}

	BOOL bPreferOEM = FALSE;
	if (bLoadNFOasOEM) {
		LPCWSTR const pszExt = pszFile + lstrlen(pszFile) - 4;
		if (pszExt >= pszFile && (StrCaseEqual(pszExt, L".nfo") || StrCaseEqual(pszExt, L".diz"))) {
			bPreferOEM = TRUE;
		}
	}

	if (!Encoding_IsValid(iDefaultEncoding)) {
		iDefaultEncoding = CPI_UTF8;
	}

	int _iDefaultEncoding = bPreferOEM ? g_DOSEncoding : iDefaultEncoding;
	if (iWeakSrcEncoding != -1 && Encoding_IsValid(iWeakSrcEncoding)) {
		_iDefaultEncoding = iWeakSrcEncoding;
	}

	int iEncoding = CPI_DEFAULT;
	const BOOL utf8Sig = cbData? IsUTF8Signature(lpData) : FALSE;
	BOOL bBOM = FALSE;
	BOOL bReverse = FALSE;

	if (cbData == 0) {
		FileVars_Init(NULL, 0, &fvCurFile);
		status->iEOLMode = iLineEndings[iDefaultEOLMode];

		if (iSrcEncoding == -1) {
			if (bLoadANSIasUTF8 && !bPreferOEM) {
				iEncoding = CPI_UTF8;
			} else {
				iEncoding = _iDefaultEncoding;
			}
		} else {
			iEncoding = iSrcEncoding;
		}

		SendMessage(hwnd, SCI_SETCODEPAGE, (mEncoding[iEncoding].uFlags & NCP_DEFAULT) ? iDefaultCodePage : SC_CP_UTF8, 0);
		EditSetNewText(hwnd, "", 0);
		SendMessage(hwnd, SCI_SETEOLMODE, status->iEOLMode, 0);
	} else if (!bSkipEncodingDetection &&
			   (iSrcEncoding == -1 || iSrcEncoding == CPI_UNICODE || iSrcEncoding == CPI_UNICODEBE) &&
			   (iSrcEncoding == CPI_UNICODE || iSrcEncoding == CPI_UNICODEBE || IsUnicode(lpData, cbData, &bBOM, &bReverse)) &&
			   (iSrcEncoding == CPI_UNICODE || iSrcEncoding == CPI_UNICODEBE || !utf8Sig)) {

		if (iSrcEncoding == CPI_UNICODE) {
			bBOM = (*((UNALIGNED PWCHAR)lpData) == 0xFEFF);
			bReverse = FALSE;
		} else if (iSrcEncoding == CPI_UNICODEBE) {
			bBOM = (*((UNALIGNED PWCHAR)lpData) == 0xFFFE);
		}

		if (iSrcEncoding == CPI_UNICODEBE || bReverse) {
			_swab(lpData, lpData, cbData);
			if (bBOM) {
				iEncoding = CPI_UNICODEBEBOM;
			} else {
				iEncoding = CPI_UNICODEBE;
			}
		} else {
			if (bBOM) {
				iEncoding = CPI_UNICODEBOM;
			} else {
				iEncoding = CPI_UNICODE;
			}
		}

		char *lpDataUTF8 = (char *)NP2HeapAlloc(cbData * kMaxMultiByteCount + 2);
		cbData = WideCharToMultiByte(CP_UTF8, 0, (bBOM ? (LPWSTR)lpData + 1 : (LPWSTR)lpData),
									 (int)(bBOM ? (cbData / sizeof(WCHAR)) : (cbData / sizeof(WCHAR)) + 1),
									 lpDataUTF8, (int)NP2HeapSize(lpDataUTF8), NULL, NULL);
		if (cbData == 0) {
			cbData = WideCharToMultiByte(CP_ACP, 0, (bBOM ? (LPWSTR)lpData + 1 : (LPWSTR)lpData),
										 -1, lpDataUTF8, (int)NP2HeapSize(lpDataUTF8), NULL, NULL);
			status->bUnicodeErr = TRUE;
		}

		SendMessage(hwnd, SCI_SETCODEPAGE, SC_CP_UTF8, 0);
		FileVars_Init(lpDataUTF8, cbData - 1, &fvCurFile);
		EditSetNewText(hwnd, lpDataUTF8, cbData - 1);
		EditDetectEOLMode(lpDataUTF8, cbData - 1, status);
		NP2HeapFree(lpDataUTF8);
	} else {
		FileVars_Init(lpData, cbData, &fvCurFile);
		if (!bSkipEncodingDetection
				&& (iSrcEncoding == -1 || iSrcEncoding == CPI_UTF8 || iSrcEncoding == CPI_UTF8SIGN)
				&& ((utf8Sig
					 || FileVars_IsUTF8(&fvCurFile)
					 || (iSrcEncoding == CPI_UTF8 || iSrcEncoding == CPI_UTF8SIGN)
					 || (!bPreferOEM && bLoadANSIasUTF8) // from menu "Reload As... UTF-8"
					 || IsUTF8(lpData, cbData)
					)
				   )
				&& !(FileVars_IsNonUTF8(&fvCurFile) && (iSrcEncoding != CPI_UTF8 && iSrcEncoding != CPI_UTF8SIGN))) {
			SendMessage(hwnd, SCI_SETCODEPAGE, SC_CP_UTF8, 0);
			if (utf8Sig) {
				EditSetNewText(hwnd, lpData + 3, cbData - 3);
				iEncoding = CPI_UTF8SIGN;
				EditDetectEOLMode(lpData + 3, cbData - 3, status);
			} else {
				EditSetNewText(hwnd, lpData, cbData);
				iEncoding = CPI_UTF8;
				EditDetectEOLMode(lpData, cbData, status);
			}
		} else {
			if (iSrcEncoding != -1) {
				iEncoding = iSrcEncoding;
			} else {
				iEncoding = FileVars_GetEncoding(&fvCurFile);
				if (iEncoding == -1) {
					if (fvCurFile.mask & FV_ENCODING) {
						iEncoding = CPI_DEFAULT;
					} else {
						if (iWeakSrcEncoding == -1) {
							iEncoding = _iDefaultEncoding;
						} else if (mEncoding[iWeakSrcEncoding].uFlags & NCP_INTERNAL) {
							iEncoding = iDefaultEncoding;
						} else {
							iEncoding = _iDefaultEncoding;
						}
					}
				}
			}

			const UINT uCodePage = mEncoding[iEncoding].uCodePage;
			if (((mEncoding[iEncoding].uFlags & NCP_8BIT) && uCodePage != CP_UTF7) ||
					(uCodePage == CP_UTF7 && IsUTF7(lpData, cbData))) {

				LPWSTR lpDataWide = (LPWSTR)NP2HeapAlloc(cbData * sizeof(WCHAR) + 16);
				const int cbDataWide = MultiByteToWideChar(uCodePage, 0, lpData, cbData, lpDataWide, (int)(NP2HeapSize(lpDataWide) / sizeof(WCHAR)));
				NP2HeapFree(lpData);
				lpData = (char *)NP2HeapAlloc(cbDataWide * kMaxMultiByteCount + 16);
				cbData = WideCharToMultiByte(CP_UTF8, 0, lpDataWide, cbDataWide, lpData, (int)NP2HeapSize(lpData), NULL, NULL);
				NP2HeapFree(lpDataWide);

				SendMessage(hwnd, SCI_SETCODEPAGE, SC_CP_UTF8, 0);
				EditSetNewText(hwnd, lpData, cbData);
				EditDetectEOLMode(lpData, cbData, status);
			} else {
				SendMessage(hwnd, SCI_SETCODEPAGE, iDefaultCodePage, 0);
				EditSetNewText(hwnd, lpData, cbData);
				iEncoding = CPI_DEFAULT;
				EditDetectEOLMode(lpData, cbData, status);
			}
		}
	}

	NP2HeapFree(lpData);
	status->iEncoding = iEncoding;
	iSrcEncoding = -1;
	iWeakSrcEncoding = -1;

	return TRUE;
}

//=============================================================================
//
// EditSaveFile()
//
BOOL EditSaveFile(HWND hwnd, LPCWSTR pszFile, BOOL bSaveCopy, EditFileIOStatus *status) {
	HANDLE hFile = CreateFile(pszFile,
					   GENERIC_WRITE,
					   FILE_SHARE_READ | FILE_SHARE_WRITE,
					   NULL, OPEN_ALWAYS,
					   FILE_ATTRIBUTE_NORMAL,
					   NULL);
	dwLastIOError = GetLastError();

	// failure could be due to missing attributes (2k/XP)
	if (hFile == INVALID_HANDLE_VALUE) {
		DWORD dwAttributes = GetFileAttributes(pszFile);
		if (dwAttributes != INVALID_FILE_ATTRIBUTES) {
			dwAttributes = dwAttributes & (FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM);
			hFile = CreateFile(pszFile,
							   GENERIC_WRITE,
							   FILE_SHARE_READ | FILE_SHARE_WRITE,
							   NULL,
							   OPEN_ALWAYS,
							   FILE_ATTRIBUTE_NORMAL | dwAttributes,
							   NULL);
			dwLastIOError = GetLastError();
		}
	}

	if (hFile == INVALID_HANDLE_VALUE) {
		return FALSE;
	}

	// ensure consistent line endings
	if (bFixLineEndings) {
		EditEnsureConsistentLineEndings(hwnd);
	}

	// strip trailing blanks
	if (bAutoStripBlanks) {
		EditStripTrailingBlanks(hwnd, TRUE);
	}

	BOOL bWriteSuccess;
	// get text
	DWORD cbData = (DWORD)SendMessage(hwnd, SCI_GETLENGTH, 0, 0);
	char *lpData = (char *)NP2HeapAlloc(cbData + 1);
	SendMessage(hwnd, SCI_GETTEXT, NP2HeapSize(lpData), (LPARAM)lpData);

	if (cbData == 0) {
		bWriteSuccess = SetEndOfFile(hFile);
		dwLastIOError = GetLastError();
	} else {
		DWORD dwBytesWritten;
		const int iEncoding = status->iEncoding;
		const UINT uFlags = mEncoding[iEncoding].uFlags;
		// FIXME: move checks in front of disk file access
		/*if ((uFlags & NCP_UNICODE) == 0 && (uFlags & NCP_UTF8_SIGN) == 0) {
				BOOL bEncodingMismatch = TRUE;
				FILEVARS fv;
				FileVars_Init(lpData, cbData, &fv);
				if (fv.mask & FV_ENCODING) {
					int iAltEncoding;
					if (FileVars_IsValidEncoding(&fv)) {
						iAltEncoding = FileVars_GetEncoding(&fv);
						if (iAltEncoding == iEncoding)
							bEncodingMismatch = FALSE;
						else if ((mEncoding[iAltEncoding].uFlags & NCP_UTF8) && (uFlags & NCP_UTF8))
							bEncodingMismatch = FALSE;
					}
					if (bEncodingMismatch) {
						Encoding_GetLabel(iAltEncoding);
						Encoding_GetLabel(iEncoding);
						InfoBox(0, L"MsgEncodingMismatch", IDS_ENCODINGMISMATCH,
							mEncoding[iAltEncoding].wchLabel,
							mEncoding[iEncoding].wchLabel);
					}
				}
			}*/
		if (uFlags & NCP_UNICODE) {
			SetEndOfFile(hFile);

			LPWSTR lpDataWide = (LPWSTR)NP2HeapAlloc(cbData * sizeof(WCHAR) + 16);
			const int cbDataWide = MultiByteToWideChar(CP_UTF8, 0, lpData, cbData, lpDataWide, (int)(NP2HeapSize(lpDataWide) / sizeof(WCHAR)));

			if (uFlags & NCP_UNICODE_BOM) {
				if (uFlags & NCP_UNICODE_REVERSE) {
					WriteFile(hFile, (LPCVOID)"\xFE\xFF", 2, &dwBytesWritten, NULL);
				} else {
					WriteFile(hFile, (LPCVOID)"\xFF\xFE", 2, &dwBytesWritten, NULL);
				}
			}

			if (uFlags & NCP_UNICODE_REVERSE) {
				_swab((char *)lpDataWide, (char *)lpDataWide, (int)(cbDataWide * sizeof(WCHAR)));
			}

			bWriteSuccess = WriteFile(hFile, lpDataWide, cbDataWide * sizeof(WCHAR), &dwBytesWritten, NULL);
			dwLastIOError = GetLastError();

			NP2HeapFree(lpDataWide);
		} else if (uFlags & NCP_UTF8) {
			SetEndOfFile(hFile);

			if (uFlags & NCP_UTF8_SIGN) {
				WriteFile(hFile, (LPCVOID)"\xEF\xBB\xBF", 3, &dwBytesWritten, NULL);
			}

			bWriteSuccess = WriteFile(hFile, lpData, cbData, &dwBytesWritten, NULL);
			dwLastIOError = GetLastError();
		} else if (uFlags & NCP_8BIT) {
			BOOL bCancelDataLoss = FALSE;
			const UINT uCodePage = mEncoding[iEncoding].uCodePage;

			LPWSTR lpDataWide = (LPWSTR)NP2HeapAlloc(cbData * 2 + 16);
			const int cbDataWide = MultiByteToWideChar(CP_UTF8, 0, lpData, cbData, lpDataWide, (int)(NP2HeapSize(lpDataWide) / sizeof(WCHAR)));
			// Special cases: 42, 50220, 50221, 50222, 50225, 50227, 50229, 54936 GB18030, 57002-11, 65000, 65001
			if (uCodePage == CP_UTF7 || uCodePage == 54936) {
				NP2HeapFree(lpData);
				lpData = (char *)NP2HeapAlloc(NP2HeapSize(lpDataWide) * 2);
			} else {
				ZeroMemory(lpData, NP2HeapSize(lpData));
			}

			if (uCodePage == CP_UTF7 || uCodePage == 54936) {
				cbData = WideCharToMultiByte(uCodePage, 0, lpDataWide, cbDataWide, lpData, (int)NP2HeapSize(lpData), NULL, NULL);
			} else {
				cbData = WideCharToMultiByte(uCodePage, WC_NO_BEST_FIT_CHARS, lpDataWide, cbDataWide, lpData, (int)NP2HeapSize(lpData), NULL, &bCancelDataLoss);
				if (!bCancelDataLoss) {
					cbData = WideCharToMultiByte(uCodePage, 0, lpDataWide, cbDataWide, lpData, (int)NP2HeapSize(lpData), NULL, NULL);
					bCancelDataLoss = FALSE;
				}
			}
			NP2HeapFree(lpDataWide);

			if (!bCancelDataLoss || InfoBox(MBOKCANCEL, L"MsgConv3", IDS_ERR_UNICODE2) == IDOK) {
				SetEndOfFile(hFile);
				bWriteSuccess = WriteFile(hFile, lpData, cbData, &dwBytesWritten, NULL);
				dwLastIOError = GetLastError();
			} else {
				bWriteSuccess = FALSE;
				status->bCancelDataLoss = TRUE;
			}
		} else {
			SetEndOfFile(hFile);
			bWriteSuccess = WriteFile(hFile, lpData, cbData, &dwBytesWritten, NULL);
			dwLastIOError = GetLastError();
		}
	}

	NP2HeapFree(lpData);
	CloseHandle(hFile);

	if (bWriteSuccess) {
		if (!bSaveCopy) {
			SendMessage(hwnd, SCI_SETSAVEPOINT, 0, 0);
		}
		return TRUE;
	}

	return FALSE;
}

//=============================================================================
//
// EditInvertCase()
//
void EditInvertCase(HWND hwnd) {
	const int iCurPos = (int)SendMessage(hwnd, SCI_GETCURRENTPOS, 0, 0);
	const int iAnchorPos = (int)SendMessage(hwnd, SCI_GETANCHOR, 0, 0);
	if (iCurPos == iAnchorPos) {
		return;
	}
	if (SC_SEL_RECTANGLE == SendMessage(hwnd, SCI_GETSELECTIONMODE, 0, 0)) {
		MsgBox(MBWARN, IDS_SELRECT);
		return;
	}

	const int iSelCount = (int)SendMessage(hwnd, SCI_GETSELTEXT, 0, 0) - 1;
	char *pszText = (char *)NP2HeapAlloc(iSelCount + 1);
	LPWSTR pszTextW = (LPWSTR)NP2HeapAlloc((iSelCount + 1) * sizeof(WCHAR));

	SendMessage(hwnd, SCI_GETSELTEXT, 0, (LPARAM)pszText);

	const UINT cpEdit = (UINT)SendMessage(hwnd, SCI_GETCODEPAGE, 0, 0);
	const int cchTextW = MultiByteToWideChar(cpEdit, 0, pszText, iSelCount, pszTextW, (int)(NP2HeapSize(pszTextW) / sizeof(WCHAR)));

	BOOL bChanged = FALSE;
	for (int i = 0; i < cchTextW; i++) {
		if (IsCharUpper(pszTextW[i])) {
			pszTextW[i] = LOWORD(CharLower((LPWSTR)(LONG_PTR)MAKELONG(pszTextW[i], 0)));
			bChanged = TRUE;
		} else if (IsCharLower(pszTextW[i])) {
			pszTextW[i] = LOWORD(CharUpper((LPWSTR)(LONG_PTR)MAKELONG(pszTextW[i], 0)));
			bChanged = TRUE;
		}
	}

	if (bChanged) {
		WideCharToMultiByte(cpEdit, 0, pszTextW, cchTextW, pszText, (int)NP2HeapSize(pszText), NULL, NULL);

		SendMessage(hwnd, SCI_BEGINUNDOACTION, 0, 0);
		SendMessage(hwnd, SCI_CLEAR, 0, 0);
		SendMessage(hwnd, SCI_ADDTEXT, iSelCount, (LPARAM)pszText);
		SendMessage(hwnd, SCI_SETSEL, iAnchorPos, iCurPos);
		SendMessage(hwnd, SCI_ENDUNDOACTION, 0, 0);
	}

	NP2HeapFree(pszText);
	NP2HeapFree(pszTextW);
}

//=============================================================================
//
// EditTitleCase()
//
void EditTitleCase(HWND hwnd) {
	const int iCurPos = (int)SendMessage(hwnd, SCI_GETCURRENTPOS, 0, 0);
	const int iAnchorPos = (int)SendMessage(hwnd, SCI_GETANCHOR, 0, 0);
	if (iCurPos == iAnchorPos) {
		return;
	}
	if (SC_SEL_RECTANGLE == SendMessage(hwnd, SCI_GETSELECTIONMODE, 0, 0)) {
		MsgBox(MBWARN, IDS_SELRECT);
		return;
	}

	const int iSelCount = (int)SendMessage(hwnd, SCI_GETSELTEXT, 0, 0) - 1;
	char *pszText = (char *)NP2HeapAlloc(iSelCount + 1);
	LPWSTR pszTextW = (LPWSTR)NP2HeapAlloc((iSelCount + 1) * sizeof(WCHAR));

	SendMessage(hwnd, SCI_GETSELTEXT, 0, (LPARAM)pszText);
	const UINT cpEdit = (UINT)SendMessage(hwnd, SCI_GETCODEPAGE, 0, 0);
	const int cchTextW = MultiByteToWideChar(cpEdit, 0, pszText, iSelCount, pszTextW, (int)(NP2HeapSize(pszTextW) / sizeof(WCHAR)));

	BOOL bChanged = FALSE;
	if (IsWin7AndAbove()) {
		LPWSTR pszMappedW = (LPWSTR)NP2HeapAlloc(NP2HeapSize(pszTextW));
		if (LCMapString(LOCALE_SYSTEM_DEFAULT,
						LCMAP_LINGUISTIC_CASING | LCMAP_TITLECASE,
						pszTextW, cchTextW, pszMappedW, (int)(NP2HeapSize(pszMappedW) / sizeof(WCHAR)))) {
			lstrcpyn(pszTextW, pszMappedW, (int)(NP2HeapSize(pszTextW) / sizeof(WCHAR)));
			bChanged = TRUE;
		} else {
			bChanged = FALSE;
		}
		NP2HeapFree(pszMappedW);
	} else {
#if 1 // BOOKMARK_EDITION
		//Slightly enhanced function to make Title Case:
		//Added some '-characters and bPrevWasSpace makes it better (for example "'Don't'" will now work)
		BOOL bNewWord = TRUE;
		BOOL bPrevWasSpace = TRUE;
		for (int i = 0; i < cchTextW; i++) {
			if (!IsCharAlphaNumeric(pszTextW[i]) && (!StrChr(L"\x0027\x0060\x0384\x2019", pszTextW[i]) ||	bPrevWasSpace)) {
				bNewWord = TRUE;
			} else {
				if (bNewWord) {
					if (IsCharLower(pszTextW[i])) {
						pszTextW[i] = LOWORD(CharUpper((LPWSTR)(LONG_PTR)MAKELONG(pszTextW[i], 0)));
						bChanged = TRUE;
					}
				} else {
					if (IsCharUpper(pszTextW[i])) {
						pszTextW[i] = LOWORD(CharLower((LPWSTR)(LONG_PTR)MAKELONG(pszTextW[i], 0)));
						bChanged = TRUE;
					}
				}
				bNewWord = FALSE;
			}

			if (StrChr(L" \r\n\t[](){}", pszTextW[i])) {
				bPrevWasSpace = TRUE;
			} else {
				bPrevWasSpace = FALSE;
			}
		}
#else
		BOOL bNewWord = TRUE;
		BOOL bWordEnd = TRUE;
		for (int i = 0; i < cchTextW; i++) {
			const BOOL bAlphaNumeric = IsCharAlphaNumeric(pszTextW[i]);
			if (!bAlphaNumeric && (!StrChr(L"\x0027\x2019\x0060\x00B4", pszTextW[i]) || bWordEnd)) {
				bNewWord = TRUE;
			} else {
				if (bNewWord) {
					if (IsCharLower(pszTextW[i])) {
						pszTextW[i] = LOWORD(CharUpper((LPWSTR)(LONG_PTR)MAKELONG(pszTextW[i], 0)));
						bChanged = TRUE;
					}
				} else {
					if (IsCharUpper(pszTextW[i])) {
						pszTextW[i] = LOWORD(CharLower((LPWSTR)(LONG_PTR)MAKELONG(pszTextW[i], 0)));
						bChanged = TRUE;
					}
				}
				bNewWord = FALSE;
			}
			bWordEnd = !bAlphaNumeric;
		}
#endif
	}

	if (bChanged) {
		WideCharToMultiByte(cpEdit, 0, pszTextW, cchTextW, pszText, (int)NP2HeapSize(pszText), NULL, NULL);

		SendMessage(hwnd, SCI_BEGINUNDOACTION, 0, 0);
		SendMessage(hwnd, SCI_CLEAR, 0, 0);
		SendMessage(hwnd, SCI_ADDTEXT, iSelCount, (LPARAM)pszText);
		SendMessage(hwnd, SCI_SETSEL, iAnchorPos, iCurPos);
		SendMessage(hwnd, SCI_ENDUNDOACTION, 0, 0);
	}

	NP2HeapFree(pszText);
	NP2HeapFree(pszTextW);
}

//=============================================================================
//
// EditSentenceCase()
//
void EditSentenceCase(HWND hwnd) {
	const int iCurPos = (int)SendMessage(hwnd, SCI_GETCURRENTPOS, 0, 0);
	const int iAnchorPos = (int)SendMessage(hwnd, SCI_GETANCHOR, 0, 0);
	if (iCurPos == iAnchorPos) {
		return;
	}
	if (SC_SEL_RECTANGLE == SendMessage(hwnd, SCI_GETSELECTIONMODE, 0, 0)) {
		MsgBox(MBWARN, IDS_SELRECT);
		return;
	}

	const int iSelCount = (int)SendMessage(hwnd, SCI_GETSELTEXT, 0, 0) - 1;
	char *pszText = (char *)NP2HeapAlloc(iSelCount + 1);
	LPWSTR pszTextW = (LPWSTR)NP2HeapAlloc((iSelCount + 1) * sizeof(WCHAR));

	SendMessage(hwnd, SCI_GETSELTEXT, 0, (LPARAM)pszText);

	const UINT cpEdit = (UINT)SendMessage(hwnd, SCI_GETCODEPAGE, 0, 0);
	const int cchTextW = MultiByteToWideChar(cpEdit, 0, pszText, iSelCount, pszTextW, (int)(NP2HeapSize(pszTextW) / sizeof(WCHAR)));

	BOOL bNewSentence = TRUE;
	BOOL bChanged = FALSE;
	for (int i = 0; i < cchTextW; i++) {
		if (StrChr(L".;!?\r\n", pszTextW[i])) {
			bNewSentence = TRUE;
		} else {
			if (IsCharAlphaNumeric(pszTextW[i])) {
				if (bNewSentence) {
					if (IsCharLower(pszTextW[i])) {
						pszTextW[i] = LOWORD(CharUpper((LPWSTR)(LONG_PTR)MAKELONG(pszTextW[i], 0)));
						bChanged = TRUE;
					}
					bNewSentence = FALSE;
				} else {
					if (IsCharUpper(pszTextW[i])) {
						pszTextW[i] = LOWORD(CharLower((LPWSTR)(LONG_PTR)MAKELONG(pszTextW[i], 0)));
						bChanged = TRUE;
					}
				}
			}
		}
	}

	if (bChanged) {
		WideCharToMultiByte(cpEdit, 0, pszTextW, cchTextW, pszText, (int)NP2HeapSize(pszText), NULL, NULL);

		SendMessage(hwnd, SCI_BEGINUNDOACTION, 0, 0);
		SendMessage(hwnd, SCI_CLEAR, 0, 0);
		SendMessage(hwnd, SCI_ADDTEXT, iSelCount, (LPARAM)pszText);
		SendMessage(hwnd, SCI_SETSEL, iAnchorPos, iCurPos);
		SendMessage(hwnd, SCI_ENDUNDOACTION, 0, 0);
	}

	NP2HeapFree(pszText);
	NP2HeapFree(pszTextW);
}

//=============================================================================
//
// EditURLEncode()
//
LPWSTR EditURLEncodeSelection(HWND hwnd, int *pcchEscaped, BOOL bTrim) {
	*pcchEscaped = 0;
	const int iSelCount = (int)SendMessage(hwnd, SCI_GETSELTEXT, 0, 0);
	if (iSelCount == 0) {
		return NULL;
	}

	char *pszText = (char *)NP2HeapAlloc(iSelCount);
	SendMessage(hwnd, SCI_GETSELTEXT, 0, (LPARAM)pszText);

	if (bTrim) {
		StrTrimA(pszText, " \t\r\n");
	}
	if (StrIsEmptyA(pszText)) {
		NP2HeapFree(pszText);
		return NULL;
	}

	LPWSTR pszTextW = (LPWSTR)NP2HeapAlloc(iSelCount * sizeof(WCHAR));
	const UINT cpEdit = (UINT)SendMessage(hwnd, SCI_GETCODEPAGE, 0, 0);
	MultiByteToWideChar(cpEdit, 0, pszText, iSelCount, pszTextW, (int)(NP2HeapSize(pszTextW) / sizeof(WCHAR)));

	// https://docs.microsoft.com/en-us/windows/desktop/api/shlwapi/nf-shlwapi-urlescapew
	LPWSTR pszEscapedW = (LPWSTR)NP2HeapAlloc(NP2HeapSize(pszTextW) * 3);  // '&', H1, H0

	DWORD cchEscapedW = (int)NP2HeapSize(pszEscapedW) / sizeof(WCHAR);
	UrlEscape(pszTextW, pszEscapedW, &cchEscapedW, URL_ESCAPE_SEGMENT_ONLY);

	NP2HeapFree(pszText);
	NP2HeapFree(pszTextW);
	*pcchEscaped = cchEscapedW;
	return pszEscapedW;
}

void EditURLEncode(HWND hwnd) {
	int iCurPos = (int)SendMessage(hwnd, SCI_GETCURRENTPOS, 0, 0);
	int iAnchorPos = (int)SendMessage(hwnd, SCI_GETANCHOR, 0, 0);
	if (iCurPos == iAnchorPos) {
		return;
	}
	if (SC_SEL_RECTANGLE == SendMessage(hwnd, SCI_GETSELECTIONMODE, 0, 0)) {
		MsgBox(MBWARN, IDS_SELRECT);
		return;
	}

	int cchEscapedW;
	LPWSTR pszEscapedW = EditURLEncodeSelection(hwnd, &cchEscapedW, FALSE);
	if (pszEscapedW == NULL) {
		return;
	}

	const UINT cpEdit = (UINT)SendMessage(hwnd, SCI_GETCODEPAGE, 0, 0);
	char *pszEscaped = (char *)NP2HeapAlloc(cchEscapedW * kMaxMultiByteCount);
	const int cchEscaped = WideCharToMultiByte(cpEdit, 0, pszEscapedW, cchEscapedW, pszEscaped, (int)NP2HeapSize(pszEscaped), NULL, NULL);
	if (iCurPos < iAnchorPos) {
		iAnchorPos = iCurPos + cchEscaped;
	} else {
		iCurPos = iAnchorPos + cchEscaped;
	}

	SendMessage(hwnd, SCI_BEGINUNDOACTION, 0, 0);
	SendMessage(hwnd, SCI_CLEAR, 0, 0);
	SendMessage(hwnd, SCI_ADDTEXT, cchEscaped, (LPARAM)pszEscaped);
	SendMessage(hwnd, SCI_SETSEL, iAnchorPos, iCurPos);
	SendMessage(hwnd, SCI_ENDUNDOACTION, 0, 0);

	NP2HeapFree(pszEscaped);
	NP2HeapFree(pszEscapedW);
}

//=============================================================================
//
// EditURLDecode()
//
void EditURLDecode(HWND hwnd) {
	int iCurPos	= (int)SendMessage(hwnd, SCI_GETCURRENTPOS, 0, 0);
	int iAnchorPos = (int)SendMessage(hwnd, SCI_GETANCHOR, 0, 0);
	if (iCurPos == iAnchorPos) {
		return;
	}
	if (SC_SEL_RECTANGLE == SendMessage(hwnd, SCI_GETSELECTIONMODE, 0, 0)) {
		MsgBox(MBWARN, IDS_SELRECT);
		return;
	}

	const int iSelCount = (int)SendMessage(hwnd, SCI_GETSELTEXT, 0, 0);

	char *pszText = (char *)NP2HeapAlloc(iSelCount);
	LPWSTR pszTextW = (LPWSTR)NP2HeapAlloc(iSelCount * sizeof(WCHAR));

	SendMessage(hwnd, SCI_GETSELTEXT, 0, (LPARAM)pszText);
	const UINT cpEdit = (UINT)SendMessage(hwnd, SCI_GETCODEPAGE, 0, 0);
	MultiByteToWideChar(cpEdit, 0, pszText, iSelCount, pszTextW, (int)(NP2HeapSize(pszTextW) / sizeof(WCHAR)));

	char *pszUnescaped = (char *)NP2HeapAlloc(NP2HeapSize(pszText) * 3);
	LPWSTR pszUnescapedW = (LPWSTR)NP2HeapAlloc(NP2HeapSize(pszTextW) * 3);

	DWORD cchUnescapedW = (DWORD)(NP2HeapSize(pszUnescapedW) / sizeof(WCHAR));
	UrlUnescape(pszTextW, pszUnescapedW, &cchUnescapedW, 0);
	const int cchUnescaped = WideCharToMultiByte(cpEdit, 0, pszUnescapedW, cchUnescapedW, pszUnescaped, (int)NP2HeapSize(pszUnescaped), NULL, NULL);

	if (iCurPos < iAnchorPos) {
		iAnchorPos = iCurPos + cchUnescaped;
	} else {
		iCurPos = iAnchorPos + cchUnescaped;
	}

	SendMessage(hwnd, SCI_BEGINUNDOACTION, 0, 0);
	SendMessage(hwnd, SCI_CLEAR, 0, 0);
	SendMessage(hwnd, SCI_ADDTEXT, cchUnescaped, (LPARAM)pszUnescaped);
	SendMessage(hwnd, SCI_SETSEL, iAnchorPos, iCurPos);
	SendMessage(hwnd, SCI_ENDUNDOACTION, 0, 0);

	NP2HeapFree(pszText);
	NP2HeapFree(pszTextW);
	NP2HeapFree(pszUnescaped);
	NP2HeapFree(pszUnescapedW);
}

//=============================================================================
//
// EditEscapeCChars()
//
void EditEscapeCChars(HWND hwnd) {
	if (EditIsEmptySelection()) {
		return;
	}
	if (SC_SEL_RECTANGLE == SendMessage(hwnd, SCI_GETSELECTIONMODE, 0, 0)) {
		MsgBox(MBWARN, IDS_SELRECT);
		return;
	}

#if NP2_USE_DESIGNATED_INITIALIZER
	EDITFINDREPLACE efr = {
		.hwnd = hwnd,
	};
#else
	EDITFINDREPLACE efr = { "", "", "", "", hwnd };
#endif

	SendMessage(hwnd, SCI_BEGINUNDOACTION, 0, 0);

	strcpy(efr.szFind, "\\");
	strcpy(efr.szReplace, "\\\\");
	EditReplaceAllInSelection(hwnd, &efr, FALSE);

	strcpy(efr.szFind, "\"");
	strcpy(efr.szReplace, "\\\"");
	EditReplaceAllInSelection(hwnd, &efr, FALSE);

	strcpy(efr.szFind, "\'");
	strcpy(efr.szReplace, "\\\'");
	EditReplaceAllInSelection(hwnd, &efr, FALSE);

	SendMessage(hwnd, SCI_ENDUNDOACTION, 0, 0);
}

//=============================================================================
//
// EditUnescapeCChars()
//
void EditUnescapeCChars(HWND hwnd) {
	if (EditIsEmptySelection()) {
		return;
	}
	if (SC_SEL_RECTANGLE == SendMessage(hwnd, SCI_GETSELECTIONMODE, 0, 0)) {
		MsgBox(MBWARN, IDS_SELRECT);
		return;
	}

#if NP2_USE_DESIGNATED_INITIALIZER
	EDITFINDREPLACE efr = {
		.hwnd = hwnd,
	};
#else
	EDITFINDREPLACE efr = { "", "", "", "", hwnd };
#endif

	SendMessage(hwnd, SCI_BEGINUNDOACTION, 0, 0);

	strcpy(efr.szFind, "\\\\");
	strcpy(efr.szReplace, "\\");
	EditReplaceAllInSelection(hwnd, &efr, FALSE);

	strcpy(efr.szFind, "\\\"");
	strcpy(efr.szReplace, "\"");
	EditReplaceAllInSelection(hwnd, &efr, FALSE);

	strcpy(efr.szFind, "\\\'");
	strcpy(efr.szReplace, "\'");
	EditReplaceAllInSelection(hwnd, &efr, FALSE);

	SendMessage(hwnd, SCI_ENDUNDOACTION, 0, 0);
}

// XML/HTML predefined entity
// https://en.wikipedia.org/wiki/List_of_XML_and_HTML_character_entity_references
// &quot;	["]
// &amp;	[&]
// &apos;	[']
// &lt;		[<]
// &gt;		[>]
// &nbsp;	[ ]
// &emsp;	[\t]
//=============================================================================
//
// EditEscapeXHTMLChars()
//
void EditEscapeXHTMLChars(HWND hwnd) {
	if (EditIsEmptySelection()) {
		return;
	}
	if (SC_SEL_RECTANGLE == SendMessage(hwnd, SCI_GETSELECTIONMODE, 0, 0)) {
		MsgBox(MBWARN, IDS_SELRECT);
		return;
	}

#if NP2_USE_DESIGNATED_INITIALIZER
	EDITFINDREPLACE efr = {
		.hwnd = hwnd,
	};
#else
	EDITFINDREPLACE efr = { "", "", "", "", hwnd };
#endif

	SendMessage(hwnd, SCI_BEGINUNDOACTION, 0, 0);

	strcpy(efr.szFind, "&");
	strcpy(efr.szReplace, "&amp;");
	EditReplaceAllInSelection(hwnd, &efr, FALSE);

	strcpy(efr.szFind, "\"");
	strcpy(efr.szReplace, "&quot;");
	EditReplaceAllInSelection(hwnd, &efr, FALSE);

	strcpy(efr.szFind, "\'");
	strcpy(efr.szReplace, "&apos;");
	EditReplaceAllInSelection(hwnd, &efr, FALSE);

	strcpy(efr.szFind, "<");
	strcpy(efr.szReplace, "&lt;");
	EditReplaceAllInSelection(hwnd, &efr, FALSE);

	strcpy(efr.szFind, ">");
	strcpy(efr.szReplace, "&gt;");
	EditReplaceAllInSelection(hwnd, &efr, FALSE);

	if (pLexCurrent->iLexer != SCLEX_XML) {
		strcpy(efr.szFind, " ");
		strcpy(efr.szReplace, "&nbsp;");
		EditReplaceAllInSelection(hwnd, &efr, FALSE);

		strcpy(efr.szFind, "\t");
		strcpy(efr.szReplace, "&emsp;");
		EditReplaceAllInSelection(hwnd, &efr, FALSE);
	}

	SendMessage(hwnd, SCI_ENDUNDOACTION, 0, 0);
}

//=============================================================================
//
// EditUnescapeXHTMLChars()
//
void EditUnescapeXHTMLChars(HWND hwnd) {
	if (EditIsEmptySelection()) {
		return;
	}
	if (SC_SEL_RECTANGLE == SendMessage(hwnd, SCI_GETSELECTIONMODE, 0, 0)) {
		MsgBox(MBWARN, IDS_SELRECT);
		return;
	}

#if NP2_USE_DESIGNATED_INITIALIZER
	EDITFINDREPLACE efr = {
		.hwnd = hwnd,
	};
#else
	EDITFINDREPLACE efr = { "", "", "", "", hwnd };
#endif

	SendMessage(hwnd, SCI_BEGINUNDOACTION, 0, 0);

	strcpy(efr.szFind, "&quot;");
	strcpy(efr.szReplace, "\"");
	EditReplaceAllInSelection(hwnd, &efr, FALSE);

	strcpy(efr.szFind, "&apos;");
	strcpy(efr.szReplace, "\'");
	EditReplaceAllInSelection(hwnd, &efr, FALSE);

	strcpy(efr.szFind, "&lt;");
	strcpy(efr.szReplace, "<");
	EditReplaceAllInSelection(hwnd, &efr, FALSE);

	strcpy(efr.szFind, "&gt;");
	strcpy(efr.szReplace, ">");
	EditReplaceAllInSelection(hwnd, &efr, FALSE);

	strcpy(efr.szFind, "&nbsp;");
	strcpy(efr.szReplace, " ");
	EditReplaceAllInSelection(hwnd, &efr, FALSE);

	strcpy(efr.szFind, "&amp;");
	strcpy(efr.szReplace, "&");
	EditReplaceAllInSelection(hwnd, &efr, FALSE);

	strcpy(efr.szFind, "&emsp;");
	strcpy(efr.szReplace, "\t");
	EditReplaceAllInSelection(hwnd, &efr, FALSE);
	SendMessage(hwnd, SCI_ENDUNDOACTION, 0, 0);
}

//=============================================================================
//
// EditChar2Hex()
//

/*				C/C++	C#	Java	JS	JSON	Python	PHP	Lua		Go
\ooo		3	1			1					1		1	1/ddd	1
\xHH		2	1		1			1			1		1			1
\uHHHH		4	1			1		1	1		1					1
\UHHHHHHHH	8	1								1					1
\xHHHH		4			1
\uHHHHHH	6				1
*/
#define MAX_ESCAPE_HEX_DIGIT	4

void EditChar2Hex(HWND hwnd) {
	const int iSelStart = (int)SendMessage(hwnd, SCI_GETSELECTIONSTART, 0, 0);
	int count = (int)SendMessage(hwnd, SCI_GETSELTEXT, 0, 0) - 1;
	if (count == 0) {
		return;
	}
	if (SC_SEL_RECTANGLE == SendMessage(hwnd, SCI_GETSELECTIONMODE, 0, 0)) {
		MsgBox(MBWARN, IDS_SELRECT);
		return;
	}

	count *= 2 + MAX_ESCAPE_HEX_DIGIT;
	char *ch = (char *)NP2HeapAlloc(count + 1);
	WCHAR *wch = (WCHAR *)NP2HeapAlloc((count + 1) * sizeof(WCHAR));
	SendMessage(hwnd, SCI_GETSELTEXT, 0, (LPARAM)ch);

	if (ch[0] == 0) {
		strcpy(ch, "\\x00");
	} else {
		const UINT cpEdit = (UINT)SendMessage(hwnd, SCI_GETCODEPAGE, 0, 0);
		char uesc = 'u';
		if (pLexCurrent->rid == NP2LEX_CSHARP) {
			uesc = 'x';
		}
		count = MultiByteToWideChar(cpEdit, 0, ch, -1, wch, count + 1) - 1; // '\0'
		for (int i = 0, j = 0; i < count; i++) {
			if (wch[i] <= 0xFF) {
				wsprintfA(ch + j, "\\x%02X", wch[i] & 0xFF); // \xhh
				j += 4;
			} else {
				wsprintfA(ch + j, "\\%c%04X", uesc, wch[i]); // \uhhhh \xhhhh
				j += 6;
			}
		}
	}

	SendMessage(hwnd, SCI_REPLACESEL, 0, (LPARAM)ch);
	SendMessage(hwnd, SCI_SETSEL, iSelStart, iSelStart + strlen(ch));
	NP2HeapFree(ch);
	NP2HeapFree(wch);
}

//=============================================================================
//
// EditHex2Char()
//
void EditHex2Char(HWND hwnd) {
	const int iSelStart = (int)SendMessage(hwnd, SCI_GETSELECTIONSTART, 0, 0);
	const int iSelEnd = (int)SendMessage(hwnd, SCI_GETSELECTIONEND, 0, 0);
	int count = (int)SendMessage(hwnd, SCI_GETSELTEXT, 0, 0) - 1;
	if (count == 0) {
		return;
	}
	if (SC_SEL_RECTANGLE == SendMessage(hwnd, SCI_GETSELECTIONMODE, 0, 0)) {
		MsgBox(MBWARN, IDS_SELRECT);
		return;
	}

	count *= 2 + MAX_ESCAPE_HEX_DIGIT;
	char *ch = (char *)NP2HeapAlloc(count + 1);
	WCHAR *wch = (WCHAR *)NP2HeapAlloc((count + 1) * sizeof(WCHAR));
	const UINT cpEdit = (UINT)SendMessage(hwnd, SCI_GETCODEPAGE, 0, 0);
	int ci = 0;
	int cch = 0;

	SendMessage(hwnd, SCI_GETSELTEXT, 0, (LPARAM)ch);

	char *p = ch;
	while (*p) {
		if (*p == '\\') {
			p++;
			if (*p == 'x' || *p == 'u') {
				p++;
				ci = 0;
				int ucc = 0;
				while (*p && (ucc++ < MAX_ESCAPE_HEX_DIGIT)) {
					if (*p >= '0' && *p <= '9') {
						ci = ci * 16 + (*p++ - '0');
					} else if (*p >= 'a' && *p <= 'f') {
						ci = ci * 16 + (*p++ - 'a') + 10;
					} else if (*p >= 'A' && *p <=  'F') {
						ci = ci * 16 + (*p++ - 'A') + 10;
					} else {
						break;
					}
				}
			} else {
				ci = *p++;
			}
		} else {
			ci = *p++;
		}
		wch[cch++] = (WCHAR)ci;
		if (ci == 0) {
			break;
		}
	}

	wch[cch] = 0;
	cch = WideCharToMultiByte(cpEdit, 0, wch, -1, ch, count + 1, NULL, NULL) - 1; // '\0'

	SendMessage(hwnd, SCI_SETTARGETSTART, iSelStart, 0);
	SendMessage(hwnd, SCI_SETTARGETEND, iSelEnd, 0);
	SendMessage(hwnd, SCI_REPLACETARGET, cch, (LPARAM)ch);
	SendMessage(hwnd, SCI_SETSEL, iSelStart, iSelStart + cch);
	NP2HeapFree(ch);
	NP2HeapFree(wch);
}

void EditShowHex(HWND hwnd) {
	const int iSelEnd = (int)SendMessage(hwnd, SCI_GETSELECTIONEND, 0, 0);
	const int count = (int)SendMessage(hwnd, SCI_GETSELTEXT, 0, 0) - 1;
	if (count == 0) {
		return;
	}
	if (SC_SEL_RECTANGLE == SendMessage(hwnd, SCI_GETSELECTIONMODE, 0, 0)) {
		MsgBox(MBWARN, IDS_SELRECT);
		return;
	}

	char *ch = (char *)NP2HeapAlloc(count + 1);
	char *cch = (char *)NP2HeapAlloc(count * 3 + 3);
	char *p = ch;
	char *t = cch;
	SendMessage(hwnd, SCI_GETSELTEXT, 0, (LPARAM)ch);
	*t++ = '[';
	while (*p) {
		int c = *p++;
		if (c < 0) {
			c += 256;
		}
		int v = c >> 4;
		*t++ = (char)((v >= 10) ? v - 10 + 'a' : v + '0');
		v = c & 0x0f;
		*t++ = (char)((v >= 10) ? v - 10 + 'a' : v + '0');
		*t++ = ' ';
	}
	*--t = ']';
	SendMessage(hwnd, SCI_INSERTTEXT, iSelEnd, (LPARAM)cch);
	SendMessage(hwnd, SCI_SETSEL, iSelEnd, iSelEnd + strlen(cch));
	NP2HeapFree(ch);
	NP2HeapFree(cch);
}

//=============================================================================
//
// EditConvertNumRadix()
//
static inline BOOL iswordstart(int ch) {
	return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || ch == '.' || ch == '_';
}

static inline BOOL iswordchar(int ch) {
	return iswordstart(ch) || (ch >= '0' && ch <= '9') || ch == '$';
}

static int ConvertNumRadix(char *tch, uint64_t num, int radix) {
	switch (radix) {
	case 16:
		return sprintf(tch, "0x%" PRIx64, num);

	case 10:
		return sprintf(tch, "%" PRIu64, num);

	case 8: {
		char buf[2 + 22 + 1] = "";
		int index = 2 + 22;
		int length = 0;
		while (num) {
			const int bit = (int)(num & 7);
			num >>= 3;
			buf[index--] = (char)('0' + bit);
			++length;
		}
		if (length == 0) {
			buf[index--] = '0';
			++length;
		}
		buf[index--] = 'O';
		buf[index] = '0';
		length += 2;
		strcat(tch, buf + index);
		return length;
	}
	break;

	case 2: {
		char buf[2 + 64 + 8 + 1] = "";
		int index = 2 + 64 + 8;
		int length = 0;
		int bit_count = 0;
		while (num) {
			const int bit = (int)(num & 1);
			num >>= 1;
			buf[index--] = (char)('0' + bit);
			++bit_count;
			++length;
			if (num && (bit_count & 7) == 0) {
				buf[index--] = '_';
				++length;
			}
		}
		if (length == 0) {
			buf[index--] = '0';
			++length;
		}
		buf[index--] = 'b';
		buf[index] = '0';
		length += 2;
		strcat(tch, buf + index);
		return length;
	}
	break;

	}
	return 0;
}

void EditConvertNumRadix(HWND hwnd, int radix) {
	const int iSelStart = (int)SendMessage(hwnd, SCI_GETSELECTIONSTART, 0, 0);
	const int iSelEnd = (int)SendMessage(hwnd, SCI_GETSELECTIONEND, 0, 0);
	const int count = (int)SendMessage(hwnd, SCI_GETSELTEXT, 0, 0) - 1;
	if (count == 0) {
		return;
	}
	if (SC_SEL_RECTANGLE == SendMessage(hwnd, SCI_GETSELECTIONMODE, 0, 0)) {
		MsgBox(MBWARN, IDS_SELRECT);
		return;
	}

	char *ch = (char *)NP2HeapAlloc(count + 1);
	char *tch = (char *)NP2HeapAlloc(2 + count * 4 + 8 + 1);
	int cch = 0;
	char *p = ch;
	uint64_t ci = 0;

	SendMessage(hwnd, SCI_GETSELTEXT, 0, (LPARAM)ch);

	while (*p) {
		if (*p == '0') {
			ci = 0;
			p++;
			if ((*p == 'x' || *p == 'X') && radix != 16) {
				p++;
				while (*p) {
					if (*p >= '0' && *p <= '9') {
						ci <<= 4;
						ci += (*p++ - '0');
					} else if (*p >= 'a' && *p <= 'f') {
						ci <<= 4;
						ci += (*p++ - 'a') + 10;;
					} else if (*p >= 'A' && *p <=  'F') {
						ci <<= 4;
						ci += (*p++ - 'A') + 10;
					} else if (*p == '_') {
						p++;
					} else {
						break;
					}
				}
				cch += ConvertNumRadix(tch + cch, ci, radix);
			} else if ((*p == 'o' || *p == 'O') && radix != 8) {
				p++;
				while (*p) {
					if (*p >= '0' && *p <= '7') {
						ci <<= 3;
						ci += (*p++ - '0');
					} else if (*p == '_') {
						p++;
					} else {
						break;
					}
				}
				cch += ConvertNumRadix(tch + cch, ci, radix);
			} else if ((*p == 'b' || *p == 'B') && radix != 2) {
				p++;
				while (*p) {
					if (*p == '0') {
						ci <<= 1;
						p++;
					} else if (*p == '1') {
						ci <<= 1;
						ci |= 1;
						p++;
					} else if (*p == '_') {
						p++;
					} else {
						break;
					}
				}
				cch += ConvertNumRadix(tch + cch, ci, radix);
			} else if ((*p >= '0' && *p <= '9') && radix != 10) {
				ci = *p++ - '0';
				while (*p) {
					if (*p >= '0' && *p <= '9') {
						ci *= 10;
						ci += (*p++ - '0');
					} else if (*p == '_') {
						p++;
					} else {
						break;
					}
				}
				cch += ConvertNumRadix(tch + cch, ci, radix);
			} else {
				tch[cch++] = '0';
			}
		} else if ((*p >= '1' && *p <= '9') && radix != 10) {
			ci = *p++ - '0';
			while (*p) {
				if (*p >= '0' && *p <= '9') {
					ci *= 10;
					ci += (*p++ - '0');
				} else if (*p == '_') {
					p++;
				} else {
					break;
				}
			}
			cch += ConvertNumRadix(tch + cch, ci, radix);
		} else if (iswordstart(*p)) {
			tch[cch++] = *p++;
			while (iswordchar(*p)) {
				tch[cch++] = *p++;
			}
		} else {
			tch[cch++] = *p++;
		}
	}
	tch[cch] = '\0';

	SendMessage(hwnd, SCI_SETTARGETSTART, iSelStart, 0);
	SendMessage(hwnd, SCI_SETTARGETEND, iSelEnd, 0);
	SendMessage(hwnd, SCI_REPLACETARGET, cch, (LPARAM)tch);
	SendMessage(hwnd, SCI_SETSEL, iSelStart, iSelStart + cch);
	NP2HeapFree(ch);
	NP2HeapFree(tch);
}

//=============================================================================
//
// EditModifyNumber()
//
void EditModifyNumber(HWND hwnd, BOOL bIncrease) {
	const int iSelStart = (int)SendMessage(hwnd, SCI_GETSELECTIONSTART, 0, 0);
	const int iSelEnd = (int)SendMessage(hwnd, SCI_GETSELECTIONEND, 0, 0);
	if (iSelStart == iSelEnd) {
		return;
	}
	if (SC_SEL_RECTANGLE == SendMessage(hwnd, SCI_GETSELECTIONMODE, 0, 0)) {
		MsgBox(MBWARN, IDS_SELRECT);
		return;
	}

	if (SendMessage(hwnd, SCI_GETSELTEXT, 0, 0) < 32) {
		char chNumber[32] = "";
		SendMessage(hwnd, SCI_GETSELTEXT, 0, (LPARAM)chNumber);
		if (strchr(chNumber, '-')) {
			return;
		}

		const int radix = StrChrIA(chNumber, 'x') ? 16 : 10;
		char *end;
		int iNumber = (int)strtol(chNumber, &end, radix);
		if (end == chNumber || iNumber < 0) {
			return;
		}

		if (bIncrease && iNumber < INT_MAX) {
			iNumber++;
		}
		if (!bIncrease && iNumber > 0) {
			iNumber--;
		}

		const int iWidth = lstrlenA(chNumber) - ((radix == 16) ? 2 : 0);
		char chFormat[32] = "";
		if (radix == 16) {
			const int len = iWidth + 1;
			BOOL bUppercase = FALSE;
			for (int i = len; i >= 0; i--) {
				if (IsCharLowerA(chNumber[i])) {
					break;
				}
				if (IsCharUpperA(chNumber[i])) {
					bUppercase = TRUE;
					break;
				}
			}
			if (bUppercase) {
				wsprintfA(chFormat, "%%#0%iX", iWidth);
			} else {
				wsprintfA(chFormat, "%%#0%ix", iWidth);
			}
		} else {
			wsprintfA(chFormat, "%%0%ii", iWidth);
		}
		wsprintfA(chNumber, chFormat, iNumber);
		SendMessage(hwnd, SCI_REPLACESEL, 0, (LPARAM)chNumber);
		SendMessage(hwnd, SCI_SETSEL, iSelStart, iSelStart + strlen(chNumber));
	}
}

extern int iTabWidth;
extern int iTabWidthG;
extern int iIndentWidth;
extern int iIndentWidthG;
extern BOOL bTabsAsSpaces;
extern BOOL bTabsAsSpacesG;
extern BOOL bTabIndents;
extern BOOL bTabIndentsG;

//=============================================================================
//
// EditTabsToSpaces()
//
void EditTabsToSpaces(HWND hwnd, int nTabWidth, BOOL bOnlyIndentingWS) {
	int iCurPos = (int)SendMessage(hwnd, SCI_GETCURRENTPOS, 0, 0);
	int iAnchorPos = (int)SendMessage(hwnd, SCI_GETANCHOR, 0, 0);
	if (iCurPos == iAnchorPos) {
		return;
	}
	if (SC_SEL_RECTANGLE == SendMessage(hwnd, SCI_GETSELECTIONMODE, 0, 0)) {
		MsgBox(MBWARN, IDS_SELRECT);
		return;
	}

	int iSelStart = (int)SendMessage(hwnd, SCI_GETSELECTIONSTART, 0, 0);
	const int iSelEnd = (int)SendMessage(hwnd, SCI_GETSELECTIONEND, 0, 0);

	const int iLine = (int)SendMessage(hwnd, SCI_LINEFROMPOSITION, iSelStart, 0);
	iSelStart = (int)SendMessage(hwnd, SCI_POSITIONFROMLINE, iLine, 0);

	const int iSelCount = iSelEnd - iSelStart;

	char *pszText = (char *)NP2HeapAlloc(iSelCount + 1);
	LPWSTR pszTextW = (LPWSTR)NP2HeapAlloc((iSelCount + 1) * sizeof(WCHAR));

	struct Sci_TextRange tr;
	tr.chrg.cpMin = iSelStart;
	tr.chrg.cpMax = iSelEnd;
	tr.lpstrText = pszText;
	SendMessage(hwnd, SCI_GETTEXTRANGE, 0, (LPARAM)&tr);

	const UINT cpEdit = (UINT)SendMessage(hwnd, SCI_GETCODEPAGE, 0, 0);
	const int cchTextW = MultiByteToWideChar(cpEdit, 0, pszText, iSelCount, pszTextW, (int)(NP2HeapSize(pszTextW) / sizeof(WCHAR)));
	NP2HeapFree(pszText);

	LPWSTR pszConvW = (LPWSTR)NP2HeapAlloc(cchTextW * sizeof(WCHAR) * nTabWidth + 2);
	int cchConvW = 0;

	BOOL bIsLineStart = TRUE;
	BOOL bModified = FALSE;
	// Contributed by Homam, Thank you very much!
	int i = 0;
	for (int iTextW = 0; iTextW < cchTextW; iTextW++) {
		const WCHAR w = pszTextW[iTextW];
		if (w == L'\t' && (!bOnlyIndentingWS || bIsLineStart)) {
			for (int j = 0; j < nTabWidth - i % nTabWidth; j++) {
				pszConvW[cchConvW++] = L' ';
			}
			i = 0;
			bModified = TRUE;
		} else {
			i++;
			if (w == L'\n' || w == L'\r') {
				i = 0;
				bIsLineStart = TRUE;
			} else if (w != L' ') {
				bIsLineStart = FALSE;
			}
			pszConvW[cchConvW++] = w;
		}
	}

	NP2HeapFree(pszTextW);

	if (bModified) {
		pszText = (char *)NP2HeapAlloc(cchConvW * kMaxMultiByteCount);

		const int cchConvM = WideCharToMultiByte(cpEdit, 0, pszConvW, cchConvW, pszText, (int)NP2HeapSize(pszText), NULL, NULL);

		if (iAnchorPos > iCurPos) {
			iCurPos = iSelStart;
			iAnchorPos = iSelStart + cchConvM;
		} else {
			iAnchorPos = iSelStart;
			iCurPos = iSelStart + cchConvM;
		}

		SendMessage(hwnd, SCI_BEGINUNDOACTION, 0, 0);
		SendMessage(hwnd, SCI_SETTARGETSTART, iSelStart, 0);
		SendMessage(hwnd, SCI_SETTARGETEND, iSelEnd, 0);
		SendMessage(hwnd, SCI_REPLACETARGET, cchConvM, (LPARAM)pszText);
		//SendMessage(hwnd, SCI_CLEAR, 0, 0);
		//SendMessage(hwnd, SCI_ADDTEXT, cchConvW, (LPARAM)pszText);
		SendMessage(hwnd, SCI_SETSEL, iAnchorPos, iCurPos);
		SendMessage(hwnd, SCI_ENDUNDOACTION, 0, 0);

		NP2HeapFree(pszText);
	}

	NP2HeapFree(pszConvW);
}

//=============================================================================
//
// EditSpacesToTabs()
//
void EditSpacesToTabs(HWND hwnd, int nTabWidth, BOOL bOnlyIndentingWS) {
	int iCurPos = (int)SendMessage(hwnd, SCI_GETCURRENTPOS, 0, 0);
	int iAnchorPos = (int)SendMessage(hwnd, SCI_GETANCHOR, 0, 0);
	if (iCurPos == iAnchorPos) {
		return;
	}
	if (SC_SEL_RECTANGLE == SendMessage(hwnd, SCI_GETSELECTIONMODE, 0, 0)) {
		MsgBox(MBWARN, IDS_SELRECT);
		return;
	}

	int iSelStart = (int)SendMessage(hwnd, SCI_GETSELECTIONSTART, 0, 0);
	const int iSelEnd = (int)SendMessage(hwnd, SCI_GETSELECTIONEND, 0, 0);

	const int iLine = (int)SendMessage(hwnd, SCI_LINEFROMPOSITION, iSelStart, 0);
	iSelStart = (int)SendMessage(hwnd, SCI_POSITIONFROMLINE, iLine, 0);

	const int iSelCount = iSelEnd - iSelStart;

	char *pszText = (char *)NP2HeapAlloc(iSelCount + 1);
	LPWSTR pszTextW = (LPWSTR)NP2HeapAlloc((iSelCount + 1) * sizeof(WCHAR));

	struct Sci_TextRange tr;
	tr.chrg.cpMin = iSelStart;
	tr.chrg.cpMax = iSelEnd;
	tr.lpstrText = pszText;
	SendMessage(hwnd, SCI_GETTEXTRANGE, 0, (LPARAM)&tr);

	const UINT cpEdit = (UINT)SendMessage(hwnd, SCI_GETCODEPAGE, 0, 0);

	const int cchTextW = MultiByteToWideChar(cpEdit, 0, pszText, iSelCount, pszTextW, (int)(NP2HeapSize(pszTextW) / sizeof(WCHAR)));
	NP2HeapFree(pszText);

	LPWSTR pszConvW = (LPWSTR)NP2HeapAlloc(cchTextW * sizeof(WCHAR) + 2);
	int cchConvW = 0;

	BOOL bIsLineStart = TRUE;
	BOOL bModified = FALSE;
	// Contributed by Homam, Thank you very much!
	int i = 0;
	int j = 0;
	WCHAR space[256];
	for (int iTextW = 0; iTextW < cchTextW; iTextW++) {
		const WCHAR w = pszTextW[iTextW];
		if ((w == L' ' || w == L'\t') && (!bOnlyIndentingWS ||  bIsLineStart)) {
			space[j++] = w;
			if (j == nTabWidth - i % nTabWidth || w == L'\t') {
				if (j > 1 || pszTextW[iTextW + 1] == L' ' || pszTextW[iTextW + 1] == L'\t') {
					pszConvW[cchConvW++] = L'\t';
				} else {
					pszConvW[cchConvW++] = w;
				}
				i = j = 0;
				bModified = bModified || (w != pszConvW[cchConvW - 1]);
			}
		} else {
			i += j + 1;
			if (j > 0) {
				//space[j] = '\0';
				for (int t = 0; t < j; t++) {
					pszConvW[cchConvW++] = space[t];
				}
				j = 0;
			}
			if (w == L'\n' || w == L'\r') {
				i = 0;
				bIsLineStart = TRUE;
			} else {
				bIsLineStart = FALSE;
			}
			pszConvW[cchConvW++] = w;
		}
	}

	if (j > 0) {
		for (int t = 0; t < j; t++) {
			pszConvW[cchConvW++] = space[t];
		}
	}

	NP2HeapFree(pszTextW);

	if (bModified || cchConvW != cchTextW) {
		pszText = (char *)NP2HeapAlloc(cchConvW * kMaxMultiByteCount + 1);
		const int cchConvM = WideCharToMultiByte(cpEdit, 0, pszConvW, cchConvW, pszText, (int)NP2HeapSize(pszText), NULL, NULL);

		if (iAnchorPos > iCurPos) {
			iCurPos = iSelStart;
			iAnchorPos = iSelStart + cchConvM;
		} else {
			iAnchorPos = iSelStart;
			iCurPos = iSelStart + cchConvM;
		}

		SendMessage(hwnd, SCI_BEGINUNDOACTION, 0, 0);
		SendMessage(hwnd, SCI_SETTARGETSTART, iSelStart, 0);
		SendMessage(hwnd, SCI_SETTARGETEND, iSelEnd, 0);
		SendMessage(hwnd, SCI_REPLACETARGET, cchConvM, (LPARAM)pszText);
		//SendMessage(hwnd, SCI_CLEAR, 0, 0);
		//SendMessage(hwnd, SCI_ADDTEXT, cchConvW, (LPARAM)pszText);
		SendMessage(hwnd, SCI_SETSEL, iAnchorPos, iCurPos);
		SendMessage(hwnd, SCI_ENDUNDOACTION, 0, 0);

		NP2HeapFree(pszText);
	}

	NP2HeapFree(pszConvW);
}

//=============================================================================
//
// EditMoveUp()
//
void EditMoveUp(HWND hwnd) {
	int iCurPos = (int)SendMessage(hwnd, SCI_GETCURRENTPOS, 0, 0);
	int iAnchorPos = (int)SendMessage(hwnd, SCI_GETANCHOR, 0, 0);
	const int iCurLine = (int)SendMessage(hwnd, SCI_LINEFROMPOSITION, iCurPos, 0);
	const int iAnchorLine = (int)SendMessage(hwnd, SCI_LINEFROMPOSITION, iAnchorPos, 0);

	if (iCurLine == iAnchorLine) {
		const int iLineCurPos = iCurPos - (int)SendMessage(hwnd, SCI_POSITIONFROMLINE, iCurLine, 0);
		const int iLineAnchorPos = iAnchorPos - (int)SendMessage(hwnd, SCI_POSITIONFROMLINE, iAnchorLine, 0);
		//const int iLineCur = (int)SendMessage(hwnd, SCI_DOCLINEFROMVISIBLE, iCurLine, 0);
		//const int iLineAnchor = (int)SendMessage(hwnd, SCI_DOCLINEFROMVISIBLE, iAnchorLine, 0);
		//if (iLineCur == iLineAnchor) {
		//}

		if (iCurLine > 0) {
			SendMessage(hwnd, SCI_BEGINUNDOACTION, 0, 0);
			SendMessage(hwnd, SCI_LINETRANSPOSE, 0, 0);
			SendMessage(hwnd, SCI_SETSEL,
						SendMessage(hwnd, SCI_POSITIONFROMLINE, iAnchorLine - 1, 0) + iLineAnchorPos,
						SendMessage(hwnd, SCI_POSITIONFROMLINE, iCurLine - 1, 0) + iLineCurPos);
			SendMessage(hwnd, SCI_CHOOSECARETX, 0, 0);
			SendMessage(hwnd, SCI_ENDUNDOACTION, 0, 0);
		}
		return;
	}

	if (SC_SEL_RECTANGLE == SendMessage(hwnd, SCI_GETSELECTIONMODE, 0, 0)) {
		MsgBox(MBWARN, IDS_SELRECT);
		return;
	}

	const int iLineSrc = min_i(iCurLine, iAnchorLine) - 1;
	//const int iLineCur = (int)SendMessage(hwnd, SCI_DOCLINEFROMVISIBLE, iCurLine, 0);
	//const int iLineAnchor = (int)SendMessage(hwnd, SCI_DOCLINEFROMVISIBLE, iAnchorLine, 0);
	//if (iLineCur == iLineAnchor) {
	//}

	if (iLineSrc >= 0) {
		const int cLine = (int)SendMessage(hwnd, SCI_GETLINE, iLineSrc, 0);
		char *pLine = (char *)NP2HeapAlloc(cLine + 1);
		SendMessage(hwnd, SCI_GETLINE, iLineSrc, (LPARAM)pLine);

		const int iLineSrcStart = (int)SendMessage(hwnd, SCI_POSITIONFROMLINE, iLineSrc, 0);
		const int iLineSrcEnd = (int)SendMessage(hwnd, SCI_POSITIONFROMLINE, iLineSrc + 1, 0);

		int iLineDest = max_i(iCurLine, iAnchorLine);
		if (max_i(iCurPos, iAnchorPos) <= SendMessage(hwnd, SCI_POSITIONFROMLINE, iLineDest, 0)) {
			if (iLineDest >= 1) {
				iLineDest--;
			}
		}

		SendMessage(hwnd, SCI_BEGINUNDOACTION, 0, 0);

		SendMessage(hwnd, SCI_SETTARGETSTART, iLineSrcStart, 0);
		SendMessage(hwnd, SCI_SETTARGETEND, iLineSrcEnd, 0);
		SendMessage(hwnd, SCI_REPLACETARGET, 0, (LPARAM)"");

		const int iLineDestStart = (int)SendMessage(hwnd, SCI_POSITIONFROMLINE, iLineDest, 0);
		SendMessage(hwnd, SCI_INSERTTEXT, iLineDestStart, (LPARAM)pLine);
		NP2HeapFree(pLine);

		if (iLineDest == SendMessage(hwnd, SCI_GETLINECOUNT, 0, 0) - 1) {
			char chaEOL[] = "\r\n";
			const int iEOLMode = (int)SendMessage(hwnd, SCI_GETEOLMODE, 0, 0);
			if (iEOLMode == SC_EOL_CR) {
				chaEOL[1] = 0;
			} else if (iEOLMode == SC_EOL_LF) {
				chaEOL[0] = '\n';
				chaEOL[1] = 0;
			}

			SendMessage(hwnd, SCI_INSERTTEXT, iLineDestStart, (LPARAM)chaEOL);
			SendMessage(hwnd, SCI_SETTARGETSTART, SendMessage(hwnd, SCI_GETLINEENDPOSITION, iLineDest, 0), 0);
			SendMessage(hwnd, SCI_SETTARGETEND, SendMessage(hwnd, SCI_GETLENGTH, 0, 0), 0);
			SendMessage(hwnd, SCI_REPLACETARGET, 0, (LPARAM)"");
		}

		if (iCurPos < iAnchorPos) {
			iCurPos = (int)SendMessage(hwnd, SCI_POSITIONFROMLINE, iCurLine - 1, 0);
			iAnchorPos = (int)SendMessage(hwnd, SCI_POSITIONFROMLINE, iLineDest, 0);
		} else {
			iAnchorPos = (int)SendMessage(hwnd, SCI_POSITIONFROMLINE, iAnchorLine - 1, 0);
			iCurPos = (int)SendMessage(hwnd, SCI_POSITIONFROMLINE, iLineDest, 0);
		}

		SendMessage(hwnd, SCI_SETSEL, iAnchorPos, iCurPos);

		SendMessage(hwnd, SCI_ENDUNDOACTION, 0, 0);
	}
}

//=============================================================================
//
// EditMoveDown()
//
void EditMoveDown(HWND hwnd) {
	int iCurPos = (int)SendMessage(hwnd, SCI_GETCURRENTPOS, 0, 0);
	int iAnchorPos = (int)SendMessage(hwnd, SCI_GETANCHOR, 0, 0);
	const int iCurLine = (int)SendMessage(hwnd, SCI_LINEFROMPOSITION, iCurPos, 0);
	const int iAnchorLine = (int)SendMessage(hwnd, SCI_LINEFROMPOSITION, iAnchorPos, 0);

	if (iCurLine == iAnchorLine) {
		const int iLineCurPos = iCurPos - (int)SendMessage(hwnd, SCI_POSITIONFROMLINE, iCurLine, 0);
		const int iLineAnchorPos = iAnchorPos - (int)SendMessage(hwnd, SCI_POSITIONFROMLINE, iAnchorLine, 0);
		//const int iLineCur = (int)SendMessage(hwnd, SCI_DOCLINEFROMVISIBLE, iCurLine, 0);
		//const int iLineAnchor = (int)SendMessage(hwnd, SCI_DOCLINEFROMVISIBLE, iAnchorLine, 0);
		//if (iLineCur == iLineAnchor) {
		//}

		if (iCurLine < SendMessage(hwnd, SCI_GETLINECOUNT, 0, 0) - 1) {
			SendMessage(hwnd, SCI_BEGINUNDOACTION, 0, 0);
			SendMessage(hwnd, SCI_GOTOLINE, iCurLine + 1, 0);
			SendMessage(hwnd, SCI_LINETRANSPOSE, 0, 0);
			SendMessage(hwnd, SCI_SETSEL,
						SendMessage(hwnd, SCI_POSITIONFROMLINE, iAnchorLine + 1, 0) + iLineAnchorPos,
						SendMessage(hwnd, SCI_POSITIONFROMLINE, iCurLine + 1, 0) + iLineCurPos);
			SendMessage(hwnd, SCI_CHOOSECARETX, 0, 0);
			SendMessage(hwnd, SCI_ENDUNDOACTION, 0, 0);
		}
		return;
	}

	if (SC_SEL_RECTANGLE == SendMessage(hwnd, SCI_GETSELECTIONMODE, 0, 0)) {
		MsgBox(MBWARN, IDS_SELRECT);
		return;
	}

	int iLineSrc = max_i(iCurLine, iAnchorLine) + 1;
	//const int iLineCur = (int)SendMessage(hwnd, SCI_DOCLINEFROMVISIBLE, iCurLine, 0);
	//const int iLineAnchor = (int)SendMessage(hwnd, SCI_DOCLINEFROMVISIBLE, iAnchorLine, 0);
	//if (iLineCur == iLineAnchor) {
	//}

	if (max_i(iCurPos, iAnchorPos) <= SendMessage(hwnd, SCI_POSITIONFROMLINE, iLineSrc - 1, 0)) {
		if (iLineSrc >= 1) {
			iLineSrc--;
		}
	}

	if (iLineSrc <= SendMessage(hwnd, SCI_GETLINECOUNT, 0, 0) - 1) {
		const BOOL bLastLine = (iLineSrc == SendMessage(hwnd, SCI_GETLINECOUNT, 0, 0) - 1);

		if (bLastLine &&
				(SendMessage(hwnd, SCI_GETLINEENDPOSITION, iLineSrc, 0) -
				 SendMessage(hwnd, SCI_POSITIONFROMLINE, iLineSrc, 0) == 0) &&
				(SendMessage(hwnd, SCI_GETLINEENDPOSITION, iLineSrc - 1, 0) -
				 SendMessage(hwnd, SCI_POSITIONFROMLINE, iLineSrc - 1, 0) == 0)) {
			return;
		}

		if (bLastLine) {
			char chaEOL[] = "\r\n";
			const int iEOLMode = (int)SendMessage(hwnd, SCI_GETEOLMODE, 0, 0);
			if (iEOLMode == SC_EOL_CR) {
				chaEOL[1] = 0;
			} else if (iEOLMode == SC_EOL_LF) {
				chaEOL[0] = '\n';
				chaEOL[1] = 0;
			}
			SendMessage(hwnd, SCI_APPENDTEXT, strlen(chaEOL), (LPARAM)chaEOL);
		}

		const int cLine = (int)SendMessage(hwnd, SCI_GETLINE, iLineSrc, 0);
		char *pLine = (char *)NP2HeapAlloc(cLine + 3);
		SendMessage(hwnd, SCI_GETLINE, iLineSrc, (LPARAM)pLine);

		const int iLineSrcStart = (int)SendMessage(hwnd, SCI_POSITIONFROMLINE, iLineSrc, 0);
		const int iLineSrcEnd = (int)SendMessage(hwnd, SCI_POSITIONFROMLINE, iLineSrc + 1, 0);
		const int iLineDest = min_i(iCurLine, iAnchorLine);

		SendMessage(hwnd, SCI_BEGINUNDOACTION, 0, 0);

		SendMessage(hwnd, SCI_SETTARGETSTART, iLineSrcStart, 0);
		SendMessage(hwnd, SCI_SETTARGETEND, iLineSrcEnd, 0);
		SendMessage(hwnd, SCI_REPLACETARGET, 0, (LPARAM)"");

		const int iLineDestStart = (int)SendMessage(hwnd, SCI_POSITIONFROMLINE, iLineDest, 0);
		SendMessage(hwnd, SCI_INSERTTEXT, iLineDestStart, (LPARAM)pLine);

		if (bLastLine) {
			SendMessage(hwnd, SCI_SETTARGETSTART,
						SendMessage(hwnd, SCI_GETLINEENDPOSITION,
									(SendMessage(hwnd, SCI_GETLINECOUNT, 0, 0) - 2), 0), 0);
			SendMessage(hwnd, SCI_SETTARGETEND, SendMessage(hwnd, SCI_GETLENGTH, 0, 0), 0);
			SendMessage(hwnd, SCI_REPLACETARGET, 0, (LPARAM)"");
		}

		NP2HeapFree(pLine);

		if (iCurPos < iAnchorPos) {
			iCurPos = (int)SendMessage(hwnd, SCI_POSITIONFROMLINE, iCurLine + 1, 0);
			iAnchorPos = (int)SendMessage(hwnd, SCI_POSITIONFROMLINE, iLineSrc + 1, 0);
		} else {
			iAnchorPos = (int)SendMessage(hwnd, SCI_POSITIONFROMLINE, iAnchorLine + 1, 0);
			iCurPos = (int)SendMessage(hwnd, SCI_POSITIONFROMLINE, iLineSrc + 1, 0);
		}

		SendMessage(hwnd, SCI_SETSEL, iAnchorPos, iCurPos);

		SendMessage(hwnd, SCI_ENDUNDOACTION, 0, 0);
	}
}

// only convert CR+LF
static void ConvertWinEditLineEndingsEx(char *s, int iEOLMode, int *linesCount) {
	int count = 0;
	if (iEOLMode != SC_EOL_CRLF) {
		char *p = s;
		const char chaEOL = (iEOLMode == SC_EOL_LF) ? '\n' : '\r';
		while (*s) {
			switch (*s) {
			case '\r':
				++count;
				if (s[1] == '\n') {
					++s;
					*p++ = chaEOL;
				} else {
					*p++ = '\r';
				}
				++s;
				break;

			case '\n':
				++count;
				*p++ = '\n';
				++s;
				break;

			default:
				*p++ = *s++;
				break;
			}
		}

		*p = '\0';
		if (linesCount != NULL) {
			*linesCount = count;
		}
	} else if (linesCount != NULL) {
		while (*s) {
			switch (*s++) {
			case '\r':
				++count;
				if (*s == '\n') {
					++s;
				}
				break;

			case '\n':
				++count;
				break;
			}
		}
		*linesCount = count;
	}
}

static inline void ConvertWinEditLineEndings(char *s, int iEOLMode) {
	ConvertWinEditLineEndingsEx(s, iEOLMode, NULL);
}

//=============================================================================
//
// EditModifyLines()
//
void EditModifyLines(HWND hwnd, LPCWSTR pwszPrefix, LPCWSTR pwszAppend) {
	if (SC_SEL_RECTANGLE == SendMessage(hwnd, SCI_GETSELECTIONMODE, 0, 0)) {
		MsgBox(MBWARN, IDS_SELRECT);
		return;
	}
	if (StrIsEmpty(pwszPrefix) && StrIsEmpty(pwszAppend)) {
		return;
	}

	BeginWaitCursor();

	const UINT cpEdit = (UINT)SendMessage(hwnd, SCI_GETCODEPAGE, 0, 0);
	const int iEOLMode = (int)SendMessage(hwnd, SCI_GETEOLMODE, 0, 0);

	const int iPrefixLen = lstrlen(pwszPrefix);
	char *mszPrefix1 = NULL;
	int iPrefixLine = 0;
	if (iPrefixLen != 0) {
		const int size = iPrefixLen * kMaxMultiByteCount + 1;
		mszPrefix1 = (char *)NP2HeapAlloc(size);
		WideCharToMultiByte(cpEdit, 0, pwszPrefix, -1, mszPrefix1, size, NULL, NULL);
		ConvertWinEditLineEndingsEx(mszPrefix1, iEOLMode, &iPrefixLine);
	}

	const int iAppendLen = lstrlen(pwszAppend);
	char *mszAppend1 = NULL;
	int iAppendLine = 0;
	if (iAppendLen != 0) {
		const int size = iAppendLen * kMaxMultiByteCount + 1;
		mszAppend1 = (char *)NP2HeapAlloc(size);
		WideCharToMultiByte(cpEdit, 0, pwszAppend, -1, mszAppend1, size, NULL, NULL);
		ConvertWinEditLineEndingsEx(mszAppend1, iEOLMode, &iAppendLine);
	}

	const int iSelStart = (int)SendMessage(hwnd, SCI_GETSELECTIONSTART, 0, 0);
	const int iSelEnd = (int)SendMessage(hwnd, SCI_GETSELECTIONEND, 0, 0);

	const int iLineStart = (int)SendMessage(hwnd, SCI_LINEFROMPOSITION, iSelStart, 0);
	int iLineEnd = (int)SendMessage(hwnd, SCI_LINEFROMPOSITION, iSelEnd, 0);

	//if (iSelStart > SendMessage(hwnd, SCI_POSITIONFROMLINE, iLineStart, 0)) {
	//	iLineStart++;
	//}

	if (iSelEnd <= SendMessage(hwnd, SCI_POSITIONFROMLINE, iLineEnd, 0)) {
		if (iLineEnd - iLineStart >= 1) {
			iLineEnd--;
		}
	}

	const char *pszPrefixNumPad = "";
	char *mszPrefix2 = NULL;
	int iPrefixNum = 0;
	int iPrefixNumWidth = 1;
	BOOL bPrefixNum = FALSE;

	const char *pszAppendNumPad = "";
	char *mszAppend2 = NULL;
	int iAppendNum = 0;
	int iAppendNumWidth = 1;
	BOOL bAppendNum = FALSE;

	if (iPrefixLen != 0) {
		char *p = mszPrefix1;
		mszPrefix2 = (char *)NP2HeapAlloc(iPrefixLen * kMaxMultiByteCount + 1);
		while (!bPrefixNum && (p = StrStrA(p, "$(")) != NULL) {
			if (strncmp(p, "$(I)", CSTRLEN("$(I)")) == 0) {
				*p = 0;
				strcpy(mszPrefix2, p + CSTRLEN("$(I)"));
				bPrefixNum = TRUE;
				iPrefixNum = 0;
				for (int i = iLineEnd - iLineStart; i >= 10; i = i / 10) {
					iPrefixNumWidth++;
				}
				pszPrefixNumPad = "";
			} else if (strncmp(p, "$(0I)", CSTRLEN("$(0I)")) == 0) {
				*p = 0;
				strcpy(mszPrefix2, p + CSTRLEN("$(0I)"));
				bPrefixNum = TRUE;
				iPrefixNum = 0;
				for (int i = iLineEnd - iLineStart; i >= 10; i = i / 10) {
					iPrefixNumWidth++;
				}
				pszPrefixNumPad = "0";
			} else if (strncmp(p, "$(N)", CSTRLEN("$(N)")) == 0) {
				*p = 0;
				strcpy(mszPrefix2, p + CSTRLEN("$(N)"));
				bPrefixNum = TRUE;
				iPrefixNum = 1;
				for (int i = iLineEnd - iLineStart + 1; i >= 10; i = i / 10) {
					iPrefixNumWidth++;
				}
				pszPrefixNumPad = "";
			} else if (strncmp(p, "$(0N)", CSTRLEN("$(0N)")) == 0) {
				*p = 0;
				strcpy(mszPrefix2, p + CSTRLEN("$(0N)"));
				bPrefixNum = TRUE;
				iPrefixNum = 1;
				for (int i = iLineEnd - iLineStart + 1; i >= 10; i = i / 10) {
					iPrefixNumWidth++;
				}
				pszPrefixNumPad = "0";
			} else if (strncmp(p, "$(L)", CSTRLEN("$(L)")) == 0) {
				*p = 0;
				strcpy(mszPrefix2, p + CSTRLEN("$(L)"));
				bPrefixNum = TRUE;
				iPrefixNum = iLineStart + 1;
				for (int i = iLineEnd + 1; i >= 10; i = i / 10) {
					iPrefixNumWidth++;
				}
				pszPrefixNumPad = "";
			} else if (strncmp(p, "$(0L)", CSTRLEN("$(0L)")) == 0) {
				*p = 0;
				strcpy(mszPrefix2, p + CSTRLEN("$(0L)"));
				bPrefixNum = TRUE;
				iPrefixNum = iLineStart + 1;
				for (int i = iLineEnd + 1; i >= 10; i = i / 10) {
					iPrefixNumWidth++;
				}
				pszPrefixNumPad = "0";
			}
			p += CSTRLEN("$(");
		}
	}

	if (iAppendLen != 0) {
		char *p = mszAppend1;
		mszAppend2 = (char *)NP2HeapAlloc(iAppendLen * kMaxMultiByteCount + 1);
		while (!bAppendNum && (p = StrStrA(p, "$(")) != NULL) {
			if (strncmp(p, "$(I)", CSTRLEN("$(I)")) == 0) {
				*p = 0;
				strcpy(mszAppend2, p + CSTRLEN("$(I)"));
				bAppendNum = TRUE;
				iAppendNum = 0;
				for (int i = iLineEnd - iLineStart; i >= 10; i = i / 10) {
					iAppendNumWidth++;
				}
				pszAppendNumPad = "";
			} else if (strncmp(p, "$(0I)", CSTRLEN("$(0I)")) == 0) {
				*p = 0;
				strcpy(mszAppend2, p + CSTRLEN("$(0I)"));
				bAppendNum = TRUE;
				iAppendNum = 0;
				for (int i = iLineEnd - iLineStart; i >= 10; i = i / 10) {
					iAppendNumWidth++;
				}
				pszAppendNumPad = "0";
			} else if (strncmp(p, "$(N)", CSTRLEN("$(N)")) == 0) {
				*p = 0;
				strcpy(mszAppend2, p + CSTRLEN("$(N)"));
				bAppendNum = TRUE;
				iAppendNum = 1;
				for (int i = iLineEnd - iLineStart + 1; i >= 10; i = i / 10) {
					iAppendNumWidth++;
				}
				pszAppendNumPad = "";
			} else if (strncmp(p, "$(0N)", CSTRLEN("$(0N)")) == 0) {
				*p = 0;
				strcpy(mszAppend2, p + CSTRLEN("$(0N)"));
				bAppendNum = TRUE;
				iAppendNum = 1;
				for (int i = iLineEnd - iLineStart + 1; i >= 10; i = i / 10) {
					iAppendNumWidth++;
				}
				pszAppendNumPad = "0";
			} else if (strncmp(p, "$(L)", CSTRLEN("$(L)")) == 0) {
				*p = 0;
				strcpy(mszAppend2, p + CSTRLEN("$(L)"));
				bAppendNum = TRUE;
				iAppendNum = iLineStart + 1;
				for (int i = iLineEnd + 1; i >= 10; i = i / 10) {
					iAppendNumWidth++;
				}
				pszAppendNumPad = "";
			} else if (strncmp(p, "$(0L)", CSTRLEN("$(0L)")) == 0) {
				*p = 0;
				strcpy(mszAppend2, p + CSTRLEN("$(0L)"));
				bAppendNum = TRUE;
				iAppendNum = iLineStart + 1;
				for (int i = iLineEnd + 1; i >= 10; i = i / 10) {
					iAppendNumWidth++;
				}
				pszAppendNumPad = "0";
			}
			p += CSTRLEN("$(");
		}
	}

	char *mszInsert = (char *)NP2HeapAlloc(2 * max_i(iPrefixLen, iAppendLen) * kMaxMultiByteCount + 1);
	SendMessage(hwnd, SCI_BEGINUNDOACTION, 0, 0);
	for (int iLine = iLineStart, iLineDest = iLineStart; iLine <= iLineEnd; iLine++, iLineDest++) {
		if (iPrefixLen != 0) {
			strcpy(mszInsert, mszPrefix1);

			if (bPrefixNum) {
				char tchFmt[64];
				char tchNum[64];
				wsprintfA(tchFmt, "%%%s%ii", pszPrefixNumPad, iPrefixNumWidth);
				wsprintfA(tchNum, tchFmt, iPrefixNum);
				strcat(mszInsert, tchNum);
				strcat(mszInsert, mszPrefix2);
				iPrefixNum++;
			}

			const int iPos = (int)SendMessage(hwnd, SCI_POSITIONFROMLINE, iLineDest, 0);
			SendMessage(hwnd, SCI_SETTARGETSTART, iPos, 0);
			SendMessage(hwnd, SCI_SETTARGETEND, iPos, 0);
			SendMessage(hwnd, SCI_REPLACETARGET, strlen(mszInsert), (LPARAM)mszInsert);
			iLineDest += iPrefixLine;
		}

		if (iAppendLen != 0) {
			strcpy(mszInsert, mszAppend1);

			if (bAppendNum) {
				char tchFmt[64];
				char tchNum[64];
				wsprintfA(tchFmt, "%%%s%ii", pszAppendNumPad, iAppendNumWidth);
				wsprintfA(tchNum, tchFmt, iAppendNum);
				strcat(mszInsert, tchNum);
				strcat(mszInsert, mszAppend2);
				iAppendNum++;
			}

			const int iPos = (int)SendMessage(hwnd, SCI_GETLINEENDPOSITION, iLineDest, 0);
			SendMessage(hwnd, SCI_SETTARGETSTART, iPos, 0);
			SendMessage(hwnd, SCI_SETTARGETEND, iPos, 0);
			SendMessage(hwnd, SCI_REPLACETARGET, strlen(mszInsert), (LPARAM)mszInsert);
			iLineDest += iAppendLine;
		}
	}
	SendMessage(hwnd, SCI_ENDUNDOACTION, 0, 0);

	//// Fix selection
	//if (iSelStart != iSelEnd && SendMessage(hwnd, SCI_GETTARGETEND, 0, 0) > SendMessage(hwnd, SCI_GETSELECTIONEND, 0, 0)) {
	//	int iCurPos = SendMessage(hwnd, SCI_GETCURRENTPOS, 0, 0);
	//	int iAnchorPos = SendMessage(hwnd, SCI_GETANCHOR, 0, 0);
	//	if (iCurPos > iAnchorPos)
	//		iCurPos = SendMessage(hwnd, SCI_GETTARGETEND, 0, 0);
	//	else
	//		iAnchorPos = SendMessage(hwnd, SCI_GETTARGETEND, 0, 0);
	//	SendMessage(hwnd, SCI_SETSEL, iAnchorPos, iCurPos);
	//}

	// extend selection to start of first line
	// the above code is not required when last line has been excluded
	if (iSelStart != iSelEnd) {
		int iCurPos = (int)SendMessage(hwnd, SCI_GETCURRENTPOS, 0, 0);
		int iAnchorPos = (int)SendMessage(hwnd, SCI_GETANCHOR, 0, 0);
		if (iCurPos < iAnchorPos) {
			iCurPos = (int)SendMessage(hwnd, SCI_POSITIONFROMLINE, iLineStart, 0);
			iAnchorPos = (int)SendMessage(hwnd, SCI_POSITIONFROMLINE, iLineEnd + 1, 0);
		} else {
			iAnchorPos = (int)SendMessage(hwnd, SCI_POSITIONFROMLINE, iLineStart, 0);
			iCurPos = (int)SendMessage(hwnd, SCI_POSITIONFROMLINE, iLineEnd + 1, 0);
		}
		SendMessage(hwnd, SCI_SETSEL, iAnchorPos, iCurPos);
	}

	EndWaitCursor();
	if (mszPrefix1 != NULL) {
		NP2HeapFree(mszPrefix1);
	}
	if (mszAppend1 != NULL) {
		NP2HeapFree(mszAppend1);
	}
	if (mszPrefix2 != NULL) {
		NP2HeapFree(mszPrefix2);
	}
	if (mszAppend2 != NULL) {
		NP2HeapFree(mszAppend2);
	}
	NP2HeapFree(mszInsert);
}

//=============================================================================
//
// EditAlignText()
//
void EditAlignText(HWND hwnd, int nMode) {
	if (SC_SEL_RECTANGLE == SendMessage(hwnd, SCI_GETSELECTIONMODE, 0, 0)) {
		MsgBox(MBWARN, IDS_SELRECT);
		return;
	}

#define BUFSIZE_ALIGN 1024

	const int iSelStart = (int)SendMessage(hwnd, SCI_GETSELECTIONSTART, 0, 0);
	const int iSelEnd = (int)SendMessage(hwnd, SCI_GETSELECTIONEND, 0, 0);
	int iCurPos = (int)SendMessage(hwnd, SCI_GETCURRENTPOS, 0, 0);
	int iAnchorPos = (int)SendMessage(hwnd, SCI_GETANCHOR, 0, 0);
	const UINT cpEdit = (UINT)SendMessage(hwnd, SCI_GETCODEPAGE, 0, 0);

	BOOL bModified = FALSE;
	const int iLineStart = (int)SendMessage(hwnd, SCI_LINEFROMPOSITION, iSelStart, 0);
	int iLineEnd = (int)SendMessage(hwnd, SCI_LINEFROMPOSITION, iSelEnd, 0);

	if (iSelEnd <= SendMessage(hwnd, SCI_POSITIONFROMLINE, iLineEnd, 0)) {
		if (iLineEnd - iLineStart >= 1) {
			iLineEnd--;
		}
	}

	int iMinIndent = BUFSIZE_ALIGN;
	int iMaxLength = 0;
	for (int iLine = iLineStart; iLine <= iLineEnd; iLine++) {
		int iLineEndPos = (int)SendMessage(hwnd, SCI_GETLINEENDPOSITION, iLine, 0);
		const int iLineIndentPos = (int)SendMessage(hwnd, SCI_GETLINEINDENTPOSITION, iLine, 0);

		if (iLineIndentPos != iLineEndPos) {
			const int iIndentCol = (int)SendMessage(hwnd, SCI_GETLINEINDENTATION, iLine, 0);
			int iTail = iLineEndPos - 1;
			int ch = (int)SendMessage(hwnd, SCI_GETCHARAT, iTail, 0);
			while (iTail >= iLineStart && (ch == ' ' || ch == '\t')) {
				iTail--;
				ch = (int)SendMessage(hwnd, SCI_GETCHARAT, iTail, 0);
				iLineEndPos--;
			}

			const int iEndCol = (int)SendMessage(hwnd, SCI_GETCOLUMN, iLineEndPos, 0);
			iMinIndent = min_i(iMinIndent, iIndentCol);
			iMaxLength = max_i(iMaxLength, iEndCol);
		}
	}

	if (iMaxLength < BUFSIZE_ALIGN) {
		for (int iLine = iLineStart; iLine <= iLineEnd; iLine++) {
			const int iIndentPos = (int)SendMessage(hwnd, SCI_GETLINEINDENTPOSITION, iLine, 0);
			const int iEndPos = (int)SendMessage(hwnd, SCI_GETLINEENDPOSITION, iLine, 0);

			if (iIndentPos == iEndPos && iEndPos > 0) {
				if (!bModified) {
					SendMessage(hwnd, SCI_BEGINUNDOACTION, 0, 0);
					bModified = TRUE;
				}

				SendMessage(hwnd, SCI_SETTARGETSTART, SendMessage(hwnd, SCI_POSITIONFROMLINE, iLine, 0), 0);
				SendMessage(hwnd, SCI_SETTARGETEND, iEndPos, 0);
				SendMessage(hwnd, SCI_REPLACETARGET, 0, (LPARAM)"");
			} else {
				char tchLineBuf[BUFSIZE_ALIGN * kMaxMultiByteCount] = "";
				WCHAR wchLineBuf[BUFSIZE_ALIGN] = L"";
				WCHAR *pWords[BUFSIZE_ALIGN];
				WCHAR *p = wchLineBuf;

				int iWords = 0;
				int iWordsLength = 0;
				const int cchLine = (int)SendMessage(hwnd, SCI_GETLINE, iLine, (LPARAM)tchLineBuf);

				if (!bModified) {
					SendMessage(hwnd, SCI_BEGINUNDOACTION, 0, 0);
					bModified = TRUE;
				}

				MultiByteToWideChar(cpEdit, 0, tchLineBuf, cchLine, wchLineBuf, COUNTOF(wchLineBuf));
				StrTrim(wchLineBuf, L"\r\n\t ");

				while (*p) {
					if (*p != L' ' && *p != L'\t') {
						pWords[iWords++] = p++;
						iWordsLength++;
						while (*p && *p != L' ' && *p != L'\t') {
							p++;
							iWordsLength++;
						}
					} else {
						*p++ = 0;
					}
				}

				if (iWords > 0) {
					if (nMode == ALIGN_JUSTIFY || nMode == ALIGN_JUSTIFY_EX) {
						BOOL bNextLineIsBlank = FALSE;
						if (nMode == ALIGN_JUSTIFY_EX) {
							if (SendMessage(hwnd, SCI_GETLINECOUNT, 0, 0) <= iLine + 1) {
								bNextLineIsBlank = TRUE;
							} else {
								const int iLineEndPos = (int)SendMessage(hwnd, SCI_GETLINEENDPOSITION, iLine + 1, 0);
								const int iLineIndentPos = (int)SendMessage(hwnd, SCI_GETLINEINDENTPOSITION, iLine + 1, 0);
								if (iLineIndentPos == iLineEndPos) {
									bNextLineIsBlank = TRUE;
								}
							}
						}

						if ((nMode == ALIGN_JUSTIFY || nMode == ALIGN_JUSTIFY_EX) &&
								iWords > 1 && iWordsLength >= 2 &&
								((nMode != ALIGN_JUSTIFY_EX || !bNextLineIsBlank || iLineStart == iLineEnd) ||
								 (bNextLineIsBlank && iWordsLength > (iMaxLength - iMinIndent) * 0.75))) {
							const int iGaps = iWords - 1;
							const int iSpacesPerGap = (iMaxLength - iMinIndent - iWordsLength) / iGaps;
							const int iExtraSpaces = (iMaxLength - iMinIndent - iWordsLength) % iGaps;

							WCHAR wchNewLineBuf[BUFSIZE_ALIGN * 3];
							lstrcpy(wchNewLineBuf, pWords[0]);
							p = StrEnd(wchNewLineBuf);

							for (int i = 1; i < iWords; i++) {
								for (int j = 0; j < iSpacesPerGap; j++) {
									*p++ = L' ';
									*p = 0;
								}
								if (i > iGaps - iExtraSpaces) {
									*p++ = L' ';
									*p = 0;
								}
								lstrcat(p, pWords[i]);
								p = StrEnd(p);
							}

							WideCharToMultiByte(cpEdit, 0, wchNewLineBuf, -1, tchLineBuf, COUNTOF(tchLineBuf), NULL, NULL);

							int iPos = (int)SendMessage(hwnd, SCI_POSITIONFROMLINE, iLine, 0);
							SendMessage(hwnd, SCI_SETTARGETSTART, iPos, 0);
							iPos = (int)SendMessage(hwnd, SCI_GETLINEENDPOSITION, iLine, 0);
							SendMessage(hwnd, SCI_SETTARGETEND, iPos, 0);
							SendMessage(hwnd, SCI_REPLACETARGET, strlen(tchLineBuf), (LPARAM)tchLineBuf);

							SendMessage(hwnd, SCI_SETLINEINDENTATION, iLine, iMinIndent);
						} else {
							WCHAR wchNewLineBuf[BUFSIZE_ALIGN];
							lstrcpy(wchNewLineBuf, pWords[0]);
							p = StrEnd(wchNewLineBuf);

							for (int i = 1; i < iWords; i++) {
								*p++ = L' ';
								*p = 0;
								lstrcat(wchNewLineBuf, pWords[i]);
								p = StrEnd(p);
							}

							WideCharToMultiByte(cpEdit, 0, wchNewLineBuf, -1, tchLineBuf, COUNTOF(tchLineBuf), NULL, NULL);

							int iPos = (int)SendMessage(hwnd, SCI_POSITIONFROMLINE, iLine, 0);
							SendMessage(hwnd, SCI_SETTARGETSTART, iPos, 0);
							iPos = (int)SendMessage(hwnd, SCI_GETLINEENDPOSITION, iLine, 0);
							SendMessage(hwnd, SCI_SETTARGETEND, iPos, 0);
							SendMessage(hwnd, SCI_REPLACETARGET, strlen(tchLineBuf), (LPARAM)tchLineBuf);

							SendMessage(hwnd, SCI_SETLINEINDENTATION, iLine, iMinIndent);
						}
					} else {
						const int iExtraSpaces = iMaxLength - iMinIndent - iWordsLength - iWords + 1;
						int iOddSpaces = iExtraSpaces % 2;

						WCHAR wchNewLineBuf[BUFSIZE_ALIGN * 3] = L"";
						p = wchNewLineBuf;

						if (nMode == ALIGN_RIGHT) {
							for (int i = 0; i < iExtraSpaces; i++) {
								*p++ = L' ';
							}
							*p = 0;
						}

						if (nMode == ALIGN_CENTER) {
							for (int i = 1; i < iExtraSpaces - iOddSpaces; i += 2) {
								*p++ = L' ';
							}
							*p = 0;
						}

						for (int i = 0; i < iWords; i++) {
							lstrcat(p, pWords[i]);
							if (i < iWords - 1) {
								lstrcat(p, L" ");
							}
							if (nMode == ALIGN_CENTER && iWords > 1 && iOddSpaces > 0 && i + 1 >= iWords / 2) {
								lstrcat(p, L" ");
								iOddSpaces--;
							}
							p = StrEnd(p);
						}

						WideCharToMultiByte(cpEdit, 0, wchNewLineBuf, -1, tchLineBuf, COUNTOF(tchLineBuf), NULL, NULL);

						int iPos;
						if (nMode == ALIGN_RIGHT || nMode == ALIGN_CENTER) {
							SendMessage(hwnd, SCI_SETLINEINDENTATION, iLine, iMinIndent);
							iPos = (int)SendMessage(hwnd, SCI_GETLINEINDENTPOSITION, iLine, 0);
						} else {
							iPos = (int)SendMessage(hwnd, SCI_POSITIONFROMLINE, iLine, 0);
						}

						SendMessage(hwnd, SCI_SETTARGETSTART, iPos, 0);
						iPos = (int)SendMessage(hwnd, SCI_GETLINEENDPOSITION, iLine, 0);
						SendMessage(hwnd, SCI_SETTARGETEND, iPos, 0);
						SendMessage(hwnd, SCI_REPLACETARGET, strlen(tchLineBuf), (LPARAM)tchLineBuf);

						if (nMode == ALIGN_LEFT) {
							SendMessage(hwnd, SCI_SETLINEINDENTATION, iLine, iMinIndent);
						}
					}
				}
			}
		}
		if (bModified) {
			SendMessage(hwnd, SCI_ENDUNDOACTION, 0, 0);
		}
	} else {
		MsgBox(MBINFO, IDS_BUFFERTOOSMALL);
	}

	if (iCurPos < iAnchorPos) {
		iCurPos = (int)SendMessage(hwnd, SCI_POSITIONFROMLINE, iLineStart, 0);
		iAnchorPos = (int)SendMessage(hwnd, SCI_POSITIONFROMLINE, iLineEnd + 1, 0);
	} else {
		iAnchorPos = (int)SendMessage(hwnd, SCI_POSITIONFROMLINE, iLineStart, 0);
		iCurPos = (int)SendMessage(hwnd, SCI_POSITIONFROMLINE, iLineEnd + 1, 0);
	}
	SendMessage(hwnd, SCI_SETSEL, iAnchorPos, iCurPos);
}

//=============================================================================
//
// EditEncloseSelection()
//
void EditEncloseSelection(HWND hwnd, LPCWSTR pwszOpen, LPCWSTR pwszClose) {
	if (SC_SEL_RECTANGLE == SendMessage(hwnd, SCI_GETSELECTIONMODE, 0, 0)) {
		MsgBox(MBWARN, IDS_SELRECT);
		return;
	}
	if (StrIsEmpty(pwszOpen) && StrIsEmpty(pwszClose)) {
		return;
	}

	BeginWaitCursor();

	const int iSelStart = (int)SendMessage(hwnd, SCI_GETSELECTIONSTART, 0, 0);
	const int iSelEnd = (int)SendMessage(hwnd, SCI_GETSELECTIONEND, 0, 0);
	const UINT cpEdit = (UINT)SendMessage(hwnd, SCI_GETCODEPAGE, 0, 0);
	const int iEOLMode = (int)SendMessage(hwnd, SCI_GETEOLMODE, 0, 0);

	char *mszOpen = NULL;
	int len = lstrlen(pwszOpen);
	if (len != 0) {
		const int size = kMaxMultiByteCount * len + 1;
		mszOpen = (char *)NP2HeapAlloc(size);
		WideCharToMultiByte(cpEdit, 0, pwszOpen, -1, mszOpen, size, NULL, NULL);
		ConvertWinEditLineEndings(mszOpen, iEOLMode);
	}

	char *mszClose = NULL;
	len = lstrlen(pwszClose);
	if (len != 0) {
		const int size = kMaxMultiByteCount * len + 1;
		mszClose = (char *)NP2HeapAlloc(size);
		WideCharToMultiByte(cpEdit, 0, pwszClose, -1, mszClose, size, NULL, NULL);
		ConvertWinEditLineEndings(mszClose, iEOLMode);
	}

	SendMessage(hwnd, SCI_BEGINUNDOACTION, 0, 0);
	len = lstrlenA(mszOpen);

	if (StrNotEmptyA(mszOpen)) {
		SendMessage(hwnd, SCI_SETTARGETSTART, iSelStart, 0);
		SendMessage(hwnd, SCI_SETTARGETEND, iSelStart, 0);
		SendMessage(hwnd, SCI_REPLACETARGET, len, (LPARAM)mszOpen);
	}

	if (StrNotEmptyA(mszClose)) {
		SendMessage(hwnd, SCI_SETTARGETSTART, iSelEnd + len, 0);
		SendMessage(hwnd, SCI_SETTARGETEND, iSelEnd + len, 0);
		SendMessage(hwnd, SCI_REPLACETARGET, strlen(mszClose), (LPARAM)mszClose);
	}

	SendMessage(hwnd, SCI_ENDUNDOACTION, 0, 0);

	// Fix selection
	if (iSelStart == iSelEnd) {
		SendMessage(hwnd, SCI_SETSEL, iSelStart + len, iSelStart + len);
	} else {
		int iCurPos = (int)SendMessage(hwnd, SCI_GETCURRENTPOS, 0, 0);
		int iAnchorPos = (int)SendMessage(hwnd, SCI_GETANCHOR, 0, 0);

		if (iCurPos < iAnchorPos) {
			iCurPos = iSelStart + len;
			iAnchorPos = iSelEnd + len;
		} else {
			iAnchorPos = iSelStart + len;
			iCurPos = iSelEnd + len;
		}
		SendMessage(hwnd, SCI_SETSEL, iAnchorPos, iCurPos);
	}

	EndWaitCursor();
	if (mszOpen != NULL) {
		NP2HeapFree(mszOpen);
	}
	if (mszClose != NULL) {
		NP2HeapFree(mszClose);
	}
}

//=============================================================================
//
// EditToggleLineComments()
//
void EditToggleLineComments(HWND hwnd, LPCWSTR pwszComment, BOOL bInsertAtStart) {
	if (SC_SEL_RECTANGLE == SendMessage(hwnd, SCI_GETSELECTIONMODE, 0, 0)) {
		MsgBox(MBWARN, IDS_SELRECT);
		return;
	}

	BeginWaitCursor();

	const int iSelStart = (int)SendMessage(hwnd, SCI_GETSELECTIONSTART, 0, 0);
	const int iSelEnd = (int)SendMessage(hwnd, SCI_GETSELECTIONEND, 0, 0);
	int iCurPos = (int)SendMessage(hwnd, SCI_GETCURRENTPOS, 0, 0);

	char mszComment[256] = "";
	const UINT cpEdit = (UINT)SendMessage(hwnd, SCI_GETCODEPAGE, 0, 0);
	WideCharToMultiByte(cpEdit, 0, pwszComment, -1, mszComment, COUNTOF(mszComment), NULL, NULL);

	const int cchComment = lstrlenA(mszComment);
	const int iLineStart = (int)SendMessage(hwnd, SCI_LINEFROMPOSITION, iSelStart, 0);
	int iLineEnd = (int)SendMessage(hwnd, SCI_LINEFROMPOSITION, iSelEnd, 0);

	if (iSelEnd <= SendMessage(hwnd, SCI_POSITIONFROMLINE, iLineEnd, 0)) {
		if (iLineEnd - iLineStart >= 1) {
			iLineEnd--;
		}
	}

	int iCommentCol = 0;
	if (!bInsertAtStart) {
		iCommentCol = 1024;
		for (int iLine = iLineStart; iLine <= iLineEnd; iLine++) {
			const int iLineEndPos = (int)SendMessage(hwnd, SCI_GETLINEENDPOSITION, iLine, 0);
			const int iLineIndentPos = (int)SendMessage(hwnd, SCI_GETLINEINDENTPOSITION, iLine, 0);

			if (iLineIndentPos != iLineEndPos) {
				const int iIndentColumn = (int)SendMessage(hwnd, SCI_GETCOLUMN, iLineIndentPos, 0);
				iCommentCol = min_i(iCommentCol, iIndentColumn);
			}
		}
	}

	SendMessage(hwnd, SCI_BEGINUNDOACTION, 0, 0);
	int iAction = 0;

	for (int iLine = iLineStart; iLine <= iLineEnd; iLine++) {
		const int iIndentPos = (int)SendMessage(hwnd, SCI_GETLINEINDENTPOSITION, iLine, 0);
		BOOL bWhitespaceLine = FALSE;
		// a line with [space/tab] only
		if (iCommentCol && iIndentPos == SendMessage(hwnd, SCI_GETLINEENDPOSITION, iLine, 0)) {
			//continue;
			bWhitespaceLine = TRUE;
		}

		char tchBuf[32] = "";
		struct Sci_TextRange tr;
		tr.chrg.cpMin = iIndentPos;
		tr.chrg.cpMax = tr.chrg.cpMin + min_i(31, cchComment);
		tr.lpstrText = tchBuf;
		SendMessage(hwnd, SCI_GETTEXTRANGE, 0, (LPARAM)&tr);

		int iCommentPos;
		int ch;
		if (_strnicmp(tchBuf, mszComment, cchComment) == 0) {
			switch (iAction) {
			case 0:
				iAction = 2;
#if defined(__cplusplus)
			[[fallthrough]];
#endif
			// fall through
			case 2:
				iCommentPos = iIndentPos;
				// a line with [space/tab] comment only
				ch = (int)SendMessage(hwnd, SCI_GETCHARAT, iIndentPos + cchComment, 0);
				if (ch == '\n' || ch == '\r') {
					iCommentPos = (int)SendMessage(hwnd, SCI_POSITIONFROMLINE, iLine, 0);
				}
				SendMessage(hwnd, SCI_SETTARGETSTART, iCommentPos, 0);
				SendMessage(hwnd, SCI_SETTARGETEND, (iIndentPos + cchComment), 0);
				SendMessage(hwnd, SCI_REPLACETARGET, 0, (LPARAM)"");
				break;
			case 1:
				iCommentPos = (int)SendMessage(hwnd, SCI_FINDCOLUMN, iLine, iCommentCol);
				ch = (int)SendMessage(hwnd, SCI_GETCHARAT, iCommentPos, 0);
				if (ch == '\t' || ch == ' ') {
					SendMessage(hwnd, SCI_INSERTTEXT, iCommentPos, (LPARAM)mszComment);
				}
				break;
			}
		} else {
			switch (iAction) {
			case 0:
				iAction = 1;
#if defined(__cplusplus)
			[[fallthrough]];
#endif
			// fall through
			case 1:
				iCommentPos = (int)SendMessage(hwnd, SCI_FINDCOLUMN, iLine, iCommentCol);
				if (!bWhitespaceLine || (iLineStart == iLineEnd)) {
					SendMessage(hwnd, SCI_INSERTTEXT, iCommentPos, (LPARAM)mszComment);
				} else {
					char tchComment[1024] = "";
					ch = 0;
					int count = iCommentCol;
					if (!bTabsAsSpaces && iTabWidth > 0) {
						ch = iCommentCol / iTabWidth;
						memset(tchComment, '\t', ch);
						count -= ch * iTabWidth;
					}
					memset(tchComment + ch, ' ', count);
					strcat(tchComment, mszComment);
					SendMessage(hwnd, SCI_INSERTTEXT, iCommentPos, (LPARAM)tchComment);
				}
				break;
			case 2:
				break;
			}
		}
	}

	SendMessage(hwnd, SCI_ENDUNDOACTION, 0, 0);

	if (iSelStart != iSelEnd) {
		int iAnchorPos;
		if (iCurPos == iSelStart) {
			iCurPos = (int)SendMessage(hwnd, SCI_POSITIONFROMLINE, iLineStart, 0);
			iAnchorPos = (int)SendMessage(hwnd, SCI_POSITIONFROMLINE, iLineEnd + 1, 0);
		} else {
			iAnchorPos = (int)SendMessage(hwnd, SCI_POSITIONFROMLINE, iLineStart, 0);
			iCurPos = (int)SendMessage(hwnd, SCI_POSITIONFROMLINE, iLineEnd + 1, 0);
		}
		SendMessage(hwnd, SCI_SETSEL, iAnchorPos, iCurPos);
	}

	EndWaitCursor();
}

//=============================================================================
//
// EditPadWithSpaces()
//
void EditPadWithSpaces(HWND hwnd, BOOL bSkipEmpty, BOOL bNoUndoGroup) {
	int iMaxColumn = 0;
	BOOL bReducedSelection = FALSE;

	int iSelStart = 0;
	int iSelEnd = 0;

	int iLineStart = 0;
	int iLineEnd = 0;

	int iRcCurLine = 0;
	int iRcAnchorLine = 0;
	int iRcCurCol = 0;
	int iRcAnchorCol = 0;

	const BOOL bIsRectangular = SC_SEL_RECTANGLE == SendMessage(hwnd, SCI_GETSELECTIONMODE, 0, 0);
	if (!bIsRectangular ) {
		iSelStart = (int)SendMessage(hwnd, SCI_GETSELECTIONSTART, 0, 0);
		iSelEnd = (int)SendMessage(hwnd, SCI_GETSELECTIONEND, 0, 0);

		iLineStart = (int)SendMessage(hwnd, SCI_LINEFROMPOSITION, iSelStart, 0);
		iLineEnd = (int)SendMessage(hwnd, SCI_LINEFROMPOSITION, iSelEnd, 0);

		if (iLineStart == iLineEnd) {
			iLineStart = 0;
			iLineEnd = (int)SendMessage(hwnd, SCI_GETLINECOUNT, 0, 0) - 1;
		} else {
			if (iSelEnd <= SendMessage(hwnd, SCI_POSITIONFROMLINE, iLineEnd, 0)) {
				if (iLineEnd - iLineStart >= 1) {
					iLineEnd--;
					bReducedSelection = TRUE;
				}
			}
		}

		for (int iLine = iLineStart; iLine <= iLineEnd; iLine++) {
			const int iPos = (int)SendMessage(hwnd, SCI_GETLINEENDPOSITION, iLine, 0);
			iMaxColumn = max_i(iMaxColumn, (int)SendMessage(hwnd, SCI_GETCOLUMN, iPos, 0));
		}
	} else {
		const int iCurPos = (int)SendMessage(hwnd, SCI_GETCURRENTPOS, 0, 0);
		const int iAnchorPos = (int)SendMessage(hwnd, SCI_GETANCHOR, 0, 0);

		iRcCurLine = (int)SendMessage(hwnd, SCI_LINEFROMPOSITION, iCurPos, 0);
		iRcAnchorLine = (int)SendMessage(hwnd, SCI_LINEFROMPOSITION, iAnchorPos, 0);

		iRcCurCol = (int)SendMessage(hwnd, SCI_GETCOLUMN, iCurPos, 0);
		iRcAnchorCol = (int)SendMessage(hwnd, SCI_GETCOLUMN, iAnchorPos, 0);

		iLineStart = 0;
		iLineEnd = (int)SendMessage(hwnd, SCI_GETLINECOUNT, 0, 0) - 1;

		for (int iLine = iLineStart; iLine <= iLineEnd; iLine++) {
			const int iPos = (int)SendMessage(hwnd, SCI_GETLINESELENDPOSITION, iLine, 0);
			if (iPos != INVALID_POSITION) {
				iMaxColumn = max_i(iMaxColumn, (int)SendMessage(hwnd, SCI_GETCOLUMN, iPos, 0));
			}
		}
	}

	char *pmszPadStr = (char *)NP2HeapAlloc((iMaxColumn + 1) * sizeof(char));
	if (pmszPadStr) {
		FillMemory(pmszPadStr, NP2HeapSize(pmszPadStr), ' ');

		if (!bNoUndoGroup) {
			SendMessage(hwnd, SCI_BEGINUNDOACTION, 0, 0);
		}

		for (int iLine = iLineStart; iLine <= iLineEnd; iLine++) {
			const int iLineSelEndPos = (int)SendMessage(hwnd, SCI_GETLINESELENDPOSITION, iLine, 0);
			if (bIsRectangular && INVALID_POSITION == iLineSelEndPos) {
				continue;
			}

			const int iPos = (int)SendMessage(hwnd, SCI_GETLINEENDPOSITION, iLine, 0);
			if (bIsRectangular && iPos > iLineSelEndPos) {
				continue;
			}

			if (bSkipEmpty && (int)SendMessage(hwnd, SCI_POSITIONFROMLINE, iLine, 0) >= iPos) {
				continue;
			}

			const int iPadLen = iMaxColumn - (int)SendMessage(hwnd, SCI_GETCOLUMN, iPos, 0);

			SendMessage(hwnd, SCI_SETTARGETSTART, iPos, 0);
			SendMessage(hwnd, SCI_SETTARGETEND, iPos, 0);
			SendMessage(hwnd, SCI_REPLACETARGET, iPadLen, (LPARAM)pmszPadStr);
		}

		NP2HeapFree(pmszPadStr);

		if (!bNoUndoGroup) {
			SendMessage(hwnd, SCI_ENDUNDOACTION, 0, 0);
		}
	}

	if (!bIsRectangular &&
			SendMessage(hwnd, SCI_LINEFROMPOSITION, iSelStart, 0) !=
			SendMessage(hwnd, SCI_LINEFROMPOSITION, iSelEnd, 0)) {
		int iCurPos = (int)SendMessage(hwnd, SCI_GETCURRENTPOS, 0, 0);
		int iAnchorPos = (int)SendMessage(hwnd, SCI_GETANCHOR, 0, 0);

		if (iCurPos < iAnchorPos) {
			iCurPos = (int)SendMessage(hwnd, SCI_POSITIONFROMLINE, iLineStart, 0);
			if (!bReducedSelection) {
				iAnchorPos = (int)SendMessage(hwnd, SCI_GETLINEENDPOSITION, iLineEnd, 0);
			} else {
				iAnchorPos = (int)SendMessage(hwnd, SCI_POSITIONFROMLINE, iLineEnd + 1, 0);
			}
		} else {
			iAnchorPos = (int)SendMessage(hwnd, SCI_POSITIONFROMLINE, iLineStart, 0);
			if (!bReducedSelection) {
				iCurPos = (int)SendMessage(hwnd, SCI_GETLINEENDPOSITION, iLineEnd, 0);
			} else {
				iCurPos = (int)SendMessage(hwnd, SCI_POSITIONFROMLINE, iLineEnd + 1, 0);
			}
		}
		SendMessage(hwnd, SCI_SETSEL, iAnchorPos, iCurPos);
	} else if (bIsRectangular) {
		const int iCurPos = (int)SendMessage(hwnd, SCI_FINDCOLUMN, iRcCurLine, iRcCurCol);
		const int iAnchorPos = (int)SendMessage(hwnd, SCI_FINDCOLUMN, iRcAnchorLine, iRcAnchorCol);

		SendMessage(hwnd, SCI_SETRECTANGULARSELECTIONCARET, iCurPos, 0);
		SendMessage(hwnd, SCI_SETRECTANGULARSELECTIONANCHOR, iAnchorPos, 0);
	}
}

//=============================================================================
//
// EditStripFirstCharacter()
//
void EditStripFirstCharacter(HWND hwnd) {
	if (SC_SEL_RECTANGLE == SendMessage(hwnd, SCI_GETSELECTIONMODE, 0, 0)) {
		MsgBox(MBWARN, IDS_SELRECT);
		return;
	}

	const int iSelStart = (int)SendMessage(hwnd, SCI_GETSELECTIONSTART, 0, 0);
	const int iSelEnd = (int)SendMessage(hwnd, SCI_GETSELECTIONEND, 0, 0);

	int iLineStart = (int)SendMessage(hwnd, SCI_LINEFROMPOSITION, iSelStart, 0);
	int iLineEnd = (int)SendMessage(hwnd, SCI_LINEFROMPOSITION, iSelEnd, 0);

	if (iLineStart != iLineEnd) {
		if (iSelStart > SendMessage(hwnd, SCI_POSITIONFROMLINE, iLineStart, 0)) {
			iLineStart++;
		}

		if (iSelEnd <= SendMessage(hwnd, SCI_POSITIONFROMLINE, iLineEnd, 0)) {
			iLineEnd--;
		}
	}

	SendMessage(hwnd, SCI_BEGINUNDOACTION, 0, 0);

	for (int iLine = iLineStart; iLine <= iLineEnd; iLine++) {
		const int iPos = (int)SendMessage(hwnd, SCI_POSITIONFROMLINE, iLine, 0);
		if (SendMessage(hwnd, SCI_GETLINEENDPOSITION, iLine, 0) - iPos > 0) {
			SendMessage(hwnd, SCI_SETTARGETSTART, iPos, 0);
			SendMessage(hwnd, SCI_SETTARGETEND, SendMessage(hwnd, SCI_POSITIONAFTER, iPos, 0), 0);
			SendMessage(hwnd, SCI_REPLACETARGET, 0, (LPARAM)"");
		}
	}
	SendMessage(hwnd, SCI_ENDUNDOACTION, 0, 0);
}

//=============================================================================
//
// EditStripLastCharacter()
//
void EditStripLastCharacter(HWND hwnd) {
	if (SC_SEL_RECTANGLE == SendMessage(hwnd, SCI_GETSELECTIONMODE, 0, 0)) {
		MsgBox(MBWARN, IDS_SELRECT);
		return;
	}

	const int iSelStart = (int)SendMessage(hwnd, SCI_GETSELECTIONSTART, 0, 0);
	const int iSelEnd = (int)SendMessage(hwnd, SCI_GETSELECTIONEND, 0, 0);

	int iLineStart = (int)SendMessage(hwnd, SCI_LINEFROMPOSITION, iSelStart, 0);
	int iLineEnd = (int)SendMessage(hwnd, SCI_LINEFROMPOSITION, iSelEnd, 0);

	if (iLineStart != iLineEnd) {
		if (iSelStart >= SendMessage(hwnd, SCI_GETLINEENDPOSITION, iLineStart, 0)) {
			iLineStart++;
		}

		if (iSelEnd < SendMessage(hwnd, SCI_GETLINEENDPOSITION, iLineEnd, 0)) {
			iLineEnd--;
		}
	}

	SendMessage(hwnd, SCI_BEGINUNDOACTION, 0, 0);

	for (int iLine = iLineStart; iLine <= iLineEnd; iLine++) {
		const int iStartPos = (int)SendMessage(hwnd, SCI_POSITIONFROMLINE, iLine, 0);
		const int iEndPos = (int)SendMessage(hwnd, SCI_GETLINEENDPOSITION, iLine, 0);

		if (iEndPos - iStartPos > 0) {
			SendMessage(hwnd, SCI_SETTARGETSTART, SendMessage(hwnd, SCI_POSITIONBEFORE, iEndPos, 0), 0);
			SendMessage(hwnd, SCI_SETTARGETEND, iEndPos, 0);
			SendMessage(hwnd, SCI_REPLACETARGET, 0, (LPARAM)"");
		}
	}
	SendMessage(hwnd, SCI_ENDUNDOACTION, 0, 0);
}

//=============================================================================
//
// EditStripTrailingBlanks()
//
void EditStripTrailingBlanks(HWND hwnd, BOOL bIgnoreSelection) {
	// Check if there is any selection... simply use a regular expression replace!
	if (!bIgnoreSelection && !EditIsEmptySelection()) {
		if (SC_SEL_RECTANGLE != SendMessage(hwnd, SCI_GETSELECTIONMODE, 0, 0)) {
#if NP2_USE_DESIGNATED_INITIALIZER
			EDITFINDREPLACE efrTrim = {
				.szFind = "[ \t]+$",
				.hwnd = hwnd,
				.fuFlags = SCFIND_REGEXP,
			};
#else
			EDITFINDREPLACE efrTrim = { "[ \t]+$", "", "", "", hwnd, SCFIND_REGEXP };
#endif
			if (EditReplaceAllInSelection(hwnd, &efrTrim, FALSE)) {
				return;
			}
		}
	}

	// Code from SciTE...
	SendMessage(hwnd, SCI_BEGINUNDOACTION, 0, 0);
	const int maxLines = (int)SendMessage(hwnd, SCI_GETLINECOUNT, 0, 0);
	for (int line = 0; line < maxLines; line++) {
		const int lineStart = (int)SendMessage(hwnd, SCI_POSITIONFROMLINE, line, 0);
		const int lineEnd = (int)SendMessage(hwnd, SCI_GETLINEENDPOSITION, line, 0);
		int i = lineEnd - 1;
		int ch = (int)SendMessage(hwnd, SCI_GETCHARAT, i, 0);
		while ((i >= lineStart) && (ch == ' ' || ch == '\t')) {
			i--;
			ch = (int)SendMessage(hwnd, SCI_GETCHARAT, i, 0);
		}
		if (i < (lineEnd - 1)) {
			SendMessage(hwnd, SCI_SETTARGETSTART, i + 1, 0);
			SendMessage(hwnd, SCI_SETTARGETEND, lineEnd, 0);
			SendMessage(hwnd, SCI_REPLACETARGET, 0, (LPARAM)"");
		}
	}
	SendMessage(hwnd, SCI_ENDUNDOACTION, 0, 0);
}

//=============================================================================
//
// EditStripLeadingBlanks()
//
void EditStripLeadingBlanks(HWND hwnd, BOOL bIgnoreSelection) {
	// Check if there is any selection... simply use a regular expression replace!
	if (!bIgnoreSelection && !EditIsEmptySelection()) {
		if (SC_SEL_RECTANGLE != SendMessage(hwnd, SCI_GETSELECTIONMODE, 0, 0)) {
#if NP2_USE_DESIGNATED_INITIALIZER
			EDITFINDREPLACE efrTrim = {
				.szFind = "^[ \t]+",
				.hwnd = hwnd,
				.fuFlags = SCFIND_REGEXP,
			};
#else
			EDITFINDREPLACE efrTrim = { "^[ \t]+", "", "", "", hwnd, SCFIND_REGEXP };
#endif
			if (EditReplaceAllInSelection(hwnd, &efrTrim, FALSE)) {
				return;
			}
		}
	}

	// Code from SciTE...
	SendMessage(hwnd, SCI_BEGINUNDOACTION, 0, 0);
	const int maxLines = (int)SendMessage(hwnd, SCI_GETLINECOUNT, 0, 0);
	for (int line = 0; line < maxLines; line++) {
		const int lineStart = (int)SendMessage(hwnd, SCI_POSITIONFROMLINE, line, 0);
		const int lineEnd = (int)SendMessage(hwnd, SCI_GETLINEENDPOSITION, line, 0);
		int i = lineStart;
		int ch = (int)SendMessage(hwnd, SCI_GETCHARAT, i, 0);
		while ((i <= lineEnd - 1) && (ch == ' ' || ch == '\t')) {
			i++;
			ch = (int)SendMessage(hwnd, SCI_GETCHARAT, i, 0);
		}
		if (i > lineStart) {
			SendMessage(hwnd, SCI_SETTARGETSTART, lineStart, 0);
			SendMessage(hwnd, SCI_SETTARGETEND, i, 0);
			SendMessage(hwnd, SCI_REPLACETARGET, 0, (LPARAM)"");
		}
	}
	SendMessage(hwnd, SCI_ENDUNDOACTION, 0, 0);
}

//=============================================================================
//
// EditCompressSpaces()
//
void EditCompressSpaces(HWND hwnd) {
	if (SC_SEL_RECTANGLE == SendMessage(hwnd, SCI_GETSELECTIONMODE, 0, 0)) {
		MsgBox(MBWARN, IDS_SELRECT);
		return;
	}

	const int iSelStart = (int)SendMessage(hwnd, SCI_GETSELECTIONSTART, 0, 0);
	const int iSelEnd = (int)SendMessage(hwnd, SCI_GETSELECTIONEND, 0, 0);
	int iCurPos = (int)SendMessage(hwnd, SCI_GETCURRENTPOS, 0, 0);
	int iAnchorPos = (int)SendMessage(hwnd, SCI_GETANCHOR, 0, 0);
	const int iLength = (int)SendMessage(hwnd, SCI_GETLENGTH, 0, 0);

	char *pszIn;
	char *pszOut;
	BOOL bIsLineStart;
	BOOL bIsLineEnd;

	if (iSelStart != iSelEnd) {
		const int iLineStart = (int)SendMessage(hwnd, SCI_LINEFROMPOSITION, iSelStart, 0);
		const int iLineEnd = (int)SendMessage(hwnd, SCI_LINEFROMPOSITION, iSelEnd, 0);
		const int cch = (int)SendMessage(hwnd, SCI_GETSELTEXT, 0, 0);
		pszIn = (char *)NP2HeapAlloc(cch);
		pszOut = (char *)NP2HeapAlloc(cch);
		SendMessage(hwnd, SCI_GETSELTEXT, 0, (LPARAM)pszIn);
		bIsLineStart =
			(iSelStart == SendMessage(hwnd, SCI_POSITIONFROMLINE, iLineStart, 0));
		bIsLineEnd =
			(iSelEnd == SendMessage(hwnd, SCI_GETLINEENDPOSITION, iLineEnd, 0));
	} else {
		const int cch = iLength + 1;
		pszIn = (char *)NP2HeapAlloc(cch);
		pszOut = (char *)NP2HeapAlloc(cch);
		SendMessage(hwnd, SCI_GETTEXT, cch, (LPARAM)pszIn);
		bIsLineStart = TRUE;
		bIsLineEnd = TRUE;
	}

	BOOL bModified = FALSE;
	char *ci;
	char *co = pszOut;
	for (ci = pszIn; *ci; ci++) {
		if (*ci == ' ' || *ci == '\t') {
			if (*ci == '\t') {
				bModified = TRUE;
			}
			while (*(ci + 1) == ' ' || *(ci + 1) == '\t') {
				ci++;
				bModified = TRUE;
			}
			if (!bIsLineStart && (*(ci + 1) != '\n' && *(ci + 1) != '\r')) {
				*co++ = ' ';
			} else {
				bModified = TRUE;
			}
		} else {
			if (*ci == '\n' || *ci == '\r') {
				bIsLineStart = TRUE;
			} else {
				bIsLineStart = FALSE;
			}
			*co++ = *ci;
		}
	}
	if (bIsLineEnd && co > pszOut && *(co - 1) == ' ') {
		*--co = 0;
		bModified = TRUE;
	}

	if (bModified) {
		if (iSelStart != iSelEnd) {
			SendMessage(hwnd, SCI_TARGETFROMSELECTION, 0, 0);
		} else {
			SendMessage(hwnd, SCI_SETTARGETSTART, 0, 0);
			SendMessage(hwnd, SCI_SETTARGETEND, iLength, 0);
		}
		SendMessage(hwnd, SCI_BEGINUNDOACTION, 0, 0);
		SendMessage(hwnd, SCI_REPLACETARGET, (WPARAM)(-1), (LPARAM)pszOut);
		if (iCurPos > iAnchorPos) {
			iCurPos = (int)SendMessage(hwnd, SCI_GETTARGETEND, 0, 0);
			iAnchorPos = (int)SendMessage(hwnd, SCI_GETTARGETSTART, 0, 0);
			SendMessage(hwnd, SCI_SETSEL, iAnchorPos, iCurPos);
		} else if (iCurPos < iAnchorPos) {
			iCurPos = (int)SendMessage(hwnd, SCI_GETTARGETSTART, 0, 0);
			iAnchorPos = (int)SendMessage(hwnd, SCI_GETTARGETEND, 0, 0);
			SendMessage(hwnd, SCI_SETSEL, iAnchorPos, iCurPos);
		}
		SendMessage(hwnd, SCI_ENDUNDOACTION, 0, 0);
	}

	NP2HeapFree(pszIn);
	NP2HeapFree(pszOut);
}

//=============================================================================
//
// EditRemoveBlankLines()
//
void EditRemoveBlankLines(HWND hwnd, BOOL bMerge) {
	if (SC_SEL_RECTANGLE == SendMessage(hwnd, SCI_GETSELECTIONMODE, 0, 0)) {
		MsgBox(MBWARN, IDS_SELRECT);
		return;
	}

	int iSelStart = (int)SendMessage(hwnd, SCI_GETSELECTIONSTART, 0, 0);
	int iSelEnd = (int)SendMessage(hwnd, SCI_GETSELECTIONEND, 0, 0);

	if (iSelStart == iSelEnd) {
		iSelStart = 0;
		iSelEnd = (int)SendMessage(hwnd, SCI_GETLENGTH, 0, 0);
	}

	int iLineStart = (int)SendMessage(hwnd, SCI_LINEFROMPOSITION, iSelStart, 0);
	int iLineEnd = (int)SendMessage(hwnd, SCI_LINEFROMPOSITION, iSelEnd, 0);

	if (iSelStart > SendMessage(hwnd, SCI_POSITIONFROMLINE, iLineStart, 0)) {
		iLineStart++;
	}

	if (iSelEnd <= SendMessage(hwnd, SCI_POSITIONFROMLINE, iLineEnd, 0) &&
			iLineEnd != SendMessage(hwnd, SCI_GETLINECOUNT, 0, 0) - 1) {
		iLineEnd--;
	}

	SendMessage(hwnd, SCI_BEGINUNDOACTION, 0, 0);

	for (int iLine = iLineStart; iLine <= iLineEnd;) {
		int nBlanks = 0;
		while (iLine + nBlanks <= iLineEnd &&
				SendMessage(hwnd, SCI_POSITIONFROMLINE, iLine + nBlanks, 0) ==
				SendMessage(hwnd, SCI_GETLINEENDPOSITION, iLine + nBlanks, 0)) {
			nBlanks++;
		}

		if (nBlanks == 0 || (nBlanks == 1 && bMerge)) {
			iLine += nBlanks + 1;
		} else {
			if (bMerge) {
				nBlanks--;
			}

			const int iTargetStart = (int)SendMessage(hwnd, SCI_POSITIONFROMLINE, iLine, 0);
			const int iTargetEnd = (int)SendMessage(hwnd, SCI_POSITIONFROMLINE, iLine + nBlanks, 0);

			SendMessage(hwnd, SCI_SETTARGETSTART, iTargetStart, 0);
			SendMessage(hwnd, SCI_SETTARGETEND, iTargetEnd, 0);
			SendMessage(hwnd, SCI_REPLACETARGET, 0, (LPARAM)"");

			if (bMerge) {
				iLine++;
			}
			iLineEnd -= nBlanks;
		}
	}
	SendMessage(hwnd, SCI_ENDUNDOACTION, 0, 0);
}

//=============================================================================
//
// EditWrapToColumn()
//
void EditWrapToColumn(HWND hwnd, int nColumn/*, int nTabWidth*/) {
	int iCurPos = (int)SendMessage(hwnd, SCI_GETCURRENTPOS, 0, 0);
	int iAnchorPos = (int)SendMessage(hwnd, SCI_GETANCHOR, 0, 0);
	if (iCurPos == iAnchorPos) {
		return;
	}
	if (SC_SEL_RECTANGLE == SendMessage(hwnd, SCI_GETSELECTIONMODE, 0, 0)) {
		MsgBox(MBWARN, IDS_SELRECT);
		return;
	}

	int iSelStart = (int)SendMessage(hwnd, SCI_GETSELECTIONSTART, 0, 0);
	const int iSelEnd = (int)SendMessage(hwnd, SCI_GETSELECTIONEND, 0, 0);
	const int iSelCount = iSelEnd - iSelStart;

	const int iLine = (int)SendMessage(hwnd, SCI_LINEFROMPOSITION, iSelStart, 0);
	iSelStart = (int)SendMessage(hwnd, SCI_POSITIONFROMLINE, iLine, 0);

	char *pszText = (char *)NP2HeapAlloc(iSelCount + 1 + 2);
	LPWSTR pszTextW = (LPWSTR)NP2HeapAlloc((iSelCount + 1 + 2) * sizeof(WCHAR));

	struct Sci_TextRange tr;
	tr.chrg.cpMin = iSelStart;
	tr.chrg.cpMax = iSelEnd;
	tr.lpstrText = pszText;
	SendMessage(hwnd, SCI_GETTEXTRANGE, 0, (LPARAM)&tr);

	const UINT cpEdit = (UINT)SendMessage(hwnd, SCI_GETCODEPAGE, 0, 0);
	const int cchTextW = MultiByteToWideChar(cpEdit, 0, pszText, iSelCount, pszTextW, (int)(NP2HeapSize(pszTextW) / sizeof(WCHAR)));
	NP2HeapFree(pszText);

	LPWSTR pszConvW = (LPWSTR)NP2HeapAlloc(cchTextW * sizeof(WCHAR) * 3 + 2);

	WCHAR wszEOL[] = L"\r\n";
	int cchEOL = 2;
	const int iEOLMode = (int)SendMessage(hwnd, SCI_GETEOLMODE, 0, 0);
	if (iEOLMode == SC_EOL_CR) {
		cchEOL = 1;
	} else if (iEOLMode == SC_EOL_LF) {
		cchEOL = 1;
		wszEOL[0] = L'\n';
	}

#define ISDELIMITER(wc) StrChr(L",;.:-+%&\xA6|/*?!\"\'~\xB4#=", wc)
#define ISWHITE(wc) StrChr(L" \t", wc)
#define ISWORDEND(wc) (/*ISDELIMITER(wc) ||*/ StrChr(L" \t\r\n", wc))

	int cchConvW = 0;
	int iLineLength = 0;
	BOOL bModified = FALSE;
	for (int iTextW = 0; iTextW < cchTextW; iTextW++) {
		const WCHAR w = pszTextW[iTextW];

		//if (ISDELIMITER(w)) {
		//	int iNextWordLen = 0;
		//	WCHAR w2 = pszTextW[iTextW + 1];

		//	if (iLineLength + iNextWordLen + 1 > nColumn) {
		//		pszConvW[cchConvW++] = wszEOL[0];
		//		if (cchEOL > 1)
		//			pszConvW[cchConvW++] = wszEOL[1];
		//		iLineLength = 0;
		//		bModified = TRUE;
		//	}

		//	while (w2 != L'\0' && !ISWORDEND(w2)) {
		//		iNextWordLen++;
		//		w2 = pszTextW[iTextW + iNextWordLen + 1];
		//	}

		//	if (ISDELIMITER(w2) && iNextWordLen > 0) // delimiters go with the word
		//		iNextWordLen++;

		//	pszConvW[cchConvW++] = w;
		//	iLineLength++;

		//	if (iNextWordLen > 0) {
		//		if (iLineLength + iNextWordLen + 1 > nColumn) {
		//			pszConvW[cchConvW++] = wszEOL[0];
		//			if (cchEOL > 1)
		//				pszConvW[cchConvW++] = wszEOL[1];
		//			iLineLength = 0;
		//			bModified = TRUE;
		//		}
		//	}
		//}

		if (ISWHITE(w)) {
			while (pszTextW[iTextW + 1] == L' ' || pszTextW[iTextW + 1] == L'\t') {
				iTextW++;
				bModified = TRUE;
			} // Modified: left out some whitespaces

			WCHAR w2 = pszTextW[iTextW + 1];
			int iNextWordLen = 0;

			while (w2 != L'\0' && !ISWORDEND(w2)) {
				iNextWordLen++;
				w2 = pszTextW[iTextW + iNextWordLen + 1];
			}

			//if (ISDELIMITER(w2) /*&& iNextWordLen > 0*/) // delimiters go with the word
			//	iNextWordLen++;
			if (iNextWordLen > 0) {
				if (iLineLength + iNextWordLen + 1 > nColumn) {
					pszConvW[cchConvW++] = wszEOL[0];
					if (cchEOL > 1) {
						pszConvW[cchConvW++] = wszEOL[1];
					}
					iLineLength = 0;
					bModified = TRUE;
				} else {
					if (iLineLength > 0) {
						pszConvW[cchConvW++] = L' ';
						iLineLength++;
					}
				}
			}
		} else {
			pszConvW[cchConvW++] = w;
			if (w == L'\r' || w == L'\n') {
				iLineLength = 0;
			} else {
				iLineLength++;
			}
		}
	}

	NP2HeapFree(pszTextW);

	if (bModified) {
		pszText = (char *)NP2HeapAlloc(cchConvW * kMaxMultiByteCount);
		const int cchConvM = WideCharToMultiByte(cpEdit, 0, pszConvW, cchConvW, pszText, (int)NP2HeapSize(pszText), NULL, NULL);

		if (iAnchorPos > iCurPos) {
			iCurPos = iSelStart;
			iAnchorPos = iSelStart + cchConvM;
		} else {
			iAnchorPos = iSelStart;
			iCurPos = iSelStart + cchConvM;
		}

		SendMessage(hwnd, SCI_BEGINUNDOACTION, 0, 0);

		SendMessage(hwnd, SCI_SETTARGETSTART, iSelStart, 0);
		SendMessage(hwnd, SCI_SETTARGETEND, iSelEnd, 0);
		SendMessage(hwnd, SCI_REPLACETARGET, cchConvM, (LPARAM)pszText);
		//SendMessage(hwnd, SCI_CLEAR, 0, 0);
		//SendMessage(hwnd, SCI_ADDTEXT, cchConvW, (LPARAM)pszText);
		SendMessage(hwnd, SCI_SETSEL, iAnchorPos, iCurPos);

		SendMessage(hwnd, SCI_ENDUNDOACTION, 0, 0);

		NP2HeapFree(pszText);
	}

	NP2HeapFree(pszConvW);
}

//=============================================================================
//
// EditJoinLinesEx()
//
void EditJoinLinesEx(HWND hwnd) {
	int iCurPos = (int)SendMessage(hwnd, SCI_GETCURRENTPOS, 0, 0);
	int iAnchorPos = (int)SendMessage(hwnd, SCI_GETANCHOR, 0, 0);
	if (iCurPos == iAnchorPos) {
		return;
	}
	if (SC_SEL_RECTANGLE == SendMessage(hwnd, SCI_GETSELECTIONMODE, 0, 0)) {
		MsgBox(MBWARN, IDS_SELRECT);
		return;
	}

	int iSelStart = (int)SendMessage(hwnd, SCI_GETSELECTIONSTART, 0, 0);
	const int iSelEnd = (int)SendMessage(hwnd, SCI_GETSELECTIONEND, 0, 0);
	const int iLine = (int)SendMessage(hwnd, SCI_LINEFROMPOSITION, iSelStart, 0);
	iSelStart = (int)SendMessage(hwnd, SCI_POSITIONFROMLINE, iLine, 0);

	const int iSelCount = iSelEnd - iSelStart;
	char *pszText = (char *)NP2HeapAlloc(iSelCount + 1 + 2);
	char *pszJoin = (char *)NP2HeapAlloc(NP2HeapSize(pszText));

	struct Sci_TextRange tr;
	tr.chrg.cpMin = iSelStart;
	tr.chrg.cpMax = iSelEnd;
	tr.lpstrText = pszText;
	SendMessage(hwnd, SCI_GETTEXTRANGE, 0, (LPARAM)&tr);

	char szEOL[] = "\r\n";
	int cchEOL = 2;
	const int iEOLMode = (int)SendMessage(hwnd, SCI_GETEOLMODE, 0, 0);
	if (iEOLMode == SC_EOL_CR) {
		cchEOL = 1;
	} else if (iEOLMode == SC_EOL_LF) {
		cchEOL = 1;
		szEOL[0] = '\n';
	}

	int cchJoin = 0;
	BOOL bModified = FALSE;
	for (int i = 0; i < iSelCount; i++) {
		if (pszText[i] == '\r' || pszText[i] == '\n') {
			if (pszText[i] == '\r' && pszText[i + 1] == '\n') {
				i++;
			}
			if (!strchr("\r\n", pszText[i + 1]) && pszText[i + 1] != 0) {
				pszJoin[cchJoin++] = ' ';
				bModified = TRUE;
			} else {
				while (strchr("\r\n", pszText[i + 1])) {
					i++;
					bModified = TRUE;
				}
				if (pszText[i + 1] != 0) {
					pszJoin[cchJoin++] = szEOL[0];
					if (cchEOL > 1) {
						pszJoin[cchJoin++] = szEOL[1];
					}
					if (cchJoin > cchEOL) {
						pszJoin[cchJoin++] = szEOL[0];
						if (cchEOL > 1) {
							pszJoin[cchJoin++] = szEOL[1];
						}
					}
				}
			}
		} else {
			pszJoin[cchJoin++] = pszText[i];
		}
	}

	NP2HeapFree(pszText);

	if (bModified) {
		if (iAnchorPos > iCurPos) {
			iCurPos = iSelStart;
			iAnchorPos = iSelStart + cchJoin;
		} else {
			iAnchorPos = iSelStart;
			iCurPos = iSelStart + cchJoin;
		}

		SendMessage(hwnd, SCI_BEGINUNDOACTION, 0, 0);

		SendMessage(hwnd, SCI_SETTARGETSTART, iSelStart, 0);
		SendMessage(hwnd, SCI_SETTARGETEND, iSelEnd, 0);
		SendMessage(hwnd, SCI_REPLACETARGET, cchJoin, (LPARAM)pszJoin);
		SendMessage(hwnd, SCI_SETSEL, iAnchorPos, iCurPos);

		SendMessage(hwnd, SCI_ENDUNDOACTION, 0, 0);
	}

	NP2HeapFree(pszJoin);
}

//=============================================================================
//
// EditSortLines()
//
typedef struct _SORTLINE {
	WCHAR *pwszLine;
	WCHAR *pwszSortEntry;
} SORTLINE;

typedef int (__stdcall *FNSTRCMP)(LPCWSTR, LPCWSTR);

int CmpStd(const void *s1, const void *s2) {
	const int cmp = StrCmp(((SORTLINE *)s1)->pwszSortEntry, ((SORTLINE *)s2)->pwszSortEntry);
	return (cmp) ? cmp : StrCmp(((SORTLINE *)s1)->pwszLine, ((SORTLINE *)s2)->pwszLine);
}

int CmpStdRev(const void *s1, const void *s2) {
	return CmpStd(s2, s1);
}

int CmpLogical(const void *s1, const void *s2) {
	int cmp = (int)StrCmpLogicalW(((SORTLINE *)s1)->pwszSortEntry, ((SORTLINE *)s2)->pwszSortEntry);
	if (cmp == 0) {
		cmp = (int)StrCmpLogicalW(((SORTLINE *)s1)->pwszLine, ((SORTLINE *)s2)->pwszLine);
	}
	if (cmp == 0) {
		cmp = StrCmp(((SORTLINE *)s1)->pwszSortEntry, ((SORTLINE *)s2)->pwszSortEntry);
	}
	return (cmp) ? cmp : StrCmp(((SORTLINE *)s1)->pwszLine, ((SORTLINE *)s2)->pwszLine);
}

int CmpLogicalRev(const void *s1, const void *s2) {
	return CmpLogical(s2, s1);
}

void EditSortLines(HWND hwnd, int iSortFlags) {
	int iCurPos = (int)SendMessage(hwnd, SCI_GETCURRENTPOS, 0, 0);
	int iAnchorPos = (int)SendMessage(hwnd, SCI_GETANCHOR, 0, 0);
	if (iCurPos == iAnchorPos) {
		return;
	}

	int iRcCurLine = 0;
	int iRcAnchorLine = 0;
	int iRcCurCol = 0;
	int iRcAnchorCol = 0;

	int iSelStart = 0;
	int iLineStart;
	int iLineEnd;
	UINT iSortColumn;

	const BOOL bIsRectangular = SC_SEL_RECTANGLE == SendMessage(hwnd, SCI_GETSELECTIONMODE, 0, 0);
	if (bIsRectangular) {
		iRcCurLine = (int)SendMessage(hwnd, SCI_LINEFROMPOSITION, iCurPos, 0);
		iRcAnchorLine = (int)SendMessage(hwnd, SCI_LINEFROMPOSITION, iAnchorPos, 0);

		iRcCurCol = (int)SendMessage(hwnd, SCI_GETCOLUMN, iCurPos, 0);
		iRcAnchorCol = (int)SendMessage(hwnd, SCI_GETCOLUMN, iAnchorPos, 0);

		iLineStart = min_i(iRcCurLine, iRcAnchorLine);
		iLineEnd = max_i(iRcCurLine, iRcAnchorLine);
		iSortColumn = min_i(iRcCurCol, iRcAnchorCol);
	} else {
		iSelStart = (int)SendMessage(hwnd, SCI_GETSELECTIONSTART, 0, 0);
		const int iSelEnd = (int)SendMessage(hwnd, SCI_GETSELECTIONEND, 0, 0);

		const int iLine = (int)SendMessage(hwnd, SCI_LINEFROMPOSITION, iSelStart, 0);
		iSelStart = (int)SendMessage(hwnd, SCI_POSITIONFROMLINE, iLine, 0);

		iLineStart = (int)SendMessage(hwnd, SCI_LINEFROMPOSITION, iSelStart, 0);
		iLineEnd = (int)SendMessage(hwnd, SCI_LINEFROMPOSITION, iSelEnd, 0);

		if (iSelEnd <= SendMessage(hwnd, SCI_POSITIONFROMLINE, iLineEnd, 0)) {
			iLineEnd--;
		}

		iSortColumn = (UINT)SendMessage(hwnd, SCI_GETCOLUMN, SendMessage(hwnd, SCI_GETCURRENTPOS, 0, 0), 0);
	}

	const int iLineCount = iLineEnd - iLineStart + 1;
	if (iLineCount < 2) {
		return;
	}

	char mszEOL[] = "\r\n";
	const UINT cpEdit = (UINT)SendMessage(hwnd, SCI_GETCODEPAGE, 0, 0);
	const int iEOLMode = (int)SendMessage(hwnd, SCI_GETEOLMODE, 0, 0);
	if (iEOLMode == SC_EOL_CR) {
		mszEOL[1] = 0;
	} else if (iEOLMode == SC_EOL_LF) {
		mszEOL[0] = '\n';
		mszEOL[1] = 0;
	}

	SendMessage(hwnd, SCI_BEGINUNDOACTION, 0, 0);
	if (bIsRectangular) {
		EditPadWithSpaces(hwnd, !(iSortFlags & SORT_SHUFFLE), TRUE);
	}

	SORTLINE *pLines = (SORTLINE *)NP2HeapAlloc(sizeof(SORTLINE) * iLineCount);
	int cchTotal = 0;
	int ichlMax = 3;
	for (int i = 0, iLine = iLineStart; iLine <= iLineEnd; i++, iLine++) {
		const int cchm = (int)SendMessage(hwnd, SCI_GETLINE, iLine, 0);
		char *pmsz = (char *)NP2HeapAlloc(cchm + 1);
		SendMessage(hwnd, SCI_GETLINE, iLine, (LPARAM)pmsz);
		StrTrimA(pmsz, "\r\n");
		cchTotal += cchm;
		ichlMax = max_i(ichlMax, cchm);

		const int cchw = MultiByteToWideChar(cpEdit, 0, pmsz, -1, NULL, 0) - 1;
		if (cchw > 0) {
			pLines[i].pwszLine = (LPWSTR)LocalAlloc(LPTR, sizeof(WCHAR) * (cchw + 1));
			MultiByteToWideChar(cpEdit, 0, pmsz, -1, pLines[i].pwszLine, (int)(LocalSize(pLines[i].pwszLine) / sizeof(WCHAR)));
			pLines[i].pwszSortEntry = pLines[i].pwszLine;

			if (iSortFlags & SORT_COLUMN) {
				UINT col = 0;
				UINT tabs = iTabWidth;
				while (*(pLines[i].pwszSortEntry)) {
					if (*(pLines[i].pwszSortEntry) == L'\t') {
						if (col + tabs <= iSortColumn) {
							col += tabs;
							tabs = iTabWidth;
							pLines[i].pwszSortEntry = CharNext(pLines[i].pwszSortEntry);
						} else {
							break;
						}
					} else if (col < iSortColumn) {
						col++;
						if (--tabs == 0) {
							tabs = iTabWidth;
						}
						pLines[i].pwszSortEntry = CharNext(pLines[i].pwszSortEntry);
					} else {
						break;
					}
				}
			}
		} else {
			pLines[i].pwszLine = StrDup(L"");
			pLines[i].pwszSortEntry = pLines[i].pwszLine;
		}
		NP2HeapFree(pmsz);
	}

	if (iSortFlags & SORT_DESCENDING) {
		if ((iSortFlags & SORT_LOGICAL)) {
			qsort(pLines, iLineCount, sizeof(SORTLINE), CmpLogicalRev);
		} else {
			qsort(pLines, iLineCount, sizeof(SORTLINE), CmpStdRev);
		}
	} else if (iSortFlags & SORT_SHUFFLE) {
		srand((UINT)GetTickCount());
		for (int i = iLineCount - 1; i > 0; i--) {
			const int j = rand() % i;
			const SORTLINE sLine = pLines[i];
			pLines[i] = pLines[j];
			pLines[j] = sLine;
		}
	} else {
		if ((iSortFlags & SORT_LOGICAL)) {
			qsort(pLines, iLineCount, sizeof(SORTLINE), CmpLogical);
		} else {
			qsort(pLines, iLineCount, sizeof(SORTLINE), CmpStd);
		}
	}

	char *pmszResult = (char *)NP2HeapAlloc(cchTotal + 2 * iLineCount + 1);
	char *pmszBuf = (char *)NP2HeapAlloc(ichlMax + 1);
	const int cbPmszBuf = (int)NP2HeapSize(pmszBuf);
	const int cbPmszResult = (int)NP2HeapSize(pmszResult);
	FNSTRCMP pfnStrCmp = (iSortFlags & SORT_NOCASE) ? StrCmpIW : StrCmpW;

	BOOL bLastDup = FALSE;
	for (int i = 0; i < iLineCount; i++) {
		if (pLines[i].pwszLine && ((iSortFlags & SORT_SHUFFLE) || StrNotEmpty(pLines[i].pwszLine))) {
			BOOL bDropLine = FALSE;
			if (!(iSortFlags & SORT_SHUFFLE)) {
				if ((iSortFlags & SORT_MERGEDUP) || (iSortFlags & SORT_UNIQDUP) || (iSortFlags & SORT_UNIQUNIQ)) {
					if (i < iLineCount - 1) {
						if (pfnStrCmp(pLines[i].pwszLine, pLines[i + 1].pwszLine) == 0) {
							bLastDup = TRUE;
							bDropLine = ((iSortFlags & SORT_MERGEDUP) || (iSortFlags & SORT_UNIQDUP));
						} else {
							bDropLine = (!bLastDup && (iSortFlags & SORT_UNIQUNIQ)) || (bLastDup && (iSortFlags & SORT_UNIQDUP));
							bLastDup = FALSE;
						}
					} else {
						bDropLine = (!bLastDup && (iSortFlags & SORT_UNIQUNIQ)) || (bLastDup && (iSortFlags & SORT_UNIQDUP));
						bLastDup = FALSE;
					}
				}
			}

			if (!bDropLine) {
				WideCharToMultiByte(cpEdit, 0, pLines[i].pwszLine, -1, pmszBuf, cbPmszBuf, NULL, NULL);
				StrCatBuffA(pmszResult, pmszBuf, cbPmszResult);
				StrCatBuffA(pmszResult, mszEOL, cbPmszResult);
			}
		}
	}

	NP2HeapFree(pmszBuf);
	for (int i = 0; i < iLineCount; i++) {
		if (pLines[i].pwszLine) {
			LocalFree(pLines[i].pwszLine);
		}
	}
	NP2HeapFree(pLines);

	if (!bIsRectangular) {
		if (iAnchorPos > iCurPos) {
			iCurPos = iSelStart;
			iAnchorPos = iSelStart + lstrlenA(pmszResult);
		} else {
			iAnchorPos = iSelStart;
			iCurPos = iSelStart + lstrlenA(pmszResult);
		}
	}

	SendMessage(hwnd, SCI_SETTARGETSTART, SendMessage(hwnd, SCI_POSITIONFROMLINE, iLineStart, 0), 0);
	SendMessage(hwnd, SCI_SETTARGETEND, SendMessage(hwnd, SCI_POSITIONFROMLINE, iLineEnd + 1, 0), 0);
	SendMessage(hwnd, SCI_REPLACETARGET, strlen(pmszResult), (LPARAM)pmszResult);
	SendMessage(hwnd, SCI_ENDUNDOACTION, 0, 0);

	NP2HeapFree(pmszResult);

	if (!bIsRectangular) {
		SendMessage(hwnd, SCI_SETSEL, iAnchorPos, iCurPos);
	} else {
		const int iTargetStart = (int)SendMessage(hwnd, SCI_GETTARGETSTART, 0, 0);
		int iTargetEnd = (int)SendMessage(hwnd, SCI_GETTARGETEND, 0, 0);
		SendMessage(hwnd, SCI_CLEARSELECTIONS, 0, 0);
		if (iTargetStart != iTargetEnd) {
			iTargetEnd -= lstrlenA(mszEOL);
			if (iRcAnchorLine > iRcCurLine) {
				iCurPos = (int)SendMessage(hwnd, SCI_FINDCOLUMN,
										   SendMessage(hwnd, SCI_LINEFROMPOSITION, iTargetStart, 0), iRcCurCol);
				iAnchorPos = (int)SendMessage(hwnd, SCI_FINDCOLUMN,
											  SendMessage(hwnd, SCI_LINEFROMPOSITION, iTargetEnd, 0), iRcAnchorCol);
			} else {
				iCurPos = (int)SendMessage(hwnd, SCI_FINDCOLUMN,
										   SendMessage(hwnd, SCI_LINEFROMPOSITION, iTargetEnd, 0), iRcCurCol);
				iAnchorPos = (int)SendMessage(hwnd, SCI_FINDCOLUMN,
											  SendMessage(hwnd, SCI_LINEFROMPOSITION, iTargetStart, 0), iRcAnchorCol);
			}
			if (iCurPos != iAnchorPos) {
				SendMessage(hwnd, SCI_SETRECTANGULARSELECTIONCARET, iCurPos, 0);
				SendMessage(hwnd, SCI_SETRECTANGULARSELECTIONANCHOR, iAnchorPos, 0);
			} else {
				SendMessage(hwnd, SCI_SETSEL, iTargetStart, iTargetStart);
			}
		} else {
			SendMessage(hwnd, SCI_SETSEL, iTargetStart, iTargetStart);
		}
	}
}

//=============================================================================
//
// EditJumpTo()
//
void EditJumpTo(HWND hwnd, int iNewLine, int iNewCol) {
	const int iMaxLine = (int)SendMessage(hwnd, SCI_GETLINECOUNT, 0, 0);

	// Jumpt to end with line set to -1
	if (iNewLine == -1) {
		SendMessage(hwnd, SCI_DOCUMENTEND, 0, 0);
		return;
	}

	// Line maximum is iMaxLine
	iNewLine = min_i(iNewLine, iMaxLine);

	// Column minimum is 1
	iNewCol = max_i(iNewCol, 1);

	if (iNewLine > 0 && iNewLine <= iMaxLine && iNewCol > 0) {
		int iNewPos	 = (int)SendMessage(hwnd, SCI_POSITIONFROMLINE, iNewLine - 1, 0);
		const int iLineEndPos = (int)SendMessage(hwnd, SCI_GETLINEENDPOSITION, iNewLine - 1, 0);

		while (iNewCol - 1 > SendMessage(hwnd, SCI_GETCOLUMN, iNewPos, 0)) {
			if (iNewPos >= iLineEndPos) {
				break;
			}

			iNewPos = (int)SendMessage(hwnd, SCI_POSITIONAFTER, iNewPos, 0);
		}

		iNewPos = min_i(iNewPos, iLineEndPos);
		EditSelectEx(hwnd, -1, iNewPos); // SCI_GOTOPOS(pos) is equivalent to SCI_SETSEL(-1, pos)
		SendMessage(hwnd, SCI_CHOOSECARETX, 0, 0);
	}
}

//=============================================================================
//
// EditSelectEx()
//
void EditSelectEx(HWND hwnd, int iAnchorPos, int iCurrentPos) {
	const int iNewLine = (int)SendMessage(hwnd, SCI_LINEFROMPOSITION, iCurrentPos, 0);
	const int iAnchorLine = (int)SendMessage(hwnd, SCI_LINEFROMPOSITION, iAnchorPos, 0);

	// Ensure that the first and last lines of a selection are always unfolded
	// This needs to be done *before* the SCI_SETSEL message
	SciCall_EnsureVisible(iAnchorLine);
	if (iAnchorLine != iNewLine) {
		SciCall_EnsureVisible(iNewLine);
	}

	SendMessage(hwnd, SCI_SETXCARETPOLICY, CARET_SLOP | CARET_STRICT | CARET_EVEN, 50);
	SendMessage(hwnd, SCI_SETYCARETPOLICY, CARET_SLOP | CARET_STRICT | CARET_EVEN, 5);
	SendMessage(hwnd, SCI_SETSEL, iAnchorPos, iCurrentPos);
	SendMessage(hwnd, SCI_SETXCARETPOLICY, CARET_SLOP | CARET_EVEN, 50);
	SendMessage(hwnd, SCI_SETYCARETPOLICY, CARET_EVEN, 0);
}

//=============================================================================
//
// EditFixPositions()
//
void EditFixPositions(HWND hwnd) {
	const int iMaxPos = (int)SendMessage(hwnd, SCI_GETLENGTH, 0, 0);
	int iCurrentPos = (int)SendMessage(hwnd, SCI_GETCURRENTPOS, 0, 0);
	const int iAnchorPos = (int)SendMessage(hwnd, SCI_GETANCHOR, 0, 0);

	if (iCurrentPos > 0 && iCurrentPos < iMaxPos) {
		const int iNewPos = (int)SendMessage(hwnd, SCI_POSITIONAFTER, SendMessage(hwnd, SCI_POSITIONBEFORE, iCurrentPos, 0), 0);
		if (iNewPos != iCurrentPos) {
			SendMessage(hwnd, SCI_SETCURRENTPOS, iNewPos, 0);
			iCurrentPos = iNewPos;
		}
	}

	if (iAnchorPos != iCurrentPos && iAnchorPos > 0 && iAnchorPos < iMaxPos) {
		const int iNewPos = (int)SendMessage(hwnd, SCI_POSITIONAFTER, SendMessage(hwnd, SCI_POSITIONBEFORE, iAnchorPos, 0), 0);
		if (iNewPos != iAnchorPos) {
			SendMessage(hwnd, SCI_SETANCHOR, iNewPos, 0);
		}
	}
}

//=============================================================================
//
// EditEnsureSelectionVisible()
//
void EditEnsureSelectionVisible(HWND hwnd) {
	const int iAnchorPos = (int)SendMessage(hwnd, SCI_GETANCHOR, 0, 0);
	const int iCurrentPos = (int)SendMessage(hwnd, SCI_GETCURRENTPOS, 0, 0);

	SendMessage(hwnd, SCI_ENSUREVISIBLE, SendMessage(hwnd, SCI_LINEFROMPOSITION, iAnchorPos, 0), 0);
	if (iAnchorPos != iCurrentPos) {
		SendMessage(hwnd, SCI_ENSUREVISIBLE, SendMessage(hwnd, SCI_LINEFROMPOSITION, iCurrentPos, 0), 0);
	}
	EditSelectEx(hwnd, iAnchorPos, iCurrentPos);
}

void EditEnsureConsistentLineEndings(HWND hwnd) {
	SendMessage(hwnd, SCI_CONVERTEOLS, SendMessage(hwnd, SCI_GETEOLMODE, 0, 0), 0);
	EditFixPositions(hwnd);
}

//=============================================================================
//
// EditGetExcerpt()
//
void EditGetExcerpt(HWND hwnd, LPWSTR lpszExcerpt, DWORD cchExcerpt) {
	const int iCurPos = (int)SendMessage(hwnd, SCI_GETCURRENTPOS, 0, 0);
	const int iAnchorPos = (int)SendMessage(hwnd, SCI_GETANCHOR, 0, 0);

	if (iCurPos == iAnchorPos || SC_SEL_RECTANGLE == SendMessage(hwnd, SCI_GETSELECTIONMODE, 0, 0)) {
		lstrcpy(lpszExcerpt, L"");
		return;
	}

	WCHAR tch[256] = L"";
	DWORD cch = 0;
	struct Sci_TextRange tr;
	/*if (iCurPos != iAnchorPos && SC_SEL_RECTANGLE != SendMessage(hwnd, SCI_GETSELECTIONMODE, 0, 0)) {*/
	tr.chrg.cpMin = (long)SendMessage(hwnd, SCI_GETSELECTIONSTART, 0, 0);
	tr.chrg.cpMax = min_l((long)SendMessage(hwnd, SCI_GETSELECTIONEND, 0, 0), (long)(tr.chrg.cpMin + COUNTOF(tch)));
	/*}
	else {
		const int iLine = (int)SendMessage(hwnd, SCI_LINEFROMPOSITION, iCurPos, 0);
		tr.chrg.cpMin = (long)SendMessage(hwnd, SCI_POSITIONFROMLINE, iLine, 0);
		tr.chrg.cpMax = min_l((long)SendMessage(hwnd, SCI_GETLINEENDPOSITION, iLine, 0), (long)(tr.chrg.cpMin + COUNTOF(tch)));
	}*/

	tr.chrg.cpMax = min_l((long)SendMessage(hwnd, SCI_GETLENGTH, 0, 0), tr.chrg.cpMax);

	char *pszText = (char *)NP2HeapAlloc((tr.chrg.cpMax - tr.chrg.cpMin) + 2);
	LPWSTR pszTextW = (LPWSTR)NP2HeapAlloc(((tr.chrg.cpMax - tr.chrg.cpMin) + 1) * sizeof(WCHAR));

	tr.lpstrText = pszText;
	SendMessage(hwnd, SCI_GETTEXTRANGE, 0, (LPARAM)&tr);
	const UINT cpEdit = (UINT)SendMessage(hwnd, SCI_GETCODEPAGE, 0, 0);
	MultiByteToWideChar(cpEdit, 0, pszText, tr.chrg.cpMax - tr.chrg.cpMin, pszTextW, (int)(NP2HeapSize(pszTextW) / sizeof(WCHAR)));

	for (WCHAR *p = pszTextW; *p && cch < COUNTOF(tch) - 1; p++) {
		if (*p == L'\r' || *p == L'\n' || *p == L'\t' || *p == L' ') {
			tch[cch++] = L' ';
			while (*(p + 1) == L'\r' || *(p + 1) == L'\n' || *(p + 1) == L'\t' || *(p + 1) == L' ') {
				p++;
			}
		} else {
			tch[cch++] = *p;
		}
	}
	tch[cch++] = L'\0';
	StrTrim(tch, L" ");

	if (cch == 1) {
		lstrcpy(tch, L" ... ");
	}

	if (cch > cchExcerpt) {
		tch[cchExcerpt - 2] = L'.';
		tch[cchExcerpt - 3] = L'.';
		tch[cchExcerpt - 4] = L'.';
	}
	lstrcpyn(lpszExcerpt, tch, cchExcerpt);

	NP2HeapFree(pszText);
	NP2HeapFree(pszTextW);
}

void EditSelectWord(HWND hwnd) {
	int iSel = (int)SendMessage(hwnd, SCI_GETSELECTIONEND, 0, 0) -
			   (int)SendMessage(hwnd, SCI_GETSELECTIONSTART, 0, 0);

	const int iPos = (int)SendMessage(hwnd, SCI_GETCURRENTPOS, 0, 0);

	if (iSel == 0) {
		int iWordStart = (int)SendMessage(hwnd, SCI_WORDSTARTPOSITION, iPos, TRUE);
		int iWordEnd = (int)SendMessage(hwnd, SCI_WORDENDPOSITION, iPos, TRUE);

		if (iWordStart == iWordEnd) {// we are in whitespace salad...
			iWordStart = (int)SendMessage(hwnd, SCI_WORDENDPOSITION, iPos, FALSE);
			iWordEnd = (int)SendMessage(hwnd, SCI_WORDENDPOSITION, iWordStart, TRUE);
			if (iWordStart != iWordEnd) {
				//if (SCLEX_HTML == SendMessage(hwnd, SCI_GETLEXER, 0, 0) &&
				//		SCE_HPHP_VARIABLE == SendMessage(hwnd, SCI_GETSTYLEAT, iWordStart, 0) &&
				//		'$' == (int)SendMessage(hwnd, SCI_GETCHARAT, iWordStart - 1, 0))
				//	iWordStart--;
				SendMessage(hwnd, SCI_SETSEL, iWordStart, iWordEnd);
			}
		} else {
			//if (SCLEX_HTML == SendMessage(hwnd, SCI_GETLEXER, 0, 0) &&
			//		SCE_HPHP_VARIABLE == SendMessage(hwnd, SCI_GETSTYLEAT, iWordStart, 0) &&
			//		'$' == (int)SendMessage(hwnd, SCI_GETCHARAT, iWordStart - 1, 0))
			//	iWordStart--;
			SendMessage(hwnd, SCI_SETSEL, iWordStart, iWordEnd);
		}

		iSel =	(int)SendMessage(hwnd, SCI_GETSELECTIONEND, 0, 0) -
				(int)SendMessage(hwnd, SCI_GETSELECTIONSTART, 0, 0);

		if (iSel == 0) {
			const int iLine = (int)SendMessage(hwnd, SCI_LINEFROMPOSITION, iPos, 0);
			const int iLineStart = (int)SendMessage(hwnd, SCI_GETLINEINDENTPOSITION, iLine, 0);
			const int iLineEnd = (int)SendMessage(hwnd, SCI_GETLINEENDPOSITION, iLine, 0);
			SendMessage(hwnd, SCI_SETSEL, iLineStart, iLineEnd);
		}
	} else {
		const int iLine = (int)SendMessage(hwnd, SCI_LINEFROMPOSITION, iPos, 0);
		const int iLineStart = (int)SendMessage(hwnd, SCI_GETLINEINDENTPOSITION, iLine, 0);
		const int iLineEnd = (int)SendMessage(hwnd, SCI_GETLINEENDPOSITION, iLine, 0);
		SendMessage(hwnd, SCI_SETSEL, iLineStart, iLineEnd);
	}
}

void EditSelectLine(HWND hwnd) {
	int iSelStart = (int)SendMessage(hwnd, SCI_GETSELECTIONSTART, 0, 0);
	int iSelEnd = (int)SendMessage(hwnd, SCI_GETSELECTIONEND, 0, 0);
	const int iLineStart = (int)SendMessage(hwnd, SCI_LINEFROMPOSITION, iSelStart, 0);
	const int iLineEnd = (int)SendMessage(hwnd, SCI_LINEFROMPOSITION, iSelEnd, 0);
	iSelStart = (int)SendMessage(hwnd, SCI_POSITIONFROMLINE, iLineStart, 0);
	iSelEnd = (int)SendMessage(hwnd, SCI_POSITIONFROMLINE, iLineEnd + 1, 0);
	SendMessage(hwnd, SCI_SETSEL, iSelStart, iSelEnd);
	SendMessage(hwnd, SCI_CHOOSECARETX, 0, 0);
}

extern BOOL bFindReplaceTransparentMode;
extern int iFindReplaceOpacityLevel;
//=============================================================================
//
// EditFindReplaceDlgProc()
//
static INT_PTR CALLBACK EditFindReplaceDlgProc(HWND hwnd, UINT umsg, WPARAM wParam, LPARAM lParam) {
	static BOOL bSwitchedFindReplace = FALSE;
	static int xFindReplaceDlgSave;
	static int yFindReplaceDlgSave;
	static EDITFINDREPLACE efrSave;

	WCHAR tch[NP2_FIND_REPLACE_LIMIT + 32];

	switch (umsg) {
	case WM_INITDIALOG: {
		static BOOL bFirstTime = TRUE;

		SetWindowLongPtr(hwnd, DWLP_USER, lParam);
		ResizeDlg_InitX(hwnd, cxFindReplaceDlg, IDC_RESIZEGRIP2);

		LPEDITFINDREPLACE lpefr = (LPEDITFINDREPLACE)lParam;
		// Get the current code page for Unicode conversion
		const UINT cpEdit = (UINT)SendMessage(lpefr->hwnd, SCI_GETCODEPAGE, 0, 0);

		// Load MRUs
		for (int i = 0; i < MRU_GetCount(mruFind); i++) {
			MRU_Enum(mruFind, i, tch, COUNTOF(tch));
			SendDlgItemMessage(hwnd, IDC_FINDTEXT, CB_ADDSTRING, 0, (LPARAM)tch);
		}

		for (int i = 0; i < MRU_GetCount(mruReplace); i++) {
			MRU_Enum(mruReplace, i, tch, COUNTOF(tch));
			SendDlgItemMessage(hwnd, IDC_REPLACETEXT, CB_ADDSTRING, 0, (LPARAM)tch);
		}

		if (!bSwitchedFindReplace) {
			int cchSelection = (int)SendMessage(lpefr->hwnd, SCI_GETSELECTIONEND, 0, 0)
							   - (int)SendMessage(lpefr->hwnd, SCI_GETSELECTIONSTART, 0, 0);
			if (cchSelection <= NP2_FIND_REPLACE_LIMIT) {
				cchSelection = (int)SendMessage(lpefr->hwnd, SCI_GETSELTEXT, 0, 0);
				char *lpszSelection = (char *)NP2HeapAlloc(cchSelection);
				SendMessage(lpefr->hwnd, SCI_GETSELTEXT, 0, (LPARAM)lpszSelection);

				// First time you bring up find/replace dialog, copy content from clipboard to find box (but only if nothing is selected in the editor)
				if (StrIsEmptyA(lpszSelection) && bFirstTime) {
					char *pClip = EditGetClipboardText(hwndEdit);
					const int len = lstrlenA(pClip);
					if (len > 0 && len <= NP2_FIND_REPLACE_LIMIT) {
						NP2HeapFree(lpszSelection);
						lpszSelection = (char *)NP2HeapAlloc(len + 2);
						lstrcpynA(lpszSelection, pClip, NP2_FIND_REPLACE_LIMIT);
					}
					LocalFree(pClip);
				}
				bFirstTime = FALSE;

				// Check lpszSelection and truncate bad chars
				//char *lpsz = strpbrk(lpszSelection, "\r\n\t");
				//if (lpsz) {
				//	*lpsz = '\0';
				//}
				char *lpszEscSel = (char *)NP2HeapAlloc((2 * NP2_FIND_REPLACE_LIMIT));
				lpefr->bTransformBS = AddBackslash(lpszEscSel, lpszSelection);

				SetDlgItemTextA2W(cpEdit, hwnd, IDC_FINDTEXT, lpszEscSel);
				NP2HeapFree(lpszSelection);
				NP2HeapFree(lpszEscSel);
			}
		}

		SendDlgItemMessage(hwnd, IDC_FINDTEXT, CB_LIMITTEXT, NP2_FIND_REPLACE_LIMIT, 0);
		SendDlgItemMessage(hwnd, IDC_FINDTEXT, CB_SETEXTENDEDUI, TRUE, 0);

		if (!GetWindowTextLength(GetDlgItem(hwnd, IDC_FINDTEXT))) {
			SetDlgItemTextA2W(CP_UTF8, hwnd, IDC_FINDTEXT, lpefr->szFindUTF8);
		}

		if (GetDlgItem(hwnd, IDC_REPLACETEXT)) {
			SendDlgItemMessage(hwnd, IDC_REPLACETEXT, CB_LIMITTEXT, NP2_FIND_REPLACE_LIMIT, 0);
			SendDlgItemMessage(hwnd, IDC_REPLACETEXT, CB_SETEXTENDEDUI, TRUE, 0);
			SetDlgItemTextA2W(CP_UTF8, hwnd, IDC_REPLACETEXT, lpefr->szReplaceUTF8);
		}

		if (lpefr->fuFlags & SCFIND_MATCHCASE) {
			CheckDlgButton(hwnd, IDC_FINDCASE, BST_CHECKED);
		}

		if (lpefr->fuFlags & SCFIND_WHOLEWORD) {
			CheckDlgButton(hwnd, IDC_FINDWORD, BST_CHECKED);
		}

		if (lpefr->fuFlags & SCFIND_WORDSTART) {
			CheckDlgButton(hwnd, IDC_FINDSTART, BST_CHECKED);
		}

		if (lpefr->fuFlags & SCFIND_REGEXP) {
			CheckDlgButton(hwnd, IDC_FINDREGEXP, BST_CHECKED);
		}

		if (lpefr->bTransformBS) {
			CheckDlgButton(hwnd, IDC_FINDTRANSFORMBS, BST_CHECKED);
		}

		if (lpefr->bWildcardSearch) {
			CheckDlgButton(hwnd, IDC_WILDCARDSEARCH, BST_CHECKED);
			CheckDlgButton(hwnd, IDC_FINDREGEXP, BST_UNCHECKED);
		}

		if (lpefr->bNoFindWrap) {
			CheckDlgButton(hwnd, IDC_NOWRAP, BST_CHECKED);
		}

		if (GetDlgItem(hwnd, IDC_REPLACE)) {
			if (bSwitchedFindReplace) {
				if (lpefr->bFindClose) {
					CheckDlgButton(hwnd, IDC_FINDCLOSE, BST_CHECKED);
				}
			} else {
				if (lpefr->bReplaceClose) {
					CheckDlgButton(hwnd, IDC_FINDCLOSE, BST_CHECKED);
				}
			}
		} else {
			if (bSwitchedFindReplace) {
				if (lpefr->bReplaceClose) {
					CheckDlgButton(hwnd, IDC_FINDCLOSE, BST_CHECKED);
				}
			} else {
				if (lpefr->bFindClose) {
					CheckDlgButton(hwnd, IDC_FINDCLOSE, BST_CHECKED);
				}
			}
		}

		if (!bSwitchedFindReplace) {
			if (xFindReplaceDlg == 0 || yFindReplaceDlg == 0) {
				CenterDlgInParent(hwnd);
			} else {
				SetDlgPos(hwnd, xFindReplaceDlg, yFindReplaceDlg);
			}
		} else {
			SetDlgPos(hwnd, xFindReplaceDlgSave, yFindReplaceDlgSave);
			bSwitchedFindReplace = FALSE;
			CopyMemory(lpefr, &efrSave, sizeof(EDITFINDREPLACE));
		}

		if (bFindReplaceTransparentMode) {
			CheckDlgButton(hwnd, IDC_TRANSPARENT, BST_CHECKED);
		}
	}
	return TRUE;

	case WM_DESTROY:
		ResizeDlg_Destroy(hwnd, &cxFindReplaceDlg, NULL);
		return FALSE;

	case WM_SIZE: {
		int dx;

		const BOOL isReplace = GetDlgItem(hwnd, IDC_REPLACETEXT) != NULL;
		ResizeDlg_Size(hwnd, lParam, &dx, NULL);
		HDWP hdwp = BeginDeferWindowPos(isReplace ? 13 : 8);
		hdwp = DeferCtlPos(hdwp, hwnd, IDC_RESIZEGRIP2, dx, 0, SWP_NOSIZE);
		hdwp = DeferCtlPos(hdwp, hwnd, IDOK, dx, 0, SWP_NOSIZE);
		hdwp = DeferCtlPos(hdwp, hwnd, IDCANCEL, dx, 0, SWP_NOSIZE);
		hdwp = DeferCtlPos(hdwp, hwnd, IDC_FINDTEXT, dx, 0, SWP_NOMOVE);
		hdwp = DeferCtlPos(hdwp, hwnd, IDC_CLEAR_FIND, dx, 0, SWP_NOSIZE);
		hdwp = DeferCtlPos(hdwp, hwnd, IDC_FINDPREV, dx, 0, SWP_NOSIZE);
		hdwp = DeferCtlPos(hdwp, hwnd, IDC_SAVEPOSITION, dx, 0, SWP_NOSIZE);
		hdwp = DeferCtlPos(hdwp, hwnd, IDC_RESETPOSITION, dx, 0, SWP_NOSIZE);
		if (isReplace) {
			hdwp = DeferCtlPos(hdwp, hwnd, IDC_REPLACETEXT, dx, 0, SWP_NOMOVE);
			hdwp = DeferCtlPos(hdwp, hwnd, IDC_CLEAR_REPLACE, dx, 0, SWP_NOSIZE);
			hdwp = DeferCtlPos(hdwp, hwnd, IDC_REPLACE, dx, 0, SWP_NOSIZE);
			hdwp = DeferCtlPos(hdwp, hwnd, IDC_REPLACEALL, dx, 0, SWP_NOSIZE);
			hdwp = DeferCtlPos(hdwp, hwnd, IDC_REPLACEINSEL, dx, 0, SWP_NOSIZE);
		}
		EndDeferWindowPos(hdwp);
	}
	return TRUE;

	case WM_GETMINMAXINFO:
		ResizeDlg_GetMinMaxInfo(hwnd, lParam);
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_FINDTEXT:
		case IDC_REPLACETEXT: {
			const BOOL bEnable = (GetWindowTextLength(GetDlgItem(hwnd, IDC_FINDTEXT)) ||
							CB_ERR != SendDlgItemMessage(hwnd, IDC_FINDTEXT, CB_GETCURSEL, 0, 0));

			EnableWindow(GetDlgItem(hwnd, IDOK), bEnable);
			EnableWindow(GetDlgItem(hwnd, IDC_FINDPREV), bEnable);
			EnableWindow(GetDlgItem(hwnd, IDC_REPLACE), bEnable);
			EnableWindow(GetDlgItem(hwnd, IDC_REPLACEALL), bEnable);
			EnableWindow(GetDlgItem(hwnd, IDC_REPLACEINSEL), bEnable);

			if (HIWORD(wParam) == CBN_CLOSEUP) {
				LONG lSelEnd;
				SendDlgItemMessage(hwnd, LOWORD(wParam), CB_GETEDITSEL, 0, (LPARAM)&lSelEnd);
				SendDlgItemMessage(hwnd, LOWORD(wParam), CB_SETEDITSEL, 0, MAKELPARAM(lSelEnd, lSelEnd));
			}
		}
		break;

		case IDC_FINDREGEXP:
			if (IsButtonChecked(hwnd, IDC_FINDREGEXP)) {
				CheckDlgButton(hwnd, IDC_FINDTRANSFORMBS, BST_UNCHECKED);
				// Can not use wildcard search together with regexp
				CheckDlgButton(hwnd, IDC_WILDCARDSEARCH, BST_UNCHECKED);
			}
			break;

		case IDC_FINDTRANSFORMBS:
			if (IsButtonChecked(hwnd, IDC_FINDTRANSFORMBS)) {
				CheckDlgButton(hwnd, IDC_FINDREGEXP, BST_UNCHECKED);
			}
			break;

		case IDC_WILDCARDSEARCH:
			CheckDlgButton(hwnd, IDC_FINDREGEXP, BST_UNCHECKED);
			//if (IsButtonChecked(hwnd, IDC_FINDWILDCARDS))
			//	CheckDlgButton(hwnd, IDC_FINDREGEXP, BST_UNCHECKED);
			break;

		case IDC_TRANSPARENT:
			bFindReplaceTransparentMode = IsButtonChecked(hwnd, IDC_TRANSPARENT);
			break;

		case IDOK:
		case IDC_FINDPREV:
		case IDC_REPLACE:
		case IDC_REPLACEALL:
		case IDC_REPLACEINSEL:
		case IDACC_SELTONEXT:
		case IDACC_SELTOPREV:
		case IDMSG_SWITCHTOFIND:
		case IDMSG_SWITCHTOREPLACE: {
			LPEDITFINDREPLACE lpefr = (LPEDITFINDREPLACE)GetWindowLongPtr(hwnd, DWLP_USER);
			const BOOL bIsFindDlg = (GetDlgItem(hwnd, IDC_REPLACE) == NULL);

			if ((bIsFindDlg && LOWORD(wParam) == IDMSG_SWITCHTOREPLACE) ||
					(!bIsFindDlg && LOWORD(wParam) == IDMSG_SWITCHTOFIND)) {
				GetDlgPos(hwnd, &xFindReplaceDlgSave, &yFindReplaceDlgSave);
				bSwitchedFindReplace = TRUE;
				CopyMemory(&efrSave, lpefr, sizeof(EDITFINDREPLACE));
			}

			// Get current code page for Unicode conversion
			const UINT cpEdit = (UINT)SendMessage(lpefr->hwnd, SCI_GETCODEPAGE, 0, 0);
			cpLastFind = cpEdit;

			if (!bSwitchedFindReplace &&
					!GetDlgItemTextA2W(cpEdit, hwnd, IDC_FINDTEXT, lpefr->szFind, COUNTOF(lpefr->szFind))) {
				EnableWindow(GetDlgItem(hwnd, IDOK), FALSE);
				EnableWindow(GetDlgItem(hwnd, IDC_FINDPREV), FALSE);
				EnableWindow(GetDlgItem(hwnd, IDC_REPLACE), FALSE);
				EnableWindow(GetDlgItem(hwnd, IDC_REPLACEALL), FALSE);
				EnableWindow(GetDlgItem(hwnd, IDC_REPLACEINSEL), FALSE);
				return TRUE;
			}

			if (GetDlgItem(hwnd, IDC_REPLACETEXT)) {
				GetDlgItemTextA2W(cpEdit, hwnd, IDC_REPLACETEXT, lpefr->szReplace, COUNTOF(lpefr->szReplace));
			}

			lpefr->bWildcardSearch = IsButtonChecked(hwnd, IDC_WILDCARDSEARCH);
			lpefr->fuFlags = 0;

			if (IsButtonChecked(hwnd, IDC_FINDCASE)) {
				lpefr->fuFlags |= SCFIND_MATCHCASE;
			}

			if (IsButtonChecked(hwnd, IDC_FINDWORD)) {
				lpefr->fuFlags |= SCFIND_WHOLEWORD;
			}

			if (IsButtonChecked(hwnd, IDC_FINDSTART)) {
				lpefr->fuFlags |= SCFIND_WORDSTART;
			}

			if (IsButtonChecked(hwnd, IDC_FINDREGEXP)) {
				lpefr->fuFlags |= SCFIND_REGEXP | SCFIND_POSIX;
			}

			lpefr->bTransformBS = IsButtonChecked(hwnd, IDC_FINDTRANSFORMBS);
			lpefr->bNoFindWrap = IsButtonChecked(hwnd, IDC_NOWRAP);

			if (bIsFindDlg) {
				lpefr->bFindClose = IsButtonChecked(hwnd, IDC_FINDCLOSE);
			} else {
				lpefr->bReplaceClose = IsButtonChecked(hwnd, IDC_FINDCLOSE);
			}

			if (!bSwitchedFindReplace) {
				// Save MRUs
				if (StrNotEmptyA(lpefr->szFind)) {
					if (GetDlgItemTextA2W(CP_UTF8, hwnd, IDC_FINDTEXT, lpefr->szFindUTF8, COUNTOF(lpefr->szFindUTF8))) {
						GetDlgItemText(hwnd, IDC_FINDTEXT, tch, COUNTOF(tch));
						MRU_Add(mruFind, tch);
					}
				}
				if (StrNotEmptyA(lpefr->szReplace)) {
					if (GetDlgItemTextA2W(CP_UTF8, hwnd, IDC_REPLACETEXT, lpefr->szReplaceUTF8, COUNTOF(lpefr->szReplaceUTF8))) {
						GetDlgItemText(hwnd, IDC_REPLACETEXT, tch, COUNTOF(tch));
						MRU_Add(mruReplace, tch);
					}
				} else {
					strcpy(lpefr->szReplaceUTF8, "");
				}
			} else {
				GetDlgItemTextA2W(CP_UTF8, hwnd, IDC_FINDTEXT, lpefr->szFindUTF8, COUNTOF(lpefr->szFindUTF8));
				if (!GetDlgItemTextA2W(CP_UTF8, hwnd, IDC_REPLACETEXT, lpefr->szReplaceUTF8, COUNTOF(lpefr->szReplaceUTF8))) {
					strcpy(lpefr->szReplaceUTF8, "");
				}
			}

			BOOL bCloseDlg;
			if (bIsFindDlg) {
				bCloseDlg = lpefr->bFindClose;
			} else {
				if (LOWORD(wParam) == IDOK) {
					bCloseDlg = FALSE;
				} else {
					bCloseDlg = lpefr->bReplaceClose;
				}
			}

			// Reload MRUs
			SendDlgItemMessage(hwnd, IDC_FINDTEXT, CB_RESETCONTENT, 0, 0);
			SendDlgItemMessage(hwnd, IDC_REPLACETEXT, CB_RESETCONTENT, 0, 0);

			for (int i = 0; i < MRU_GetCount(mruFind); i++) {
				MRU_Enum(mruFind, i, tch, COUNTOF(tch));
				SendDlgItemMessage(hwnd, IDC_FINDTEXT, CB_ADDSTRING, 0, (LPARAM)tch);
			}

			for (int i = 0; i < MRU_GetCount(mruReplace); i++) {
				MRU_Enum(mruReplace, i, tch, COUNTOF(tch));
				SendDlgItemMessage(hwnd, IDC_REPLACETEXT, CB_ADDSTRING, 0, (LPARAM)tch);
			}

			SetDlgItemTextA2W(CP_UTF8, hwnd, IDC_FINDTEXT, lpefr->szFindUTF8);
			SetDlgItemTextA2W(CP_UTF8, hwnd, IDC_REPLACETEXT, lpefr->szReplaceUTF8);

			if (!bSwitchedFindReplace) {
				SendMessage(hwnd, WM_NEXTDLGCTL, (WPARAM)(GetFocus()), 1);
			}

			if (bCloseDlg) {
				//EndDialog(hwnd, LOWORD(wParam));
				DestroyWindow(hwnd);
				hDlgFindReplace = NULL;
			}

			switch (LOWORD(wParam)) {
			case IDOK: // find next
			case IDACC_SELTONEXT:
				if (!bIsFindDlg) {
					bReplaceInitialized = TRUE;
				}
				EditFindNext(lpefr->hwnd, lpefr, LOWORD(wParam) == IDACC_SELTONEXT || KeyboardIsKeyDown(VK_SHIFT));
				break;

			case IDC_FINDPREV: // find previous
			case IDACC_SELTOPREV:
				if (!bIsFindDlg) {
					bReplaceInitialized = TRUE;
				}
				EditFindPrev(lpefr->hwnd, lpefr, LOWORD(wParam) == IDACC_SELTOPREV || KeyboardIsKeyDown(VK_SHIFT));
				break;

			case IDC_REPLACE:
				bReplaceInitialized = TRUE;
				EditReplace(lpefr->hwnd, lpefr);
				break;

			case IDC_REPLACEALL:
				bReplaceInitialized = TRUE;
				EditReplaceAll(lpefr->hwnd, lpefr, TRUE);
				break;

			case IDC_REPLACEINSEL:
				bReplaceInitialized = TRUE;
				EditReplaceAllInSelection(lpefr->hwnd, lpefr, TRUE);
				break;
			}

			// Wildcard search will enable regexp, so I turn it off again otherwise it will be on in the gui
			if (lpefr->bWildcardSearch	&&	(lpefr->fuFlags & SCFIND_REGEXP)) {
				lpefr->fuFlags ^= SCFIND_REGEXP;
			}
		}
		break;

		case IDCANCEL:
			//EndDialog(hwnd, IDCANCEL);
			DestroyWindow(hwnd);
			break;

		case IDACC_FIND:
			PostWMCommand(hwndMain, IDM_EDIT_FIND);
			break;

		case IDACC_REPLACE:
			PostWMCommand(hwndMain, IDM_EDIT_REPLACE);
			break;

		case IDACC_SAVEPOS:
			GetDlgPos(hwnd, &xFindReplaceDlg, &yFindReplaceDlg);
			break;

		case IDACC_RESETPOS:
			CenterDlgInParent(hwnd);
			xFindReplaceDlg = yFindReplaceDlg = 0;
			break;

		case IDACC_FINDNEXT:
			PostWMCommand(hwnd, IDOK);
			break;

		case IDACC_FINDPREV:
			PostWMCommand(hwnd, IDC_FINDPREV);
			break;

		case IDACC_REPLACENEXT:
			if (GetDlgItem(hwnd, IDC_REPLACE) != NULL) {
				PostWMCommand(hwnd, IDC_REPLACE);
			}
			break;

		case IDACC_SAVEFIND: {
			SendWMCommand(hwndMain, IDM_EDIT_SAVEFIND);
			LPCEDITFINDREPLACE lpefr = (LPCEDITFINDREPLACE)GetWindowLongPtr(hwnd, DWLP_USER);
			SetDlgItemTextA2W(CP_UTF8, hwnd, IDC_FINDTEXT, lpefr->szFindUTF8);
			CheckDlgButton(hwnd, IDC_FINDREGEXP, BST_UNCHECKED);
			CheckDlgButton(hwnd, IDC_FINDTRANSFORMBS, BST_UNCHECKED);
			PostMessage(hwnd, WM_NEXTDLGCTL, (WPARAM)(GetDlgItem(hwnd, IDC_FINDTEXT)), 1);
		}
		break;
		}
		return TRUE;

	case WM_NOTIFY: {
		LPNMHDR pnmhdr = (LPNMHDR)lParam;
		switch (pnmhdr->code) {
		case NM_CLICK:
		case NM_RETURN:
			switch (pnmhdr->idFrom) {
			case IDC_TOGGLEFINDREPLACE:
				if (GetDlgItem(hwnd, IDC_REPLACE)) {
					PostWMCommand(hwndMain, IDM_EDIT_FIND);
				} else {
					PostWMCommand(hwndMain, IDM_EDIT_REPLACE);
				}
				break;

			// Display help messages in the find/replace windows
			case IDC_BACKSLASHHELP:
				MsgBox(MBINFO, IDS_BACKSLASHHELP);
				break;

			case IDC_REGEXPHELP:
				MsgBox(MBINFO, IDS_REGEXPHELP);
				break;

			case IDC_WILDCARDHELP:
				MsgBox(MBINFO, IDS_WILDCARDHELP);
				break;

			case IDC_CLEAR_FIND:
				GetDlgItemText(hwnd, IDC_FINDTEXT, tch, COUNTOF(tch));
				SendDlgItemMessage(hwnd, IDC_FINDTEXT, CB_RESETCONTENT, 0, 0);
				MRU_Empty(mruFind);
				MRU_Save(mruFind);
				SetDlgItemText(hwnd, IDC_FINDTEXT, tch);
				break;

			case IDC_CLEAR_REPLACE:
				GetDlgItemText(hwnd, IDC_REPLACETEXT, tch, COUNTOF(tch));
				SendDlgItemMessage(hwnd, IDC_REPLACETEXT, CB_RESETCONTENT, 0, 0);
				MRU_Empty(mruReplace);
				MRU_Save(mruReplace);
				SetDlgItemText(hwnd, IDC_REPLACETEXT, tch);
				break;

			case IDC_SAVEPOSITION:
				PostWMCommand(hwnd, IDACC_SAVEPOS);
				break;

			case IDC_RESETPOSITION:
				PostWMCommand(hwnd, IDACC_RESETPOS);
				break;
			}
			break;
		}
	}
	break;

	case WM_ACTIVATE :
		SetWindowTransparentMode(hwnd, (LOWORD(wParam) == WA_INACTIVE && bFindReplaceTransparentMode), iFindReplaceOpacityLevel);
		break;
	}

	return FALSE;
}

//=============================================================================
//
// EditFindReplaceDlg()
//
HWND EditFindReplaceDlg(HWND hwnd, LPEDITFINDREPLACE lpefr, BOOL bReplace) {
	lpefr->hwnd = hwnd;
	HWND hDlg = CreateThemedDialogParam(g_hInstance,
								   (bReplace) ? MAKEINTRESOURCE(IDD_REPLACE) : MAKEINTRESOURCE(IDD_FIND),
								   GetParent(hwnd),
								   EditFindReplaceDlgProc,
								   (LPARAM)lpefr);

	ShowWindow(hDlg, SW_SHOW);
	return hDlg;
}

// Wildcard search uses the regexp engine to perform a simple search with * ? as wildcards instead of more advanced and user-unfriendly regexp syntax
void EscapeWildcards(char *szFind2, LPEDITFINDREPLACE lpefr) {
	char szWildcardEscaped[NP2_FIND_REPLACE_LIMIT];
	int iSource = 0;
	int iDest = 0;

	lpefr->fuFlags |= SCFIND_REGEXP;

	while (szFind2[iSource]) {
		const char c = szFind2[iSource];
		if (c == '*') {
			szWildcardEscaped[iDest++] = '.';
			szWildcardEscaped[iDest] = '*';
		} else if (c == '?') {
			szWildcardEscaped[iDest] = '.';
		} else {
			if (c == '.' || c == '^' || c == '$' || c == '\\' || c == '[' || c == ']' || c == '+') {
				szWildcardEscaped[iDest++] = '\\';
			}
			szWildcardEscaped[iDest] = c;
		}
		iSource++;
		iDest++;
	}
	szWildcardEscaped[iDest] = 0;
	lstrcpynA(szFind2, szWildcardEscaped, COUNTOF(szWildcardEscaped));
}

//=============================================================================
//
// EditFindNext()
//
BOOL EditFindNext(HWND hwnd, LPEDITFINDREPLACE lpefr, BOOL fExtendSelection) {
	if (StrIsEmptyA(lpefr->szFind)) {
		return /*EditFindReplaceDlg(hwnd, lpefr, FALSE)*/FALSE;
	}

	char szFind2[NP2_FIND_REPLACE_LIMIT];
	lstrcpynA(szFind2, lpefr->szFind, COUNTOF(szFind2));
	if (lpefr->bTransformBS) {
		const UINT cpEdit = (UINT)SendMessage(hwnd, SCI_GETCODEPAGE, 0, 0);
		TransformBackslashes(szFind2, (lpefr->fuFlags & SCFIND_REGEXP), cpEdit);
	}

	if (StrIsEmptyA(szFind2)) {
		InfoBox(0, L"MsgNotFound", IDS_NOTFOUND);
		return FALSE;
	}

	if (lpefr->bWildcardSearch) {
		EscapeWildcards(szFind2, lpefr);
	}

	const int iSelPos = (int)SendMessage(hwnd, SCI_GETCURRENTPOS, 0, 0);
	const int iSelAnchor = (int)SendMessage(hwnd, SCI_GETANCHOR, 0, 0);

	struct Sci_TextToFind ttf;
	ZeroMemory(&ttf, sizeof(ttf));
	ttf.chrg.cpMin = (int)SendMessage(hwnd, SCI_GETSELECTIONEND, 0, 0);
	ttf.chrg.cpMax = (int)SendMessage(hwnd, SCI_GETLENGTH, 0, 0);
	ttf.lpstrText = szFind2;

	int iPos = (int)SendMessage(hwnd, SCI_FINDTEXT, lpefr->fuFlags, (LPARAM)&ttf);
	BOOL bSuppressNotFound = FALSE;

	if (iPos == -1 && ttf.chrg.cpMin > 0 && !lpefr->bNoFindWrap && !fExtendSelection) {
		if (!lpefr->bNoFindWrap || (IDOK == InfoBox(MBOKCANCEL, L"MsgFindWrap1", IDS_FIND_WRAPFW))) {
			ttf.chrg.cpMin = 0;
			iPos = (int)SendMessage(hwnd, SCI_FINDTEXT, lpefr->fuFlags, (LPARAM)&ttf);
		} else {
			bSuppressNotFound = TRUE;
		}
	}

	if (iPos == -1) {
		// notfound
		if (!bSuppressNotFound) {
			InfoBox(0, L"MsgNotFound", IDS_NOTFOUND);
		}
		return FALSE;
	}

	if (!fExtendSelection) {
		EditSelectEx(hwnd, ttf.chrgText.cpMin, ttf.chrgText.cpMax);
	} else {
		EditSelectEx(hwnd, min_i(iSelAnchor, iSelPos), ttf.chrgText.cpMax);
	}

	return TRUE;
}

//=============================================================================
//
// EditFindPrev()
//
BOOL EditFindPrev(HWND hwnd, LPEDITFINDREPLACE lpefr, BOOL fExtendSelection) {
	if (StrIsEmptyA(lpefr->szFind)) {
		return /*EditFindReplaceDlg(hwnd, lpefr, FALSE)*/FALSE;
	}

	char szFind2[NP2_FIND_REPLACE_LIMIT];
	lstrcpynA(szFind2, lpefr->szFind, COUNTOF(szFind2));
	if (lpefr->bTransformBS) {
		const UINT cpEdit = (UINT)SendMessage(hwnd, SCI_GETCODEPAGE, 0, 0);
		TransformBackslashes(szFind2, (lpefr->fuFlags & SCFIND_REGEXP), cpEdit);
	}

	if (StrIsEmptyA(szFind2)) {
		InfoBox(0, L"MsgNotFound", IDS_NOTFOUND);
		return FALSE;
	}

	if (lpefr->bWildcardSearch) {
		EscapeWildcards(szFind2, lpefr);
	}

	const int iSelPos = (int)SendMessage(hwnd, SCI_GETCURRENTPOS, 0, 0);
	const int iSelAnchor = (int)SendMessage(hwnd, SCI_GETANCHOR, 0, 0);

	struct Sci_TextToFind ttf;
	ZeroMemory(&ttf, sizeof(ttf));
	ttf.chrg.cpMin = max_i(0, (int)SendMessage(hwnd, SCI_GETSELECTIONSTART, 0, 0));
	ttf.chrg.cpMax = 0;
	ttf.lpstrText = szFind2;

	int iPos = (int)SendMessage(hwnd, SCI_FINDTEXT, lpefr->fuFlags, (LPARAM)&ttf);
	const int iLength = (int)SendMessage(hwnd, SCI_GETLENGTH, 0, 0);
	BOOL bSuppressNotFound = FALSE;

	if (iPos == -1 && ttf.chrg.cpMin < iLength && !lpefr->bNoFindWrap && !fExtendSelection) {
		if (!lpefr->bNoFindWrap || (IDOK == InfoBox(MBOKCANCEL, L"MsgFindWrap2", IDS_FIND_WRAPRE))) {
			ttf.chrg.cpMin = iLength;
			iPos = (int)SendMessage(hwnd, SCI_FINDTEXT, lpefr->fuFlags, (LPARAM)&ttf);
		} else {
			bSuppressNotFound = TRUE;
		}
	}

	if (iPos == -1) {
		// notfound
		if (!bSuppressNotFound) {
			InfoBox(0, L"MsgNotFound", IDS_NOTFOUND);
		}
		return FALSE;
	}

	if (!fExtendSelection) {
		EditSelectEx(hwnd, ttf.chrgText.cpMin, ttf.chrgText.cpMax);
	} else {
		EditSelectEx(hwnd, max_i(iSelPos, iSelAnchor), ttf.chrgText.cpMin);
	}

	return TRUE;
}

//=============================================================================
//
// EditReplace()
//
BOOL EditReplace(HWND hwnd, LPEDITFINDREPLACE lpefr) {
	if (StrIsEmptyA(lpefr->szFind)) {
		return /*EditFindReplaceDlg(hwnd, lpefr, TRUE)*/FALSE;
	}

	const UINT cpEdit = (UINT)SendMessage(hwnd, SCI_GETCODEPAGE, 0, 0);
	char szFind2[NP2_FIND_REPLACE_LIMIT];
	lstrcpynA(szFind2, lpefr->szFind, COUNTOF(szFind2));
	if (lpefr->bTransformBS) {
		TransformBackslashes(szFind2, (lpefr->fuFlags & SCFIND_REGEXP), cpEdit);
	}

	if (StrIsEmptyA(szFind2)) {
		InfoBox(0, L"MsgNotFound", IDS_NOTFOUND);
		return FALSE;
	}

	if (lpefr->bWildcardSearch) {
		EscapeWildcards(szFind2, lpefr);
	}

	int iReplaceMsg = (lpefr->fuFlags & SCFIND_REGEXP) ? SCI_REPLACETARGETRE : SCI_REPLACETARGET;
	char *pszReplace2;
	if (strcmp(lpefr->szReplace, "^c") == 0) {
		iReplaceMsg = SCI_REPLACETARGET;
		pszReplace2 = EditGetClipboardText(hwnd);
	} else {
		//strcpy(szReplace2, lpefr->szReplace);
		pszReplace2 = StrDupA(lpefr->szReplace);
		if (lpefr->bTransformBS) {
			TransformBackslashes(pszReplace2, (lpefr->fuFlags & SCFIND_REGEXP), cpEdit);
		}
	}

	if (!pszReplace2) {
		pszReplace2 = StrDupA("");
	}

	const int iSelStart = (int)SendMessage(hwnd, SCI_GETSELECTIONSTART, 0, 0);
	const int iSelEnd = (int)SendMessage(hwnd, SCI_GETSELECTIONEND, 0, 0);

	struct Sci_TextToFind ttf;
	ZeroMemory(&ttf, sizeof(ttf));
	ttf.chrg.cpMin = (int)SendMessage(hwnd, SCI_GETSELECTIONSTART, 0, 0); // Start!
	ttf.chrg.cpMax = (int)SendMessage(hwnd, SCI_GETLENGTH, 0, 0);
	ttf.lpstrText = szFind2;

	int iPos = (int)SendMessage(hwnd, SCI_FINDTEXT, lpefr->fuFlags, (LPARAM)&ttf);
	BOOL bSuppressNotFound = FALSE;

	if (iPos == -1 && ttf.chrg.cpMin > 0 && !lpefr->bNoFindWrap) {
		if (!lpefr->bNoFindWrap || (IDOK == InfoBox(MBOKCANCEL, L"MsgFindWrap1", IDS_FIND_WRAPFW))) {
			ttf.chrg.cpMin = 0;
			iPos = (int)SendMessage(hwnd, SCI_FINDTEXT, lpefr->fuFlags, (LPARAM)&ttf);
		} else {
			bSuppressNotFound = TRUE;
		}
	}

	if (iPos == -1) {
		// notfound
		LocalFree(pszReplace2);
		if (!bSuppressNotFound) {
			InfoBox(0, L"MsgNotFound", IDS_NOTFOUND);
		}
		return FALSE;
	}

	if (iSelStart != ttf.chrgText.cpMin || iSelEnd != ttf.chrgText.cpMax) {
		LocalFree(pszReplace2);
		EditSelectEx(hwnd, ttf.chrgText.cpMin, ttf.chrgText.cpMax);
		return FALSE;
	}

	SendMessage(hwnd, SCI_SETTARGETSTART, ttf.chrgText.cpMin, 0);
	SendMessage(hwnd, SCI_SETTARGETEND, ttf.chrgText.cpMax, 0);
	SendMessage(hwnd, iReplaceMsg, (WPARAM)(-1), (LPARAM)pszReplace2);

	ttf.chrg.cpMin = (int)SendMessage(hwnd, SCI_GETTARGETEND, 0, 0);
	ttf.chrg.cpMax = (int)SendMessage(hwnd, SCI_GETLENGTH, 0, 0);

	iPos = (int)SendMessage(hwnd, SCI_FINDTEXT, lpefr->fuFlags, (LPARAM)&ttf);

	bSuppressNotFound = FALSE;

	if (iPos == -1 && ttf.chrg.cpMin > 0 && !lpefr->bNoFindWrap) {
		if (!lpefr->bNoFindWrap || (IDOK == InfoBox(MBOKCANCEL, L"MsgFindWrap1", IDS_FIND_WRAPFW))) {
			ttf.chrg.cpMin = 0;
			iPos = (int)SendMessage(hwnd, SCI_FINDTEXT, lpefr->fuFlags, (LPARAM)&ttf);
		} else {
			bSuppressNotFound = TRUE;
		}
	}

	if (iPos != -1) {
		EditSelectEx(hwnd, ttf.chrgText.cpMin, ttf.chrgText.cpMax);
	} else {
		EditSelectEx(hwnd,
					 (int)SendMessage(hwnd, SCI_GETTARGETEND, 0, 0),
					 (int)SendMessage(hwnd, SCI_GETTARGETEND, 0, 0));
		if (!bSuppressNotFound) {
			InfoBox(0, L"MsgNotFound", IDS_NOTFOUND);
		}
	}

	LocalFree(pszReplace2);

	return TRUE;
}

//=============================================================================
//
// EditMarkAll()
// Mark all occurrences of the text currently selected (by Aleksandar Lekov)
//

extern int iMatchesCount;

void EditMarkAll(HWND hwnd, int iMarkOccurrences, BOOL bMarkOccurrencesMatchCase, BOOL bMarkOccurrencesMatchWords) {
	iMatchesCount = 0;
	// feature is off
	if (!iMarkOccurrences) {
		return;
	}

	const int iTextLen = (int)SendMessage(hwnd, SCI_GETLENGTH, 0, 0);

	// get current selection
	int iSelStart = (int)SendMessage(hwnd, SCI_GETSELECTIONSTART, 0, 0);
	const int iSelEnd = (int)SendMessage(hwnd, SCI_GETSELECTIONEND, 0, 0);
	int iSelCount = iSelEnd - iSelStart;

	// clear existing indicator
	SendMessage(hwnd, SCI_SETINDICATORCURRENT, 1, 0);
	SendMessage(hwnd, SCI_INDICATORCLEARRANGE, 0, iTextLen);

	// if nothing selected or multiple lines are selected exit
	if (iSelCount == 0 ||
			(int)SendMessage(hwnd, SCI_LINEFROMPOSITION, iSelStart, 0) !=
			(int)SendMessage(hwnd, SCI_LINEFROMPOSITION, iSelEnd, 0)) {
		return;
	}

	// scintilla/src/Editor.h SelectionText.LengthWithTerminator()
	iSelCount = (int)SendMessage(hwnd, SCI_GETSELTEXT, 0, 0) - 1;
	char *pszText = (char *)NP2HeapAlloc(iSelCount + 1);
	SendMessage(hwnd, SCI_GETSELTEXT, 0, (LPARAM)pszText);

	// exit if selection is not a word and Match whole words only is enabled
	if (bMarkOccurrencesMatchWords) {
		const UINT cpEdit = (UINT)SendMessage(hwnd, SCI_GETCODEPAGE, 0, 0);
		const BOOL dbcs = !(cpEdit == CP_UTF8 || cpEdit == 0);
		// CharClassify::SetDefaultCharClasses()
		for (iSelStart = 0; iSelStart < iSelCount; ++iSelStart) {
			const unsigned char ch = pszText[iSelStart];
			if (dbcs && IsDBCSLeadByteEx(cpEdit, ch)) {
				++iSelStart;
			} else if (!(ch >= 0x80 || IsDocWordChar(ch))) {
				NP2HeapFree(pszText);
				return;
			}
		}
	}

	struct Sci_TextToFind ttf;
	ZeroMemory(&ttf, sizeof(ttf));
	ttf.chrg.cpMin = 0;
	ttf.chrg.cpMax = iTextLen;
	ttf.lpstrText = pszText;

	// set style
	const COLORREF fore = 0xff << ((iMarkOccurrences - 1) << 3);
	SendMessage(hwnd, SCI_INDICSETALPHA, 1, 100);
	SendMessage(hwnd, SCI_INDICSETFORE, 1, fore);
	SendMessage(hwnd, SCI_INDICSETSTYLE, 1, INDIC_ROUNDBOX);

	const int findFlag = (bMarkOccurrencesMatchCase ? SCFIND_MATCHCASE : 0) | (bMarkOccurrencesMatchWords ? SCFIND_WHOLEWORD : 0);
	int iPos;
	while ((iPos = (int)SendMessage(hwnd, SCI_FINDTEXT, findFlag, (LPARAM)&ttf)) != -1) {
		// mark this match
		++iMatchesCount;
		SendMessage(hwnd, SCI_INDICATORFILLRANGE, iPos, iSelCount);
		ttf.chrg.cpMin = ttf.chrgText.cpMin + iSelCount;
		if (ttf.chrg.cpMin == ttf.chrg.cpMax) {
			break;
		}
	}

	NP2HeapFree(pszText);
}

//=============================================================================
//
// EditReplaceAll()
//
BOOL EditReplaceAll(HWND hwnd, LPEDITFINDREPLACE lpefr, BOOL bShowInfo) {
	if (StrIsEmptyA(lpefr->szFind)) {
		return /*EditFindReplaceDlg(hwnd, lpefr, TRUE)*/FALSE;
	}

	// Show wait cursor...
	BeginWaitCursor();

	const UINT cpEdit = (UINT)SendMessage(hwnd, SCI_GETCODEPAGE, 0, 0);
	char szFind2[NP2_FIND_REPLACE_LIMIT];
	lstrcpynA(szFind2, lpefr->szFind, COUNTOF(szFind2));
	if (lpefr->bTransformBS) {
		TransformBackslashes(szFind2, (lpefr->fuFlags & SCFIND_REGEXP), cpEdit);
	}

	if (StrIsEmptyA(szFind2)) {
		InfoBox(0, L"MsgNotFound", IDS_NOTFOUND);
		return FALSE;
	}

	if (lpefr->bWildcardSearch) {
		EscapeWildcards(szFind2, lpefr);
	}

	int iReplaceMsg = (lpefr->fuFlags & SCFIND_REGEXP) ? SCI_REPLACETARGETRE : SCI_REPLACETARGET;
	const BOOL bRegexStartOfLine = (lpefr->fuFlags & SCFIND_REGEXP) && (szFind2[0] == '^');
	const BOOL bRegexStartOrEndOfLine =
		(lpefr->fuFlags & SCFIND_REGEXP) &&
		((!strcmp(szFind2, "$") || !strcmp(szFind2, "^") || !strcmp(szFind2, "^$")));

	char *pszReplace2;
	if (strcmp(lpefr->szReplace, "^c") == 0) {
		iReplaceMsg = SCI_REPLACETARGET;
		pszReplace2 = EditGetClipboardText(hwnd);
	} else {
		//strcpy(szReplace2, lpefr->szReplace);
		pszReplace2 = StrDupA(lpefr->szReplace);
		if (lpefr->bTransformBS) {
			TransformBackslashes(pszReplace2, (lpefr->fuFlags & SCFIND_REGEXP), cpEdit);
		}
	}

	if (!pszReplace2) {
		pszReplace2 = StrDupA("");
	}

	struct Sci_TextToFind ttf;
	ZeroMemory(&ttf, sizeof(ttf));
	ttf.chrg.cpMin = 0;
	ttf.chrg.cpMax = (int)SendMessage(hwnd, SCI_GETLENGTH, 0, 0);
	ttf.lpstrText = szFind2;

	int iCount = 0;
	while ((int)SendMessage(hwnd, SCI_FINDTEXT, lpefr->fuFlags, (LPARAM)&ttf) != -1) {
		if (iCount == 0 && bRegexStartOrEndOfLine) {
			if (0 == SendMessage(hwnd, SCI_GETLINEENDPOSITION, 0, 0)) {
				ttf.chrgText.cpMin = 0;
				ttf.chrgText.cpMax = 0;
			}
		}

		if (++iCount == 1) {
			SendMessage(hwnd, SCI_BEGINUNDOACTION, 0, 0);
		}

		SendMessage(hwnd, SCI_SETTARGETSTART, ttf.chrgText.cpMin, 0);
		SendMessage(hwnd, SCI_SETTARGETEND, ttf.chrgText.cpMax, 0);
		const int iReplacedLen = (int)SendMessage(hwnd, iReplaceMsg, (WPARAM)(-1), (LPARAM)pszReplace2);

		ttf.chrg.cpMin = ttf.chrgText.cpMin + iReplacedLen;
		ttf.chrg.cpMax = (int)SendMessage(hwnd, SCI_GETLENGTH, 0, 0);

		if (ttf.chrg.cpMin == ttf.chrg.cpMax) {
			break;
		}

		//const int ch = (int)SendMessage(hwnd, SCI_GETCHARAT, SendMessage(hwnd, SCI_GETTARGETEND, 0, 0), 0);
		if (/*ch == '\r' || ch == '\n' || iReplacedLen == 0 || */
			ttf.chrgText.cpMin == ttf.chrgText.cpMax &&
			!(bRegexStartOrEndOfLine && iReplacedLen > 0)) {
			ttf.chrg.cpMin = (int)SendMessage(hwnd, SCI_POSITIONAFTER, ttf.chrg.cpMin, 0);
		}

		if (bRegexStartOfLine) {
			const int iLine = (int)SendMessage(hwnd, SCI_LINEFROMPOSITION, ttf.chrg.cpMin, 0);
			const int ilPos = (int)SendMessage(hwnd, SCI_POSITIONFROMLINE, iLine, 0);

			if (ilPos == ttf.chrg.cpMin) {
				ttf.chrg.cpMin = (int)SendMessage(hwnd, SCI_POSITIONFROMLINE, iLine + 1, 0);
			}
			if (ttf.chrg.cpMin == ttf.chrg.cpMax) {
				break;
			}
		}
	}

	if (iCount) {
		SendMessage(hwnd, SCI_ENDUNDOACTION, 0, 0);
	}

	// Remove wait cursor
	EndWaitCursor();

	if (bShowInfo) {
		if (iCount > 0) {
			InfoBox(0, L"MsgReplaceCount", IDS_REPLCOUNT, iCount);
		} else {
			InfoBox(0, L"MsgNotFound", IDS_NOTFOUND);
		}
	}

	LocalFree(pszReplace2);

	return TRUE;
}

//=============================================================================
//
// EditReplaceAllInSelection()
//
BOOL EditReplaceAllInSelection(HWND hwnd, LPEDITFINDREPLACE lpefr, BOOL bShowInfo) {
	if (StrIsEmptyA(lpefr->szFind)) {
		return /*EditFindReplaceDlg(hwnd, lpefr, TRUE)*/FALSE;
	}
	if (SC_SEL_RECTANGLE == SendMessage(hwnd, SCI_GETSELECTIONMODE, 0, 0)) {
		MsgBox(MBWARN, IDS_SELRECT);
		return FALSE;
	}

	// Show wait cursor...
	BeginWaitCursor();

	const UINT cpEdit = (UINT)SendMessage(hwnd, SCI_GETCODEPAGE, 0, 0);
	char szFind2[NP2_FIND_REPLACE_LIMIT];
	lstrcpynA(szFind2, lpefr->szFind, COUNTOF(szFind2));
	if (lpefr->bTransformBS) {
		TransformBackslashes(szFind2, (lpefr->fuFlags & SCFIND_REGEXP), cpEdit);
	}

	if (StrIsEmptyA(szFind2)) {
		InfoBox(0, L"MsgNotFound", IDS_NOTFOUND);
		return FALSE;
	}

	if (lpefr->bWildcardSearch) {
		EscapeWildcards(szFind2, lpefr);
	}

	int iReplaceMsg = (lpefr->fuFlags & SCFIND_REGEXP) ? SCI_REPLACETARGETRE : SCI_REPLACETARGET;
	const BOOL bRegexStartOfLine = (lpefr->fuFlags & SCFIND_REGEXP) && (szFind2[0] == '^');
	const BOOL bRegexStartOrEndOfLine =
		(lpefr->fuFlags & SCFIND_REGEXP) &&
		((!strcmp(szFind2, "$") || !strcmp(szFind2, "^") || !strcmp(szFind2, "^$")));

	char *pszReplace2;
	if (strcmp(lpefr->szReplace, "^c") == 0) {
		iReplaceMsg = SCI_REPLACETARGET;
		pszReplace2 = EditGetClipboardText(hwnd);
	} else {
		//strcpy(szReplace2, lpefr->szReplace);
		pszReplace2 = StrDupA(lpefr->szReplace);
		if (lpefr->bTransformBS) {
			TransformBackslashes(pszReplace2, (lpefr->fuFlags & SCFIND_REGEXP), cpEdit);
		}
	}

	if (!pszReplace2) {
		pszReplace2 = StrDupA("");
	}

	struct Sci_TextToFind ttf;
	ZeroMemory(&ttf, sizeof(ttf));
	ttf.chrg.cpMin = (int)SendMessage(hwnd, SCI_GETSELECTIONSTART, 0, 0);
	ttf.chrg.cpMax = (int)SendMessage(hwnd, SCI_GETLENGTH, 0, 0);
	ttf.lpstrText = szFind2;

	int iCount = 0;
	BOOL fCancel = FALSE;
	while ((int)SendMessage(hwnd, SCI_FINDTEXT, lpefr->fuFlags, (LPARAM)&ttf) != -1 && !fCancel) {
		if (ttf.chrgText.cpMin >= SendMessage(hwnd, SCI_GETSELECTIONSTART, 0, 0) &&
			ttf.chrgText.cpMax <= SendMessage(hwnd, SCI_GETSELECTIONEND, 0, 0)) {

			if (ttf.chrg.cpMin == 0 && iCount == 0 && bRegexStartOrEndOfLine) {
				if (0 == SendMessage(hwnd, SCI_GETLINEENDPOSITION, 0, 0)) {
					ttf.chrgText.cpMin = 0;
					ttf.chrgText.cpMax = 0;
				}
			}

			if (++iCount == 1) {
				SendMessage(hwnd, SCI_BEGINUNDOACTION, 0, 0);
			}

			SendMessage(hwnd, SCI_SETTARGETSTART, ttf.chrgText.cpMin, 0);
			SendMessage(hwnd, SCI_SETTARGETEND, ttf.chrgText.cpMax, 0);
			const int iReplacedLen = (int)SendMessage(hwnd, iReplaceMsg, (WPARAM)(-1), (LPARAM)pszReplace2);

			ttf.chrg.cpMin = ttf.chrgText.cpMin + iReplacedLen;
			ttf.chrg.cpMax = (int)SendMessage(hwnd, SCI_GETLENGTH, 0, 0);

			if (ttf.chrg.cpMin == ttf.chrg.cpMax) {
				fCancel = TRUE;
			}

			//const int ch = (int)SendMessage(hwnd, SCI_GETCHARAT, SendMessage(hwnd, SCI_GETTARGETEND, 0, 0), 0);
			if (/*ch == '\r' || ch == '\n' || iReplacedLen == 0 || */
				ttf.chrgText.cpMin == ttf.chrgText.cpMax &&
				!(bRegexStartOrEndOfLine && iReplacedLen > 0)) {
				ttf.chrg.cpMin = (int)SendMessage(hwnd, SCI_POSITIONAFTER, ttf.chrg.cpMin, 0);
			}

			if (bRegexStartOfLine) {
				const int iLine = (int)SendMessage(hwnd, SCI_LINEFROMPOSITION, ttf.chrg.cpMin, 0);
				const int ilPos = (int)SendMessage(hwnd, SCI_POSITIONFROMLINE, iLine, 0);

				if (ilPos == ttf.chrg.cpMin) {
					ttf.chrg.cpMin = (int)SendMessage(hwnd, SCI_POSITIONFROMLINE, iLine + 1, 0);
				}
				if (ttf.chrg.cpMin == ttf.chrg.cpMax) {
					break;
				}
			}
		} else { // gone across selection, cancel
			fCancel = TRUE;
		}
	}

	if (iCount) {
		if (SendMessage(hwnd, SCI_GETSELECTIONEND, 0, 0) <
				SendMessage(hwnd, SCI_GETTARGETEND, 0, 0)) {
			int iAnchorPos = (int)SendMessage(hwnd, SCI_GETANCHOR, 0, 0);
			int iCurrentPos = (int)SendMessage(hwnd, SCI_GETCURRENTPOS, 0, 0);

			if (iAnchorPos > iCurrentPos) {
				iAnchorPos = (int)SendMessage(hwnd, SCI_GETTARGETEND, 0, 0);
			} else {
				iCurrentPos = (int)SendMessage(hwnd, SCI_GETTARGETEND, 0, 0);
			}

			EditSelectEx(hwnd, iAnchorPos, iCurrentPos);
		}

		SendMessage(hwnd, SCI_ENDUNDOACTION, 0, 0);
	}

	// Remove wait cursor
	EndWaitCursor();

	if (bShowInfo) {
		if (iCount > 0) {
			InfoBox(0, L"MsgReplaceCount", IDS_REPLCOUNT, iCount);
		} else {
			InfoBox(0, L"MsgNotFound", IDS_NOTFOUND);
		}
	}

	LocalFree(pszReplace2);

	return TRUE;
}

//=============================================================================
//
// EditLineNumDlgProc()
//
static INT_PTR CALLBACK EditLineNumDlgProc(HWND hwnd, UINT umsg, WPARAM wParam, LPARAM lParam) {
	UNREFERENCED_PARAMETER(lParam);

	switch (umsg) {
	case WM_INITDIALOG: {
		const int iCurLine = (int)SendMessage(hwndEdit, SCI_LINEFROMPOSITION,
										SendMessage(hwndEdit, SCI_GETCURRENTPOS, 0, 0), 0) + 1;
		const int iMaxLine = (int)SendMessage(hwndEdit, SCI_GETLINECOUNT, 0, 0);
		const int iLength = (int)SendMessage(hwndEdit, SCI_GETLENGTH, 0, 0);

		SetDlgItemInt(hwnd, IDC_LINENUM, iCurLine, FALSE);
		SendDlgItemMessage(hwnd, IDC_LINENUM, EM_LIMITTEXT, 20, 0);
		SendDlgItemMessage(hwnd, IDC_COLNUM, EM_LIMITTEXT, 20, 0);

		WCHAR tchLn[32];
		WCHAR tchLines[64];
		WCHAR tchFmt[64];

		wsprintf(tchLn, L"%i", iMaxLine);
		FormatNumberStr(tchLn);
		GetDlgItemText(hwnd, IDC_LINE_RANGE, tchFmt, COUNTOF(tchFmt));
		wsprintf(tchLines, tchFmt, tchLn);
		SetDlgItemText(hwnd, IDC_LINE_RANGE, tchLines);

		wsprintf(tchLn, L"%i", iLength);
		FormatNumberStr(tchLn);
		GetDlgItemText(hwnd, IDC_COLUMN_RANGE, tchFmt, COUNTOF(tchFmt));
		wsprintf(tchLines, tchFmt, tchLn);
		SetDlgItemText(hwnd, IDC_COLUMN_RANGE, tchLines);

		CenterDlgInParent(hwnd);
	}
	return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK: {
			BOOL fTranslated;
			BOOL fTranslated2;
			WCHAR tchLn[32];
			int iNewLine = 0;
			int iNewCol;

			// Extract line number from the text entered
			// For example: "5410:" will result in 5410
			GetDlgItemText(hwnd, IDC_LINENUM, tchLn, COUNTOF(tchLn));
			fTranslated = CRTStrToInt(tchLn, &iNewLine);

			if (SendDlgItemMessage(hwnd, IDC_COLNUM, WM_GETTEXTLENGTH, 0, 0) > 0) {
				iNewCol = GetDlgItemInt(hwnd, IDC_COLNUM, &fTranslated2, FALSE);
			} else {
				iNewCol = 1;
				fTranslated2 = fTranslated;
			}

			if (!(fTranslated || fTranslated2)) {
				PostMessage(hwnd, WM_NEXTDLGCTL, (WPARAM)(GetDlgItem(hwnd, IDC_LINENUM)), 1);
				return TRUE;
			}

			const int iMaxLine = (int)SendMessage(hwndEdit, SCI_GETLINECOUNT, 0, 0);
			const int iLength = (int)SendMessage(hwndEdit, SCI_GETLENGTH, 0, 0);
			// directly goto specific position
			if (fTranslated2 && !fTranslated) {
				if (iNewCol > 0 && iNewCol <= iLength) {
					SendMessage(hwndEdit, SCI_GOTOPOS, iNewCol - 1, 0);
					SendMessage(hwndEdit, SCI_CHOOSECARETX, 0, 0);
					EndDialog(hwnd, IDOK);
				} else {
					PostMessage(hwnd, WM_NEXTDLGCTL, (WPARAM)(GetDlgItem(hwnd, IDC_COLNUM)), 1);
				}
			} else if (iNewLine > 0 && iNewLine <= iMaxLine && iNewCol > 0) {
				//int iNewPos = SendMessage(hwndEdit, SCI_POSITIONFROMLINE, iNewLine - 1, 0);
				//const int iLineEndPos = SendMessage(hwndEdit, SCI_GETLINEENDPOSITION, iNewLine - 1, 0);
				//while (iNewCol-1 > SendMessage(hwndEdit, SCI_GETCOLUMN, iNewPos, 0)) {
				//	if (iNewPos >= iLineEndPos) {
				//		break;
				//	}
				//	iNewPos = SendMessage(hwndEdit, SCI_POSITIONAFTER, (iNewPos, 0);
				//}
				//iNewPos = min_i(iNewPos, iLineEndPos);
				//SendMessage(hwndEdit, SCI_GOTOPOS, iNewPos, 0);
				//SendMessage(hwndEdit, SCI_CHOOSECARETX, 0, 0);
				EditJumpTo(hwndEdit, iNewLine, iNewCol);
				EndDialog(hwnd, IDOK);
			} else {
				PostMessage(hwnd, WM_NEXTDLGCTL, (WPARAM)(GetDlgItem(hwnd, ((iNewCol > 0) ? IDC_LINENUM : IDC_COLNUM))), 1);
			}
		}
		break;

		case IDCANCEL:
			EndDialog(hwnd, IDCANCEL);
			break;
		}

		return TRUE;
	}

	return FALSE;
}

//=============================================================================
//
// EditLinenumDlg()
//
BOOL EditLineNumDlg(HWND hwnd) {
	const INT_PTR iResult = ThemedDialogBoxParam(g_hInstance, MAKEINTRESOURCE(IDD_LINENUM), GetParent(hwnd), EditLineNumDlgProc, (LPARAM)hwnd);
	return iResult == IDOK;
}

//=============================================================================
//
// EditModifyLinesDlg()
//
//
extern int cxModifyLinesDlg;
extern int cyModifyLinesDlg;
extern int cxEncloseSelectionDlg;
extern int cyEncloseSelectionDlg;
extern int cxInsertTagDlg;
extern int cyInsertTagDlg;

static INT_PTR CALLBACK EditModifyLinesDlgProc(HWND hwnd, UINT umsg, WPARAM wParam, LPARAM lParam) {
	static int id_hover;
	static int id_capture;
	static HFONT hFontHover;
	static HCURSOR hCursorNormal;
	static HCURSOR hCursorHover;

	switch (umsg) {
	case WM_INITDIALOG: {
		ResizeDlg_InitY2(hwnd, cxModifyLinesDlg, cyModifyLinesDlg, IDC_RESIZEGRIP2, IDC_MODIFY_LINE_PREFIX, IDC_MODIFY_LINE_APPEND);

		id_hover = 0;
		id_capture = 0;

		HFONT hFontNormal = (HFONT)SendDlgItemMessage(hwnd, IDC_MODIFY_LINE_DLN_NP, WM_GETFONT, 0, 0);
		if (hFontNormal == NULL) {
			hFontNormal = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
		}

		LOGFONT lf;
		GetObject(hFontNormal, sizeof(LOGFONT), &lf);
		lf.lfUnderline = TRUE;
		hFontHover = CreateFontIndirect(&lf);

		hCursorNormal = LoadCursor(NULL, IDC_ARROW);
		if ((hCursorHover = LoadCursor(NULL, IDC_HAND)) == NULL) {
			hCursorHover = LoadCursor(g_hInstance, IDC_ARROW);
		}

		MultilineEditSetup(hwnd, IDC_MODIFY_LINE_PREFIX);
		SetDlgItemText(hwnd, IDC_MODIFY_LINE_PREFIX, wchPrefixLines.buffer);
		MultilineEditSetup(hwnd, IDC_MODIFY_LINE_APPEND);
		SetDlgItemText(hwnd, IDC_MODIFY_LINE_APPEND, wchAppendLines.buffer);
		CenterDlgInParent(hwnd);
	}
	return TRUE;

	case WM_DESTROY:
		ResizeDlg_Destroy(hwnd, &cxModifyLinesDlg, &cyModifyLinesDlg);
		DeleteObject(hFontHover);
		return FALSE;

	case WM_SIZE: {
		int dx;
		int dy;

		ResizeDlg_Size(hwnd, lParam, &dx, &dy);
		const int cy = ResizeDlg_CalcDeltaY2(hwnd, dy, 50, IDC_MODIFY_LINE_PREFIX, IDC_MODIFY_LINE_APPEND);
		HDWP hdwp = BeginDeferWindowPos(15);
		hdwp = DeferCtlPos(hdwp, hwnd, IDC_RESIZEGRIP2, dx, dy, SWP_NOSIZE);
		hdwp = DeferCtlPos(hdwp, hwnd, IDOK, dx, dy, SWP_NOSIZE);
		hdwp = DeferCtlPos(hdwp, hwnd, IDCANCEL, dx, dy, SWP_NOSIZE);
		hdwp = DeferCtlPos(hdwp, hwnd, IDC_MODIFY_LINE_PREFIX, dx, cy, SWP_NOMOVE);
		hdwp = DeferCtlPos(hdwp, hwnd, IDC_MODIFY_LINE_APPEND, 0, cy, SWP_NOSIZE);
		hdwp = DeferCtlPos(hdwp, hwnd, IDC_MODIFY_LINE_TIP2, 0, cy, SWP_NOSIZE);
		hdwp = DeferCtlPos(hdwp, hwnd, IDC_MODIFY_LINE_DLN_NP, 0, dy, SWP_NOSIZE);
		hdwp = DeferCtlPos(hdwp, hwnd, IDC_MODIFY_LINE_DLN_ZP, 0, dy, SWP_NOSIZE);
		hdwp = DeferCtlPos(hdwp, hwnd, IDC_MODIFY_LINE_TIP_DLN, 0, dy, SWP_NOSIZE);
		hdwp = DeferCtlPos(hdwp, hwnd, IDC_MODIFY_LINE_CN_NP, 0, dy, SWP_NOSIZE);
		hdwp = DeferCtlPos(hdwp, hwnd, IDC_MODIFY_LINE_CN_ZP, 0, dy, SWP_NOSIZE);
		hdwp = DeferCtlPos(hdwp, hwnd, IDC_MODIFY_LINE_TIP_CN, 0, dy, SWP_NOSIZE);
		hdwp = DeferCtlPos(hdwp, hwnd, IDC_MODIFY_LINE_ZCN_NP, 0, dy, SWP_NOSIZE);
		hdwp = DeferCtlPos(hdwp, hwnd, IDC_MODIFY_LINE_ZCN_ZP, 0, dy, SWP_NOSIZE);
		hdwp = DeferCtlPos(hdwp, hwnd, IDC_MODIFY_LINE_TIP_ZCN, 0, dy, SWP_NOSIZE);
		EndDeferWindowPos(hdwp);
		ResizeDlgCtl(hwnd, IDC_MODIFY_LINE_APPEND, dx, dy - cy);
	}
	return TRUE;

	case WM_GETMINMAXINFO:
		ResizeDlg_GetMinMaxInfo(hwnd, lParam);
		return TRUE;

	case WM_NCACTIVATE:
		if (!wParam) {
			if (id_hover != 0) {
				//int _id_hover = id_hover;
				id_hover = 0;
				id_capture = 0;
				//InvalidateRect(GetDlgItem(hwnd, id_hover), NULL, FALSE);
			}
		}
		return FALSE;

	case WM_CTLCOLORSTATIC: {
		const DWORD dwId = GetWindowLong((HWND)lParam, GWL_ID);

		if (dwId >= IDC_MODIFY_LINE_DLN_NP && dwId <= IDC_MODIFY_LINE_ZCN_ZP) {
			HDC hdc = (HDC)wParam;
			SetBkMode(hdc, TRANSPARENT);
			if (GetSysColorBrush(COLOR_HOTLIGHT)) {
				SetTextColor(hdc, GetSysColor(COLOR_HOTLIGHT));
			} else {
				SetTextColor(hdc, RGB(0, 0, 255));
			}
			SelectObject(hdc, /*dwId == id_hover?*/hFontHover/*:hFontNormal*/);
			return (LONG_PTR)GetSysColorBrush(COLOR_BTNFACE);
		}
	}
	break;

	case WM_MOUSEMOVE: {
		const POINT pt = { LOWORD(lParam), HIWORD(lParam) };
		HWND hwndHover = ChildWindowFromPoint(hwnd, pt);
		const DWORD dwId = GetWindowLong(hwndHover, GWL_ID);

		if (GetActiveWindow() == hwnd) {
			if (dwId >= IDC_MODIFY_LINE_DLN_NP && dwId <= IDC_MODIFY_LINE_ZCN_ZP) {
				if (id_capture == (int)dwId || id_capture == 0) {
					if (id_hover != id_capture || id_hover == 0) {
						id_hover = dwId;
						//InvalidateRect(GetDlgItem(hwnd, dwId), NULL, FALSE);
					}
				} else if (id_hover != 0) {
					//int _id_hover = id_hover;
					id_hover = 0;
					//InvalidateRect(GetDlgItem(hwnd, _id_hover), NULL, FALSE);
				}
			} else if (id_hover != 0) {
				//int _id_hover = id_hover;
				id_hover = 0;
				//InvalidateRect(GetDlgItem(hwnd, _id_hover), NULL, FALSE);
			}
			SetCursor(id_hover != 0 ? hCursorHover : hCursorNormal);
		}
	}
	break;

	case WM_LBUTTONDOWN: {
		const POINT pt = { LOWORD(lParam), HIWORD(lParam) };
		HWND hwndHover = ChildWindowFromPoint(hwnd, pt);
		const DWORD dwId = GetWindowLong(hwndHover, GWL_ID);

		if (dwId >= IDC_MODIFY_LINE_DLN_NP && dwId <= IDC_MODIFY_LINE_ZCN_ZP) {
			GetCapture();
			id_hover = dwId;
			id_capture = dwId;
			//InvalidateRect(GetDlgItem(hwnd, dwId), NULL, FALSE);
		}
		SetCursor(id_hover != 0 ? hCursorHover : hCursorNormal);
	}
	break;

	case WM_LBUTTONUP: {
		//const POINT pt = { LOWORD(lParam), HIWORD(lParam) };
		//HWND hwndHover = ChildWindowFromPoint(hwnd, pt);
		//const DWORD dwId = GetWindowLong(hwndHover, GWL_ID);

		if (id_capture != 0) {
			ReleaseCapture();
			if (id_hover == id_capture) {
				const int id_focus = GetWindowLong(GetFocus(), GWL_ID);
				if (id_focus == IDC_MODIFY_LINE_PREFIX || id_focus == IDC_MODIFY_LINE_APPEND) {
					WCHAR wch[8];
					GetDlgItemText(hwnd, id_capture, wch, COUNTOF(wch));
					SendDlgItemMessage(hwnd, id_focus, EM_SETSEL, 0, -1);
					SendDlgItemMessage(hwnd, id_focus, EM_REPLACESEL, TRUE, (LPARAM)wch);
					PostMessage(hwnd, WM_NEXTDLGCTL, (WPARAM)(GetFocus()), 1);
				}
			}
			id_capture = 0;
		}
		SetCursor(id_hover != 0 ? hCursorHover : hCursorNormal);
	}
	break;

	case WM_CANCELMODE:
		if (id_capture != 0) {
			ReleaseCapture();
			id_hover = 0;
			id_capture = 0;
			SetCursor(hCursorNormal);
		}
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK: {
			DString_GetDlgItemText(&wchPrefixLines, hwnd, IDC_MODIFY_LINE_PREFIX);
			DString_GetDlgItemText(&wchAppendLines, hwnd, IDC_MODIFY_LINE_APPEND);

			EditModifyLines(hwndEdit, wchPrefixLines.buffer, wchAppendLines.buffer);
			EndDialog(hwnd, IDOK);
		}
		break;

		case IDCANCEL:
			EndDialog(hwnd, IDCANCEL);
			break;
		}
		return TRUE;
	}

	return FALSE;
}

//=============================================================================
//
// EditModifyLinesDlg()
//
void EditModifyLinesDlg(HWND hwnd) {
	ThemedDialogBoxParam(g_hInstance, MAKEINTRESOURCE(IDD_MODIFYLINES), hwnd, EditModifyLinesDlgProc, 0);
}

//=============================================================================
//
// EditAlignDlgProc()
//
//
static INT_PTR CALLBACK EditAlignDlgProc(HWND hwnd, UINT umsg, WPARAM wParam, LPARAM lParam) {
	switch (umsg) {
	case WM_INITDIALOG: {
		SetWindowLongPtr(hwnd, DWLP_USER, lParam);
		const int iAlignMode = *((int *)lParam);
		CheckRadioButton(hwnd, IDC_ALIGN_LEFT, IDC_ALIGN_JUSTIFY_PAR, iAlignMode + IDC_ALIGN_LEFT);
		CenterDlgInParent(hwnd);
	}
	return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK: {
			int *piAlignMode = (int *)GetWindowLongPtr(hwnd, DWLP_USER);
			const int iAlignMode = GetCheckedRadioButton(hwnd, IDC_ALIGN_LEFT, IDC_ALIGN_JUSTIFY_PAR) - IDC_ALIGN_LEFT;
			*piAlignMode = iAlignMode;
			EndDialog(hwnd, IDOK);
		}
		break;

		case IDCANCEL:
			EndDialog(hwnd, IDCANCEL);
			break;
		}
		return TRUE;
	}

	return FALSE;
}

//=============================================================================
//
// EditAlignDlg()
//
BOOL EditAlignDlg(HWND hwnd, int *piAlignMode) {
	const INT_PTR iResult = ThemedDialogBoxParam(g_hInstance, MAKEINTRESOURCE(IDD_ALIGN), hwnd, EditAlignDlgProc, (LPARAM)piAlignMode);
	return iResult == IDOK;
}

//=============================================================================
//
// EditEncloseSelectionDlgProc()
//
//
static INT_PTR CALLBACK EditEncloseSelectionDlgProc(HWND hwnd, UINT umsg, WPARAM wParam, LPARAM lParam) {
	switch (umsg) {
	case WM_INITDIALOG: {
		ResizeDlg_InitY2(hwnd, cxEncloseSelectionDlg, cyEncloseSelectionDlg, IDC_RESIZEGRIP2, IDC_MODIFY_LINE_PREFIX, IDC_MODIFY_LINE_APPEND);

		MultilineEditSetup(hwnd, IDC_MODIFY_LINE_PREFIX);
		SetDlgItemText(hwnd, IDC_MODIFY_LINE_PREFIX, wchPrefixSelection.buffer);
		MultilineEditSetup(hwnd, IDC_MODIFY_LINE_APPEND);
		SetDlgItemText(hwnd, IDC_MODIFY_LINE_APPEND, wchAppendSelection.buffer);
		CenterDlgInParent(hwnd);
	}
	return TRUE;

	case WM_DESTROY:
		ResizeDlg_Destroy(hwnd, &cxEncloseSelectionDlg, &cyEncloseSelectionDlg);
		return FALSE;

	case WM_SIZE: {
		int dx;
		int dy;

		ResizeDlg_Size(hwnd, lParam, &dx, &dy);
		const int cy = ResizeDlg_CalcDeltaY2(hwnd, dy, 50, IDC_MODIFY_LINE_PREFIX, IDC_MODIFY_LINE_APPEND);
		HDWP hdwp = BeginDeferWindowPos(6);
		hdwp = DeferCtlPos(hdwp, hwnd, IDC_RESIZEGRIP2, dx, dy, SWP_NOSIZE);
		hdwp = DeferCtlPos(hdwp, hwnd, IDOK, dx, dy, SWP_NOSIZE);
		hdwp = DeferCtlPos(hdwp, hwnd, IDCANCEL, dx, dy, SWP_NOSIZE);
		hdwp = DeferCtlPos(hdwp, hwnd, IDC_MODIFY_LINE_PREFIX, dx, cy, SWP_NOMOVE);
		hdwp = DeferCtlPos(hdwp, hwnd, IDC_MODIFY_LINE_APPEND, 0, cy, SWP_NOSIZE);
		hdwp = DeferCtlPos(hdwp, hwnd, IDC_MODIFY_LINE_TIP2, 0, cy, SWP_NOSIZE);
		EndDeferWindowPos(hdwp);
		ResizeDlgCtl(hwnd, IDC_MODIFY_LINE_APPEND, dx, dy - cy);
	}
	return TRUE;

	case WM_GETMINMAXINFO:
		ResizeDlg_GetMinMaxInfo(hwnd, lParam);
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK: {
			DString_GetDlgItemText(&wchAppendSelection, hwnd, IDC_MODIFY_LINE_PREFIX);
			DString_GetDlgItemText(&wchPrefixSelection, hwnd, IDC_MODIFY_LINE_APPEND);

			EditEncloseSelection(hwndEdit, wchPrefixSelection.buffer, wchAppendSelection.buffer);
			EndDialog(hwnd, IDOK);
		}
		break;

		case IDCANCEL:
			EndDialog(hwnd, IDCANCEL);
			break;
		}
		return TRUE;
	}

	return FALSE;
}

//=============================================================================
//
// EditEncloseSelectionDlg()
//
void EditEncloseSelectionDlg(HWND hwnd) {
	ThemedDialogBoxParam(g_hInstance, MAKEINTRESOURCE(IDD_ENCLOSESELECTION), hwnd, EditEncloseSelectionDlgProc, 0);
}

//=============================================================================
//
// EditInsertTagDlgProc()
//
//
static INT_PTR CALLBACK EditInsertTagDlgProc(HWND hwnd, UINT umsg, WPARAM wParam, LPARAM lParam) {
	switch (umsg) {
	case WM_INITDIALOG: {
		ResizeDlg_InitY2(hwnd, cxInsertTagDlg, cyInsertTagDlg, IDC_RESIZEGRIP2, IDC_MODIFY_LINE_PREFIX, IDC_MODIFY_LINE_APPEND);

		MultilineEditSetup(hwnd, IDC_MODIFY_LINE_PREFIX);
		SetDlgItemText(hwnd, IDC_MODIFY_LINE_PREFIX, L"<tag>");
		MultilineEditSetup(hwnd, IDC_MODIFY_LINE_APPEND);
		SetDlgItemText(hwnd, IDC_MODIFY_LINE_APPEND, L"</tag>");

		SetFocus(GetDlgItem(hwnd, IDC_MODIFY_LINE_PREFIX));
		PostMessage(GetDlgItem(hwnd, IDC_MODIFY_LINE_PREFIX), EM_SETSEL, 1, 4);
		CenterDlgInParent(hwnd);
	}
	return FALSE;

	case WM_DESTROY:
		ResizeDlg_Destroy(hwnd, &cxInsertTagDlg, &cyInsertTagDlg);
		return FALSE;

	case WM_SIZE: {
		int dx;
		int dy;

		ResizeDlg_Size(hwnd, lParam, &dx, &dy);
		const int cy = ResizeDlg_CalcDeltaY2(hwnd, dy, 75, IDC_MODIFY_LINE_PREFIX, IDC_MODIFY_LINE_APPEND);
		HDWP hdwp = BeginDeferWindowPos(6);
		hdwp = DeferCtlPos(hdwp, hwnd, IDC_RESIZEGRIP2, dx, dy, SWP_NOSIZE);
		hdwp = DeferCtlPos(hdwp, hwnd, IDOK, dx, dy, SWP_NOSIZE);
		hdwp = DeferCtlPos(hdwp, hwnd, IDCANCEL, dx, dy, SWP_NOSIZE);
		hdwp = DeferCtlPos(hdwp, hwnd, IDC_MODIFY_LINE_PREFIX, dx, cy, SWP_NOMOVE);
		hdwp = DeferCtlPos(hdwp, hwnd, IDC_MODIFY_LINE_APPEND, 0, cy, SWP_NOSIZE);
		hdwp = DeferCtlPos(hdwp, hwnd, IDC_MODIFY_LINE_TIP2, 0, cy, SWP_NOSIZE);
		EndDeferWindowPos(hdwp);
		ResizeDlgCtl(hwnd, IDC_MODIFY_LINE_APPEND, dx, dy - cy);
	}
	return TRUE;

	case WM_GETMINMAXINFO:
		ResizeDlg_GetMinMaxInfo(hwnd, lParam);
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_MODIFY_LINE_PREFIX: {
			if (HIWORD(wParam) == EN_CHANGE) {
				DString wszOpen = { NULL, 0 };
				BOOL bClear = TRUE;

				DString_GetDlgItemText(&wszOpen, hwnd, IDC_MODIFY_LINE_PREFIX);
				const int len = lstrlen(wszOpen.buffer);
				if (len >= 3) {
					LPCWSTR pwsz1 = StrChr(wszOpen.buffer, L'<');
					if (pwsz1 != NULL) {
						LPWSTR wchIns = (LPWSTR)NP2HeapAlloc((len + 5) * sizeof(WCHAR));
						lstrcpy(wchIns, L"</");
						int	cchIns = 2;
						const WCHAR *pwCur = pwsz1 + 1;

						while (*pwCur &&
								*pwCur != L'<' &&
								*pwCur != L'>' &&
								*pwCur != L' ' &&
								*pwCur != L'\t' &&
								*pwCur != L'\r' &&
								*pwCur != L'\n' &&
								(StrChr(L":_-.", *pwCur) || isalnum(*pwCur))) {
							wchIns[cchIns++] = *pwCur++;
						}

						while (*pwCur && *pwCur != L'>') {
							pwCur++;
						}

						if (*pwCur == L'>' && *(pwCur - 1) != L'/') {
							wchIns[cchIns++] = L'>';
							wchIns[cchIns] = L'\0';

							if (cchIns > 3 && !(
									StrCaseEqual(wchIns, L"</base>") &&
									StrCaseEqual(wchIns, L"</bgsound>") &&
									StrCaseEqual(wchIns, L"</br>") &&
									StrCaseEqual(wchIns, L"</embed>") &&
									StrCaseEqual(wchIns, L"</hr>") &&
									StrCaseEqual(wchIns, L"</img>") &&
									StrCaseEqual(wchIns, L"</input>") &&
									StrCaseEqual(wchIns, L"</link>") &&
									StrCaseEqual(wchIns, L"</meta>"))) {

								SetDlgItemText(hwnd, IDC_MODIFY_LINE_APPEND, wchIns);
								bClear = FALSE;
							}
						}
						NP2HeapFree(wchIns);
					}
				}

				if (bClear) {
					SetDlgItemText(hwnd, IDC_MODIFY_LINE_PREFIX, L"");
				}
				DString_Free(&wszOpen);
			}
		}
		break;

		case IDOK: {
			DString wszOpen = { NULL, 0 };
			DString wszClose = { NULL, 0 };
			DString_GetDlgItemText(&wszOpen, hwnd, IDC_MODIFY_LINE_PREFIX);
			DString_GetDlgItemText(&wszClose, hwnd, IDC_MODIFY_LINE_APPEND);

			EditEncloseSelection(hwndEdit, wszOpen.buffer, wszClose.buffer);
			DString_Free(&wszOpen);
			DString_Free(&wszClose);
			EndDialog(hwnd, IDOK);
		}
		break;

		case IDCANCEL:
			EndDialog(hwnd, IDCANCEL);
			break;
		}
		return TRUE;
	}

	return FALSE;
}

//=============================================================================
//
// EditInsertTagDlg()
//
void EditInsertTagDlg(HWND hwnd) {
	ThemedDialogBoxParam(g_hInstance, MAKEINTRESOURCE(IDD_INSERTTAG), hwnd, EditInsertTagDlgProc, 0);
}

typedef struct UnicodeControlCharacter {
	LPCSTR uccUTF8;
	LPCSTR representation;
} UnicodeControlCharacter;

// https://en.wikipedia.org/wiki/Unicode_control_characters
// scintilla/scripts/GenerateCharTable.py
static const UnicodeControlCharacter kUnicodeControlCharacterTable[] = {
	{ "\xe2\x80\x8e", "LRM" },	// U+200E	LRM		Left-to-right mark
	{ "\xe2\x80\x8f", "RLM" },	// U+200F	RLM		Right-to-left mark
	{ "\xe2\x80\x8d", "ZWJ" },	// U+200D	ZWJ		Zero width joiner
	{ "\xe2\x80\x8c", "ZWNJ" },	// U+200C	ZWNJ	Zero width non-joiner
	{ "\xe2\x80\xaa", "LRE" },	// U+202A	LRE		Start of left-to-right embedding
	{ "\xe2\x80\xab", "RLE" },	// U+202B	RLE		Start of right-to-left embedding
	{ "\xe2\x80\xad", "LRO" },	// U+202D	LRO		Start of left-to-right override
	{ "\xe2\x80\xae", "RLO" },	// U+202E	RLO		Start of right-to-left override
	{ "\xe2\x80\xac", "PDF" },	// U+202C	PDF		Pop directional formatting
	{ "\xe2\x81\xae", "NADS" },	// U+206E	NADS	National digit shapes substitution
	{ "\xe2\x81\xaf", "NODS" },	// U+206F	NODS	Nominal (European) digit shapes
	{ "\xe2\x81\xab", "ASS" },	// U+206B	ASS		Activate symmetric swapping
	{ "\xe2\x81\xaa", "ISS" },	// U+206A	ISS		Inhibit symmetric swapping
	{ "\xe2\x81\xad", "AAFS" },	// U+206D	AAFS	Activate Arabic form shaping
	{ "\xe2\x81\xac", "IAFS" },	// U+206C	IAFS	Inhibit Arabic form shaping
	// Scintilla built-in, Editor::SetRepresentations()
	{ "\x1e", NULL },			// U+001E	RS		Record Separator (Block separator)
	{ "\x1f", NULL },			// U+001F	US		Unit Separator (Segment separator)
	{ "\xe2\x80\xa8", NULL },	// U+2028	LS		Line Separator
	{ "\xe2\x80\xa9", NULL },	// U+2029	PS		Paragraph Separator
};

void EditInsertUnicodeControlCharacter(HWND hwnd, int menu) {
	menu = menu - IDM_INSERT_UNICODE_LRM;
	const UnicodeControlCharacter ucc = kUnicodeControlCharacterTable[menu];
	SendMessage(hwnd, SCI_REPLACESEL, 0, (LPARAM)ucc.uccUTF8);
}

void EditShowUnicodeControlCharacter(HWND hwnd, BOOL bShow) {
	for (UINT i = 0; i < (UINT)COUNTOF(kUnicodeControlCharacterTable); i++) {
		const UnicodeControlCharacter ucc = kUnicodeControlCharacterTable[i];
		if (StrIsEmptyA(ucc.representation)) {
			continue;
		}
		if (bShow) {
			SendMessage(hwnd, SCI_SETREPRESENTATION, (WPARAM)ucc.uccUTF8, (LPARAM)ucc.representation);
		} else {
			SendMessage(hwnd, SCI_CLEARREPRESENTATION, (WPARAM)ucc.uccUTF8, 0);
		}
	}
}

//=============================================================================
//
// EditSortDlgProc()
//
//
static INT_PTR CALLBACK EditSortDlgProc(HWND hwnd, UINT umsg, WPARAM wParam, LPARAM lParam) {
	switch (umsg) {
	case WM_INITDIALOG: {
		SetWindowLongPtr(hwnd, DWLP_USER, lParam);
		int *piSortFlags = (int *)lParam;
		const int iSortFlags = *piSortFlags;

		if (iSortFlags & SORT_DESCENDING) {
			CheckRadioButton(hwnd, IDC_SORT_ASC, IDC_SORT_SHUFFLE, IDC_SORT_DESC);
		} else if (iSortFlags & SORT_SHUFFLE) {
			CheckRadioButton(hwnd, IDC_SORT_ASC, IDC_SORT_SHUFFLE, IDC_SORT_SHUFFLE);
			EnableWindow(GetDlgItem(hwnd, IDC_SORT_MERGE_DUP), FALSE);
			EnableWindow(GetDlgItem(hwnd, IDC_SORT_REMOVE_DUP), FALSE);
			EnableWindow(GetDlgItem(hwnd, IDC_SORT_REMOVE_UNIQUE), FALSE);
			EnableWindow(GetDlgItem(hwnd, IDC_SORT_IGNORE_CASE), FALSE);
			EnableWindow(GetDlgItem(hwnd, IDC_SORT_LOGICAL_NUMBER), FALSE);
		} else {
			CheckRadioButton(hwnd, IDC_SORT_ASC, IDC_SORT_SHUFFLE, IDC_SORT_ASC);
		}

		if (iSortFlags & SORT_MERGEDUP) {
			CheckDlgButton(hwnd, IDC_SORT_MERGE_DUP, BST_CHECKED);
		}

		if (iSortFlags & SORT_UNIQDUP) {
			CheckDlgButton(hwnd, IDC_SORT_REMOVE_DUP, BST_CHECKED);
			EnableWindow(GetDlgItem(hwnd, IDC_SORT_MERGE_DUP), FALSE);
		}

		if (iSortFlags & SORT_UNIQUNIQ) {
			CheckDlgButton(hwnd, IDC_SORT_REMOVE_UNIQUE, BST_CHECKED);
		}

		if (iSortFlags & SORT_NOCASE) {
			CheckDlgButton(hwnd, IDC_SORT_IGNORE_CASE, BST_CHECKED);
		}

		if (iSortFlags & SORT_LOGICAL) {
			CheckDlgButton(hwnd, IDC_SORT_LOGICAL_NUMBER, BST_CHECKED);
		}

		if (SC_SEL_RECTANGLE != SendMessage(hwndEdit, SCI_GETSELECTIONMODE, 0, 0)) {
			*piSortFlags &= ~SORT_COLUMN;
			EnableWindow(GetDlgItem(hwnd, IDC_SORT_COLUMN), FALSE);
		} else {
			*piSortFlags |= SORT_COLUMN;
			CheckDlgButton(hwnd, IDC_SORT_COLUMN, BST_CHECKED);
		}
		CenterDlgInParent(hwnd);
	}
	return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK: {
			int *piSortFlags = (int *)GetWindowLongPtr(hwnd, DWLP_USER);
			int iSortFlags = 0;
			if (IsButtonChecked(hwnd, IDC_SORT_DESC)) {
				iSortFlags |= SORT_DESCENDING;
			}
			if (IsButtonChecked(hwnd, IDC_SORT_SHUFFLE)) {
				iSortFlags |= SORT_SHUFFLE;
			}
			if (IsButtonChecked(hwnd, IDC_SORT_MERGE_DUP)) {
				iSortFlags |= SORT_MERGEDUP;
			}
			if (IsButtonChecked(hwnd, IDC_SORT_REMOVE_DUP)) {
				iSortFlags |= SORT_UNIQDUP;
			}
			if (IsButtonChecked(hwnd, IDC_SORT_REMOVE_UNIQUE)) {
				iSortFlags |= SORT_UNIQUNIQ;
			}
			if (IsButtonChecked(hwnd, IDC_SORT_IGNORE_CASE)) {
				iSortFlags |= SORT_NOCASE;
			}
			if (IsButtonChecked(hwnd, IDC_SORT_LOGICAL_NUMBER)) {
				iSortFlags |= SORT_LOGICAL;
			}
			if (IsButtonChecked(hwnd, IDC_SORT_COLUMN)) {
				iSortFlags |= SORT_COLUMN;
			}
			*piSortFlags = iSortFlags;
			EndDialog(hwnd, IDOK);
		}
		break;

		case IDCANCEL:
			EndDialog(hwnd, IDCANCEL);
			break;

		case IDC_SORT_ASC:
		case IDC_SORT_DESC:
			EnableWindow(GetDlgItem(hwnd, IDC_SORT_MERGE_DUP), !IsButtonChecked(hwnd, IDC_SORT_REMOVE_UNIQUE));
			EnableWindow(GetDlgItem(hwnd, IDC_SORT_REMOVE_DUP), TRUE);
			EnableWindow(GetDlgItem(hwnd, IDC_SORT_REMOVE_UNIQUE), TRUE);
			EnableWindow(GetDlgItem(hwnd, IDC_SORT_IGNORE_CASE), TRUE);
			EnableWindow(GetDlgItem(hwnd, IDC_SORT_LOGICAL_NUMBER), TRUE);
			break;

		case IDC_SORT_SHUFFLE:
			EnableWindow(GetDlgItem(hwnd, IDC_SORT_MERGE_DUP), FALSE);
			EnableWindow(GetDlgItem(hwnd, IDC_SORT_REMOVE_DUP), FALSE);
			EnableWindow(GetDlgItem(hwnd, IDC_SORT_REMOVE_UNIQUE), FALSE);
			EnableWindow(GetDlgItem(hwnd, IDC_SORT_IGNORE_CASE), FALSE);
			EnableWindow(GetDlgItem(hwnd, IDC_SORT_LOGICAL_NUMBER), FALSE);
			break;

		case IDC_SORT_REMOVE_DUP:
			EnableWindow(GetDlgItem(hwnd, IDC_SORT_MERGE_DUP), !IsButtonChecked(hwnd, IDC_SORT_REMOVE_DUP));
			break;
		}
		return TRUE;
	}

	return FALSE;
}

//=============================================================================
//
// EditSortDlg()
//
BOOL EditSortDlg(HWND hwnd, int *piSortFlags) {
	const INT_PTR iResult = ThemedDialogBoxParam(g_hInstance, MAKEINTRESOURCE(IDD_SORT), hwnd, EditSortDlgProc, (LPARAM)piSortFlags);
	return iResult == IDOK;
}

void EditSelectionAction(HWND hwnd, int action) {
	static const LPCWSTR kActionKeys[] = {
		L"GoogleSearchUrl",
		L"BingSearchUrl",
		L"WikiSearchUrl",
		L"CustomAction1",
		L"CustomAction2",
	};

	WCHAR szCmdTemplate[256];
	action -= CMD_ONLINE_SEARCH_GOOGLE;
	LPCWSTR actionKey = kActionKeys[action];
	BOOL bCmdEnabled = IniGetString(INI_SECTION_NAME_FLAGS, actionKey, L"", szCmdTemplate, COUNTOF(szCmdTemplate));
	if (!bCmdEnabled && action < 3) {
		bCmdEnabled = GetString(IDS_GOOGLE_SEARCH_URL + action, szCmdTemplate, COUNTOF(szCmdTemplate));
	}
	if (!bCmdEnabled) {
		return;
	}

	int cchEscapedW;
	LPWSTR pszEscapedW = EditURLEncodeSelection(hwnd, &cchEscapedW, TRUE);
	if (pszEscapedW == NULL) {
		return;
	}

	LPWSTR lpszCommand = (LPWSTR)NP2HeapAlloc(sizeof(WCHAR) * (cchEscapedW + COUNTOF(szCmdTemplate) + MAX_PATH + 32));
	const size_t cbCommand = NP2HeapSize(lpszCommand);
	wsprintf(lpszCommand, szCmdTemplate, pszEscapedW);
	ExpandEnvironmentStringsEx(lpszCommand, (DWORD)(cbCommand / sizeof(WCHAR)));

	LPWSTR lpszArgs = (LPWSTR)NP2HeapAlloc(cbCommand);
	ExtractFirstArgument(lpszCommand, lpszCommand, lpszArgs);

	WCHAR wchDirectory[MAX_PATH] = L"";
	if (StrNotEmpty(szCurFile)) {
		lstrcpy(wchDirectory, szCurFile);
		PathRemoveFileSpec(wchDirectory);
	}

	SHELLEXECUTEINFO sei;
	ZeroMemory(&sei, sizeof(SHELLEXECUTEINFO));
	sei.cbSize = sizeof(SHELLEXECUTEINFO);
	sei.fMask = SEE_MASK_NOZONECHECKS;
	sei.hwnd = NULL;
	sei.lpVerb = NULL;
	sei.lpFile = lpszCommand;
	sei.lpParameters = lpszArgs;
	sei.lpDirectory = wchDirectory;
	sei.nShow = SW_SHOWNORMAL;

	ShellExecuteEx(&sei);

	NP2HeapFree(pszEscapedW);
	NP2HeapFree(lpszCommand);
	NP2HeapFree(lpszArgs);
}

void TryBrowseFile(HWND hwnd, LPCWSTR pszFile, BOOL bWarn) {
	WCHAR tchParam[MAX_PATH + 4] = L"";
	WCHAR tchExeFile[MAX_PATH + 4] = L"";
	WCHAR tchTemp[MAX_PATH + 4];

	if (IniGetString(INI_SECTION_NAME_FLAGS, L"filebrowser.exe", L"", tchTemp, COUNTOF(tchTemp))) {
		ExtractFirstArgument(tchTemp, tchExeFile, tchParam);
	}
	if (StrIsEmpty(tchExeFile)) {
		lstrcpy(tchExeFile, L"metapath.exe");
	}
	if (PathIsRelative(tchExeFile)) {
		GetModuleFileName(NULL, tchTemp, COUNTOF(tchTemp));
		PathRemoveFileSpec(tchTemp);
		PathAppend(tchTemp, tchExeFile);
		if (PathFileExists(tchTemp)) {
			lstrcpy(tchExeFile, tchTemp);
		}
	}

	if (StrNotEmpty(tchParam) && StrNotEmpty(pszFile)) {
		StrCatBuff(tchParam, L" ", COUNTOF(tchParam));
	}

	if (StrNotEmpty(pszFile)) {
		lstrcpy(tchTemp, pszFile);
		PathQuoteSpaces(tchTemp);
		StrCatBuff(tchParam, tchTemp, COUNTOF(tchParam));
	}

	SHELLEXECUTEINFO sei;
	ZeroMemory(&sei, sizeof(SHELLEXECUTEINFO));

	sei.cbSize = sizeof(SHELLEXECUTEINFO);
	sei.fMask = SEE_MASK_FLAG_NO_UI | SEE_MASK_NOZONECHECKS;
	sei.hwnd = hwnd;
	sei.lpVerb = NULL;
	sei.lpFile = tchExeFile;
	sei.lpParameters = tchParam;
	sei.lpDirectory = NULL;
	sei.nShow = SW_SHOWNORMAL;

	ShellExecuteEx(&sei);

	if ((INT_PTR)sei.hInstApp < 32) {
		if (bWarn) {
			if (MsgBox(MBYESNOWARN, IDS_ERR_BROWSE) == IDYES) {
				OpenHelpLink(hwnd, IDM_HELP_LATEST_RELEASE);
			}
		} else if (StrNotEmpty(pszFile)) {
			OpenContainingFolder(hwnd, pszFile, FALSE);
		}
	}
}

char* EditGetStringAroundCaret(HWND hwnd, LPCSTR delimiters) {
	const int iCurrentPos = (int)SendMessage(hwnd, SCI_GETCURRENTPOS, 0, 0);
	const int iLine = (int)SendMessage(hwnd, SCI_LINEFROMPOSITION, iCurrentPos, 0);
	int iLineStart = (int)SendMessage(hwnd, SCI_POSITIONFROMLINE, iLine, 0);
	int iLineEnd = (int)SendMessage(hwnd, SCI_GETLINEENDPOSITION, iLine, 0);
	if (iLineStart == iLineEnd) {
		// empty line
		return NULL;
	}

	struct Sci_TextToFind ft = { { iCurrentPos, 0 }, delimiters, { 0, 0 } };
	const int findFlag = SCFIND_REGEXP | SCFIND_POSIX;

	// forward
	if (iCurrentPos < iLineEnd) {
		ft.chrg.cpMax = iLineEnd;
		const int iPos = (int)SendMessage(hwnd, SCI_FINDTEXT, findFlag, (LPARAM)&ft);
		if (iPos >= 0) {
			iLineEnd = (int)SendMessage(hwnd, SCI_POSITIONBEFORE, ft.chrgText.cpMax ,0);
		}
	}

	// backword
	if (iCurrentPos > iLineStart) {
		ft.chrg.cpMax = iLineStart;
		const int iPos = (int)SendMessage(hwnd, SCI_FINDTEXT, findFlag, (LPARAM)&ft);
		if (iPos >= 0) {
			iLineStart = (int)SendMessage(hwnd, SCI_POSITIONAFTER, ft.chrgText.cpMin ,0);
		}
	}

	if (iLineStart >= iLineEnd) {
		return NULL;
	}

	char *mszSelection = (char *)NP2HeapAlloc(iLineEnd - iLineStart + 1);
	struct Sci_TextRange tr = { { iLineStart, iLineEnd }, mszSelection };
	SendMessage(hwnd, SCI_GETTEXTRANGE, 0, (LPARAM)&tr);

	return mszSelection;
}

extern BOOL bOpenFolderWithMetapath;

void EditOpenSelection(HWND hwnd, int type) {
	int cchSelection = (int)SendMessage(hwnd, SCI_GETSELTEXT, 0, 0);
	char *mszSelection = NULL;
	if (cchSelection > 1) {
		mszSelection = (char *)NP2HeapAlloc(cchSelection);
		SendMessage(hwnd, SCI_GETSELTEXT, 0, (LPARAM)mszSelection);
		char *lpsz = strpbrk(mszSelection, "\r\n\t");
		if (lpsz) {
			*lpsz = '\0';
		}
	} else {
		// string terminated by space or quotes
		mszSelection = EditGetStringAroundCaret(hwnd, "[\\s'`\"<>|*,;]");
	}

	if (mszSelection == NULL) {
		return;
	}
	/* remove quotes, spaces and some invalid filename characters (except '/', '\' and '?') */
	StrTrimA(mszSelection, " \t\r\n'`\"<>|:*,;");
	cchSelection = lstrlenA(mszSelection);
	if (cchSelection != 0) {
		LPWSTR wszSelection = (LPWSTR)NP2HeapAlloc((max_i(MAX_PATH, cchSelection) + 32) * sizeof(WCHAR));
		LPWSTR link = wszSelection + 16;

		const UINT cpEdit = (UINT)SendMessage(hwnd, SCI_GETCODEPAGE, 0, 0);
		MultiByteToWideChar(cpEdit, 0, mszSelection, -1, link, cchSelection);

		WCHAR path[MAX_PATH];
		WCHAR wchDirectory[MAX_PATH] = L"";
		DWORD dwAttributes = GetFileAttributes(link);
		if (dwAttributes == INVALID_FILE_ATTRIBUTES) {
			if (StrNotEmpty(szCurFile)) {
				lstrcpy(wchDirectory, szCurFile);
				PathRemoveFileSpec(wchDirectory);
				PathCombine(path, wchDirectory, link);
				dwAttributes = GetFileAttributes(path);
			}
			if (dwAttributes == INVALID_FILE_ATTRIBUTES && GetFullPathName(link, COUNTOF(path), path, NULL)) {
				dwAttributes = GetFileAttributes(path);
			}
			if (dwAttributes != INVALID_FILE_ATTRIBUTES) {
				lstrcpy(link, path);
			}
		} else if (GetFullPathName(link, COUNTOF(path), path, NULL)) {
			lstrcpy(link, path);
		}

		if (type == 4) { // containing folder
			if (dwAttributes == INVALID_FILE_ATTRIBUTES) {
				type = 0;
			}
		} else if (dwAttributes != INVALID_FILE_ATTRIBUTES) {
			if (dwAttributes & FILE_ATTRIBUTE_DIRECTORY) {
				type = 3;
			} else {
				const BOOL can = Style_CanOpenFile(link);
				// open supported file in a new window
				type = can ? 2 : 1;
			}
		} else if (StrChr(link, L':')) { // link
			// TODO: check scheme
			type = 1;
		} else if (StrChr(link, L'@')) { // email
			lstrcpy(wszSelection, L"mailto:");
			lstrcpy(wszSelection + CSTRLEN(L"mailto:"), link);
			type = 1;
			link = wszSelection;
		}

		switch (type) {
		case 1:
			ShellExecute(hwndMain, L"open", link, NULL, NULL, SW_SHOWNORMAL);
			break;

		case 2: {
			WCHAR szModuleName[MAX_PATH];
			GetModuleFileName(NULL, szModuleName, COUNTOF(szModuleName));

			lstrcpyn(wchDirectory, link, COUNTOF(wchDirectory));
			PathRemoveFileSpec(wchDirectory);
			PathQuoteSpaces(link);

			SHELLEXECUTEINFO sei;
			ZeroMemory(&sei, sizeof(SHELLEXECUTEINFO));
			sei.cbSize = sizeof(SHELLEXECUTEINFO);
			sei.fMask = SEE_MASK_NOZONECHECKS;
			sei.hwnd = hwndMain;
			sei.lpVerb = NULL;
			sei.lpFile = szModuleName;
			sei.lpParameters = link;
			sei.lpDirectory = wchDirectory;
			sei.nShow = SW_SHOWNORMAL;

			ShellExecuteEx(&sei);
		}
		break;

		case 3:
			if (bOpenFolderWithMetapath) {
				TryBrowseFile(hwndMain, link, FALSE);
			} else {
				OpenContainingFolder(hwndMain, link, FALSE);
			}
			break;

		case 4:
			OpenContainingFolder(hwndMain, link, TRUE);
			break;
		}

		NP2HeapFree(wszSelection);
	}

	NP2HeapFree(mszSelection);
}

//=============================================================================
//
// FileVars_Init()
//

extern BOOL bNoEncodingTags;
extern int fNoFileVariables;

BOOL FileVars_Init(LPCSTR lpData, DWORD cbData, LPFILEVARS lpfv) {
	ZeroMemory(lpfv, sizeof(FILEVARS));
	if ((fNoFileVariables && bNoEncodingTags) || !lpData || !cbData) {
		return TRUE;
	}

	char tch[512];
	lstrcpynA(tch, lpData, min_u(cbData + 1, COUNTOF(tch)));
	const BOOL utf8Sig = IsUTF8Signature(lpData);
	BOOL bDisableFileVariables = FALSE;

	if (!fNoFileVariables) {
		int i;
		if (FileVars_ParseInt(tch, "enable-local-variables", &i) && (!i)) {
			bDisableFileVariables = TRUE;
		}

		if (!bDisableFileVariables) {
			if (FileVars_ParseInt(tch, "tab-width", &i)) {
				lpfv->iTabWidth = clamp_i(i, 1, 256);
				lpfv->mask |= FV_TABWIDTH;
			}

			if (FileVars_ParseInt(tch, "c-basic-indent", &i)) {
				lpfv->iIndentWidth = clamp_i(i, 0, 256);
				lpfv->mask |= FV_INDENTWIDTH;
			}

			if (FileVars_ParseInt(tch, "indent-tabs-mode", &i)) {
				lpfv->bTabsAsSpaces = i == 0;
				lpfv->mask |= FV_TABSASSPACES;
			}

			if (FileVars_ParseInt(tch, "c-tab-always-indent", &i)) {
				lpfv->bTabIndents = i != 0;
				lpfv->mask |= FV_TABINDENTS;
			}

			if (FileVars_ParseInt(tch, "truncate-lines", &i)) {
				lpfv->fWordWrap = i == 0;
				lpfv->mask |= FV_WORDWRAP;
			}

			if (FileVars_ParseInt(tch, "fill-column", &i)) {
				lpfv->iLongLinesLimit = clamp_i(i, 0, NP2_LONG_LINE_LIMIT);
				lpfv->mask |= FV_LONGLINESLIMIT;
			}
		}
	}

	if (!utf8Sig && !bNoEncodingTags && !bDisableFileVariables) {
		if (FileVars_ParseStr(tch, "encoding", lpfv->tchEncoding, COUNTOF(lpfv->tchEncoding))) {
			lpfv->mask |= FV_ENCODING;
		} else if (FileVars_ParseStr(tch, "charset", lpfv->tchEncoding, COUNTOF(lpfv->tchEncoding))) {
			lpfv->mask |= FV_ENCODING;
		} else if (FileVars_ParseStr(tch, "coding", lpfv->tchEncoding, COUNTOF(lpfv->tchEncoding))) {
			lpfv->mask |= FV_ENCODING;
		}
	}

	if (!fNoFileVariables && !bDisableFileVariables) {
		if (FileVars_ParseStr(tch, "mode", lpfv->tchMode, COUNTOF(lpfv->tchMode))) {
			lpfv->mask |= FV_MODE;
		}
	}

	if (lpfv->mask == 0 && cbData > COUNTOF(tch)) {
		lstrcpynA(tch, lpData + cbData - COUNTOF(tch) + 1, COUNTOF(tch));
		if (!fNoFileVariables) {
			int i;
			if (FileVars_ParseInt(tch, "enable-local-variables", &i) && (!i)) {
				bDisableFileVariables = TRUE;
			}

			if (!bDisableFileVariables) {
				if (FileVars_ParseInt(tch, "tab-width", &i)) {
					lpfv->iTabWidth = clamp_i(i, 1, 256);
					lpfv->mask |= FV_TABWIDTH;
				}

				if (FileVars_ParseInt(tch, "c-basic-indent", &i)) {
					lpfv->iIndentWidth = clamp_i(i, 0, 256);
					lpfv->mask |= FV_INDENTWIDTH;
				}

				if (FileVars_ParseInt(tch, "indent-tabs-mode", &i)) {
					lpfv->bTabsAsSpaces = i == 0;
					lpfv->mask |= FV_TABSASSPACES;
				}

				if (FileVars_ParseInt(tch, "c-tab-always-indent", &i)) {
					lpfv->bTabIndents = i != 0;
					lpfv->mask |= FV_TABINDENTS;
				}

				if (FileVars_ParseInt(tch, "truncate-lines", &i)) {
					lpfv->fWordWrap = i == 0;
					lpfv->mask |= FV_WORDWRAP;
				}

				if (FileVars_ParseInt(tch, "fill-column", &i)) {
					lpfv->iLongLinesLimit = clamp_i(i, 0, NP2_LONG_LINE_LIMIT);
					lpfv->mask |= FV_LONGLINESLIMIT;
				}
			}
		}

		if (!utf8Sig && !bNoEncodingTags && !bDisableFileVariables) {
			if (FileVars_ParseStr(tch, "encoding", lpfv->tchEncoding, COUNTOF(lpfv->tchEncoding))) {
				lpfv->mask |= FV_ENCODING;
			} else if (FileVars_ParseStr(tch, "charset", lpfv->tchEncoding, COUNTOF(lpfv->tchEncoding))) {
				lpfv->mask |= FV_ENCODING;
			} else if (FileVars_ParseStr(tch, "coding", lpfv->tchEncoding, COUNTOF(lpfv->tchEncoding))) {
				lpfv->mask |= FV_ENCODING;
			}
		}

		if (!fNoFileVariables && !bDisableFileVariables) {
			if (FileVars_ParseStr(tch, "mode", lpfv->tchMode, COUNTOF(lpfv->tchMode))) {
				lpfv->mask |= FV_MODE;
			}
		}
	}

	if (lpfv->mask & FV_ENCODING) {
		lpfv->iEncoding = Encoding_MatchA(lpfv->tchEncoding);
	}

	return TRUE;
}

//=============================================================================
//
// FileVars_Apply()
//

extern BOOL fWordWrap;
extern BOOL fWordWrapG;
extern int iWordWrapMode;
extern int iLongLinesLimit;
extern int iLongLinesLimitG;
extern int iWrapCol;

BOOL FileVars_Apply(HWND hwnd, LPCFILEVARS lpfv) {
	if (lpfv->mask & FV_TABWIDTH) {
		iTabWidth = lpfv->iTabWidth;
	} else {
		iTabWidth = iTabWidthG;
	}
	SendMessage(hwnd, SCI_SETTABWIDTH, iTabWidth, 0);

	if (lpfv->mask & FV_INDENTWIDTH) {
		iIndentWidth = lpfv->iIndentWidth;
	} else if (lpfv->mask & FV_TABWIDTH) {
		iIndentWidth = 0;
	} else {
		iIndentWidth = iIndentWidthG;
	}
	SendMessage(hwnd, SCI_SETINDENT, iIndentWidth, 0);

	if (lpfv->mask & FV_TABSASSPACES) {
		bTabsAsSpaces = lpfv->bTabsAsSpaces;
	} else {
		bTabsAsSpaces = bTabsAsSpacesG;
	}
	SendMessage(hwnd, SCI_SETUSETABS, !bTabsAsSpaces, 0);

	if (lpfv->mask & FV_TABINDENTS) {
		bTabIndents = lpfv->bTabIndents;
	} else {
		bTabIndents = bTabIndentsG;
	}
	SendMessage(hwndEdit, SCI_SETTABINDENTS, bTabIndents, 0);

	if (lpfv->mask & FV_WORDWRAP) {
		fWordWrap = lpfv->fWordWrap;
	} else {
		fWordWrap = fWordWrapG;
	}

	SendMessage(hwndEdit, SCI_SETWRAPMODE, (fWordWrap? iWordWrapMode : SC_WRAP_NONE), 0);

	if (lpfv->mask & FV_LONGLINESLIMIT) {
		iLongLinesLimit = lpfv->iLongLinesLimit;
	} else {
		iLongLinesLimit = iLongLinesLimitG;
	}
	SendMessage(hwnd, SCI_SETEDGECOLUMN, iLongLinesLimit, 0);

	iWrapCol = 0;

	return TRUE;
}

//=============================================================================
//
// FileVars_ParseInt()
//
BOOL FileVars_ParseInt(LPCSTR pszData, LPCSTR pszName, int *piValue) {
	LPCSTR pvStart = pszData;

	while ((pvStart = StrStrA(pvStart, pszName)) != NULL) {
		const unsigned char chPrev = (pvStart > pszData) ? *(pvStart - 1) : 0;
		if (!isalpha(chPrev) && chPrev != '-' && chPrev != '_') {
			pvStart += lstrlenA(pszName);
			while (*pvStart == ' ') {
				pvStart++;
			}
			if (*pvStart == ':' || *pvStart == '=') {
				break;
			}
		} else {
			pvStart += lstrlenA(pszName);
		}
	}

	if (pvStart) {
		while (*pvStart && strchr(":=\"' \t", *pvStart)) {
			pvStart++;
		}

		char *pvEnd;
		*piValue = (int)strtol(pvStart, &pvEnd, 10);
		if (pvEnd != pvStart) {
			return TRUE;
		}

		switch (*pvStart) {
		case 't':
		case 'y':
			*piValue = 1;
			return TRUE;
		case 'f':
		case 'n':
			*piValue = 0;
			return FALSE;
		}
	}

	return FALSE;
}

//=============================================================================
//
// FileVars_ParseStr()
//
BOOL FileVars_ParseStr(LPCSTR pszData, LPCSTR pszName, char *pszValue, int cchValue) {
	LPCSTR pvStart = pszData;

	while ((pvStart = StrStrA(pvStart, pszName)) != NULL) {
		const unsigned char chPrev = (pvStart > pszData) ? *(pvStart - 1) : 0;
		if (!isalpha(chPrev) && chPrev != '-' && chPrev != '_') {
			pvStart += lstrlenA(pszName);
			while (*pvStart == ' ') {
				pvStart++;
			}
			if (*pvStart == ':' || *pvStart == '=') {
				break;
			}
		} else {
			pvStart += lstrlenA(pszName);
		}
	}

	if (pvStart) {
		BOOL bQuoted = FALSE;

		while (*pvStart && strchr(":=\"' \t", *pvStart)) {
			if (*pvStart == '\'' || *pvStart == '"') {
				bQuoted = TRUE;
			}
			pvStart++;
		}

		char tch[32];
		lstrcpynA(tch, pvStart, COUNTOF(tch));

		char *pvEnd = tch;
		while (*pvEnd && (isalnum((unsigned char)(*pvEnd)) || strchr("+-/_", *pvEnd) || (bQuoted && *pvEnd == ' '))) {
			pvEnd++;
		}
		*pvEnd = '\0';
		StrTrimA(tch, " \t:=\"'");

		lstrcpynA(pszValue, tch, cchValue);
		return TRUE;
	}

	return FALSE;
}

#if 0
//=============================================================================
//
// SciInitThemes()
//
extern BOOL bIsAppThemed;
static WNDPROC pfnSciWndProc = NULL;

static LRESULT CALLBACK SciThemedWndProc(HWND hwnd, UINT umsg, WPARAM wParam, LPARAM lParam);

void SciInitThemes(HWND hwnd) {
	pfnSciWndProc = (WNDPROC)SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)SciThemedWndProc);
}

//=============================================================================
//
// SciThemedWndProc()
//
LRESULT CALLBACK SciThemedWndProc(HWND hwnd, UINT umsg, WPARAM wParam, LPARAM lParam) {
	static RECT rcContent;

	switch (umsg) {
	case WM_NCCALCSIZE:
		if (wParam) {
			const LRESULT lresult = CallWindowProc(pfnSciWndProc, hwnd, WM_NCCALCSIZE, wParam, lParam);
			if (bIsAppThemed) {
				HTHEME hTheme = OpenThemeData(hwnd, L"edit");
				if (hTheme) {
					NCCALCSIZE_PARAMS *csp = (NCCALCSIZE_PARAMS *)lParam;
					BOOL bSuccess = FALSE;
					RECT rcClient;

					if (GetThemeBackgroundContentRect(hTheme, NULL, EP_EDITTEXT, ETS_NORMAL, &csp->rgrc[0], &rcClient) == S_OK) {
						InflateRect(&rcClient, -1, -1);

						rcContent.left = rcClient.left - csp->rgrc[0].left;
						rcContent.top = rcClient.top - csp->rgrc[0].top;
						rcContent.right = csp->rgrc[0].right - rcClient.right;
						rcContent.bottom = csp->rgrc[0].bottom - rcClient.bottom;

						CopyRect(&csp->rgrc[0], &rcClient);
						bSuccess = TRUE;
					}
					CloseThemeData(hTheme);

					if (bSuccess) {
						return WVR_REDRAW;
					}
				}
			}
			return lresult;
		}
		break;

	case WM_NCPAINT: {
		const LRESULT lresult = CallWindowProc(pfnSciWndProc, hwnd, WM_NCPAINT, wParam, lParam);
		if (bIsAppThemed) {
			HTHEME hTheme = OpenThemeData(hwnd, L"edit");
			if (hTheme) {
				HDC hdc = GetWindowDC(hwnd);

				RECT rcBorder;
				GetWindowRect(hwnd, &rcBorder);
				OffsetRect(&rcBorder, -rcBorder.left, -rcBorder.top);

				RECT rcClient;
				CopyRect(&rcClient, &rcBorder);
				rcClient.left += rcContent.left;
				rcClient.top += rcContent.top;
				rcClient.right -= rcContent.right;
				rcClient.bottom -= rcContent.bottom;

				ExcludeClipRect(hdc, rcClient.left, rcClient.top, rcClient.right, rcClient.bottom);

				if (IsThemeBackgroundPartiallyTransparent(hTheme, EP_EDITTEXT, ETS_NORMAL)) {
					DrawThemeParentBackground(hwnd, hdc, &rcBorder);
				}

				int nState;
				if (!IsWindowEnabled(hwnd)) {
					nState = ETS_DISABLED;
				} else if (GetFocus() == hwnd) {
					nState = ETS_FOCUSED;
				} else if (SendMessage(hwnd, SCI_GETREADONLY, 0, 0)) {
					nState = ETS_READONLY;
				} else {
					nState = ETS_NORMAL;
				}

				DrawThemeBackground(hTheme, hdc, EP_EDITTEXT, nState, &rcBorder, NULL);
				CloseThemeData(hTheme);

				ReleaseDC(hwnd, hdc);
				return 0;
			}
		}
		return lresult;
	}
	}

	return CallWindowProc(pfnSciWndProc, hwnd, umsg, wParam, lParam);
}

#endif

// End of Edit.c
