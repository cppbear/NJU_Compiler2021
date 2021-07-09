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
struct  {yylval.node = newNode("STRUCT", -1, Ter, yytext); return STRUCT;}
return  {yylval.node = newNode("RETURN", -1, Ter, yytext); return RETURN;}
if      {yylval.node = newNode("IF", -1, Ter, yytext); return IF;}
else    {yylval.node = newNode("ELSE", -1, Ter, yytext); return ELSE;}
while   {yylval.node = newNode("WHILE", -1, Ter, yytext); return WHILE;}
{type}  {yylval.node = newNode("TYPE", -1, TYpe, yytext); return TYPE;}
"("     {yylval.node = newNode("LP", -1, Ter, yytext); return LP;}
")"     {yylval.node = newNode("RP", -1, Ter, yytext); return RP;}
"["     {yylval.node = newNode("LB", -1, Ter, yytext); return LB;}
]       {yylval.node = newNode("RB", -1, Ter, yytext); return RB;}
"{"     {yylval.node = newNode("LC", -1, Ter, yytext); return LC;}
"}"     {yylval.node = newNode("RC", -1, Ter, yytext); return RC;}
;       {yylval.node = newNode("SEMI", -1, Ter, yytext); return SEMI;}
,       {yylval.node = newNode("COMMA", -1, Ter, yytext); return COMMA;}
=       {yylval.node = newNode("ASSIGNOP", -1, Ter, yytext); return ASSIGNOP;}
{relop} {yylval.node = newNode("RELOP", -1, Relop, yytext); return RELOP;}
"+"     {yylval.node = newNode("PLUS", -1, Ter, yytext); return PLUS;}
-       {yylval.node = newNode("MINUS", -1, Ter, yytext); return MINUS;}
"*"     {yylval.node = newNode("STAR", -1, Ter, yytext); return STAR;}
"/"     {yylval.node = newNode("DIV", -1, Ter, yytext); return DIV;}
&&      {yylval.node = newNode("AND", -1, Ter, yytext); return AND;}
"||"    {yylval.node = newNode("OR", -1, Ter, yytext); return OR;}
"."     {yylval.node = newNode("DOT", -1, Ter, yytext); return DOT;}
!       {yylval.node = newNode("NOT", -1, Ter, yytext); return NOT;}
{letter}+({digit}|{letter})*    {yylval.node = newNode("ID", -1, Id, yytext); return ID;}
([1-9]{digit}*|0)               {yylval.node = newNode("INT", -1, Int, yytext); return INT;}
0[0-7]+                         {yylval.node = newNode("OCT", -1, Int, yytext); return INT;}
0[xX][0-9a-fA-F]+               {yylval.node = newNode("HEX", -1, Int, yytext); return INT;}
{digit}+\.{digit}+              {yylval.node = newNode("FLOAT", -1, Float, yytext); return FLOAT;}
0[0-7]*[89]+[0-9]*                      {printf("Error type A at Line %d: Illegal octal number \'%s\'.\n", yylineno, yytext); yylval.node = newNode("OCT", -1, Int, yytext); right = 0; return INT;}
0[xX][0-9a-fA-F]*[g-zG-Z]+[0-9a-zA-Z]*  {printf("Error type A at Line %d: Illegal hexadecimal number \'%s\'.\n", yylineno, yytext); yylval.node = newNode("HEX", -1, Int, yytext); right = 0; return INT;}
{digit}+\.|\.{digit}+                   {printf("Error type A at Line %d: Illegal float number \'%s\'.\n", yylineno, yytext);}
{digit}+({digit}|{letter})+     {printf("Error type A at Line %d: Illegal ID \'%s\'.\n", yylineno, yytext); yylval.node = newNode("ID", -1, Id, yytext); right = 0; return ID;}

.   {printf("Error type A at Line %d: Mysterious character \'%s\'.\n", yylineno, yytext); right = 0;}
%%
