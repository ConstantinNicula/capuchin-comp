#include "symbol_table.h"
#include "utils.h"

IMPL_VECTOR_TYPE(Symbol, Symbol_t*);

const char* symbolScopeStr[_NUM_SYMBOL_SCOPES] = {
   [SCOPE_GLOBAL] = "SCOPE_GLOBAL", 
   [SCOPE_LOCAL] = "SCOPE_LOCAL", 
   [SCOPE_FREE] = "SCOPE_FREE", 
   [SCOPE_BUILTIN] = "SCOPE_BUILTIN", 
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
        .numDefinitions = 0,
        .freeSymbols = createVectorSymbol(), 
    };
    return symTable;
}

void cleanupSymbolTable(SymbolTable_t* symTable) {
    if (symTable == NULL) return;
    cleanupHashMap(&(symTable->store), (HashMapElemCleanupFn_t) cleanupSymbol);
    cleanupVectorSymbol(&(symTable->freeSymbols), NULL); // ref other definitions
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

Symbol_t* symbolTableDefineBuiltin(SymbolTable_t* symTable, uint32_t index, const char* name) {
    Symbol_t* sym = createSymbol(name, SCOPE_BUILTIN, index);
    hashMapInsert(symTable->store, name, sym);
    return sym;
}

Symbol_t* symbolTableDefineFree(SymbolTable_t* symTable, Symbol_t* original) {
    vectorSymbolAppend(symTable->freeSymbols, original);

    Symbol_t* symbol = createSymbol(original->name, SCOPE_FREE, 
                          vectorSymbolGetCount(symTable->freeSymbols)-1);

    hashMapInsert(symTable->store, original->name, symbol);
    return symbol;
}

Symbol_t* symbolTableResolve(SymbolTable_t* symTable, const char* name) {
    Symbol_t* sym = hashMapGet(symTable->store, name);
    if (!sym && symTable->outer) {
        sym = symbolTableResolve(symTable->outer, name);
        
        if (!sym) return NULL;
        if (sym->scope == SCOPE_GLOBAL || sym->scope == SCOPE_BUILTIN) {
            return sym;
        }

        return symbolTableDefineFree(symTable, sym);
    }
    return sym;
}