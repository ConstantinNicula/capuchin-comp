#include "unity.h"
#include "vector.h"
#include "utils.h"

void setUp(void) {
    // set stuff up here
}

void tearDown(void) {
    // clean stuff up here
}

static void cleanupStr(char** str){
    if (!(*str)) return;
    free(*str);
    *str = NULL;
}

static char* copyStr(char* str) {
    return cloneString(str);
}

void vectorTestBasic() {
    Vector_t* vec = createVector();
    cleanupVector(&vec, (VectorElemCleanupFn_t)cleanupStr);    
}

void vectorTestInsert() {
    Vector_t* vec = createVector();

    vectorAppend(vec, cloneString("my value 0"));
    vectorAppend(vec, cloneString("my value 1"));
    vectorAppend(vec, cloneString("my value 2"));  
    vectorAppend(vec, cloneString("my value 3"));
    vectorAppend(vec, cloneString("my value 4"));

    TEST_ASSERT_EQUAL_INT_MESSAGE(5, vec->cnt, "Wrong item count");  
    TEST_ASSERT_EQUAL_INT_MESSAGE(7, vec->cap, "Wrong capacity");  
    TEST_ASSERT_EQUAL_STRING("my value 0", vec->buf[0]);
    TEST_ASSERT_EQUAL_STRING("my value 1", vec->buf[1]);
    TEST_ASSERT_EQUAL_STRING("my value 2", vec->buf[2]);
    TEST_ASSERT_EQUAL_STRING("my value 3", vec->buf[3]);
    TEST_ASSERT_EQUAL_STRING("my value 4", vec->buf[4]);

    cleanupVector(&vec, (VectorElemCleanupFn_t)cleanupStr);
}


void vectorTestCopy() {
    Vector_t* vec = createVector();
    vectorAppend(vec, cloneString("my value 0"));
    vectorAppend(vec, cloneString("my value 1"));
    vectorAppend(vec, cloneString("my value 2"));  
    vectorAppend(vec, cloneString("my value 3"));
    vectorAppend(vec, cloneString("my value 4"));

    Vector_t* newVec = copyVector(vec,(VectorElemCopyFn_t)copyStr);
    TEST_ASSERT_EQUAL_INT_MESSAGE(5, vec->cnt, "Wrong item count");  
    TEST_ASSERT_EQUAL_INT_MESSAGE(7, vec->cap, "Wrong capacity");  
    TEST_ASSERT_EQUAL_STRING("my value 0", vec->buf[0]);
    TEST_ASSERT_EQUAL_STRING("my value 1", vec->buf[1]);
    TEST_ASSERT_EQUAL_STRING("my value 2", vec->buf[2]);
    TEST_ASSERT_EQUAL_STRING("my value 3", vec->buf[3]);
    TEST_ASSERT_EQUAL_STRING("my value 4", vec->buf[4]);

    cleanupVector(&newVec, (VectorElemCleanupFn_t)cleanupStr);
    cleanupVector(&vec, (VectorElemCleanupFn_t)cleanupStr);

}


// not needed when using generate_test_runner.rb
int main(void) {
   UNITY_BEGIN();
   RUN_TEST(vectorTestBasic);
   RUN_TEST(vectorTestInsert);
   RUN_TEST(vectorTestCopy);
   return UNITY_END();
}