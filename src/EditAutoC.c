// Edit AutoCompletion

#include <windows.h>
#include <shlwapi.h>
#include <commctrl.h>
#include <commdlg.h>
#include <limits.h>
#include "Edit.h"
#include "Styles.h"
#include "Helpers.h"
#include "SciCall.h"
#include "resource.h"

static inline BOOL IsASpaceOrTab(int ch) {
	return ch == ' ' || ch == '\t';
}

static inline BOOL IsAAlpha(int ch) {
	return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z');
}

static inline BOOL IsEscapeChar(int ch) {
	return ch == 't' || ch == 'n' || ch == 'r' || ch == 'a' || ch == 'b' || ch == 'v' || ch == 'f'
		|| ch == '0'
		|| ch == '$'; // PHP
	// x u U
}

static inline BOOL IsCppCommentStyle(int style) {
	return style == SCE_C_COMMENT
		|| style == SCE_C_COMMENTLINE
		|| style == SCE_C_COMMENTDOC
		|| style == SCE_C_COMMENTLINEDOC
		|| style == SCE_C_COMMENTDOC_TAG
		|| style == SCE_C_COMMENTDOC_TAG_XML;
}

static inline BOOL IsCppStringStyle(int style) {
	return style == SCE_C_STRING
		|| style == SCE_C_CHARACTER
		|| style == SCE_C_STRINGEOL
		|| style == SCE_C_STRINGRAW
		|| style == SCE_C_VERBATIM
		|| style == SCE_C_DSTRINGX
		|| style == SCE_C_DSTRINGQ
		|| style == SCE_C_DSTRINGT;
}

static inline BOOL IsSpecialStartChar(int ch, int chPrev) {
	return (ch == '.')	// member
		   || (ch == '#')	// preprocessor
		   || (ch == '@') // Java/PHP/Doxygen Doc Tag
		   // ObjC Keyword, Java Annotation, Python Decorator, Cobra Directive
		   || (ch == '<') // HTML/XML Tag, C# Doc Tag
		   || (ch == '\\')// Doxygen Doc Tag, LaTeX Command
		   || (chPrev == '<' && ch == '/')	// HTML/XML Close Tag
		   || (chPrev == '-' && ch == '>')	// member(C/C++/PHP)
		   || (chPrev == ':' && ch == ':');// namespace(C++), static member(C++/Java8/PHP)
}


#include "EditAutoC_Data0.c"
#include "EditAutoC_WordList.c"

//=============================================================================
//
// EditCompleteWord()
// Auto-complete words
//
extern struct EditAutoCompletionConfig autoCompletionConfig;

// CharClassify::SetDefaultCharClasses()
static inline BOOL IsDefaultWordChar(int ch) {
	return ch >= 0x80 || isalnum(ch) || ch == '_';
}

BOOL IsDocWordChar(int ch) {
	if (isalnum(ch) || ch == '_' || ch == '.') {
		return TRUE;
	}

	switch (pLexCurrent->rid) {
	case NP2LEX_ANSI:
	case NP2LEX_CSS:
	case NP2LEX_DOT:
	case NP2LEX_LISP:
	case NP2LEX_SMALI:
		return (ch == '-');
	case NP2LEX_ASM:
	case NP2LEX_FORTRAN:
		return (ch == '#' || ch == '%');
	case NP2LEX_AU3:
		return (ch == '#' || ch == '$' || ch == '@');

	case NP2LEX_BASH:
	case NP2LEX_BATCH:
	case NP2LEX_HTML:
	case NP2LEX_PHP:
	case NP2LEX_PS1:
		return (ch == '-' || ch == '$');

	case NP2LEX_CIL:
	case NP2LEX_VERILOG:
		return (ch == '$');
	case NP2LEX_CPP:
		return (ch == '#' || ch == '@' || ch == ':');
	case NP2LEX_CSHARP:
		return (ch == '#' || ch == '@');

	case NP2LEX_D:
	case NP2LEX_FSHARP:
	case NP2LEX_HAXE:
	case NP2LEX_INNO:
	case NP2LEX_VB:
		return (ch == '#');

	case NP2LEX_JAVA:
		return (ch == '$' || ch == '@' || ch == ':');

	case NP2LEX_GROOVY:
	case NP2LEX_SCALA:
	case NP2LEX_PYTHON:
	case NP2LEX_PERL:
	case NP2LEX_RUBY:
	case NP2LEX_SQL:
	case NP2LEX_TCL:
		return (ch == '$' || ch == '@');

	case NP2LEX_LLVM:
		return (ch == '@' || ch == '%');

	case NP2LEX_MAKE:
	case NP2LEX_NSIS:
		return (ch == '-' || ch == '$' || ch == '!');

	case NP2LEX_XML:
		return (ch == '-' || ch == ':');
	}
	return FALSE;
}

BOOL IsAutoCompletionWordCharacter(int ch) {
	return (ch < 0x80) ? IsDocWordChar(ch) : SciCall_IsAutoCompletionWordCharacter(ch);
}

static inline BOOL IsOperatorStyle(int style) {
	switch (pLexCurrent->iLexer) {
	case SCLEX_ASM:
		return style == SCE_ASM_OPERATOR;
	case SCLEX_AU3:
		return style == SCE_AU3_OPERATOR;

	case SCLEX_BASH:
		return style == SCE_SH_OPERATOR;
	case SCLEX_BATCH:
		return style == SCE_BAT_OPERATOR;

	case SCLEX_CMAKE:
		return style == SCE_CMAKE_OPERATOR;
	case SCLEX_CONF:
		return style == SCE_CONF_OPERATOR;
	case SCLEX_CPP:
	case SCLEX_GRAPHVIZ:
	case SCLEX_JSON:
	case SCLEX_LISP:
	case SCLEX_LLVM:
	case SCLEX_NSIS:
	case SCLEX_VIM:
		return style == SCE_C_OPERATOR;
	case SCLEX_CSS:
		return style == SCE_CSS_OPERATOR;

	case SCLEX_FORTRAN:
		return style == SCE_F_OPERATOR;
	case SCLEX_FSHARP:
		return style == SCE_FSHARP_OPERATOR;

	case SCLEX_HTML:
		return style == SCE_HPHP_OPERATOR || style == SCE_HJ_SYMBOLS || style == SCE_HJA_SYMBOLS;

	case SCLEX_LATEX:
	case SCLEX_TEXINFO:
		return style == SCE_L_OPERATOR;
	case SCLEX_LUA:
		return style == SCE_LUA_OPERATOR;

	case SCLEX_MAKEFILE:
		return style == SCE_MAKE_OPERATOR;
	case SCLEX_MATLAB:
		return style == SCE_MAT_OPERATOR;

	case SCLEX_PASCAL:
		return style == SCE_PAS_OPERATOR;
	case SCLEX_PERL:
		return style == SCE_PL_OPERATOR;
	case SCLEX_POWERSHELL:
		return style == SCE_POWERSHELL_OPERATOR;
	case SCLEX_PYTHON:
		return style == SCE_PY_OPERATOR;

	case SCLEX_RUBY:
		return style == SCE_RB_OPERATOR;

	case SCLEX_SMALI:
		return style == SCE_SMALI_OPERATOR;
	case SCLEX_SQL:
		return style == SCE_SQL_OPERATOR;

	case SCLEX_TCL:
		return style == SCE_TCL_OPERATOR;

	case SCLEX_VB:
	case SCLEX_VBSCRIPT:
		return style == SCE_B_OPERATOR;
	case SCLEX_VERILOG:
		return style == SCE_V_OPERATOR;
	case SCLEX_VHDL:
		return style == SCE_VHDL_OPERATOR;
	}
	return FALSE;
}

static inline BOOL IsWordStyleToIgnore(int style) {
	switch (pLexCurrent->iLexer) {
	case SCLEX_CPP:
		return style == SCE_C_WORD
			|| style == SCE_C_WORD2
			|| style == SCE_C_PREPROCESSOR;

	case SCLEX_JSON:
		return style == SCE_C_WORD;

	case SCLEX_PYTHON:
		return style == SCE_PY_WORD
			|| style == SCE_PY_WORD2
			|| style == SCE_PY_BUILDIN_CONST
			|| style == SCE_PY_BUILDIN_FUNC
			|| style == SCE_PY_ATTR
			|| style == SCE_PY_OBJ_FUNC;

	case SCLEX_SMALI:
		return style == SCE_SMALI_WORD
			|| style == SCE_SMALI_DIRECTIVE
			|| style == SCE_SMALI_INSTRUCTION;
	case SCLEX_SQL:
		return style == SCE_SQL_WORD
			|| style == SCE_SQL_WORD2
			|| style == SCE_SQL_USER1
			|| style == SCE_SQL_HEX			// BLOB Hex
			|| style == SCE_SQL_HEX2;		// BLOB Hex
	}
	return FALSE;
}

