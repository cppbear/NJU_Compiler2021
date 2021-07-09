#include "mips.h"
#include <assert.h>

#define PRECODE ".data\n_prompt: .asciiz \"Enter an integer:\"\n_ret: .asciiz \"\\n\"\n.globl main\n"           \
                ".text\nread:\n  li $v0, 4\n  la $a0, _prompt\n  syscall\n  li $v0, 5\n  syscall\n  jr $ra\n\n" \
                "write:\n  li $v0, 1\n  syscall\n  li $v0, 4\n  la $a0, _ret\n  syscall\n  move $v0, $0\n  jr $ra\n"

extern unsigned varCount, tmpCount;
extern InterCodes* head;
int* varOffset;
int regOcc;

void initAssembly()
{
    int n = varCount + tmpCount - 1;
    varOffset = (int*)malloc(sizeof(int) * n);
    for (int i = 0; i < n; i++) {
        varOffset[i] = -1;
    }
    regOcc = -1;
}

int calcuOffset(Operand* op, int fpoffset)
{
    if (op->kind == VARIABLE || op->kind == TEMP || op->kind == PARAMETER) {
        int offset = 0;
        if (op->kind == TEMP) {
            offset = varCount - 1;
        }
        if (varOffset[op->u.var_no + offset] == -1) {
            varOffset[op->u.var_no + offset] = fpoffset;
            return 4;
        } else
            return 0;
    } else
        return 0;
}

int allocVar(InterCodes* begin)
{
    int fpoffset = 4;
    InterCodes* p = begin;
    while (p && p->code.kind != FUNCTION) {
        if (!p->isDelete)
            switch (p->code.kind) {
            case ASSIGN: {
                fpoffset += calcuOffset(p->code.u.assign.left, fpoffset);
                fpoffset += calcuOffset(p->code.u.assign.right, fpoffset);
            } break;
            case ADD:
            case SUB:
            case MUL:
            case DIV: {
                fpoffset += calcuOffset(p->code.u.binop.res, fpoffset);
                fpoffset += calcuOffset(p->code.u.binop.op1, fpoffset);
                fpoffset += calcuOffset(p->code.u.binop.op2, fpoffset);
            } break;
            case IF: {
                fpoffset += calcuOffset(p->code.u.cond.op1, fpoffset);
                fpoffset += calcuOffset(p->code.u.cond.op2, fpoffset);
            } break;
            case DEC:
                if (varOffset[p->code.u.dec.op->u.var_no] == -1) {
                    varOffset[p->code.u.dec.op->u.var_no] = fpoffset + p->code.u.dec.size - 4;
                    fpoffset += p->code.u.dec.size;
                }
                break;
            case CALL: {
                fpoffset += calcuOffset(p->code.u.sinop.res, fpoffset);
            } break;
            case RETURN:
            case ARG:
            case PARAM:
            case READ:
            case WRITE:
                fpoffset += calcuOffset(p->code.u.single.op, fpoffset);
                break;
            }
        p = p->next;
    }
    return fpoffset - 4;
}

void cleanReg(FILE* f)
{
    if (regOcc != -1) {
        fprintf(f, "  sw $t0, -%d($fp)\n", regOcc);
        regOcc = -1;
    }
}

