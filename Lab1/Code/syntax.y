%locations
%define parse.error verbose
%{
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
typedef unsigned char uint8_t;
int yylex();
int yyerror(const char* msg, ...);
//int yyerror(const char* msg);

typedef enum {
    Int, Float, Id, Type, Relop, Ter, Nter, Null
} MyType;

uint8_t right = 1;

typedef struct {
    char* name;
    MyType type;
    int line;
    union {
        unsigned type_int;
        float type_float;
        char type_str[40];
    } val;
    //struct Node* parent;
    struct Node* child;
    struct Node* next;
} Node;

Node* newNode(char* _name, MyType _type, ...)
{
    va_list list;
    Node* res = (Node*)malloc(sizeof(Node));
    res->name = _name;
    res->type = _type;
    res->child = res->next = NULL;
    va_start(list, _type);
    switch (_type)
    {
        case Int: {
            if (*res->name == 'O')
                sscanf(va_arg(list, char*),"%ou",&res->val.type_int);
            else if (*res->name == 'H')
                sscanf(va_arg(list, char*),"%xu",&res->val.type_int);
            else
                res->val.type_int = (unsigned)atoi(va_arg(list, char*));
            break;
            }
        case Float: res->val.type_float = (float)atof(va_arg(list, char*)); break;
        case Id:
        case Type:
        case Relop:
        case Ter: strcpy(res->val.type_str, va_arg(list, char*)); break;
        case Nter: {
            int num = va_arg(list, int);
            res->line = va_arg(list, int);
            res->child = (struct Node*)va_arg(list, Node*);
            Node* tmp = (Node*)res->child;
            for (int i = 0; i < num - 1; i++)
            {
                tmp->next = (struct Node*)va_arg(list, Node*);
                tmp = (Node*)tmp->next;
            }
            break;
        }
        case Null: break;
        default: printf("Wrong Type: %s\n", res->name); break;
    }
    va_end(list);
    return res;
}

void outPut(Node* node, int dep)
{
    if (node->type != Null)
    {
        for (int i = 0; i < dep; i++)
        {
            printf("  ");
        }
    } 
    Node* p = node;
    switch (p->type)
    {
        case Nter: printf("%s (%d)\n", p->name, p->line); break;
        case Relop:
        case Ter: printf("%s\n", p->name); break;
        case Type: printf("TYPE: %s\n", p->val.type_str); break;
        case Id: printf("ID: %s\n", p->val.type_str); break;
        case Float: printf("FLOAT: %f\n", p->val.type_float); break;
        case Int: printf("INT: %u\n", p->val.type_int); break;
        case Null: break;
        default: printf("Wrong Type: %s\n", p->name); break;
    }
    if (p->child)
    {
        outPut((Node*)p->child, dep + 1);
    }
    if (p->next)
    {
        outPut((Node*)p->next, dep);
    }
}
/*
struct Error {
    int line;
    uint8_t 
}
*/
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
Program : ExtDefList {root = newNode("Program", Nter, 1, @1.first_line, $1);}
    ;
ExtDefList :            {$$ = newNode("ExtDefList", Null);} //0个或多个ExtDef
    | ExtDef ExtDefList {$$ = newNode("ExtDefList", Nter, 2, @1.first_line, $1, $2);}
    ;
ExtDef : Specifier ExtDecList SEMI  {$$ = newNode("ExtDef", Nter, 3, @1.first_line, $1, $2, $3);}// 全局变量
    | Specifier SEMI                {$$ = newNode("ExtDef", Nter, 2, @1.first_line, $1, $2);} // 结构体
    | Specifier FunDec CompSt       {$$ = newNode("ExtDef", Nter, 3, @1.first_line, $1, $2, $3);}// 函数
    | error SEMI                    {yyerror("Missing type or Wrong type", @1.first_line); yyerrok;}
    | Specifier error               {yyerror("Possibly missing \";\" at this or last line", @2.first_line); yyerrok;}
    //| error Specifier SEMI          {yyerror("error Specifier SEMI", @1.first_line);}
    ;
ExtDecList : VarDec             {$$ = newNode("ExtDecList", Nter, 1, @1.first_line, $1);}
    | VarDec COMMA ExtDecList   {$$ = newNode("ExtDecList", Nter, 3, @1.first_line, $1, $2, $3);}
    | VarDec error ExtDecList   {yyerror("Missing \",\"", @2.first_line); yyerrok;}
    | VarDec error              {yyerror("Possibly missing \";\" at this or last line", @2.first_line); yyerrok;}
    //VarDec error ExtDecList
    ;

// Specifiers
Specifier : TYPE        {$$ = newNode("Specifier", Nter, 1, @1.first_line, $1);}   // 类型描述符
    | StructSpecifier   {$$ = newNode("Specifier", Nter, 1, @1.first_line, $1);}
    ;
StructSpecifier : STRUCT OptTag LC DefList RC   {$$ = newNode("StructSpecifier", Nter, 5, @1.first_line, $1, $2, $3, $4, $5);} // 结构体类型
    | STRUCT Tag                                {$$ = newNode("StructSpecifier", Nter, 2, @1.first_line, $1, $2);}
    | STRUCT OptTag LC error RC                 {yyerror("Wrong struct definition", @2.first_line); yyerrok;}
    ;
OptTag :    {$$ = newNode("OptTag", Null);}
    | ID    {$$ = newNode("OptTag", Nter, 1, @1.first_line, $1);}
    ;
Tag : ID    {$$ = newNode("Tag", Nter, 1, @1.first_line, $1);}
    ;

// Declarators
VarDec : ID                 {$$ = newNode("VarDec", Nter, 1, @1.first_line, $1);} // 变量的定义
    | VarDec LB INT RB      {$$ = newNode("VarDec", Nter, 4, @1.first_line, $1, $2, $3, $4);}
    | VarDec LB error RB    {yyerror("Missing \"]\"", @3.first_line); yyerrok;}
    | VarDec LB error       {yyerror("Missing \"]\"", @3.first_line); yyerrok;}
    ;
FunDec : ID LP VarList RP   {$$ = newNode("FunDec", Nter, 4, @1.first_line, $1, $2, $3, $4);}            // 函数头的定义
    | ID LP RP              {$$ = newNode("FunDec", Nter, 3, @1.first_line, $1, $2, $3);}
    | ID LP error RP        {yyerror("Wrong argument(s)", @3.first_line); yyerrok;}
    //| ID LP error           {yyerror("ID LP error", @3.first_line);}
    | error LP VarList RP   {yyerror("Wrong function name", @1.first_line); yyerrok;}
    ;
VarList : ParamDec COMMA VarList    {$$ = newNode("VarList", Nter, 3, @1.first_line, $1, $2, $3);}// 形参列表
    | ParamDec                      {$$ = newNode("VarList", Nter, 1, @1.first_line, $1);}
    ;
ParamDec : Specifier VarDec {$$ = newNode("ParamDec", Nter, 2, @1.first_line, $1, $2);}
    ;

// Statements
CompSt : LC DefList StmtList RC {$$ = newNode("CompSt", Nter, 4, @1.first_line, $1, $2, $3, $4);}// 花括号括起来的语句块
    //| LC StmtList error RC      {yyerror("Define before using", @1.first_line);}
    | LC error RC                  {yyerror("Wrong statement(s)", @1.first_line); yyerrok;}
    ;
StmtList :          {{$$ = newNode("StmtList", Null);}}// 0个或多个语句
    | Stmt StmtList {$$ = newNode("StmtList", Nter, 2, @1.first_line, $1, $2);}
    //| error StmtList {yyerror("error StmtList", @1.first_line);}
    //| Stmt error StmtList {yyerror("Stmt error StmtList", @2.first_line);}
    ;
Stmt : Exp SEMI                     {$$ = newNode("Stmt", Nter, 2, @1.first_line, $1, $2);}   // 语句
    | CompSt                        {$$ = newNode("Stmt", Nter, 1, @1.first_line, $1);}
    | RETURN Exp SEMI               {$$ = newNode("Stmt", Nter, 3, @1.first_line, $1, $2, $3);}
    | IF LP Exp RP Stmt             {$$ = newNode("Stmt", Nter, 5, @1.first_line, $1, $2, $3, $4, $5);}
    | IF LP Exp RP Stmt ELSE Stmt   {$$ = newNode("Stmt", Nter, 7, @1.first_line, $1, $2, $3, $4, $5, $6, $7);}
    | WHILE LP Exp RP Stmt          {$$ = newNode("Stmt", Nter, 5, @1.first_line, $1, $2, $3, $4, $5);}
    | error SEMI                    {yyerror("Wrong expression or Definition after statement", @1.first_line); yyerrok;}
    | Exp error                     {if (@1.first_line != ignore_line) {yyerror("Possibly missing \";\" at this or last line", @1.first_line); yyerrok;} yyerrok;}
    //| Exp error SEMI                {yyerror("Missing \";\"", @2.first_line);}
    | RETURN Exp error              {yyerror("Possibly missing \";\" at this or last line", @3.first_line); yyerrok;}
    | RETURN error SEMI             {yyerror("Wrong return value", @2.first_line);}
    ;

// Local Definitions            局部变量
DefList :           {{$$ = newNode("DefList", Null);}}// 0个或多个变量定义
    | Def DefList   {$$ = newNode("DefList", Nter, 2, @1.first_line, $1, $2);}
    ;
Def : Specifier DecList SEMI    {$$ = newNode("Def", Nter, 3, @1.first_line, $1, $2, $3);}//一条变量定义
    | Specifier error SEMI      {yyerror("Possibly missing ID", @2.first_line);}
    //| Specifier DecList error   {yyerror("Specifier DecList error Possibly missing \";\" at this or last line", @3.first_line); yyerrok;}
    //| Specifier error   {yyerror("Specifier DecList error Possibly missing \";\" at this or last line", @2.first_line); yyerrok;}
    ;
DecList : Dec           {$$ = newNode("DecList", Nter, 1, @1.first_line, $1);}
    | Dec COMMA DecList {$$ = newNode("DecList", Nter, 3, @1.first_line, $1, $2, $3);}
    | Dec error DecList {yyerror("Missing \",\"", @2.first_line);}
    | Dec error         {yyerror("Possibly missing \";\" at this or last line", @2.first_line); yyerrok;}
    ;
Dec : VarDec                {$$ = newNode("Dec", Nter, 1, @1.first_line, $1);}
    | VarDec ASSIGNOP Exp   {$$ = newNode("Dec", Nter, 3, @1.first_line, $1, $2, $3);} // 定义时可以初始化
    | VarDec ASSIGNOP error   {yyerror("Wrong expression", @3.first_line); yyerrok;}
    ;

// Expressions
Exp : Exp ASSIGNOP Exp      {$$ = newNode("Exp", Nter, 3, @1.first_line, $1, $2, $3);}
    | Exp AND Exp           {$$ = newNode("Exp", Nter, 3, @1.first_line, $1, $2, $3);}
    | Exp OR Exp            {$$ = newNode("Exp", Nter, 3, @1.first_line, $1, $2, $3);}
    | Exp RELOP Exp         {$$ = newNode("Exp", Nter, 3, @1.first_line, $1, $2, $3);}
    | Exp PLUS Exp          {$$ = newNode("Exp", Nter, 3, @1.first_line, $1, $2, $3);}
    | Exp MINUS Exp         {$$ = newNode("Exp", Nter, 3, @1.first_line, $1, $2, $3);}
    | Exp STAR Exp          {$$ = newNode("Exp", Nter, 3, @1.first_line, $1, $2, $3);}
    | Exp DIV Exp           {$$ = newNode("Exp", Nter, 3, @1.first_line, $1, $2, $3);}
    | LP Exp RP             {$$ = newNode("Exp", Nter, 3, @1.first_line, $1, $2, $3);}
    | MINUS Exp %prec NOT   {$$ = newNode("Exp", Nter, 2, @1.first_line, $1, $2);}
    | NOT Exp               {$$ = newNode("Exp", Nter, 2, @1.first_line, $1, $2);}
    | ID LP Args RP         {$$ = newNode("Exp", Nter, 4, @1.first_line, $1, $2, $3, $4);}
    | ID LP RP              {$$ = newNode("Exp", Nter, 3, @1.first_line, $1, $2, $3);}
    | Exp LB Exp RB         {$$ = newNode("Exp", Nter, 4, @1.first_line, $1, $2, $3, $4);}
    | Exp DOT ID            {$$ = newNode("Exp", Nter, 3, @1.first_line, $1, $2, $3);}
    | ID                    {$$ = newNode("Exp", Nter, 1, @1.first_line, $1);}
    | INT                   {$$ = newNode("Exp", Nter, 1, @1.first_line, $1);}
    | FLOAT                 {$$ = newNode("Exp", Nter, 1, @1.first_line, $1);}
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
    //| ID LP error          {yyerror("Missing \")\"", @3.first_line);}
    | Exp LB error RB       {yyerror("Missing \"]\"", @3.first_line);}
    //| Exp error RB       {yyerror("Missing \"[\"", @3.first_line);}
    | Exp LB error SEMI     {yyerror("Missing \"]\"", @3.first_line); ignore_line = @3.first_line;}
    //| Exp LB error      {yyerror("Missing \"]\"", @3.first_line);}
    ;
Args : Exp COMMA Args   {$$ = newNode("Args", Nter, 3, @1.first_line, $1, $2, $3);}
    | Exp               {$$ = newNode("Args", Nter, 1, @1.first_line, $1);}
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
        //va_list list;
        //va_start(list, msg);
        //printf("Error type B at Line %d: %s.\n", va_arg(list, int), msg);
        printf(" %s.\n", msg);
        //va_end(list);
        myerr++;
    }
    return 0;
}
