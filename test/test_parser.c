#include <stdlib.h> 

#include "unity.h"
#include "parser.h"
#include "ast.h"
#include "utils.h"


typedef enum {
    EXPECT_INTEGER,
    EXPECT_BOOL, 
    EXPECT_STRING,
} ExpectType_t;

typedef struct GenericExpect {
    ExpectType_t type;
    union {
        char* sl;
        int64_t il;
        bool bl; 
    };
} GenericExpect_t;

#define _BOOL(x) (GenericExpect_t){.type=EXPECT_BOOL, .bl=(x)}
#define _INT(x) (GenericExpect_t){.type=EXPECT_INTEGER, .il=(x)}
#define _STRING(x) (GenericExpect_t){.type=EXPECT_STRING, .sl=(x)}


void testLetStatement(Statement_t* s, const char* name);
void testLiteralExpression(Expression_t* expression, GenericExpect_t exp);
void testInifxExpression(Expression_t* exp, GenericExpect_t expL, const char* op, GenericExpect_t expR);
void testIntegerLiteral(Expression_t* exp, int64_t value);
void testBooleanLiteral(Expression_t* exp, bool value);
void testIdentifier(Expression_t *exp, const char* value);
void checkParserErrors(Parser_t* parser);

void setUp(void) {
    // set stuff up here
}

void tearDown(void) {
    // clean stuff up here
}

void parseTestLetStatement(void) {
    typedef struct TestCase{
        const char* input;
        const char* expIdent;
        GenericExpect_t expValue;
    }TestCase_t;


    TestCase_t tests[] = {
        {"let x = 5;", "x", _INT(5)},
        {"let y = true;", "y", _BOOL(true)},
        {"let foobar = y;", "foobar", _STRING("y")},
    };

    uint32_t cnt = sizeof(tests) / sizeof(TestCase_t);

    for (uint32_t i = 0; i < cnt; i++ ) {
        TestCase_t *tc = &tests[i];

        Lexer_t* lexer = createLexer(tc->input);
        Parser_t* parser = createParser(lexer);
        Program_t* program = parserParseProgram(parser);
        checkParserErrors(parser);


        TEST_ASSERT_NOT_NULL_MESSAGE(program, "parserParseProgram returned NULL");
        TEST_ASSERT_EQUAL_UINT32_MESSAGE(1u, programGetStatementCount(program), "program does not contain 1 statement(s)");
    
        Statement_t** statements = programGetStatements(program);
    
        testLetStatement(statements[0], tc->expIdent);

        testLiteralExpression(((LetStatement_t*)statements[0])->value, tc->expValue);

        cleanupParser(&parser);
        cleanupProgram(&program);
    }
    
}

void parserTestInvalidLetStatement(void) {
    const char* input = "let x  5;\
                    let 10;\
                    let  = 838383;";

    Lexer_t* lexer = createLexer(input);
    Parser_t* parser = createParser(lexer);
    
    Program_t* program = parserParseProgram(parser);


    cleanupParser(&parser);
    cleanupProgram(&program);
}

void parserTestReturnStatements() {
    const char* input = "return 5;\
                        return 10;\
                        return 993322;";

    Lexer_t* lexer = createLexer(input);
    Parser_t* parser = createParser(lexer);
    
    Program_t* program = parserParseProgram(parser);
    checkParserErrors(parser);


    TEST_ASSERT_NOT_NULL_MESSAGE(program, "ParserParseProgram returned NULL!");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(3u, programGetStatementCount(program), "Program does not contain 3 statements!");
    
    int num_tests = 3;
    Statement_t** statements = programGetStatements(program);

    for (int i = 0 ; i < num_tests; i++) {
        Statement_t* s = statements[i];
        TEST_ASSERT_EQUAL_INT_MESSAGE(STATEMENT_RETURN, s->type, "Check statement type!");
        TEST_ASSERT_EQUAL_STRING_MESSAGE("return", statementTokenLiteral(s), "Check statement literal!");
    }
    
    cleanupParser(&parser);
    cleanupProgram(&program);
}

void parserTestStringLiteralExpression() {
    const char* input = "\"hello world\"";

    Lexer_t* lexer = createLexer(input);
    Parser_t* parser = createParser(lexer);
    
    Program_t* program = parserParseProgram(parser);
    checkParserErrors(parser);

    TEST_ASSERT_NOT_NULL_MESSAGE(program, "ParserParseProgram returned NULL!");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(1u, programGetStatementCount(program), "Program does not contain 1 statements!");

    Statement_t* s = programGetStatements(program)[0];
    StringLiteral_t* literal = (StringLiteral_t*)((ExpressionStatement_t*)s)->expression; 
    TEST_ASSERT_EQUAL_INT_MESSAGE(EXPRESSION_STRING_LITERAL, literal->type, "Check expression type!");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("hello world", literal->value, "Check statement literal!");

    cleanupParser(&parser);
    cleanupProgram(&program);
}

