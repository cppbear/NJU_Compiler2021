#include "semantic.h"

void perror(const char *__s);
int yyrestart();
int yyparse();
int yydebug;
extern unsigned char right;
extern Node *root;
extern TableList *hashTable[HASHSIZE];
extern int syserr, myerr;

int main(int argc, char **argv)
{
    if (argc <= 1)
        return 1;
    FILE *f = fopen(argv[1], "r");
    if (!f)
    {
        perror(argv[1]);
        return 1;
    }
    yyrestart(f);
    yydebug = 1;
    yyparse();
    if (right)
    {
        // outPut(root, 0);
        initTable();
        Program(root);
        // printTable();
    }
    return 0;
}
