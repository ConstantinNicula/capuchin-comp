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

void testResolveLocal() {
    SymbolTable_t *global = createSymbolTable();
    symbolTableDefine(global, "a");
    symbolTableDefine(global, "b");
    
    SymbolTable_t *local = createEnclosedSymbolTable(global);
    symbolTableDefine(local, "c");
    symbolTableDefine(local, "d");    

    Symbol_t expected[] = {
        {.name="a", .scope=SCOPE_GLOBAL, .index=0},
        {.name="b", .scope=SCOPE_GLOBAL, .index=1},
        {.name="c", .scope=SCOPE_LOCAL, .index=0},
        {.name="d", .scope=SCOPE_LOCAL, .index=1},
    };

    int numTests = sizeof(expected)/sizeof(expected[0]);
    
    for (int i = 0; i < numTests; i++) {
        Symbol_t* result = symbolTableResolve(local, expected[i].name);
        compareSymbol(&expected[i], result);
    }
cleanupSymbolTable(local);
    cleanupSymbolTable(global);
}

void testResolveNestedLocal() {
    SymbolTable_t *global = createSymbolTable();
    symbolTableDefine(global, "a");
    symbolTableDefine(global, "b");

    SymbolTable_t *firstLocal = createEnclosedSymbolTable(global);
    symbolTableDefine(firstLocal, "c");
    symbolTableDefine(firstLocal, "d");    

    SymbolTable_t *secondLocal = createEnclosedSymbolTable(firstLocal);
    symbolTableDefine(secondLocal, "e");
    symbolTableDefine(secondLocal, "f");    

    typedef struct TestCase {
        SymbolTable_t* table;
        Symbol_t expected[25]; 
        int expectedCnt; 
    } TestCase_t;

    TestCase_t tests[] = {
        {
            .table = firstLocal, 
            .expectedCnt = 4, 
            .expected = {
                {.name="a", .scope=SCOPE_GLOBAL, .index=0},
                {.name="b", .scope=SCOPE_GLOBAL, .index=1},
                {.name="c", .scope=SCOPE_LOCAL, .index=0},
                {.name="d", .scope=SCOPE_LOCAL, .index=1},
            }
        },
        {
            .table = secondLocal, 
            .expectedCnt = 4, 
            .expected = {
                {.name="a", .scope=SCOPE_GLOBAL, .index=0},
                {.name="b", .scope=SCOPE_GLOBAL, .index=1},
                {.name="e", .scope=SCOPE_LOCAL, .index=0},
                {.name="f", .scope=SCOPE_LOCAL, .index=1},
            }
        }
    };

    int numTests = sizeof(tests)/sizeof(tests[0]);
    
    for (int i = 0; i < numTests; i++) {
        for (int j = 0; j < tests[i].expectedCnt; j++) {
            Symbol_t* result = symbolTableResolve(tests[i].table, 
                                                  tests[i].expected[j].name);
            compareSymbol(&tests[i].expected[j], result);
        }
   }

    cleanupSymbolTable(secondLocal);
    cleanupSymbolTable(firstLocal);
    cleanupSymbolTable(global);
}

void testDefine() {
    Symbol_t expected[] = {
        {.name="a", .scope=SCOPE_GLOBAL, .index=0},
        {.name="b", .scope=SCOPE_GLOBAL, .index=1},
        {.name="c", .scope=SCOPE_LOCAL, .index=0},
        {.name="d", .scope=SCOPE_LOCAL, .index=1},
        {.name="e", .scope=SCOPE_LOCAL, .index=0},
        {.name="f", .scope=SCOPE_LOCAL, .index=1},
    };

    SymbolTable_t* global = createSymbolTable();
    Symbol_t* a = symbolTableDefine(global, "a");
    compareSymbol(&expected[0], a);
    Symbol_t* b = symbolTableDefine(global, "b");
    compareSymbol(&expected[1], b);

    SymbolTable_t* firstLocal = createEnclosedSymbolTable(global);
    Symbol_t* c = symbolTableDefine(firstLocal, "c");
    compareSymbol(&expected[2], c);
    Symbol_t* d = symbolTableDefine(firstLocal, "d");
    compareSymbol(&expected[3], d);

    SymbolTable_t* secondLocal = createEnclosedSymbolTable(global);
    Symbol_t* e = symbolTableDefine(secondLocal, "e");
    compareSymbol(&expected[4], e);
    Symbol_t* f = symbolTableDefine(secondLocal, "f");
    compareSymbol(&expected[5], f);

    cleanupSymbolTable(secondLocal);
    cleanupSymbolTable(firstLocal);
    cleanupSymbolTable(global);
}

void testDefineResolveBuiltin() {
    SymbolTable_t* global = createSymbolTable();
    SymbolTable_t* firstLocal = createEnclosedSymbolTable(global);
    SymbolTable_t* secondLocal = createEnclosedSymbolTable(firstLocal);

    Symbol_t expected[] = {
        {.name="a", .scope=SCOPE_BUILTIN, .index=0},
        {.name="c", .scope=SCOPE_BUILTIN, .index=1},
        {.name="e", .scope=SCOPE_BUILTIN, .index=2},
        {.name="f", .scope=SCOPE_BUILTIN, .index=3},
    };
    int numExpected = sizeof(expected) / sizeof(expected[0]);

    SymbolTable_t* tables[] = { 
        global, firstLocal, secondLocal
    };
    int numTables = sizeof(tables) / sizeof(tables[0]);

    for (int j = 0; j < numExpected; j++) {
        symbolTableDefineBuiltin(global, j, expected[j].name);
    }

    for (int i = 0; i < numTables; i++) {
        for (int j = 0; j < numExpected; j++) {
            Symbol_t* result = symbolTableResolve(tables[i], expected[j].name);
            compareSymbol(&expected[j], result);
        }
   }

    cleanupSymbolTable(secondLocal);
    cleanupSymbolTable(firstLocal);
    cleanupSymbolTable(global);
}

