#include "unity.h"
#include "utils.h"
#include "compiler.h"
#include "lexer.h"
#include "parser.h"
#include "test_helper.h"
#include "gc.h"

void setUp(void) {
    // set stuff up here
}

void tearDown(void) {
    // clean stuff up here
}

#define MAX_CONSTANTS 128
#define MAX_INSTRUCTIONS 128

typedef struct TestCase {
    const char* input;
    GenericExpect_t expConstants[MAX_CONSTANTS];
    SliceByte_t expInstructions[MAX_INSTRUCTIONS];  
} TestCase_t;

void runCompilerTests(TestCase_t* testCases, int numTestCases);
void testInstructions(SliceByte_t expected[], SliceByte_t actual);
SliceByte_t concatInstructions(SliceByte_t expected[], int num);
void testConstants(GenericExpect_t expected[], VectorObjects_t* actual); 

void testIntegerObject(int64_t expected, Object_t* obj); 

void cleanupInstructions(Instructions_t instr[]);

void testCompilerBasic() {
    TestCase_t testCases[] = {
        {
            .input = "1 + 2",
            .expConstants = {_INT(1), _INT(2), _END()},
            .expInstructions = {
                codeMakeV(OP_CONSTANT, 0),
                codeMakeV(OP_CONSTANT, 1),
                codeMakeV(OP_ADD),
                codeMakeV(OP_POP),
                NULL
            }
        },
        {
            .input = "1; 2",
            .expConstants = {_INT(1), _INT(2), _END()},
            .expInstructions = {
                codeMakeV(OP_CONSTANT, 0),
                codeMakeV(OP_POP),
                codeMakeV(OP_CONSTANT, 1),
                codeMakeV(OP_POP),
                NULL
            }
        },
        {
            .input = "1 - 2",
            .expConstants = {_INT(1), _INT(2), _END()},
            .expInstructions = {
                codeMakeV(OP_CONSTANT, 0),
                codeMakeV(OP_CONSTANT, 1),
                codeMakeV(OP_SUB),
                codeMakeV(OP_POP),
                NULL
            }
        }, 
        {
            .input = "1 * 2",
            .expConstants = {_INT(1), _INT(2), _END()},
            .expInstructions = {
                codeMakeV(OP_CONSTANT, 0),
                codeMakeV(OP_CONSTANT, 1),
                codeMakeV(OP_MUL),
                codeMakeV(OP_POP),
                NULL
            }
        },
        {
            .input = "2 / 1",
            .expConstants = {_INT(2), _INT(1), _END()},
            .expInstructions = {
                codeMakeV(OP_CONSTANT, 0),
                codeMakeV(OP_CONSTANT, 1),
                codeMakeV(OP_DIV),
                codeMakeV(OP_POP),
                NULL
            }
        },
        {
            .input = "true",
            .expConstants = {_END()},
            .expInstructions = {
                codeMakeV(OP_TRUE),
                codeMakeV(OP_POP),
                NULL
            }
        },
        {
            .input = "false",
            .expConstants = {_END()},
            .expInstructions = {
                codeMakeV(OP_FALSE),
                codeMakeV(OP_POP),
                NULL
            }
        },
        {
            .input = "1 > 2",
            .expConstants = {_INT(1), _INT(2), _END()},
            .expInstructions = {
                codeMakeV(OP_CONSTANT, 0),
                codeMakeV(OP_CONSTANT, 1),
                codeMakeV(OP_GREATER_THAN),
                codeMakeV(OP_POP),
                NULL
            }
        },
        {
            .input = "1 < 2",
            .expConstants = {_INT(2), _INT(1), _END()},
            .expInstructions = {
                codeMakeV(OP_CONSTANT, 0),
                codeMakeV(OP_CONSTANT, 1),
                codeMakeV(OP_GREATER_THAN),
                codeMakeV(OP_POP),
                NULL
            }
        },
        {
            .input = "1 == 2",
            .expConstants = {_INT(1), _INT(2), _END()},
            .expInstructions = {
                codeMakeV(OP_CONSTANT, 0),
                codeMakeV(OP_CONSTANT, 1),
                codeMakeV(OP_EQUAL),
                codeMakeV(OP_POP),
                NULL
            }
        },
        {
            .input = "1 != 2",
            .expConstants = {_INT(1), _INT(2), _END()},
            .expInstructions = {
                codeMakeV(OP_CONSTANT, 0),
                codeMakeV(OP_CONSTANT, 1),
                codeMakeV(OP_NOT_EQUAL),
                codeMakeV(OP_POP),
                NULL
            }
        },
        {
            .input = "true == false",
            .expConstants = {_END()},
            .expInstructions = {
                codeMakeV(OP_TRUE),
                codeMakeV(OP_FALSE),
                codeMakeV(OP_EQUAL),
                codeMakeV(OP_POP),
                NULL
            }
        },
        {
            .input = "true != false",
            .expConstants = {_END()},
            .expInstructions = {
                codeMakeV(OP_TRUE),
                codeMakeV(OP_FALSE),
                codeMakeV(OP_NOT_EQUAL),
                codeMakeV(OP_POP),
                NULL
            }
        },
        {
            .input = "-1",
            .expConstants = {_INT(1), _END()},
            .expInstructions = {
                codeMakeV(OP_CONSTANT, 0),
                codeMakeV(OP_MINUS),
                codeMakeV(OP_POP),
                NULL
            }
        },
        {
            .input = "!true",
            .expConstants = { _END()},
            .expInstructions = {
                codeMakeV(OP_TRUE),
                codeMakeV(OP_BANG),
                codeMakeV(OP_POP),
                NULL
            }
        },
    };
    int numTestCases = sizeof(testCases) / sizeof(testCases[0]);

    runCompilerTests(testCases, numTestCases);
}

