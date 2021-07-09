#include "semantic.h"
#include <assert.h>

char msg[1024];
int collision = 0;
unsigned char semright = 1;

TableList* hashTable[HASHSIZE + 1];

unsigned hash_pjw(char* name)
{
    unsigned val = 0, i;
    for (; *name; ++name) {
        val = (val << 2) + *name;
        if (i = val & ~HASHSIZE)
            val = (val ^ (i >> 12)) & HASHSIZE;
    }
    return val;
}

void initTable()
{
    for (int i = 0; i < HASHSIZE + 1; ++i) {
        hashTable[i] = NULL;
    }
    Type* type = (Type*)malloc(sizeof(Type));
    type->kind = INT;
    Type* functype = (Type*)malloc(sizeof(Type));
    functype->kind = FUNC;
    functype->u.function.argc = 0;
    functype->u.function.args = NULL;
    functype->u.function.ret = type;
    TableList* item = (TableList*)malloc(sizeof(TableList));
    item->name = "read";
    item->next = NULL;
    item->op = NULL;
    item->size = 0;
    item->type = functype;
    insert(item);
    FieldList* arg = (FieldList*)malloc(sizeof(FieldList));
    arg->name = "";
    arg->tail = NULL;
    arg->type = type;
    functype = (Type*)malloc(sizeof(Type));
    functype->kind = FUNC;
    functype->u.function.argc = 1;
    functype->u.function.args = arg;
    functype->u.function.ret = type;
    item = (TableList*)malloc(sizeof(TableList));
    item->name = "write";
    item->next = NULL;
    item->op = NULL;
    item->size = 0;
    item->type = functype;
    insert(item);
}

TableList* search(char* name)
{
    unsigned index = hash_pjw(name);
    for (TableList* node = hashTable[index]; node; node = node->next) {
        if (!strcmp(name, node->name))
            return node;
    }
    return NULL;
}

void insert(TableList* item)
{
    unsigned index = hash_pjw(item->name);
    item->next = hashTable[index];
    hashTable[index] = item;
}

int structcmp(FieldList* s1, FieldList* s2)
{
    int res = 0;
    while (s1 && s2) {
        if (s1 == s2)
            return 1;
        else if (s1->type->kind != s2->type->kind) {
            return 0;
        } else if (s1->type->kind == STRUCTVAR || s1->type->kind == STRUCTURE) {
            res = structcmp(s1->type->u.structvar, s2->type->u.structvar);
            if (!res)
                return 0;
        } else if (s1->type->kind == ARRAY) {
            res = arraycmp(s1->type, s2->type);
            if (!res)
                return 0;
        } else {
            res = 1;
        }
        s1 = s1->tail;
        s2 = s2->tail;
    }
    if (s1 || s2)
        res = 0;
    return res;
}

int arraycmp(Type* t1, Type* t2)
{
    int res;
    while (t1 && t2) {
        if (t1 == t2)
            return 1;
        else if (t1->u.array.elem->kind != t2->u.array.elem->kind) {
            return 0;
        } else if (t1->u.array.elem->kind == STRUCTURE || t1->u.array.elem->kind == STRUCTVAR) {
            res = structcmp(t1->u.array.elem->u.structure, t2->u.array.elem->u.structure);
            if (!res)
                return 0;
            else
                return 1;
        } else if (t1->u.array.elem->kind != ARRAY) {
            return 1;
        }
        t1 = t1->u.array.elem;
        t2 = t2->u.array.elem;
    }
    if (t1 || t2)
        res = 0;
    return res;
}

int funccmp(FieldList* arg1, FieldList* arg2)
{
    int res;
    while (arg1 && arg2) {
        if (arg1 == arg2)
            return 1;
        else if (arg1->type->kind != arg2->type->kind) {
            return 0;
        } else if (arg1->type->kind == STRUCTVAR) {
            res = structcmp(arg1->type->u.structvar, arg2->type->u.structvar);
            if (!res)
                return 0;
        } else if (arg1->type->kind == ARRAY) {
            res = arraycmp(arg1->type, arg2->type);
            if (!res)
                return 0;
        } else
            res = 1;
        arg1 = arg1->tail;
        arg2 = arg2->tail;
    }
    if (arg1 || arg2)
        res = 0;
    return res;
}

void Program(Node* node)
{
    if (!strcmp("ExtDefList", node->child->name)) {
        ExtDefList(node->child);
    }
}

void ExtDefList(Node* node)
{
    if (node->no == 2) // ExtDef ExtDefList
    {
        ExtDef(node->child);
        ExtDefList(childAt(node, 1));
    }
}

