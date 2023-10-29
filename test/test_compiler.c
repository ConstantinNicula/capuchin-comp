#include "unity.h"
#include "utils.h"
#include "compiler.h"
#include "lexer.h"
#include "parser.h"
#include "test_helper.h"
#include "gc.h"

void setUp(void)
{
    // set stuff up here
}

void tearDown(void)
{
    // clean stuff up here
}

#define MAX_CONSTANTS 128
#define MAX_INSTRUCTIONS 128

typedef struct TestCase
{
    const char *input;
    GenericExpect_t expConstants[MAX_CONSTANTS];
    SliceByte_t expInstructions[MAX_INSTRUCTIONS];
} TestCase_t;

void runCompilerTests(TestCase_t *testCases, int numTestCases);
void testInstructions(SliceByte_t expected[], SliceByte_t actual);
SliceByte_t concatInstructions(SliceByte_t expected[], int num);

void testConstants(GenericExpect_t expected[], VectorObjects_t *actual);
void testIntegerObject(int64_t expected, Object_t *obj);
void testStringObject(const char* str, Object_t *obj); 
void testCompiledFunction(SliceByte_t intr[], Object_t*obj);
void cleanupInstructions(Instructions_t instr[]);



void testCompilerBasic() {
    TestCase_t testCases[] = {
        {
            .input = "1 + 2",
            .expConstants = {_INT(1), _INT(2), _END},
            .expInstructions = { codeMakeV(OP_CONSTANT, 0), codeMakeV(OP_CONSTANT, 1), codeMakeV(OP_ADD), codeMakeV(OP_POP), NULL }
        },
        {
            .input = "1; 2",
            .expConstants = {_INT(1), _INT(2), _END},
            .expInstructions = {codeMakeV(OP_CONSTANT, 0), codeMakeV(OP_POP), codeMakeV(OP_CONSTANT, 1), codeMakeV(OP_POP), NULL}
        },
        {
            .input = "1 - 2",
            .expConstants = {_INT(1), _INT(2), _END},
            .expInstructions = {codeMakeV(OP_CONSTANT, 0), codeMakeV(OP_CONSTANT, 1), codeMakeV(OP_SUB), codeMakeV(OP_POP), NULL}
        },
        {
            .input = "1 * 2",
            .expConstants = {_INT(1), _INT(2), _END},
            .expInstructions = {codeMakeV(OP_CONSTANT, 0), codeMakeV(OP_CONSTANT, 1), codeMakeV(OP_MUL), codeMakeV(OP_POP), NULL}
        },
        {
            .input = "2 / 1",
            .expConstants = {_INT(2), _INT(1), _END},
            .expInstructions = {codeMakeV(OP_CONSTANT, 0), codeMakeV(OP_CONSTANT, 1), codeMakeV(OP_DIV), codeMakeV(OP_POP), NULL}
        },
        {
            .input = "true",
            .expConstants = {_END},
            .expInstructions = {codeMakeV(OP_TRUE), codeMakeV(OP_POP), NULL}
        },
        {
            .input = "false",
            .expConstants = {_END},
            .expInstructions = {codeMakeV(OP_FALSE), codeMakeV(OP_POP), NULL}
        },
        {
            .input = "1 > 2",
            .expConstants = {_INT(1), _INT(2), _END},
            .expInstructions = {codeMakeV(OP_CONSTANT, 0), codeMakeV(OP_CONSTANT, 1), codeMakeV(OP_GREATER_THAN), codeMakeV(OP_POP), NULL}
        },
        {
            .input = "1 < 2",
            .expConstants = {_INT(2), _INT(1), _END},
            .expInstructions = {codeMakeV(OP_CONSTANT, 0), codeMakeV(OP_CONSTANT, 1), codeMakeV(OP_GREATER_THAN), codeMakeV(OP_POP), NULL}
        },
        {
            .input = "1 == 2",
            .expConstants = {_INT(1), _INT(2), _END},
            .expInstructions = {codeMakeV(OP_CONSTANT, 0), codeMakeV(OP_CONSTANT, 1), codeMakeV(OP_EQUAL), codeMakeV(OP_POP), NULL}
        },
        {
            .input = "1 != 2",
            .expConstants = {_INT(1), _INT(2), _END},
            .expInstructions = {codeMakeV(OP_CONSTANT, 0), codeMakeV(OP_CONSTANT, 1), codeMakeV(OP_NOT_EQUAL), codeMakeV(OP_POP), NULL}
        },
        {
            .input = "true == false",
            .expConstants = {_END},
            .expInstructions = {codeMakeV(OP_TRUE), codeMakeV(OP_FALSE), codeMakeV(OP_EQUAL), codeMakeV(OP_POP), NULL}
        },
        {
            .input = "true != false",
            .expConstants = {_END},
            .expInstructions = {codeMakeV(OP_TRUE), codeMakeV(OP_FALSE), codeMakeV(OP_NOT_EQUAL), codeMakeV(OP_POP), NULL}
        },
        {
            .input = "-1",
            .expConstants = {_INT(1), _END},
            .expInstructions = {codeMakeV(OP_CONSTANT, 0), codeMakeV(OP_MINUS), codeMakeV(OP_POP), NULL}
        },
        {
            .input = "!true",
            .expConstants = {_END},
            .expInstructions = {codeMakeV(OP_TRUE), codeMakeV(OP_BANG), codeMakeV(OP_POP), NULL}
        },
    };
    int numTestCases = sizeof(testCases) / sizeof(testCases[0]);

    runCompilerTests(testCases, numTestCases);
}

