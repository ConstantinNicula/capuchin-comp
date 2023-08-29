#ifndef _AST_H_
#define _AST_H_

#include <stdint.h>
#include <stdbool.h>
#include "token.h"
#include "vector.h"

/************************************
 *     GENERIC EXPRESSION NODE      *
 ************************************/

typedef enum ExpressionType
{
    EXPRESSION_IDENTIFIER,
    EXPRESSION_INTEGER_LITERAL,
    EXPRESSION_BOOLEAN_LITERAL,
    EXPRESSION_STRING_LITERAL,
    EXPRESSION_ARRAY_LITERAL,
    EXPRESSION_PREFIX_EXPRESSION,
    EXPRESSION_INFIX_EXPRESSION,
    EXPRESSION_IF_EXPRESSION,
    EXPRESSION_FUNCTION_LITERAL,
    EXPRESSION_CALL_EXPRESSION,
    EXPRESSION_INDEX_EXPRESSION,
    EXPRESSION_HASH_LITERAL,
    EXPRESSION_INVALID
} ExpressionType_t;

typedef struct Expression
{
    ExpressionType_t type;
    Token_t *token;
} Expression_t;

Expression_t *copyExpression(Expression_t *expr);
void cleanupExpression(Expression_t **expr);

char *expressionToString(Expression_t *expr);
const char *expressionTokenLiteral(Expression_t *expr);

// function pointers for cleanup / expresion to string
typedef void (*ExpressionCleanupFn_t)(void **);
typedef void *(*ExpressionCopyFn_t)(const void *);
typedef char *(*ExpressionToStringFn_t)(const void *);

/************************************
 *           IDENTIFIER             *
 ************************************/

typedef struct Identifier
{
    ExpressionType_t type;
    Token_t *token;
    const char *value;
} Identifier_t;

Identifier_t *createIdentifier(const Token_t *tok, const char *val);
Identifier_t *copyIdentifier(const Identifier_t *ident);
void cleanupIdentifier(Identifier_t **ident);

char *identifierToString(const Identifier_t *ident);

/************************************
 *      INTEGER LITERAL             *
 ************************************/

typedef struct IntegerLiteral
{
    ExpressionType_t type;
    Token_t *token;
    int64_t value;
} IntegerLiteral_t;

IntegerLiteral_t *createIntegerLiteral(const Token_t *tok);
IntegerLiteral_t *copyIntegerLiteral(const IntegerLiteral_t *il);
void cleanupIntegerLiteral(IntegerLiteral_t **il);

char *integerLiteralToString(const IntegerLiteral_t *il);

/************************************
 *          BOOLEAN                 *
 ************************************/

typedef struct BooleanLiteral
{
    ExpressionType_t type;
    Token_t *token;
    bool value;
} BooleanLiteral_t;

BooleanLiteral_t *createBooleanLiteral(const Token_t *tok);
BooleanLiteral_t *copyBooleanLiteral(const BooleanLiteral_t *bl);
void cleanupBooleanLiteral(BooleanLiteral_t **bl);

char *booleanLiteralToString(const BooleanLiteral_t *bl);

/************************************
 *        STRING LITERAL            *
 ************************************/

typedef struct StringLiteral
{
    ExpressionType_t type;
    Token_t *token;
    char *value;
} StringLiteral_t;

StringLiteral_t *createStringLiteral(const Token_t *tok);
StringLiteral_t *copyStringLiteral(const StringLiteral_t *sl);
void cleanupStringLiteral(StringLiteral_t **sl);

char *stringLiteralToString(const StringLiteral_t *sl);

/************************************
 *        ARRAY LITERAL             *
 ************************************/

typedef struct ArrayLiteral
{
    ExpressionType_t type;
    Token_t *token;
    Vector_t *elements;
} ArrayLiteral_t;

ArrayLiteral_t *createArrayLiteral(const Token_t *tok);
ArrayLiteral_t *copyArrayLiteral(const ArrayLiteral_t *al);
void cleanupArrayLiteral(ArrayLiteral_t **al);

