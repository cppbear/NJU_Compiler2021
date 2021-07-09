%locations
%define parse.error verbose
%{
#include "node.h"
typedef unsigned char uint8_t;
int yylex();
int yyerror(const char* msg, ...);

uint8_t right = 1;

int ignore_line = 0;
int syserr = 0;
int myerr = 0;
Node* root;
%}

%union {
    Node* node;
}

%right <node> ASSIGNOP
%left <node> OR
%left <node> AND
%left <node> RELOP
%left <node> PLUS MINUS
%left <node> STAR DIV
%right <node> NOT
%left <node> LP RP LB RB DOT

%token <node> INT //OCT HEX
%token <node> FLOAT
%token <node> ID
%token <node> STRUCT RETURN IF ELSE WHILE
%token <node> TYPE
%token <node> SEMI COMMA
%token <node> LC RC

%type <node> Program ExtDefList ExtDef Specifier ExtDecList FunDec CompSt VarDec StructSpecifier OptTag DefList Tag VarList ParamDec StmtList Exp Stmt Def DecList Dec Args

%%
// High-Level Definitions           高层语法
Program : ExtDefList {root = newNode("Program", 1, Nter, 1, @1.first_line, $1);}
    ;
ExtDefList :            {$$ = newNode("ExtDefList", 1, Null);} //0个或多个ExtDef
    | ExtDef ExtDefList {$$ = newNode("ExtDefList", 2, Nter, 2, @1.first_line, $1, $2);}
    ;
ExtDef : Specifier ExtDecList SEMI  {$$ = newNode("ExtDef", 1, Nter, 3, @1.first_line, $1, $2, $3);}// 全局变量
    | Specifier SEMI                {$$ = newNode("ExtDef", 2, Nter, 2, @1.first_line, $1, $2);} // 结构体
    | Specifier FunDec CompSt       {$$ = newNode("ExtDef", 3, Nter, 3, @1.first_line, $1, $2, $3);}// 函数
    | error SEMI                    {yyerror("Missing type or Wrong type", @1.first_line); yyerrok;}
    | Specifier error               {yyerror("Possibly missing \";\" at this or last line", @2.first_line); yyerrok;}
    ;
ExtDecList : VarDec             {$$ = newNode("ExtDecList", 1, Nter, 1, @1.first_line, $1);}
    | VarDec COMMA ExtDecList   {$$ = newNode("ExtDecList", 2, Nter, 3, @1.first_line, $1, $2, $3);}
    | VarDec error ExtDecList   {yyerror("Missing \",\"", @2.first_line); yyerrok;}
    | VarDec error              {yyerror("Possibly missing \";\" at this or last line", @2.first_line); yyerrok;}
    ;

// Specifiers
Specifier : TYPE        {$$ = newNode("Specifier", 1, Nter, 1, @1.first_line, $1); $$->no = 1;}   // 类型描述符
    | StructSpecifier   {$$ = newNode("Specifier", 2, Nter, 1, @1.first_line, $1); $$->no = 2;}
    ;
StructSpecifier : STRUCT OptTag LC DefList RC   {$$ = newNode("StructSpecifier", 1, Nter, 5, @1.first_line, $1, $2, $3, $4, $5);} // 结构体类型
    | STRUCT Tag                                {$$ = newNode("StructSpecifier", 2, Nter, 2, @1.first_line, $1, $2);}
    | STRUCT OptTag LC error RC                 {yyerror("Wrong struct definition", @2.first_line); yyerrok;}
    ;
OptTag :    {$$ = newNode("OptTag", 1, Null);}
    | ID    {$$ = newNode("OptTag", 2, Nter, 1, @1.first_line, $1);}
    ;
Tag : ID    {$$ = newNode("Tag", 1, Nter, 1, @1.first_line, $1);}
    ;

// Declarators
VarDec : ID                 {$$ = newNode("VarDec", 1, Nter, 1, @1.first_line, $1);} // 变量的定义
    | VarDec LB INT RB      {$$ = newNode("VarDec", 2, Nter, 4, @1.first_line, $1, $2, $3, $4);}
    | VarDec LB error RB    {yyerror("Missing \"]\"", @3.first_line); yyerrok;}
    | VarDec LB error       {yyerror("Missing \"]\"", @3.first_line); yyerrok;}
    ;