void runCompilerTests(TestCase_t* tc, int numTc) {
    for (int i = 0; i < numTc; i++) {
        Lexer_t* lexer = createLexer(tc[i].input);
        Parser_t* parser = createParser(lexer);
        Program_t* program = parserParseProgram(parser);

        Compiler_t compiler = createCompiler();
        CompError_t err = compilerCompile(&compiler, program); 
        TEST_INT(COMP_NO_ERROR, err, "Compiler error");

        Bytecode_t bytecode = compilerGetBytecode(&compiler);   

        testInstructions(tc[i].expInstructions, 
                        bytecode.instructions);

        testConstants(tc[i].expConstants, 
                    bytecode.constants);

        // cleanup expects(there are runtime allocated) 
        cleanupInstructions(tc[i].expInstructions);

        cleanupCompiler(&compiler);
        cleanupParser(&parser);
        cleanupProgram(&program);
        gcForceRun(&program);
    }
}

void testInstructions(SliceByte_t expected[], SliceByte_t actual) {
    int numExpected = 0; 
    while (expected[numExpected] != NULL) numExpected++;

    SliceByte_t concatted = concatInstructions(expected, numExpected); 
    TEST_STRING(instructionsToString(concatted), instructionsToString(actual), "Wrong instruction"); 
    cleanupSliceByte(concatted);
}

SliceByte_t concatInstructions(SliceByte_t expected[], int num) {
    SliceByte_t out = createSliceByte(0);
    for (int i = 0; i < num; i++) {
        sliceByteAppend(&out, expected[i], sliceByteGetLen(expected[i]));
    }
    return out; 
}

void testConstants(GenericExpect_t expected[], VectorObjects_t* actual) {
    int numExpected = 0;
    while (expected[numExpected].type != EXPECT_END) numExpected++;
    
    TEST_INT(numExpected, vectorObjectsGetCount(actual), "wrong number of constants");
    Object_t** objects = vectorObjectsGetBuffer(actual); 
    for (int i = 0; i < numExpected; i++) {
        switch(expected[i].type) {
            case EXPECT_INTEGER: 
                testIntegerObject(expected[i].il, objects[i]);
                break;
            default: 
                TEST_ABORT();
        }
    }
} 

void testIntegerObject(int64_t expected, Object_t* obj) {
    TEST_ASSERT_NOT_NULL_MESSAGE(obj, "Object is null");
    TEST_ASSERT_EQUAL_INT_MESSAGE(OBJECT_INTEGER, obj->type, "Object type not OBJECT_INTEGER");
    Integer_t* intObj = (Integer_t*) obj;
    TEST_ASSERT_EQUAL_INT64_MESSAGE(expected, intObj->value, "Object value is not correct");
}


void cleanupInstructions(Instructions_t instr[]) {
    int num = 0; 
    while (instr[num] != NULL) num++;
    for (int i = 0; i < num; i++) {
        cleanupSliceByte(instr[i]);
    }
}

// not needed when using generate_test_runner.rb
int main(void) {
   UNITY_BEGIN();
   RUN_TEST(testCompilerBasic);
   return UNITY_END();
}