// https://en.wikipedia.org/wiki/Printf_format_string
static inline BOOL IsStringFormatChar(int ch, int style) {
	if (!IsAAlpha(ch)) {
		return FALSE;
	}
	switch (pLexCurrent->iLexer) {
	case SCLEX_CPP:
		return style != SCE_C_OPERATOR;

	case SCLEX_FSHARP:
		return style != SCE_FSHARP_OPERATOR;

	case SCLEX_LUA:
		return style != SCE_LUA_OPERATOR;

	case SCLEX_MATLAB:
		return style != SCE_MAT_OPERATOR;

	case SCLEX_PERL:
		return style != SCE_PL_OPERATOR;
	case SCLEX_PYTHON:
		return style != SCE_PY_OPERATOR;

	case SCLEX_RUBY:
		return style != SCE_RB_OPERATOR;

	case SCLEX_TCL:
		return style != SCE_TCL_OPERATOR;
	}
	return FALSE;
}

static inline BOOL NeedSpaceAfterKeyword(const char *word, Sci_Position length) {
	const char *p = StrStrA(
		" if for try using while elseif switch foreach synchronized "
		, word);
	return p != NULL && p[-1] == ' ' && p[length] == ' ';
}

#define HTML_TEXT_BLOCK_TAG		0
#define HTML_TEXT_BLOCK_CDATA	1
#define HTML_TEXT_BLOCK_JS		2
#define HTML_TEXT_BLOCK_VBS		3
#define HTML_TEXT_BLOCK_PYTHON	4
#define HTML_TEXT_BLOCK_PHP		5
#define HTML_TEXT_BLOCK_CSS		6

static int GetCurrentHtmlTextBlock(HWND hwnd) {
	UNREFERENCED_PARAMETER(hwnd);

	const Sci_Position iCurrentPos = SciCall_GetCurrentPos();
	const int iCurrentStyle = SciCall_GetStyleAt(iCurrentPos);
	if (iCurrentStyle == SCE_H_CDATA) {
		return HTML_TEXT_BLOCK_CDATA;
	}
	if ((iCurrentStyle >= SCE_HJ_START && iCurrentStyle <= SCE_HJ_REGEX)
		|| (iCurrentStyle >= SCE_HJA_START && iCurrentStyle <= SCE_HJA_REGEX)) {
		return HTML_TEXT_BLOCK_JS;
	}
	if ((iCurrentStyle >= SCE_HB_START && iCurrentStyle <= SCE_HB_STRINGEOL)
		|| (iCurrentStyle >= SCE_HBA_START && iCurrentStyle <= SCE_HBA_STRINGEOL)) {
		return HTML_TEXT_BLOCK_VBS;
	}
	if ((iCurrentStyle >= SCE_HP_START && iCurrentStyle <= SCE_HP_IDENTIFIER)
		|| (iCurrentStyle >= SCE_HPA_START && iCurrentStyle <= SCE_HPA_IDENTIFIER)) {
		return HTML_TEXT_BLOCK_PYTHON;
	}
	if ((iCurrentStyle >= SCE_HPHP_DEFAULT && iCurrentStyle <= SCE_HPHP_OPERATOR)
		|| iCurrentStyle == SCE_HPHP_HEREDOC
		|| iCurrentStyle == SCE_HPHP_NOWDOC
		|| iCurrentStyle == SCE_HPHP_COMPLEX_VARIABLE) {
		return HTML_TEXT_BLOCK_PHP;
	}
	return HTML_TEXT_BLOCK_TAG;
}

static void EscapeRegex(LPSTR pszOut, LPCSTR pszIn) {
	char ch;
	while ((ch = *pszIn++) != '\0') {
		if (ch == '.'		// any character
			|| ch == '^'	// start of line
			|| ch == '$'	// end of line
			|| ch == '?'	// 0 or 1 times
			|| ch == '*'	// 0 or more times
			|| ch == '+'	// 1 or more times
			|| ch == '[' || ch == ']'
			|| ch == '(' || ch == ')'
			) {
			*pszOut++ = '\\';
		}
		*pszOut++ = ch;
	}
	*pszOut++ = '\0';
}

void AutoC_AddDocWord(struct WordList *pWList, BOOL bIgnoreCase, char prefix) {
	LPCSTR const pRoot = pWList->pWordStart;
	const int iRootLen = pWList->iStartLen;
	struct Sci_TextToFind ft = { { 0, 0 }, NULL, { 0, 0 } };
	struct Sci_TextRange tr = { { 0, 0 }, NULL };
	const Sci_Position iCurrentPos = SciCall_GetCurrentPos() - iRootLen - (prefix ? 1 : 0);
	const int iDocLen = SciCall_GetLength();
	const int findFlag = SCFIND_REGEXP | SCFIND_POSIX | (bIgnoreCase ? 0 : SCFIND_MATCHCASE);

	// optimization for small string
	char onStack[256];
	char *pFind;
	if (iRootLen * 2 + 32 < (int)sizeof(onStack)) {
		ZeroMemory(onStack, sizeof(onStack));
		pFind = onStack;
	} else {
		pFind = (char *)NP2HeapAlloc(iRootLen * 2 + 32);
	}

	if (prefix) {
		char buf[2] = { prefix, '\0' };
		EscapeRegex(pFind, buf);
	}
	if (iRootLen == 0) {
		// find an identifier
		strcat(pFind, "[A-Za-z0-9_]");
		strcat(pFind, "\\i?");
	} else {
		if (IsDefaultWordChar((unsigned char)pRoot[0])) {
			strcat(pFind, "\\h");
		}
		EscapeRegex(pFind + strlen(pFind), pRoot);
		if (IsDefaultWordChar((unsigned char)pRoot[iRootLen - 1])) {
			strcat(pFind, "\\i?");
		} else {
			strcat(pFind, "\\i");
		}
	}

	ft.lpstrText = pFind;
	ft.chrg.cpMax = iDocLen;
	Sci_Position iPosFind = SciCall_FindText(findFlag, &ft);

	while (iPosFind >= 0 && iPosFind < iDocLen) {
		Sci_Position wordEnd = iPosFind + iRootLen;
		const int style = SciCall_GetStyleAt(wordEnd - 1);
		wordEnd = ft.chrgText.cpMax;
		if (iPosFind != iCurrentPos && !IsWordStyleToIgnore(style)) {
			// find all word after '::', '->', '.' and '-'
			BOOL bSubWord = FALSE;
			while (wordEnd < iDocLen) {
				const int ch = SciCall_GetCharAt(wordEnd);
				if (!(ch == ':' || ch == '.' || ch == '-')) {
					break;
				}

				BOOL bFound = FALSE;
				Sci_Position width = 0;
				int chNext = SciCall_GetCharacterAndWidth(wordEnd + 1, &width);
				if ((ch == '-' && chNext == '>') || (ch == ':' && chNext == ':')) {
					chNext = SciCall_GetCharacterAndWidth(wordEnd + 2, &width);
					if (IsAutoCompletionWordCharacter(chNext)) {
						bFound = TRUE;
						wordEnd += 2;
					}
				} else if (ch == '.' || (ch == '-' && !IsOperatorStyle(SciCall_GetStyleAt(wordEnd)))) {
					if (IsAutoCompletionWordCharacter(chNext)) {
						bFound = TRUE;
						++wordEnd;
					}
				}
				if (!bFound) {
					break;
				}

				while (wordEnd < iDocLen && !IsDefaultWordChar(chNext)) {
					wordEnd += width;
					chNext = SciCall_GetCharacterAndWidth(wordEnd, &width);
					if (!IsAutoCompletionWordCharacter(chNext)) {
						break;
					}
				}

				wordEnd = SciCall_WordEndPosition(wordEnd, TRUE);
				bSubWord = TRUE;
			}

			Sci_Position wordLength = wordEnd - iPosFind;
			if (wordLength >= iRootLen) {
				char *pWord = pWList->wordBuf + DefaultAlignment;
				BOOL bChanged = FALSE;
				tr.lpstrText = pWord;
				tr.chrg.cpMin = (Sci_PositionCR)iPosFind;
				tr.chrg.cpMax = (Sci_PositionCR)((wordLength > NP2_AUTOC_MAX_WORD_LENGTH)? iPosFind + NP2_AUTOC_MAX_WORD_LENGTH : wordEnd);

				SciCall_GetTextRange(&tr);

				Sci_Position before = SciCall_PositionBefore(iPosFind);
				if (before + 1 == iPosFind) {
					const int ch = SciCall_GetCharAt(before);
					if (ch == '\\') { // word after escape char
						before = SciCall_PositionBefore(before);
						const int chPrev = (before + 2 == iPosFind) ? SciCall_GetCharAt(before) : 0;
						if (chPrev != '\\' && IsEscapeChar(*pWord)) {
							pWord++;
							--wordLength;
							bChanged = TRUE;
						}
					} else if (ch == '%') { // word after format char
						if (IsStringFormatChar(*pWord, SciCall_GetStyleAt(before))) {
							pWord++;
							--wordLength;
							bChanged = TRUE;
						}
					}
				}
				if (prefix && prefix == *pWord) {
					pWord++;
					--wordLength;
					bChanged = TRUE;
				}

				//if (pLexCurrent->rid == NP2LEX_PHP && wordLength >= 2 && *pWord == '$' && pWord[1] == '$') {
				//	pWord++;
				//	--wordLength;
				//	bChanged = TRUE;
				//}
				while (wordLength > 0 && (pWord[wordLength - 1] == '-' || pWord[wordLength - 1] == ':' || pWord[wordLength - 1] == '.')) {
					--wordLength;
					pWord[wordLength] = '\0';
				}
				if (bChanged) {
					CopyMemory(pWList->wordBuf, pWord, wordLength + 1);
					pWord = pWList->wordBuf;
				}

				bChanged = wordLength >= iRootLen && WordList_StartsWith(pWList, pWord);
				if (bChanged && !(pWord[0] == ':' && pWord[1] != ':')) {
					BOOL space = FALSE;
					if (!(pLexCurrent->iLexer == SCLEX_CPP && style == SCE_C_MACRO)) {
						while (IsASpaceOrTab(SciCall_GetCharAt(wordEnd))) {
							space = TRUE;
							wordEnd++;
						}
					}

					if (SciCall_GetCharAt(wordEnd) == '(') {
						if (space && NeedSpaceAfterKeyword(pWord, wordLength)) {
							pWord[wordLength++] = ' ';
						}

						pWord[wordLength++] = '(';
						pWord[wordLength++] = ')';
					}

					if (wordLength >= iRootLen) {
						pWord[wordLength] = '\0';
						WordList_AddWord(pWList, pWord, (int)wordLength);
						if (bSubWord) {
							for (int i = 0; i < (int)wordLength - 1; i++) {
								const char ch = pWord[i];
								if (ch == '.' || ch == '-' || ch == ':') {
									if (i >= iRootLen) {
										pWord[i] = '\0';
										WordList_AddWord(pWList, pWord, i);
										pWord[i] = ch;
									}
									if (ch != '.' && (pWord[i + 1] == '>' || pWord[i + 1] == ':')) {
										++i;
									}
								}
							}
						}
					}
				}
			}
		}

		ft.chrg.cpMin = (Sci_PositionCR)wordEnd;
		iPosFind = SciCall_FindText(findFlag, &ft);
	}

	if (pFind != onStack) {
		NP2HeapFree(pFind);
	}
}