void ExtDef(Node* node)
{
    // printf("ExtDef\n");
    Type* type = (Type*)malloc(sizeof(Type));
    FieldList* attr = (FieldList*)malloc(sizeof(FieldList));
    attr->type = type;
    attr->tail = NULL;
    switch (node->no) {
    case 1: // Specifier ExtDecList SEMI
        node->child->inh = attr;
        Specifier(node->child);
        childAt(node, 1)->inh = node->child->syn;
        ExtDecList(childAt(node, 1));
        break;
    case 2: // Specifier SEMI
        node->child->inh = attr;
        Specifier(node->child);
        break;
    case 3: // Specifier FunDec CompSt
        type->kind = FUNC;
        node->child->inh = attr;
        childAt(node, 1)->inh = attr;
        Specifier(node->child);
        if (node->child->syn->type->kind == STRUCTURE) {
            Type* type = (Type*)malloc(sizeof(Type));
            type->kind = STRUCTVAR;
            type->u.structvar = node->child->syn->type->u.structure;
            node->child->inh->type->u.function.ret = type;
        } else {
            node->child->inh->type->u.function.ret = node->child->syn->type;
        }
        node->child->inh->type->u.function.argc = 0;
        node->child->inh->type->u.function.args = NULL;
        FunDec(childAt(node, 1));
        childAt(node, 2)->inh = node->child->syn;
        CompSt(childAt(node, 2));
        break;
    default:
        break;
    }
}

void Specifier(Node* node)
{
    // printf("Specifier\n");
    Type* type = (Type*)malloc(sizeof(Type));
    switch (node->no) {
    case 1: // TYPE
        if (!strcmp("int", node->child->val.type_str)) {
            type->kind = INT;
        } else {
            type->kind = FLOAT;
        }
        FieldList* attr = (FieldList*)malloc(sizeof(FieldList));
        attr->type = type;
        attr->tail = NULL;
        node->syn = attr;
        break;
    case 2: // StructSpecifier
    {
        type->kind = STRUCTURE;
        type->u.structure = NULL;
        FieldList* attr = (FieldList*)malloc(sizeof(FieldList));
        attr->type = type;
        attr->tail = NULL;
        node->child->inh = attr;
        StructSpecifier(node->child);
        node->syn = node->child->syn;
    } break;
    default:
        break;
    }
}

void ExtDecList(Node* node)
{
    // printf("ExtDecList\n");
    if (node->inh->type->kind == STRUCTURE && !node->inh->type->u.structure) {
        sprintf(msg, "Undefined structure \"%s\"", node->inh->name);
        error(17, node->line, msg);
        return;
    }
    node->child->inh = node->inh;
    VarDec(node->child);
    node->syn = node->child->syn;
    switch (node->no) {
    case 1: // VarDec
        break;
    case 2: // VarDec COMMA ExtDecList
    {
        childAt(node, 2)->inh = node->inh;
        ExtDecList(childAt(node, 2));
        FieldList* p = node->syn;
        while (p->tail) {
            p = p->tail;
        }
        p->tail = childAt(node, 2)->syn;
    } break;
    default:
        break;
    }
}

void FunDec(Node* node)
{
    // printf("FunDec\n");
    TableList* res = search(node->child->val.type_str);
    if (!res) {
        TableList* item = (TableList*)malloc(sizeof(TableList));
        item->name = node->child->val.type_str;
        item->next = NULL;
        item->op = NULL;
        item->size = 0;
        item->type = node->inh->type;
        insert(item);
    } else {
        sprintf(msg, "Redefined function \"%s\"", node->child->val.type_str);
        error(4, node->line, msg);
        node->inh->type->kind = WRONGFUNC;
    }
    switch (node->no) {
    case 1: // ID LP VarList RP
        childAt(node, 2)->inh = node->inh;
        VarList(childAt(node, 2));
        node->inh->type->u.function.args = childAt(node, 2)->syn;
        break;
    case 2: // ID LP RP
        node->inh->type->u.function.args = NULL;
        break;
    default:
        break;
    }
}

void CompSt(Node* node)
{
    // printf("CompSt\n");
    childAt(node, 1)->inh = node->inh;
    childAt(node, 2)->inh = node->inh;
    DefList(childAt(node, 1));
    StmtList(childAt(node, 2));
}