void parserTestParsingHashLiteralsStringKeys() {
    const char* input = "{\"one\": 1, \"two\": 2, \"three\": 3}";
    Lexer_t* lexer = createLexer(input);
    Parser_t* parser = createParser(lexer);
    
    Program_t* program = parserParseProgram(parser);
    checkParserErrors(parser);

    TEST_ASSERT_NOT_NULL_MESSAGE(program, "ParserParseProgram returned NULL!");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(1u, programGetStatementCount(program), "Program does not contain 1 statements!");
    Statement_t* s = programGetStatements(program)[0];

    HashLiteral_t* hash = (HashLiteral_t*)((ExpressionStatement_t*)s)->expression; 
    TEST_ASSERT_EQUAL_INT_MESSAGE(EXPRESSION_HASH_LITERAL, hash->type, "Check expression type!");

    typedef struct Expect {
        const char* key;
        int64_t value;
    }Expect_t;

    Expect_t expects[] = {
        {.key = "one", .value = 1},
        {.key = "two", .value = 2},
        {.key = "three", .value = 3}
    };

    uint32_t cnt = sizeof(expects)/sizeof(expects[0]);
    for(uint32_t i = 0; i < cnt; i++) {
        Expression_t *key, *value;
        hashLiteralGetPair(hash, i, &key, &value);

        TEST_ASSERT_EQUAL_INT_MESSAGE(EXPRESSION_STRING_LITERAL, key->type, "Check key expression type!");
        TEST_ASSERT_EQUAL_STRING_MESSAGE(expects[i].key, ((StringLiteral_t*)key)->value, "Check key value!");

        testIntegerLiteral(value, expects[i].value); 
    } 

    cleanupParser(&parser);
    cleanupProgram(&program);
}

void parserTestParsingHashLiteralsIntegerKeys() {
    const char* input = "{1: 1, 2: 2, 3: 3}";
    Lexer_t* lexer = createLexer(input);
    Parser_t* parser = createParser(lexer);
    
    Program_t* program = parserParseProgram(parser);
    checkParserErrors(parser);

    TEST_ASSERT_NOT_NULL_MESSAGE(program, "ParserParseProgram returned NULL!");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(1u, programGetStatementCount(program), "Program does not contain 1 statements!");
    Statement_t* s = programGetStatements(program)[0];

    HashLiteral_t* hash = (HashLiteral_t*)((ExpressionStatement_t*)s)->expression; 
    TEST_ASSERT_EQUAL_INT_MESSAGE(EXPRESSION_HASH_LITERAL, hash->type, "Check expression type!");

    typedef struct Expect {
        int64_t key;
        int64_t value;
    }Expect_t;

    Expect_t expects[] = {
        {.key = 1, .value = 1},
        {.key = 2, .value = 2},
        {.key = 3, .value = 3}
    };

    uint32_t cnt = sizeof(expects)/sizeof(expects[0]);
    for(uint32_t i = 0; i < cnt; i++) {
        Expression_t *key, *value;
        hashLiteralGetPair(hash, i, &key, &value);

        testIntegerLiteral(key, expects[i].key); 
        testIntegerLiteral(value, expects[i].value); 
    } 

    cleanupParser(&parser);
    cleanupProgram(&program);
}

void parserTestParsingHashLiteralsBooleanKeys() {
    const char* input = "{true: 1, false: 2}";
    Lexer_t* lexer = createLexer(input);
    Parser_t* parser = createParser(lexer);
    
    Program_t* program = parserParseProgram(parser);
    checkParserErrors(parser);

    TEST_ASSERT_NOT_NULL_MESSAGE(program, "ParserParseProgram returned NULL!");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(1u, programGetStatementCount(program), "Program does not contain 1 statements!");
    Statement_t* s = programGetStatements(program)[0];

    HashLiteral_t* hash = (HashLiteral_t*)((ExpressionStatement_t*)s)->expression; 
    TEST_ASSERT_EQUAL_INT_MESSAGE(EXPRESSION_HASH_LITERAL, hash->type, "Check expression type!");

    typedef struct Expect {
        bool key;
        int64_t value;
    }Expect_t;

    Expect_t expects[] = {
        {.key = true, .value = 1},
        {.key = false, .value = 2},
    };

    uint32_t cnt = sizeof(expects)/sizeof(expects[0]);
    for(uint32_t i = 0; i < cnt; i++) {
        Expression_t *key, *value;
        hashLiteralGetPair(hash, i, &key, &value);

        testBooleanLiteral(key, expects[i].key); 
        testIntegerLiteral(value, expects[i].value); 
    } 

    cleanupParser(&parser);
    cleanupProgram(&program);
}