FunDec : ID LP VarList RP   {$$ = newNode("FunDec", 1, Nter, 4, @1.first_line, $1, $2, $3, $4);}            // 函数头的定义
    | ID LP RP              {$$ = newNode("FunDec", 2, Nter, 3, @1.first_line, $1, $2, $3);}
    | ID LP error RP        {yyerror("Wrong argument(s)", @3.first_line); yyerrok;}
    | error LP VarList RP   {yyerror("Wrong function name", @1.first_line); yyerrok;}
    ;
VarList : ParamDec COMMA VarList    {$$ = newNode("VarList", 1, Nter, 3, @1.first_line, $1, $2, $3);}// 形参列表
    | ParamDec                      {$$ = newNode("VarList", 2, Nter, 1, @1.first_line, $1);}
    ;
ParamDec : Specifier VarDec {$$ = newNode("ParamDec", 1, Nter, 2, @1.first_line, $1, $2);}
    ;

// Statements
CompSt : LC DefList StmtList RC {$$ = newNode("CompSt", 1, Nter, 4, @1.first_line, $1, $2, $3, $4);}// 花括号括起来的语句块
    | LC error RC                  {yyerror("Wrong statement(s)", @1.first_line); yyerrok;}
    ;
StmtList :          {{$$ = newNode("StmtList", 1, Null);}}// 0个或多个语句
    | Stmt StmtList {$$ = newNode("StmtList", 2, Nter, 2, @1.first_line, $1, $2);}
    ;
Stmt : Exp SEMI                     {$$ = newNode("Stmt", 1, Nter, 2, @1.first_line, $1, $2);}   // 语句
    | CompSt                        {$$ = newNode("Stmt", 2, Nter, 1, @1.first_line, $1);}
    | RETURN Exp SEMI               {$$ = newNode("Stmt", 3, Nter, 3, @1.first_line, $1, $2, $3);}
    | IF LP Exp RP Stmt             {$$ = newNode("Stmt", 4, Nter, 5, @1.first_line, $1, $2, $3, $4, $5);}
    | IF LP Exp RP Stmt ELSE Stmt   {$$ = newNode("Stmt", 5, Nter, 7, @1.first_line, $1, $2, $3, $4, $5, $6, $7);}
    | WHILE LP Exp RP Stmt          {$$ = newNode("Stmt", 6, Nter, 5, @1.first_line, $1, $2, $3, $4, $5);}
    | error SEMI                    {yyerror("Wrong expression or Definition after statement", @1.first_line); yyerrok;}
    | Exp error                     {if (@1.first_line != ignore_line) {yyerror("Possibly missing \";\" at this or last line", @1.first_line); yyerrok;} yyerrok;}
    | RETURN Exp error              {yyerror("Possibly missing \";\" at this or last line", @3.first_line); yyerrok;}
    | RETURN error SEMI             {yyerror("Wrong return value", @2.first_line);}
    ;

// Local Definitions            局部变量
DefList :           {{$$ = newNode("DefList", 1, Null);}}// 0个或多个变量定义
    | Def DefList   {$$ = newNode("DefList", 2, Nter, 2, @1.first_line, $1, $2);}
    ;
Def : Specifier DecList SEMI    {$$ = newNode("Def", 1, Nter, 3, @1.first_line, $1, $2, $3);}//一条变量定义
    | Specifier error SEMI      {yyerror("Possibly missing ID", @2.first_line);}
    ;
DecList : Dec           {$$ = newNode("DecList", 1, Nter, 1, @1.first_line, $1);}
    | Dec COMMA DecList {$$ = newNode("DecList", 2, Nter, 3, @1.first_line, $1, $2, $3);}
    | Dec error DecList {yyerror("Missing \",\"", @2.first_line);}
    | Dec error         {yyerror("Possibly missing \";\" at this or last line", @2.first_line); yyerrok;}
    ;
Dec : VarDec                {$$ = newNode("Dec", 1, Nter, 1, @1.first_line, $1);}
    | VarDec ASSIGNOP Exp   {$$ = newNode("Dec", 2, Nter, 3, @1.first_line, $1, $2, $3);} // 定义时可以初始化
    | VarDec ASSIGNOP error   {yyerror("Wrong expression", @3.first_line); yyerrok;}
    ;