void VarDec(Node* node)
{
    // printf("VarDec\n");
    TableList* res;
    assert(node->inh);
    switch (node->no) {
    case 1: // ID
        res = search(node->child->val.type_str);
        if (!strcmp(node->child->val.type_str, "id_jAh9_Lg")) {
            int a = 0;
        }
        assert(node->inh);
        if (!res) {
            TableList* item = (TableList*)malloc(sizeof(TableList));
            item->name = node->child->val.type_str;
            item->next = NULL;
            item->op = NULL;
            item->size = 0;
            FieldList* attr = (FieldList*)malloc(sizeof(FieldList));
            switch (node->inh->type->kind) {
            case INT:
                item->type = node->inh->type;
                attr->name = node->child->val.type_str;
                attr->type = node->inh->type;
                attr->tail = NULL;
                node->syn = attr;
                item->size = 4;
                break;
            case FLOAT:
                item->type = node->inh->type;
                attr->name = node->child->val.type_str;
                attr->type = node->inh->type;
                attr->tail = NULL;
                node->syn = attr;
                item->size = 4;
                break;
            case ARRAY:
                item->type = node->inh->type;
                attr->name = node->child->val.type_str;
                attr->type = node->inh->type;
                attr->tail = NULL;
                node->syn = attr;
                break;
            case STRUCTVAR:
            case STRUCTURE: {
                if (!node->inh->type->u.structure) {
                    sprintf(msg, "Undefined structure \"%s\"", node->inh->name);
                    error(17, node->line, msg);
                    node->syn = NULL;
                    return;
                }
                Type* type = (Type*)malloc(sizeof(Type));
                type->kind = STRUCTVAR;
                type->u.structvar = node->inh->type->u.structure;
                attr->name = node->child->val.type_str;
                attr->type = type;
                attr->tail = NULL;
                item->type = type;
                node->syn = attr;
            } break;
            default:
                printf("name: %s, type: %d\n", node->child->val.type_str, node->inh->type->kind);
                exit(-1);
                break;
            }
            insert(item);
        } else {
            if (node->instruct) {
                sprintf(msg, "Redefined field \"%s\"", node->child->val.type_str);
                error(15, node->line, msg);
            } else {
                sprintf(msg, "Redefined variable \"%s\"", node->child->val.type_str);
                error(3, node->line, msg);
            }
        }
        break;
    case 2: // VarDec LB INT RB
    {
        if (node->instruct)
            node->child->instruct = 1;
        Type* type = (Type*)malloc(sizeof(Type));
        type->kind = ARRAY;
        type->u.array.size = childAt(node, 2)->val.type_int;
        if (node->inh->type->kind == STRUCTURE) {
            Type* p = (Type*)malloc(sizeof(Type));
            p->kind = STRUCTVAR;
            p->u.structvar = node->inh->type->u.structure;
            type->u.array.elem = p;
        } else {
            type->u.array.elem = node->inh->type;
        }
        FieldList* attr = (FieldList*)malloc(sizeof(FieldList));
        attr->tail = NULL;
        attr->type = type;
        node->child->inh = attr;
        VarDec(node->child);
        node->syn = node->child->syn;
        break;
    }
    default:
        break;
    }
}

void StructSpecifier(Node* node)
{
    // printf("StructSpecifier\n");
    assert(node->inh);
    assert(node->inh->type->kind == STRUCTURE);
    switch (node->no) {
    case 1: // STRUCT OptTag LC DefList RC
    {
        Type* type = (Type*)malloc(sizeof(Type));
        type->kind = STRUCTURE;
        FieldList* field = (FieldList*)malloc(sizeof(FieldList));
        field->tail = NULL;
        field->type = type;
        childAt(node, 1)->inh = node->inh;
        childAt(node, 3)->inh = node->inh;
        childAt(node, 3)->instruct = 1;
        OptTag(childAt(node, 1));
        DefList(childAt(node, 3));
        if (!node->inh->type->u.structure) {
            node->inh->type->u.structure = childAt(node, 3)->syn;
        } else {
            FieldList* p = node->inh->type->u.structure;
            if (p->type->kind == STRUCTURE) {
                p->type->u.structure = field;
            } else {
                p->tail = field;
            }
        }
        node->syn = node->inh;
    } break;
    case 2: // STRUCT Tag
        node->inh->type->u.structure = NULL;
        childAt(node, 1)->inh = node->inh;
        Tag(childAt(node, 1));
        node->syn = node->inh;
        break;
    default:
        break;
    }
}