void parserTestParsingEmptyHashLiteral() {
    const char* input = "{}";
    Lexer_t* lexer = createLexer(input);
    Parser_t* parser = createParser(lexer);
    
    Program_t* program = parserParseProgram(parser);
    checkParserErrors(parser);

    TEST_ASSERT_NOT_NULL_MESSAGE(program, "ParserParseProgram returned NULL!");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(1u, programGetStatementCount(program), "Program does not contain 1 statements!");
    Statement_t* s = programGetStatements(program)[0];

    HashLiteral_t* hash = (HashLiteral_t*)((ExpressionStatement_t*)s)->expression; 
    TEST_ASSERT_EQUAL_INT_MESSAGE(EXPRESSION_HASH_LITERAL, hash->type, "Check expression type!");

    TEST_ASSERT_EQUAL_INT_MESSAGE(0, hashLiteralGetPairsCount(hash), "Check number of elements");
    
    cleanupParser(&parser);
    cleanupProgram(&program);
}

void parserTestParsingHashLiteralsWithExpressions() {
    const char* input = "{\"one\": 0 + 1, \"two\": 10 - 8, \"three\": 15 / 5}";
    Lexer_t* lexer = createLexer(input);
    Parser_t* parser = createParser(lexer);
    
    Program_t* program = parserParseProgram(parser);
    checkParserErrors(parser);

    TEST_ASSERT_NOT_NULL_MESSAGE(program, "ParserParseProgram returned NULL!");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(1u, programGetStatementCount(program), "Program does not contain 1 statements!");
    Statement_t* s = programGetStatements(program)[0];

    HashLiteral_t* hash = (HashLiteral_t*)((ExpressionStatement_t*)s)->expression; 
    TEST_ASSERT_EQUAL_INT_MESSAGE(EXPRESSION_HASH_LITERAL, hash->type, "Check expression type!");

    Expression_t *key, *value;
    hashLiteralGetPair(hash, 0, &key, &value);
    TEST_ASSERT_EQUAL_INT_MESSAGE(EXPRESSION_STRING_LITERAL, key->type, "Check key expression type!");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("one", ((StringLiteral_t*)key)->value, "Check key value!");
    testInifxExpression(value, _INT(0), "+", _INT(1));

    hashLiteralGetPair(hash, 1, &key, &value);
    TEST_ASSERT_EQUAL_INT_MESSAGE(EXPRESSION_STRING_LITERAL, key->type, "Check key expression type!");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("two", ((StringLiteral_t*)key)->value, "Check key value!");
    testInifxExpression(value, _INT(10), "-", _INT(8));

    hashLiteralGetPair(hash, 2, &key, &value);
    TEST_ASSERT_EQUAL_INT_MESSAGE(EXPRESSION_STRING_LITERAL, key->type, "Check key expression type!");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("three", ((StringLiteral_t*)key)->value, "Check key value!");
    testInifxExpression(value, _INT(15), "/", _INT(5));

    cleanupParser(&parser);
    cleanupProgram(&program);
}



void parserTestIfStatement() {
    const char* input = "if (x < y) { x }";

    Lexer_t* lexer = createLexer(input);
    Parser_t* parser = createParser(lexer);
    
    Program_t* program = parserParseProgram(parser);
    
    checkParserErrors(parser);
    TEST_ASSERT_NOT_NULL_MESSAGE(program, "ParserParseProgram returned NULL!");

    TEST_ASSERT_EQUAL_UINT32_MESSAGE(1u, programGetStatementCount(program), "Program does not contain 1 statement!");
    Statement_t* stmt = programGetStatements(program)[0];
    
    TEST_ASSERT_EQUAL_INT_MESSAGE(STATEMENT_EXPRESSION, stmt->type, "Statement type not STATEMENT_EXPRESSION");
    ExpressionStatement_t* exprStmt = (ExpressionStatement_t*) stmt;

    TEST_ASSERT_EQUAL_INT_MESSAGE(EXPRESSION_IF_EXPRESSION, exprStmt->expression->type, "Expresion type not EXPRESSION_IF_EXPRESSION");
    IfExpression_t* ifExpr = (IfExpression_t*) exprStmt->expression;

    // Condition
    testInifxExpression(ifExpr->condition, (GenericExpect_t)_STRING("x"), "<", (GenericExpect_t)_STRING("y"));

    // Consequence block 
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(1u, blockStatementGetStatementCount(ifExpr->consequence), "Consequence block does not contain 1 statement!");
    Statement_t* conseqStmt = blockStatementGetStatements(ifExpr->consequence)[0];

    TEST_ASSERT_EQUAL_INT_MESSAGE(STATEMENT_EXPRESSION, conseqStmt->type, "Consequence statement type not STATEMENT_EXPRESSION");
    ExpressionStatement_t* conseqExprStmt = (ExpressionStatement_t*) conseqStmt;

    testIdentifier(conseqExprStmt->expression, "x");

    // Alternative block 
    TEST_ASSERT_NULL_MESSAGE(ifExpr->alternative, "Alternative block statement not null");

    cleanupParser(&parser);
    cleanupProgram(&program);
}