void testConditionals() {

    TestCase_t testCases[] = {
        {
            .input = "if (true) {10}; 3333;",
            .expConstants = {_INT(10), _INT(3333), _END},
            .expInstructions = {
                codeMakeV(OP_TRUE),
                codeMakeV(OP_JUMP_NOT_TRUTHY, 10),
                codeMakeV(OP_CONSTANT, 0),
                codeMakeV(OP_JUMP, 11),
                codeMakeV(OP_NULL),
                codeMakeV(OP_POP),
                codeMakeV(OP_CONSTANT, 1),
                codeMakeV(OP_POP),
                NULL,
            }
        },
        {
            .input = "if (true) {10} else {20}; 3333;", 
            .expConstants = {_INT(10), _INT(20), _INT(3333), _END}, 
            .expInstructions = {
                codeMakeV(OP_TRUE),
                codeMakeV(OP_JUMP_NOT_TRUTHY, 10),
                codeMakeV(OP_CONSTANT, 0),
                codeMakeV(OP_JUMP, 13),
                codeMakeV(OP_CONSTANT, 1),
                codeMakeV(OP_POP),
                codeMakeV(OP_CONSTANT, 2),
                codeMakeV(OP_POP),
                NULL,
            }},

    };
    int numTestCases = sizeof(testCases) / sizeof(testCases[0]);
    runCompilerTests(testCases, numTestCases);
}

void testGlobalLetStatements() {

    TestCase_t testCases[] = {
        {.input = "let one = 1;"
                  "let two = 2;",
         .expConstants = {_INT(1), _INT(2), _END},
         .expInstructions = {
             codeMakeV(OP_CONSTANT, 0),
             codeMakeV(OP_SET_GLOBAL, 0),
             codeMakeV(OP_CONSTANT, 1),
             codeMakeV(OP_SET_GLOBAL, 1),
             NULL,
         }},
        {.input = "let one = 1;"
                  "one;",
         .expConstants = {_INT(1), _END},
         .expInstructions = {
             codeMakeV(OP_CONSTANT, 0),
             codeMakeV(OP_SET_GLOBAL, 0),
             codeMakeV(OP_GET_GLOBAL, 0),
             codeMakeV(OP_POP),
             NULL,
         }},
        {.input = "let one = 1;"
                  "let two = one;"
                  "two;",
         .expConstants = {_INT(1), _END},
         .expInstructions = {
             codeMakeV(OP_CONSTANT, 0),
             codeMakeV(OP_SET_GLOBAL, 0),
             codeMakeV(OP_GET_GLOBAL, 0),
             codeMakeV(OP_SET_GLOBAL, 1),
             codeMakeV(OP_GET_GLOBAL, 1),
             codeMakeV(OP_POP),
             NULL,
         }}};

    int numTestCases = sizeof(testCases) / sizeof(testCases[0]);
    runCompilerTests(testCases, numTestCases);
}

void testStringExpressions() {

    TestCase_t testCases[] = {
        {
            .input = "\"monkey\"",
            .expConstants = {_STRING("monkey"), _END},
            .expInstructions = {
                codeMakeV(OP_CONSTANT, 0),
                codeMakeV(OP_POP),
                NULL}
        },
        {
            .input = "\"mon\" + \"key\"", 
            .expConstants = {
                _STRING("mon"), 
                _STRING("key"), 
                _END
            }, 
            .expInstructions = {
                codeMakeV(OP_CONSTANT, 0), 
                codeMakeV(OP_CONSTANT, 1), 
                codeMakeV(OP_ADD), 
                codeMakeV(OP_POP), 
            NULL
            }
        },
    };

    int numTestCases = sizeof(testCases) / sizeof(testCases[0]);
    runCompilerTests(testCases, numTestCases);
}