void AutoC_AddKeyword(struct WordList *pWList, int iCurrentStyle) {
	for (int i = 0; i < NUMKEYWORD; i++) {
		const char *pKeywords = pLexCurrent->pKeyWords->pszKeyWords[i];
		if (StrNotEmptyA(pKeywords) && !(currentLexKeywordAttr[i] & KeywordAttr_NoAutoComp)) {
			WordList_AddListEx(pWList, pKeywords);
		}
	}

	// additional keywords
	if (np2_LexKeyword && !(pLexCurrent->iLexer == SCLEX_CPP && !IsCppCommentStyle(iCurrentStyle))) {
		WordList_AddList(pWList, (*np2_LexKeyword)[0]);
		WordList_AddList(pWList, (*np2_LexKeyword)[1]);
		WordList_AddList(pWList, (*np2_LexKeyword)[2]);
		WordList_AddList(pWList, (*np2_LexKeyword)[3]);
	}
}

#define AutoC_AddSpecWord_Finish	1
#define AutoC_AddSpecWord_Keyword	2
INT AutoC_AddSpecWord(struct WordList *pWList, int iCurrentStyle, int ch, int chPrev) {
	if (pLexCurrent->iLexer == SCLEX_CPP && IsCppCommentStyle(iCurrentStyle) && np2_LexKeyword) {
		if ((ch == '@' && (*np2_LexKeyword == kwJavaDoc || *np2_LexKeyword == kwPHPDoc || *np2_LexKeyword == kwDoxyDoc))
				|| (ch == '\\' && *np2_LexKeyword == kwDoxyDoc)
				|| ((ch == '<' || chPrev == '<') && *np2_LexKeyword == kwNETDoc)) {
			WordList_AddList(pWList, (*np2_LexKeyword)[0]);
			WordList_AddList(pWList, (*np2_LexKeyword)[1]);
			WordList_AddList(pWList, (*np2_LexKeyword)[2]);
			WordList_AddList(pWList, (*np2_LexKeyword)[3]);
			return AutoC_AddSpecWord_Finish;
		}
	} else if ((pLexCurrent->rid == NP2LEX_HTML || pLexCurrent->rid == NP2LEX_XML || pLexCurrent->rid == NP2LEX_CONF)
			   && ((ch == '<') || (chPrev == '<' && ch == '/'))) {
		if (pLexCurrent->rid == NP2LEX_HTML || pLexCurrent->rid == NP2LEX_CONF) { // HTML Tag
			WordList_AddList(pWList, pLexCurrent->pKeyWords->pszKeyWords[0]);// HTML Tag
		} else {
			WordList_AddList(pWList, pLexCurrent->pKeyWords->pszKeyWords[3]);// XML Tag
			if (np2_LexKeyword) { // XML Tag
				WordList_AddList(pWList, (*np2_LexKeyword)[0]);
			}
		}
		return AutoC_AddSpecWord_Keyword; // application defined tags
	} else if ((pLexCurrent->iLexer == SCLEX_CPP && iCurrentStyle == SCE_C_DEFAULT)
			   || (pLexCurrent->iLexer == SCLEX_PYTHON && iCurrentStyle == SCE_PY_DEFAULT)
			   || (pLexCurrent->rid == NP2LEX_VB && iCurrentStyle == SCE_B_DEFAULT)
			   || (pLexCurrent->iLexer == SCLEX_SMALI && iCurrentStyle == SCE_C_DEFAULT)) {
		if (ch == '#' && pLexCurrent->iLexer == SCLEX_CPP) { // #preprocessor
			const char *pKeywords = pLexCurrent->pKeyWords->pszKeyWords[2];
			if (StrNotEmptyA(pKeywords)) {
				WordList_AddListEx(pWList, pKeywords);
				return AutoC_AddSpecWord_Finish;
			}
		} else if (ch == '#' && pLexCurrent->rid == NP2LEX_VB) { // #preprocessor
			const char *pKeywords = pLexCurrent->pKeyWords->pszKeyWords[3];
			if (StrNotEmptyA(pKeywords)) {
				WordList_AddListEx(pWList, pKeywords);
				return AutoC_AddSpecWord_Finish;
			}
		} else if (ch == '@') { // @directive, @annotation, @decorator
			if (pLexCurrent->rid == NP2LEX_CSHARP) { // verbatim identifier
				//WordList_AddList(pWList, pLexCurrent->pKeyWords->pszKeyWords[0]);
				//WordList_AddList(pWList, pLexCurrent->pKeyWords->pszKeyWords[1]);
			} else {
				const char *pKeywords = pLexCurrent->pKeyWords->pszKeyWords[3];
				if (StrNotEmptyA(pKeywords)) {
					WordList_AddListEx(pWList, pKeywords);
					// user defined annotation
					return AutoC_AddSpecWord_Keyword;
				}
			}
		} else if (ch == '.' && pLexCurrent->iLexer == SCLEX_SMALI) {
			WordList_AddList(pWList, pLexCurrent->pKeyWords->pszKeyWords[9]);
			return AutoC_AddSpecWord_Finish;
		}
		//else if (chPrev == ':' && ch == ':') {
		//	WordList_AddList(pWList, "C++/namespace C++/Java8/PHP/static SendMessage()");
		//}
		//else if (chPrev == '-' && ch == '>') {
		//	WordList_AddList(pWList, "C/C++pointer PHP-variable");
		//}
	}
	return 0;
}

void EditCompleteUpdateConfig(void) {
	int i = 0;
	const int mask = autoCompletionConfig.fAutoCompleteFillUpMask;
	if (mask & AutoCompleteFillUpEnter) {
		autoCompletionConfig.szAutoCompleteFillUp[i++] = '\r';
		autoCompletionConfig.szAutoCompleteFillUp[i++] = '\n';
	}
	if (mask & AutoCompleteFillUpTab) {
		autoCompletionConfig.szAutoCompleteFillUp[i++] = '\t';
	}
	if (mask & AutoCompleteFillUpSpace) {
		autoCompletionConfig.szAutoCompleteFillUp[i++] = ' ';
	}

	const BOOL punctuation = mask & AutoCompleteFillUpPunctuation;
	int k = 0;
	for (UINT j = 0; j < (UINT)COUNTOF(autoCompletionConfig.wszAutoCompleteFillUp); j++) {
		const WCHAR c = autoCompletionConfig.wszAutoCompleteFillUp[j];
		if (c == L'\0') {
			break;
		}
		if (c < 0x80 && ispunct((unsigned char)c)) {
			autoCompletionConfig.wszAutoCompleteFillUp[k++] = c;
			if (punctuation) {
				autoCompletionConfig.szAutoCompleteFillUp[i++] = (char)c;
			}
		}
	}

	if (i == 0) {
		autoCompletionConfig.fAutoCompleteFillUpMask = AutoCompleteFillUpEnter;
		autoCompletionConfig.szAutoCompleteFillUp[i++] = '\r';
		autoCompletionConfig.szAutoCompleteFillUp[i++] = '\n';
	}
	autoCompletionConfig.szAutoCompleteFillUp[i] = '\0';
	autoCompletionConfig.wszAutoCompleteFillUp[k] = L'\0';
}

