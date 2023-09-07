#include "unity.h"
#include "test_helper.h"
#include "utils.h"
#include "lexer.h"
#include "parser.h"
#include "compiler.h"
#include "vm.h"
#include "gc.h"

void setUp(void) {
    // set stuff up here
}

void tearDown(void) {
    // clean stuff up here
}

typedef struct TestCase {
    const char *input;
    GenericExpect_t exp; 
} TestCase_t;


void runVmTest(TestCase_t tc[], int numTestCases); 
void testExpectedObject(GenericExpect_t *expected, Object_t* actual); 
void testIntegerObject(int64_t expected, Object_t* obj); 
void testBooleanObject(bool expected, Object_t* obj);

void runVmTest(TestCase_t tc[], int numTestCases) {

    for(int i = 0; i < numTestCases; i++) {
        Lexer_t* lexer = createLexer(tc[i].input);
        Parser_t* parser = createParser(lexer);
        Program_t* program = parserParseProgram(parser);

        Compiler_t compiler = createCompiler();
        CompError_t compErr = compilerCompile(&compiler, program); 
        TEST_INT(COMP_NO_ERROR, compErr, "Compiler error");

        Bytecode_t bytecode = compilerGetBytecode(&compiler);   
        Vm_t vm = createVm(&bytecode);
        VmError_t vmErr = vmRun(&vm); 
        TEST_INT(VM_NO_ERROR, vmErr, "Vm error"); 

        Object_t* stackElem = vmLastPoppedStackElem(&vm);

        testExpectedObject(&tc[i].exp, stackElem);

        cleanupVm(&vm);
        cleanupCompiler(&compiler);
        cleanupParser(&parser);
        cleanupProgram(&program);
        gcForceRun();
    }

}

void testExpectedObject(GenericExpect_t *expected, Object_t* actual) {
    switch(expected->type) {
        case EXPECT_INTEGER: 
            testIntegerObject(expected->il, actual);
            break;
        case EXPECT_BOOL: 
            testBooleanObject(expected->bl, actual);
            break;
        default: 
            TEST_ABORT();
    }
}

void testIntegerObject(int64_t expected, Object_t* obj) {
    TEST_ASSERT_NOT_NULL_MESSAGE(obj, "Object is null");
    TEST_ASSERT_EQUAL_INT_MESSAGE(OBJECT_INTEGER, obj->type, "Object type not OBJECT_INTEGER");
    Integer_t* intObj = (Integer_t*) obj;
    TEST_ASSERT_EQUAL_INT64_MESSAGE(expected, intObj->value, "Object value is not correct");
}

void testBooleanObject(bool expected, Object_t* obj) {
    TEST_ASSERT_NOT_NULL_MESSAGE(obj, "Object is null");
    TEST_ASSERT_EQUAL_INT_MESSAGE(OBJECT_BOOLEAN, obj->type, "Object type not OBJECT_BOOLEAN");
    Boolean_t* boolObj = (Boolean_t*) obj;
    TEST_ASSERT_EQUAL_INT_MESSAGE(expected, boolObj->value, "Object value is not correct");
}


void testIntegerArithmetic() {
    TestCase_t vmTestCases[] = {
        {"1", _INT(1)}, 
        {"2", _INT(2)},
        {"1+2", _INT(3)}, // FIXME
        {"1 - 2", _INT(-1)},
        {"1 * 2", _INT(2)},
        {"4 / 2", _INT(2)},
        {"50 / 2 * 2 + 10 - 5", _INT(55)},
        {"5 + 5 + 5 + 5 - 10", _INT(10)},
        {"2 * 2 * 2 * 2 * 2", _INT(32)},
        {"5 * 2 + 10", _INT(20)},
        {"5 + 2 * 10", _INT(25)},
        {"5 * (2 + 10)", _INT(60)},
    };

    int numTestCases = sizeof(vmTestCases) / sizeof(vmTestCases[0]);
    runVmTest(vmTestCases, numTestCases);
}

void testBooleanExpressions() {
    TestCase_t vmTestCases[] = {
        {"true", _BOOL(true)}, 
        {"false", _BOOL(false)},
    };

    int numTestCases = sizeof(vmTestCases) / sizeof(vmTestCases[0]);
    runVmTest(vmTestCases, numTestCases);
}

// not needed when using generate_test_runner.rb
int main(void) {
   UNITY_BEGIN();
   RUN_TEST(testIntegerArithmetic);
   RUN_TEST(testBooleanExpressions);
   return UNITY_END();
}