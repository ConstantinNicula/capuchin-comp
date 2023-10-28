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
void testErrorObject(const char* expected, Object_t *obj); 

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
        TEST_INT(VM_NO_ERROR, vmErr.code, vmErr.str); 

        Object_t* stackElem = vmLastPoppedStackElem(&vm);

        testExpectedObject(&tc[i].exp, stackElem);

        cleanupVmError(&vmErr);
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
        case EXPECT_ERROR:
            testErrorObject(expected->el, actual); 
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
        Object_t* expKeyObject = (Object_t*)createInteger(hl.keys[i].il);

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
    TEST_ASSERT_EQUAL_INT_MESSAGE(OBJECT_NULL, obj->type, "Object type not OBJECT_NULL");
}

void testErrorObject(const char* expected, Object_t *obj) {
    TEST_NOT_NULL(obj, "Object is null");
    TEST_INT(OBJECT_ERROR, obj->type, "Object type not OBJECT_ERROR");
    Error_t *errObj = (Error_t *)obj;
    TEST_STRING(expected, errObj->message, "Error message is not correct");
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

void testIndexExpression() {
    TestCase_t vmTestCases[] = {
        {"[1, 2, 3][1]", _INT(2)},
        {"[1, 2, 3][0 + 2]", _INT(3)},
        {"[[1, 1, 1]][0][0]", _INT(1)},
        {"[][0]", _NIL},
        {"[1, 2, 3][99]", _NIL},
        {"[1][-1]", _NIL},
        {"{1: 1, 2: 2}[1]", _INT(1)},
        {"{1: 1, 2: 2}[2]", _INT(2)},
        {"{1: 1}[0]", _NIL},
        {"{}[0]", _NIL},
    };

    int numTestCases = sizeof(vmTestCases) / sizeof(vmTestCases[0]);
    runVmTest(vmTestCases, numTestCases);
}

void testCallingFunctionsWithArguments() {
    TestCase_t vmTestCases[] = {
        {
            "let fivePlusTen = fn() {5 + 10; };" 
            "fivePlusTen();", 
            _INT(15)
        },
        {
            "let one = fn() { 1; };" 
            "let two = fn() { 2; };" 
            "one() + two()", 
            _INT(3)
        },
        {
            "let a = fn() { 1 };" 
            "let b = fn() { a() + 1 };" 
            "let c = fn() { b() + 1 };" 
            "c();",
            _INT(3)
        },
    };

    int numTestCases = sizeof(vmTestCases) / sizeof(vmTestCases[0]);
    runVmTest(vmTestCases, numTestCases);
}

void testFunctionsWithReturnStatement() {
    TestCase_t vmTestCases[] = {
        {
            "let earlyExit = fn() {return 99; 100;};" 
            "earlyExit();", 
            _INT(99)
        },
        {
            "let earlyExit = fn() { return 99; return 100;};" 
            "earlyExit();", 
            _INT(99)
        },
        
    };

    int numTestCases = sizeof(vmTestCases) / sizeof(vmTestCases[0]);
    runVmTest(vmTestCases, numTestCases);
}

void testFunctionsWithoutReturnValue() {
    TestCase_t vmTestCases[] = {
        {
            "let noReturn = fn() {};" 
            "noReturn();", 
            _NIL 
        },
        {
            "let noReturn = fn() {}" 
            "let noReturnTwo = fn() {noReturn();}" 
            "noReturn();"
            "noReturnTwo();",
            _NIL 
        },
        
    };

    int numTestCases = sizeof(vmTestCases) / sizeof(vmTestCases[0]);
    runVmTest(vmTestCases, numTestCases);
}


void testCallingFunctionsWithBindings() {
    TestCase_t vmTestCases[] = {
        {
            "let one = fn() { let one = 1; one };"
            "one();",
            _INT(1),
        },
        {
            "let oneAndTwo = fn() { let one = 1; let two = 2; one + two; };"
            "oneAndTwo();",
            _INT(3),
        },
        {
            "let oneAndTwo = fn() { let one = 1; let two = 2; one + two; };"
            "let threeAndFour = fn() { let three = 3; let four = 4; three + four; };"
            "oneAndTwo() + threeAndFour();",
            _INT(10),
        },
        {
            "let firstFoobar = fn() { let foobar = 50; foobar; };"
            "let secondFoobar = fn() { let foobar = 100; foobar; };"
            "firstFoobar() + secondFoobar();",
            _INT(150),
        },
        {
            "let globalSeed = 50;"
            "let minusOne = fn() {"
            "   let num = 1;"
            "   globalSeed - num;"
            "}"
            "let minusTwo = fn() {"
            "   let num = 2;"
            "   globalSeed - num;"
            "}"
            "minusOne() + minusTwo();",
            _INT(97),
        },
    };

    int numTestCases = sizeof(vmTestCases) / sizeof(vmTestCases[0]);
    runVmTest(vmTestCases, numTestCases);
}

void testFirstClassFunctions() {
    TestCase_t vmTestCases[] = {
        {
            "let returnsOneReturner = fn() {"
            "    let returnsOne = fn() { 1; };"
            "    returnsOne;"
            "};"
            "returnsOneReturner()();",
            _INT(1),
        }
    };

    int numTestCases = sizeof(vmTestCases) / sizeof(vmTestCases[0]);
    runVmTest(vmTestCases, numTestCases);
}

void testCallingFunctionsWithArgumentsAndLocalBindings() {
    TestCase_t vmTestCases[] = {
        {
            "let identity = fn(a) {a};"
            "identity(4);",
            _INT(4),
        },
        { 
            "let sum = fn(a, b) {a + b;};"
            "sum(1, 2);",
            _INT(3),
        },
        { 
            "let sum = fn(a, b) {"
            "   let c = a + b;"
            "   c;"
            "};"
            "sum(1, 2);",
            _INT(3),
        },
        { 
            "let sum = fn(a, b) {"
            "   let c = a + b;"
            "   c;"
            "};"
            "sum(1, 2) + sum(3, 4);",
            _INT(10),
        },
        { 
            "let sum = fn(a, b) {"
            "   let c = a + b;"
            "   c;"
            "};"
            "let outer = fn() {"
            "    sum(1, 2) + sum(3, 4);"
            "};"
            "outer();",
            _INT(10),
        }

    };

    int numTestCases = sizeof(vmTestCases) / sizeof(vmTestCases[0]);
    runVmTest(vmTestCases, numTestCases);
}

void testCallingFunctionsWithWrongArguments() {
    typedef struct TestCase {
        const char* input;
        const char* expected;
    } TestCase_t;

    TestCase_t testCases[] = {
        {
            .input = "fn() {1;}(1);",
            .expected = "wrong number of arguments: want=0, got=1"
        },
        {
            .input = "fn(a) {a;}();",
            .expected = "wrong number of arguments: want=1, got=0"
        },
        {
            .input = "fn(a, b) {a + b;}(1);",
            .expected = "wrong number of arguments: want=2, got=1"
        }
    };

    int numTestCases = sizeof(testCases) / sizeof(testCases[0]);
    for (int i = 0; i < numTestCases; i++) {
        Lexer_t* lexer = createLexer(testCases[i].input);
        Parser_t* parser = createParser(lexer);
        Program_t* program = parserParseProgram(parser);

        Compiler_t compiler = createCompiler();
        CompError_t compErr = compilerCompile(&compiler, program); 
        TEST_INT(COMP_NO_ERROR, compErr, "Compiler error");

        Bytecode_t bytecode = compilerGetBytecode(&compiler);   
        Vm_t vm = createVm(&bytecode);
        VmError_t vmErr = vmRun(&vm); 
        TEST_STRING(testCases[i].expected, vmErr.str, "wrong VM error");

        cleanupVmError(&vmErr);
        cleanupVm(&vm);
        cleanupCompiler(&compiler);
        cleanupParser(&parser);
        cleanupProgram(&program);
        gcForceRun();
    }
}


void testBuiltinFunctions() {
    TestCase_t vmTestCases[] = {
        {"len(\"\")", _INT(0)},
        {"len(\"four\")", _INT(4)},
        {"len(\"hello world\")", _INT(11)},
        {"len(1)", _ERROR("argument to `len` not supported, got INTEGER")},
        {"len(\"one\", \"two\")", _ERROR("wrong number of arguments. got=2, want=1")},
        {"len([1, 2, 3])", _INT(3)},
        {"len([])", _INT(0)},
        {"puts(\"hello\", \"world!\")", _NIL},
        {"first([1, 2, 3])", _INT(1)},
        {"first([])", _NIL},
        {"first(1)", _ERROR("argument to `first` must be ARRAY, got INTEGER")},
        {"last([1, 2, 3])", _INT(3)},
        {"last([])", _NIL},
        {"last(1)", _ERROR("argument to `last` must be ARRAY, got INTEGER")},
        {"rest([1, 2, 3])", _ARRAY(_INT(2), _INT(3), _END)},
        {"rest([])", _NIL},
        {"push([], 1)", _ARRAY(_INT(1), _END)},
        {"push(1, 1)", _ERROR("argument to `push` must be ARRAY, got INTEGER")},
    };

    int numTestCases = sizeof(vmTestCases) / sizeof(vmTestCases[0]);
    runVmTest(vmTestCases, numTestCases);
}

void testClosures() {
    TestCase_t vmTestCases[] = {
        {
            "let newClosure = fn(a) {"
            "   fn() {a; };"
            "};"
            "let closure = newClosure(99);"
            "closure()",
            _INT(99),
        },
        {
            "let newAdder = fn(a, b) {"
            "   fn(c) {a + b + c};"
            "};"
            "let adder = newAdder(1, 2);"
            "adder(8)",
            _INT(11),
        },
        {
            "let newAdder = fn(a, b) {"
            "   let c = a + b"
            "   fn(d) {c + d};"
            "};"
            "let adder = newAdder(1, 2);"
            "adder(8)",
            _INT(11),
        },
        {
            "let newAdderOuter = fn(a, b) {"
            "   let c = a + b"
            "   fn(d) {"
            "       let e = d + c"
            "          fn(f) {e + f;}"
            "   };"
            "};"
            "let newAdderInner = newAdderOuter(1,2)"
            "let adder = newAdderInner(3)"
            "adder(8)",
            _INT(14),
        },
        {
            "let a = 1;"
            "let newAdderOuter = fn(b) {"
            "   fn(c) {"
            "       fn(d) { a + b + c + d};"
            "   };"
            "};"
            "let newAdderInner = newAdderOuter(2)"
            "let adder = newAdderInner(3)"
            "adder(8)",
            _INT(14),
        },
        {
            "let newClosure = fn(a, b) {"
            "   let one = fn() { a; }"
            "   let two = fn() { b; }"
            "   fn() { one() + two(); }"
            "};"
            "let closure = newClosure(9, 90)"
            "closure()",
            _INT(99),
        },
   };

    int numTestCases = sizeof(vmTestCases) / sizeof(vmTestCases[0]);
    runVmTest(vmTestCases, numTestCases);
}

void testRecursiveFunctions() {
    TestCase_t vmTestCases[] = {
        {
            "let countDown = fn(x) {"
            "   if (x == 0) {"
            "       return 0;"
            "   } else {"
            "       return countDown(x - 1);"
            "   }"
            "};"
            "countDown(1)",
            _INT(0),
        },
        {
            "let countDown = fn(x) {"
            "   if (x == 0) {"
            "       return 0;"
            "   } else {"
            "       return countDown(x - 1);"
            "   }"
            "};"
            "let wrapper = fn() {"
            "   countDown(1);"
            "};"
            "wrapper()",
            _INT(0),
        },
        {
            "let wrapper = fn() {"
            "   let countDown = fn(x) {"
            "       if (x == 0) {"
            "           return 0;"
            "       } else {"
            "           return countDown(x - 1)"
            "       }"
            "  }"
            "  countDown(1);"
            "};"
            "wrapper();",
            _INT(0)
        } 
   };

    int numTestCases = sizeof(vmTestCases) / sizeof(vmTestCases[0]);
    runVmTest(vmTestCases, numTestCases);
}
int main(void) {
    UNITY_BEGIN();
    RUN_TEST(testIntegerArithmetic);
    RUN_TEST(testBooleanExpressions);
    RUN_TEST(testConditionals);
    RUN_TEST(testGlobalLetStatements);
    RUN_TEST(testArrayLiterals);
    RUN_TEST(testHashLiterals);
    RUN_TEST(testIndexExpression);
    RUN_TEST(testCallingFunctionsWithArguments);
    RUN_TEST(testFunctionsWithReturnStatement);
    RUN_TEST(testFunctionsWithoutReturnValue);
    RUN_TEST(testCallingFunctionsWithBindings);
    RUN_TEST(testCallingFunctionsWithArgumentsAndLocalBindings);
    RUN_TEST(testCallingFunctionsWithWrongArguments);
    RUN_TEST(testFirstClassFunctions);
    RUN_TEST(testBuiltinFunctions);
    RUN_TEST(testClosures);
    RUN_TEST(testRecursiveFunctions);
    return UNITY_END();
}