void parserTestIfElseStatement() {
    const char* input = "if (x < y) { x } else { y }";

    Lexer_t* lexer = createLexer(input);
    Parser_t* parser = createParser(lexer);
    
    Program_t* program = parserParseProgram(parser);
    
    checkParserErrors(parser);
    TEST_ASSERT_NOT_NULL_MESSAGE(program, "ParserParseProgram returned NULL!");

    TEST_ASSERT_EQUAL_UINT32_MESSAGE(1u, programGetStatementCount(program), "Program does not contain 1 statement!");
    Statement_t* stmt = programGetStatements(program)[0];
    
    TEST_ASSERT_EQUAL_INT_MESSAGE(STATEMENT_EXPRESSION, stmt->type, "Statement type not STATEMENT_EXPRESSION");
    ExpressionStatement_t* exprStmt = (ExpressionStatement_t*) stmt;

    TEST_ASSERT_EQUAL_INT_MESSAGE(EXPRESSION_IF_EXPRESSION, exprStmt->expression->type, "Expresion type not EXPRESSION_IF_EXPRESSION");
    IfExpression_t* ifExpr = (IfExpression_t*) exprStmt->expression;

    // Condition
    testInifxExpression(ifExpr->condition, (GenericExpect_t)_STRING("x"), "<", (GenericExpect_t)_STRING("y"));

    // Consequence block 
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(1u, blockStatementGetStatementCount(ifExpr->consequence), "Consequence block does not contain 1 statement!");
    Statement_t* conseqStmt = blockStatementGetStatements(ifExpr->consequence)[0];

    TEST_ASSERT_EQUAL_INT_MESSAGE(STATEMENT_EXPRESSION, conseqStmt->type, "Consequence statement type not STATEMENT_EXPRESSION");
    ExpressionStatement_t* conseqExprStmt = (ExpressionStatement_t*) conseqStmt;

    testIdentifier(conseqExprStmt->expression, "x");

    // Alternative block 
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(1u, blockStatementGetStatementCount(ifExpr->alternative), "Consequence block does not contain 1 statement!");
    Statement_t* alterStmt = blockStatementGetStatements(ifExpr->alternative)[0];

    TEST_ASSERT_EQUAL_INT_MESSAGE(STATEMENT_EXPRESSION, alterStmt->type, "Consequence statement type not STATEMENT_EXPRESSION");
    ExpressionStatement_t* alterExprStmt = (ExpressionStatement_t*) alterStmt;

    testIdentifier(alterExprStmt->expression, "y");
    
    cleanupParser(&parser);
    cleanupProgram(&program);
    
}


void parserTestIdentifierExpression() {
    const char* input = "foobar;";

    Lexer_t* lexer = createLexer(input);
    Parser_t* parser = createParser(lexer);
    Program_t* prog = parserParseProgram(parser);
    checkParserErrors(parser);

    TEST_ASSERT_EQUAL_INT_MESSAGE(1, programGetStatementCount(prog), "Not enough statements in code");
    Statement_t** stmts = programGetStatements(prog);

    TEST_ASSERT_EQUAL_INT_MESSAGE(STATEMENT_EXPRESSION, stmts[0]->type, "Statements[0] is not Expression Statement");
    ExpressionStatement_t* es = (ExpressionStatement_t*)stmts[0];

    TEST_ASSERT_EQUAL_INT_MESSAGE(EXPRESSION_IDENTIFIER, es->expression->type, "Expression type not EXPRESSION_IDENTIFIER");
    Identifier_t* ident = (Identifier_t*)es->expression;

    TEST_ASSERT_EQUAL_STRING_MESSAGE("foobar", ident->value, "Check ident value");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("foobar", ident->token->literal, "Check literal value");

    cleanupParser(&parser);
    cleanupProgram(&prog);
}


void parserTestIntegerLiteralExpression() {
    const char* input = "5;";

    Lexer_t* lexer = createLexer(input);
    Parser_t* parser = createParser(lexer);
    Program_t* prog = parserParseProgram(parser);
    checkParserErrors(parser);


    TEST_ASSERT_EQUAL_INT_MESSAGE(1, programGetStatementCount(prog), "Not enough statements in code"); 
    Statement_t** stmts = programGetStatements(prog);

    TEST_ASSERT_EQUAL_INT_MESSAGE(STATEMENT_EXPRESSION, stmts[0]->type, "Statements[0] is not Expression Statement");
    ExpressionStatement_t* es = (ExpressionStatement_t*)stmts[0];

    testIntegerLiteral(es->expression, 5);

    cleanupParser(&parser);
    cleanupProgram(&prog);
}

