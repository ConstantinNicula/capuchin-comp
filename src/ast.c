#include <malloc.h>

#include "ast.h"
#include "token.h"
#include "utils.h"
#include "sbuf.h"

/************************************ 
 *       COMMON UTILS DEF           *
 ************************************/

static void cleanupStatementVec(Vector_t** statements);
static char* statementVecToString(Vector_t* statements, bool indent);


/************************************ 
 *         EXPRESSION NODE          *
 ************************************/

static ExpressionCleanupFn_t expressionCleanupFns[] = {
    [EXPRESSION_IDENTIFIER]=(ExpressionCleanupFn_t)cleanupIdentifier,
    [EXPRESSION_INTEGER_LITERAL]=(ExpressionCleanupFn_t)cleanupIntegerLiteral,
    [EXPRESSION_STRING_LITERAL]=(ExpressionCleanupFn_t)cleanupStringLiteral,
    [EXPRESSION_BOOLEAN_LITERAL]=(ExpressionCleanupFn_t)cleanupBooleanLiteral,
    [EXPRESSION_ARRAY_LITERAL]=(ExpressionCleanupFn_t)cleanupArrayLiteral,
    [EXPRESSION_INDEX_EXPRESSION]=(ExpressionCleanupFn_t)cleanupIndexExpression,
    [EXPRESSION_PREFIX_EXPRESSION]=(ExpressionCleanupFn_t)cleanupPrefixExpression,
    [EXPRESSION_INFIX_EXPRESSION]=(ExpressionCleanupFn_t)cleanupInfixExpression,
    [EXPRESSION_IF_EXPRESSION]=(ExpressionCleanupFn_t)cleanupIfExpression,
    [EXPRESSION_FUNCTION_LITERAL]=(ExpressionCleanupFn_t)cleanupFunctionLiteral,
    [EXPRESSION_CALL_EXPRESSION]=(ExpressionCleanupFn_t)cleanupCallExpression,
    [EXPRESSION_HASH_LITERAL]=(ExpressionCleanupFn_t)cleanupHashLiteral,
    [EXPRESSION_INVALID]=NULL
};

static ExpressionCopyFn_t expressionCopyFns[] = {
    [EXPRESSION_IDENTIFIER]=(ExpressionCopyFn_t)copyIdentifier,
    [EXPRESSION_INTEGER_LITERAL]=(ExpressionCopyFn_t)copyIntegerLiteral,
    [EXPRESSION_STRING_LITERAL]=(ExpressionCopyFn_t)copyStringLiteral,
    [EXPRESSION_BOOLEAN_LITERAL]=(ExpressionCopyFn_t)copyBooleanLiteral,
    [EXPRESSION_ARRAY_LITERAL]=(ExpressionCopyFn_t)copyArrayLiteral,
    [EXPRESSION_INDEX_EXPRESSION]=(ExpressionCopyFn_t)copyIndexExpression,
    [EXPRESSION_PREFIX_EXPRESSION]=(ExpressionCopyFn_t)copyPrefixExpression,
    [EXPRESSION_INFIX_EXPRESSION]=(ExpressionCopyFn_t)copyInfixExpression,
    [EXPRESSION_IF_EXPRESSION]=(ExpressionCopyFn_t)copyIfExpression,
    [EXPRESSION_FUNCTION_LITERAL]=(ExpressionCopyFn_t)copyFunctionLiteral,
    [EXPRESSION_CALL_EXPRESSION]=(ExpressionCopyFn_t)copyCallExpression,
    [EXPRESSION_HASH_LITERAL]=(ExpressionCopyFn_t)copyHashLiteral,
    [EXPRESSION_INVALID]=NULL
};


static ExpressionToStringFn_t expressionToStringFns[] = {
    [EXPRESSION_IDENTIFIER]=(ExpressionToStringFn_t)identifierToString,
    [EXPRESSION_INTEGER_LITERAL]=(ExpressionToStringFn_t)integerLiteralToString,
    [EXPRESSION_BOOLEAN_LITERAL]=(ExpressionToStringFn_t)booleanLiteralToString,
    [EXPRESSION_STRING_LITERAL]=(ExpressionToStringFn_t)stringLiteralToString,
    [EXPRESSION_ARRAY_LITERAL]=(ExpressionToStringFn_t)arrayLiteralToString,
    [EXPRESSION_INDEX_EXPRESSION]=(ExpressionToStringFn_t)indexExpressionToString,
    [EXPRESSION_PREFIX_EXPRESSION]=(ExpressionToStringFn_t)prefixExpressionToString,
    [EXPRESSION_INFIX_EXPRESSION]=(ExpressionToStringFn_t)infixExpressionToString,
    [EXPRESSION_IF_EXPRESSION]=(ExpressionToStringFn_t)ifExpressionToString,
    [EXPRESSION_FUNCTION_LITERAL]=(ExpressionToStringFn_t)functionLiteralToString,
    [EXPRESSION_CALL_EXPRESSION]=(ExpressionToStringFn_t)callExpressionToString,
    [EXPRESSION_HASH_LITERAL]=(ExpressionToStringFn_t)hashLiteralToString,
    [EXPRESSION_INVALID]=NULL
};