char *arrayLiteralToString(const ArrayLiteral_t *al);
uint32_t arrayLiteralGetElementCount(const ArrayLiteral_t *al);
Expression_t **arrayLiteralGetElements(const ArrayLiteral_t *al);

/************************************
 *         HASH LITERAL             *
 ************************************/

typedef struct HashLiteral
{
    ExpressionType_t type;
    Token_t *token;
    Vector_t* keys;
    Vector_t* values;
} HashLiteral_t;

HashLiteral_t *createHashLiteral(const Token_t *tok);
HashLiteral_t *copyHashLiteral(const HashLiteral_t *hl);
void cleanupHashLiteral(HashLiteral_t **hl);

char *hashLiteralToString(const HashLiteral_t *hl);
uint32_t hashLiteralGetPairsCount(const HashLiteral_t *hl);
void hashLiteralGetPair(const HashLiteral_t*hl, uint32_t idx,  Expression_t** key, Expression_t** value); 
void hashLiteralSetPair(HashLiteral_t* hl, Expression_t* key, Expression_t* value); 

/************************************
 *       INDEX EXPRESSION           *
 ************************************/

typedef struct IndexExpression
{
    ExpressionType_t type;
    Token_t *token;
    Expression_t *left;
    Expression_t *right;
} IndexExpression_t;

IndexExpression_t *createIndexExpression(const Token_t *tok);
IndexExpression_t *copyIndexExpression(const IndexExpression_t *al);
void cleanupIndexExpression(IndexExpression_t **al);

char *indexExpressionToString(const IndexExpression_t *al);

/************************************
 *      PREFIX EXPRESSION           *
 ************************************/

typedef struct PrefixExpression
{
    ExpressionType_t type;
    Token_t *token;
    char *operator;
    Expression_t *right;
} PrefixExpression_t;

PrefixExpression_t *createPrefixExpresion(const Token_t *tok);
PrefixExpression_t *copyPrefixExpression(const PrefixExpression_t *exp);
void cleanupPrefixExpression(PrefixExpression_t **exp);

char *prefixExpressionToString(const PrefixExpression_t *exp);

/************************************
 *      INFIX EXPRESSION           *
 ************************************/

typedef struct InfixExpression
{
    ExpressionType_t type;
    Token_t *token;
    Expression_t *left;
    char *operator;
    Expression_t *right;
} InfixExpression_t;

InfixExpression_t *createInfixExpresion(const Token_t *tok);
InfixExpression_t *copyInfixExpression(const InfixExpression_t *exp);
void cleanupInfixExpression(InfixExpression_t **exp);

char *infixExpressionToString(const InfixExpression_t *exp);

/************************************
 *          IF EXPRESSION           *
 ************************************/

typedef struct BlockStatement BlockStatement_t;

typedef struct IfExpression
{
    ExpressionType_t type;
    Token_t *token;
    Expression_t *condition;
    BlockStatement_t *consequence;
    BlockStatement_t *alternative;
} IfExpression_t;

IfExpression_t *createIfExpresion(const Token_t *tok);
IfExpression_t *copyIfExpression(const IfExpression_t *exp);
void cleanupIfExpression(IfExpression_t **exp);

char *ifExpressionToString(const IfExpression_t *exp);

/************************************
 *    FUNCTION EXPRESSION           *
 ************************************/

typedef struct FunctionLiteral
{
    ExpressionType_t type;
    Token_t *token;
    Vector_t *parameters;
    BlockStatement_t *body;
} FunctionLiteral_t;

FunctionLiteral_t *createFunctionLiteral(const Token_t *tok);
FunctionLiteral_t *copyFunctionLiteral(const FunctionLiteral_t *exp);
void cleanupFunctionLiteral(FunctionLiteral_t **exp);

char *functionLiteralToString(const FunctionLiteral_t *exp);
void functionLiteralAppendParameter(FunctionLiteral_t *exp, const Identifier_t *param);
uint32_t functionLiteralGetParameterCount(const FunctionLiteral_t *exp);
Identifier_t **functionLiteralGetParameters(const FunctionLiteral_t *exp);

/************************************
 *        CALL EXPRESSION           *
 ************************************/

typedef struct CallExpression
{
    ExpressionType_t type;
    Token_t *token;
    Expression_t *function;
    Vector_t *arguments;
} CallExpression_t;