void OptTag(Node* node)
{
    // printf("OptTag\n");
    assert(node->inh);
    if (node->no == 2) // ID
    {
        TableList* res = search(node->child->val.type_str);
        if (!res) {
            TableList* item = (TableList*)malloc(sizeof(TableList));
            item->name = node->child->val.type_str;
            item->next = NULL;
            item->op = NULL;
            item->size = 0;
            assert(node->inh->type->kind == STRUCTURE);
            item->type = node->inh->type;
            node->inh->name = node->child->val.type_str;
            insert(item);
        } else {
            sprintf(msg, "Duplicated name \"%s\"", node->child->val.type_str);
            error(16, node->line, msg);
        }
    }
}

void DefList(Node* node)
{
    // printf("DefList\n");
    assert(node->inh);
    if (node->no == 2) // Def DefList
    {
        if (node->instruct) {
            node->child->instruct = 1;
            childAt(node, 1)->instruct = 1;
        }
        node->child->inh = node->inh;
        childAt(node, 1)->inh = node->inh;
        Def(node->child);
        node->syn = node->child->syn;
        DefList(childAt(node, 1));
        FieldList* p = node->syn;
        if (!p)
            return;
        while (p->tail) {
            p = p->tail;
        }
        p->tail = childAt(node, 1)->syn;
    }
}

void Tag(Node* node)
{
    // printf("Tag\n");
    assert(node->inh);
    TableList* res = search(node->child->val.type_str);
    if (!res) {
        node->inh->name = node->child->val.type_str;
    } else {
        node->inh->name = res->name;
        node->inh->type = res->type;
    }
}

void VarList(Node* node)
{
    // printf("VarList\n");
    ParamDec(node->child);
    if (node->child->syn)
        node->inh->type->u.function.argc++;
    else {
        node->inh->type->kind = WRONGFUNC;
        return;
    }
    node->syn = node->child->syn;
    switch (node->no) {
    case 1: // ParamDec COMMA VarList
    {
        childAt(node, 2)->inh = node->inh;
        VarList(childAt(node, 2));
        FieldList* p = node->syn;
        while (p->tail) {
            p = p->tail;
        }
        p->tail = childAt(node, 2)->syn;
    } break;
    case 2: // ParamDec
        break;
    default:
        break;
    }
}

void ParamDec(Node* node)
{
    // printf("ParamDec\n");
    Specifier(node->child);
    childAt(node, 1)->inh = node->child->syn;
    VarDec(childAt(node, 1));
    node->syn = childAt(node, 1)->syn;
    if (childAt(node, 1)->syn) {
        node->syn = (FieldList*)malloc(sizeof(FieldList));
        node->syn->name = childAt(node, 1)->syn->name;
        node->syn->type = childAt(node, 1)->syn->type;
        node->syn->tail = NULL;
    } else {
        node->syn = NULL;
    }
}

void StmtList(Node* node)
{
    // printf("StmtList\n");
    if (node->no == 2) {
        node->child->inh = node->inh;
        childAt(node, 1)->inh = node->inh;
        Stmt(node->child);
        StmtList(childAt(node, 1));
    }
}

void Stmt(Node* node)
{
    // printf("Stmt\n");
    switch (node->no) {
    case 1: // Exp SEMI
        Exp(node->child);
        break;
    case 2: // CompSt
        node->child->inh = node->inh;
        CompSt(node->child);
        break;
    case 3: // RETURN Exp SEMI
        Exp(childAt(node, 1));
        if (childAt(node, 1)->syn) {
            if (node->inh->type->kind != childAt(node, 1)->syn->type->kind) {
                if (node->inh->type->kind == STRUCTURE && childAt(node, 1)->syn->type->kind == STRUCTVAR) {
                    int res = structcmp(node->inh->type->u.structure, childAt(node, 1)->syn->type->u.structvar);
                    if (res)
                        break;
                }
                error(8, childAt(node, 1)->line, "Type mismatched for return");
            }
        }
        break;
    case 4: // IF LP Exp RP Stmt
        childAt(node, 4)->inh = node->inh;
        Exp(childAt(node, 2));
        if (childAt(node, 2)->syn) {
            if (childAt(node, 2)->syn->type->kind != INT) {
                error(7, childAt(node, 2)->line, "Type mismatched for operands");
            }
        }
        Stmt(childAt(node, 4));
        break;
    case 5: // IF LP Exp RP Stmt ELSE Stmt
        childAt(node, 4)->inh = node->inh;
        childAt(node, 6)->inh = node->inh;
        Exp(childAt(node, 2));
        if (childAt(node, 2)->syn) {
            if (childAt(node, 2)->syn->type->kind != INT) {
                error(7, childAt(node, 2)->line, "Type mismatched for operands");
            }
        }
        Stmt(childAt(node, 4));
        Stmt(childAt(node, 6));
        break;
    case 6: // WHILE LP Exp RP Stmt
        childAt(node, 4)->inh = node->inh;
        Exp(childAt(node, 2));
        if (childAt(node, 2)->syn) {
            if (childAt(node, 2)->syn->type->kind != INT) {
                error(7, childAt(node, 2)->line, "Type mismatched for operands");
            }
        }
        Stmt(childAt(node, 4));
        break;
    default:
        break;
    }
}