void cleanupExpression(Expression_t** expr) { 
    if (*expr == NULL)
        return;
    
    if ((*expr)->type  >= 0  && (*expr)->type < EXPRESSION_INVALID) {
        ExpressionCleanupFn_t cleanupFn = expressionCleanupFns[(*expr)->type];
        cleanupFn((void**)expr);
    }
}

Expression_t* copyExpression(Expression_t* expr) {
    if (expr && expr->type  >= 0 && expr->type < EXPRESSION_INVALID) {
        ExpressionCopyFn_t copyFn = expressionCopyFns[expr->type];
        return copyFn((void*)expr);
    }
    return NULL;
}

char* expressionToString(Expression_t* expr) {
    if (expr && expr->type  >= 0 && expr->type < EXPRESSION_INVALID) {
        ExpressionToStringFn_t toStringFn = expressionToStringFns[expr->type];
        return toStringFn(expr);
    }
    return cloneString("");
}

const char* expressionTokenLiteral(Expression_t* expr) {
    if (expr && expr->type  >= 0 && expr->type < EXPRESSION_INVALID) {
        return expr->token->literal; 
    }
    return "";
}


/************************************ 
 *           IDENTIFIER             *
 ************************************/

Identifier_t* createIdentifier(const Token_t* tok, const char* val) {
    Identifier_t* ident = mallocChk(sizeof(Identifier_t));

    *ident =  (Identifier_t) {
        .type = EXPRESSION_IDENTIFIER,
        .token = copyToken(tok),
        .value = cloneString(val)
    };

    return ident;
}

Identifier_t* copyIdentifier(const Identifier_t* ident){
    return createIdentifier(ident->token, ident->value);
}

void cleanupIdentifier(Identifier_t** ident) {
    if (!(*ident))
        return;

    cleanupToken(&(*ident)->token);
    free((void*)(*ident)->value);
    
    free(*ident);
    *ident = NULL;
}

char* identifierToString(const Identifier_t* ident) {
    // Return a copy to avoid free errors
    return cloneString(ident->value);
}


/************************************ 
 *      INTEGER LITERAL             *
 ************************************/

IntegerLiteral_t* createIntegerLiteral(const Token_t* tok) {
    IntegerLiteral_t* il = mallocChk(sizeof(IntegerLiteral_t));

    *il = (IntegerLiteral_t) {
        .type = EXPRESSION_INTEGER_LITERAL,
        .token = copyToken(tok),
        .value = 0
    };
    return il;
}

IntegerLiteral_t* copyIntegerLiteral(const IntegerLiteral_t* il){
    IntegerLiteral_t* newIl = createIntegerLiteral(il->token);
    newIl->value = il->value;
    return newIl;
}

void cleanupIntegerLiteral(IntegerLiteral_t** il) {
    if (!(*il)) return;
    
    cleanupToken(&(*il)->token);
    free(*il);
    *il = NULL;
}

char* integerLiteralToString(const IntegerLiteral_t* il) {
    return cloneString(il->token->literal);
}


/************************************ 
 *          BOOLEAN                 *
 ************************************/

BooleanLiteral_t* createBooleanLiteral(const Token_t* tok) {
    BooleanLiteral_t* bl = mallocChk(sizeof(BooleanLiteral_t));

    *bl = (BooleanLiteral_t) {
        .type = EXPRESSION_BOOLEAN_LITERAL,
        .token = copyToken(tok),
        .value = false
    };

    return bl;
}

BooleanLiteral_t* copyBooleanLiteral(const BooleanLiteral_t* bl) {
    BooleanLiteral_t* newBl = createBooleanLiteral(bl->token);
    newBl->value = bl->value;
    return newBl;
}

void cleanupBooleanLiteral(BooleanLiteral_t** bl) {
    if (!(*bl)) return;
    
    cleanupToken(&(*bl)->token);

    free(*bl);
    *bl = NULL;
}

