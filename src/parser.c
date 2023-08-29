#include <malloc.h>
#include <stdbool.h>
#include <string.h> 

#include "parser.h"
#include "utils.h"

/* Operator precedence levels */
typedef enum PrecValue{
    PREC_LOWEST = 0,
    PREC_EQUALS, 
    PREC_LESSGREATER,
    PREC_SUM, 
    PREC_PRODUCT, 
    PREC_PREFIX, 
    PREC_CALL,
    PREC_INDEX
} PrecValue_t;

/* Precedences for operators, values which are not define default to 0-PREC_LOWEST*/
static PrecValue_t _precedences[_TOKEN_TYPE_CNT] = {
    [TOKEN_EQ]=PREC_EQUALS,
    [TOKEN_NOT_EQ]=PREC_EQUALS,
    [TOKEN_LT]=PREC_LESSGREATER,
    [TOKEN_GT]=PREC_LESSGREATER,
    [TOKEN_PLUS]=PREC_SUM,
    [TOKEN_MINUS]=PREC_SUM,
    [TOKEN_SLASH]=PREC_PRODUCT,
    [TOKEN_ASTERISK]=PREC_PRODUCT,
    [TOKEN_LPAREN]=PREC_CALL,
    [TOKEN_LBRACKET]=PREC_INDEX
};

/* Parsing functions */

static Statement_t* parserParseStatement(Parser_t* parser);
static Statement_t* parserParseLetStatement(Parser_t* parser);
static Statement_t* parserParseReturnStatement(Parser_t* parser);
static Statement_t* parserParseExpressionStatement(Parser_t* parser);
static BlockStatement_t* parserParseBlockStatement(Parser_t* parser);

static Expression_t* parserParseExpression(Parser_t* parser, PrecValue_t precedence);
static Expression_t* parserParseIdentifier(Parser_t* parser);
static Expression_t* parserParseIntegerLiteral(Parser_t* parser);
static Expression_t* parserParseStringLiteral(Parser_t* parser);
static Expression_t* parserParsePrefixExpression(Parser_t* parser);
static Expression_t* parserParseInfixExpression(Parser_t* parser, Expression_t* left);
static Expression_t* parserParseBoolean(Parser_t* parser);
static Expression_t* parserParseGroupedExpression(Parser_t* parser);
static Expression_t* parserParseIfExpression(Parser_t* parser);
static Expression_t* parserParseFunctionLiteral(Parser_t* parser);
static void parserParseFunctionParameters(Parser_t* parser, FunctionLiteral_t* fl);
static Expression_t* parserParseCallExpression(Parser_t* parser, Expression_t* function);
static Expression_t* parserParseArrayLiteral(Parser_t* parser);
static Vector_t* parserParseExpressionList(Parser_t* parser, TokenType_t end); 
static Expression_t* parserParseIndexExpression(Parser_t*parser, Expression_t* left);
static Expression_t* parserParseHashLiteral(Parser_t* parser);

static PrecValue_t parserPeekPrecedence(Parser_t* parser);
static PrecValue_t parserCurPrecedence(Parser_t* parser);

static void parserRegisterPrefix(Parser_t* parser, TokenType_t tokType, PrefixParseFn_t fn);
static void parserRegisterInfix(Parser_t* parser, TokenType_t tokType, InfixParseFn_t fn);


/* Token manipulation */

static void parserNextToken(Parser_t* parser);
static bool parserCurTokenIs(Parser_t* parser, TokenType_t tokType);
static bool parserPeekTokenIs(Parser_t* parser, TokenType_t tokType);
static bool parserExpectPeek(Parser_t* parser, TokenType_t tokType);

/* ERROR handling */

static void parserAppendError(Parser_t* parser, char*);
static void parserNoPrefixParseFnError(Parser_t* parser, TokenType_t tok);
static void parserPeekError(Parser_t* parser, TokenType_t expTokenType);


/* Allocation & Cleanup functions */

