// Scintilla source code edit control
/** @file LexerSimple.h
 ** A simple lexer with no state.
 **/
// Copyright 1998-2010 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef LEXERSIMPLE_H
#define LEXERSIMPLE_H

namespace Scintilla {

// A simple lexer with no state
class LexerSimple : public LexerBase {
	const LexerModule *module;
public:
	explicit LexerSimple(const LexerModule *module_) noexcept;
	void SCI_METHOD Lex(Sci_PositionU startPos, Sci_Position lengthDoc, int initStyle, IDocument *pAccess) override;
	void SCI_METHOD Fold(Sci_PositionU startPos, Sci_Position lengthDoc, int initStyle, IDocument *pAccess) override;
};

}

#endif