char* booleanLiteralToString(const BooleanLiteral_t* bl) {
    return bl->value ? cloneString("true") : cloneString("false");
}

/************************************ 
 *        STRING LITERAL            *
 ************************************/

StringLiteral_t* createStringLiteral(const Token_t* tok) {
    StringLiteral_t* sl = mallocChk(sizeof(StringLiteral_t));
    *sl = (StringLiteral_t) {
        .type = EXPRESSION_STRING_LITERAL, 
        .token = copyToken(tok),
        .value = NULL
    };
    return sl;
}

StringLiteral_t* copyStringLiteral(const StringLiteral_t* sl) {
    StringLiteral_t* newSl = createStringLiteral(sl->token);
    newSl->value = cloneString(sl->value);
    return newSl;
}

void cleanupStringLiteral(StringLiteral_t** sl) {
    if(!(*sl)) return;

    cleanupToken(&(*sl)->token); 
    if((*sl)->value) 
        free((*sl)->value);
    
    free((*sl));
    *sl = NULL;
}

char* stringLiteralToString(const StringLiteral_t* sl) {
    return cloneString(sl->value);    
}


/************************************ 
 *        ARRAY LITERAL             *
 ************************************/

ArrayLiteral_t* createArrayLiteral(const Token_t* tok) {
    ArrayLiteral_t* al = mallocChk(sizeof(ArrayLiteral_t));
    *al = (ArrayLiteral_t) {
        .type = EXPRESSION_ARRAY_LITERAL,
        .token = copyToken(tok),
        .elements = NULL
    };
    return al;
}

ArrayLiteral_t* copyArrayLiteral(const ArrayLiteral_t* al) {
    ArrayLiteral_t* newAl = createArrayLiteral(al->token);
    newAl->elements = copyVector(al->elements, (VectorElemCopyFn_t)copyExpression);
    return newAl;
}

void cleanupArrayLiteral(ArrayLiteral_t** al) {
    if (!(*al)) return;

    cleanupToken(&(*al)->token);
    cleanupVector(&(*al)->elements, (VectorElemCleanupFn_t)cleanupExpression);
    free(*al);
    *al = NULL;
}

char* arrayLiteralToString(const ArrayLiteral_t* al) {
    Strbuf_t* sbuf = createStrbuf();
    strbufWrite(sbuf, "[");
    
    uint32_t cnt = arrayLiteralGetElementCount(al);
    Expression_t** elems = arrayLiteralGetElements(al);
    for (uint32_t i = 0; i < cnt; i++) {
        strbufConsume(sbuf, expressionToString(elems[i]));
        if (i != (cnt-1)) {
            strbufWrite(sbuf, ", ");
        }
    }
    strbufWrite(sbuf, "]");
    
    return detachStrbuf(&sbuf);
}

uint32_t arrayLiteralGetElementCount(const ArrayLiteral_t* al) {
    return vectorGetCount(al->elements);
}

Expression_t** arrayLiteralGetElements(const ArrayLiteral_t* al) {
    return (Expression_t**) vectorGetBuffer(al->elements);
}

/************************************
 *         HASH LITERAL             *
 ************************************/

HashLiteral_t *createHashLiteral(const Token_t *tok) {
    HashLiteral_t* hash = mallocChk(sizeof(HashLiteral_t));
    *hash = (HashLiteral_t) {
        .type = EXPRESSION_HASH_LITERAL,
        .token = copyToken(tok),
        .keys = createVector(),
        .values = createVector()
    };
    return hash;
}

HashLiteral_t *copyHashLiteral(const HashLiteral_t *hl) {
    HashLiteral_t* newHash = mallocChk(sizeof(HashLiteral_t));
    *newHash = (HashLiteral_t) {
        .type = EXPRESSION_HASH_LITERAL,
        .token = copyToken(hl->token),
        .keys = copyVector(hl->keys, (VectorElemCopyFn_t)copyExpression),
        .values = copyVector(hl->values, (VectorElemCopyFn_t)copyExpression),
    };
    return newHash;
}

void cleanupHashLiteral(HashLiteral_t **hl) {
    if(!(*hl)) return;
    cleanupToken(&(*hl)->token);
    cleanupVector(&(*hl)->keys, (VectorElemCleanupFn_t)cleanupExpression);
    cleanupVector(&(*hl)->values, (VectorElemCleanupFn_t)cleanupExpression);
    free(*hl);
    *hl = NULL;
}