Parser_t* createParser(Lexer_t* lexer) {
    Parser_t* parser = mallocChk(sizeof(Parser_t));

    parser->lexer = lexer;
    parser->curToken = parser->peekToken = NULL;

    memset(parser->prefixParseFns, 0, sizeof(PrefixParseFn_t) * _TOKEN_TYPE_CNT);
    parserRegisterPrefix(parser, TOKEN_IDENT, parserParseIdentifier);
    parserRegisterPrefix(parser, TOKEN_INT, parserParseIntegerLiteral);
    parserRegisterPrefix(parser, TOKEN_BANG, parserParsePrefixExpression);
    parserRegisterPrefix(parser, TOKEN_MINUS, parserParsePrefixExpression);
    parserRegisterPrefix(parser, TOKEN_TRUE, parserParseBoolean);
    parserRegisterPrefix(parser, TOKEN_FALSE, parserParseBoolean);
    parserRegisterPrefix(parser, TOKEN_LPAREN, parserParseGroupedExpression);
    parserRegisterPrefix(parser, TOKEN_IF, parserParseIfExpression);
    parserRegisterPrefix(parser, TOKEN_FUNCTION, parserParseFunctionLiteral);
    parserRegisterPrefix(parser, TOKEN_STRING, parserParseStringLiteral);
    parserRegisterPrefix(parser, TOKEN_LBRACKET, parserParseArrayLiteral);
    parserRegisterPrefix(parser, TOKEN_LBRACE, parserParseHashLiteral);

    memset(parser->infixParserFns, 0, sizeof(InfixParseFn_t) * _TOKEN_TYPE_CNT);
    parserRegisterInfix(parser, TOKEN_PLUS, parserParseInfixExpression);
    parserRegisterInfix(parser, TOKEN_MINUS, parserParseInfixExpression);
    parserRegisterInfix(parser, TOKEN_SLASH, parserParseInfixExpression);
    parserRegisterInfix(parser, TOKEN_ASTERISK, parserParseInfixExpression);
    parserRegisterInfix(parser, TOKEN_EQ, parserParseInfixExpression);
    parserRegisterInfix(parser, TOKEN_NOT_EQ, parserParseInfixExpression);
    parserRegisterInfix(parser, TOKEN_LT, parserParseInfixExpression);
    parserRegisterInfix(parser, TOKEN_GT, parserParseInfixExpression);
    parserRegisterInfix(parser, TOKEN_LPAREN, parserParseCallExpression);
    parserRegisterInfix(parser, TOKEN_LBRACKET, parserParseIndexExpression);
    parser->errors = createVector();

    parserNextToken(parser);
    parserNextToken(parser);
    return parser;
}

static void cleanupError(char** str) {
    if(!*str) return;
    free(*str);
}

void cleanupParser(Parser_t** parser) {
    if (!(*parser)) 
        return;
    cleanupLexer(&(*parser)->lexer);
    cleanupToken(&(*parser)->peekToken);
    cleanupToken(&(*parser)->curToken);

    cleanupVector(&(*parser)->errors, (VectorElemCleanupFn_t)cleanupError);
    
    free(*parser);
    *parser = NULL;
}



/* Core parsing logic */

Program_t* parserParseProgram(Parser_t* parser) {
    Program_t* program = createProgram();

    while (!parserCurTokenIs(parser, TOKEN_EOF)) {
        Statement_t* stmt = parserParseStatement(parser);
        
        if (stmt != NULL) {
            programAppendStatement(program, stmt);
        } 
    
        parserNextToken(parser);
    }

    return program;
}

static Statement_t* parserParseStatement(Parser_t* parser) {
    switch (parser->curToken->type) {
        case TOKEN_LET: 
            return parserParseLetStatement(parser);
        case TOKEN_RETURN: 
            return parserParseReturnStatement(parser);
        default:
            return parserParseExpressionStatement(parser);
    }
}

static Statement_t* parserParseLetStatement(Parser_t* parser) {
    LetStatement_t* stmt = createLetStatement(parser->curToken);

    if (!parserExpectPeek(parser, TOKEN_IDENT)) {
        goto cleanup;
    }

    stmt->name = createIdentifier(parser->curToken, parser->curToken->literal);

    if (!parserExpectPeek(parser, TOKEN_ASSIGN)) {
        goto cleanup;
    }

    parserNextToken(parser);

    stmt->value = parserParseExpression(parser, PREC_LOWEST);

    if (parserPeekTokenIs(parser, TOKEN_SEMICOLON))
    {
        parserNextToken(parser);
    }

    return (Statement_t*)stmt;

cleanup:
    cleanupLetStatement(&stmt);
    return NULL;
}


