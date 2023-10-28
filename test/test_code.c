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
        {.op = OP_ADD, .operands={}, .expLen=1, .expBytes={(uint8_t)OP_ADD}},
        {.op = OP_GET_LOCAL, .operands={255}, .expLen=2, .expBytes={(uint8_t)OP_GET_LOCAL, 255}},
        {.op = OP_CLOSURE, .operands={65534, 255l}, .expLen=4, .expBytes={(uint8_t)OP_CLOSURE, 255, 254, 255}}
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

void testCodeReadOperands() {
    typedef struct TestCase {
        OpCode_t op;
        int numOperands; 
        int operands[OP_MAX_ARGS];
        int bytesRead;
    } TestCase_t;

    TestCase_t testCases[] = {
        {.op = OP_CONSTANT, .numOperands=1, .operands={65535}, .bytesRead=2},
        {.op = OP_GET_LOCAL, .numOperands=1, .operands={255}, .bytesRead=1},
        {.op = OP_CLOSURE, .numOperands=2, .operands={65534, 255}, .bytesRead=3},
    };
    int numTestCases = sizeof(testCases) / sizeof(testCases[0]);

    for (int i = 0; i < numTestCases; i++) {
        SliceByte_t instruction = codeMake(testCases[i].op, testCases[i].operands);

        const OpDefinition_t* def = opLookup(testCases[i].op);
        TEST_NOT_NULL(def, "Definition not found");

        uint8_t bytesRead = 0;  
        SliceInt_t operandsRead = codeReadOperands(def, &instruction[1], &bytesRead);
        TEST_INT(testCases[i].bytesRead, bytesRead, "wrong number of bytes read");

        for (int j = 0; j < testCases[i].numOperands; j++) {
            TEST_INT(testCases[i].operands[j], operandsRead[j], "Operand wrong");
        }

        cleanupSliceByte(instruction);
        cleanupSliceInt(operandsRead);
    }
}

void testInstructionsString() {
    SliceByte_t instructions[] = {
        codeMakeV(OP_ADD), 
        codeMakeV(OP_GET_LOCAL, 1),
        codeMakeV(OP_CONSTANT, 2),
        codeMakeV(OP_CONSTANT, 65535),
        codeMakeV(OP_CLOSURE, 65535, 255),
    };
    int numInstructions = sizeof(instructions) / sizeof(instructions[0]);

    const char* expected = "0000 OpAdd\n"
                        "0001 OpGetLocal 1\n"
                        "0003 OpConstant 2\n"
                        "0006 OpConstant 65535\n"
                        "0009 OpClosure 65535 255\n";

    SliceByte_t concatted = createSliceByte(0);
    for (int i = 0; i < numInstructions; i++) {
        sliceByteAppend(&concatted, instructions[i], sliceByteGetLen(instructions[i]));
    }

    char* res = instructionsToString(concatted);
    TEST_STRING(expected, res, "instructions wrongly formatted");
    
    cleanupSliceByte(concatted);
    for(int i = 0;  i < numInstructions; i++) {
        cleanupSliceByte(instructions[i]);
    }
    free(res);
}

// not needed when using generate_test_runner.rb
int main(void) {
    UNITY_BEGIN();
    RUN_TEST(testCodeMake);
    RUN_TEST(testInstructionsString);
    RUN_TEST(testCodeReadOperands);
    return UNITY_END();
}