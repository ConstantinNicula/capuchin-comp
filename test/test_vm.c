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
void testStringObject(const char* expected, Object_t *obj); 
void testNullObject(Object_t* obj);
void testArrayObject(GenericExpect_t *al, Object_t* obj); 
void testHashObject(GenericHash_t hl, Object_t* obj); 

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
        case EXPECT_STRING: 
            testStringObject(expected->sl, actual);
            break;
        case EXPECT_NULL:
            testNullObject(actual);
            break;
        case EXPECT_ARRAY:
            testArrayObject(expected->al, actual);
            break;
        case EXPECT_HASH: 
            testHashObject(expected->hl, actual);
            break; 
        default: 
            TEST_ABORT();
    }
}

void testArrayObject(GenericExpect_t *al, Object_t* obj) {
    TEST_ASSERT_NOT_NULL_MESSAGE(obj, "Object is null");
    TEST_ASSERT_EQUAL_INT_MESSAGE(OBJECT_ARRAY, obj->type, "Object type not OBJECT_ARRAY");

    Array_t* arrObj = (Array_t*) obj;
    Object_t** elems = arrayGetElements(arrObj);   
    uint32_t elemCnt = arrayGetElementCount(arrObj); 
    uint32_t cnt = 0;

    while (al && al->type != EXPECT_END && cnt < elemCnt) {
        testExpectedObject(al, elems[cnt]);
        al++;
        cnt++;        
    }
    TEST_INT(cnt, elemCnt, "Wrong number of elements");
}