static Statement_t* parserParseReturnStatement(Parser_t* parser) {
    ReturnStatement_t* stmt = createReturnStatement(parser->curToken);
    parserNextToken(parser);

    stmt->returnValue = parserParseExpression(parser, PREC_LOWEST);

    if (parserPeekTokenIs(parser, TOKEN_SEMICOLON))
    {
        parserNextToken(parser);
    }

    return (Statement_t*)stmt;
}


static Statement_t* parserParseExpressionStatement(Parser_t* parser) {
    ExpressionStatement_t* stmt = createExpressionStatement(parser->curToken);

    stmt->expression = parserParseExpression(parser, PREC_LOWEST);

    // optional semicolon at the end of sequence
    if (parserPeekTokenIs(parser, TOKEN_SEMICOLON)) {
        parserNextToken(parser);
    }

    return (Statement_t*)stmt;
}


static BlockStatement_t* parserParseBlockStatement(Parser_t* parser) {
    BlockStatement_t* block = createBlockStatement(parser->curToken);
    parserNextToken(parser);

    while (!parserCurTokenIs(parser, TOKEN_RBRACE) && !parserCurTokenIs(parser, TOKEN_EOF)) {
        Statement_t* stmt = parserParseStatement(parser);
        if (stmt != NULL) {
            blockStatementAppendStatement(block, stmt);
        }
        parserNextToken(parser);
    }

    return block;
}


static Expression_t* parserParseExpression(Parser_t* parser, PrecValue_t precedence) { 
    PrefixParseFn_t prefix = parser->prefixParseFns[parser->curToken->type];
    
    if( prefix == NULL) {
        parserNoPrefixParseFnError(parser, parser->curToken->type);
        return NULL;
    }

    Expression_t* leftExp = prefix(parser);

    while (!parserPeekTokenIs(parser, TOKEN_SEMICOLON) && precedence < parserPeekPrecedence(parser)) {
        InfixParseFn_t infix = parser->infixParserFns[parser->peekToken->type];
        if (infix == NULL) {
            return leftExp;
        }

        parserNextToken(parser);

        leftExp = infix(parser, leftExp);
    }

    return leftExp;
}

static Expression_t* parserParseIdentifier(Parser_t* parser) {
    return (Expression_t*)createIdentifier(parser->curToken, parser->curToken->literal);
}

static Expression_t* parserParseIntegerLiteral(Parser_t* parser) {
    IntegerLiteral_t* lit = createIntegerLiteral(parser->curToken);

    if (!strToInteger(lit->token->literal, &lit->value)) {
        char* err = strFormat("Could not parser %s as integer", lit->token->literal);
        parserAppendError(parser, err);
        cleanupIntegerLiteral(&lit);
        return NULL;
    }

    return (Expression_t*)lit;
}

static Expression_t* parserParseStringLiteral(Parser_t* parser) {
    StringLiteral_t* lit = (StringLiteral_t*)createStringLiteral(parser->curToken);
    lit->value = cloneString(parser->curToken->literal);
    return (Expression_t*)lit;
}

static Expression_t* parserParsePrefixExpression(Parser_t* parser) {
    PrefixExpression_t* expression = createPrefixExpresion(parser->curToken);
    expression->operator = cloneString(parser->curToken->literal);

    parserNextToken(parser);
    expression->right = parserParseExpression(parser, PREC_PREFIX);

    return (Expression_t*)expression;
}

static Expression_t* parserParseInfixExpression(Parser_t* parser, Expression_t* left) {
    InfixExpression_t* expression = createInfixExpresion(parser->curToken);
    expression->left = left;
    expression->operator = cloneString(parser->curToken->literal);
    
    PrecValue_t precedence = parserCurPrecedence(parser);
    parserNextToken(parser);
    expression->right = parserParseExpression(parser, precedence);

    return (Expression_t*)expression;
}

static Expression_t* parserParseBoolean(Parser_t* parser) {
    BooleanLiteral_t* bl = createBooleanLiteral(parser->curToken);
    bl->value = parserCurTokenIs(parser, TOKEN_TRUE);
    return (Expression_t*)bl;
}

