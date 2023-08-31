#include "unity.h"
#include "slice.h"
#include "utils.h"

void setUp(void) {
    // set stuff up here
}

void tearDown(void) {
    // clean stuff up here
}
void sliceTestBasic() {
    SliceByte_t instructions = createSliceByte(4);
    instructions[0] = 12;
    instructions[1] = 13;
    instructions[2] = 14;
    instructions[3] = 15;
    cleanupSliceByte(&instructions);
}

void sliceTestGetLen() {
    SliceByte_t temp = createSliceByte(100);
    TEST_ASSERT_EQUAL_size_t(100, sliceByteGetLen(temp));
    cleanupSliceByte(&temp);
}

void sliceTestAppend() {
    SliceByte_t sa = createSliceByte(10);
    for (int i = 0; i < sliceByteGetLen(sa); i++) {
        sa[i] = i;
    }

    SliceByte_t sb = createSliceByte(10);
    for (int i = 0; i < sliceByteGetLen(sb); i++) {
        sb[i] = i + 10;
    }

    sliceByteAppend(&sa, sb, 10);
    TEST_ASSERT_EQUAL_size_t(20, sliceByteGetLen(sa));
    
    for (int i = 0; i < 20 ; i ++) {
        TEST_ASSERT_EQUAL_UINT8(i, sa[i]);
    }
    cleanupSliceByte(&sa);
    cleanupSliceByte(&sb);
}

// not needed when using generate_test_runner.rb
int main(void) {
   UNITY_BEGIN();
   RUN_TEST(sliceTestBasic);
   RUN_TEST(sliceTestGetLen);
   RUN_TEST(sliceTestAppend);
   return UNITY_END();
}