char *hashLiteralToString(const HashLiteral_t *al) {
    Strbuf_t* sbuf = createStrbuf();

    strbufWrite(sbuf, "{");
    uint32_t cnt = hashLiteralGetPairsCount(al);
    for (uint32_t i = 0; i < cnt; i++) {
        Expression_t *key, *value;
        hashLiteralGetPair(al, i, &key, &value);

        strbufConsume(sbuf, expressionToString(key));
        strbufWrite(sbuf, ":");
        strbufConsume(sbuf, expressionToString(value));
        
        if (i != (cnt - 1)) {
            strbufWrite(sbuf, ", ");
        }
    }
    strbufWrite(sbuf, "}");
    return detachStrbuf(&sbuf);
}

uint32_t hashLiteralGetPairsCount(const HashLiteral_t *hl) {
    return vectorGetCount(hl->keys);
}

void hashLiteralGetPair(const HashLiteral_t*hl, uint32_t idx,  Expression_t** key, Expression_t** value) {
    if (idx < 0 || idx >= hashLiteralGetPairsCount(hl)) {
        *key = *value = NULL;
        return;
    }

    *key = ((Expression_t**)vectorGetBuffer(hl->keys))[idx];
    *value = ((Expression_t**)vectorGetBuffer(hl->values))[idx];
}

void hashLiteralSetPair(HashLiteral_t* hl, Expression_t* key, Expression_t* value) {
    vectorAppend(hl->keys, key);
    vectorAppend(hl->values, value);
}



/************************************ 
 *       INDEX EXPRESSION           *
 ************************************/

IndexExpression_t* createIndexExpression(const Token_t* tok) {
    IndexExpression_t* expr = mallocChk(sizeof(IndexExpression_t));
    *expr = (IndexExpression_t) {
        .type = EXPRESSION_INDEX_EXPRESSION,
        .token = copyToken(tok),
        .left = NULL, 
        .right = NULL,
    };
    return expr; 
}

IndexExpression_t* copyIndexExpression(const IndexExpression_t* al) {
    IndexExpression_t* newExpr = createIndexExpression(al->token);
    newExpr->left = copyExpression(al->left);
    newExpr->right = copyExpression(al->right);
    return newExpr; 
}

void cleanupIndexExpression(IndexExpression_t** al) {
    if(!(*al)) return;

    cleanupToken(&(*al)->token);
    cleanupExpression(&(*al)->left);
    cleanupExpression(&(*al)->right);
    free(*al);
    *al = NULL;
}


char* indexExpressionToString(const IndexExpression_t* al) {
    Strbuf_t* sbuf = createStrbuf();
    strbufWrite(sbuf, "(");
    strbufConsume(sbuf, expressionToString(al->left));
    strbufWrite(sbuf, "[");
    strbufConsume(sbuf, expressionToString(al->right));
    strbufWrite(sbuf, "])");

    return detachStrbuf(&sbuf);
}


/************************************ 
 *      PREFIX EXPRESSION           *
 ************************************/

PrefixExpression_t* createPrefixExpresion(const Token_t* tok) {
    PrefixExpression_t* exp = mallocChk(sizeof(PrefixExpression_t));

    *exp = (PrefixExpression_t) {
        .type = EXPRESSION_PREFIX_EXPRESSION,
        .token = copyToken(tok),
        .operator = NULL,
        .right = NULL
    };

    return exp;
}

PrefixExpression_t* copyPrefixExpression(const PrefixExpression_t* exp) {
    PrefixExpression_t* newExp = createPrefixExpresion(exp->token);
    newExp->operator = cloneString(exp->operator);
    newExp->right = copyExpression(exp->right);
    return newExp;
}

void cleanupPrefixExpression(PrefixExpression_t** exp) {
    if (!(*exp)) return;

    cleanupToken(&(*exp)->token);
    free((*exp)->operator);
    cleanupExpression(&(*exp)->right);
    
    free(*exp);
    *exp = NULL;
}

char* prefixExpressionToString(const PrefixExpression_t* exp) {
    Strbuf_t* sbuf = createStrbuf();

    strbufWrite(sbuf, "(");
    strbufWrite(sbuf, exp->operator);
    strbufConsume(sbuf, expressionToString(exp->right));
    strbufWrite(sbuf, ")");

    return detachStrbuf(&sbuf);
}



/************************************ 
 *      INFIX EXPRESSION           *
 ************************************/

InfixExpression_t* createInfixExpresion(const Token_t* tok) {
    InfixExpression_t* exp = mallocChk(sizeof(InfixExpression_t));

    *exp = (InfixExpression_t) {
        .type = EXPRESSION_INFIX_EXPRESSION,
        .token = copyToken(tok),
        .left = NULL, 
        .operator = NULL, 
        .right = NULL
    };

    return exp;
}

