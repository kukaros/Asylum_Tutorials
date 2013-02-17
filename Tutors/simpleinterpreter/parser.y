%locations

%{

#include "interpreter.h"

#ifdef _MSC_VER
#   pragma warning(push)
#   pragma warning(disable : 4102)
#   pragma warning(disable : 4273)
#   pragma warning(disable : 4065)
#   pragma warning(disable : 4267)
#   pragma warning(disable : 4244)
#   pragma warning(disable : 4996)
#endif

int yylex();
void yyerror(const char *s);

Interpreter* interpreter = 0;

%}

%union
{
    std::string* text_t;
}

%token             QUOTE
%token             LRB
%token             RRB
%token             LB
%token             RB
%token             LSB
%token             RSB
%token             SEMICOLON
%token             EQ
%token             COMMA
%token             PRINT
%token             INT

%token<text_t>     NUMBER
%token<text_t>     IDENTIFIER
%token<text_t>     STRING

%type<text_t>      string

%%

program: function
         {
             parser_out("program -> function");
         }
;

function: function_header LB function_body RB
          {
              parser_out("function -> function_header LB function_body RB");
          }
;

function_header: typename IDENTIFIER LRB argument_list RRB
                 {
                     parser_out("function_header -> typename IDENTIFIER LRB argument_list RRB");
                 }
;

function_body: statement_block
               {
                   parser_out("function_body -> statement_block");
               }
;

argument_list: /* empty */
;

typename: INT
          {
              parser_out("typename -> INT");
          }
;

statement_block: statement SEMICOLON
                 {
                     parser_out("statement_block -> statement SEMICOLON");
                 }
               | statement_block statement SEMICOLON
                 {
                     parser_out("statement_block -> statement_block statement SEMICOLON ");
                 }
;

statement: print
           {
               parser_out("statement -> print");
           }
;

print: PRINT string
       {
           parser_out("print -> PRINT string");
           interpreter->AddCodeEntry(OP_PRINT, interpreter->Allocate<std::string>($2), 0);
       }
;

string: QUOTE STRING QUOTE
        {
            parser_out("string -> QUOTE STRING QUOTE");
            $$ = $2;
        }
;


%%

#ifdef _MSC_VER
#    pragma warning(pop)
#endif
