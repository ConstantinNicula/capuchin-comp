#include <malloc.h>
#include "unity.h"
#include "sbuf.h"

void setUp(void) {
    // set stuff up here
}

void tearDown(void) {
    // clean stuff up here
}

void sbuf_simple(void) {
    Strbuf_t* sbuf = createStrbuf();
    strbufWrite(sbuf, "hello ");
    strbufWrite(sbuf, "World");
    strbufWrite(sbuf, "!");
    TEST_ASSERT_EQUAL_STRING("hello World!", detachStrbuf(&sbuf));
}




// not needed when using generate_test_runner.rb
int main(void) {
    UNITY_BEGIN();
    RUN_TEST(sbuf_simple);
    return UNITY_END();
}