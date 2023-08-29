#include "unity.h"
#include "token.h"


void setUp(void) {
    // set stuff up here
}

void tearDown(void) {
    // clean stuff up here
}

void token_create_test(void) {
    Token_t* tok = createToken(TOKEN_EOF, NULL, 0);
    TEST_ASSERT_NOT_NULL(tok);
    cleanupToken(&tok);
}


void token_create_test_2(void) {
    Token_t* tok = createToken(TOKEN_ASSIGN, "=", 1u);

    TEST_ASSERT_EQUAL_INT_MESSAGE(TOKEN_ASSIGN, tok->type, "Token type check");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("=", tok->literal, "Token literal check");
    cleanupToken(&tok);
}



void token_create_test_3(void) {
    Token_t* tok = createToken(TOKEN_ASSIGN, "123456", 3);

    TEST_ASSERT_EQUAL_INT_MESSAGE(TOKEN_ASSIGN, tok->type, "Token type check");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("123", tok->literal, "Token literal check");
    cleanupToken(&tok);
}




// not needed when using generate_test_runner.rb
int main(void) {
    UNITY_BEGIN();
    RUN_TEST(token_create_test);
    RUN_TEST(token_create_test_2);
    RUN_TEST(token_create_test_3);
    return UNITY_END();
}