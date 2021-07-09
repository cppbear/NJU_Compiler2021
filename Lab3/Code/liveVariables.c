#include "optimize.h"
#include <stdbool.h>

extern unsigned varCount, tmpCount;
extern EntryList* entryhead;

// 活跃变量
void liveVariables()
{
    EntryList* p = entryhead->next;
    int n = varCount + tmpCount - 1;
    while (p) {
        int m = p->blocknum + 2;
        bool** out = (bool**)malloc(sizeof(bool*) * m);
        bool** in = (bool**)malloc(sizeof(bool*) * m);
        for (int i = 0; i < m; i++) {
            out[i] = (bool*)malloc(sizeof(bool) * n);
            in[i] = (bool*)malloc(sizeof(bool) * n);
            memset(out[i], false, sizeof(bool) * n);
            memset(in[i], false, sizeof(bool) * n);
        }
        bool change = true;
        BasicBlocks* q = p->exit->predecessor;
        int num = 0;
        BasicBlocks* end;
        while (q) {
            if (q->block->id > num)
                end = q;
            q = q->next;
        }
        // 迭代计算活跃变量
        while (change) {
            change = false;
            BasicBlock* block = end->block;
            while (true) {
                BasicBlocks* succ = block->successor;
                if (succ) {
                    bool newOut[n];
                    memset(newOut, false, sizeof(bool) * n);
                    while (succ) {
                        for (int i = 1; i < n; i++) {
                            if (succ->block->id != -1)
                                newOut[i] |= in[succ->block->id][i];
                        }
                        succ = succ->next;
                    }
                    bool neq = memcmp(newOut, out[block->id], sizeof(bool) * n);
                    bool allFalse = true;
                    for (int i = 0; i < n; i++) {
                        if (newOut[i]) {
                            allFalse = false;
                            break;
                        }
                    }
                    if (neq || allFalse) {
                        if (neq)
                            change = true;
                        memcpy(out[block->id], newOut, sizeof(bool) * n);
                        bool newIn[n];
                        memset(newIn, false, sizeof(bool) * n);
                        int def_use[n][2];
                        memset(def_use, 0, sizeof(int) * n * 2);
                        InterCodes* codeptr = block->first;
                        int line = 1;
                        while (codeptr != block->last->next) {
                            if (codeptr->code.kind == ASSIGN) {
                                Operand* l = codeptr->code.u.assign.left;
                                Operand* r = codeptr->code.u.assign.right;
                                if (l->type == VAL) {
                                    if ((l->kind == VARIABLE || l->kind == PARAMETER) && def_use[l->u.var_no][0] == 0) {
                                        def_use[l->u.var_no][0] = line;
                                    } else if (l->kind == TEMP && def_use[l->u.var_no + varCount - 1][0] == 0) {
                                        def_use[l->u.var_no + varCount - 1][0] = line;
                                    }
                                }
                                if (r->type == VAL) {
                                    if ((r->kind == VARIABLE || r->kind == PARAMETER) && def_use[r->u.var_no][1] == 0) {
                                        def_use[r->u.var_no][1] = line;
                                    } else if (r->kind == TEMP && def_use[r->u.var_no + varCount - 1][1] == 0) {
                                        def_use[r->u.var_no + varCount - 1][1] = line;
                                    }
                                }
                            } else if (codeptr->code.kind == ADD || codeptr->code.kind == SUB || codeptr->code.kind == MUL || codeptr->code.kind == DIV) {
                                Operand* res = codeptr->code.u.binop.res;
                                Operand* op1 = codeptr->code.u.binop.op1;
                                Operand* op2 = codeptr->code.u.binop.op2;
                                if (res->type == VAL) {
                                    if ((res->kind == VARIABLE || res->kind == PARAMETER) && def_use[res->u.var_no][0] == 0) {
                                        def_use[res->u.var_no][0] = line;
                                    } else if (res->kind == TEMP && def_use[res->u.var_no + varCount - 1][0] == 0) {
                                        def_use[res->u.var_no + varCount - 1][0] = line;
                                    }
                                }
                                if (op1->type == VAL) {
                                    if ((op1->kind == VARIABLE || op1->kind == PARAMETER) && def_use[op1->u.var_no][1] == 0) {
                                        def_use[op1->u.var_no][1] = line;
                                    } else if (op1->kind == TEMP && def_use[op1->u.var_no + varCount - 1][1] == 0) {
                                        def_use[op1->u.var_no + varCount - 1][1] = line;
                                    }
                                }
                                if (op2->type == VAL) {
                                    if ((op2->kind == VARIABLE || op2->kind == PARAMETER) && def_use[op2->u.var_no][1] == 0) {
                                        def_use[op2->u.var_no][1] = line;
                                    } else if (op2->kind == TEMP && def_use[op2->u.var_no + varCount - 1][1] == 0) {
                                        def_use[op2->u.var_no + varCount - 1][1] = line;
                                    }
                                }
                            } else if (codeptr->code.kind == IF) {
                                Operand* op1 = codeptr->code.u.cond.op1;
                                Operand* op2 = codeptr->code.u.cond.op2;
                                if (op1->type == VAL) {
                                    if ((op1->kind == VARIABLE || op1->kind == PARAMETER) && def_use[op1->u.var_no][1] == 0) {
                                        def_use[op1->u.var_no][1] = line;
                                    } else if (op1->kind == TEMP && def_use[op1->u.var_no + varCount - 1][1] == 0) {
                                        def_use[op1->u.var_no + varCount - 1][1] = line;
                                    }
                                }
                                if (op2->type == VAL) {
                                    if ((op2->kind == VARIABLE || op2->kind == PARAMETER) && def_use[op2->u.var_no][1] == 0) {
                                        def_use[op2->u.var_no][1] = line;
                                    } else if (op2->kind == TEMP && def_use[op2->u.var_no + varCount - 1][1] == 0) {
                                        def_use[op2->u.var_no + varCount - 1][1] = line;
                                    }
                                }
                            } else if (codeptr->code.kind == RETURN || codeptr->code.kind == ARG || codeptr->code.kind == WRITE) {
                                Operand* op = codeptr->code.u.single.op;
                                if (op->type == VAL) {
                                    if ((op->kind == VARIABLE || op->kind == PARAMETER) && def_use[op->u.var_no][1] == 0) {
                                        def_use[op->u.var_no][1] = line;
                                    } else if (op->kind == TEMP && def_use[op->u.var_no + varCount - 1][1] == 0) {
                                        def_use[op->u.var_no + varCount - 1][1] = line;
                                    }
                                }
                            } else if (codeptr->code.kind == READ) {
                                Operand* op = codeptr->code.u.single.op;
                                if (op->type == VAL) {
                                    if ((op->kind == VARIABLE || op->kind == PARAMETER) && def_use[op->u.var_no][0] == 0) {
                                        def_use[op->u.var_no][0] = line;
                                    } else if (op->kind == TEMP && def_use[op->u.var_no + varCount - 1][0] == 0) {
                                        def_use[op->u.var_no + varCount - 1][0] = line;
                                    }
                                }
                            } else if (codeptr->code.kind == CALL) {
                                Operand* res = codeptr->code.u.sinop.res;
                                if (res->type == VAL) {
                                    if ((res->kind == VARIABLE || res->kind == PARAMETER) && def_use[res->u.var_no][0] == 0) {
                                        def_use[res->u.var_no][0] = line;
                                    } else if (res->kind == TEMP && def_use[res->u.var_no + varCount - 1][0] == 0) {
                                        def_use[res->u.var_no + varCount - 1][0] = line;
                                    }
                                }
                            }
                            line++;
                            codeptr = codeptr->next;
                        }
                        for (int i = 1; i < n; i++) {
                            bool flag = out[block->id][i];
                            if (def_use[i][0] != 0 && (def_use[i][0] < def_use[i][1] || def_use[i][1] == 0))
                                flag = false;
                            else if (def_use[i][1] != 0 && (def_use[i][1] <= def_use[i][0] || def_use[i][0] == 0))
                                flag = true;
                            newIn[i] = flag;
                        }
                        if (memcmp(in[block->id], newIn, sizeof(bool) * n)) {
                            change = true;
                            memcpy(in[block->id], newIn, sizeof(bool) * n);
                        }
                    }
                }
                if (block->id == 1)
                    break;
                block = block->prev;
            }
        }
        BasicBlock* block = p->entry->successor->block;
        // 标记死代码
        while (block) {
            InterCodes* codeptr = block->last;
            bool codeout[n], codein[n];
            memcpy(codeout, out[block->id], sizeof(bool) * n);
            //memset(codein, false, sizeof(bool) * n);
            memcpy(codein, codeout, sizeof(bool) * n);
            while (codeptr != block->first->prev) {
                //memcpy(codein, codeout, sizeof(bool) * n);
                int kind = codeptr->code.kind;
                bool delete = false;
                if (kind == ASSIGN) {
                    Operand* l = codeptr->code.u.assign.left;
                    Operand* r = codeptr->code.u.assign.right;
                    if (l->type == VAL) {
                        if (l->kind == VARIABLE || l->kind == PARAMETER) {
                            codein[l->u.var_no] = false;
                            if (!codeout[l->u.var_no])
                                delete = true;
                        } else if (l->kind == TEMP) {
                            codein[l->u.var_no + varCount - 1] = false;
                            if (!codeout[l->u.var_no + varCount - 1])
                                delete = true;
                        }
                    }
                    if (r->type == VAL) {
                        if (r->kind == VARIABLE || r->kind == PARAMETER) {
                            if (!delete)
                                codein[r->u.var_no] = true;
                        } else if (r->kind == TEMP) {
                            if (!delete)
                                codein[r->u.var_no + varCount - 1] = true;
                        }
                    }
                } else if (kind == ADD || kind == SUB || kind == MUL || kind == DIV) {
                    Operand* res = codeptr->code.u.binop.res;
                    Operand* op1 = codeptr->code.u.binop.op1;
                    Operand* op2 = codeptr->code.u.binop.op2;
                    if (res->type == VAL) {
                        if (res->kind == VARIABLE || res->kind == PARAMETER) {
                            codein[res->u.var_no] = false;
                            if (!codeout[res->u.var_no])
                                delete = true;
                        } else if (res->kind == TEMP) {
                            codein[res->u.var_no + varCount - 1] = false;
                            if (!codeout[res->u.var_no + varCount - 1])
                                delete = true;
                        }
                    }
                    if (op1->type == VAL) {
                        if (op1->kind == VARIABLE || op1->kind == PARAMETER) {
                            if (!delete)
                                codein[op1->u.var_no] = true;
                        } else if (op1->kind == TEMP) {
                            if (!delete)
                                codein[op1->u.var_no + varCount - 1] = true;
                        }
                    }
                    if (op2->type == VAL) {
                        if (op2->kind == VARIABLE || op2->kind == PARAMETER) {
                            if (!delete)
                                codein[op2->u.var_no] = true;
                        } else if (op2->kind == TEMP) {
                            if (!delete)
                                codein[op2->u.var_no + varCount - 1] = true;
                        }
                    }
                } else if (kind == IF) {
                    Operand* op1 = codeptr->code.u.cond.op1;
                    Operand* op2 = codeptr->code.u.cond.op2;
                    if (op1->type == VAL) {
                        if (op1->kind == VARIABLE || op1->kind == PARAMETER) {
                            codein[op1->u.var_no] = true;
                        } else if (op1->kind == TEMP) {
                            codein[op1->u.var_no + varCount - 1] = true;
                        }
                    }
                    if (op2->type == VAL) {
                        if (op2->kind == VARIABLE || op2->kind == PARAMETER) {
                            codein[op2->u.var_no] = true;
                        } else if (op2->kind == TEMP) {
                            codein[op2->u.var_no + varCount - 1] = true;
                        }
                    }
                } else if (kind == RETURN || kind == ARG || kind == WRITE) {
                    Operand* op = codeptr->code.u.single.op;
                    if (op->type == VAL) {
                        if (op->kind == VARIABLE || op->kind == PARAMETER) {
                            codein[op->u.var_no] = true;
                        } else if (op->kind == TEMP) {
                            codein[op->u.var_no + varCount - 1] = true;
                        }
                    }
                } else if (kind == READ) {
                    Operand* op = codeptr->code.u.single.op;
                    if (op->type == VAL) {
                        if (op->kind == VARIABLE || op->kind == PARAMETER) {
                            codein[op->u.var_no] = false;
                        } else if (op->kind == TEMP) {
                            codein[op->u.var_no + varCount - 1] = false;
                        }
                    }
                } else if (kind == CALL) {
                    Operand* res = codeptr->code.u.sinop.res;
                    if (res->type == VAL) {
                        if (res->kind == VARIABLE || res->kind == PARAMETER) {
                            codein[res->u.var_no] = false;
                        } else if (res->kind == TEMP) {
                            codein[res->u.var_no + varCount - 1] = false;
                        }
                    }
                }
                codeptr->isDelete = delete;
                codeptr = codeptr->prev;
                memcpy(codeout, codein, sizeof(bool) * n);
            }
            if (block->id == p->blocknum)
                break;
            block = block->next;
        }
        p = p->next;
        // 释放空间
        for (int i = 0; i < m; i++) {
            free(out[i]);
            free(in[i]);
        }
        free(out);
        free(in);
    }
}
