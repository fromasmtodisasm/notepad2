//@file HereDoc.h

#ifndef HEREDOC_H
#define HEREDOC_H

#ifdef SCI_NAMESPACE
namespace Scintilla {
#endif

#define HERE_DELIM_MAX 256		// maximum length of HERE doc delimiter
#define QUOTE_DELIM_STACK_MAX	7

// Class to manage HERE doc sequence
class HereDocCls {
public:
	int State;
	// 0: '<<' encountered
	// 1: collect the delimiter
	// 1b: text between the end of the delimiter and the EOL
	// 2: here doc text (lines after the delimiter)
	int Quote;		// the char after '<<'
	bool Quoted;	// true if Quote in ('\'','"','`')
	bool Indented;	// indented delimiter (for <<-)
	int DelimiterLength;	// strlen(Delimiter)
	char *Delimiter;		// the Delimiter, 256: sizeof PL_tokenbuf
	HereDocCls();
	void Append(int ch);
	~HereDocCls() {
		delete []Delimiter;
	}
};

// Class to manage quote pairs
class QuoteCls {
public:
	int Rep;
	int	Count;
	int Up, Down;
	QuoteCls() {
		this->New();
	}
	void New(int r = 1);
	void Open(int u);
	void Start(int u);
	QuoteCls(const QuoteCls& q);
	QuoteCls& operator=(const QuoteCls& q);
};

// Class to manage quote pairs that nest
class QuoteStackCls {
public:
	int Count;
	int Up, Down;
	int Style;
	int Depth;			// levels pushed
	int *CountStack;
	int *UpStack;
	int *StyleStack;
	QuoteStackCls();
	void Start(int u, int s);
	void Push(int u, int s);
	void Pop(void);
	~QuoteStackCls() {
		delete[] CountStack;
		delete[] UpStack;
		delete[] StyleStack;
	}
};


#ifdef SCI_NAMESPACE
}
#endif

#endif