static Expression_t* parserParseGroupedExpression(Parser_t* parser) {
    parserNextToken(parser);
    
    Expression_t* exp = parserParseExpression(parser, PREC_LOWEST);

    if (!parserExpectPeek(parser, TOKEN_RPAREN)) {
        cleanupExpression(&exp);
        return NULL;
    }

    return exp;
}

static Expression_t* parserParseIfExpression(Parser_t* parser) {
    IfExpression_t* expression = createIfExpresion(parser->curToken);

    if (!parserExpectPeek(parser, TOKEN_LPAREN)) {
        cleanupIfExpression(&expression);
        return NULL;
    }
    
    parserNextToken(parser);
    expression->condition = parserParseExpression(parser, PREC_LOWEST);

    if (!parserExpectPeek(parser, TOKEN_RPAREN)) {
        cleanupIfExpression(&expression);
        return NULL;
    }

    if (!parserExpectPeek(parser, TOKEN_LBRACE)) {
        cleanupIfExpression(&expression);
        return NULL;
    }
    
    expression->consequence = parserParseBlockStatement(parser);

    if (parserPeekTokenIs(parser, TOKEN_ELSE)) {
        parserNextToken(parser);
        
        if (!parserExpectPeek(parser, TOKEN_LBRACE)) {
            cleanupIfExpression(&expression);
            return NULL;
        }

        expression->alternative = parserParseBlockStatement(parser);
    }

    return (Expression_t*)expression;

}

static Expression_t* parserParseFunctionLiteral(Parser_t* parser) {
    FunctionLiteral_t* expression = createFunctionLiteral(parser->curToken);

    if (!parserExpectPeek(parser, TOKEN_LPAREN)) {
        cleanupFunctionLiteral(&expression);
        return NULL;
    }

    parserParseFunctionParameters(parser, expression);

    if (!parserExpectPeek(parser, TOKEN_LBRACE)) {
        cleanupFunctionLiteral(&expression);
        return NULL;
    }

    expression->body = parserParseBlockStatement(parser);

    return (Expression_t*)expression;
}

static void parserParseFunctionParameters(Parser_t* parser, FunctionLiteral_t* fl) {
    Identifier_t* ident = NULL;

    if (parserPeekTokenIs(parser, TOKEN_RPAREN)) {
        parserNextToken(parser);
        return;
    }

    parserNextToken(parser);

    ident = createIdentifier(parser->curToken, parser->curToken->literal);
    functionLiteralAppendParameter(fl, ident);

    while (parserPeekTokenIs(parser, TOKEN_COMMA)) {
        parserNextToken(parser);
        parserNextToken(parser);
        ident = createIdentifier(parser->curToken, parser->curToken->literal);
        functionLiteralAppendParameter(fl, ident);
    }

    if (!parserExpectPeek(parser, TOKEN_RPAREN)) {
        return ;
    }
}


static Expression_t* parserParseCallExpression(Parser_t* parser, Expression_t* function) {
    CallExpression_t* callExpr = createCallExpression(parser->curToken);
    callExpr->function = function;
    callExpr->arguments = parserParseExpressionList(parser, TOKEN_RPAREN);
    if (!callExpr->arguments) {
        cleanupExpression((Expression_t**)&callExpr);
        return NULL;
    }
    return (Expression_t*)callExpr;
}

static Expression_t* parserParseArrayLiteral(Parser_t* parser) {
    ArrayLiteral_t* arrayExpr = createArrayLiteral(parser->curToken);
    arrayExpr->elements = parserParseExpressionList(parser, TOKEN_RBRACKET);
    if (!arrayExpr->elements) {
        cleanupExpression((Expression_t**)&arrayExpr);
        return NULL;
    }
    return (Expression_t*)arrayExpr;
}

static Vector_t* parserParseExpressionList(Parser_t* parser, TokenType_t end) {
    Vector_t* list = createVector();
    if (parserPeekTokenIs(parser, end)){
        parserNextToken(parser);
        return list;
    }
    parserNextToken(parser);
  
    Expression_t* expr = parserParseExpression(parser, PREC_LOWEST);
    vectorAppend(list, (void*)expr);

    while (parserPeekTokenIs(parser, TOKEN_COMMA))
    {
        parserNextToken(parser);
        parserNextToken(parser);    
        expr = parserParseExpression(parser, PREC_LOWEST);
        vectorAppend(list, (void*)expr);
    }

    if (!parserExpectPeek(parser, end)) {
        cleanupVector(&list, (VectorElemCleanupFn_t)cleanupExpression);
        char* err = strFormat("Expression list missing terminator %s", tokenTypeToStr(end));
        parserAppendError(parser, err);
        return NULL;
    }

    return list;
}