void Exp(Node* node)
{
    // printf("Exp\n");
    TableList* res;
    switch (node->no) {
    case 1: // Exp ASSIGNOP Exp
    {
        int id = node->child->no;
        if (id < 14 || id > 16) {
            error(6, node->child->line, "The left-hand side of an assignment must be a variable");
            Exp(childAt(node, 2));
            return;
        } else {
            Exp(node->child);
        }
        Exp(childAt(node, 2));
        if (node->child->syn && childAt(node, 2)->syn) {
            if (node->child->syn->type->kind != childAt(node, 2)->syn->type->kind) {
                error(5, node->child->line, "Type mismatched for assignment");
            } else if (node->child->syn->type->kind == STRUCTVAR) {
                FieldList* p = node->child->syn->type->u.structvar;
                FieldList* q = childAt(node, 2)->syn->type->u.structvar;
                int res = structcmp(p, q);
                if (!res) {
                    error(5, node->child->line, "Type mismatched for assignment");
                } else {
                    node->syn = node->child->syn;
                }
            } else if (node->child->syn->type->kind == ARRAY) {
                Type* p = node->child->syn->type;
                Type* q = childAt(node, 2)->syn->type;
                int res = arraycmp(p, q);
                if (!res) {
                    error(5, node->child->line, "Type mismatched for assignment");
                } else {
                    node->syn = node->child->syn;
                }
            } else {
                node->syn = node->child->syn;
            }
        } else {
            error(5, node->child->line, "Type mismatched for assignment");
        }
    } break;
    case 2: // Exp AND Exp
        Exp(node->child);
        Exp(childAt(node, 2));
        if (node->child->syn && childAt(node, 2)->syn) {
            if (node->child->syn->type->kind != INT || childAt(node, 2)->syn->type->kind != INT) {
                error(7, node->child->line, "Type mismatched for operands");
            } else {
                Type* type = (Type*)malloc(sizeof(Type));
                type->kind = INT;
                FieldList* attr = (FieldList*)malloc(sizeof(FieldList));
                attr->name = NULL;
                attr->type = type;
                attr->tail = NULL;
                node->syn = attr;
            }
        }
        break;
    case 3: // Exp OR Exp
        Exp(node->child);
        Exp(childAt(node, 2));
        if (node->child->syn && childAt(node, 2)->syn) {
            if (node->child->syn->type->kind != INT || childAt(node, 2)->syn->type->kind != INT) {
                error(7, node->child->line, "Type mismatched for operands");
            } else {
                Type* type = (Type*)malloc(sizeof(Type));
                type->kind = INT;
                FieldList* attr = (FieldList*)malloc(sizeof(FieldList));
                attr->name = NULL;
                attr->type = type;
                attr->tail = NULL;
                node->syn = attr;
            }
        }
        break;
    case 4: // Exp RELOP Exp
        Exp(node->child);
        Exp(childAt(node, 2));
        if (node->child->syn && childAt(node, 2)->syn) {
            if (node->child->syn->type->kind == INT && childAt(node, 2)->syn->type->kind == INT || node->child->syn->type->kind == FLOAT && childAt(node, 2)->syn->type->kind == FLOAT) {
                Type* type = (Type*)malloc(sizeof(Type));
                type->kind = INT;
                FieldList* attr = (FieldList*)malloc(sizeof(FieldList));
                attr->name = NULL;
                attr->type = type;
                attr->tail = NULL;
                node->syn = attr;
            } else {
                error(7, node->child->line, "Type mismatched for operands");
            }
        }
        break;
    case 5: // Exp PLUS Exp
        Exp(node->child);
        Exp(childAt(node, 2));
        if (node->child->syn && childAt(node, 2)->syn) {
            if (node->child->syn->type->kind == INT && childAt(node, 2)->syn->type->kind == INT || node->child->syn->type->kind == FLOAT && childAt(node, 2)->syn->type->kind == FLOAT) {
                Type* type = (Type*)malloc(sizeof(Type));
                type->kind = node->child->syn->type->kind;
                FieldList* attr = (FieldList*)malloc(sizeof(FieldList));
                attr->name = NULL;
                attr->type = type;
                attr->tail = NULL;
                node->syn = attr;
            } else {
                error(7, node->child->line, "Type mismatched for operands");
            }
        } else {
            error(7, node->child->line, "Type mismatched for operands");
        }
        break;
    case 6: // Exp MINUS Exp
        Exp(node->child);
        Exp(childAt(node, 2));
        if (node->child->syn && childAt(node, 2)->syn) {
            if (node->child->syn->type->kind == INT && childAt(node, 2)->syn->type->kind == INT || node->child->syn->type->kind == FLOAT && childAt(node, 2)->syn->type->kind == FLOAT) {
                Type* type = (Type*)malloc(sizeof(Type));
                type->kind = node->child->syn->type->kind;
                FieldList* attr = (FieldList*)malloc(sizeof(FieldList));
                attr->name = NULL;
                attr->type = type;
                attr->tail = NULL;
                node->syn = attr;
            } else {
                error(7, node->child->line, "Type mismatched for operands");
            }
        } else {
            error(7, node->child->line, "Type mismatched for operands");
        }
        break;
    case 7: // Exp STAR Exp
        Exp(node->child);
        Exp(childAt(node, 2));
        if (node->child->syn && childAt(node, 2)->syn) {
            if (node->child->syn->type->kind == INT && childAt(node, 2)->syn->type->kind == INT || node->child->syn->type->kind == FLOAT && childAt(node, 2)->syn->type->kind == FLOAT) {
                Type* type = (Type*)malloc(sizeof(Type));
                type->kind = node->child->syn->type->kind;
                FieldList* attr = (FieldList*)malloc(sizeof(FieldList));
                attr->name = NULL;
                attr->type = type;
                attr->tail = NULL;
                node->syn = attr;
            } else {
                error(7, node->child->line, "Type mismatched for operands");
            }
        } else {
            error(7, node->child->line, "Type mismatched for operands");
        }
        break;
    case 8: // Exp DIV Exp
        Exp(node->child);
        Exp(childAt(node, 2));
        if (node->child->syn && childAt(node, 2)->syn) {
            if (node->child->syn->type->kind == INT && childAt(node, 2)->syn->type->kind == INT || node->child->syn->type->kind == FLOAT && childAt(node, 2)->syn->type->kind == FLOAT) {
                Type* type = (Type*)malloc(sizeof(Type));
                type->kind = node->child->syn->type->kind;
                FieldList* attr = (FieldList*)malloc(sizeof(FieldList));
                attr->name = NULL;
                attr->type = type;
                attr->tail = NULL;
                node->syn = attr;
            } else {
                error(7, node->child->line, "Type mismatched for operands");
            }
        } else {
            error(7, node->child->line, "Type mismatched for operands");
        }
        break;
    case 9: // LP Exp RP
        Exp(childAt(node, 1));
        node->syn = childAt(node, 1)->syn;
        break;
    case 10: // MINUS Exp
        Exp(childAt(node, 1));
        if (childAt(node, 1)->syn) {
            if (childAt(node, 1)->syn->type->kind == INT || childAt(node, 1)->syn->type->kind == FLOAT) {
                node->syn = childAt(node, 1)->syn;
            } else {
                error(7, node->line, "Type mismatched for operands");
            }
        }
        break;
    case 11: // NOT Exp
        Exp(childAt(node, 1));
        if (childAt(node, 1)->syn) {
            if (childAt(node, 1)->syn->type->kind == INT) {
                node->syn = childAt(node, 1)->syn;
            } else {
                error(7, node->line, "Type mismatched for operands");
            }
        }
        break;
    case 12: // ID LP Args RP
        res = search(node->child->val.type_str);
        Args(childAt(node, 2));
        if (!childAt(node, 2)->syn) {
            node->syn = NULL;
            return;
        }
        if (res) {
            if (res->type->kind != FUNC) {
                sprintf(msg, "\"%s\" is not a function", res->name);
                error(11, node->line, msg);
            } else {
                int tmp = funccmp(res->type->u.function.args, childAt(node, 2)->syn);
                if (tmp) {
                    FieldList* attr = (FieldList*)malloc(sizeof(FieldList));
                    attr->tail = NULL;
                    attr->name = NULL;
                    attr->type = res->type->u.function.ret;
                    node->syn = attr;
                } else {
                    sprintf(msg, "Function \"%s\" is not applicable for arguments", res->name);
                    error(9, childAt(node, 2)->line, msg);
                }
            }
        } else {
            sprintf(msg, "Undefined function \"%s\"", node->child->val.type_str);
            error(2, node->line, msg);
            return;
        }
        break;
    case 13: // ID LP RP
        res = search(node->child->val.type_str);
        if (res) {
            if (res->type->kind != FUNC) {
                sprintf(msg, "\"%s\" is not a function", res->name);
                error(11, node->line, msg);
            } else if (res->type->u.function.argc != 0) {
                sprintf(msg, "Function \"%s\" is not applicable for arguments", res->name);
                error(9, node->line, msg);
            } else {
                FieldList* attr = (FieldList*)malloc(sizeof(FieldList));
                attr->tail = NULL;
                attr->name = NULL;
                attr->type = res->type->u.function.ret;
                node->syn = attr;
            }
        } else {
            sprintf(msg, "Undefined function \"%s\"", node->child->val.type_str);
            error(2, node->line, msg);
        }
        break;
    case 14: // Exp LB Exp RB
    {
        Exp(node->child);
        FieldList* attr = node->child->syn;
        if (!attr)
            return;
        if (attr->type->kind != ARRAY) {
            sprintf(msg, "\"%s\" is not a array", attr->name);
            error(10, node->child->line, msg);
        } else {
            FieldList* p = (FieldList*)malloc(sizeof(FieldList));
            p->tail = NULL;
            p->name = attr->name;
            p->type = attr->type->u.array.elem;
            node->syn = p;
        }
        Exp(childAt(node, 2));
        attr = childAt(node, 2)->syn;
        if (!attr)
            return;
        if (attr->type->kind != INT) {
            sprintf(msg, "Index of array \"%s\" is not an integer", node->child->syn->name);
            error(12, childAt(node, 2)->line, msg);
        }
    } break;
    case 15: // Exp DOT ID
    {
        Exp(node->child);
        FieldList* attr = node->child->syn;
        if (!attr)
            return;
        if (attr->type->kind != STRUCTVAR) {
            error(13, node->child->line, "Illegal use of \".\"");
        } else {
            int flag = 0;
            for (FieldList* p = attr->type->u.structvar; p; p = p->tail) {
                if (!strcmp(childAt(node, 2)->val.type_str, p->name)) {
                    flag = 1;
                    node->syn = p;
                    break;
                }
            }
            if (!flag) {
                sprintf(msg, "Non-existent field \"%s\"", childAt(node, 2)->val.type_str);
                error(14, node->child->line, msg);
            }
        }
    } break;
    case 16: // ID
        res = search(node->child->val.type_str);
        if (res) {
            FieldList* attr = (FieldList*)malloc(sizeof(FieldList));
            attr->tail = NULL;
            attr->type = res->type;
            attr->name = res->name;
            node->syn = attr;
        } else {
            sprintf(msg, "Undefined Variable \"%s\"", node->child->val.type_str);
            error(1, node->line, msg);
        }
        break;
    case 17: // INT
    {
        Type* type = (Type*)malloc(sizeof(Type));
        type->kind = INT;
        FieldList* attr = (FieldList*)malloc(sizeof(FieldList));
        attr->tail = NULL;
        attr->type = type;
        attr->name = NULL;
        node->syn = attr;
    } break;
    case 18: // FLOAT
    {
        Type* type = (Type*)malloc(sizeof(Type));
        type->kind = FLOAT;
        FieldList* attr = (FieldList*)malloc(sizeof(FieldList));
        attr->tail = NULL;
        attr->type = type;
        attr->name = NULL;
        node->syn = attr;
    } break;
    default:
        break;
    }
}