void EditCompleteWord(HWND hwnd, BOOL autoInsert) {
	const Sci_Position iCurrentPos = SciCall_GetCurrentPos();
	const int iCurrentStyle = SciCall_GetStyleAt(iCurrentPos);
	const int iLine = SciCall_LineFromPosition(iCurrentPos);
	const Sci_Position iLineStartPos = SciCall_PositionFromLine(iLine);

	autoCompletionConfig.iPreviousItemCount = 0; // recreate list

	// word before current position
	Sci_Position iStartWordPos = iCurrentPos;
	do {
		Sci_Position before = iStartWordPos;
		iStartWordPos = SciCall_WordStartPosition(before, TRUE);
		const BOOL nonWord = iStartWordPos == before;
		before = SciCall_PositionBefore(iStartWordPos);
		if (nonWord) {
			// non-word
			if (before + 1 != iStartWordPos) {
				break;
			}

			const int ch = SciCall_GetCharAt(before);
			if (!IsDocWordChar(ch) || IsSpecialStartChar(ch, '\0')) {
				break;
			}

			iStartWordPos = before;
		} else {
			const Sci_Position iPos = SciCall_WordEndPosition(before, TRUE);
			if (iPos == iStartWordPos) {
				// after CJK word
				break;
			}
		}
	} while (iStartWordPos > iLineStartPos);
	if (iStartWordPos == iCurrentPos) {
		return;
	}

	// beginning of word
	int ch = SciCall_GetCharAt(iStartWordPos);

	int chPrev = '\0';
	int chPrev2 = '\0';
	if (ch < 0x80 && iStartWordPos > iLineStartPos) {
		Sci_Position before = SciCall_PositionBefore(iStartWordPos);
		if (before + 1 == iStartWordPos) {
			chPrev = SciCall_GetCharAt(before);
			const Sci_Position before2 = SciCall_PositionBefore(before);
			if (before2 >= iLineStartPos && before2 + 1 == before) {
				chPrev2 = SciCall_GetCharAt(before2);
			}
			if ((chPrev == '\\' && chPrev2 != '\\' && IsEscapeChar(ch)) // word after escape char
				// word after format char
				|| (chPrev == '%' && IsStringFormatChar(ch, SciCall_GetStyleAt(before)))) {
				++iStartWordPos;
				ch = SciCall_GetCharAt(iStartWordPos);
				chPrev = '\0';
			}
		}
	}

	int iRootLen = autoCompletionConfig.iMinWordLength;
	if (ch >= '0' && ch <= '9') {
		if (autoCompletionConfig.iMinNumberLength <= 0) { // ignore number
			return;
		}

		iRootLen = autoCompletionConfig.iMinNumberLength;
		if (ch == '0') {
			// number prefix
			const int chNext = SciCall_GetCharAt(iStartWordPos + 1) | 0x20;
			if (chNext == 'x' || chNext == 'b' || chNext == 'o') {
				iRootLen += 2;
			}
		}
	}

	if (iCurrentPos - iStartWordPos < iRootLen) {
		return;
	}

	// preprocessor like: # space preprocessor
	if (pLexCurrent->rid == NP2LEX_CPP && (chPrev == '#' || IsASpaceOrTab(chPrev))) {
		Sci_Position before = iStartWordPos - 1;
		if (chPrev != '#') {
			while (before >= iLineStartPos) {
				chPrev = SciCall_GetCharAt(before);
				if (!IsASpaceOrTab(chPrev)) {
					break;
				}
				--before;
			}
		}
		if (chPrev == '#') {
			if (before > iLineStartPos) {
				--before;
				while (before >= iLineStartPos && IsASpaceOrTab(SciCall_GetCharAt(before))) {
					--before;
				}
				if (before >= iLineStartPos) {
					chPrev = '\0';
				}
			}
			ch = chPrev;
		}
		chPrev = '\0';
	} else if (IsSpecialStartChar(chPrev, chPrev2)) {
		ch = chPrev;
		chPrev = chPrev2;
	}

	// optimization for small string
	char onStack[128];
	char *pRoot;
	if (iCurrentPos - iStartWordPos + 1 < (Sci_Position)sizeof(onStack)) {
		ZeroMemory(onStack, sizeof(onStack));
		pRoot = onStack;
	} else {
		pRoot = (char *)NP2HeapAlloc(iCurrentPos - iStartWordPos + 1);
	}

	struct Sci_TextRange tr = { { 0, 0 }, NULL };
	tr.lpstrText = pRoot;
	tr.chrg.cpMin = (Sci_PositionCR)iStartWordPos;
	tr.chrg.cpMax = (Sci_PositionCR)iCurrentPos;
	SciCall_GetTextRange(&tr);
	iRootLen = lstrlenA(pRoot);

	BOOL bIgnore = iRootLen != 0 && (pRoot[0] >= '0' && pRoot[0] <= '9');
	struct WordList *pWList = WordList_Alloc(pRoot, iRootLen, bIgnore);
	BOOL bIgnoreDoc = FALSE;
	char prefix = '\0';

	if (!bIgnore && IsSpecialStartChar(ch, chPrev)) {
		const int result = AutoC_AddSpecWord(pWList, iCurrentStyle, ch, chPrev);
		if (result == AutoC_AddSpecWord_Finish) {
			bIgnore = TRUE;
			bIgnoreDoc = TRUE;
		} else if (result == AutoC_AddSpecWord_Keyword) {
			bIgnore = TRUE;
			// HTML/XML Tag
			if (ch == '/' || ch == '>') {
				ch = '<';
			}
			prefix = (char)ch;
		}
	}

	BOOL retry = FALSE;
	const BOOL bScanWordsInDocument = autoCompletionConfig.bScanWordsInDocument;
	do {
		if (!bIgnore) {
			// keywords
			AutoC_AddKeyword(pWList, iCurrentStyle);
		}
		if (bScanWordsInDocument) {
			if (!bIgnoreDoc || pWList->nWordCount == 0) {
				AutoC_AddDocWord(pWList, bIgnore, prefix);
			}
			if (prefix && pWList->nWordCount == 0) {
				prefix = '\0';
				AutoC_AddDocWord(pWList, bIgnore, prefix);
			}
		}

		retry = FALSE;
		if (pWList->nWordCount == 0 && iRootLen != 0) {
			const char *pSubRoot = strpbrk(pWList->pWordStart, ":.#@<\\/->$%");
			if (pSubRoot) {
				while (*pSubRoot && strchr(":.#@<\\/->$%", *pSubRoot)) {
					pSubRoot++;
				}
				if (*pSubRoot) {
					iRootLen = lstrlenA(pSubRoot);
					WordList_UpdateRoot(pWList, pSubRoot, iRootLen);
					retry = TRUE;
					bIgnore = FALSE;
					bIgnoreDoc = FALSE;
					prefix = '\0';
				}
			}
		}
	} while (retry);

	autoCompletionConfig.iPreviousItemCount = pWList->nWordCount;
	if (pWList->nWordCount > 0 && !(pWList->nWordCount == 1 && pWList->iMaxLength == iRootLen)) {
		char *pList = NULL;
		WordList_GetList(pWList, &pList);
		//DLog(pList);
		SendMessage(hwnd, SCI_AUTOCSETORDER, SC_ORDER_PRESORTED, 0); // pre-sorted
		SendMessage(hwnd, SCI_AUTOCSETIGNORECASE, 1, 0); // case insensitive
		SendMessage(hwnd, SCI_AUTOCSETSEPARATOR, '\n', 0);
		SendMessage(hwnd, SCI_AUTOCSETFILLUPS, 0, (LPARAM)autoCompletionConfig.szAutoCompleteFillUp);
		SendMessage(hwnd, SCI_AUTOCSETCHOOSESINGLE, 0, 0);
		//SendMessage(hwnd, SCI_AUTOCSETDROPRESTOFWORD, 1, 0); // delete orginal text: pRoot
		SendMessage(hwnd, SCI_AUTOCSETMAXWIDTH, (pWList->iMaxLength << 1), 0); // width columns, default auto
		SendMessage(hwnd, SCI_AUTOCSETMAXHEIGHT, min_i(pWList->nWordCount, autoCompletionConfig.iVisibleItemCount), 0); // height rows, default 5
		SendMessage(hwnd, SCI_AUTOCSETCHOOSESINGLE, autoInsert, 0);
		SendMessage(hwnd, SCI_AUTOCSHOW, pWList->iStartLen, (LPARAM)(pList));
		NP2HeapFree(pList);
	}

	if (pRoot != onStack) {
		NP2HeapFree(pRoot);
	}
	WordList_Free(pWList);
	NP2HeapFree(pWList);
}

