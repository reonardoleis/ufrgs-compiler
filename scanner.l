%{

#include "definitions.h"
#include "hash.h"
#include "symbols.h"
#include "ast.h"
#include "y.tab.h"

%}

%s MULTI_LINE_COMMENT

%%


<MULTI_LINE_COMMENT>\/\/\/          { BEGIN(0); }
<MULTI_LINE_COMMENT>[^\n\/]*              { /* ignore */ }
<MULTI_LINE_COMMENT>\n                   { if(yytext[0] == '\n') { increment_line_number(); } }
<MULTI_LINE_COMMENT>.                    { /* ignore */ }
(\\\\\\)            { BEGIN(MULTI_LINE_COMMENT); }

(\\\\)[^\\](.)*(\n)      { increment_line_number(); }



"char"        {  return KW_CHAR; }
"int"         { return KW_INT; }
"real"        {  return KW_REAL; }
"bool"        { return KW_BOOL; }
"if"          { return KW_IF; }
"else"        { return KW_ELSE; }
"loop"        { return KW_LOOP; }
"input"       { return KW_INPUT; }
"output"      { return KW_OUTPUT; }
"return"      { return KW_RETURN; }

"<="          {  return OPERATOR_LE; }
">="          {  return OPERATOR_GE; }
"=="          {  return OPERATOR_EQ; }
"!="          {  return OPERATOR_DIF; }

[0-9]+                  {  yylval.symbol = hash_insert(yytext, SYMBOL_LIT_INTEGER, DATATYPE_INT); return LIT_INT; }
\'[^\']\'                   {  yylval.symbol = hash_insert(yytext, SYMBOL_LIT_CHAR, DATATYPE_CHAR); return LIT_CHAR; }
[0-9]+\.([0-9]+)        {   yylval.symbol = hash_insert(yytext, SYMBOL_LIT_REAL, DATATYPE_REAL); return LIT_REAL; }
(\"([^\"\n]|\\\")*\")            {   yylval.symbol = hash_insert(yytext, SYMBOL_LIT_STRING, 0); return LIT_STRING; }

[a-zA-Z._]+ {   yylval.symbol = hash_insert(yytext, SYMBOL_IDENTIFIER, 0); return TK_IDENTIFIER; }
[,;()\[\]{}=+\-*/%<>&|~] {  return yytext[0]; }

[\n\t ]       { if(yytext[0] == '\n') { increment_line_number(); } }

.             { return TOKEN_ERROR; }






%%

#include "y.tab.c"
#include "main.c"