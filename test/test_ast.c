#include <malloc.h>
#include "unity.h"
#include "ast.h"


void setUp(void) {
    // set stuff up here
}

void tearDown(void) {
    // clean stuff up here
}

void ast_create_test(void) {
    Program_t* prog = createProgram();
    const char* lt = "let";
    const char* name = "myVar";

    LetStatement_t* ls = createLetStatement(createToken(TOKEN_IDENT, lt, 3u));
    ls->name = createIdentifier(createToken(TOKEN_IDENT, name, 5u), name);
    ls->value = NULL;
    programAppendStatement(prog, (Statement_t*)ls);

    char* prog_str = programToString(prog);
    TEST_ASSERT_EQUAL_INT_MESSAGE(1u, programGetStatementCount(prog), "Invalid statement count");    
    TEST_ASSERT_EQUAL_STRING_MESSAGE("let myVar = ;", prog_str, "Invalid program string");
    
    cleanupProgram(&prog);
    free(prog_str);
}



// not needed when using generate_test_runner.rb
int main(void) {
    UNITY_BEGIN();
    RUN_TEST(ast_create_test);
    return UNITY_END();
}