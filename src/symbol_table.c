#include "symbol_table.h"
#include "utils.h"

const char* symbolScopeStr[_NUM_SYMBOL_SCOPES] = {
   [SCOPE_GLOBAL] = "SCOPE_GLOBAL", 
};
const char* symbolScopeToString(SymbolScope_t scope) {
    if (scope < 0 || scope >= _NUM_SYMBOL_SCOPES)
        return NULL;
    return symbolScopeStr[scope];
} 

Symbol_t* createSymbol(const char* name, SymbolScope_t scope, uint32_t index) {
    Symbol_t* sym = mallocChk(sizeof(Symbol_t));
    *sym = (Symbol_t) {
        .index = index,
        .name = cloneString(name),
        .scope = scope
    };
    return sym;
}

void cleanupSymbol(Symbol_t** sym) {
    if ((*sym) == NULL) return;
    free((*sym)->name);
    free((*sym));
    *sym = NULL;
}

SymbolTable_t* createSymbolTable() {
    return createEnclosedSymbolTable(NULL);
}

SymbolTable_t* createEnclosedSymbolTable(SymbolTable_t* outer) {
    SymbolTable_t* symTable = mallocChk(sizeof(SymbolTable_t));
    *symTable = (SymbolTable_t) {
        .outer = outer,
        .store = createHashMap(),
        .numDefinitions = 0
    };
    return symTable;
}

void cleanupSymbolTable(SymbolTable_t* symTable) {
    if (symTable == NULL) return;
    cleanupHashMap(&(symTable->store), (HashMapElemCleanupFn_t) cleanupSymbol);
    free(symTable);
}


Symbol_t* symbolTableDefine(SymbolTable_t* symTable, const char* name) {
    Symbol_t* sym = NULL;
    if (!symTable->outer) {
        sym = createSymbol(name, SCOPE_GLOBAL, symTable->numDefinitions);
    } else {
        sym = createSymbol(name, SCOPE_LOCAL, symTable->numDefinitions);
    }

    hashMapInsert(symTable->store, name, sym);
    symTable->numDefinitions++;
    return sym;
}


Symbol_t* symbolTableResolve(SymbolTable_t* symTable, const char* name) {
    Symbol_t* sym = hashMapGet(symTable->store, name);
    if (!sym && symTable->outer) {
        return symbolTableResolve(symTable->outer, name);
    }
    return sym;
}