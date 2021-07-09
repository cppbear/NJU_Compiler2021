#include "mips.h"

void perror(const char* __s);
int yyrestart();
int yyparse();
int yydebug;
bool optimization = false;
extern unsigned char right;
extern unsigned char semright;
extern unsigned char interight;
extern Node* root;
extern TableList* hashTable[HASHSIZE + 1];
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
        //printTable();
        if (semright) {
            setVariable();
            initInterCodes();
            tranProgram(root);
            if (interight) {
                optimize();
                if (argc == 4) {
                    writeInterCodes(argv[2], optimization);
                    initAssembly();
                    MIPS(argv[3]);
                } else if (argc == 3) {
                    initAssembly();
                    MIPS(argv[2]);
                }
            }
        }
    }
    return 0;
}