void testArrayLiterals() {

    TestCase_t testCases[] = {
        {
            .input = "[]",
            .expConstants = {_END},
            .expInstructions = {
                codeMakeV(OP_ARRAY, 0),
                codeMakeV(OP_POP),
                NULL
            }
        },
        {
            .input = "[1, 2, 3]",
            .expConstants = {_INT(1), _INT(2),_INT(3),_END},
            .expInstructions = {
                codeMakeV(OP_CONSTANT, 0),
                codeMakeV(OP_CONSTANT, 1),
                codeMakeV(OP_CONSTANT, 2),
                codeMakeV(OP_ARRAY, 3),
                codeMakeV(OP_POP),
                NULL
            }
        },
        {
            .input = "[1 + 2, 3 - 4, 5 * 6]",
            .expConstants = {_INT(1), _INT(2),_INT(3), _INT(4), _INT(5), _INT(6), _END},
            .expInstructions = {
                codeMakeV(OP_CONSTANT, 0),
                codeMakeV(OP_CONSTANT, 1),
                codeMakeV(OP_ADD),

                codeMakeV(OP_CONSTANT, 2),
                codeMakeV(OP_CONSTANT, 3),
                codeMakeV(OP_SUB),

                codeMakeV(OP_CONSTANT, 4),
                codeMakeV(OP_CONSTANT, 5),
                codeMakeV(OP_MUL),

                codeMakeV(OP_ARRAY, 3),
                codeMakeV(OP_POP),
                NULL
            }
        }
    };

    int numTestCases = sizeof(testCases) / sizeof(testCases[0]);
    runCompilerTests(testCases, numTestCases);
}

void testHashLiterals() {

    TestCase_t testCases[] = {
        {
            .input = "{}",
            .expConstants = {_END},
            .expInstructions = {
                codeMakeV(OP_HASH, 0),
                codeMakeV(OP_POP),
                NULL
            }
        },
        {
            .input = "{1: 2, 3: 4, 5: 6}",
            .expConstants = {_INT(1), _INT(2),_INT(3), _INT(4), _INT(5), _INT(6), _END},
            .expInstructions = {
                codeMakeV(OP_CONSTANT, 0),
                codeMakeV(OP_CONSTANT, 1),
                codeMakeV(OP_CONSTANT, 2),
                codeMakeV(OP_CONSTANT, 3),
                codeMakeV(OP_CONSTANT, 4),
                codeMakeV(OP_CONSTANT, 5),
                codeMakeV(OP_HASH, 6),
                codeMakeV(OP_POP),
                NULL
            }
        },
        {
            .input = "{1: 2 + 3, 4: 5 * 6}",
            .expConstants = {_INT(1), _INT(2),_INT(3), _INT(4), _INT(5), _INT(6), _END},
            .expInstructions = {
                codeMakeV(OP_CONSTANT, 0),
                codeMakeV(OP_CONSTANT, 1),
                codeMakeV(OP_CONSTANT, 2),
                codeMakeV(OP_ADD),

                codeMakeV(OP_CONSTANT, 3),
                codeMakeV(OP_CONSTANT, 4),
                codeMakeV(OP_CONSTANT, 5),
                codeMakeV(OP_MUL),

                codeMakeV(OP_HASH,  4),  
                codeMakeV(OP_POP),
                NULL
            }
        }
    };

    int numTestCases = sizeof(testCases) / sizeof(testCases[0]);
    runCompilerTests(testCases, numTestCases);
}


void testIndexExpressions() {

    TestCase_t testCases[] = {
        {
            .input = "[1, 2, 3][1 + 1]",
            .expConstants = {_INT(1), _INT(2),_INT(3),_INT(1),_INT(1), _END},
            .expInstructions = {
                codeMakeV(OP_CONSTANT, 0), 
                codeMakeV(OP_CONSTANT, 1), 
                codeMakeV(OP_CONSTANT, 2), 
                codeMakeV(OP_ARRAY, 3), 
                codeMakeV(OP_CONSTANT, 3), 
                codeMakeV(OP_CONSTANT, 4), 
                codeMakeV(OP_ADD, 0),
                codeMakeV(OP_INDEX),
                codeMakeV(OP_POP),
                NULL
            }
        },
        {
            .input = "{1: 2}[2 - 1]",
            .expConstants = {_INT(1), _INT(2), _INT(2), _INT(1), _END},
            .expInstructions = {
                codeMakeV(OP_CONSTANT, 0),
                codeMakeV(OP_CONSTANT, 1),
                codeMakeV(OP_HASH, 2),
                codeMakeV(OP_CONSTANT, 2),
                codeMakeV(OP_CONSTANT, 3),
                codeMakeV(OP_SUB),
                codeMakeV(OP_INDEX),
                codeMakeV(OP_POP),
                NULL
            }
        }, 
    };


    int numTestCases = sizeof(testCases) / sizeof(testCases[0]);
    runCompilerTests(testCases, numTestCases);
}