InfixExpression_t* copyInfixExpression(const InfixExpression_t* exp) {
    InfixExpression_t* newExp = createInfixExpresion(exp->token);
    newExp->left = copyExpression(exp->left);
    newExp->operator = cloneString(exp->operator);
    newExp->right = copyExpression(exp->right);
    return newExp;
}

void cleanupInfixExpression(InfixExpression_t** exp) {
    if (!(*exp))return;
    
    cleanupToken(&(*exp)->token);
    cleanupExpression(&(*exp)->left);
    free((*exp)->operator);
    cleanupExpression(&(*exp)->right);

    free(*exp);
    *exp = NULL;
}

char* infixExpressionToString(const InfixExpression_t* exp) {
    Strbuf_t* sbuf = createStrbuf();

    strbufWrite(sbuf, "(");
    strbufConsume(sbuf, expressionToString(exp->left));
    strbufWrite(sbuf, " ");
    strbufWrite(sbuf, exp->operator);
    strbufWrite(sbuf, " ");
    strbufConsume(sbuf, expressionToString(exp->right));
    strbufWrite(sbuf, ")");

    return detachStrbuf(&sbuf);
}


/************************************ 
 *          IF EXPRESSION           *
 ************************************/

IfExpression_t* createIfExpresion(const Token_t* tok) {
    IfExpression_t* exp = mallocChk(sizeof(IfExpression_t));

    *exp = (IfExpression_t) {
        .type = EXPRESSION_IF_EXPRESSION, 
        .token = copyToken(tok),
        .condition = NULL, 
        .consequence = NULL, 
        .alternative = NULL
    };

    return exp;
}

IfExpression_t* copyIfExpression(const IfExpression_t* exp) {
    IfExpression_t* newExp = createIfExpresion(exp->token);
    newExp->condition = copyExpression(exp->condition);
    newExp->consequence = copyBlockStatement(exp->consequence);
    newExp->alternative = exp->alternative ? copyBlockStatement(exp->alternative) : NULL;
    return newExp;
}

void cleanupIfExpression(IfExpression_t** exp) {
    if (!(*exp)) return;

    cleanupToken(&(*exp)->token);
    cleanupExpression(&(*exp)->condition);
    cleanupBlockStatement(&(*exp)->consequence);
    cleanupBlockStatement(&(*exp)->alternative);

    free(*exp);
    *exp = NULL;
}

char* ifExpressionToString(const IfExpression_t* exp) {
    Strbuf_t* sbuf = createStrbuf();

    strbufWrite(sbuf, "if");
    strbufConsume(sbuf, expressionToString(exp->condition));
    strbufWrite(sbuf, " ");
    strbufConsume(sbuf, blockStatementToString(exp->consequence));

    if (exp->alternative) {
        strbufWrite(sbuf, "else ");
        strbufConsume(sbuf, blockStatementToString(exp->alternative));
    }

    return detachStrbuf(&sbuf);
}



/************************************ 
 *    FUNCTION EXPRESSION           *
 ************************************/

FunctionLiteral_t* createFunctionLiteral(const Token_t* tok) {
    FunctionLiteral_t *exp = mallocChk(sizeof(FunctionLiteral_t));

    *exp = (FunctionLiteral_t) {
        .type = EXPRESSION_FUNCTION_LITERAL, 
        .token = copyToken(tok),
        .parameters = createVector(),
        .body = NULL
    };

    return exp;
}

FunctionLiteral_t* copyFunctionLiteral(const FunctionLiteral_t* exp) {
    FunctionLiteral_t* newExp = mallocChk(sizeof(FunctionLiteral_t));
    *newExp = (FunctionLiteral_t) {
        .type = EXPRESSION_FUNCTION_LITERAL, 
        .token = copyToken(exp->token),
        .parameters = copyVector(exp->parameters, (VectorElemCopyFn_t)copyExpression),
        .body = copyBlockStatement(exp->body)
    };

    return newExp;
}

void cleanupFunctionLiteral(FunctionLiteral_t** exp) {
    if (!(*exp)) return;

    cleanupToken(&(*exp)->token);
    cleanupVector(&(*exp)->parameters, (VectorElemCleanupFn_t)cleanupIdentifier);
    cleanupBlockStatement(&(*exp)->body);

    free(*exp);
    *exp = NULL;
}

