#include "EditLexer.h"
#include "EditStyle.h"

static KEYWORDLIST Keywords_Verilog = {{
"always and assign attribute begin buf bufif0 bufif1 case casex casez cmos deassign default defparam disable edge "
" else end endattribute endcase endfunction endmodule endprimitive endspecify endtable endtask event for force "
" forever fork function highz0 highz1 if ifnone initial inout input integer join medium module large localparam "
" macromodule nand negedge nmos nor not notif0 notif1 or output parameter pmos posedge primitive pull0 pull1 pulldown "
" pullup rcmos real realtime reg release repeat rnmos rpmos rtran rtranif0 rtranif1 scalared signed small specify "
" specparam strength strong0 strong1 supply0 supply1 table task time tran tranif0 tranif1 tri tri0 tri1 triand trior "
" trireg unsigned vectored wait wand weak0 weak1 while wire wor xnor xor"

, // 1 System Task and Function
"readmemb readmemh sreadmemb sreadmemh display write strobe monitor fdisplay fwrite fstrobe fmonitor "
" fopen fclose time stime realtime scale printtimescale timeformat stop finish save incsave restart "
" input log nolog key nokey scope showscopes showscopes showvars showvars countdrivers list monitoron "
" monitoroff dumpon dumpoff dumpfile dumplimit dumpflush dumpvars dumpall reset reset reset reset reset "
" random getpattern rtoi itor realtobits bitstoreal setup hold setuphold period width skew recovery"

, NULL, NULL, NULL, NULL, NULL, NULL, NULL

, NULL, NULL, NULL, NULL, NULL, NULL, NULL
}};

static EDITSTYLE Styles_Verilog[] = {
	EDITSTYLE_DEFAULT,
	{ SCE_V_WORD, NP2STYLE_Keyword, EDITSTYLE_HOLE(L"Keyword"), L"fore:#0000FF" },
	{ SCE_V_WORD2, 63586, EDITSTYLE_HOLE(L"System Task and Function"), L"fore:#0080FF" },
	{ MULTI_STYLE(SCE_V_COMMENT, SCE_V_COMMENTLINE, SCE_V_COMMENTLINEBANG, 0), NP2STYLE_Comment, EDITSTYLE_HOLE(L"Comment"), L"fore:#608060" },
	{ SCE_V_STRING, NP2STYLE_String, EDITSTYLE_HOLE(L"String"), L"fore:#008000" },
	{ SCE_V_NUMBER, NP2STYLE_Number, EDITSTYLE_HOLE(L"Number"), L"fore:#FF0000" },
	{ SCE_V_OPERATOR, NP2STYLE_Operator, EDITSTYLE_HOLE(L"Operator"), L"fore:#B000B0" },
	{ SCE_V_PREPROCESSOR, NP2STYLE_Preprocessor, EDITSTYLE_HOLE(L"Preprocessor"), L"fore:#FF8000" },
	{ SCE_V_USER, 63585, EDITSTYLE_HOLE(L"User Define"), L"fore:#B5E71F" },
};

EDITLEXER lexVerilog = {
	SCLEX_VERILOG, NP2LEX_VERILOG,
	EDITLEXER_HOLE(L"Verilog HDL Source", Styles_Verilog),
	L"v; vl",
	&Keywords_Verilog,
	Styles_Verilog
};

