#include "unity.h"
#include "utils.h"
#include "code.h"
#include "test_helper.h"


void setUp(void) {
    // set stuff up here
}

void tearDown(void) {
    // clean stuff up here
}

void testCodeMake() {
    #define OP_MAX_BYTE_LEN 12
    typedef struct TestCase {
        OpCode_t op;
        int operands[OP_MAX_ARGS]; 
        uint8_t expLen;
        uint8_t expBytes[OP_MAX_BYTE_LEN];
    }TestCase_t;

    TestCase_t testCases[] = {
        {.op = OP_CONSTANT, .operands={65534}, .expLen=3, .expBytes={(uint8_t)OP_CONSTANT, 255, 254}},
    };

    int numTestCases = sizeof(testCases) / sizeof(testCases[0]);
    for (int i = 0; i < numTestCases; i++) {
        SliceByte_t instruction = codeMake(testCases[i].op, testCases[i].operands);

        // Check length 
        TEST_INT(testCases[i].expLen, sliceByteGetLen(instruction), "Instruction has wrong length");
        // Check buffer 
        TEST_BYTES(testCases[i].expBytes, instruction, testCases[i].expLen, "Wrong byte at pos"); 

        cleanupSliceByte(instruction);
    }
}

// not needed when using generate_test_runner.rb
int main(void) {
   UNITY_BEGIN();
   RUN_TEST(testCodeMake);
   return UNITY_END();
}