char* functionLiteralToString(const FunctionLiteral_t* exp) {
    Strbuf_t* sbuf = createStrbuf();

    strbufWrite(sbuf, exp->token->literal);
    strbufWrite(sbuf, "(");
    uint32_t paramCnt = functionLiteralGetParameterCount(exp);
    Identifier_t** params = functionLiteralGetParameters(exp);
    for (uint32_t i = 0; i < paramCnt; i++) {
        strbufConsume(sbuf, identifierToString(params[i]));
        if (i != (paramCnt-1)) {
            strbufWrite(sbuf, ", ");
        }
    }
    strbufWrite(sbuf, ")");
    strbufConsume(sbuf, blockStatementToString(exp->body));

    return detachStrbuf(&sbuf);
}

void functionLiteralAppendParameter(FunctionLiteral_t* exp, const Identifier_t* param) {
    vectorAppend(exp->parameters, (void*)param);
}

uint32_t functionLiteralGetParameterCount(const FunctionLiteral_t* exp) {
    return vectorGetCount(exp->parameters);
}

Identifier_t** functionLiteralGetParameters(const FunctionLiteral_t* exp) {
    return (Identifier_t**) vectorGetBuffer(exp->parameters);
}


/************************************ 
 *        CALL EXPRESSION           *
 ************************************/

CallExpression_t* createCallExpression(const Token_t* tok) {
    CallExpression_t* exp = mallocChk(sizeof(CallExpression_t));

    *exp = (CallExpression_t) {
        .type = EXPRESSION_CALL_EXPRESSION, 
        .token =  copyToken(tok),
        .function = NULL, 
        .arguments = NULL
    };
 
    return exp;
}

CallExpression_t* copyCallExpression(const CallExpression_t* exp) {
    CallExpression_t* newExp = mallocChk(sizeof(CallExpression_t));
    *newExp = (CallExpression_t) {
        .type = EXPRESSION_CALL_EXPRESSION, 
        .token =  copyToken(exp->token),
        .function = copyExpression(exp->function),
        .arguments = copyVector(exp->arguments, (VectorElemCopyFn_t)copyExpression)
    };

    return newExp;
}

void cleanupCallExpression(CallExpression_t** exp) {
    if (!(*exp)) return;
    
    cleanupToken(&(*exp)->token);
    cleanupExpression(&(*exp)->function);
    cleanupVector(&(*exp)->arguments,(VectorElemCleanupFn_t) cleanupExpression);

    free(*exp);
    *exp = NULL;
}

char* callExpressionToString(const CallExpression_t* exp) {
    Strbuf_t* sbuf = createStrbuf();

    strbufConsume(sbuf, expressionToString(exp->function));
    strbufWrite(sbuf, "(");
    
    uint32_t argCnt = callExpresionGetArgumentCount(exp);
    Expression_t** args = callExpressionGetArguments(exp);

    for (uint32_t i = 0; i < argCnt; i++) {
        strbufConsume(sbuf, expressionToString(args[i]));
        if (i != argCnt - 1) {
            strbufWrite(sbuf, ", ");
        }
    }

    strbufWrite(sbuf, ")");
    return detachStrbuf(&sbuf);
}


void callExpressionAppendArgument(CallExpression_t* exp, const Expression_t* arg) {
    vectorAppend(exp->arguments, (void*) arg);
}
uint32_t callExpresionGetArgumentCount(const CallExpression_t* exp) {
    return vectorGetCount(exp->arguments);
}

Expression_t** callExpressionGetArguments(const CallExpression_t* exp) {
    return (Expression_t**) vectorGetBuffer(exp->arguments);
}





/************************************ 
 *         GENERIC STATEMENT        *
 ************************************/

static StatementCleanupFn_t statementCleanupFns[] = {
    [STATEMENT_LET]=(StatementCleanupFn_t)cleanupLetStatement,
    [STATEMENT_RETURN]=(StatementCleanupFn_t)cleanupReturnStatement,
    [STATEMENT_EXPRESSION]=(StatementCleanupFn_t)cleanupExpressionStatement,
    [STATEMENT_BLOCK]=(StatementCleanupFn_t)cleanupBlockStatement,
    [STATEMENT_INVALID]=NULL
};

static StatementCopyFn_t statementCopyFns[] = {
    [STATEMENT_LET]=(StatementCopyFn_t)copyLetStatement,
    [STATEMENT_RETURN]=(StatementCopyFn_t)copyReturnStatement,
    [STATEMENT_EXPRESSION]=(StatementCopyFn_t)copyExpressionStatement,
    [STATEMENT_BLOCK]=(StatementCopyFn_t)copyBlockStatement,
    [STATEMENT_INVALID]=NULL
};