void testFunctions() {

    TestCase_t testCases[] = {
        {
            .input = "fn() {return 5 + 10}",
            .expConstants = {
                _INT(5), 
                _INT(10),
                _FUNC(
                    codeMakeV(OP_CONSTANT, 0),
                    codeMakeV(OP_CONSTANT, 1),
                    codeMakeV(OP_ADD),
                    codeMakeV(OP_RETURN_VALUE),
                    NULL
                ),
                _END
            },
            .expInstructions = {
                codeMakeV(OP_CLOSURE, 2, 0),
                codeMakeV(OP_POP),
                NULL
            }
        },
        {
            .input = "fn() {5 + 10}",
            .expConstants = {
                _INT(5), 
                _INT(10),
                _FUNC(
                    codeMakeV(OP_CONSTANT, 0),
                    codeMakeV(OP_CONSTANT, 1),
                    codeMakeV(OP_ADD),
                    codeMakeV(OP_RETURN_VALUE),
                    NULL
                ),
                _END
            },
            .expInstructions = {
                codeMakeV(OP_CLOSURE, 2, 0),
                codeMakeV(OP_POP),
                NULL
            }
        },
        {
            .input = "fn() {1; 2}",
            .expConstants = {
                _INT(1), 
                _INT(2),
                _FUNC(
                    codeMakeV(OP_CONSTANT, 0),
                    codeMakeV(OP_POP),
                    codeMakeV(OP_CONSTANT, 1),
                    codeMakeV(OP_RETURN_VALUE),
                    NULL
                ),
                _END
            },
            .expInstructions = {
                codeMakeV(OP_CLOSURE, 2, 0),
                codeMakeV(OP_POP),
                NULL
            }
        },
        {
            .input = "fn() { }",
            .expConstants = {
                _FUNC(
                    codeMakeV(OP_RETURN, 0),
                    NULL
                ),
                _END
            },
            .expInstructions = {
                codeMakeV(OP_CLOSURE, 0, 0),
                codeMakeV(OP_POP),
                NULL
            }
        }
    };

    int numTestCases = sizeof(testCases) / sizeof(testCases[0]);
    runCompilerTests(testCases, numTestCases);
}


void testCompilerScopes() {
    Compiler_t compiler = createCompiler();
    SymbolTable_t* globalSymbolTable = compiler.symbolTable;
    TEST_INT(0, compiler.scopeIndex, "scopeIndex wrong");

    compilerEmit(&compiler, OP_MUL, NULL);

    compilerEnterScope(&compiler);
    TEST_INT(1, compiler.scopeIndex, "scopeIndex wrong");
    TEST_ASSERT_MESSAGE(compiler.symbolTable->outer == globalSymbolTable, "compiler did not enclose symbolTable");

    compilerEmit(&compiler, OP_SUB, NULL);
    TEST_INT(1, sliceByteGetLen(compiler.scopes->buf[compiler.scopeIndex].instructions),
         "instructions length wrong");

    EmittedInstruction_t last = compiler.scopes->buf[compiler.scopeIndex].lastInstruction;
    TEST_INT(OP_SUB, last.opcode, "lastInstruction.opcode wrong");

    Instructions_t tmp = compilerLeaveScope(&compiler);
    TEST_INT(0, compiler.scopeIndex, "scopIndex wrong");
    TEST_ASSERT_MESSAGE(compiler.symbolTable == globalSymbolTable, "compiler did not restore globalSymbolTable");

    compilerEmit(&compiler, OP_ADD, NULL);

    TEST_INT(2, sliceByteGetLen(compiler.scopes->buf[compiler.scopeIndex].instructions),
         "instructions length wrong");

    last = compiler.scopes->buf[compiler.scopeIndex].lastInstruction;
    TEST_INT(OP_ADD, last.opcode, "lastInstruction.opcode wrong");

    EmittedInstruction_t previous = compiler.scopes->buf[compiler.scopeIndex].previousInstruction;
    TEST_INT(OP_MUL, previous.opcode, "previousInstruction.opcode wrong");

    cleanupSliceByte(tmp);
    cleanupCompiler(&compiler);
}