void testResolveFree() {
    SymbolTable_t* global = createSymbolTable();
    symbolTableDefine(global, "a");
    symbolTableDefine(global, "b");

    SymbolTable_t* firstLocal = createEnclosedSymbolTable(global);
    symbolTableDefine(firstLocal, "c");
    symbolTableDefine(firstLocal, "d");

    SymbolTable_t* secondLocal = createEnclosedSymbolTable(firstLocal);
    symbolTableDefine(secondLocal, "e");
    symbolTableDefine(secondLocal, "f");

    typedef struct TestCase {
        SymbolTable_t* table;
        int numExpectedSymbols;
        Symbol_t expectedSymbols[10];

        int numExpectedFreeSymbols;
        Symbol_t expectedFreeSymbols[10];
    } TestCase_t;

    TestCase_t testCases[] = {
        {
            .table = firstLocal, 
            .numExpectedSymbols = 4, 
            .expectedSymbols = {
                {.name = "a", .scope = SCOPE_GLOBAL, .index = 0},
                {.name = "b", .scope = SCOPE_GLOBAL, .index = 1},
                {.name = "c", .scope = SCOPE_LOCAL, .index = 0},
                {.name = "d", .scope = SCOPE_LOCAL, .index = 1},
            },
            .numExpectedFreeSymbols = 0,
            .expectedFreeSymbols = {},
        },
        {
            .table = secondLocal, 
            .numExpectedSymbols = 6, 
            .expectedSymbols = {
                {.name = "a", .scope = SCOPE_GLOBAL, .index = 0},
                {.name = "b", .scope = SCOPE_GLOBAL, .index = 1},
                {.name = "c", .scope = SCOPE_FREE, .index = 0},
                {.name = "d", .scope = SCOPE_FREE, .index = 1},
                {.name = "e", .scope = SCOPE_LOCAL, .index = 0},
                {.name = "f", .scope = SCOPE_LOCAL, .index = 1},
            },
            .numExpectedFreeSymbols = 2,
            .expectedFreeSymbols = {
                {.name = "c", .scope = SCOPE_LOCAL, .index = 0},
                {.name = "d", .scope = SCOPE_LOCAL, .index = 1},
            },
        },
    };

    int numTestCases = sizeof(testCases) / sizeof(testCases[0]);

    for (int i = 0; i < numTestCases; i++) {
        for (int j = 0; j < testCases[i].numExpectedSymbols; j++) {
            Symbol_t* result = symbolTableResolve(testCases[i].table, 
                                                  testCases[i].expectedSymbols[j].name);
            compareSymbol(&testCases[i].expectedSymbols[j], result);
        }

        TEST_INT(testCases[i].numExpectedFreeSymbols, 
                vectorSymbolGetCount(testCases[i].table->freeSymbols),
                "Wrong number of free symbols"); 

        for (int j = 0; j < testCases[i].numExpectedFreeSymbols; j++) {
            compareSymbol(&testCases[i].expectedFreeSymbols[j],
                          vectorSymbolGetBuffer(testCases[i].table->freeSymbols)[j]);
        }
    }

    cleanupSymbolTable(secondLocal);
    cleanupSymbolTable(firstLocal);
    cleanupSymbolTable(global);
}

void testResolveUnresolvableFree() {
    SymbolTable_t* global = createSymbolTable();
    symbolTableDefine(global, "a");

    SymbolTable_t* firstLocal = createEnclosedSymbolTable(global);
    symbolTableDefine(firstLocal, "c");

    SymbolTable_t* secondLocal = createEnclosedSymbolTable(firstLocal);
    symbolTableDefine(secondLocal, "e");
    symbolTableDefine(secondLocal, "f");

    Symbol_t expected[]  =  {
        {.name = "a", .scope = SCOPE_GLOBAL, .index = 0}, 
        {.name = "c", .scope = SCOPE_FREE, .index = 0}, 
        {.name = "e", .scope = SCOPE_LOCAL, .index = 0}, 
        {.name = "f", .scope = SCOPE_LOCAL, .index = 1}, 
    };
    
    int numExpected = sizeof(expected) / sizeof(expected[0]);
    for (int i = 0; i < numExpected; i++) {
        Symbol_t* result = symbolTableResolve(secondLocal, expected[i].name);
        compareSymbol(&expected[i], result);
    }

    const char* expectedUnresolvable[] = {"b", "d"};
    int numExpectedUnresolvable = sizeof(expectedUnresolvable)/sizeof(expectedUnresolvable[0]);
    for (int i = 0; i < numExpectedUnresolvable; i++) {
        Symbol_t* result = symbolTableResolve(secondLocal, expectedUnresolvable[i]);
        TEST_ASSERT_MESSAGE(result == NULL, "symbol should not have been resolved");
    }

    cleanupSymbolTable(secondLocal);
    cleanupSymbolTable(firstLocal);
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
   RUN_TEST(testResolveLocal);
   RUN_TEST(testResolveNestedLocal);
   RUN_TEST(testDefine);
   RUN_TEST(testDefineResolveBuiltin);
   RUN_TEST(testResolveFree);
   RUN_TEST(testResolveUnresolvableFree);
   return UNITY_END();
}