static StatementToStringFn_t statementToStringFns[] = {
    [STATEMENT_LET]=(StatementToStringFn_t)letStatementToString,
    [STATEMENT_RETURN]=(StatementToStringFn_t)returnStatementToString,
    [STATEMENT_EXPRESSION]=(StatementToStringFn_t)expressionStatementToString,
    [STATEMENT_BLOCK]=(StatementToStringFn_t)blockStatementToString,
    [STATEMENT_INVALID]=NULL
};



void cleanupStatement(Statement_t** st) {
    if (st && (*st)->type >= 0 && (*st)->type < STATEMENT_INVALID) {
        StatementCleanupFn_t cleanupFn = statementCleanupFns[(*st)->type];
        cleanupFn((void**) st);
    }
}

Statement_t* copyStatement(const Statement_t* st) {
    if (st && (st->type >= 0) && (st->type <= STATEMENT_INVALID) ) {
        StatementCopyFn_t copyFn = statementCopyFns[st->type];
        return copyFn(st);
    }
    return NULL;
}

const char* statementTokenLiteral(const Statement_t* st) {
    if (st && st->type >= 0 && st->type <STATEMENT_INVALID ) {
        return st->token->literal;
    }
    return "";
}

char* statementToString(const Statement_t* st) {
    if (st && (st->type >= 0) && (st->type <=STATEMENT_INVALID) ) {
        StatementToStringFn_t toStringFn = statementToStringFns[st->type];
        return toStringFn(st);
    }
    return cloneString("");
}


/************************************ 
 *         LET STATEMENT            *
 ************************************/

LetStatement_t* createLetStatement(const Token_t* token) {
    LetStatement_t* st = mallocChk(sizeof(LetStatement_t));

    *st = (LetStatement_t) {
        .type = STATEMENT_LET, 
        .token = copyToken(token),
        .name = NULL,
        .value = NULL
    };

    return st;
}

LetStatement_t* copyLetStatement(const LetStatement_t* st) {
    LetStatement_t* newSt = createLetStatement(st->token);
    newSt->name = copyIdentifier(st->name);
    newSt->value = copyExpression(st->value);
    return newSt;
}

void cleanupLetStatement(LetStatement_t** st) {
    if (!(*st)) return;

    cleanupToken(&(*st)->token);
    cleanupIdentifier(&(*st)->name);
    cleanupExpression(&(*st)->value);
    
    free(*st);
    *st = NULL;
}

char* letStatementToString(const LetStatement_t* st) {
    Strbuf_t* sbuf = createStrbuf();
    
    strbufWrite(sbuf, st->token->literal);
    strbufWrite(sbuf, " ");
    strbufConsume(sbuf, identifierToString(st->name));
    strbufWrite(sbuf, " = ");
    if (st->value != NULL) {
        strbufWrite(sbuf, expressionToString(st->value));
    }
    strbufWrite(sbuf, ";");

    return detachStrbuf(&sbuf);
}


/************************************ 
 *      RETURN STATEMENT            *
 ************************************/

ReturnStatement_t* createReturnStatement(const Token_t* token) {
    ReturnStatement_t* st = mallocChk(sizeof(ReturnStatement_t));
    
    *st = (ReturnStatement_t) {
        .type = STATEMENT_RETURN,
        .token = copyToken(token),
        .returnValue = NULL 
    };

    return st;
}

ReturnStatement_t* copyReturnStatement(const ReturnStatement_t* st) {
    ReturnStatement_t* newSt = createReturnStatement(st->token);
    newSt->returnValue = copyExpression(st->returnValue);
    return newSt;
}

void cleanupReturnStatement(ReturnStatement_t** st) {
    if (!(*st)) return;
    
    cleanupToken(&(*st)->token);
    cleanupExpression(&(*st)->returnValue);
    
    free(*st);
    *st = NULL;
}

char* returnStatementToString(const ReturnStatement_t* st) {
    Strbuf_t* sbuf = createStrbuf();

    strbufWrite(sbuf, st->token->literal);
    strbufWrite(sbuf, " ");

    if (st->returnValue != NULL) {
        strbufConsume(sbuf, expressionToString(st->returnValue));
    }
    strbufWrite(sbuf, ";");

    return detachStrbuf(&sbuf);
}

/************************************ 
 *      EXPRESSION STATEMENT        *
 ************************************/

ExpressionStatement_t* createExpressionStatement(const Token_t* token) {
    ExpressionStatement_t* st = mallocChk(sizeof(ExpressionStatement_t));

    *st = (ExpressionStatement_t) {
        .type = STATEMENT_EXPRESSION, 
        .token = copyToken(token),
        .expression = NULL
    };

    return st;
}