static BOOL CanAutoCloseSingleQuote(int chPrev, int iCurrentStyle) {
	const int iLexer = pLexCurrent->iLexer;
	if (chPrev >= 0x80	// someone's
		|| (iLexer == SCLEX_CPP && iCurrentStyle == SCE_C_NUMBER)
		|| (iLexer == SCLEX_LISP && iCurrentStyle == SCE_C_OPERATOR)
		|| (iLexer == SCLEX_MATLAB && iCurrentStyle == SCE_MAT_OPERATOR) // transpose operator
		|| (iLexer == SCLEX_PERL && iCurrentStyle == SCE_PL_OPERATOR && chPrev == '&') // SCE_PL_IDENTIFIER
		|| ((iLexer == SCLEX_VB || iLexer == SCLEX_VBSCRIPT) && (iCurrentStyle == SCE_B_DEFAULT))
		|| (iLexer == SCLEX_VERILOG && (iCurrentStyle == SCE_V_NUMBER || iCurrentStyle == SCE_V_DEFAULT))
		|| (iLexer == SCLEX_TEXINFO && iCurrentStyle == SCE_L_DEFAULT && chPrev == '@') // SCE_L_SPECIAL
	) {
		return FALSE;
	}

	// someone's, don't
	if (isalnum(chPrev)) {
		// character prefix
		if (pLexCurrent->rid == NP2LEX_CPP || pLexCurrent->rid == NP2LEX_RC || iLexer == SCLEX_PYTHON || iLexer == SCLEX_SQL) {
			const int lower = chPrev | 0x20;
			const int chPrev2 = SciCall_GetCharAt(SciCall_GetCurrentPos() - 3);
			const BOOL bSubWord = chPrev2 >= 0x80 || isalnum(chPrev2);

			switch (iLexer) {
			case SCLEX_CPP:
				return (lower == 'u' || chPrev == 'L') && !bSubWord;

			case SCLEX_PYTHON: {
				const int lower2 = chPrev2 | 0x20;
				return (lower == 'r' || lower == 'u' || lower == 'b' || lower == 'f') && (!bSubWord || (lower != lower2 && (lower2 == 'r' || lower2 == 'u' || lower2 == 'b' || lower2 == 'f')));
			}

			case SCLEX_SQL:
				return (lower == 'q' || lower == 'x' || lower == 'b') && !bSubWord;
			}
		}
		return FALSE;
	}

	return TRUE;
}

void EditAutoCloseBraceQuote(HWND hwnd, int ch) {
	const Sci_Position iCurPos = SciCall_GetCurrentPos();
	const int chPrev = SciCall_GetCharAt(iCurPos - 2);
	const int chNext = SciCall_GetCharAt(iCurPos);
	const int iCurrentStyle = SciCall_GetStyleAt(iCurPos - 2);
	const int iNextStyle = SciCall_GetStyleAt(iCurPos);

	if (pLexCurrent->iLexer == SCLEX_CPP) {
		// within char
		if (iCurrentStyle == SCE_C_CHARACTER && iNextStyle == SCE_C_CHARACTER && pLexCurrent->rid != NP2LEX_PHP) {
			return;
		}
		if (ch == '`' && !IsCppStringStyle(iCurrentStyle)) {
			return;
		}
	}

	// escape sequence
	if (ch != ',' && (chPrev == '\\' || (pLexCurrent->iLexer == SCLEX_BATCH && chPrev == '^'))) {
		return;
	}

	const int mask = autoCompletionConfig.fAutoInsertMask;
	char tchIns[2] = "";
	switch (ch) {
	case '(':
		if (mask & AutoInsertParenthesis) {
			tchIns[0] = ')';
		}
		break;
	case '[':
		if ((mask & AutoInsertSquareBracket) && !(pLexCurrent->rid == NP2LEX_SMALI)) { // Smali array type
			tchIns[0] = ']';
		}
		break;
	case '{':
		if (mask & AutoInsertBrace) {
			tchIns[0] = '}';
		}
		break;
	case '<':
		if ((mask & AutoInsertAngleBracket) && (pLexCurrent->rid == NP2LEX_CPP || pLexCurrent->rid == NP2LEX_CSHARP || pLexCurrent->rid == NP2LEX_JAVA)) {
			// geriatric type, template
			if (iCurrentStyle == SCE_C_CLASS || iCurrentStyle == SCE_C_INTERFACE || iCurrentStyle ==  SCE_C_STRUCT) {
				tchIns[0] = '>';
			}
		}
		break;
	case '\"':
		if ((mask & AutoInsertDoubleQuote)) {
			tchIns[0] = '\"';
		}
		break;
	case '\'':
		if ((mask & AutoInsertSingleQuote) && CanAutoCloseSingleQuote(chPrev, iCurrentStyle)) {
			tchIns[0] = '\'';
		}
		break;
	case '`':
		//if (pLexCurrent->iLexer == SCLEX_BASH
		//|| pLexCurrent->rid == NP2LEX_JULIA
		//|| pLexCurrent->iLexer == SCLEX_MAKEFILE
		//|| pLexCurrent->iLexer == SCLEX_SQL
		//) {
		//	tchIns[0] = '`';
		//} else if (0) {
		//	tchIns[0] = '\'';
		//}
		if (mask & AutoInsertBacktick) {
			tchIns[0] = '`';
		}
		break;
	case ',':
		if ((mask & AutoInsertSpaceAfterComma) && !(chNext == ' ' || chNext == '\t' || (chPrev == '\'' && chNext == '\'') || (chPrev == '\"' && chNext == '\"'))) {
			tchIns[0] = ' ';
		}
		break;
	default:
		break;
	}
	if (tchIns[0]) {
		SendMessage(hwnd, SCI_BEGINUNDOACTION, 0, 0);
		SendMessage(hwnd, SCI_REPLACESEL, 0, (LPARAM)tchIns);
		const Sci_Position iCurrentPos = (ch == ',') ? iCurPos + 1 : iCurPos;
		SendMessage(hwnd, SCI_SETSEL, iCurrentPos, iCurrentPos);
		SendMessage(hwnd, SCI_ENDUNDOACTION, 0, 0);
	}
}

static inline BOOL IsHtmlVoidTag(const char *word, int length) {
	// see classifyTagHTML() in LexHTML.cxx
	const char *p = StrStrIA(
		// void elements
		" area base basefont br col command embed frame hr img input isindex keygen link meta param source track wbr "
		, word);
	return p != NULL && p[-1] == ' ' && p[length] == ' ';
}

void EditAutoCloseXMLTag(HWND hwnd) {
	char tchBuf[512];
	const Sci_Position iCurPos = SciCall_GetCurrentPos();
	int iHelper = (int)(iCurPos - (COUNTOF(tchBuf) - 1));
	const int iStartPos = max_i(0, iHelper);
	const Sci_Position iSize = iCurPos - iStartPos;
	BOOL autoClosed = FALSE;

	if (pLexCurrent->iLexer == SCLEX_CPP) {
		int iCurrentStyle = SciCall_GetStyleAt(iCurPos);
		if (iCurrentStyle == SCE_C_OPERATOR || iCurrentStyle == SCE_C_DEFAULT) {
			iHelper = FALSE;
		} else {
			const int iLine = SciCall_LineFromPosition(iCurPos);
			Sci_Position iCurrentLinePos = SciCall_PositionFromLine(iLine);
			while (iCurrentLinePos < iCurPos && IsASpaceOrTab(SciCall_GetCharAt(iCurrentLinePos))) {
				iCurrentLinePos++;
			}
			iCurrentStyle = SciCall_GetStyleAt(iCurrentLinePos);
			if (SciCall_GetCharAt(iCurrentLinePos) == '#' && iCurrentStyle == SCE_C_PREPROCESSOR) {
				iHelper = FALSE;
			}
		}
	}

	if (iSize >= 3 && iHelper) {
		struct Sci_TextRange tr;
		tr.chrg.cpMin = iStartPos;
		tr.chrg.cpMax = (Sci_PositionCR)iCurPos;
		tr.lpstrText = tchBuf;
		SendMessage(hwnd, SCI_GETTEXTRANGE, 0, (LPARAM)&tr);

		if (tchBuf[iSize - 2] != '/') {
			char tchIns[516] = "</";
			int cchIns = 2;
			const char *pBegin = tchBuf;
			const char *pCur = tchBuf + iSize - 2;

			while (pCur > pBegin && *pCur != '<' && *pCur != '>') {
				--pCur;
			}

			if (*pCur == '<') {
				pCur++;
				while (strchr(":_-.", *pCur) || isalnum((unsigned char)(*pCur))) {
					tchIns[cchIns++] = *pCur;
					pCur++;
				}
			}

			tchIns[cchIns++] = '>';
			tchIns[cchIns] = '\0';

			iHelper = cchIns > 3;
			if (iHelper && pLexCurrent->iLexer == SCLEX_HTML) {
				tchIns[cchIns - 1] = '\0';
				iHelper = !IsHtmlVoidTag(tchIns + 2, cchIns - 3);
			}
			if (iHelper) {
				tchIns[cchIns - 1] = '>';
				autoClosed = TRUE;
				SendMessage(hwnd, SCI_BEGINUNDOACTION, 0, 0);
				SendMessage(hwnd, SCI_REPLACESEL, 0, (LPARAM)tchIns);
				SendMessage(hwnd, SCI_SETSEL, iCurPos, iCurPos);
				SendMessage(hwnd, SCI_ENDUNDOACTION, 0, 0);
			}
		}
	}
	if (!autoClosed && autoCompletionConfig.bCompleteWord) {
		const Sci_Position iPos = SciCall_GetCurrentPos();
		if (SciCall_GetCharAt(iPos - 2) == '-') {
			EditCompleteWord(hwnd, FALSE); // obj->field, obj->method
		}
	}
}

