%{
#include "syntax.tab.h"
int yycolumn = 1;
#define YY_USER_ACTION \
    yylloc.first_line = yylloc.last_line = yylineno; \
    yylloc.first_column = yycolumn; \
    yylloc.last_column = yycolumn + yyleng - 1; \
    yycolumn += yyleng;

extern uint8_t right;
%}

%option yylineno
delim       [ \t]
ws          {delim}+
digit       [0-9]
letter      [_a-zA-Z]
nonletter   [`~@#$%^|\\\?:\"\']
relop       >|<|>=|<=|==|!=
type        int|float
%%
\n      {yycolumn = 1;}
{ws}    {}
struct  {yylval.node = newNode("STRUCT", Ter, yytext); return STRUCT;}
return  {yylval.node = newNode("RETURN", Ter, yytext); return RETURN;}
if      {yylval.node = newNode("IF", Ter, yytext); return IF;}
else    {yylval.node = newNode("ELSE", Ter, yytext); return ELSE;}
while   {yylval.node = newNode("WHILE", Ter, yytext); return WHILE;}
{type}  {yylval.node = newNode("TYPE", Type, yytext); return TYPE;}
"("     {yylval.node = newNode("LP", Ter, yytext); return LP;}
")"     {yylval.node = newNode("RP", Ter, yytext); return RP;}
"["     {yylval.node = newNode("LB", Ter, yytext); return LB;}
]       {yylval.node = newNode("RB", Ter, yytext); return RB;}
"{"     {yylval.node = newNode("LC", Ter, yytext); return LC;}
"}"     {yylval.node = newNode("RC", Ter, yytext); return RC;}
;       {yylval.node = newNode("SEMI", Ter, yytext); return SEMI;}
,       {yylval.node = newNode("COMMA", Ter, yytext); return COMMA;}
=       {yylval.node = newNode("ASSIGNOP", Ter, yytext); return ASSIGNOP;}
{relop} {yylval.node = newNode("RELOP", Relop, yytext); return RELOP;}
"+"     {yylval.node = newNode("PLUS", Ter, yytext); return PLUS;}
-       {yylval.node = newNode("MINUS", Ter, yytext); return MINUS;}
"*"     {yylval.node = newNode("STAR", Ter, yytext); return STAR;}
"/"     {yylval.node = newNode("DIV", Ter, yytext); return DIV;}
&&      {yylval.node = newNode("AND", Ter, yytext); return AND;}
"||"    {yylval.node = newNode("OR", Ter, yytext); return OR;}
"."     {yylval.node = newNode("DOT", Ter, yytext); return DOT;}
!       {yylval.node = newNode("NOT", Ter, yytext); return NOT;}
{letter}+({digit}|{letter})*    {yylval.node = newNode("ID", Id, yytext); return ID;}
([1-9]{digit}*|0)               {yylval.node = newNode("INT", Int, yytext); return INT;}
0[0-7]+                         {yylval.node = newNode("OCT", Int, yytext); return INT;}
0[xX][0-9a-fA-F]+               {yylval.node = newNode("HEX", Int, yytext); return INT;}
{digit}+\.{digit}+              {yylval.node = newNode("FLOAT", Float, yytext); return FLOAT;}
0[0-7]*[89]+[0-9]*                      {printf("Error type A at Line %d: Illegal octal number \'%s\'.\n", yylineno, yytext); yylval.node = newNode("OCT", Int, yytext); right = 0; return INT;}
0[xX][0-9a-fA-F]*[g-zG-Z]+[0-9a-zA-Z]*  {printf("Error type A at Line %d: Illegal hexadecimal number \'%s\'.\n", yylineno, yytext); yylval.node = newNode("HEX", Int, yytext); right = 0; return INT;}
{digit}+\.|\.{digit}+                   {printf("Error type A at Line %d: Illegal float number \'%s\'.\n", yylineno, yytext);}
{digit}+({digit}|{letter})+     {printf("Error type A at Line %d: Illegal ID \'%s\'.\n", yylineno, yytext); yylval.node = newNode("ID", Id, yytext); right = 0; return ID;}

.   {printf("Error type A at Line %d: Mysterious character \'%s\'.\n", yylineno, yytext); right = 0;}
%%
