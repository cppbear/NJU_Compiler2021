#include "optimize.h"
#include <stdbool.h>

extern unsigned varCount, tmpCount;
extern EntryList* entryhead;

// 常量折叠
void constProp()
{
    EntryList* p = entryhead->next;
    int n = varCount + tmpCount - 1;
    while (p) {
        int m = p->blocknum + 2;
        ValueType** out = (ValueType**)malloc(sizeof(ValueType*) * m);
        ValueType** in = (ValueType**)malloc(sizeof(ValueType*) * m);
        for (int i = 0; i < m; i++) {
            out[i] = (ValueType*)malloc(sizeof(ValueType) * n);
            in[i] = (ValueType*)malloc(sizeof(ValueType) * n);
            for (int j = 0; j < n; j++) {
                out[i][j].kind = UNDEF;
                out[i][j].val = 0;
                in[i][j].kind = UNDEF;
                in[i][j].val = 0;
            }
        }
        bool change = true;
        while (change) {
            change = false;
            BasicBlock* block = p->entry->successor->block;
            while (true) {
                BasicBlocks* pred = block->predecessor;
                if (pred) {
                    ValueType newIn[n];
                    for (int i = 0; i < n; i++) {
                        newIn[i].kind = UNDEF;
                        newIn[i].val = 0;
                    }
                    memcpy(newIn, out[pred->block->id], sizeof(ValueType) * n);
                    pred = pred->next;
                    while (pred) {
                        for (int i = 0; i < n; i++) {
                            if (newIn[i].kind == NAC || out[pred->block->id][i].kind == NAC) {
                                newIn[i].kind = NAC;
                                newIn[i].val = 0;
                            } else if (newIn[i].kind == UNDEF) {
                                newIn[i].kind = out[pred->block->id][i].kind;
                                newIn[i].val = out[pred->block->id][i].val;
                            } else if (newIn[i].kind == CONST && out[pred->block->id][i].kind == CONST) {
                                if (newIn[i].val != out[pred->block->id][i].val) {
                                    newIn[i].kind = NAC;
                                    newIn[i].val = 0;
                                }
                            }
                        }
                        pred = pred->next;
                    }
                    bool neq = memcmp(newIn, in[block->id], sizeof(ValueType) * n);
                    bool allUndef = true;
                    for (int i = 0; i < n; i++) {
                        if (newIn[i].kind != UNDEF) {
                            allUndef = false;
                            break;
                        }
                    }
                    if (neq || allUndef) {
                        if (neq)
                            change = true;
                        memcpy(in[block->id], newIn, sizeof(ValueType) * n);
                        ValueType newOut[n];
                        memcpy(newOut, newIn, sizeof(ValueType) * n);
                        InterCodes* codeptr = block->first;
                        while (codeptr != block->last->next) {
                            switch (codeptr->code.kind) {
                            case ASSIGN: {
                                Operand* l = codeptr->code.u.assign.left;
                                Operand* r = codeptr->code.u.assign.right;
                                if (l->type == VAL) {
                                    if (r->kind == CONSTANT) {
                                        if (l->kind == VARIABLE) {
                                            newOut[l->u.var_no].kind = CONST;
                                            newOut[l->u.var_no].val = r->u.value;
                                        } else if (l->kind == TEMP) {
                                            newOut[l->u.var_no + varCount - 1].kind = CONST;
                                            newOut[l->u.var_no + varCount - 1].val = r->u.value;
                                        }
                                        // else if (l->kind == PARAMETER) {
                                        //     newOut[l->u.var_no].kind = NAC;
                                        //     newOut[l->u.var_no].val = 0;
                                        // }
                                    } else if (r->type == VAL) {
                                        bool lassign = false, rassign = false;
                                        int loffset = 0, roffset = 0;
                                        if (l->kind == VARIABLE)
                                            lassign = true;
                                        else if (l->kind == TEMP) {
                                            lassign = true;
                                            loffset = varCount - 1;
                                        }
                                        if (r->kind == VARIABLE || r->kind == PARAMETER)
                                            rassign = true;
                                        else if (r->kind == TEMP) {
                                            rassign = true;
                                            roffset = varCount - 1;
                                        }
                                        if (lassign && rassign) {
                                            newOut[l->u.var_no + loffset].kind = newOut[r->u.var_no + roffset].kind;
                                            newOut[l->u.var_no + loffset].val = newOut[r->u.var_no + roffset].val;
                                        }
                                        // if (l->kind == PARAMETER) {
                                        //     newOut[l->u.var_no].kind = NAC;
                                        //     newOut[l->u.var_no].val = 0;
                                        // }
                                    } else if (r->type == ADDRESS) {
                                        bool lassign = false;
                                        int loffset = 0;
                                        if (l->kind == VARIABLE || l->kind == PARAMETER)
                                            lassign = true;
                                        else if (l->kind == TEMP) {
                                            lassign = true;
                                            loffset = varCount - 1;
                                        }
                                        if (lassign) {
                                            newOut[l->u.var_no + loffset].kind = NAC;
                                            newOut[l->u.var_no + loffset].val = 0;
                                        }
                                    }
                                }
                            } break;
                            case ADD:
                            case SUB:
                            case MUL:
                            case DIV: {
                                Operand* res = codeptr->code.u.binop.res;
                                Operand* op1 = codeptr->code.u.binop.op1;
                                Operand* op2 = codeptr->code.u.binop.op2;
                                bool resassign = false, op1assign = false, op2assign = false;
                                int resoffset = 0, op1offset = 0, op2offset = 0;
                                if (res->type == VAL) {
                                    if (res->kind == VARIABLE)
                                        resassign = true;
                                    else if (res->kind == TEMP) {
                                        resassign = true;
                                        resoffset = varCount - 1;
                                    }
                                }
                                if (op1->type == VAL) {
                                    if (op1->kind == VARIABLE || op1->kind == PARAMETER || op1->kind == CONSTANT)
                                        op1assign = true;
                                    else if (op1->kind == TEMP) {
                                        op1assign = true;
                                        op1offset = varCount - 1;
                                    }
                                }
                                if (op2->type == VAL) {
                                    if (op2->kind == VARIABLE || op2->kind == PARAMETER || op2->kind == CONSTANT)
                                        op2assign = true;
                                    else if (op2->kind == TEMP) {
                                        op2assign = true;
                                        op2offset = varCount - 1;
                                    }
                                }
                                if (resassign && op1assign && op2assign) {
                                    switch (codeptr->code.kind) {
                                    case ADD:
                                        if (op1->kind == CONSTANT && op2->kind == CONSTANT) {
                                            newOut[res->u.var_no + resoffset].kind = CONST;
                                            newOut[res->u.var_no + resoffset].val = op1->u.value + op2->u.value;
                                        } else if (op1->kind == CONSTANT && newOut[op2->u.var_no + op2offset].kind == CONST) {
                                            newOut[res->u.var_no + resoffset].kind = CONST;
                                            newOut[res->u.var_no + resoffset].val = op1->u.value + newOut[op2->u.var_no + op2offset].val;
                                        } else if (newOut[op1->u.var_no + op1offset].kind == CONST && op2->kind == CONSTANT) {
                                            newOut[res->u.var_no + resoffset].kind = CONST;
                                            newOut[res->u.var_no + resoffset].val = newOut[op1->u.var_no + op1offset].val + op2->u.value;
                                        } else if (newOut[op1->u.var_no + op1offset].kind == CONST && newOut[op2->u.var_no + op2offset].kind == CONST) {
                                            newOut[res->u.var_no + resoffset].kind = CONST;
                                            newOut[res->u.var_no + resoffset].val = newOut[op1->u.var_no + op1offset].val + newOut[op2->u.var_no + op2offset].val;
                                        } else if (newOut[op1->u.var_no + op1offset].kind == NAC || newOut[op2->u.var_no + op2offset].kind == NAC) {
                                            newOut[res->u.var_no + resoffset].kind = NAC;
                                            newOut[res->u.var_no + resoffset].val = 0;
                                        } else {
                                            newOut[res->u.var_no + resoffset].kind = UNDEF;
                                            newOut[res->u.var_no + resoffset].val = 0;
                                        }
                                        break;
                                    case SUB:
                                        if (op1->kind == CONSTANT && op2->kind == CONSTANT) {
                                            newOut[res->u.var_no + resoffset].kind = CONST;
                                            newOut[res->u.var_no + resoffset].val = op1->u.value - op2->u.value;
                                        } else if (op1->kind == CONSTANT && newOut[op2->u.var_no + op2offset].kind == CONST) {
                                            newOut[res->u.var_no + resoffset].kind = CONST;
                                            newOut[res->u.var_no + resoffset].val = op1->u.value - newOut[op2->u.var_no + op2offset].val;
                                        } else if (newOut[op1->u.var_no + op1offset].kind == CONST && op2->kind == CONSTANT) {
                                            newOut[res->u.var_no + resoffset].kind = CONST;
                                            newOut[res->u.var_no + resoffset].val = newOut[op1->u.var_no + op1offset].val - op2->u.value;
                                        } else if (newOut[op1->u.var_no + op1offset].kind == CONST && newOut[op2->u.var_no + op2offset].kind == CONST) {
                                            newOut[res->u.var_no + resoffset].kind = CONST;
                                            newOut[res->u.var_no + resoffset].val = newOut[op1->u.var_no + op1offset].val - newOut[op2->u.var_no + op2offset].val;
                                        } else if (newOut[op1->u.var_no + op1offset].kind == NAC || newOut[op2->u.var_no + op2offset].kind == NAC) {
                                            newOut[res->u.var_no + resoffset].kind = NAC;
                                            newOut[res->u.var_no + resoffset].val = 0;
                                        } else {
                                            newOut[res->u.var_no + resoffset].kind = UNDEF;
                                            newOut[res->u.var_no + resoffset].val = 0;
                                        }
                                        break;
                                    case MUL:
                                        if (op1->kind == CONSTANT && op2->kind == CONSTANT) {
                                            newOut[res->u.var_no + resoffset].kind = CONST;
                                            newOut[res->u.var_no + resoffset].val = op1->u.value * op2->u.value;
                                        } else if (op1->kind == CONSTANT && newOut[op2->u.var_no + op2offset].kind == CONST) {
                                            newOut[res->u.var_no + resoffset].kind = CONST;
                                            newOut[res->u.var_no + resoffset].val = op1->u.value * newOut[op2->u.var_no + op2offset].val;
                                        } else if (newOut[op1->u.var_no + op1offset].kind == CONST && op2->kind == CONSTANT) {
                                            newOut[res->u.var_no + resoffset].kind = CONST;
                                            newOut[res->u.var_no + resoffset].val = newOut[op1->u.var_no + op1offset].val * op2->u.value;
                                        } else if (newOut[op1->u.var_no + op1offset].kind == CONST && newOut[op2->u.var_no + op2offset].kind == CONST) {
                                            newOut[res->u.var_no + resoffset].kind = CONST;
                                            newOut[res->u.var_no + resoffset].val = newOut[op1->u.var_no + op1offset].val * newOut[op2->u.var_no + op2offset].val;
                                        } else if (newOut[op1->u.var_no + op1offset].kind == NAC || newOut[op2->u.var_no + op2offset].kind == NAC) {
                                            newOut[res->u.var_no + resoffset].kind = NAC;
                                            newOut[res->u.var_no + resoffset].val = 0;
                                        } else {
                                            newOut[res->u.var_no + resoffset].kind = UNDEF;
                                            newOut[res->u.var_no + resoffset].val = 0;
                                        }
                                        break;
                                    case DIV:
                                        if (op1->kind == CONSTANT && op2->kind == CONSTANT) {
                                            newOut[res->u.var_no + resoffset].kind = CONST;
                                            newOut[res->u.var_no + resoffset].val = op1->u.value / op2->u.value;
                                        } else if (op1->kind == CONSTANT && newOut[op2->u.var_no + op2offset].kind == CONST) {
                                            if (newOut[op2->u.var_no + op2offset].val == 0) {
                                                newOut[res->u.var_no + resoffset].kind = NAC;
                                                newOut[res->u.var_no + resoffset].val = 0;
                                            } else {
                                                newOut[res->u.var_no + resoffset].kind = CONST;
                                                newOut[res->u.var_no + resoffset].val = (op1->u.value / newOut[op2->u.var_no + op2offset].val);
                                            }
                                            if ((op1->u.value ^ newOut[op2->u.var_no + op2offset].val) < 0) {
                                                newOut[res->u.var_no + resoffset].kind = NAC;
                                                newOut[res->u.var_no + resoffset].val = 0;
                                            }
                                        } else if (newOut[op1->u.var_no + op1offset].kind == CONST && op2->kind == CONSTANT) {
                                            newOut[res->u.var_no + resoffset].kind = CONST;
                                            newOut[res->u.var_no + resoffset].val = newOut[op1->u.var_no + op1offset].val / op2->u.value;
                                            if ((newOut[op1->u.var_no + op1offset].val ^ op2->u.value) < 0) {
                                                newOut[res->u.var_no + resoffset].kind = NAC;
                                                newOut[res->u.var_no + resoffset].val = 0;
                                            }
                                        } else if (newOut[op1->u.var_no + op1offset].kind == CONST && newOut[op2->u.var_no + op2offset].kind == CONST) {
                                            if (newOut[op2->u.var_no + op2offset].val == 0) {
                                                newOut[res->u.var_no + resoffset].kind = NAC;
                                                newOut[res->u.var_no + resoffset].val = 0;
                                            } else {
                                                newOut[res->u.var_no + resoffset].kind = CONST;
                                                newOut[res->u.var_no + resoffset].val = newOut[op1->u.var_no + op1offset].val / newOut[op2->u.var_no + op2offset].val;
                                            }
                                            if ((newOut[op1->u.var_no + op1offset].val ^ newOut[op2->u.var_no + op2offset].val) < 0) {
                                                newOut[res->u.var_no + resoffset].kind = NAC;
                                                newOut[res->u.var_no + resoffset].val = 0;
                                            }
                                        } else if (newOut[op1->u.var_no + op1offset].kind == NAC || newOut[op2->u.var_no + op2offset].kind == NAC) {
                                            newOut[res->u.var_no + resoffset].kind = NAC;
                                            newOut[res->u.var_no + resoffset].val = 0;
                                        } else {
                                            newOut[res->u.var_no + resoffset].kind = UNDEF;
                                            newOut[res->u.var_no + resoffset].val = 0;
                                        }
                                        break;
                                    }
                                } else if (res->type == ADDRESS || op1->type == ADDRESS || op2->type == ADDRESS) {
                                    if (res->kind == TEMP) {
                                        resoffset = varCount - 1;
                                    }
                                    newOut[res->u.var_no + resoffset].kind = NAC;
                                    newOut[res->u.var_no + resoffset].val = 0;
                                }
                                // if (res->kind == PARAMETER) {
                                //     newOut[res->u.var_no].kind = NAC;
                                //     newOut[res->u.var_no].val = 0;
                                // }
                            } break;
                            case CALL: {
                                Operand* res = codeptr->code.u.sinop.res;
                                bool resassign = false;
                                int resoffset = 0;
                                if (res->kind == VARIABLE || res->kind == PARAMETER)
                                    resassign = true;
                                else if (res->kind == TEMP) {
                                    resassign = true;
                                    resoffset = varCount - 1;
                                }
                                if (resassign) {
                                    newOut[res->u.var_no + resoffset].kind = NAC;
                                    newOut[res->u.var_no + resoffset].val = 0;
                                }
                            } break;
                            case READ: {
                                Operand* op = codeptr->code.u.single.op;
                                bool opassign = false;
                                int opoffset = 0;
                                if (op->kind == VARIABLE || op->kind == PARAMETER)
                                    opassign = true;
                                else if (op->kind == TEMP) {
                                    opassign = true;
                                    opoffset = varCount - 1;
                                }
                                if (opassign) {
                                    newOut[op->u.var_no + opoffset].kind = NAC;
                                    newOut[op->u.var_no + opoffset].val = 0;
                                }
                            } break;
                            case PARAM: {
                                Operand* op = codeptr->code.u.single.op;
                                newOut[op->u.var_no].kind = NAC;
                                newOut[op->u.var_no].val = 0;
                            } break;
                            default:
                                break;
                            }
                            codeptr = codeptr->next;
                        }
                        if (memcmp(out[block->id], newOut, sizeof(ValueType) * n)) {
                            change = true;
                            memcpy(out[block->id], newOut, sizeof(ValueType) * n);
                        }
                    }
                } // 收尾
                if (block->id == p->blocknum)
                    break;
                block = block->next;
            }
        }
        // 折叠
        BasicBlock* block = p->entry->successor->block;
        while (block) {
            InterCodes* codeptr = block->first;
            ValueType codein[n], codeout[n];
            memcpy(codein, in[block->id], sizeof(ValueType) * n);
            memcpy(codeout, in[block->id], sizeof(ValueType) * n);
            while (codeptr != block->last->next) {
                switch (codeptr->code.kind) {
                case ASSIGN: {
                    Operand* l = codeptr->code.u.assign.left;
                    Operand* r = codeptr->code.u.assign.right;
                    if (l->type == VAL) {
                        if (r->kind == CONSTANT) {
                            if (l->kind == VARIABLE) {
                                codeout[l->u.var_no].kind = CONST;
                                codeout[l->u.var_no].val = r->u.value;
                            } else if (l->kind == TEMP) {
                                codeout[l->u.var_no + varCount - 1].kind = CONST;
                                codeout[l->u.var_no + varCount - 1].val = r->u.value;
                            }
                            // else if (l->kind == PARAMETER) {
                            //     codeout[l->u.var_no].kind = NAC;
                            //     codeout[l->u.var_no].val = 0;
                            // }
                        } else if (r->type == VAL) {
                            bool lassign = false, rassign = false;
                            int loffset = 0, roffset = 0;
                            if (l->kind == VARIABLE || l->kind == PARAMETER)
                                lassign = true;
                            else if (l->kind == TEMP) {
                                lassign = true;
                                loffset = varCount - 1;
                            }
                            if (r->kind == VARIABLE || r->kind == PARAMETER)
                                rassign = true;
                            else if (r->kind == TEMP) {
                                rassign = true;
                                roffset = varCount - 1;
                            }
                            if (lassign && rassign) {
                                codeout[l->u.var_no + loffset].kind = codeout[r->u.var_no + roffset].kind;
                                codeout[l->u.var_no + loffset].val = codeout[r->u.var_no + roffset].val;
                                if (codeout[r->u.var_no + roffset].kind == CONST) {
                                    Operand* con = newConst(codeout[r->u.var_no + roffset].val);
                                    codeptr->code.u.assign.right = con;
                                }
                            }
                            // if (l->kind == PARAMETER) {
                            //     codeout[l->u.var_no].kind = NAC;
                            //     codeout[l->u.var_no].val = 0;
                            // }
                        } else if (r->type == ADDRESS) {
                            bool lassign = false;
                            int loffset = 0;
                            if (l->kind == VARIABLE || l->kind == PARAMETER)
                                lassign = true;
                            else if (l->kind == TEMP) {
                                lassign = true;
                                loffset = varCount - 1;
                            }
                            if (lassign) {
                                codeout[l->u.var_no + loffset].kind = NAC;
                                codeout[l->u.var_no + loffset].val = 0;
                            }
                        }
                    }
                } break;
                case ADD:
                case SUB:
                case MUL:
                case DIV: {
                    Operand* res = codeptr->code.u.binop.res;
                    Operand* op1 = codeptr->code.u.binop.op1;
                    Operand* op2 = codeptr->code.u.binop.op2;
                    bool resassign = false, op1assign = false, op2assign = false;
                    int resoffset = 0, op1offset = 0, op2offset = 0;
                    if (res->type == VAL) {
                        if (res->kind == VARIABLE || res->kind == PARAMETER)
                            resassign = true;
                        else if (res->kind == TEMP) {
                            resassign = true;
                            resoffset = varCount - 1;
                        }
                    }
                    if (op1->type == VAL) {
                        if (op1->kind == VARIABLE || op1->kind == PARAMETER || op1->kind == CONSTANT)
                            op1assign = true;
                        else if (op1->kind == TEMP) {
                            op1assign = true;
                            op1offset = varCount - 1;
                        }
                    }
                    if (op2->type == VAL) {
                        if (op2->kind == VARIABLE || op2->kind == PARAMETER || op2->kind == CONSTANT)
                            op2assign = true;
                        else if (op2->kind == TEMP) {
                            op2assign = true;
                            op2offset = varCount - 1;
                        }
                    }
                    if (resassign && op1assign && op2assign) {
                        switch (codeptr->code.kind) {
                        case ADD:
                            if (op1->kind == CONSTANT && op2->kind == CONSTANT) {
                                codeout[res->u.var_no + resoffset].kind = CONST;
                                codeout[res->u.var_no + resoffset].val = op1->u.value + op2->u.value;
                                Operand* con = newConst(codeout[res->u.var_no + resoffset].val);
                                codeptr->code.kind = ASSIGN;
                                codeptr->code.type = NORMAL;
                                codeptr->code.u.assign.right = con;
                            } else if (op1->kind == CONSTANT && codeout[op2->u.var_no + op2offset].kind == CONST) {
                                codeout[res->u.var_no + resoffset].kind = CONST;
                                codeout[res->u.var_no + resoffset].val = op1->u.value + codeout[op2->u.var_no + op2offset].val;
                                Operand* con = newConst(codeout[res->u.var_no + resoffset].val);
                                codeptr->code.kind = ASSIGN;
                                codeptr->code.type = NORMAL;
                                codeptr->code.u.assign.right = con;
                            } else if (codeout[op1->u.var_no + op1offset].kind == CONST && op2->kind == CONSTANT) {
                                codeout[res->u.var_no + resoffset].kind = CONST;
                                codeout[res->u.var_no + resoffset].val = codeout[op1->u.var_no + op1offset].val + op2->u.value;
                                Operand* con = newConst(codeout[res->u.var_no + resoffset].val);
                                codeptr->code.kind = ASSIGN;
                                codeptr->code.type = NORMAL;
                                codeptr->code.u.assign.right = con;
                            } else if (codeout[op1->u.var_no + op1offset].kind == CONST && codeout[op2->u.var_no + op2offset].kind == CONST) {
                                codeout[res->u.var_no + resoffset].kind = CONST;
                                codeout[res->u.var_no + resoffset].val = codeout[op1->u.var_no + op1offset].val + codeout[op2->u.var_no + op2offset].val;
                                Operand* con = newConst(codeout[res->u.var_no + resoffset].val);
                                codeptr->code.kind = ASSIGN;
                                codeptr->code.type = NORMAL;
                                codeptr->code.u.assign.right = con;
                            } else if (codeout[op1->u.var_no + op1offset].kind == NAC || codeout[op2->u.var_no + op2offset].kind == NAC) {
                                codeout[res->u.var_no + resoffset].kind = NAC;
                                codeout[res->u.var_no + resoffset].val = 0;
                                if (codeout[op1->u.var_no + op1offset].kind == CONST && op1->kind != CONSTANT) {
                                    Operand* con = newConst(codeout[op1->u.var_no + op1offset].val);
                                    codeptr->code.u.binop.op1 = con;
                                }
                                if (codeout[op2->u.var_no + op2offset].kind == CONST && op2->kind != CONSTANT) {
                                    Operand* con = newConst(codeout[op2->u.var_no + op2offset].val);
                                    codeptr->code.u.binop.op2 = con;
                                }
                            } else {
                                codeout[res->u.var_no + resoffset].kind = UNDEF;
                                codeout[res->u.var_no + resoffset].val = 0;
                            }
                            break;
                        case SUB:
                            if (op1->kind == CONSTANT && op2->kind == CONSTANT) {
                                codeout[res->u.var_no + resoffset].kind = CONST;
                                codeout[res->u.var_no + resoffset].val = op1->u.value - op2->u.value;
                                Operand* con = newConst(codeout[res->u.var_no + resoffset].val);
                                codeptr->code.kind = ASSIGN;
                                codeptr->code.type = NORMAL;
                                codeptr->code.u.assign.right = con;
                            } else if (op1->kind == CONSTANT && codeout[op2->u.var_no + op2offset].kind == CONST) {
                                codeout[res->u.var_no + resoffset].kind = CONST;
                                codeout[res->u.var_no + resoffset].val = op1->u.value - codeout[op2->u.var_no + op2offset].val;
                                Operand* con = newConst(codeout[res->u.var_no + resoffset].val);
                                codeptr->code.kind = ASSIGN;
                                codeptr->code.type = NORMAL;
                                codeptr->code.u.assign.right = con;
                            } else if (codeout[op1->u.var_no + op1offset].kind == CONST && op2->kind == CONSTANT) {
                                codeout[res->u.var_no + resoffset].kind = CONST;
                                codeout[res->u.var_no + resoffset].val = codeout[op1->u.var_no + op1offset].val - op2->u.value;
                                Operand* con = newConst(codeout[res->u.var_no + resoffset].val);
                                codeptr->code.kind = ASSIGN;
                                codeptr->code.type = NORMAL;
                                codeptr->code.u.assign.right = con;
                            } else if (codeout[op1->u.var_no + op1offset].kind == CONST && codeout[op2->u.var_no + op2offset].kind == CONST) {
                                codeout[res->u.var_no + resoffset].kind = CONST;
                                codeout[res->u.var_no + resoffset].val = codeout[op1->u.var_no + op1offset].val - codeout[op2->u.var_no + op2offset].val;
                                Operand* con = newConst(codeout[res->u.var_no + resoffset].val);
                                codeptr->code.kind = ASSIGN;
                                codeptr->code.type = NORMAL;
                                codeptr->code.u.assign.right = con;
                            } else if (codeout[op1->u.var_no + op1offset].kind == NAC || codeout[op2->u.var_no + op2offset].kind == NAC) {
                                codeout[res->u.var_no + resoffset].kind = NAC;
                                codeout[res->u.var_no + resoffset].val = 0;
                                if (codeout[op1->u.var_no + op1offset].kind == CONST && op1->kind != CONSTANT) {
                                    Operand* con = newConst(codeout[op1->u.var_no + op1offset].val);
                                    codeptr->code.u.binop.op1 = con;
                                }
                                if (codeout[op2->u.var_no + op2offset].kind == CONST && op2->kind != CONSTANT) {
                                    Operand* con = newConst(codeout[op2->u.var_no + op2offset].val);
                                    codeptr->code.u.binop.op2 = con;
                                }
                            } else {
                                codeout[res->u.var_no + resoffset].kind = UNDEF;
                                codeout[res->u.var_no + resoffset].val = 0;
                            }
                            break;
                        case MUL:
                            if (op1->kind == CONSTANT && op2->kind == CONSTANT) {
                                codeout[res->u.var_no + resoffset].kind = CONST;
                                codeout[res->u.var_no + resoffset].val = op1->u.value * op2->u.value;
                                Operand* con = newConst(codeout[res->u.var_no + resoffset].val);
                                codeptr->code.kind = ASSIGN;
                                codeptr->code.type = NORMAL;
                                codeptr->code.u.assign.right = con;
                            } else if (op1->kind == CONSTANT && codeout[op2->u.var_no + op2offset].kind == CONST) {
                                codeout[res->u.var_no + resoffset].kind = CONST;
                                codeout[res->u.var_no + resoffset].val = op1->u.value * codeout[op2->u.var_no + op2offset].val;
                                Operand* con = newConst(codeout[res->u.var_no + resoffset].val);
                                codeptr->code.kind = ASSIGN;
                                codeptr->code.type = NORMAL;
                                codeptr->code.u.assign.right = con;
                            } else if (codeout[op1->u.var_no + op1offset].kind == CONST && op2->kind == CONSTANT) {
                                codeout[res->u.var_no + resoffset].kind = CONST;
                                codeout[res->u.var_no + resoffset].val = codeout[op1->u.var_no + op1offset].val * op2->u.value;
                                Operand* con = newConst(codeout[res->u.var_no + resoffset].val);
                                codeptr->code.kind = ASSIGN;
                                codeptr->code.type = NORMAL;
                                codeptr->code.u.assign.right = con;
                            } else if (codeout[op1->u.var_no + op1offset].kind == CONST && codeout[op2->u.var_no + op2offset].kind == CONST) {
                                codeout[res->u.var_no + resoffset].kind = CONST;
                                codeout[res->u.var_no + resoffset].val = codeout[op1->u.var_no + op1offset].val * codeout[op2->u.var_no + op2offset].val;
                                Operand* con = newConst(codeout[res->u.var_no + resoffset].val);
                                codeptr->code.kind = ASSIGN;
                                codeptr->code.type = NORMAL;
                                codeptr->code.u.assign.right = con;
                            } else if (codeout[op1->u.var_no + op1offset].kind == NAC || codeout[op2->u.var_no + op2offset].kind == NAC) {
                                codeout[res->u.var_no + resoffset].kind = NAC;
                                codeout[res->u.var_no + resoffset].val = 0;
                                if (codeout[op1->u.var_no + op1offset].kind == CONST && op1->kind != CONSTANT) {
                                    Operand* con = newConst(codeout[op1->u.var_no + op1offset].val);
                                    codeptr->code.u.binop.op1 = con;
                                }
                                if (codeout[op2->u.var_no + op2offset].kind == CONST && op2->kind != CONSTANT) {
                                    Operand* con = newConst(codeout[op2->u.var_no + op2offset].val);
                                    codeptr->code.u.binop.op2 = con;
                                }
                            } else {
                                codeout[res->u.var_no + resoffset].kind = UNDEF;
                                codeout[res->u.var_no + resoffset].val = 0;
                            }
                            break;
                        case DIV:
                            if (op1->kind == CONSTANT && op2->kind == CONSTANT) {
                                codeout[res->u.var_no + resoffset].kind = CONST;
                                codeout[res->u.var_no + resoffset].val = op1->u.value / op2->u.value;
                                Operand* con = newConst(codeout[res->u.var_no + resoffset].val);
                                codeptr->code.kind = ASSIGN;
                                codeptr->code.type = NORMAL;
                                codeptr->code.u.assign.right = con;
                            } else if (op1->kind == CONSTANT && codeout[op2->u.var_no + op2offset].kind == CONST) {
                                if (codeout[op2->u.var_no + op2offset].val == 0 || (op1->u.value ^ codeout[op2->u.var_no + op2offset].val) < 0) {
                                    codeout[res->u.var_no + resoffset].kind = NAC;
                                    codeout[res->u.var_no + resoffset].val = 0;
                                } else {
                                    codeout[res->u.var_no + resoffset].kind = CONST;
                                    codeout[res->u.var_no + resoffset].val = op1->u.value / codeout[op2->u.var_no + op2offset].val;
                                    Operand* con = newConst(codeout[res->u.var_no + resoffset].val);
                                    codeptr->code.kind = ASSIGN;
                                    codeptr->code.type = NORMAL;
                                    codeptr->code.u.assign.right = con;
                                }
                            } else if (codeout[op1->u.var_no + op1offset].kind == CONST && op2->kind == CONSTANT) {
                                if ((codeout[op1->u.var_no + op1offset].val ^ op2->u.value) < 0) {
                                    codeout[res->u.var_no + resoffset].kind = NAC;
                                    codeout[res->u.var_no + resoffset].val = 0;
                                } else {
                                    codeout[res->u.var_no + resoffset].kind = CONST;
                                    codeout[res->u.var_no + resoffset].val = codeout[op1->u.var_no + op1offset].val / op2->u.value;
                                    Operand* con = newConst(codeout[res->u.var_no + resoffset].val);
                                    codeptr->code.kind = ASSIGN;
                                    codeptr->code.type = NORMAL;
                                    codeptr->code.u.assign.right = con;
                                }
                            } else if (codeout[op1->u.var_no + op1offset].kind == CONST && codeout[op2->u.var_no + op2offset].kind == CONST) {
                                if (codeout[op2->u.var_no + op2offset].val == 0 || (codeout[op1->u.var_no + op1offset].val ^ codeout[op2->u.var_no + op2offset].val) < 0) {
                                    codeout[res->u.var_no + resoffset].kind = NAC;
                                    codeout[res->u.var_no + resoffset].val = 0;
                                } else {
                                    codeout[res->u.var_no + resoffset].kind = CONST;
                                    codeout[res->u.var_no + resoffset].val = codeout[op1->u.var_no + op1offset].val / codeout[op2->u.var_no + op2offset].val;
                                    Operand* con = newConst(codeout[res->u.var_no + resoffset].val);
                                    codeptr->code.kind = ASSIGN;
                                    codeptr->code.type = NORMAL;
                                    codeptr->code.u.assign.right = con;
                                }
                            } else if (codeout[op1->u.var_no + op1offset].kind == NAC || codeout[op2->u.var_no + op2offset].kind == NAC) {
                                codeout[res->u.var_no + resoffset].kind = NAC;
                                codeout[res->u.var_no + resoffset].val = 0;
                                if (codeout[op1->u.var_no + op1offset].kind == CONST && op1->kind != CONSTANT) {
                                    Operand* con = newConst(codeout[op1->u.var_no + op1offset].val);
                                    codeptr->code.u.binop.op1 = con;
                                }
                                if (codeout[op2->u.var_no + op2offset].kind == CONST && op2->kind != CONSTANT) {
                                    Operand* con = newConst(codeout[op2->u.var_no + op2offset].val);
                                    codeptr->code.u.binop.op2 = con;
                                }
                            } else {
                                codeout[res->u.var_no + resoffset].kind = UNDEF;
                                codeout[res->u.var_no + resoffset].val = 0;
                            }
                            break;
                        }
                    } else if (res->type == ADDRESS || op1->type == ADDRESS || op2->type == ADDRESS) {
                        if (res->kind == TEMP) {
                            resoffset = varCount - 1;
                        }
                        codeout[res->u.var_no + resoffset].kind = NAC;
                        codeout[res->u.var_no + resoffset].val = 0;
                        int op1offset = 0, op2offset = 0;
                        if (op1->type == VAL && op1->kind != CONSTANT) {
                            if (op1->kind == TEMP) {
                                op1offset = varCount - 1;
                            }
                            if (codeout[op1->u.var_no + op1offset].kind == CONST) {
                                Operand* con = newConst(codeout[op1->u.var_no + op1offset].val);
                                codeptr->code.u.binop.op1 = con;
                            }
                        }
                        if (op2->type == VAL && op2->kind != CONSTANT) {
                            if (op2->kind == TEMP) {
                                op2offset = varCount - 1;
                            }
                            if (codeout[op2->u.var_no + op2offset].kind == CONST) {
                                Operand* con = newConst(codeout[op2->u.var_no + op2offset].val);
                                codeptr->code.u.binop.op2 = con;
                            }
                        }
                    }
                    // if (res->kind == PARAMETER) {
                    //     codeout[res->u.var_no].kind = NAC;
                    //     codeout[res->u.var_no].val = 0;
                    // }
                } break;
                case CALL: {
                    Operand* res = codeptr->code.u.sinop.res;
                    bool resassign = false;
                    int resoffset = 0;
                    if (res->kind == VARIABLE || res->kind == PARAMETER)
                        resassign = true;
                    else if (res->kind == TEMP) {
                        resassign = true;
                        resoffset = varCount - 1;
                    }
                    if (resassign) {
                        codeout[res->u.var_no + resoffset].kind = NAC;
                        codeout[res->u.var_no + resoffset].val = 0;
                    }
                } break;
                case READ: {
                    Operand* op = codeptr->code.u.single.op;
                    bool opassign = false;
                    int opoffset = 0;
                    if (op->kind == VARIABLE || op->kind == PARAMETER)
                        opassign = true;
                    else if (op->kind == TEMP) {
                        opassign = true;
                        opoffset = varCount - 1;
                    }
                    if (opassign) {
                        codeout[op->u.var_no + opoffset].kind = NAC;
                        codeout[op->u.var_no + opoffset].val = 0;
                    }
                } break;
                case PARAM: {
                    Operand* op = codeptr->code.u.single.op;
                    codeout[op->u.var_no].kind = NAC;
                    codeout[op->u.var_no].val = 0;
                } break;
                case ARG:
                case RETURN:
                case WRITE: {
                    Operand* op = codeptr->code.u.single.op;
                    if (op->type == VAL) {
                        int opoffset = 0;
                        if (op->kind == TEMP)
                            opoffset = varCount - 1;
                        if (codeout[op->u.var_no + opoffset].kind == CONST) {
                            Operand* con = newConst(codeout[op->u.var_no + opoffset].val);
                            codeptr->code.u.single.op = con;
                        }
                    }
                } break;
                case IF: {
                    Operand* op1 = codeptr->code.u.cond.op1;
                    Operand* op2 = codeptr->code.u.cond.op2;
                    int op1offset = 0, op2offset = 0;
                    if (op1->kind == TEMP)
                        op1offset = varCount - 1;
                    if (op2->kind == TEMP)
                        op2offset = varCount - 1;
                    if (op1->kind != CONSTANT && codeout[op1->u.var_no + op1offset].kind == CONST) {
                        Operand* con = newConst(codeout[op1->u.var_no + op1offset].val);
                        codeptr->code.u.cond.op1 = con;
                    }
                    if (op2->kind != CONSTANT && codeout[op2->u.var_no + op2offset].kind == CONST) {
                        Operand* con = newConst(codeout[op2->u.var_no + op2offset].val);
                        codeptr->code.u.cond.op2 = con;
                    }
                } break;
                default:
                    break;
                }
                codeptr = codeptr->next;
            }
            if (block->id == p->blocknum)
                break;
            block = block->next;
        }
        // 收尾
        p = p->next;
        for (int i = 0; i < m; i++) {
            free(out[i]);
            free(in[i]);
        }
        free(out);
        free(in);
    }
}