ExpressionStatement_t* copyExpressionStatement(const ExpressionStatement_t* st) {
    ExpressionStatement_t* newSt = createExpressionStatement(st->token);
    newSt->expression = copyExpression(st->expression);
    return newSt;
}

void cleanupExpressionStatement(ExpressionStatement_t** st) {
    if (!(*st))return;

    cleanupToken(&(*st)->token);
    cleanupExpression(&(*st)->expression);

    free(*st);
    *st = NULL;
}

char* expressionStatementToString(const ExpressionStatement_t* st) {
    if (st->expression != NULL) {
        return expressionToString(st->expression);
    }
    return cloneString("");
}



/************************************ 
 *         BLOCK STATEMENT          *
 ************************************/

BlockStatement_t* createBlockStatement(const Token_t* token) {
    BlockStatement_t* st = mallocChk(sizeof(BlockStatement_t));

    *st = (BlockStatement_t) {
        .type = STATEMENT_BLOCK, 
        .token = copyToken(token),
        .statements = createVector()
    };

    return st;
}

BlockStatement_t* copyBlockStatement(const BlockStatement_t* st) {
    BlockStatement_t* newSt =  mallocChk(sizeof(BlockStatement_t));
    *newSt = (BlockStatement_t) {
        .type = STATEMENT_BLOCK, 
        .token = copyToken(st->token),
        .statements = copyVector(st->statements, (VectorElemCopyFn_t)copyStatement)
    };

    return newSt;
}

void cleanupBlockStatement(BlockStatement_t** st) {
    if (!(*st)) return;
    
    cleanupToken(&(*st)->token);
    cleanupStatementVec(&(*st)->statements);
    
    free(*st);
    *st = NULL;
}

char* blockStatementToString(const BlockStatement_t* st) {
    return statementVecToString(st->statements, true);
}

uint32_t blockStatementGetStatementCount(const BlockStatement_t* st) {
    return vectorGetCount(st->statements);
}

Statement_t** blockStatementGetStatements(const BlockStatement_t* st) {
    return (Statement_t**) vectorGetBuffer(st->statements);
}

void blockStatementAppendStatement(BlockStatement_t* block, const Statement_t* st) {
    vectorAppend(block->statements, (void*) st);
}


/************************************ 
 *      PROGRAM NODE                *
 ************************************/

Program_t* createProgram() {
    Program_t* prog = (Program_t*) malloc(sizeof(Program_t));
    if (prog == NULL)
        return NULL;
    prog->statements = createVector();
    return prog;
}

Statement_t** programGetStatements(const Program_t* prog) {
    return (Statement_t**)vectorGetBuffer(prog->statements);
}

uint32_t programGetStatementCount(const Program_t* prog) {
    return vectorGetCount(prog->statements);
}

Program_t* copyProgram(const Program_t* prog) {
    Program_t* newProg = createProgram();
    newProg->statements = copyVector(prog->statements, (VectorElemCopyFn_t)copyStatement);
    return newProg;
}

void cleanupProgram(Program_t** prog) {
    if (*prog == NULL) 
        return;

    cleanupStatementVec(&(*prog)->statements);
    
    free(*prog);
    *prog = NULL;
}

void programAppendStatement(Program_t* prog, const Statement_t* st) {
    vectorAppend(prog->statements, (void*) st);
}

const char* programTokenLiteral(const Program_t* prog) {
    if (programGetStatementCount(prog) > 0u) 
    {   
        Statement_t** stmts = programGetStatements(prog);
        return statementTokenLiteral(stmts[0]);
    } else  {
        return "";
    }
}

char* programToString(const Program_t* prog) {
    return statementVecToString(prog->statements, false);
}


/************************************ 
 *         COMMON UTILS              *
 ************************************/

static void cleanupStatementVec(Vector_t** statements) {
    cleanupVector(statements, (VectorElemCleanupFn_t)cleanupStatement);
    *statements = NULL;
}


static char* statementVecToString(Vector_t* statements, bool indent) {
    Strbuf_t* sbuf = createStrbuf();
    
    int32_t cnt = vectorGetCount(statements);
    Statement_t** stmts = (Statement_t**) vectorGetBuffer(statements);

    for (uint32_t i = 0; i < cnt; i++) {
        if (indent)
            strbufWrite(sbuf, "\t");
        strbufConsume(sbuf, statementToString(stmts[i]));
        if(i != (cnt-1))
            strbufWrite(sbuf, "\n");
    }

    return detachStrbuf(&sbuf);
}