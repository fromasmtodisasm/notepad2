// Scintilla source code edit control
/** @file StyleContext.cxx
 ** Lexer infrastructure.
 **/
// Copyright 1998-2004 by Neil Hodgson <neilh@scintilla.org>
// This file is in the public domain.

#include <cstdlib>
#include <cassert>

#include "ILexer.h"

#include "LexAccessor.h"
#include "Accessor.h"
#include "StyleContext.h"
#include "CharacterSet.h"

using namespace Scintilla;

void StyleContext::GetNextChar() noexcept {
	if (multiByteAccess) {
		chNext = multiByteAccess->GetCharacterAndWidth(currentPos + width, &widthNext);
	} else {
		chNext = static_cast<unsigned char>(styler.SafeGetCharAt(currentPos + width));
		widthNext = 1;
	}
	// End of line determined from line end position, allowing CR, LF,
	// CRLF and Unicode line ends as set by document.
	if (currentLine < lineDocEnd)
		atLineEnd = static_cast<Sci_Position>(currentPos) >= (lineStartNext - 1);
	else // Last line
		atLineEnd = static_cast<Sci_Position>(currentPos) >= lineStartNext;
}

StyleContext::StyleContext(Sci_PositionU startPos, Sci_PositionU length,
	int initStyle, LexAccessor &styler_, unsigned char chMask) noexcept :
	styler(styler_),
	multiByteAccess(nullptr),
	endPos(startPos + length),
	posRelative(0),
	currentPosLastRelative(LexAccessor::extremePosition),
	offsetRelative(0),
	currentPos(startPos),
	currentLine(-1),
	lineStartNext(-1),
	atLineEnd(false),
	state(initStyle & chMask), // Mask off all bits which aren't in the chMask.
	chPrev(0),
	ch(0),
	chNext(0),
	width(0),
	widthNext(1) {
	if (styler.Encoding() != enc8bit) {
		multiByteAccess = styler.MultiByteAccess();
	}
	styler.StartAt(startPos/*, chMask*/);
	styler.StartSegment(startPos);
	currentLine = styler.GetLine(startPos);
	lineStartNext = styler.LineStart(currentLine + 1);
	lengthDocument = static_cast<Sci_PositionU>(styler.Length());
	if (endPos == lengthDocument)
		endPos++;
	lineDocEnd = styler.GetLine(lengthDocument);
	atLineStart = static_cast<Sci_PositionU>(styler.LineStart(currentLine)) == startPos;

	// Variable width is now 0 so GetNextChar gets the char at currentPos into chNext/widthNext
	width = 0;
	GetNextChar();
	ch = chNext;
	width = widthNext;

	GetNextChar();
}

void StyleContext::Complete() {
	styler.ColourTo(currentPos - ((currentPos > lengthDocument) ? 2 : 1), state);
	styler.Flush();
}

void StyleContext::Forward() noexcept {
	if (currentPos < endPos) {
		atLineStart = atLineEnd;
		if (atLineStart) {
			currentLine++;
			lineStartNext = styler.LineStart(currentLine + 1);
		}
		chPrev = ch;
		currentPos += width;
		ch = chNext;
		width = widthNext;
		GetNextChar();
	} else {
		atLineStart = false;
		chPrev = ' ';
		ch = ' ';
		chNext = ' ';
		atLineEnd = true;
	}
}

void StyleContext::Forward(Sci_Position nb) noexcept {
	for (Sci_Position i = 0; i < nb; i++) {
		Forward();
	}
}

void StyleContext::ForwardBytes(Sci_Position nb) noexcept {
	const Sci_PositionU forwardPos = currentPos + nb;
	const Sci_PositionU currentPosStart = currentPos;
	while (forwardPos > currentPos) {
		Forward();
		if (currentPos == currentPosStart) {
			// Reached end
			return;
		}
	}
}

void StyleContext::SetState(int state_) {
	styler.ColourTo(currentPos - ((currentPos > lengthDocument) ? 2 : 1), state);
	state = state_;
}

void StyleContext::ForwardSetState(int state_) {
	Forward();
	styler.ColourTo(currentPos - ((currentPos > lengthDocument) ? 2 : 1), state);
	state = state_;
}

int StyleContext::GetRelativeCharacter(Sci_Position n) noexcept {
	if (n == 0)
		return ch;
	if (multiByteAccess) {
		if ((currentPosLastRelative != currentPos) ||
			((n > 0) && ((offsetRelative < 0) || (n < offsetRelative))) ||
			((n < 0) && ((offsetRelative > 0) || (n > offsetRelative)))) {
			posRelative = currentPos;
			offsetRelative = 0;
		}
		const Sci_Position diffRelative = n - offsetRelative;
		const Sci_Position posNew = multiByteAccess->GetRelativePosition(posRelative, diffRelative);
		const int chReturn = multiByteAccess->GetCharacterAndWidth(posNew, nullptr);
		posRelative = posNew;
		currentPosLastRelative = currentPos;
		offsetRelative = n;
		return chReturn;
	} else {
		// fast version for single byte encodings
		return static_cast<unsigned char>(styler.SafeGetCharAt(currentPos + n));
	}
}