void parserTestPrefixExpressions() {
    typedef struct TestCase {
        const char* input;
        const char* operator;
        GenericExpect_t integerValue;
    } TestCase_t;
    
    TestCase_t prefixTests[] = {
        {"!5;", "!", _INT(5)},
        {"-15;", "-", _INT(15)}
    };

    uint32_t cnt = sizeof(prefixTests) / sizeof(TestCase_t);

    for (uint32_t i = 0; i < cnt; ++i) {
        TestCase_t *tc = &prefixTests[i];

        Lexer_t* lexer = createLexer(tc->input);
        Parser_t* parser = createParser(lexer);
        Program_t* prog = parserParseProgram(parser);
        checkParserErrors(parser);

        TEST_ASSERT_EQUAL_INT_MESSAGE(1, programGetStatementCount(prog), "Statement count check!");
        Statement_t** stmts = programGetStatements(prog);
        
        TEST_ASSERT_EQUAL_INT_MESSAGE(STATEMENT_EXPRESSION, stmts[0]->type, "Statements[0] is not Expression Statement");
        ExpressionStatement_t* es = (ExpressionStatement_t*)stmts[0];

        TEST_ASSERT_EQUAL_INT_MESSAGE(EXPRESSION_PREFIX_EXPRESSION, es->expression->type, "Expression type not EXPRESSION_PREFIX_EXPRESSION");
        PrefixExpression_t* pe = (PrefixExpression_t*)es->expression;

        TEST_ASSERT_EQUAL_STRING_MESSAGE(tc->operator, pe->operator, "Operator check!");
        testLiteralExpression(pe->right, tc->integerValue);

        cleanupProgram(&prog);
        cleanupParser(&parser);
    }
}

void parserTestInfixExpressions() {
    typedef struct TestCase{
        const char* input;
        GenericExpect_t leftValue;
        const char* operator;
        GenericExpect_t rightValue;
    }TestCase_t;

    TestCase_t infixTests[] = {
        {"5 + 5;", _INT(5), "+", _INT(5)},
        {"5 - 5;", _INT(5), "-", _INT(5)},
        {"5 * 5;", _INT(5), "*", _INT(5)},
        {"5 / 5;", _INT(5), "/", _INT(5)},
        {"5 > 5;", _INT(5), ">", _INT(5)},
        {"5 < 5;", _INT(5), "<", _INT(5)},
        {"5 == 5;", _INT(5), "==", _INT(5)},
        {"5 != 5;", _INT(5), "!=", _INT(5)},
        {"true == true", _BOOL(true), "==", _BOOL(true)},
        {"true != false", _BOOL(true), "!=", _BOOL(false)},
        {"false == false", _BOOL(false), "==", _BOOL(false)},
    };

    uint32_t cnt = sizeof(infixTests) / sizeof(TestCase_t);

    for (uint32_t i = 0; i < cnt; i++ ) {
        TestCase_t *tc = &infixTests[i];

        Lexer_t* lexer = createLexer(tc->input);
        Parser_t* parser = createParser(lexer);
        Program_t* prog = parserParseProgram(parser);
        checkParserErrors(parser);

        TEST_ASSERT_EQUAL_INT_MESSAGE(1, programGetStatementCount(prog), "Statement count check!");
        Statement_t** stmts = programGetStatements(prog);

        TEST_ASSERT_EQUAL_INT_MESSAGE(STATEMENT_EXPRESSION, stmts[0]->type, "Statements[0] is not Expression Statement");
        ExpressionStatement_t* es = (ExpressionStatement_t*)stmts[0];

        TEST_ASSERT_EQUAL_INT_MESSAGE(EXPRESSION_INFIX_EXPRESSION, es->expression->type, "Expression type not EXPRESSION_INFIX_EXPRESSION");
        InfixExpression_t* ie = (InfixExpression_t*)es->expression;

        testLiteralExpression(ie->left, tc->leftValue);
        TEST_ASSERT_EQUAL_STRING_MESSAGE(tc->operator, ie->operator, "Operator check!");
        testLiteralExpression(ie->right, tc->rightValue);

        cleanupProgram(&prog);
        cleanupParser(&parser);
    }

}