void testFunctionCalls() {

    TestCase_t testCases[] = {
        {
            .input = "fn() {24}();",
            .expConstants = {
                _INT(24), 
                _FUNC(
                    codeMakeV(OP_CONSTANT, 0),
                    codeMakeV(OP_RETURN_VALUE),
                    NULL
                ),
                _END
            },
            .expInstructions = {
                codeMakeV(OP_CLOSURE, 1, 0),
                codeMakeV(OP_CALL, 0),
                codeMakeV(OP_POP),
                NULL
            }
        },
        {
            .input = "let noArg = fn(){ 24 }; noArg();",
            .expConstants = {
                _INT(24), 
                _FUNC(
                    codeMakeV(OP_CONSTANT, 0),
                    codeMakeV(OP_RETURN_VALUE),
                    NULL
                ),
                _END
            },
            .expInstructions = {
                codeMakeV(OP_CLOSURE, 1, 0),
                codeMakeV(OP_SET_GLOBAL, 0),
                codeMakeV(OP_GET_GLOBAL, 0), 
                codeMakeV(OP_CALL, 0),
                codeMakeV(OP_POP),
                NULL
            }
        },
        {
            .input = "let oneArg = fn(a) { a }"
                     "oneArg(24);" ,
            .expConstants = {
                _FUNC(
                    codeMakeV(OP_GET_LOCAL, 0), 
                    codeMakeV(OP_RETURN_VALUE),
                    NULL
                ),
                _INT(24),
                _END
            },
            .expInstructions = {
                codeMakeV(OP_CLOSURE, 0, 0),
                codeMakeV(OP_SET_GLOBAL, 0),
                codeMakeV(OP_GET_GLOBAL, 0), 
                codeMakeV(OP_CONSTANT, 1), 
                codeMakeV(OP_CALL, 1),
                codeMakeV(OP_POP),
                NULL
            }
        },
        {
            .input = "let manyArg = fn(a, b, c) {a; b; c;}"
                     "manyArg(24, 25, 26);" ,
            .expConstants = {
                _FUNC(
                    codeMakeV(OP_GET_LOCAL, 0),
                    codeMakeV(OP_POP),
                    codeMakeV(OP_GET_LOCAL, 1),
                    codeMakeV(OP_POP),
                    codeMakeV(OP_GET_LOCAL, 2),
                    codeMakeV(OP_RETURN_VALUE),
                    NULL
                ),
                _INT(24),
                _INT(25),
                _INT(26),
                _END
            },
            .expInstructions = {
                codeMakeV(OP_CLOSURE, 0, 0),
                codeMakeV(OP_SET_GLOBAL, 0),
                codeMakeV(OP_GET_GLOBAL, 0), 
                codeMakeV(OP_CONSTANT, 1), 
                codeMakeV(OP_CONSTANT, 2), 
                codeMakeV(OP_CONSTANT, 3), 
                codeMakeV(OP_CALL, 3),
                codeMakeV(OP_POP),
                NULL
            }
        },
   };

    int numTestCases = sizeof(testCases) / sizeof(testCases[0]);
    runCompilerTests(testCases, numTestCases);
}

void testLetStatementScopes() {

    TestCase_t testCases[] = {
        {
            .input = "let num = 55;"
                     "fn() {num}",
            .expConstants = {
                _INT(55), 
                _FUNC(
                    codeMakeV(OP_GET_GLOBAL, 0),
                    codeMakeV(OP_RETURN_VALUE),
                    NULL
                ),
                _END
            },
            .expInstructions = {
                codeMakeV(OP_CONSTANT, 0),
                codeMakeV(OP_SET_GLOBAL, 0),
                codeMakeV(OP_CLOSURE, 1, 0),
                codeMakeV(OP_POP),
                NULL
            }
        },
        {
            .input = "fn () {\n"
                    "   let num = 55;\n"
                    "   num\n"
                    "}",
            .expConstants = {
                _INT(55), 
                _FUNC(
                    codeMakeV(OP_CONSTANT, 0),
                    codeMakeV(OP_SET_LOCAL, 0),
                    codeMakeV(OP_GET_LOCAL, 0),
                    codeMakeV(OP_RETURN_VALUE),
                    NULL
                ),
                _END
            },
            .expInstructions = {
                codeMakeV(OP_CLOSURE, 1, 0),
                codeMakeV(OP_POP),
                NULL
            }
        },
        {
            .input = "fn () {\n"
                    "   let a = 55;\n"
                    "   let b = 77;\n"
                    "   a + b\n"
                    "}",
            .expConstants = {
                _INT(55), 
                _INT(77), 
                _FUNC(
                    codeMakeV(OP_CONSTANT, 0),
                    codeMakeV(OP_SET_LOCAL, 0),
                    codeMakeV(OP_CONSTANT, 1),
                    codeMakeV(OP_SET_LOCAL, 1),
                    codeMakeV(OP_GET_LOCAL, 0),
                    codeMakeV(OP_GET_LOCAL, 1),
                    codeMakeV(OP_ADD),
                    codeMakeV(OP_RETURN_VALUE),
                    NULL
                ),
                _END
            },
            .expInstructions = {
                codeMakeV(OP_CLOSURE, 2, 0),
                codeMakeV(OP_POP),
                NULL
            }
        }
   };

    int numTestCases = sizeof(testCases) / sizeof(testCases[0]);
    runCompilerTests(testCases, numTestCases);

}

