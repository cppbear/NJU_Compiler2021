#include "optimize.h"
#include <stdbool.h>

extern InterCodes* head;
extern unsigned varCount, tmpCount;
extern bool optimization;
BasicBlock* bbhead;
BasicBlock* bbtail;
EntryList* entryhead;

// 打印中间结果供调试
void printCode(InterCodes* ir)
{
    switch (ir->code.kind) {
    case LABEL:
        printf("LABEL %s :\n", printOperand(ir->code.u.single.op));
        break;
    case FUNCTION:
        printf("FUNCTION %s :\n", printOperand(ir->code.u.single.op));
        break;
    case ASSIGN: {
        char* l = printOperand(ir->code.u.assign.left);
        char* r = printOperand(ir->code.u.assign.right);
        switch (ir->code.type) {
        case NORMAL:
            printf("%s := %s\n", l, r);
            break;
        case GETADDR:
            printf("%s := &%s\n", l, r);
            break;
        case GETVAL:
            printf("%s := *%s\n", l, r);
            break;
        case SETVAL:
            printf("*%s := %s\n", l, r);
            break;
        case COPY:
            printf("*%s := *%s\n", l, r);
            break;
        default:
            break;
        }
    } break;
    case ADD: {
        char* r = printOperand(ir->code.u.binop.res);
        char* op1 = printOperand(ir->code.u.binop.op1);
        char* op2 = printOperand(ir->code.u.binop.op2);
        if (ir->code.type == NORMAL)
            printf("%s := %s + %s\n", r, op1, op2);
        else if (ir->code.type == GETADDR)
            printf("%s := &%s + %s\n", r, op1, op2);
        else {
            printf("Wrong Binop:ADD type!\n");
            exit(-1);
        }
    } break;
    case SUB: {
        char* r = printOperand(ir->code.u.binop.res);
        char* op1 = printOperand(ir->code.u.binop.op1);
        char* op2 = printOperand(ir->code.u.binop.op2);
        printf("%s := %s - %s\n", r, op1, op2);
    } break;
    case MUL: {
        char* r = printOperand(ir->code.u.binop.res);
        char* op1 = printOperand(ir->code.u.binop.op1);
        char* op2 = printOperand(ir->code.u.binop.op2);
        printf("%s := %s * %s\n", r, op1, op2);
    } break;
    case DIV: {
        char* r = printOperand(ir->code.u.binop.res);
        char* op1 = printOperand(ir->code.u.binop.op1);
        char* op2 = printOperand(ir->code.u.binop.op2);
        printf("%s := %s / %s\n", r, op1, op2);
    } break;
    case GOTO:
        printf("GOTO %s\n", printOperand(ir->code.u.single.op));
        break;
    case IF: {
        char* op1 = printOperand(ir->code.u.cond.op1);
        char* op2 = printOperand(ir->code.u.cond.op2);
        char* tar = printOperand(ir->code.u.cond.target);
        printf("IF %s %s %s GOTO %s\n", op1, ir->code.u.cond.relop, op2, tar);
    } break;
    case RETURN:
        printf("RETURN %s\n", printOperand(ir->code.u.single.op));
        break;
    case DEC:
        printf("DEC %s %u\n", printOperand(ir->code.u.dec.op), ir->code.u.dec.size);
        break;
    case ARG:
        printf("ARG %s\n", printOperand(ir->code.u.single.op));
        break;
    case CALL: {
        char* res = printOperand(ir->code.u.sinop.res);
        char* op = printOperand(ir->code.u.sinop.op);
        printf("%s := CALL %s\n", res, op);
    } break;
    case PARAM:
        printf("PARAM %s\n", printOperand(ir->code.u.single.op));
        break;
    case READ:
        printf("READ %s\n", printOperand(ir->code.u.single.op));
        break;
    case WRITE:
        if (ir->code.u.single.op->type == VAL)
            printf("WRITE %s\n", printOperand(ir->code.u.single.op));
        else
            printf("WRITE *%s\n", printOperand(ir->code.u.single.op));
        break;
    default:
        break;
    }
}

void printCodes(InterCodes* begin, InterCodes* end, int full)
{
    InterCodes* p = begin;
    while (p != end->next) {
        if (p == begin || p == end || full) {
            printCode(p);
        }
        p = p->next;
    }
}

void printDelete()
{
    int num = 0;
    InterCodes* p = head->next;
    while (p) {
        if (p->isDelete) {
            printCode(p);
            num++;
        }
        p = p->next;
    }
    printf("Delete %d code(s).\n", num);
}