void parserTestOperatorPrecedenceParsing() {
    typedef struct TestCase{
        const char* input;
        const char* expected;
    }TestCase_t;

    TestCase_t tests[] = {
        { "-a * b", "((-a) * b)"},
        { "!-a", "(!(-a))"},
        {"a + b + c", "((a + b) + c)"},
        {"a + b - c", "((a + b) - c)"},
        {"a * b * c", "((a * b) * c)"},
        {"a * b / c", "((a * b) / c)"},
        {"a + b / c", "(a + (b / c))"},
        {"a + b * c + d / e - f", "(((a + (b * c)) + (d / e)) - f)",},
        {"3 + 4; -5 * 5", "(3 + 4)\n((-5) * 5)",},
        {"5 > 4 == 3 < 4", "((5 > 4) == (3 < 4))",},
        {"5 < 4 != 3 > 4", "((5 < 4) != (3 > 4))",},
        {"3 + 4 * 5 == 3 * 1 + 4 * 5", "((3 + (4 * 5)) == ((3 * 1) + (4 * 5)))",},
        {"3 + 4 * 5 == 3 * 1 + 4 * 5","((3 + (4 * 5)) == ((3 * 1) + (4 * 5)))",},
        {"true", "true" },
		{"false","false"},
		{"3 > 5 == false","((3 > 5) == false)"},
		{"3 < 5 == true", "((3 < 5) == true)"},
        {"1 + (2 + 3) + 4", "((1 + (2 + 3)) + 4)"},
        {"(5 + 5) * 2", "((5 + 5) * 2)"},
        {"2 / (5 + 5)", "(2 / (5 + 5))"},
        {"-(5 + 5)", "(-(5 + 5))"},
        {"!(true == true)", "(!(true == true))"},
        {"a + add(b * c) + d","((a + add((b * c))) + d)"},
        {"add(a, b, 1, 2 * 3, 4 + 5, add(6, 7 * 8))", "add(a, b, 1, (2 * 3), (4 + 5), add(6, (7 * 8)))"},
        {"add(a + b + c * d / f + g)", "add((((a + b) + ((c * d) / f)) + g))"},
        { "a * [1, 2, 3, 4][b * c] * d",
        "((a * ([1, 2, 3, 4][(b * c)])) * d)"},
        { "add(a * b[2], b[1], 2 * [1, 2][1])",
        "add((a * (b[2])), (b[1]), (2 * ([1, 2][1])))"}
    };

    uint32_t cnt = sizeof(tests) / sizeof(TestCase_t);

    for (uint32_t i = 0; i < cnt; i++ ) {
        TestCase_t *tc = &tests[i];

        Lexer_t* lexer = createLexer(tc->input);
        Parser_t* parser = createParser(lexer);
        Program_t* prog = parserParseProgram(parser);
        checkParserErrors(parser);

        char* actual = programToString(prog);
        TEST_ASSERT_EQUAL_STRING_MESSAGE(tc->expected, actual, "Check statement");
        free(actual);

        cleanupProgram(&prog);
        cleanupParser(&parser);
    }
}


void parserTestFunctionLiteral() {
    const char* input = "fn ( x, y) { x + y; }";
    
    Lexer_t* lexer = createLexer(input);
    Parser_t* parser = createParser(lexer);
    Program_t* prog = parserParseProgram(parser);
    checkParserErrors(parser);

    TEST_ASSERT_EQUAL_INT_MESSAGE(1, programGetStatementCount(prog), "Not enough statements in program");
    Statement_t* stmt = programGetStatements(prog)[0];

    TEST_ASSERT_EQUAL_INT_MESSAGE(STATEMENT_EXPRESSION, stmt->type, "Statement is not Expression Statement");
    ExpressionStatement_t* exprStmt = (ExpressionStatement_t*)stmt;

    TEST_ASSERT_EQUAL_INT_MESSAGE(EXPRESSION_FUNCTION_LITERAL, exprStmt->expression->type, "Expression type not EXPRESSION_FUNCTION_LITERAL");
    FunctionLiteral_t* fl = (FunctionLiteral_t*)exprStmt->expression;

    Identifier_t** params = functionLiteralGetParameters(fl);
    testIdentifier((Expression_t*)params[0], "x");
    testIdentifier((Expression_t*)params[1], "y");
    
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, blockStatementGetStatementCount(fl->body), "Not enough statements in function body");
    Statement_t* bodyStmt = blockStatementGetStatements(fl->body)[0];

    TEST_ASSERT_EQUAL_INT_MESSAGE(STATEMENT_EXPRESSION, bodyStmt->type, "Body statement is not Expression Statement");
    ExpressionStatement_t* bodyExprStmt = (ExpressionStatement_t*)bodyStmt;

    testInifxExpression(bodyExprStmt->expression, _STRING("x"), "+", _STRING("y"));

    cleanupParser(&parser);
    cleanupProgram(&prog);
}

void parserTestFunctionParameterParsing() {
    typedef struct TestCase{
        const char* input;
        GenericExpect_t expectedParams[10];
        uint32_t cnt;
    }TestCase_t;

    TestCase_t tests[] = {
        {.input="fn() {};", .expectedParams={}, .cnt=0},
        {.input="fn(x) {};", .expectedParams={_STRING("x")}, .cnt=1},
        {.input="fn(x, yes) {};", .expectedParams={_STRING("x"), _STRING("yes")}, .cnt=2},
    };

    uint32_t cnt = sizeof(tests) / sizeof(TestCase_t);

    for (uint32_t i = 0; i < cnt; i++ ) {
        TestCase_t *tc = &tests[i];

        Lexer_t* lexer = createLexer(tc->input);
        Parser_t* parser = createParser(lexer);
        Program_t* prog = parserParseProgram(parser);
        checkParserErrors(parser);

        ExpressionStatement_t* stmt = (ExpressionStatement_t*)programGetStatements(prog)[0];
        FunctionLiteral_t* function = (FunctionLiteral_t*)stmt->expression; 

        TEST_ASSERT_EQUAL_UINT32_MESSAGE(tc->cnt, functionLiteralGetParameterCount(function), "Invalid parameter count");
        Identifier_t** params = functionLiteralGetParameters(function);

        for (uint32_t j = 0; j < tc->cnt; j++) {
            testLiteralExpression((Expression_t*)params[j], tc->expectedParams[j]);
        }

        cleanupProgram(&prog);
        cleanupParser(&parser);
    }


}


