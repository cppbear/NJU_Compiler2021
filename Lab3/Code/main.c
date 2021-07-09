#include "optimize.h"

void perror(const char* __s);
int yyrestart();
int yyparse();
int yydebug;
bool optimization = false;
extern unsigned char right;
extern unsigned char semright;
extern unsigned char interight;
extern Node* root;
extern TableList* hashTable[HASHSIZE];
extern int syserr, myerr;

int main(int argc, char** argv)
{
    if (argc <= 1)
        return 1;
    FILE* f = fopen(argv[1], "r");
    if (!f) {
        perror(argv[1]);
        return 1;
    }
    yyrestart(f);
    yydebug = 1;
    yyparse();
    if (right) {
        initTable();
        Program(root);
        if (semright) {
            setVariable();
            initInterCodes();
            tranProgram(root);
            if (interight) {
                optimize();
                writeInterCodes(argv[2], optimization);
            }
        }
    }
    return 0;
}
