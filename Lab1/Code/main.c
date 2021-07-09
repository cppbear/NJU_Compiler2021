#include <stdio.h>
//#include "syntax.tab.c"
//#include "lex.yy.c"
//extern FILE *yyin;

void perror(const char *__s);
int yyrestart();
int yyparse();
int yydebug;
extern unsigned char right;
extern struct Node *root;
extern int syserr, myerr;
void outPut(struct Node *node, int dep);

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
        outPut(root, 0);
    }
    else if (syserr > myerr)
    {
        printf("\n");
    }
    return 0;
}