void Def(Node* node)
{
    // Specifier DecList SEMI
    // printf("Def\n");
    assert(node->inh);
    if (node->instruct) {
        node->child->instruct = 1;
        childAt(node, 1)->instruct = 1;
    }
    node->child->inh = node->inh;
    Specifier(node->child);
    childAt(node, 1)->inh = node->child->syn;
    DecList(childAt(node, 1));
    node->syn = childAt(node, 1)->syn;
}

void DecList(Node* node)
{
    // printf("DecList\n");
    assert(node->inh);
    if (node->instruct)
        node->child->instruct = 1;
    if (node->inh->type->kind == STRUCTURE && !node->inh->type->u.structure) {
        sprintf(msg, "Undefined structure \"%s\"", node->inh->name);
        error(17, node->line, msg);
        return;
    }
    node->child->inh = node->inh;
    Dec(node->child);
    node->syn = node->child->syn;
    switch (node->no) {
    case 1: // Dec
        break;
    case 2: // Dec COMMA DecList
    {
        if (node->instruct)
            childAt(node, 2)->instruct = 1;
        childAt(node, 2)->inh = node->inh;
        DecList(childAt(node, 2));
        FieldList* p = node->syn;
        while (p->tail) {
            p = p->tail;
        }
        p->tail = childAt(node, 2)->syn;
    } break;
    default:
        break;
    }
}