BOOL IsIndentKeywordStyle(int style) {
	switch (pLexCurrent->iLexer) {
	//case SCLEX_AU3:
	//	return style == SCE_AU3_KEYWORD;
	case SCLEX_BASH:
		return style == SCE_BAT_WORD;

	case SCLEX_CMAKE:
		return style == SCE_CMAKE_WORD;
	//case SCLEX_CPP:
	//	return style == SCE_C_PREPROCESSOR;

	//case SCLEX_INNOSETUP:
	//	return style == SCE_INNO_KEYWORD_PASCAL;
	case SCLEX_LUA:
		return style == SCE_LUA_WORD;
	case SCLEX_MAKEFILE:
		return style == SCE_MAKE_PREPROCESSOR;
	case SCLEX_MATLAB:
		return style == SCE_MAT_KEYWORD;
	//case SCLEX_NSIS:
	//	return style == SCE_C_WORD || style == SCE_C_PREPROCESSOR;

	//case SCLEX_PASCAL:
	case SCLEX_RUBY:
		return style == SCE_RB_WORD;
	case SCLEX_SQL:
		return style == SCE_SQL_WORD;

	//case SCLEX_VB:
	//case SCLEX_VBSCRIPT:
	//	return style == SCE_B_KEYWORD;
	//case SCLEX_VERILOG:
	//	return style == SCE_V_WORD;
	//case SCLEX_VHDL:
	//	return style == SCE_VHDL_KEYWORD;
	}
	return FALSE;
}

const char *EditKeywordIndent(const char *head, int *indent) {
	char word[64] = "";
	char word_low[64] = "";
	int length = 0;
	const char *endPart = NULL;
	*indent = 0;

	while (*head && length < 63 && IsAAlpha(*head)) {
		word[length] = *head;
		word_low[length] = (char)((*head) | 0x20);
		++length;
		++head;
	}

	switch (pLexCurrent->iLexer) {
	//case SCLEX_AU3:
	case SCLEX_BASH:
		if (np2LexLangIndex == IDM_LANG_CSHELL) {
			if (!strcmp(word, "if")) {
				*indent = 2;
				endPart = "endif";
			} else if (!strcmp(word, "switch")) {
				*indent = 2;
				endPart = "endif";
			} else if (!strcmp(word, "foreach") || !strcmp(word, "while")) {
				*indent = 2;
				endPart = "end";
			}
		} else {
			if (!strcmp(word, "if")) {
				*indent = 2;
				endPart = "fi";
			} else if (!strcmp(word, "case")) {
				*indent = 2;
				endPart = "esac";
			} else if (!strcmp(word, "do")) {
				*indent = 2;
				endPart = "done";
			}
		}
		break;

	case SCLEX_CMAKE:
		if (!strcmp(word, "function")) {
			*indent = 2;
			endPart = "endfunction()";
		} else if (!strcmp(word, "macro")) {
			*indent = 2;
			endPart = "endmacro()";
		} else if (!strcmp(word, "if")) {
			*indent = 2;
			endPart = "endif()";
		} else if (!strcmp(word, "foreach")) {
			*indent = 2;
			endPart = "endforeach()";
		} else if (!strcmp(word, "while")) {
			*indent = 2;
			endPart = "endwhile()";
		}
		break;
	//case SCLEX_CPP:

	//case SCLEX_INNOSETUP:
	case SCLEX_LUA:
		if (!strcmp(word, "function") || !strcmp(word, "if") || !strcmp(word, "do")) {
			*indent = 2;
			endPart = "end";
		}
		break;

	case SCLEX_MAKEFILE:
		if (!strcmp(word, "if")) {
			*indent = 2;
			endPart = "endif";
		} else if (!strcmp(word, "define")) {
			*indent = 2;
			endPart = "endef";
		} else if (!strcmp(word, "for")) {
			*indent = 2;
			endPart = "endfor";
		}
		break;
	case SCLEX_MATLAB:
		if (!strcmp(word, "function")) {
			*indent = 1;
		} else if (!strcmp(word, "if") || !strcmp(word, "for") || !strcmp(word, "while") || !strcmp(word, "switch") || !strcmp(word, "try")) {
			*indent = 2;
			if (pLexCurrent->rid == NP2LEX_OCTAVE || np2LexLangIndex == IDM_LANG_OCTAVE) {
				if (strcmp(word, "if") == 0) {
					endPart = "endif";
				} else if (strcmp(word, "for") == 0) {
					endPart = "endfor";
				} else if (strcmp(word, "while") == 0) {
					endPart = "endwhile";
				} else if (strcmp(word, "switch") == 0) {
					endPart = "endswitch";
				} else if (strcmp(word, "try") == 0) {
					endPart = "end_try_catch";
				}
			}
			if (endPart == NULL) {
				endPart = "end";
			}
		}
		break;

	//case SCLEX_NSIS:
	//case SCLEX_PASCAL:
	case SCLEX_RUBY:
		if (!strcmp(word, "if") || !strcmp(word, "do") || !strcmp(word, "while") || !strcmp(word, "for")) {
			*indent = 2;
			endPart = "end";
		}
		break;

	case SCLEX_SQL:
		if (!strcmp(word_low, "if")) {
			*indent = 2;
			endPart = "END IF;";
		} else if (!strcmp(word_low, "while")) {
			*indent = 2;
			endPart = "END WHILE;";
		} else if (!strcmp(word_low, "repeat")) {
			*indent = 2;
			endPart = "END REPEAT;";
		} else if (!strcmp(word_low, "loop") || !strcmp(word_low, "for")) {
			*indent = 2;
			endPart = "END LOOP;";
		} else if (!strcmp(word_low, "case")) {
			*indent = 2;
			endPart = "END CASE;";
		} else if (!strcmp(word_low, "begin")) {
			*indent = 2;
			if (StrStrIA(head, "transaction") != NULL) {
				endPart = "COMMIT;";
			} else {
				endPart = "END";
			}
		} else if (!strcmp(word_low, "start")) {
			if (StrStrIA(head, "transaction") != NULL) {
				*indent = 2;
				endPart = "COMMIT;";
			}
		}
		break;

	//case SCLEX_VB:
	//case SCLEX_VBSCRIPT:
	//case SCLEX_VERILOG:
	//case SCLEX_VHDL:
	}
	return endPart;
}

extern BOOL	bTabsAsSpaces;
extern BOOL	bTabIndents;
extern int	iTabWidth;
extern int	iIndentWidth;