void testBuiltins() {
    TestCase_t testCases[] = {
        {
            .input = "len([]);"
                     "push([], 1);",
            .expConstants = { _INT(1), _END },
            .expInstructions = {
                codeMakeV(OP_GET_BUILTIN, 0),
                codeMakeV(OP_ARRAY, 0),
                codeMakeV(OP_CALL, 1),
                codeMakeV(OP_POP),
                codeMakeV(OP_GET_BUILTIN, 5),
                codeMakeV(OP_ARRAY, 0),
                codeMakeV(OP_CONSTANT, 0),
                codeMakeV(OP_CALL, 2),
                codeMakeV(OP_POP), 
                NULL
            }
        },
        {
            .input = "fn() {len([])}",
            .expConstants = { 
                _FUNC(
                    codeMakeV(OP_GET_BUILTIN, 0),
                    codeMakeV(OP_ARRAY, 0),
                    codeMakeV(OP_CALL, 1),
                    codeMakeV(OP_RETURN_VALUE),
                    NULL
                ),
                _END
            },
            .expInstructions = {
                codeMakeV(OP_CLOSURE, 0, 0),
                codeMakeV(OP_POP),
                NULL
            }
        },
        
   };

    int numTestCases = sizeof(testCases) / sizeof(testCases[0]);
    runCompilerTests(testCases, numTestCases);

}