void printBlocks()
{
    BasicBlock* p = bbhead->next;
    while (p) {
        printf("============Block %d=============\n", p->id);
        printCodes(p->first, p->last, 0);
        p = p->next;
    }
    printf("=================================\n");
}

void printFlow()
{
    printf("============Flow==============\n");
    BasicBlock* p = bbhead->next;
    while (p) {
        printf("------------------------------\nBlock %d,\tsucc: ", p->id);
        BasicBlocks* q = p->successor;
        while (q && q->next) {
            printf("%d, ", q->block->id);
            q = q->next;
        }
        if (q)
            printf("%d\n", q->block->id);
        else
            printf("\n");
        p = p->next;
    }
    printf("==============================\n");
}

void printEntry()
{
    printf("===========Entry============\n");
    EntryList* p = entryhead->next;
    while (p) {
        printf("%d\n", p->blocknum);
        p = p->next;
    }
}

// 初始化基本块链表
void initBasicBlockList()
{
    bbhead = (BasicBlock*)malloc(sizeof(BasicBlock));
    bbhead->first = bbhead->last = NULL;
    bbhead->prev = bbhead->next = NULL;
    bbhead->predecessor = bbhead->successor = NULL;
    bbtail = bbhead;
    entryhead = (EntryList*)malloc(sizeof(EntryList));
    entryhead->entry = NULL;
    entryhead->next = NULL;
}

// 添加基本块
void addBasicBlock(BasicBlock* block)
{
    bbtail->next = block;
    block->prev = bbtail;
    bbtail = bbtail->next;
}

// 创建前驱后继链表
BasicBlocks* newBasicBlocks(BasicBlock* block)
{
    BasicBlocks* res = (BasicBlocks*)malloc(sizeof(BasicBlocks));
    res->block = block;
    res->next = NULL;
    return res;
}

// 添加流图入口
void addEntry(BasicBlock* entry)
{
    EntryList* newEntry = (EntryList*)malloc(sizeof(EntryList));
    newEntry->entry = entry;
    newEntry->exit = NULL;
    newEntry->blocknum = 0;
    newEntry->next = entryhead->next;
    entryhead->next = newEntry;
}

// 添加前驱后继关系
void addPred(BasicBlock* block, BasicBlock* pre)
{
    BasicBlocks* p = block->predecessor;
    if (!p)
        block->predecessor = newBasicBlocks(pre);
    else {
        while (p->next)
            p = p->next;
        p->next = newBasicBlocks(pre);
    }
}

void addSucc(BasicBlock* block, BasicBlock* suc)
{
    BasicBlocks* p = block->successor;
    if (!p)
        block->successor = newBasicBlocks(suc);
    else {
        while (p->next)
            p = p->next;
        p->next = newBasicBlocks(suc);
    }
}

void addPredSucc(BasicBlock* pre, BasicBlock* suc)
{
    addSucc(pre, suc);
    addPred(suc, pre);
}

// 查找label所在基本块
BasicBlock* findLabel(int no)
{
    BasicBlock* p = bbhead->next;
    while (p) {
        if (p->first->code.kind == LABEL && p->first->code.u.single.op->u.var_no == no)
            return p;
        p = p->next;
    }
}

// 构建基本块
void buildBasicBlocks()
{
    int blockid = 1;
    InterCodes* p = head->next;
    BasicBlock* newBlock = (BasicBlock*)malloc(sizeof(BasicBlock));
    newBlock->id = blockid;
    ++blockid;
    newBlock->first = p;
    newBlock->prev = newBlock->next = NULL;
    newBlock->predecessor = newBlock->successor = NULL;
    p = p->next;
    while (p->next) {
        int kind = p->code.kind;
        int prekind = p->prev->code.kind;
        if (kind == FUNCTION || kind == LABEL || prekind == RETURN || prekind == IF || prekind == GOTO) {
            newBlock->last = p->prev;
            addBasicBlock(newBlock);
            if (kind == FUNCTION)
                blockid = 1;
            newBlock = (BasicBlock*)malloc(sizeof(BasicBlock));
            newBlock->id = blockid;
            ++blockid;
            newBlock->first = p;
            newBlock->prev = newBlock->next = NULL;
            newBlock->predecessor = newBlock->successor = NULL;
        }
        p = p->next;
    }
    int kind = p->code.kind;
    if (kind == RETURN || kind == IF || kind == GOTO) {
        newBlock->last = p;
        addBasicBlock(newBlock);
    } else {
        newBlock->last = p->prev;
        addBasicBlock(newBlock);
        newBlock = (BasicBlock*)malloc(sizeof(BasicBlock));
        newBlock->id = blockid;
        ++blockid;
        newBlock->first = newBlock->last = p;
        newBlock->prev = newBlock->next = NULL;
        newBlock->predecessor = newBlock->successor = NULL;
        addBasicBlock(newBlock);
    }
    //newBlock->last = p;
    //addBasicBlock(newBlock);
}