static Expression_t* parserParseIndexExpression(Parser_t*parser, Expression_t* left) {
    IndexExpression_t* exprIndex = createIndexExpression(parser->curToken);
    parserNextToken(parser);

    exprIndex->left = left;
    exprIndex->right = parserParseExpression(parser, PREC_LOWEST);

    if (!parserExpectPeek(parser, TOKEN_RBRACKET)) {
        cleanupIndexExpression(&exprIndex);
        return NULL;
    }
    
    return (Expression_t*)exprIndex;
}

static Expression_t* parserParseHashLiteral(Parser_t* parser) {
    HashLiteral_t* hash = createHashLiteral(parser->curToken);

    while(!parserPeekTokenIs(parser, TOKEN_RBRACE)) {
        parserNextToken(parser);

        Expression_t* key = parserParseExpression(parser, PREC_LOWEST);
        if (!parserExpectPeek(parser, TOKEN_COLON)){
            cleanupExpression(&key);
            cleanupHashLiteral(&hash);
            return NULL;
        }
        parserNextToken(parser);
        Expression_t* value = parserParseExpression(parser, PREC_LOWEST);

        if(!parserPeekTokenIs(parser, TOKEN_RBRACE)&& !parserExpectPeek(parser, TOKEN_COMMA)) {
            cleanupExpression(&key);
            cleanupExpression(&value);
            cleanupHashLiteral(&hash);
            return NULL;
        }

        hashLiteralSetPair(hash, key, value);
    }

    if (!parserExpectPeek(parser, TOKEN_RBRACE)) {
        cleanupHashLiteral(&hash);
        return NULL;
    }

    return (Expression_t*)hash;
}

static PrecValue_t parserPeekPrecedence(Parser_t* parser) {
    return _precedences[parser->peekToken->type];
}

static PrecValue_t parserCurPrecedence(Parser_t* parser) {
    return _precedences[parser->curToken->type];
}


static void parserRegisterPrefix(Parser_t* parser, TokenType_t tokType, PrefixParseFn_t fn) {
    parser->prefixParseFns[tokType] = fn;
}
static void parserRegisterInfix(Parser_t* parser, TokenType_t tokType, InfixParseFn_t fn) {
    parser->infixParserFns[tokType] = fn;
}


/* Token manipulation functions */

static void parserNextToken(Parser_t* parser) {
    if (parser->curToken)
        cleanupToken(&parser->curToken);
    parser->curToken = parser->peekToken;
    parser->peekToken = lexerNextToken(parser->lexer);
}

static bool parserCurTokenIs(Parser_t* parser, TokenType_t tokType) {
    return parser->curToken->type == tokType;
}

static bool parserPeekTokenIs(Parser_t* parser, TokenType_t tokType) {
    return parser->peekToken->type == tokType;
}

static bool parserExpectPeek(Parser_t* parser, TokenType_t expTokType) {
    if( parser->peekToken->type == expTokType) {
        parserNextToken(parser);
        return true;
    } else {
        parserPeekError(parser, expTokType);
        return false;
    }
    
}


/* ERROR handling functions */

const char** parserGetErrors(Parser_t* parser) {
    return (const char**) vectorGetBuffer(parser->errors);
}

uint32_t parserGetErrorCount(Parser_t* parser) {
    return vectorGetCount(parser->errors);
}


static void parserNoPrefixParseFnError(Parser_t* parser, TokenType_t tokType) {
    char* msg = strFormat("No prefix parser function for %s found", tokenTypeToStr(tokType));
    parserAppendError(parser, msg);
}

static void parserAppendError(Parser_t* parser, char* err) {
    vectorAppend(parser->errors, (void*) err);
}

static void parserPeekError(Parser_t* parser, TokenType_t expTokenType) {
    char* strBuffer = strFormat("Expected next token to be %s, got %s instead", 
                                tokenTypeToStr(expTokenType),   
                                tokenTypeToStr(parser->peekToken->type));
    parserAppendError(parser, strBuffer);
}