void parserTestCallExpressionParsing() {
    const char* input = "add(1, 2 * 3, 4 + 5)";
    
    Lexer_t* lexer = createLexer(input);
    Parser_t* parser = createParser(lexer);
    Program_t* prog = parserParseProgram(parser);
    checkParserErrors(parser);

    TEST_ASSERT_EQUAL_INT_MESSAGE(1, programGetStatementCount(prog), "Not enough statements in program");
    Statement_t* stmt = programGetStatements(prog)[0];

    TEST_ASSERT_EQUAL_INT_MESSAGE(STATEMENT_EXPRESSION, stmt->type, "Statement is not Expression Statement");
    ExpressionStatement_t* exprStmt = (ExpressionStatement_t*)stmt;

    TEST_ASSERT_EQUAL_INT_MESSAGE(EXPRESSION_CALL_EXPRESSION, exprStmt->expression->type, "Expression type not EXPRESSION_CALL_EXPRESSION");
    CallExpression_t* callExp = (CallExpression_t*)exprStmt->expression;

    testIdentifier(callExp->function, "add");

    TEST_ASSERT_EQUAL_UINT32_MESSAGE(3u, callExpresionGetArgumentCount(callExp), "Wrong number of arguments");
    Expression_t** args = callExpressionGetArguments(callExp);

    testLiteralExpression(args[0], _INT(1));
    testInifxExpression(args[1], _INT(2), "*", _INT(3));
    testInifxExpression(args[2], _INT(4), "+", _INT(5));

    cleanupParser(&parser);
    cleanupProgram(&prog);
}

void parserTestParsingArrayLiterals() {
    const char* input = "[1, 2 * 3, 4 + 5]";
    
    Lexer_t* lexer = createLexer(input);
    Parser_t* parser = createParser(lexer);
    Program_t* prog = parserParseProgram(parser);
    checkParserErrors(parser);

    TEST_ASSERT_EQUAL_INT_MESSAGE(1, programGetStatementCount(prog), "Not enough statements in program");
    Statement_t* stmt = programGetStatements(prog)[0];

    TEST_ASSERT_EQUAL_INT_MESSAGE(STATEMENT_EXPRESSION, stmt->type, "Statement is not Expression Statement");
    ExpressionStatement_t* exprStmt = (ExpressionStatement_t*)stmt;

    TEST_ASSERT_EQUAL_INT_MESSAGE(EXPRESSION_ARRAY_LITERAL, exprStmt->expression->type, "Expression type not EXPRESSION_ARRAY_LITERAL");
    ArrayLiteral_t* array = (ArrayLiteral_t*)exprStmt->expression;


    TEST_ASSERT_EQUAL_UINT32_MESSAGE(3u, arrayLiteralGetElementCount(array), "Wrong number of elements");
    Expression_t** args = arrayLiteralGetElements(array);

    testLiteralExpression(args[0], _INT(1));
    testInifxExpression(args[1], _INT(2), "*", _INT(3));
    testInifxExpression(args[2], _INT(4), "+", _INT(5));

    cleanupParser(&parser);
    cleanupProgram(&prog);

}

void parserTestParsingIndexExpressions() {
    const char* input = "myArray[1 + 1]";
    
    Lexer_t* lexer = createLexer(input);
    Parser_t* parser = createParser(lexer);
    Program_t* prog = parserParseProgram(parser);
    checkParserErrors(parser);

    TEST_ASSERT_EQUAL_INT_MESSAGE(1, programGetStatementCount(prog), "Not enough statements in program");
    Statement_t* stmt = programGetStatements(prog)[0];

    TEST_ASSERT_EQUAL_INT_MESSAGE(STATEMENT_EXPRESSION, stmt->type, "Statement is not Expression Statement");
    ExpressionStatement_t* exprStmt = (ExpressionStatement_t*)stmt;

    TEST_ASSERT_EQUAL_INT_MESSAGE(EXPRESSION_INDEX_EXPRESSION, exprStmt->expression->type, "Expression type not EXPRESSION_INDEX_EXPRESSION");
    IndexExpression_t* exprIndex = (IndexExpression_t*)exprStmt->expression;

    testIdentifier(exprIndex->left, "myArray");
    testInifxExpression(exprIndex->right, _INT(1), "+", _INT(1));
    cleanupParser(&parser);
    cleanupProgram(&prog);


} 