// 构建流图
void buildFlow()
{
    BasicBlock* p = bbhead->next;
    while (p) {
        if (p->first->code.kind == FUNCTION) {
            BasicBlock* mark = p;
            BasicBlock* entry = (BasicBlock*)malloc(sizeof(BasicBlock));
            entry->id = 0;
            entry->first = entry->last = NULL;
            entry->predecessor = NULL;
            entry->successor = newBasicBlocks(p);
            p->predecessor = newBasicBlocks(entry);
            addEntry(entry);
            BasicBlock* exit = (BasicBlock*)malloc(sizeof(BasicBlock));
            entryhead->next->exit = exit;
            exit->id = -1;
            exit->first = exit->last = NULL;
            exit->predecessor = exit->successor = NULL;
            while (p) {
                entryhead->next->blocknum++;
                if (p != mark && p->first->code.kind == FUNCTION) {
                    entryhead->next->blocknum--;
                    break;
                } else if (p->last->code.kind == RETURN) {
                    addPredSucc(p, exit);
                    p = p->next;
                } else if (p->last->code.kind == IF) {
                    int label_no = p->last->code.u.cond.target->u.var_no;
                    BasicBlock* res = findLabel(label_no);
                    addPredSucc(p, p->next);
                    addPredSucc(p, res);
                    p = p->next;
                } else if (p->last->code.kind == GOTO) {
                    int label_no = p->last->code.u.single.op->u.var_no;
                    BasicBlock* res = findLabel(label_no);
                    addPredSucc(p, res);
                    p = p->next;
                } else {
                    if (p->next && p->next->first->code.kind != FUNCTION)
                        addPredSucc(p, p->next);
                    p = p->next;
                }
            }
        } else {
            //printf("Block id: %d, Code kind: %d\n", p->id, p->first->code.kind);
            //printf("NOT a Function?!\n");
            exit(-1);
        }
    }
}

// 级联跳转，可能会使执行代码条数变多，还是关闭比较好
void casJump()
{
    InterCodes* codeptr = head->next;
    while (codeptr && codeptr->next && codeptr->next->next) {
        if (codeptr->code.kind == IF && codeptr->next->code.kind == GOTO && codeptr->next->next->code.kind == LABEL) {
            int if_target = codeptr->code.u.cond.target->u.var_no;
            int goto_label = codeptr->next->code.u.single.op->u.var_no;
            int label = codeptr->next->next->code.u.single.op->u.var_no;
            if (if_target == label && if_target != goto_label) {
                char* relop = codeptr->code.u.cond.relop;
                if (!strcmp("==", relop)) {
                    strcpy(codeptr->code.u.cond.relop, "!=");
                } else if (!strcmp("!=", relop)) {
                    strcpy(codeptr->code.u.cond.relop, "==");
                } else if (!strcmp(">", relop)) {
                    strcpy(codeptr->code.u.cond.relop, "<=");
                } else if (!strcmp("<", relop)) {
                    strcpy(codeptr->code.u.cond.relop, ">=");
                } else if (!strcmp(">=", relop)) {
                    strcpy(codeptr->code.u.cond.relop, "<");
                } else if (!strcmp("<=", relop)) {
                    strcpy(codeptr->code.u.cond.relop, ">");
                }
                codeptr->code.u.cond.target = codeptr->next->code.u.single.op;
                codeptr->next = codeptr->next->next;
                codeptr->next->prev = codeptr;
            }
            codeptr = codeptr->next->next;
        }
        codeptr = codeptr->next;
    }
}

void optimize()
{
    optimization = true;
    //casJump();
    //inlineFunc();
    initBasicBlockList();
    buildBasicBlocks();
    buildFlow();
    //printBlocks();
    //printFlow();
    //printEntry();
    constProp();
    liveVariables();
    //printDelete();
}
