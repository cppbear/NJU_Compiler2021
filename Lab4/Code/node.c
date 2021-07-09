#include "node.h"

Node *newNode(char *_name, int _no, MyType _type, ...)
{
    va_list list;
    Node *res = (Node *)malloc(sizeof(Node));
    res->name = _name;
    res->type = _type;
    res->no = _no;
    res->syn = res->inh = NULL;
    res->parent = res->child = res->next = NULL;
    res->instruct = 0;
    va_start(list, _type);
    switch (_type)
    {
    case Int:
    {
        if (*res->name == 'O')
            sscanf(va_arg(list, char *), "%ou", &res->val.type_int);
        else if (*res->name == 'H')
            sscanf(va_arg(list, char *), "%xu", &res->val.type_int);
        else
            res->val.type_int = (unsigned)atoi(va_arg(list, char *));
        break;
    }
    case Float:
        res->val.type_float = (float)atof(va_arg(list, char *));
        break;
    case Id:
    case TYpe:
    case Relop:
    case Ter:
        strcpy(res->val.type_str, va_arg(list, char *));
        break;
    case Nter:
    {
        int num = va_arg(list, int);
        res->line = va_arg(list, int);
        res->child = va_arg(list, Node *);
        res->child->parent = res;
        Node *tmp = res->child;
        for (int i = 0; i < num - 1; i++)
        {
            tmp->next = va_arg(list, Node *);
            tmp = tmp->next;
        }
        break;
    }
    case Null:
        break;
    default:
        printf("Wrong Type: %s\n", res->name);
        break;
    }
    va_end(list);
    return res;
}

void outPut(Node *node, int dep)
{
    if (node->type != Null)
    {
        for (int i = 0; i < dep; i++)
        {
            printf("  ");
        }
    }
    Node *p = node;
    switch (p->type)
    {
    case Nter:
        printf("%s (%d)\n", p->name, p->line);
        break;
    case Relop:
    case Ter:
        printf("%s\n", p->name);
        break;
    case TYpe:
        printf("TYPE: %s\n", p->val.type_str);
        break;
    case Id:
        printf("ID: %s\n", p->val.type_str);
        break;
    case Float:
        printf("FLOAT: %f\n", p->val.type_float);
        break;
    case Int:
        printf("INT: %u\n", p->val.type_int);
        break;
    case Null:
        break;
    default:
        printf("Wrong Type: %s\n", p->name);
        break;
    }
    if (p->child)
    {
        outPut((Node *)p->child, dep + 1);
    }
    if (p->next)
    {
        outPut((Node *)p->next, dep);
    }
}

Node *childAt(Node *node, int index)
{
    Node *p = node->child;
    for (int i = 0; i < index; ++i)
    {
        if (!p)
        {
            printf("Index of child out of range!\n");
            exit(-1);
        }
        p = p->next;
    }
    return p;
}