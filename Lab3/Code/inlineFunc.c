#include "optimize.h"
#include <stdbool.h>

extern InterCodes* head;

typedef struct Inline Inline;

struct Inline {
    InterCodes *first, *last;
    Inline* next;
};

Inline* inlinehead;

int findFunc()
{
    int count = 0;
    inlinehead = (Inline*)malloc(sizeof(Inline));
    inlinehead->first = inlinehead->last = NULL;
    inlinehead->next = NULL;
    InterCodes* codeptr = head->next;
    while (codeptr) {
        bool canInline = true;
        if (codeptr->code.kind == FUNCTION) {
            InterCodes* funcbegin = codeptr;
            TableList* res = search(codeptr->code.u.single.op->u.func_name);
            if (!strcmp("main", res->name)) {
                codeptr = codeptr->next;
                continue;
            }
            FieldList* arg = res->type->u.function.args;
            while (arg) {
                if (arg->type->kind != INT) {
                    canInline = false;
                    break;
                }
                arg = arg->tail;
            }
            codeptr = codeptr->next;
            while (codeptr->next && codeptr->next->code.kind != FUNCTION) {
                int kind = codeptr->code.kind;
                if (kind == LABEL || kind == GOTO || kind == IF || kind == CALL) {
                    canInline = false;
                    break;
                }
                codeptr = codeptr->next;
            }
            if (canInline) {
                if (!codeptr->next || codeptr->next->code.kind == FUNCTION) {
                    Inline* func = (Inline*)malloc(sizeof(Inline));
                    func->first = funcbegin;
                    func->last = codeptr;
                    func->next = inlinehead->next;
                    inlinehead->next = func;
                    count++;
                    arg = res->type->u.function.args;
                    while (arg) {
                        TableList* argres = search(arg->name);
                        argres->op->kind = VARIABLE;
                        arg = arg->tail;
                    }
                }
                codeptr = codeptr->next;
            }
        } else {
            codeptr = codeptr->next;
        }
    }
    return count;
}

void copyCode(InterCodes* dest, InterCodes* src)
{
    dest->isDelete = src->isDelete;
    dest->code.kind = src->code.kind;
    dest->code.type = src->code.type;
    switch (src->code.kind) {
    case LABEL:
    case FUNCTION:
    case GOTO:
    case RETURN:
    case ARG:
    case PARAM:
    case READ:
    case WRITE:
        dest->code.u.single.op = src->code.u.single.op;
        break;
    case ASSIGN:
        dest->code.u.assign.left = src->code.u.assign.left;
        dest->code.u.assign.right = src->code.u.assign.right;
        break;
    case ADD:
    case SUB:
    case MUL:
    case DIV:
        dest->code.u.binop.res = src->code.u.binop.res;
        dest->code.u.binop.op1 = src->code.u.binop.op1;
        dest->code.u.binop.op2 = src->code.u.binop.op2;
        break;
    case IF:
        dest->code.u.cond.op1 = src->code.u.cond.op1;
        dest->code.u.cond.op2 = src->code.u.cond.op2;
        dest->code.u.cond.target = src->code.u.cond.target;
        strcpy(dest->code.u.cond.relop, src->code.u.cond.relop);
        break;
    case CALL:
        dest->code.u.sinop.res = src->code.u.sinop.res;
        dest->code.u.sinop.op = src->code.u.sinop.op;
        break;
    case DEC:
        dest->code.u.dec.op = src->code.u.dec.op;
        dest->code.u.dec.size = src->code.u.dec.size;
        break;
    }
}

void copyCodes(Inline* func, InterCodes* begin)
{
    InterCodes* codeptr = func->first->next;
    while (codeptr->code.kind == PARAM) {
        codeptr = codeptr->next;
    }
    //begin = (InterCodes*)malloc(sizeof(InterCodes));
    copyCode(begin, codeptr);
    begin->prev = begin->next = NULL;
    InterCodes* tail = begin;
    codeptr = codeptr->next;
    while (codeptr != func->last) {
        InterCodes* tmp = (InterCodes*)malloc(sizeof(InterCodes));
        copyCode(tmp, codeptr);
        tmp->next = NULL;
        tail->next = tmp;
        tmp->prev = tail;
        tail = tail->next;
        codeptr = codeptr->next;
    }
}

void replaceFunc()
{
    InterCodes* codeptr = head->next;
    while (codeptr) {
        if (codeptr->code.kind == CALL) {
            Inline* func = inlinehead->next;
            while (func) {
                if (!strcmp(func->first->code.u.single.op->u.func_name, codeptr->code.u.sinop.op->u.func_name)) {
                    InterCodes* arg = codeptr->prev;
                    InterCodes* param = func->first->next;
                    // 参数赋值
                    while (param->code.kind == PARAM && arg->code.kind == ARG) {
                        Operand* r = arg->code.u.single.op;
                        arg->code.kind = ASSIGN;
                        arg->code.type = NORMAL;
                        arg->code.u.assign.left = param->code.u.single.op;
                        arg->code.u.assign.right = r;
                        param = param->next;
                        arg = arg->prev;
                    }
                    InterCodes* beforeCall = codeptr->prev;
                    Operand* res = codeptr->code.u.sinop.res;
                    Operand* ret = func->last->code.u.single.op;
                    // 展开函数
                    InterCodes* newBegin = (InterCodes*)malloc(sizeof(InterCodes));
                    copyCodes(func, newBegin);
                    InterCodes* newEnd = newBegin;
                    while (newEnd->next)
                        newEnd = newEnd->next;
                    beforeCall->next = newBegin;
                    newBegin->prev = beforeCall;
                    newEnd->next = codeptr;
                    codeptr->prev = newEnd;
                    codeptr->code.kind = ASSIGN;
                    codeptr->code.type = NORMAL;
                    codeptr->code.u.assign.left = res;
                    codeptr->code.u.assign.right = ret;
                    break;
                }
                func = func->next;
            }
        }
        codeptr = codeptr->next;
    }
    Inline* func = inlinehead->next;
    while (func) {
        func->first->prev->next = func->last->next;
        func->last->next->prev = func->first->prev;
        for (InterCodes* p = func->first; p != func->last; p = p->next) {
            free(p);
            int a = 0;
        }
        func = func->next;
    }
}

void inlineFunc()
{
    int count = findFunc();
    int round = 0;
    while (count > 0 && round < 20) {
        replaceFunc();
        //char fname[1024];
        round++;
        //printf("%d\n", round);
        //sprintf(fname, "round.ir", round);
        //writeInterCodes(fname, false);
        count = findFunc();
    }
}