void Dec(Node* node)
{
    // printf("Dec\n");
    assert(node->inh);
    if (node->instruct)
        node->child->instruct = 1;
    node->child->inh = node->inh;
    VarDec(node->child);
    node->syn = node->child->syn;
    switch (node->no) {
    case 1: // VarDec
        break;
    case 2: // VarDec ASSIGNOP Exp
        if (node->instruct) {
            sprintf(msg, "Try to init field \"%s\" in structure", node->child->syn->name);
            error(15, node->child->line, msg);
            return;
        }
        Exp(childAt(node, 2));
        if (node->child->syn && childAt(node, 2)->syn) {
            if (node->child->syn->type->kind != childAt(node, 2)->syn->type->kind) {
                error(5, node->child->line, "Type mismatched for assignment");
            }
        }
        break;
    default:
        break;
    }
}

void Args(Node* node)
{
    // printf("Args\n");
    Exp(node->child);
    if (node->child->syn) {
        node->syn = (FieldList*)malloc(sizeof(FieldList));
        node->syn->name = node->child->syn->name;
        node->syn->type = node->child->syn->type;
        node->syn->tail = NULL;
    } else {
        node->syn = NULL;
    }
    switch (node->no) {
    case 1: // Exp COMMA Args
        Args(childAt(node, 2));
        if (node->syn)
            node->syn->tail = childAt(node, 2)->syn;
        break;
    case 2: // Exp
        break;
    default:
        break;
    }
}

