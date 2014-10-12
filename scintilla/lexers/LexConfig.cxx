// Lexer for Configuration Files.

#include <string.h>
#include <assert.h>
#include <ctype.h>

#include "ILexer.h"
#include "Scintilla.h"
#include "SciLexer.h"

#include "WordList.h"
#include "CharacterSet.h"
#include "LexAccessor.h"
#include "Accessor.h"
#include "LexerModule.h"

#ifdef SCI_NAMESPACE
using namespace Scintilla;
#endif

static inline bool IsConfOp(int ch) {
	return ch == '=' || ch == ':' || ch == ';' || ch == '{' || ch == '}' || ch == '(' || ch == ')' ||
		ch == '!' || ch == ',' || ch == '|' || ch == '*';
}
static inline bool IsUnit(int ch) {
	return ch == 'K' || ch == 'M' || ch == 'G' || ch == 'k' || ch == 'm' || ch == 'g';
}
static inline bool IsDelimiter(int ch) {
	return IsASpace(ch) || IsConfOp(ch);
}

static void ColouriseConfDoc(unsigned int startPos, int length, int initStyle, WordList *[], Accessor &styler) {
	int state = initStyle;
	int chPrev, ch = 0, chNext = styler[startPos];
	styler.StartAt(startPos);
	styler.StartSegment(startPos);
	unsigned int endPos = startPos + length;
	if (endPos == (unsigned)styler.Length())
		++endPos;

	int visibleChars = 0;
	bool insideTag = false;
	int lineCurrent = styler.GetLine(startPos);

	for (unsigned int i = startPos; i < endPos; i++) {
		chPrev = ch;
		ch = chNext;
		chNext = styler.SafeGetCharAt(i + 1);

		const bool atEOL = (ch == '\r' && chNext != '\n') || (ch == '\n');
		const bool atLineStart = i == (unsigned)styler.LineStart(lineCurrent);

		switch (state) {
		case SCE_CONF_OPERATOR:
			styler.ColourTo(i - 1, state);
			state = SCE_CONF_DEFAULT;
			break;
		case SCE_CONF_NUMBER:
			if (!(IsADigit(ch) || ch == '.')) {
				if (IsUnit(ch) && IsDelimiter(chNext)) {
					styler.ColourTo(i, state);
					state = SCE_CONF_DEFAULT;
					continue;
				} else if (iswordchar(ch)) {
					state = SCE_CONF_IDENTIFIER;
				} else {
					styler.ColourTo(i - 1, state);
					state = SCE_CONF_DEFAULT;
				}
			}
			break;
		case SCE_CONF_HEXNUM:
			if (!IsHexDigit(ch)) {
				styler.ColourTo(i - 1, state);
				state = SCE_CONF_DEFAULT;
			}
			break;
		case SCE_CONF_STRING:
			if (atLineStart) {
				styler.ColourTo(i - 1, state);
				state = SCE_CONF_DEFAULT;
			} else if (ch == '\\' && (chNext == '\\' || chNext == '\"')) {
				i++;
				ch = chNext;
				chNext = styler.SafeGetCharAt(i + 1);
			} else if (ch == '\"') {
				styler.ColourTo(i, state);
				state = SCE_CONF_DEFAULT;
				continue;
			}
			break;
		case SCE_CONF_DIRECTIVE:
			if (IsDelimiter(ch) || (insideTag && ch == '>')) {
				styler.ColourTo(i - 1, state);
				state = SCE_CONF_DEFAULT;
			}
			break;
		case SCE_CONF_SECTION:
		case SCE_CONF_COMMENT:
			if (atLineStart) {
				styler.ColourTo(i - 1, state);
				state = SCE_CONF_DEFAULT;
			}
			break;
		case SCE_CONF_IDENTIFIER:
			if (IsDelimiter(ch) || (insideTag && ch == '>')) {
				styler.ColourTo(i - 1, state);
				state = SCE_CONF_DEFAULT;
			}
			break;
		}

		if (state != SCE_CONF_COMMENT &&  ch == '\\' && (chNext == '\n' || chNext == '\r')) {
			i++;
			lineCurrent++;
			ch = chNext;
			chNext = styler.SafeGetCharAt(i + 1);
			if (ch == '\r' && chNext == '\n') {
				i++;
				ch = chNext;
				chNext = styler.SafeGetCharAt(i + 1);
			}
			continue;
		}

		if (state == SCE_CONF_DEFAULT) {
			if (ch == '#') {
				styler.ColourTo(i - 1, state);
				state = SCE_CONF_COMMENT;
			} else if (ch == '\"') {
				styler.ColourTo(i - 1, state);
				state = SCE_CONF_STRING;
			} else if (IsConfOp(ch)) {
				styler.ColourTo(i - 1, state);
				state = SCE_CONF_OPERATOR;
			} else if (visibleChars == 0 && !IsASpace(ch)) {
				styler.ColourTo(i - 1, state);
				if (ch == '[') {
					state = SCE_CONF_SECTION;
				} else {
					state = SCE_CONF_DIRECTIVE;
					insideTag = ch == '<';
				}
			} else if (insideTag && ch == '>') {
				styler.ColourTo(i - 1, state);
				styler.ColourTo(i, SCE_CONF_DIRECTIVE);
				state = SCE_CONF_DEFAULT;
				insideTag = false;
			} else if ((ch == '+' || ch == '-') && IsAlphaNumeric(chNext)) {
				styler.ColourTo(i - 1, state);
				state = SCE_CONF_OPERATOR;
			} else if (IsADigit(ch)) {
				styler.ColourTo(i - 1, state);
				if (ch == '0' && (chNext == 'x' || chNext == 'X')) {
					state = SCE_CONF_HEXNUM;
					i++;
					ch = chNext;
					chNext = styler.SafeGetCharAt(i + 1);
				} else {
					state = SCE_CONF_NUMBER;
				}
			} else if (!IsASpace(ch)) {
				styler.ColourTo(i - 1, state);
				state = SCE_CONF_IDENTIFIER;
			}
		}

		if (atEOL || i == endPos-1) {
			lineCurrent++;
			visibleChars = 0;
		}
		if (!isspacechar(ch)) {
			visibleChars++;
		}
	}

	// Colourise remaining document
	styler.ColourTo(endPos - 1, state);

}

