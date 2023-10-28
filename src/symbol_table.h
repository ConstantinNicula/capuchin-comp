#ifndef _SYMBOL_TABLE_H_
#define _SYMBOL_TABLE_H_
#include "hmap.h"
#include "vector.h"
#include <stdint.h>

typedef enum SymbolScope {
    SCOPE_GLOBAL,
    SCOPE_LOCAL,
    SCOPE_BUILTIN,
    SCOPE_FREE,
    _NUM_SYMBOL_SCOPES
} SymbolScope_t;

const char* symbolScopeToString(SymbolScope_t scope); 

typedef struct Symbol {
    char* name;
    SymbolScope_t scope;
    uint32_t index;
} Symbol_t;

Symbol_t* createSymbol(const char* name, SymbolScope_t scope, uint32_t index);
void cleanupSymbol(Symbol_t** sym);



DEFINE_VECTOR_TYPE(Symbol, Symbol_t*);

typedef struct SymbolTable {
    struct SymbolTable* outer; 
    HashMap_t* store;
    uint32_t numDefinitions;
    VectorSymbol_t* freeSymbols; 
} SymbolTable_t;

SymbolTable_t* createSymbolTable();
SymbolTable_t* createEnclosedSymbolTable(SymbolTable_t* outer);
void cleanupSymbolTable(SymbolTable_t* symTable);

Symbol_t* symbolTableDefine(SymbolTable_t* symTable, const char* name);
Symbol_t* symbolTableDefineBuiltin(SymbolTable_t* symTable, uint32_t index, const char* name);
Symbol_t* symbolTableResolve(SymbolTable_t* symTable, const char* name);

#endif