CallExpression_t *createCallExpression(const Token_t *tok);
CallExpression_t *copyCallExpression(const CallExpression_t *exp);
void cleanupCallExpression(CallExpression_t **exp);

char *callExpressionToString(const CallExpression_t *exp);
void callExpressionAppendArgument(CallExpression_t *exp, const Expression_t *arg);
uint32_t callExpresionGetArgumentCount(const CallExpression_t *exp);
Expression_t **callExpressionGetArguments(const CallExpression_t *exp);

/************************************
 *         GENERIC STATEMENT        *
 ************************************/

typedef enum StatementType
{
    STATEMENT_LET,
    STATEMENT_RETURN,
    STATEMENT_EXPRESSION,
    STATEMENT_BLOCK,
    STATEMENT_INVALID
} StatementType_t;

typedef struct Statement
{
    StatementType_t type;
    Token_t *token;
} Statement_t;

// function pointers for cleanup / expresion to string
typedef void (*StatementCleanupFn_t)(void **);
typedef void *(*StatementCopyFn_t)(const void *);
typedef char *(*StatementToStringFn_t)(const void *);

Statement_t *copyStatement(const Statement_t *st);
void cleanupStatement(Statement_t **st);

const char *statementTokenLiteral(const Statement_t *st);
char *statementToString(const Statement_t *st);

/************************************
 *         LET STATEMENT            *
 ************************************/

typedef struct LetStatement
{
    StatementType_t type;
    Token_t *token;
    Identifier_t *name;
    Expression_t *value;
} LetStatement_t;

LetStatement_t *createLetStatement(const Token_t *token);
LetStatement_t *copyLetStatement(const LetStatement_t *st);
void cleanupLetStatement(LetStatement_t **st);

char *letStatementToString(const LetStatement_t *st);

/************************************
 *      RETURN STATEMENT            *
 ************************************/

typedef struct ReturnStatement
{
    StatementType_t type;
    Token_t *token;
    Expression_t *returnValue;
} ReturnStatement_t;

ReturnStatement_t *createReturnStatement(const Token_t *token);
ReturnStatement_t *copyReturnStatement(const ReturnStatement_t *st);
void cleanupReturnStatement(ReturnStatement_t **st);

char *returnStatementToString(const ReturnStatement_t *st);

/************************************
 *     EXPRESSION STATEMENT         *
 ************************************/

typedef struct ExpressionStatement
{
    StatementType_t type;
    Token_t *token;
    Expression_t *expression;
} ExpressionStatement_t;

ExpressionStatement_t *createExpressionStatement(const Token_t *token);
ExpressionStatement_t *copyExpressionStatement(const ExpressionStatement_t *st);
void cleanupExpressionStatement(ExpressionStatement_t **st);

char *expressionStatementToString(const ExpressionStatement_t *st);

/************************************
 *         BLOCK STATEMENT          *
 ************************************/

typedef struct BlockStatement
{
    StatementType_t type;
    Token_t *token;
    Vector_t *statements;
} BlockStatement_t;

BlockStatement_t *createBlockStatement(const Token_t *token);
BlockStatement_t *copyBlockStatement(const BlockStatement_t *st);
void cleanupBlockStatement(BlockStatement_t **st);

char *blockStatementToString(const BlockStatement_t *st);
uint32_t blockStatementGetStatementCount(const BlockStatement_t *st);
Statement_t **blockStatementGetStatements(const BlockStatement_t *st);
void blockStatementAppendStatement(BlockStatement_t *block, const Statement_t *st);

/************************************
 *      PROGRAM NODE                *
 ************************************/

typedef struct Program
{
    Vector_t *statements;
} Program_t;

Program_t *createProgram();
Program_t *copyProgram(const Program_t *prog);
void cleanupProgram(Program_t **prog);

Statement_t **programGetStatements(const Program_t *prog);
uint32_t programGetStatementCount(const Program_t *prog);
void programAppendStatement(Program_t *prog, const Statement_t *st);
const char *programTokenLiteral(const Program_t *prog);
char *programToString(const Program_t *prog);

#endif