void testClosures() {

    TestCase_t testCases[] = {
        {
            .input = "fn(a) {"
                     "  fn(b) {"
                     "      a + b"
                     "  }"
                     "}",
            .expConstants = {
                _FUNC(
                    codeMakeV(OP_GET_FREE, 0),
                    codeMakeV(OP_GET_LOCAL, 0),
                    codeMakeV(OP_ADD),
                    codeMakeV(OP_RETURN_VALUE),
                    NULL
                ),
                _FUNC(
                    codeMakeV(OP_GET_LOCAL, 0),
                    codeMakeV(OP_CLOSURE, 0, 1),
                    codeMakeV(OP_RETURN_VALUE),
                    NULL
                ),
                _END
            },
            .expInstructions = {
                codeMakeV(OP_CLOSURE, 1, 0),
                codeMakeV(OP_POP),
                NULL
            }
        },
        {
            .input = "fn(a) {"
                     "  fn(b) {"
                     "      fn(c) {"
                     "      a + b + c"
                     "      }"
                     "  }"
                     "}",
            .expConstants = {
                _FUNC(
                    codeMakeV(OP_GET_FREE, 0),
                    codeMakeV(OP_GET_FREE, 1),
                    codeMakeV(OP_ADD),
                    codeMakeV(OP_GET_LOCAL, 0),
                    codeMakeV(OP_ADD),
                    codeMakeV(OP_RETURN_VALUE),
                    NULL
                ),
                _FUNC(
                    codeMakeV(OP_GET_FREE, 0),
                    codeMakeV(OP_GET_LOCAL, 0),
                    codeMakeV(OP_CLOSURE, 0, 2),
                    codeMakeV(OP_RETURN_VALUE),
                    NULL
                ),
                _FUNC(
                    codeMakeV(OP_GET_LOCAL, 0),
                    codeMakeV(OP_CLOSURE, 1, 1),
                    codeMakeV(OP_RETURN_VALUE),
                    NULL
                ),
                _END
            },
            .expInstructions = {
                codeMakeV(OP_CLOSURE, 2, 0),
                codeMakeV(OP_POP),
                NULL
            }
        },
        {
            .input = "let global = 55;"
                     "fn () {"
                     "  let a = 66;"
                     "  fn() {"
                     "      let b = 77;"
                     "      fn() {"
                     "          let c = 88;"
                     "          global + a + b + c;"
                     "      }"
                     "  }"
                     "}",
            .expConstants = {
                _INT(55),
                _INT(66),
                _INT(77),
                _INT(88),
                _FUNC(
                    codeMakeV(OP_CONSTANT, 3),
                    codeMakeV(OP_SET_LOCAL, 0),
                    codeMakeV(OP_GET_GLOBAL, 0),
                    codeMakeV(OP_GET_FREE, 0),
                    codeMakeV(OP_ADD),
                    codeMakeV(OP_GET_FREE, 1),
                    codeMakeV(OP_ADD),
                    codeMakeV(OP_GET_LOCAL, 0),
                    codeMakeV(OP_ADD),
                    codeMakeV(OP_RETURN_VALUE),
                    NULL
                ),
                _FUNC(
                    codeMakeV(OP_CONSTANT, 2),
                    codeMakeV(OP_SET_LOCAL, 0),
                    codeMakeV(OP_GET_FREE, 0),
                    codeMakeV(OP_GET_LOCAL, 0),
                    codeMakeV(OP_CLOSURE, 4, 2),
                    codeMakeV(OP_RETURN_VALUE),
                    NULL
                ),
                _FUNC(
                    codeMakeV(OP_CONSTANT, 1),
                    codeMakeV(OP_SET_LOCAL, 0),
                    codeMakeV(OP_GET_LOCAL, 0),
                    codeMakeV(OP_CLOSURE, 5, 1),
                    codeMakeV(OP_RETURN_VALUE),
                    NULL
                ),
                _END
            },
            .expInstructions = {
                codeMakeV(OP_CONSTANT, 0),
                codeMakeV(OP_SET_GLOBAL, 0), 
                codeMakeV(OP_CLOSURE, 6, 0),
                codeMakeV(OP_POP),
                NULL
            }
        },
   };

    int numTestCases = sizeof(testCases) / sizeof(testCases[0]);
    runCompilerTests(testCases, numTestCases);

}

void testRecursiveFunctions() {
    TestCase_t testCases[] = {
        {
            .input = "let countDown = fn(x) { countDown(x - 1); };"
                     "countDown(1);",
            .expConstants = {
                _INT(1),
                _FUNC(
                    codeMakeV(OP_CURRENT_CLOSURE),
                    codeMakeV(OP_GET_LOCAL, 0),
                    codeMakeV(OP_CONSTANT, 0),
                    codeMakeV(OP_SUB),
                    codeMakeV(OP_CALL, 1),
                    codeMakeV(OP_RETURN_VALUE),
                    NULL
                ),
                _INT(1), 
                _END
            },
            .expInstructions = {
                codeMakeV(OP_CLOSURE, 1, 0),
                codeMakeV(OP_SET_GLOBAL, 0),
                codeMakeV(OP_GET_GLOBAL, 0),
                codeMakeV(OP_CONSTANT, 2),
                codeMakeV(OP_CALL, 1),
                codeMakeV(OP_POP),
                NULL
            }
        },
        {
            .input = "let wrapper = fn() {"
                     "  let countDown = fn(x) {countDown(x - 1)}"
                     "  countDown(1);"
                     "};"
                     "wrapper();",
            .expConstants = {
                _INT(1),
                _FUNC(
                    codeMakeV(OP_CURRENT_CLOSURE),
                    codeMakeV(OP_GET_LOCAL, 0),
                    codeMakeV(OP_CONSTANT, 0),
                    codeMakeV(OP_SUB),
                    codeMakeV(OP_CALL, 1),
                    codeMakeV(OP_RETURN_VALUE),
                    NULL
                ),
                _INT(1), 
                _FUNC(
                    codeMakeV(OP_CLOSURE, 1, 0),
                    codeMakeV(OP_SET_LOCAL, 0),
                    codeMakeV(OP_GET_LOCAL, 0), 
                    codeMakeV(OP_CONSTANT, 2),
                    codeMakeV(OP_CALL, 1),
                    codeMakeV(OP_RETURN_VALUE),
                    NULL
                ),
                _END
            },
            .expInstructions = {
                codeMakeV(OP_CLOSURE, 3, 0),
                codeMakeV(OP_SET_GLOBAL, 0),
                codeMakeV(OP_GET_GLOBAL, 0),
                codeMakeV(OP_CALL, 0),
                codeMakeV(OP_POP),
                NULL
            }
        },
   };

    int numTestCases = sizeof(testCases) / sizeof(testCases[0]);
    runCompilerTests(testCases, numTestCases);

}


