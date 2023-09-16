#include "unity.h"
#include "symbol_table.h"
#include "utils.h"
#include "test_helper.h"

void setUp(void) {
    // set stuff up here
}

void tearDown(void) {
    // clean stuff up here
}

bool compareSymbol(Symbol_t* exp, Symbol_t* actual);

void testDefine() {
    Symbol_t expected[] = {
        {.name="a", .scope=SCOPE_GLOBAL, .index=0},
        {.name="b", .scope=SCOPE_GLOBAL, .index=1}
    };
    int numTests = sizeof(expected)/sizeof(expected[0]);
    
    SymbolTable_t* global = createSymbolTable();

    for (int i = 0 ; i < numTests; i++) {
        Symbol_t *actual = symbolTableDefine(global, expected[i].name);
        compareSymbol(&expected[i], actual);
    }

    cleanupSymbolTable(global);
}

void testResolveGlobal() {
    SymbolTable_t *global = createSymbolTable();
    symbolTableDefine(global, "a");
    symbolTableDefine(global, "b");
    
    Symbol_t expected[] = {
        {.name="a", .scope=SCOPE_GLOBAL, .index=0},
        {.name="b", .scope=SCOPE_GLOBAL, .index=1}
    };
    int numTests = sizeof(expected)/sizeof(expected[0]);
    
    for (int i = 0; i < numTests; i++) {
        Symbol_t* result = symbolTableResolve(global, expected[i].name);
        compareSymbol(&expected[i], result);
    }

    cleanupSymbolTable(global);
}

bool compareSymbol(Symbol_t* exp, Symbol_t* actual) {
    TEST_NOT_NULL(actual, "Symbol is null, could not be resolved!");
    TEST_STRING(exp->name, actual->name, "Wrong symbol name");
    TEST_INT(exp->index, actual->index, "Wrong symbol index");
    TEST_INT(exp->scope, actual->scope, "Wrong symbol scope");
    return true;
}

// not needed when using generate_test_runner.rb
int main(void) {
   UNITY_BEGIN();
   RUN_TEST(testDefine);
   RUN_TEST(testResolveGlobal);
   return UNITY_END();
}