void printType(Type* type, int depth)
{
    assert(type);
    for (int i = 0; i < depth; ++i)
        printf("  ");
    printf("type: ");
    switch (type->kind) {
    case INT:
        printf("int\n");
        break;
    case FLOAT:
        printf("float\n");
        break;
    case ARRAY:
        printf("array, size: %d\n", type->u.array.size);
        printType(type->u.array.elem, depth + 1);
        break;
    case STRUCTURE: {
        printf("structure\n");
        FieldList* p = type->u.structure;
        while (p) {
            for (int i = 0; i < depth + 1; ++i)
                printf("  ");
            printf("name: %s\n", p->name);
            printType(p->type, depth + 1);
            p = p->tail;
        }
        break;
    }
    case STRUCTVAR: {
        printf("structvar\n");
        FieldList* p = type->u.structvar;
        while (p) {
            for (int i = 0; i < depth + 1; ++i)
                printf("  ");
            printf("name: %s\n", p->name);
            printType(p->type, depth + 1);
            p = p->tail;
        }
    } break;
    case FUNC: {
        printf("function\n");
        Type* ret = type->u.function.ret;
        for (int i = 0; i < depth + 1; ++i)
            printf("  ");
        printf("return:\n");
        printType(ret, depth + 2);
        for (int i = 0; i < depth + 1; ++i)
            printf("  ");
        printf("argc: %d\n", type->u.function.argc);
        for (int i = 0; i < depth + 1; ++i)
            printf("  ");
        printf("args:\n");
        FieldList* p = type->u.function.args;
        while (p) {
            for (int i = 0; i < depth + 2; ++i)
                printf("  ");
            printf("name: %s\n", p->name);
            printType(p->type, depth + 2);
            p = p->tail;
        }
    } break;
    default:
        // printf("%d\n", type->kind);
        break;
    }
}

void printTable()
{
    printf("---------------------------\n");
    for (int i = 0; i < HASHSIZE + 1; ++i) {
        if (!hashTable[i])
            continue;
        else {
            TableList* p = hashTable[i];
            if (p->next) {
                ++collision;
            }
            while (p) {
                if (p->type->kind != WRONGFUNC) {
                    printf("name: %s\n", p->name);
                    printType(p->type, 0);
                    printf("---------------------------\n");
                }
                p = p->next;
            }
        }
    }
    printf("collision: %d\n", collision);
}

void error(int type, int line, char* msg)
{
    printf("Error type %d at Line %d: %s.\n", type, line, msg);
    semright = 0;
}
