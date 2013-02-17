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

WHITESPACE     [ \t]
IDSTART        [a-zA-Z_]
IDCHAR         [a-zA-Z_0-9]
NUMBER         [0-9]|[1-9]([0-9]+)

%x lex_str

%%

"\r\n"                 { ++yylloc.first_line; } // win
"\n"                   { ++yylloc.first_line; } // unix

{WHITESPACE}+          { lexer_out("WHITESPACE"); }
"//"(.*)               { lexer_out("COMMENT"); }

"int"                  { lexer_out("INT");                          return INT; }
"void"                 { lexer_out("VOID");                         return VOID; }

"print"                { lexer_out("PRINT");                        return PRINT; }
"if"                   { lexer_out("IF");                           return IF; }
"else"                 { lexer_out("ELSE");                         return ELSE; }
"while"                { lexer_out("WHILE");                        return WHILE; }
"return"               { lexer_out("RETURN");                       return RETURN; }

"="                    { lexer_out("EQ");                           return EQ; }
"+="                   { lexer_out("PEQ");                          return PEQ; }
"-="                   { lexer_out("MEQ");                          return MEQ; }
"*="                   { lexer_out("SEQ");                          return SEQ; }
"/="                   { lexer_out("DEQ");                          return DEQ; }
"%="                   { lexer_out("OEQ");                          return OEQ; }

"||"                   { lexer_out("OR");                           return OR; }
"&&"                   { lexer_out("AND");                          return AND; }

"=="                   { lexer_out("ISEQU");                        return ISEQU; }
"!="                   { lexer_out("NOTEQU");                       return NOTEQU; }
"!"                    { lexer_out("NOT");                          return NOT; }

"<"                    { lexer_out("LT");                           return LT; }
"<="                   { lexer_out("LE");                           return LE; }
">"                    { lexer_out("GT");                           return GT; }
">="                   { lexer_out("GE");                           return GE; }

"+"                    { lexer_out("PLUS");                         return PLUS; }
"-"                    { lexer_out("MINUS");                        return MINUS; }
"*"                    { lexer_out("STAR");                         return STAR; }
"/"                    { lexer_out("DIV");                          return DIV; }
"%"                    { lexer_out("MOD");                          return MOD; }

"++"                   { lexer_out("INC");                          return INC; }
"--"                   { lexer_out("DEC");                          return DEC; }

"("                    { lexer_out("LRB");                          return LRB; }
")"                    { lexer_out("RRB");                          return RRB; }
"{"                    { lexer_out("LB");                           return LB; }
"}"                    { lexer_out("RB");                           return RB; }
"["                    { lexer_out("LSB");                          return LSB; }
"]"                    { lexer_out("RSB");                          return RSB; }
";"                    { lexer_out("SEMICOLON");                    return SEMICOLON; }
"\""                   { lexer_out("QUOTE");       BEGIN(lex_str);  return QUOTE; }
","                    { lexer_out("COMMA");                        return COMMA; }

{IDSTART}({IDCHAR}*)   { lexer_out("IDENTIFIER");                   return IDENTIFIER; }
{NUMBER}               { lexer_out("NUMBER");                       return NUMBER; }

<lex_str>"\""          { lexer_out("QUOTE");       BEGIN(INITIAL);  return QUOTE; }
<lex_str>[^\"]+        { lexer_out("STRING");                       return STRING; }

<<EOF>>                { return 0; }
.                      { lexer_out("ln " << yylineno << ": lexical error");  return 0; }

%%

#ifdef _MSC_VER
#    pragma warning(pop)
#endif