#define IsCommentLine(line)		IsLexCommentLine(line, styler, SCE_CONF_COMMENT)

static void FoldConfDoc(unsigned int startPos, int length, int initStyle, WordList *[], Accessor &styler) {
	if (styler.GetPropertyInt("fold") == 0)
		return;
	const bool foldComment = styler.GetPropertyInt("fold.comment") != 0;
	const bool foldCompact = styler.GetPropertyInt("fold.compact", 1) != 0;

	unsigned int endPos = startPos + length;
	int visibleChars = 0;
	int lineCurrent = styler.GetLine(startPos);
	int levelCurrent = SC_FOLDLEVELBASE;
	if (lineCurrent > 0)
		levelCurrent = styler.LevelAt(lineCurrent-1) >> 16;
	int levelNext = levelCurrent;

	char chNext = styler[startPos];
	int styleNext = styler.StyleAt(startPos);
	int style = initStyle;

	for (unsigned int i = startPos; i < endPos; i++) {
		char ch = chNext;
		chNext = styler.SafeGetCharAt(i + 1);
		//int stylePrev = style;
		style = styleNext;
		styleNext = styler.StyleAt(i + 1);
		bool atEOL = (ch == '\r' && chNext != '\n') || (ch == '\n');

		if (foldComment && atEOL && IsCommentLine(lineCurrent)) {
			if (!IsCommentLine(lineCurrent - 1) && IsCommentLine(lineCurrent + 1))
				levelNext++;
			else if (IsCommentLine(lineCurrent - 1) && !IsCommentLine(lineCurrent + 1))
				levelNext--;
		}

		if (style == SCE_CONF_DIRECTIVE) {
			if ((ch == '<' && chNext != '/'))
				levelNext++;
			else if ((ch == '<' && chNext == '/') || (ch == '/' && chNext == '>'))
				levelNext--;
		}

		if (style == SCE_CONF_OPERATOR) {
			if (ch == '{')
				levelNext++;
			else if (ch == '}')
				levelNext--;
		}

		if (!isspacechar(ch))
			visibleChars++;

		if (atEOL || (i == endPos-1)) {
			int levelUse = levelCurrent;
			int lev = levelUse | levelNext << 16;
			if (visibleChars == 0 && foldCompact)
				lev |= SC_FOLDLEVELWHITEFLAG;
			if (levelUse < levelNext)
				lev |= SC_FOLDLEVELHEADERFLAG;
			if (lev != styler.LevelAt(lineCurrent)) {
				styler.SetLevel(lineCurrent, lev);
			}
			lineCurrent++;
			levelCurrent = levelNext;
			visibleChars = 0;
		}
	}
}

LexerModule lmConf(SCLEX_CONF, ColouriseConfDoc, "conf", FoldConfDoc);