void testLetStatement(Statement_t* s, const char* name) {
    TEST_ASSERT_EQUAL_STRING_MESSAGE("let", statementTokenLiteral(s), "Check statement literal!");
    
    TEST_ASSERT_EQUAL_INT_MESSAGE(STATEMENT_LET, s->type, "Check statement type!");
    LetStatement_t* letStmt = (LetStatement_t*) s;

    TEST_ASSERT_EQUAL_STRING_MESSAGE(name, letStmt->name->value, "Check name value!");
    TEST_ASSERT_EQUAL_STRING_MESSAGE(name, letStmt->name->token->literal, "Check name literal!");
}


void testInifxExpression(Expression_t* expr, GenericExpect_t expL, const char* op, GenericExpect_t expR) {
    TEST_ASSERT_EQUAL_INT_MESSAGE(EXPRESSION_INFIX_EXPRESSION, expr->type, "Expression not EXPRESSION_INFIX_EXPRESSION");
    InfixExpression_t* infExpr = (InfixExpression_t*) expr; 

    testLiteralExpression(infExpr->left, expL);
    TEST_ASSERT_EQUAL_STRING_MESSAGE(infExpr->operator, op, "Infix expression has wrong operator");
    testLiteralExpression(infExpr->right, expR);
}




void testLiteralExpression(Expression_t* expression, GenericExpect_t exp) {
    switch(exp.type){
        case EXPECT_STRING:
            testIdentifier(expression, exp.sl);
            break;
        case EXPECT_INTEGER: 
            testIntegerLiteral(expression, exp.il);
            break;
        case EXPECT_BOOL:
            testBooleanLiteral(expression, exp.bl);
            break;        
    }
}

void testIdentifier(Expression_t *exp, const char* value) {
    TEST_ASSERT_EQUAL_INT_MESSAGE(EXPRESSION_IDENTIFIER, exp->type, "Check expression is EXPRESSION_IDENTIFIER");
    Identifier_t* id = (Identifier_t*)exp;

    TEST_ASSERT_EQUAL_STRING_MESSAGE(value, id->value, "Check identifier value");
    TEST_ASSERT_EQUAL_STRING_MESSAGE(value, id->token->literal, "Check identifeier literal");
}


void testIntegerLiteral(Expression_t* exp, int64_t value) {
    TEST_ASSERT_EQUAL_INT_MESSAGE(EXPRESSION_INTEGER_LITERAL, exp->type, "Check expression is EXPRESSION_INTEGER_LITERAL");
    IntegerLiteral_t* il = (IntegerLiteral_t*)exp;

    TEST_ASSERT_EQUAL_INT_MESSAGE(value, il->value, "Check ident value");

    char* lit = strFormat("%d", value);
    TEST_ASSERT_EQUAL_STRING_MESSAGE(lit, il->token->literal, "Check literal value");
    free(lit);
}

void testBooleanLiteral(Expression_t* exp, bool value) {
    TEST_ASSERT_EQUAL_INT_MESSAGE(EXPRESSION_BOOLEAN_LITERAL, exp->type, "Check expression is EXPRESSION_BOOLEAN_LITERAL");
    BooleanLiteral_t* bl = (BooleanLiteral_t*)exp;

    TEST_ASSERT_EQUAL_INT_MESSAGE(value, bl->value, "Check identifier value");
    TEST_ASSERT_EQUAL_STRING_MESSAGE( (value ? "true" : "false"), bl->token->literal, "Check identifeier literal");
}


void checkParserErrors(Parser_t* parser) {
    uint32_t errCnt = parserGetErrorCount(parser);
    const char** errors = parserGetErrors(parser);
    for (uint32_t i = 0; i < errCnt; i++) 
    {
        TEST_MESSAGE(errors[i]);
    }

    TEST_ASSERT_EQUAL_UINT32_MESSAGE(0u, errCnt, "Parser has errors!");
}

// not needed when using generate_test_runner.rb
int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(parseTestLetStatement);
    RUN_TEST(parserTestInvalidLetStatement);
    RUN_TEST(parserTestReturnStatements);
    RUN_TEST(parserTestIdentifierExpression);
    RUN_TEST(parserTestIntegerLiteralExpression);
    RUN_TEST(parserTestPrefixExpressions);
    RUN_TEST(parserTestInfixExpressions);
    RUN_TEST(parserTestOperatorPrecedenceParsing);
    RUN_TEST(parserTestIfStatement);
    RUN_TEST(parserTestIfElseStatement);
    RUN_TEST(parserTestFunctionLiteral);
    RUN_TEST(parserTestFunctionParameterParsing);
    RUN_TEST(parserTestCallExpressionParsing);
    RUN_TEST(parserTestStringLiteralExpression);
    RUN_TEST(parserTestParsingArrayLiterals);
    RUN_TEST(parserTestParsingIndexExpressions);
    RUN_TEST(parserTestParsingHashLiteralsStringKeys);
    RUN_TEST(parserTestParsingEmptyHashLiteral);
    RUN_TEST(parserTestParsingHashLiteralsWithExpressions);
    RUN_TEST(parserTestParsingHashLiteralsBooleanKeys);
    RUN_TEST(parserTestParsingHashLiteralsIntegerKeys);
    return UNITY_END();
}