void EditAutoIndent(HWND hwnd) {
	int iCurPos = (int)SendMessage(hwnd, SCI_GETCURRENTPOS, 0, 0);
	//const int iAnchorPos = (int)SendMessage(hwnd, SCI_GETANCHOR, 0, 0);
	const int iCurLine = (int)SendMessage(hwnd, SCI_LINEFROMPOSITION, iCurPos, 0);
	//const int iLineLength = (int)SendMessage(hwnd, SCI_LINELENGTH, iCurLine, 0);
	//const int iIndentBefore = (int)SendMessage(hwnd, SCI_GETLINEINDENTATION, iCurLine - 1, 0);

	// Move bookmark along with line if inserting lines (pressing return at beginning of line) because Scintilla does not do this for us
	if (iCurLine > 0) {
		const int iPrevLineLength = (int)SendMessage(hwnd, SCI_GETLINEENDPOSITION, iCurLine - 1, 0) -
							  (int)SendMessage(hwnd, SCI_POSITIONFROMLINE, iCurLine - 1, 0);
		if (iPrevLineLength == 0) {
			const int bitmask = (int)SendMessage(hwnd, SCI_MARKERGET, iCurLine - 1, 0);
			if (bitmask & 1) {
				SendMessage(hwnd, SCI_MARKERDELETE, iCurLine - 1, 0);
				SendMessage(hwnd, SCI_MARKERADD, iCurLine, 0);
			}
		}
	}

	if (iCurLine > 0/* && iLineLength <= 2*/) {
		const int iPrevLineLength = (int)SendMessage(hwnd, SCI_LINELENGTH, iCurLine - 1, 0);
		if (iPrevLineLength < 2) {
			return;
		}
		char *pLineBuf = (char *)NP2HeapAlloc(2 * iPrevLineLength + 1 + iIndentWidth * 2 + 2 + 64);
		if (pLineBuf == NULL) {
			return;
		}

		const int iEOLMode = (int)SendMessage(hwnd, SCI_GETEOLMODE, 0, 0);
		int indent = 0;
		int	iIndentLen = 0;
		int commentStyle = 0;
		SciCall_GetLine(iCurLine - 1, pLineBuf);
		pLineBuf[iPrevLineLength] = '\0';

		int ch = pLineBuf[iPrevLineLength - 2];
		if (ch == '\r') {
			ch = pLineBuf[iPrevLineLength - 3];
			iIndentLen = 1;
		}
		if (ch == '{' || ch == '[' || ch == '(') {
			indent = 2;
		} else if (ch == ':') { // case label/Python
			indent = 1;
		} else if (ch == '*' || ch == '!') { // indent block comment
			iIndentLen = iPrevLineLength - (2 + iIndentLen);
			if (iIndentLen >= 2 && pLineBuf[iIndentLen - 2] == '/' && pLineBuf[iIndentLen - 1] == '*') {
				indent = 1;
				commentStyle = 1;
			}
		}
		iIndentLen = 0;
		ch = SciCall_GetCharAt(SciCall_PositionFromLine(iCurLine));
		if (indent == 2 && !(ch == '}' || ch == ']' || ch == ')')) {
			indent = 1;
		} else if (!indent && (ch == '}' || ch == ']' || ch == ')')) {
			indent = 1;
		}

		char *pPos;
		const char *endPart = NULL;
		for (pPos = pLineBuf; *pPos; pPos++) {
			if (*pPos != ' ' && *pPos != '\t') {
				if (!indent && IsAAlpha(*pPos)) { // indent on keywords
					const int style = SciCall_GetStyleAt(SciCall_PositionFromLine(iCurLine - 1) + iIndentLen);
					if (IsIndentKeywordStyle(style)) {
						endPart = EditKeywordIndent(pPos, &indent);
					}
				}
				if (indent) {
					memset(pPos, 0, iPrevLineLength - iIndentLen);
				}
				*pPos = '\0';
				break;
			}
			iIndentLen += 1;
		}

		int iIndentPos = iCurPos;
		if (indent) {
			int pad = iIndentWidth;
			iIndentPos += iIndentLen;
			ch = ' ';
			if (bTabIndents) {
				if (bTabsAsSpaces) {
					pad = iTabWidth;
					ch = ' ';
				} else {
					pad = 1;
					ch = '\t';
				}
			}
			if (commentStyle) {
				iIndentPos += 2;
				*pPos++ = ' ';
				*pPos++ = '*';
			} else {
				iIndentPos += pad;
				while (pad-- > 0) {
					*pPos++ = (char)ch;
				}
			}
			if (iEOLMode == SC_EOL_CRLF || iEOLMode == SC_EOL_CR) {
				*pPos++ = '\r';
			}
			if (iEOLMode == SC_EOL_CRLF || iEOLMode == SC_EOL_LF) {
				*pPos++ = '\n';
			}
			if (indent == 2) {
				lstrcpynA(pPos, pLineBuf, iIndentLen + 1);
				pPos += iIndentLen;
				if (endPart) {
					iIndentLen = lstrlenA(endPart);
					lstrcpynA(pPos, endPart, iIndentLen + 1);
					pPos += iIndentLen;
				}
			}
			*pPos = '\0';
		}

		if (*pLineBuf) {
			SendMessage(hwnd, SCI_BEGINUNDOACTION, 0, 0);
			SendMessage(hwnd, SCI_ADDTEXT, strlen(pLineBuf), (LPARAM)pLineBuf);
			if (indent) {
				if (indent == 1) {// remove new line
					iCurPos = iIndentPos + ((iEOLMode == SC_EOL_CRLF) ? 2 : 1);
					SendMessage(hwnd, SCI_SETSEL, iIndentPos, iCurPos);
					SendMessage(hwnd, SCI_REPLACESEL, 0, (LPARAM)"");
				}
				SendMessage(hwndEdit, SCI_SETSEL, iIndentPos, iIndentPos);
			}
			SendMessage(hwnd, SCI_ENDUNDOACTION, 0, 0);

			//const int iPrevLineStartPos = (int)SendMessage(hwnd, SCI_POSITIONFROMLINE, iCurLine - 1, 0);
			//const int iPrevLineEndPos = (int)SendMessage(hwnd, SCI_GETLINEENDPOSITION, iCurLine - 1, 0);
			//const int iPrevLineIndentPos = (int)SendMessage(hwnd, SCI_GETLINEINDENTPOSITION, iCurLine - 1, 0);

			//if (iPrevLineEndPos == iPrevLineIndentPos) {
			//	SendMessage(hwnd, SCI_BEGINUNDOACTION, 0, 0);
			//	SendMessage(hwnd, SCI_SETTARGETSTART, iPrevLineStartPos, 0);
			//	SendMessage(hwnd, SCI_SETTARGETEND, iPrevLineEndPos, 0);
			//	SendMessage(hwnd, SCI_REPLACETARGET, 0, (LPARAM)"");
			//	SendMessage(hwnd, SCI_ENDUNDOACTION, 0, 0);
			//}
		}

		NP2HeapFree(pLineBuf);
		//const int int iIndent = (int)SendMessage(hwnd, SCI_GETLINEINDENTATION, iCurLine, 0);
		//SendMessage(hwnd, SCI_SETLINEINDENTATION, iCurLine, iIndentBefore);
		//iIndentLen = /*- iIndent +*/ SendMessage(hwnd, SCI_GETLINEINDENTATION, iCurLine, 0);
		//if (iIndentLen > 0)
		//	SendMessage(hwnd, SCI_SETSEL, iAnchorPos + iIndentLen, iCurPos + iIndentLen);
	}
}

void EditToggleCommentLine(HWND hwnd) {
	switch (pLexCurrent->iLexer) {
	case SCLEX_ASM: {
		LPCWSTR ch;
		switch (autoCompletionConfig.iAsmLineCommentChar) {
		case AsmLineCommentCharSemicolon:
		default:
			ch = L";";
			break;
		case AsmLineCommentCharSharp:
			ch = L"# ";
			break;
		case AsmLineCommentCharSlash:
			ch = L"//";
			break;
		case AsmLineCommentCharAt:
			ch = L"@ ";
			break;
		}
		EditToggleLineComments(hwnd, ch, FALSE);
	} break;

	case SCLEX_AU3:
	case SCLEX_INNOSETUP:
	case SCLEX_LISP:
	case SCLEX_LLVM:
	case SCLEX_PROPERTIES:
		EditToggleLineComments(hwnd, L";", FALSE);
		break;

	case SCLEX_BASH:
		EditToggleLineComments(hwnd, ((np2LexLangIndex == IDM_LANG_M4)? L"dnl " : L"#"), FALSE);
		break;

	case SCLEX_BATCH:
		EditToggleLineComments(hwnd, L"@rem ", TRUE);
		break;

	case SCLEX_CIL:
	case SCLEX_CSS:
	case SCLEX_FSHARP:
	case SCLEX_GRAPHVIZ:
	case SCLEX_JSON:
	case SCLEX_PASCAL:
	case SCLEX_VERILOG:
		EditToggleLineComments(hwnd, L"//", FALSE);
		break;

	case SCLEX_CMAKE:
	case SCLEX_CONF:
	case SCLEX_PERL:
	case SCLEX_POWERSHELL:
	case SCLEX_TCL:
		EditToggleLineComments(hwnd, L"#", FALSE);
		break;

	case SCLEX_CPP:
		switch (pLexCurrent->rid) {
		case NP2LEX_AWK:
		case NP2LEX_JAM:
			EditToggleLineComments(hwnd, L"#", FALSE);
			break;
		default:
			EditToggleLineComments(hwnd, L"//", FALSE);
			break;
		}
		break;

	case SCLEX_FORTRAN:
		EditToggleLineComments(hwnd, L"!", FALSE);
		break;

	case SCLEX_HTML:
	case SCLEX_XML: {
		const int block = GetCurrentHtmlTextBlock(hwnd);
		switch (block) {
		case HTML_TEXT_BLOCK_VBS:
			EditToggleLineComments(hwnd, L"'", FALSE);
			break;

		case HTML_TEXT_BLOCK_PYTHON:
			EditToggleLineComments(hwnd, L"#", FALSE);
			break;

		case HTML_TEXT_BLOCK_CDATA:
		case HTML_TEXT_BLOCK_JS:
		case HTML_TEXT_BLOCK_PHP:
		case HTML_TEXT_BLOCK_CSS:
			EditToggleLineComments(hwnd, L"//", FALSE);
			break;
		}
	} break;

	case SCLEX_LATEX:
		EditToggleLineComments(hwnd, L"%", FALSE);
		break;

	case SCLEX_LUA:
	case SCLEX_VHDL:
		EditToggleLineComments(hwnd, L"--", FALSE);
		break;

	case SCLEX_MAKEFILE:
	case SCLEX_NSIS:
	case SCLEX_PYTHON:
	case SCLEX_RUBY:
	case SCLEX_SMALI:
		EditToggleLineComments(hwnd, L"#", FALSE);
		break;

	case SCLEX_MATLAB:
		if (pLexCurrent->rid == NP2LEX_JULIA) {
			EditToggleLineComments(hwnd, L"#", FALSE);
		} else if (pLexCurrent->rid == NP2LEX_SCILAB || np2LexLangIndex == IDM_LANG_SCILAB) {
			EditToggleLineComments(hwnd, L"//", FALSE);
		} else {
			EditToggleLineComments(hwnd, L"%", FALSE);
		}
		break;

	case SCLEX_SQL:
		EditToggleLineComments(hwnd, L"-- ", FALSE); // extra space
		break;

	case SCLEX_TEXINFO:
		EditToggleLineComments(hwnd, L"@c ", FALSE);
		break;

	case SCLEX_VB:
	case SCLEX_VBSCRIPT:
		EditToggleLineComments(hwnd, L"'", FALSE);
		break;

	case SCLEX_VIM:
		EditToggleLineComments(hwnd, L"\"", FALSE);
		break;
	}
}