void MIPS(const char* fielname)
{
    InterCodes* p = head->next;
    FILE* f = fopen(fielname, "w");
    fprintf(f, PRECODE);
    while (p) {
        if (!p->isDelete)
            switch (p->code.kind) {
            case FUNCTION:
                if (!strcmp("main", p->code.u.single.op->u.func_name))
                    fprintf(f, "\nmain:\n");
                else
                    fprintf(f, "\n_%s:\n", printOperand(p->code.u.single.op));
                fprintf(f, "  addi $sp, $sp, -4\n  sw $fp, 0($sp)\n  move $fp, $sp\n");
                int offset = allocVar(p->next);
                fprintf(f, "  addi $sp, $sp, -%d\n", offset);
                break;
            case LABEL:
                fprintf(f, "%s:\n", printOperand(p->code.u.single.op));
                break;
            case GOTO:
                fprintf(f, "  j %s\n", printOperand(p->code.u.single.op));
                break;
            case ASSIGN: {
                Operand* l = p->code.u.assign.left;
                Operand* r = p->code.u.assign.right;
                int loffset = 0;
                if (l->kind == TEMP)
                    loffset = varCount - 1;
                switch (p->code.type) {
                case NORMAL: {
                    regOcc = varOffset[l->u.var_no + loffset];
                    if (r->kind == CONSTANT) {
                        fprintf(f, "  li $t0, %lld\n", r->u.value);
                    } else {
                        int roffset = 0;
                        if (r->kind == TEMP)
                            roffset = varCount - 1;
                        fprintf(f, "  lw $t1, -%d($fp)\n", varOffset[r->u.var_no + roffset]);
                        fprintf(f, "  move $t0, $t1\n");
                    }
                } break;
                case GETADDR: {
                    regOcc = varOffset[l->u.var_no + loffset];
                    int roffset = 0;
                    if (r->kind == TEMP) {
                        printf("hhhhhh\n");
                        roffset = varCount - 1;
                    }
                    fprintf(f, "  addi $t0, $fp, -%d\n", varOffset[r->u.var_no + roffset]);
                } break;
                case GETVAL: {
                    regOcc = varOffset[l->u.var_no + loffset];
                    int roffset = 0;
                    if (r->kind == TEMP)
                        roffset = varCount - 1;
                    fprintf(f, "  lw $t1, -%d($fp)\n", varOffset[r->u.var_no + roffset]);
                    fprintf(f, "  lw $t0, 0($t1)\n");
                } break;
                case SETVAL: {
                    fprintf(f, "  lw $t0, -%d($fp)\n", varOffset[l->u.var_no + loffset]);
                    int roffset = 0;
                    if (r->kind == TEMP)
                        roffset = varCount - 1;
                    fprintf(f, "  lw $t1, -%d($fp)\n", varOffset[r->u.var_no + roffset]);
                    fprintf(f, "  sw $t1, 0($t0)\n");
                } break;
                case COPY: {
                    fprintf(f, "  lw $t0, -%d($fp)\n", varOffset[l->u.var_no + loffset]);
                    int roffset = 0;
                    if (r->kind == TEMP)
                        roffset = varCount - 1;
                    fprintf(f, "  lw $t1, -%d($fp)\n", varOffset[r->u.var_no + roffset]);
                    fprintf(f, "  lw $t1, 0($t1)\n");
                    fprintf(f, "  sw $t1, 0($t0)\n");
                } break;
                default:
                    break;
                }
                cleanReg(f);
            } break;
            case ADD: {
                Operand* res = p->code.u.binop.res;
                Operand* op1 = p->code.u.binop.op1;
                Operand* op2 = p->code.u.binop.op2;
                int resoffset = 0, op1offset = 0, op2offset = 0;
                if (res->kind == TEMP)
                    resoffset = varCount - 1;
                if (op1->kind == TEMP)
                    op1offset = varCount - 1;
                if (op2->kind == TEMP)
                    op2offset = varCount - 1;
                regOcc = varOffset[res->u.var_no + resoffset];
                if (p->code.type == NORMAL) {
                    if (op1->kind != CONSTANT && op2->kind == CONSTANT) {
                        fprintf(f, "  lw $t1, -%d($fp)\n", varOffset[op1->u.var_no + op1offset]);
                        fprintf(f, "  addi $t0, $t1, %lld\n", op2->u.value);
                    } else if (op1->kind == CONSTANT && op2->kind != CONSTANT) {
                        fprintf(f, "  lw $t2, -%d($fp)\n", varOffset[op2->u.var_no + op2offset]);
                        fprintf(f, "  addi $t0, $t2, %lld\n", op1->u.value);
                    } else {
                        fprintf(f, "  lw $t1, -%d($fp)\n", varOffset[op1->u.var_no + op1offset]);
                        fprintf(f, "  lw $t2, -%d($fp)\n", varOffset[op2->u.var_no + op2offset]);
                        fprintf(f, "  add $t0, $t1, $t2\n");
                    }
                } else if (p->code.type == GETADDR) {
                    if (op2->kind == CONSTANT) {
                        int addroffset = op2->u.value - varOffset[op1->u.var_no + op1offset];
                        fprintf(f, "  addi $t0, $fp, %d\n", addroffset);
                    } else {
                        fprintf(f, "  addi $t0, $fp, -%d\n", varOffset[op1->u.var_no + op1offset]);
                        fprintf(f, "  lw $t2, -%d($fp)\n", varOffset[op2->u.var_no + op2offset]);
                        fprintf(f, "  add $t0, $t0, $t2\n");
                    }
                }
                cleanReg(f);
            } break;
            case SUB: {
                Operand* res = p->code.u.binop.res;
                Operand* op1 = p->code.u.binop.op1;
                Operand* op2 = p->code.u.binop.op2;
                int resoffset = 0, op1offset = 0, op2offset = 0;
                if (res->kind == TEMP)
                    resoffset = varCount - 1;
                if (op1->kind == TEMP)
                    op1offset = varCount - 1;
                if (op2->kind == TEMP)
                    op2offset = varCount - 1;
                regOcc = varOffset[res->u.var_no + resoffset];
                if (op1->kind != CONSTANT && op2->kind == CONSTANT) {
                    fprintf(f, "  lw $t1, -%d($fp)\n", varOffset[op1->u.var_no + op1offset]);
                    fprintf(f, "  addi $t0, $t1, %lld\n", -op2->u.value);
                } else if (op1->kind == CONSTANT && op2->kind != CONSTANT) {
                    fprintf(f, "  li $t1, %lld\n", op1->u.value);
                    fprintf(f, "  lw $t2, -%d($fp)\n", varOffset[op2->u.var_no + op2offset]);
                    fprintf(f, "  sub $t0, $t1, $t2\n");
                } else {
                    fprintf(f, "  lw $t1, -%d($fp)\n", varOffset[op1->u.var_no + op1offset]);
                    fprintf(f, "  lw $t2, -%d($fp)\n", varOffset[op2->u.var_no + op2offset]);
                    fprintf(f, "  sub $t0, $t1, $t2\n");
                }
                cleanReg(f);
            } break;
            case MUL: {
                Operand* res = p->code.u.binop.res;
                Operand* op1 = p->code.u.binop.op1;
                Operand* op2 = p->code.u.binop.op2;
                int resoffset = 0, op1offset = 0, op2offset = 0;
                if (res->kind == TEMP)
                    resoffset = varCount - 1;
                if (op1->kind == TEMP)
                    op1offset = varCount - 1;
                if (op2->kind == TEMP)
                    op2offset = varCount - 1;
                regOcc = varOffset[res->u.var_no + resoffset];
                if (op1->kind != CONSTANT && op2->kind == CONSTANT) {
                    fprintf(f, "  lw $t1, -%d($fp)\n", varOffset[op1->u.var_no + op1offset]);
                    fprintf(f, "  li $t2, %lld\n", op2->u.value);
                    fprintf(f, "  mul $t0, $t1, $t2\n");
                } else if (op1->kind == CONSTANT && op2->kind != CONSTANT) {
                    fprintf(f, "  li $t1, %lld\n", op1->u.value);
                    fprintf(f, "  lw $t2, -%d($fp)\n", varOffset[op2->u.var_no + op2offset]);
                    fprintf(f, "  mul $t0, $t1, $t2\n");
                } else {
                    fprintf(f, "  lw $t1, -%d($fp)\n", varOffset[op1->u.var_no + op1offset]);
                    fprintf(f, "  lw $t2, -%d($fp)\n", varOffset[op2->u.var_no + op2offset]);
                    fprintf(f, "  mul $t0, $t1, $t2\n");
                }
                cleanReg(f);
            } break;
            case DIV: {
                Operand* res = p->code.u.binop.res;
                Operand* op1 = p->code.u.binop.op1;
                Operand* op2 = p->code.u.binop.op2;
                int resoffset = 0, op1offset = 0, op2offset = 0;
                if (res->kind == TEMP)
                    resoffset = varCount - 1;
                if (op1->kind == TEMP)
                    op1offset = varCount - 1;
                if (op2->kind == TEMP)
                    op2offset = varCount - 1;
                regOcc = varOffset[res->u.var_no + resoffset];
                if (op1->kind != CONSTANT && op2->kind == CONSTANT) {
                    fprintf(f, "  lw $t1, -%d($fp)\n", varOffset[op1->u.var_no + op1offset]);
                    fprintf(f, "  li $t2, %lld\n", op2->u.value);
                    fprintf(f, "  div $t1, $t2\n  mflo $t0\n");
                } else if (op1->kind == CONSTANT && op2->kind != CONSTANT) {
                    fprintf(f, "  li $t1, %lld\n", op1->u.value);
                    fprintf(f, "  lw $t2, -%d($fp)\n", varOffset[op2->u.var_no + op2offset]);
                    fprintf(f, "  div $t1, $t2\n  mflo $t0\n");
                } else {
                    fprintf(f, "  lw $t1, -%d($fp)\n", varOffset[op1->u.var_no + op1offset]);
                    fprintf(f, "  lw $t2, -%d($fp)\n", varOffset[op2->u.var_no + op2offset]);
                    fprintf(f, "  div $t1, $t2\n  mflo $t0\n");
                }
                cleanReg(f);
            } break;
            case IF: {
                Operand* op1 = p->code.u.cond.op1;
                Operand* op2 = p->code.u.cond.op2;
                Operand* tar = p->code.u.cond.target;
                int resoffset = 0, op1offset = 0, op2offset = 0;
                if (op1->kind == TEMP)
                    op1offset = varCount - 1;
                if (op2->kind == TEMP)
                    op2offset = varCount - 1;
                if (op1->kind == CONSTANT)
                    fprintf(f, "  li $t1, %lld\n", op1->u.value);
                else
                    fprintf(f, "  lw $t1, -%d($fp)\n", varOffset[op1->u.var_no + op1offset]);
                if (op2->kind == CONSTANT)
                    fprintf(f, "  li $t2, %lld\n", op2->u.value);
                else
                    fprintf(f, "  lw $t2, -%d($fp)\n", varOffset[op2->u.var_no + op2offset]);
                char* relop = p->code.u.cond.relop;
                if (!strcmp("==", relop))
                    fprintf(f, "  beq $t1, $t2, %s\n", printOperand(tar));
                else if (!strcmp("!=", relop))
                    fprintf(f, "  bne $t1, $t2, %s\n", printOperand(tar));
                else if (!strcmp(">", relop))
                    fprintf(f, "  bgt $t1, $t2, %s\n", printOperand(tar));
                else if (!strcmp("<", relop))
                    fprintf(f, "  blt $t1, $t2, %s\n", printOperand(tar));
                else if (!strcmp(">=", relop))
                    fprintf(f, "  bge $t1, $t2, %s\n", printOperand(tar));
                else if (!strcmp("<=", relop))
                    fprintf(f, "  ble $t1, $t2, %s\n", printOperand(tar));
            } break;
            case RETURN: {
                Operand* ret = p->code.u.single.op;
                int retoffset = 0;
                if (ret->kind == TEMP)
                    retoffset = varCount - 1;
                if (ret->kind == CONSTANT) {
                    fprintf(f, "  li $v0, %lld\n", ret->u.value);
                } else {
                    fprintf(f, "  lw $v0, -%d($fp)\n", varOffset[ret->u.var_no + retoffset]);
                }
                fprintf(f, "  addi $sp, $fp, 4\n  lw $fp, 0($fp)\n");
                fprintf(f, "  jr $ra\n");
            } break;
            case ARG: {
                fprintf(f, "  addi $sp, $sp, -4\n");
                Operand* op = p->code.u.single.op;
                int opoffset = 0;
                if (op->kind == TEMP)
                    opoffset = varCount - 1;
                if (op->kind == CONSTANT) {
                    fprintf(f, "  li $t0, %lld\n", op->u.value);
                }
                else {
                    fprintf(f, "  lw $t0, -%d($fp)\n", varOffset[op->u.var_no + opoffset]);
                }
                fprintf(f, "  sw $t0, 0($sp)\n");
            } break;
            case CALL: {
                Operand* res = p->code.u.sinop.res;
                Operand* op = p->code.u.sinop.op;
                fprintf(f, "  addi $sp, $sp, -4\n  sw $ra, 0($sp)\n");
                if (!strcmp("main", op->u.func_name))
                    fprintf(f, "  jal main\n");
                else
                    fprintf(f, "  jal _%s\n", printOperand(op));
                fprintf(f, "  lw $ra, 0($sp)\n  addi $sp, $sp, 4\n");
                int resoffset = 0;
                if (res->kind == TEMP)
                    resoffset = varCount - 1;
                fprintf(f, "  sw $v0, -%d($fp)\n", varOffset[res->u.var_no + resoffset]);
                TableList* func = search(op->u.func_name);
                int argc = func->type->u.function.argc;
                fprintf(f, "  addi $sp, $sp, %d\n", argc * 4);
            } break;
            case PARAM: {
                assert(p->prev->code.kind == FUNCTION);
                TableList* func = search(p->prev->code.u.single.op->u.func_name);
                int argc = func->type->u.function.argc;
                for (int i = 0; i < argc; i++) {
                    Operand* op = p->code.u.single.op;
                    fprintf(f, "  lw $t0, %d($fp)\n", 8 + i * 4);
                    fprintf(f, "  sw $t0, -%d($fp)\n", varOffset[op->u.var_no]);
                    p = p->next;
                }
                p = p->prev;
            } break;
            case READ: {
                Operand* op = p->code.u.single.op;
                int opoffset = 0;
                if (op->kind == TEMP) {
                    opoffset = varCount - 1;
                }
                fprintf(f, "  addi $sp, $sp, -4\n  sw $ra, 0($sp)\n  jal read\n  lw $ra, 0($sp)\n  addi $sp, $sp, 4\n");
                fprintf(f, "  sw $v0, -%d($fp)\n", varOffset[op->u.var_no + opoffset]);
            } break;
            case WRITE: {
                Operand* op = p->code.u.single.op;
                if (op->kind == CONSTANT) {
                    fprintf(f, "  li $a0, %lld\n", op->u.value);
                } else {
                    int opoffset = 0;
                    if (op->kind == TEMP) {
                        opoffset = varCount - 1;
                    }
                    fprintf(f, "  lw $a0, -%d($fp)\n", varOffset[op->u.var_no + opoffset]);
                }
                fprintf(f, "  addi $sp, $sp, -4\n  sw $ra, 0($sp)\n  jal write\n  lw $ra, 0($sp)\n  addi $sp, $sp, 4\n");
            } break;
            default:
                break;
            }
        p = p->next;
    }
    fclose(f);
}