void runCompilerTests(TestCase_t *tc, int numTc)
{
    for (int i = 0; i < numTc; i++)
    {
        Lexer_t *lexer = createLexer(tc[i].input);
        Parser_t *parser = createParser(lexer);
        Program_t *program = parserParseProgram(parser);

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

        cleanupBytecode(&bytecode);
        cleanupCompiler(&compiler);
        cleanupParser(&parser);
        cleanupProgram(&program);
        gcForceRun();
    }
}

void testInstructions(SliceByte_t expected[], SliceByte_t actual)
{
    int numExpected = 0;
    while (expected[numExpected] != NULL)
        numExpected++;

    SliceByte_t concatted = concatInstructions(expected, numExpected);
    char *expStr = instructionsToString(concatted);
    char *actualStr = instructionsToString(actual);

    TEST_STRING(expStr, actualStr, "Wrong instruction");

    cleanupSliceByte(concatted);
    free(expStr);
    free(actualStr);
}

SliceByte_t concatInstructions(SliceByte_t expected[], int num)
{
    SliceByte_t out = createSliceByte(0);
    for (int i = 0; i < num; i++) {
        sliceByteAppend(&out, expected[i], sliceByteGetLen(expected[i]));
    }
    return out;
}

void cleanupInstructions(Instructions_t instr[]) 
{
    int num = 0;
    while (instr[num] != NULL) {
        cleanupSliceByte(instr[num]);
        num++;
    }
    
}

void testConstants(GenericExpect_t expected[], VectorObjects_t *actual)
{
    int numExpected = 0;
    while (expected[numExpected].type != EXPECT_END)
        numExpected++;

    TEST_INT(numExpected, vectorObjectsGetCount(actual), "wrong number of constants");
    Object_t **objects = vectorObjectsGetBuffer(actual);
    for (int i = 0; i < numExpected; i++)
    {
        switch (expected[i].type)
        {
        case EXPECT_INTEGER:
            testIntegerObject(expected[i].il, objects[i]);
            break;
        case EXPECT_STRING:
            testStringObject(expected[i].sl, objects[i]);
            break;
        case EXPECT_COMPILED_FUNCTION:
            testCompiledFunction(expected[i].fl, objects[i]);
            break;
        default:
            TEST_ABORT();
        }

        gcClearRef(objects[i], GC_REF_COMPILE_CONSTANT);
    }
}


void testIntegerObject(int64_t expected, Object_t *obj)
{
    TEST_NOT_NULL(obj, "Object is null");
    TEST_INT(OBJECT_INTEGER, obj->type, "Object type not OBJECT_INTEGER");
    Integer_t *intObj = (Integer_t *)obj;
    TEST_ASSERT_EQUAL_INT64_MESSAGE(expected, intObj->value, "Object value is not correct");
}

void testStringObject(const char* expected, Object_t *obj) {
    TEST_NOT_NULL(obj, "Object is null");
    TEST_INT(OBJECT_STRING, obj->type, "Object type not OBJECT_STRING");
    String_t *strObj = (String_t *)obj;
    TEST_STRING(expected, strObj->value, "Object value is not correct");
}


void testCompiledFunction(SliceByte_t expInstr[], Object_t*obj) {
    TEST_NOT_NULL(obj, "Object is null");
    TEST_INT(OBJECT_COMPILED_FUNCTION, obj->type, "Object type not OBJECT_COMPILED_FUNCTION");
    CompiledFunction_t *comFunc = (CompiledFunction_t *)obj;

    testInstructions(expInstr, comFunc->instructions);

    // expected instr no longer needed, cleanup
    cleanupInstructions(expInstr);
}
// not needed when using generate_test_runner.rb
int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(testCompilerBasic);
    RUN_TEST(testConditionals);
    RUN_TEST(testGlobalLetStatements);
    RUN_TEST(testStringExpressions);
    RUN_TEST(testArrayLiterals);
    RUN_TEST(testHashLiterals);
    RUN_TEST(testIndexExpressions);
    RUN_TEST(testCompilerScopes);
    RUN_TEST(testFunctions);
    RUN_TEST(testFunctionCalls);
    RUN_TEST(testLetStatementScopes);
    RUN_TEST(testBuiltins);
    RUN_TEST(testClosures);
    RUN_TEST(testRecursiveFunctions);
    return UNITY_END();
}