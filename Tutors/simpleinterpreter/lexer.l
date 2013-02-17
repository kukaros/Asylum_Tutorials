%option noyywrap
%option yylineno
%option never-interactive

%{

#include "parser.hpp"
#include "interpreter.h"

#define YY_DECL int yyflex YY_PROTO(( void ))

#ifdef _MSC_VER
#   pragma warning(push)
#   pragma warning(disable : 4102)
#   pragma warning(disable : 4273)
#   pragma warning(disable : 4065)
#   pragma warning(disable : 4267)
#   pragma warning(disable : 4244)
#   pragma warning(disable : 4996)
#endif

%}

WHITESPACE     [ \t\n\r]
IDSTART        [a-zA-Z_]
IDCHAR         [a-zA-Z_0-9]
NUMBER         [0-9]|[1-9]([0-9]+)

%x str

%%

{WHITESPACE}+          { lexer_out("WHITESPACE"); }
"//"(.*)               { lexer_out("COMMENT"); }

"print"                { lexer_out("PRINT");                        return PRINT; }
"int"                  { lexer_out("INT");                          return INT; }

"("                    { lexer_out("LRB");                          return LRB; }
")"                    { lexer_out("RRB");                          return RRB; }
"{"                    { lexer_out("LB");                           return LB; }
"}"                    { lexer_out("RB");                           return RB; }
"["                    { lexer_out("LSB");                          return LSB; }
"]"                    { lexer_out("RSB");                          return RSB; }
";"                    { lexer_out("SEMICOLON");                    return SEMICOLON; }
"="                    { lexer_out("EQ");                           return EQ; }
"\""                   { lexer_out("QUOTE");       BEGIN(str);      return QUOTE; }
","                    { lexer_out("COMMA");                        return COMMA; }

{IDSTART}({IDCHAR}*)   { lexer_out("IDENTIFIER");                   return IDENTIFIER; }
{NUMBER}               { lexer_out("NUMBER");                       return NUMBER; }

<str>"\""              { lexer_out("QUOTE");       BEGIN(INITIAL);  return QUOTE; }
<str>[^\"]+            { lexer_out("STRING");                       return STRING; }

<<EOF>>                { return 0; }
.                      { lexer_out("ln " << yylineno << ": lexical error");  return 0; }

%%

#ifdef _MSC_VER
#    pragma warning(pop)
#endif