void testHashObject(GenericHash_t hl, Object_t* obj) {
    TEST_ASSERT_NOT_NULL_MESSAGE(obj, "Object is null");
    TEST_ASSERT_EQUAL_INT_MESSAGE(OBJECT_HASH, obj->type, "Object type not OBJECT_HASH");
    Hash_t* hashObj = (Hash_t*) obj;

    // get nr of elements 
    int cnt = 0; 
    while (hl.keys[cnt].type != EXPECT_END) {cnt++;}

    int i = 0; 
    while (i < cnt) {
        // TO DO: fix for generic object type. 
        Object_t* expKeyObject = createInteger(hl.keys[i].il);

        HashPair_t* pair = hashGetPair(hashObj, expKeyObject);
        TEST_NOT_NULL(pair, "no pair found!");

        testExpectedObject(&hl.values[i], pair->value);
        testExpectedObject(&hl.keys[i], pair->key);
        
        i++;        
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

void testStringObject(const char* expected, Object_t *obj) {
    TEST_NOT_NULL(obj, "Object is null");
    TEST_INT(OBJECT_STRING, obj->type, "Object type not OBJECT_STRING");
    String_t *strObj = (String_t *)obj;
    TEST_STRING(expected, strObj->value, "Object value is not correct");
}

void testNullObject(Object_t* obj) {
    TEST_ASSERT_EQUAL_INT_MESSAGE(OBJECT_NULL, obj->type, "Object type not OBJECT_BOOLEAN");
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
        {"-5", _INT(-5)},
        {"-10", _INT(-10)},
        {"-50 + 100 + -50", _INT(0)},
        {"(5 + 10 * 2 + 15 / 3) * 2 + -10", _INT(50)},
    };

    int numTestCases = sizeof(vmTestCases) / sizeof(vmTestCases[0]);
    runVmTest(vmTestCases, numTestCases);
}

void testBooleanExpressions() {
    TestCase_t vmTestCases[] = {
        {"true", _BOOL(true)}, 
        {"false", _BOOL(false)},
        {"1 < 2", _BOOL(true)},
        {"1 > 2", _BOOL(false)},
        {"1 < 1", _BOOL(false)},
        {"1 > 1", _BOOL(false)},
        {"1 == 1", _BOOL(true)},
        {"1 != 1", _BOOL(false)},
        {"1 == 2", _BOOL(false)},
        {"1 != 2", _BOOL(true)},
        {"true == true", _BOOL(true)},
        {"false == false", _BOOL(true)},
        {"true == false", _BOOL(false)},
        {"true != false", _BOOL(true)},
        {"false != true", _BOOL(true)},
        {"(1 < 2) == true", _BOOL(true)},
        {"(1 < 2) == false", _BOOL(false)},
        {"(1 > 2) == true", _BOOL(false)},
        {"(1 > 2) == false", _BOOL(true)},
        {"!true", _BOOL(false)},
        {"!false", _BOOL(true)},
        {"!5", _BOOL(false)},
        {"!!true", _BOOL(true)},
        {"!!false", _BOOL(false)},
        {"!!5", _BOOL(true)},
        {"!(if (false) { 5; })", _BOOL(true)},
    };

    int numTestCases = sizeof(vmTestCases) / sizeof(vmTestCases[0]);
    runVmTest(vmTestCases, numTestCases);
}

void testConditionals() {
    TestCase_t vmTestCases[] = {
        {"if (true) { 10 }", _INT(10)},
        {"if (true) { 10 } else { 20 }", _INT(10)},
        {"if (false) { 10 } else { 20 } ", _INT(20)},
        {"if (1) { 10 }", _INT(10)},
        {"if (1 < 2) { 10 }", _INT(10)},
        {"if (1 < 2) { 10 } else { 20 }", _INT(10)},
        {"if (1 > 2) { 10 } else { 20 }", _INT(20)},
        {"if (1 > 2) { 10 }", _NIL},
        {"if (false) { 10 }", _NIL},
        {"if ((if (false) { 10 })) { 10 } else { 20 }", _INT(20)},
    };

    int numTestCases = sizeof(vmTestCases) / sizeof(vmTestCases[0]);
    runVmTest(vmTestCases, numTestCases);
}

void testGlobalLetStatements() {
    TestCase_t vmTestCases[] = {
        {"let one = 1; one", _INT(1)},
        {"let one = 1; let two = 2; one + two", _INT(3)},
        {"let one = 1; let two = one + one; one + two", _INT(3)},
    };

    int numTestCases = sizeof(vmTestCases) / sizeof(vmTestCases[0]);
    runVmTest(vmTestCases, numTestCases);
}

void testStringExpressions() {
    TestCase_t vmTestCases[] = {
        {"\"monkey\"", _STRING("monkey")},
        {"\"mon\" + \"key\"", _STRING("monkey")},
        {"\"mon\" + \"key\" + \"banana\"", _STRING("monkeybanana")},
    };

    int numTestCases = sizeof(vmTestCases) / sizeof(vmTestCases[0]);
    runVmTest(vmTestCases, numTestCases);
}

void testArrayLiterals() {
    TestCase_t vmTestCases[] = {
        {"[]", _ARRAY(_END)},
        {"[1, 2, 3]", _ARRAY( _INT(1), _INT(2), _INT(3), _END)},
        {"[1 + 2, 3 * 4, 5 + 6]", _ARRAY( _INT(3), _INT(12), _INT(11), _END)},
    };

    int numTestCases = sizeof(vmTestCases) / sizeof(vmTestCases[0]);
    runVmTest(vmTestCases, numTestCases);
}

void testHashLiterals() {
    TestCase_t vmTestCases[] = {
        {"{}", _HASH({_END}, {_END})},
        {"{1: 2, 2: 3}", _HASH(
            _LIT({_INT(1), _INT(2), _END}), 
            _LIT({_INT(2), _INT(3), _END})
        )},
        {"{1 + 1: 2 * 2, 3 + 3: 4 * 4}", _HASH(
            _LIT({_INT(2), _INT(6), _END}), 
            _LIT({_INT(4), _INT(16), _END})
        )},
    };

    int numTestCases = sizeof(vmTestCases) / sizeof(vmTestCases[0]);
    runVmTest(vmTestCases, numTestCases);
}

// not needed when using generate_test_runner.rb
int main(void) {
    UNITY_BEGIN();
    RUN_TEST(testIntegerArithmetic);
    RUN_TEST(testBooleanExpressions);
    RUN_TEST(testConditionals);
    RUN_TEST(testGlobalLetStatements);
    RUN_TEST(testArrayLiterals);
    RUN_TEST(testHashLiterals);
    return UNITY_END();
}