void EditEncloseSelectionNewLine(HWND hwnd, LPCWSTR pwszOpen, LPCWSTR pwszClose) {
	WCHAR start[64] = L"";
	WCHAR end[64] = L"";
	const int iEOLMode = (int)SendMessage(hwnd, SCI_GETEOLMODE, 0, 0);
	LPCWSTR lineEnd = (iEOLMode == SC_EOL_LF) ? L"\n" : ((iEOLMode == SC_EOL_CR) ? L"\r" : L"\r\n");

	Sci_Position pos = SciCall_GetSelectionStart();
	int line = SciCall_LineFromPosition(pos);
	if (pos != SciCall_PositionFromLine(line)) {
		lstrcat(start, lineEnd);
	}
	lstrcat(start, pwszOpen);
	lstrcat(start, lineEnd);

	pos = SciCall_GetSelectionEnd();
	line = SciCall_LineFromPosition(pos);
	if (pos != SciCall_PositionFromLine(line)) {
		lstrcat(end, lineEnd);
	}
	lstrcat(end, pwszClose);
	lstrcat(end, lineEnd);
	EditEncloseSelection(hwnd, start, end);
}

void EditToggleCommentBlock(HWND hwnd) {
	switch (pLexCurrent->iLexer) {
	case SCLEX_ASM:
	case SCLEX_CIL:
	case SCLEX_CSS:
	case SCLEX_GRAPHVIZ:
	case SCLEX_JSON:
	case SCLEX_NSIS:
	case SCLEX_SQL:
	case SCLEX_VERILOG:
	case SCLEX_VHDL:
		EditEncloseSelection(hwnd, L"/*", L"*/");
		break;

	case SCLEX_AU3:
		EditEncloseSelectionNewLine(hwnd, L"#cs", L"#ce");
		break;

	case SCLEX_CPP:
		switch (pLexCurrent->rid) {
		case NP2LEX_AWK:
		case NP2LEX_JAM:
			break;

		default:
			EditEncloseSelection(hwnd, L"/*", L"*/");
			break;
		}
		break;

	case SCLEX_FORTRAN:
		EditEncloseSelectionNewLine(hwnd, L"#if 0", L"#endif");
		break;

	case SCLEX_FSHARP:
		EditEncloseSelection(hwnd, L"(*", L"*)");
		break;

	case SCLEX_HTML:
	case SCLEX_XML: {
		const int block = GetCurrentHtmlTextBlock(hwnd);
		switch (block) {
		case HTML_TEXT_BLOCK_TAG:
			EditEncloseSelection(hwnd, L"<!--", L"-->");
			break;

		case HTML_TEXT_BLOCK_VBS:
		case HTML_TEXT_BLOCK_PYTHON:
			break;

		case HTML_TEXT_BLOCK_CDATA:
		case HTML_TEXT_BLOCK_JS:
		case HTML_TEXT_BLOCK_PHP:
		case HTML_TEXT_BLOCK_CSS:
			EditEncloseSelection(hwnd, L"/*", L"*/");
			break;
		}
	} break;

	case SCLEX_INNOSETUP:
	case SCLEX_PASCAL:
		EditEncloseSelection(hwnd, L"{", L"}");
		break;

	case SCLEX_LATEX:
		EditEncloseSelectionNewLine(hwnd, L"\\begin{comment}", L"\\end{comment}");
		break;

	case SCLEX_LISP:
		EditEncloseSelection(hwnd, L"#|", L"|#");
		break;

	case SCLEX_LUA:
		EditEncloseSelection(hwnd, L"--[[", L"--]]");
		break;

	case SCLEX_MATLAB:
		if (pLexCurrent->rid == NP2LEX_JULIA) {
			EditEncloseSelectionNewLine(hwnd, L"#=", L"=#");
		} else if (pLexCurrent->rid == NP2LEX_SCILAB || np2LexLangIndex == IDM_LANG_SCILAB) {
			EditEncloseSelection(hwnd, L"/*", L"*/");
		} else {
			EditEncloseSelectionNewLine(hwnd, L"%{", L"%}");
		}
		break;

	case SCLEX_POWERSHELL:
		EditEncloseSelection(hwnd, L"<#", L"#>");
		break;

	case SCLEX_TCL:
		EditEncloseSelectionNewLine(hwnd, L"if (0) {", L"}");
		break;
	}
}

// see Style_SniffShebang() in Styles.c
void EditInsertScriptShebangLine(HWND hwnd) {
	const char *prefix = "#!/usr/bin/env ";
	const char *name = NULL;

	switch (pLexCurrent->iLexer) {
	case SCLEX_BASH:
		switch (np2LexLangIndex) {
		case IDM_LANG_CSHELL:
			prefix = "#!/bin/csh";
			break;

		case IDM_LANG_M4:
			name = "m4";
			break;

		default:
			prefix = "#!/bin/bash";
			break;
		}
		break;

	case SCLEX_CPP:
		switch (pLexCurrent->rid) {
		case NP2LEX_AWK:
			name = "awk";
			break;

		case NP2LEX_GO:
			name = "go";
			break;

		case NP2LEX_GROOVY:
			name = "groovy";
			break;

		case NP2LEX_JS:
			name = "node";
			break;

		case NP2LEX_PHP:
			name = "php";
			break;

		case NP2LEX_SCALA:
			name = "scala";
			break;
		}
		break;

	case SCLEX_LUA:
		name = "lua";
		break;

	case SCLEX_PERL:
		name = "perl";
		break;

	case SCLEX_PYTHON:
		name = "python";
		break;

	case SCLEX_RUBY:
		name = "ruby";
		break;

	case SCLEX_TCL:
		name = "wish";
		break;
	}

	char line[128];
	strcpy(line, prefix);
	if (name != NULL) {
		strcat(line, name);
	}

	const Sci_Position iCurrentPos = SciCall_GetCurrentPos();
	if (iCurrentPos == 0 && (name != NULL || pLexCurrent->iLexer == SCLEX_BASH)) {
		const int iEOLMode = (int)SendMessage(hwnd, SCI_GETEOLMODE, 0, 0);
		LPCSTR lineEnd = (iEOLMode == SC_EOL_LF) ? "\n" : ((iEOLMode == SC_EOL_CR) ? "\r" : "\r\n");
		strcat(line, lineEnd);
	}
	SendMessage(hwnd, SCI_REPLACESEL, 0, (LPARAM)line);
}

void EditShowCallTips(HWND hwnd, Sci_Position position) {
	const int iLine = (int)SendMessage(hwnd, SCI_LINEFROMPOSITION, position, 0);
	const int iDocLen = SciCall_GetLine(iLine, NULL); // get length
	char *pLine = (char *)NP2HeapAlloc(iDocLen + 1);
	SciCall_GetLine(iLine, pLine);
	char *text = (char *)NP2HeapAlloc(iDocLen + 1 + 128);
	wsprintfA(text, "ShowCallTips(%d, %d, %d)\n%s", iLine + 1, (int)position, iDocLen, pLine);
	SendMessage(hwnd, SCI_CALLTIPSHOW, position, (LPARAM)text);
	NP2HeapFree(pLine);
	NP2HeapFree(text);
}

// End of EditAutoC.c
