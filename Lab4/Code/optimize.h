#include "intermediate.h"

typedef struct BasicBlock BasicBlock;
typedef struct BasicBlocks BasicBlocks;
typedef struct EntryList EntryList;
typedef struct ValueType ValueType;

struct BasicBlock {
    int id;
    InterCodes* first;
    InterCodes* last;
    BasicBlock* prev;
    BasicBlock* next;
    BasicBlocks* predecessor;
    BasicBlocks* successor;
};

struct BasicBlocks {
    BasicBlock* block;
    BasicBlocks* next;
};

struct EntryList {
    BasicBlock* entry;
    BasicBlock* exit;
    int blocknum;
    EntryList* next;
};

struct ValueType {
    enum {UNDEF, NAC, CONST} kind;
    long long int val;
};

void printCode(InterCodes* ir);
void printCodes(InterCodes* begin, InterCodes* end, int full);
void printDelete();
void printBlocks();
void printFlow();
void printEntry();
void initBasicBlockList();
void addBasicBlock(BasicBlock* block);
BasicBlocks* newBasicBlocks(BasicBlock* block);
void addEntry(BasicBlock* entry);
void addPredSucc(BasicBlock* pre, BasicBlock* suc);
BasicBlock* findLabel(int no);
void buildBasicBlocks();
void buildFlow();
void optimize();
void liveVariables();
void constProp();
void inlineFunc();
