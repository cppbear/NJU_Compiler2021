#include "semantic.h"
#include <stdbool.h>

typedef struct Operand_ Operand;
typedef struct InterCode InterCode;
typedef struct InterCodes InterCodes;

struct Operand_ {
    enum { VARIABLE, TEMP, PARAMETER, CONSTANT, LAB, FUNCT } kind;
    enum { VAL, ADDRESS} type;
    union {
        int var_no;
        long long int value;
        char* func_name;
    } u;
};

struct InterCode {
    enum { LABEL, FUNCTION, ASSIGN, ADD, SUB, MUL, DIV, GOTO, IF, RETURN, DEC, ARG, CALL, PARAM, READ, WRITE } kind;
    enum { NORMAL, GETADDR, GETVAL, SETVAL, COPY } type;
    union {
        struct {
            Operand *left, *right;
        } assign;
        struct {
            Operand *res, *op1, *op2;
        } binop;
        struct {
            Operand *res, *op;
        } sinop;
        struct {
            Operand* op;
        } single;
        struct {
            Operand *op1, *op2, *target;
            char relop[4];
        } cond;
        struct {
            Operand* op;
            unsigned size;
        } dec;
    } u;
};

struct InterCodes {
    InterCode code;
    bool isDelete;
    InterCodes *prev, *next;
};

void initInterCodes();
void setVariable();
Operand* newConst(long long int val);
void createAssign(unsigned type, Operand* left, Operand* right);
void createBinop(unsigned kind, unsigned type, Operand* res, Operand* op1, Operand* op2);
void createSinop(unsigned kind, Operand* res, Operand* op);
void createSingle(unsigned kind, Operand* op);
void createCond(Operand* op1, Operand* op2, Operand* target, char* re);
void createDec(Operand* op, unsigned size);
char* printOperand(Operand* op);
void writeInterCodes(const char* fielname, bool opt);

void tranCond(int id, Node* node, Operand* label_true, Operand* label_false);
void tranExp(Node* node, Operand* place);
void tranArgs(Node* node, Operand* arg_list[], int index);
void tranCompst(Node* node);

void tranProgram(Node* node);
void tranExtDefList(Node* node);
void tranExtDef(Node* node);
void tranFunDec(Node* node);
void tranCompSt(Node* node);
void tranVarDec(Node* node, int isFunc, int isStruct);
void tranDefList(Node* node);
void tranVarList(Node* node, int isFunc);
void tranParamDec(Node* node, int isFunc);
void tranStmtList(Node* node);
void tranStmt(Node* node);
void tranDef(Node* node);
void tranDecList(Node* node, int isStruct);
void tranDec(Node* node, int isStruct);