// Expressions
Exp : Exp ASSIGNOP Exp      {$$ = newNode("Exp", 1, Nter, 3, @1.first_line, $1, $2, $3); $$->no = 1;}
    | Exp AND Exp           {$$ = newNode("Exp", 2, Nter, 3, @1.first_line, $1, $2, $3); $$->no = 2;}
    | Exp OR Exp            {$$ = newNode("Exp", 3, Nter, 3, @1.first_line, $1, $2, $3); $$->no = 3;}
    | Exp RELOP Exp         {$$ = newNode("Exp", 4, Nter, 3, @1.first_line, $1, $2, $3); $$->no = 4;}
    | Exp PLUS Exp          {$$ = newNode("Exp", 5, Nter, 3, @1.first_line, $1, $2, $3);}
    | Exp MINUS Exp         {$$ = newNode("Exp", 6, Nter, 3, @1.first_line, $1, $2, $3);}
    | Exp STAR Exp          {$$ = newNode("Exp", 7, Nter, 3, @1.first_line, $1, $2, $3);}
    | Exp DIV Exp           {$$ = newNode("Exp", 8, Nter, 3, @1.first_line, $1, $2, $3);}
    | LP Exp RP             {$$ = newNode("Exp", 9, Nter, 3, @1.first_line, $1, $2, $3);}
    | MINUS Exp %prec NOT   {$$ = newNode("Exp", 10, Nter, 2, @1.first_line, $1, $2);}
    | NOT Exp               {$$ = newNode("Exp", 11, Nter, 2, @1.first_line, $1, $2);}
    | ID LP Args RP         {$$ = newNode("Exp", 12, Nter, 4, @1.first_line, $1, $2, $3, $4);}
    | ID LP RP              {$$ = newNode("Exp", 13, Nter, 3, @1.first_line, $1, $2, $3);}
    | Exp LB Exp RB         {$$ = newNode("Exp", 14, Nter, 4, @1.first_line, $1, $2, $3, $4);}
    | Exp DOT ID            {$$ = newNode("Exp", 15, Nter, 3, @1.first_line, $1, $2, $3);}
    | ID                    {$$ = newNode("Exp", 16, Nter, 1, @1.first_line, $1);}
    | INT                   {$$ = newNode("Exp", 17, Nter, 1, @1.first_line, $1);}
    | FLOAT                 {$$ = newNode("Exp", 18, Nter, 1, @1.first_line, $1);}
    | Exp ASSIGNOP error    {yyerror("Wrong expression", @3.first_line); yyerrok;}
    | Exp AND error         {yyerror("Wrong expression", @3.first_line); yyerrok;}
    | Exp OR error          {yyerror("Wrong expression", @3.first_line); yyerrok;}
    | Exp RELOP error       {yyerror("Wrong expression", @3.first_line); yyerrok;}
    | Exp PLUS error        {yyerror("Wrong expression", @3.first_line); yyerrok;}
    | Exp MINUS error       {yyerror("Wrong expression", @3.first_line); yyerrok;}
    | Exp STAR error        {yyerror("Wrong expression", @3.first_line); yyerrok;}
    | Exp DIV error         {yyerror("Wrong expression", @3.first_line); yyerrok;}
    | LP error RP           {yyerror("Wrong expression", @2.first_line);}
    | MINUS error           {yyerror("Wrong expression", @2.first_line); yyerrok;}
    | NOT error             {yyerror("Wrong expression", @2.first_line); yyerrok;}
    | ID LP error RP        {yyerror("Wrong argument(s)", @3.first_line); yyerrok;}
    | ID LP error SEMI      {yyerror("Missing \")\"", @3.first_line); ignore_line = @3.first_line;}
    | Exp LB error RB       {yyerror("Missing \"]\"", @3.first_line);}
    | Exp LB error SEMI     {yyerror("Missing \"]\"", @3.first_line); ignore_line = @3.first_line;}
    ;
Args : Exp COMMA Args   {$$ = newNode("Args", 1, Nter, 3, @1.first_line, $1, $2, $3);}
    | Exp               {$$ = newNode("Args", 2, Nter, 1, @1.first_line, $1);}
    ;

%%
#include "lex.yy.c"

int yyerror(const char *msg, ...)
{
    right = 0;
    if (msg[0] == 's' && msg[1] == 'y')
    {
        printf("Error type B at Line %d: %s.", yylineno, msg);
        syserr++;
    }
    else
    {
        printf(" %s.\n", msg);
        myerr++;
    }
    return 0;
}
