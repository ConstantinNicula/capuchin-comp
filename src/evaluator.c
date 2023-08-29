#include <string.h>
#include "evaluator.h"
#include "utils.h"
#include "gc.h"

static Object_t* evalStatement(Statement_t* stmt, Environment_t* env);
static Object_t* evalBlockStatement(BlockStatement_t* stmt, Environment_t* env);

static Object_t* evalExpression(Expression_t* expr, Environment_t* env);
static Object_t* evalIfExpression(IfExpression_t* expr, Environment_t* env);

static Object_t* evalPrefixExpression(TokenType_t operator, Object_t* right);
static Object_t* evalBangOperatorPrefixExpression(Object_t* right);
static Object_t* evalMinusOperatorPrefixExpression(Object_t* right);
static Object_t* evalInfixExpression(TokenType_t operator, Object_t* left, Object_t* right);
static Object_t* evalIntegerInfixExpression(TokenType_t operator, Integer_t* left, Integer_t* right);
static Object_t* evalStringInfixExpression(TokenType_t operator, String_t* left, String_t* right);

static Vector_t* evalExpressions(Vector_t* exprs, Environment_t* env);
static Object_t* applyFunction(Object_t* function, Vector_t* args);
static Environment_t* extendFunctionEnv(Function_t* function, Vector_t* args);
static Object_t* unwrapReturnValue(Object_t* obj);

static Object_t* evalIndexExpression(Object_t* left, Object_t* index);
static Object_t* evalArrayIndexExpresssion(Array_t* left, Integer_t* index);
static Object_t* evalHashLiteral(HashLiteral_t* node, Environment_t* env);
static Object_t* evalHashIndexExpression(Hash_t* hash, Object_t* key);

static bool isTruthy(Object_t* obj);
static bool isError(Object_t* obj);


Object_t* evalProgram(Program_t* prog, Environment_t* env) {
    uint32_t count = programGetStatementCount(prog);
    Statement_t** stmts = programGetStatements(prog);
    Object_t* result = NULL;

    for (uint32_t i = 0; i < count; i++) {
        result = evalStatement(stmts[i], env);
        
        if (!result) {
            Error_t* err = createError(cloneString("evalStatement return NULL ptr"));
            return  gcGetExtRef(err);
        }

        switch(result->type) {
            case OBJECT_RETURN_VALUE: { 
                Object_t* value = ((ReturnValue_t*)result)->value;
                return gcGetExtRef(value);
            }   
            case OBJECT_ERROR: {
                return gcGetExtRef(result);
            }
            default:
                break; // nothing to do  
        }
    }

    return gcGetExtRef(result);

}

static Object_t* evalStatement(Statement_t* stmt, Environment_t* env) {
    switch (stmt->type)
    {
        case STATEMENT_EXPRESSION: 
            return evalExpression(((ExpressionStatement_t*)stmt)->expression, env);
        case STATEMENT_BLOCK: 
            return evalBlockStatement((BlockStatement_t*)stmt, env);
        case STATEMENT_RETURN: {
            Object_t* evalRes = evalExpression(((ReturnStatement_t*)stmt)->returnValue, env);
            if (isError(evalRes)) return evalRes;
            return (Object_t*)createReturnValue(evalRes);
        } 
        case STATEMENT_LET: {
            Object_t* evalRes = evalExpression(((LetStatement_t*)stmt)->value, env);
            if (isError(evalRes)) return evalRes;
            environmentSet(env, ((LetStatement_t*)stmt)->name->value, evalRes);
            return (Object_t*) createNull();
        }
        default:
            return NULL;
    }
}

static Object_t* evalBlockStatement(BlockStatement_t* stmt, Environment_t* env) {
    Statement_t** stmts = blockStatementGetStatements((BlockStatement_t*)stmt);
    uint32_t count = blockStatementGetStatementCount((BlockStatement_t*)stmt);

    Object_t* result = NULL;
    for (uint32_t i = 0; i < count; i++) {
        result = evalStatement(stmts[i], env);
        if (!result)
            break; 
        if (result->type == OBJECT_RETURN_VALUE || result->type == OBJECT_ERROR) {
            return result;
        }
    }
    return result;
}


static Object_t* evalExpression(Expression_t* expr, Environment_t* env) {
    switch(expr->type) 
    {
        case EXPRESSION_INTEGER_LITERAL: {
            return (Object_t*)createInteger(((IntegerLiteral_t*)expr)->value);
        }

        case EXPRESSION_BOOLEAN_LITERAL: {
            return (Object_t*)createBoolean(((BooleanLiteral_t*)expr)->value);
        }
        
        case EXPRESSION_STRING_LITERAL: {
            return (Object_t*)createString(((StringLiteral_t*)expr)->value);
        }

        case EXPRESSION_IF_EXPRESSION: {
            return (Object_t*)evalIfExpression((IfExpression_t*)expr, env);
        }

        case EXPRESSION_PREFIX_EXPRESSION: {
            TokenType_t op =  ((PrefixExpression_t*) expr)->token->type;
            
            Object_t* evalRight = evalExpression(((PrefixExpression_t*) expr)->right, env);
            if (isError(evalRight)) {
                return evalRight;
            }
            return evalPrefixExpression(op, evalRight);;
        }

        case EXPRESSION_INFIX_EXPRESSION: {
            TokenType_t op = ((InfixExpression_t*) expr)->token->type;
            
            Object_t* evalLeft = evalExpression(((InfixExpression_t*) expr)->left, env);
            if (isError(evalLeft)) {
                return evalLeft;
            }

            Object_t* evalRight = evalExpression(((InfixExpression_t*) expr)->right, env);
            if (isError(evalRight)) {
                return evalRight;
            }

            return evalInfixExpression(op, evalLeft, evalRight);;
        }

        case EXPRESSION_IDENTIFIER: {
            Object_t* val = environmentGet(env, ((Identifier_t*)expr)->value);
            if (!val) {
                char* message = strFormat("identifier not found: %s", ((Identifier_t*)expr)->value);
                return (Object_t*)createError(message);
            }
            return val;
        }

        case EXPRESSION_FUNCTION_LITERAL: {
            FunctionLiteral_t* funcLit = ((FunctionLiteral_t*)expr);
            return (Object_t*) createFunction(funcLit->parameters, funcLit->body, env);
        }

        case EXPRESSION_CALL_EXPRESSION: {
            CallExpression_t* callExpr = (CallExpression_t*) expr;
            Object_t* function = evalExpression(callExpr->function, env);
            if (isError(function)) { 
                return function;
            }

            Vector_t* args = evalExpressions(callExpr->arguments, env);
            uint32_t argsCnt = vectorGetCount(args);
            Object_t** argsBuf = (Object_t**)vectorGetBuffer(args);
            
            if ( argsCnt == 1 && isError(argsBuf[0])) {
                Error_t* err = (Error_t*)argsBuf[0];
                cleanupVector(&args, NULL);
                return (Object_t*)err;
            }
            Object_t* result = applyFunction(function, args);
            cleanupVector(&args, NULL);
            return result;
        }

        case EXPRESSION_ARRAY_LITERAL: {
            ArrayLiteral_t* arrLit = (ArrayLiteral_t*) expr;

            Vector_t* elems = evalExpressions(arrLit->elements, env);
            uint32_t elemsCnt = vectorGetCount(elems);
            Object_t** elemsBuf = (Object_t**)vectorGetBuffer(elems);
            
            if ( elemsCnt == 1 && isError(elemsBuf[0])) {
                Error_t* err = (Error_t*)elemsBuf[0];
                cleanupVector(&elems, NULL);
                return (Object_t*)err;
            }

            Array_t* arr = createArray();
            arr->elements = elems;
            return (Object_t*) arr; 
        }

        case EXPRESSION_INDEX_EXPRESSION: {
            IndexExpression_t* indexExpr = (IndexExpression_t*)expr;
            Object_t* left = evalExpression(indexExpr->left, env);
            if (isError(left)){
                return left;
            }

            Object_t* index = evalExpression(indexExpr->right, env);
            if (isError(index)){
                return index;
            }

            return evalIndexExpression(left, index);
        }

        case EXPRESSION_HASH_LITERAL: 
            return evalHashLiteral((HashLiteral_t*)expr, env);

        default: 
            char* message = strFormat("unknown expression type: %d(%s)", 
                                    expr->type, 
                                    expr->token->literal);
            return (Object_t*)createError(message);
    }
}

static Vector_t* evalExpressions(Vector_t* exprs, Environment_t* env) {
    Vector_t* result = createVector();
    
    uint32_t exprCnt = vectorGetCount(exprs);
    Expression_t** exprBuf = (Expression_t**)vectorGetBuffer(exprs);

    for (int i = 0; i < exprCnt; i++) {
        Object_t* evaluated = evalExpression(exprBuf[i], env);
        if (isError(evaluated)){
            cleanupVectorContents(result, NULL);
            vectorAppend(result, evaluated);
            return result;
        }

        vectorAppend(result, evaluated);
    }

    return result;
}

static Object_t* applyFunction(Object_t* function, Vector_t* args) {
    switch(function->type) {
        case OBJECT_FUNCTION: 
            Environment_t* extendedEnv = extendFunctionEnv((Function_t*)function, args); 
            if (!extendedEnv) {
                char* message = strFormat("Invalid parameter count: expected(%d) received (%d)", 
                                            functionGetParameterCount((Function_t*)function), vectorGetCount(args));
                return (Object_t*) createError(message);
            }
            Object_t* evaluated = evalBlockStatement(((Function_t*)function)->body, extendedEnv);
            return unwrapReturnValue(evaluated);
        case OBJECT_BUILTIN:
            return ((Builtin_t*)function)->func(args);
        default:
            char* message = strFormat("not a function: %s", objectTypeToString(function->type));
            return (Object_t*) createError(message);
    }
}

static Environment_t* extendFunctionEnv(Function_t* function, Vector_t* args) {
    Environment_t* env = createEnvironment(function->environment);

    uint32_t argsCnt = vectorGetCount(args);
    uint32_t paramsCnt = functionGetParameterCount(function);

    if (argsCnt != paramsCnt) {
        return NULL;
    }

    Object_t** argsBuf = (Object_t**)vectorGetBuffer(args);
    Identifier_t** paramBuf = functionGetParameters(function);
    for (uint32_t i = 0; i < argsCnt; i++) {
        environmentSet(env, paramBuf[i]->value, argsBuf[i]);
    }

    return env;
}

static Object_t* unwrapReturnValue(Object_t* obj) {
    if (!obj) 
        return (Object_t*) createNull();

    if (obj->type == OBJECT_RETURN_VALUE) {
        return ((ReturnValue_t*) obj)->value;
    }
    return obj;
}



static Object_t* evalIfExpression(IfExpression_t* expr, Environment_t* env) {
    Object_t* condition = evalExpression(expr->condition, env);
    if (isError(condition)) {
        return condition;
    }

    Object_t* evalRes = NULL;
    if (isTruthy(condition)) {
        evalRes = evalStatement((Statement_t*)expr->consequence, env);
    } else if (expr->alternative) {
        evalRes = evalStatement((Statement_t*)expr->alternative, env);
    } else {
        evalRes = (Object_t*)createNull();
    }

    return evalRes; 
}


static Object_t* evalPrefixExpression(TokenType_t operator, Object_t* right) {
    // TO DO use proper enum, easier to switch on token type instead of "operator" field    
    switch(operator) {
        case TOKEN_BANG: 
            return evalBangOperatorPrefixExpression(right);
        case TOKEN_MINUS: 
            return evalMinusOperatorPrefixExpression(right);
        default: 
            char* err = strFormat("unknown operator: %s%s", 
                                    tokenTypeToStr(operator),
                                    objectTypeToString(right->type));
            return (Object_t*)createError(err);
    }
}

static Object_t* evalStringInfixExpression(TokenType_t operator, String_t* left, String_t* right) {
    switch(operator) {
        case TOKEN_PLUS:
            char* strConcat = strFormat("%s%s", left->value, right->value);
            String_t* ret = createString(strConcat);
            free(strConcat);
            return (Object_t*) ret;

        case TOKEN_EQ:
            if(strcmp(left->value, right->value) == 0){
                return (Object_t*)createBoolean(true);
            }
            return (Object_t*) createBoolean(false);

        default: 
            char* err = strFormat("unknown operator: %s %s %s", 
                                        objectTypeToString(left->type),
                                        tokenTypeToStr(operator),
                                        objectTypeToString(right->type));
                    return (Object_t*)createError(err);
    }
}

static Object_t* evalBangOperatorPrefixExpression(Object_t* right) {
    switch (right->type) {
        case OBJECT_BOOLEAN:
            return ((Boolean_t*)right)->value? (Object_t*)createBoolean(false): (Object_t*)createBoolean(true);
        case OBJECT_NULL:
            return (Object_t*)createBoolean(true);
        default:    
            return (Object_t*)createBoolean(false);
    }
}


static Object_t* evalMinusOperatorPrefixExpression(Object_t* right) {
    if (right->type != OBJECT_INTEGER) {
        char* message = strFormat("unknown operator: -%s", objectTypeToString(right->type));
        return (Object_t*)createError(message);
    }

    int64_t value = ((Integer_t*)right)->value;
    return (Object_t*) createInteger(-value);
}


static Object_t* evalInfixExpression(TokenType_t operator, Object_t* left, Object_t* right) {
    // Early exit on mismatched types     
    if (left->type != right->type) {
        char* message = strFormat("type mismatch: %s %s %s", 
                            objectTypeToString(left->type), 
                            tokenTypeToStr(operator), 
                            objectTypeToString(right->type));
        return (Object_t*) createError(message);
    }

    // Integers
    if (left->type == OBJECT_INTEGER && right->type == OBJECT_INTEGER) {
        return evalIntegerInfixExpression(operator, (Integer_t*)left, (Integer_t*)right);
    }

    // Booleans
    if (left->type == OBJECT_BOOLEAN && right->type == OBJECT_BOOLEAN) {
        bool leftVal = ((Boolean_t*) left)->value;
        bool rightVal = ((Boolean_t*) right)->value;

        switch(operator) {
            case TOKEN_EQ:
                return (Object_t*) createBoolean(leftVal == rightVal);
            case TOKEN_NOT_EQ:
                return (Object_t*) createBoolean(leftVal != rightVal);
            default: 
                // Do nothing (no op)
                break;
        }
    }

    // Strings 
    if (left->type == OBJECT_STRING && right->type == OBJECT_STRING) {
        return evalStringInfixExpression(operator, (String_t*)left, (String_t*)right);
    }

    // No Op found 
    char* message = strFormat("unknown operator: %s %s %s",  
                            objectTypeToString(left->type), 
                            tokenTypeToStr(operator), 
                            objectTypeToString(right->type));
    return (Object_t*) createError(message);
}

static Object_t* evalIntegerInfixExpression(TokenType_t operator, Integer_t* left, Integer_t* right) {
    switch(operator) {
        case TOKEN_PLUS: 
            return (Object_t*) createInteger(left->value + right->value);
        case TOKEN_MINUS: 
            return (Object_t*) createInteger(left->value - right->value);
        case TOKEN_ASTERISK: 
            return (Object_t*) createInteger(left->value * right->value);
        case TOKEN_SLASH: 
            return (Object_t*) createInteger(left->value / right->value);

        case TOKEN_LT:
            return (Object_t*) createBoolean(left->value < right->value);
        case TOKEN_GT: 
            return (Object_t*) createBoolean(left->value > right->value);
        case TOKEN_EQ:
            return (Object_t*) createBoolean(left->value == right->value);
        case TOKEN_NOT_EQ:
            return (Object_t*) createBoolean(left->value != right->value);
        
        default: 
            char* message = strFormat("unkown operator: %s %s %s", 
                            objectTypeToString(left->type), 
                            tokenTypeToStr(operator), 
                            objectTypeToString(right->type));
            return (Object_t*)createError(message);
    }
}

static Object_t* evalIndexExpression(Object_t* left, Object_t* index) {
    if (left->type == OBJECT_ARRAY && index->type == OBJECT_INTEGER) {
        return evalArrayIndexExpresssion((Array_t*)left, (Integer_t*)index);
    }

    if (left->type == OBJECT_HASH) {
        return evalHashIndexExpression((Hash_t*)left, index);
        
    }

    char* message = strFormat("index operator not supported: %s", objectTypeToString(left->type));
    return (Object_t*)createError(message);
}

static Object_t* evalArrayIndexExpresssion(Array_t* left, Integer_t* index) {
    uint32_t max = arrayGetElementCount(left);
    if (index->value < 0 || index->value >= max) {
        return (Object_t*)createNull();
    }
    return arrayGetElements(left)[index->value];
}

static Object_t* evalHashIndexExpression(Hash_t* hash, Object_t* key) {
    if (!objectIsHashable(key)) {
        char* message = strFormat("unusable as hash key: %s", objectTypeToString(key->type));
        return (Object_t*)createError(message);
    }

    HashPair_t* pair = hashGetPair(hash, key);
    if (!pair) {
        return (Object_t*) createNull();
    }

    return pair->value;
}

static Object_t* evalHashLiteral(HashLiteral_t* node, Environment_t* env) {
    Hash_t* hash = createHash();
    uint32_t pairCnt = hashLiteralGetPairsCount(node);
    for(uint32_t i = 0; i < pairCnt; i++) {
        Expression_t *keyNode, *valueNode;
        hashLiteralGetPair(node, i, &keyNode, &valueNode);

        Object_t* key = evalExpression(keyNode, env);
        if (isError(key)) {
            return key;
        }

        if (!objectIsHashable(key)) {
            char* err = strFormat("unusable as hash key: %s", objectTypeToString(key->type));
            return (Object_t*) createError(err);
        }

        Object_t* value = evalExpression(valueNode, env);
        if (isError(value)) {
            return key;
        }

        hashInsertPair(hash, createHashPair(key, value));
    }
    return (Object_t*) hash;
}

static bool isTruthy(Object_t* obj) {
    switch(obj->type) {
        case OBJECT_BOOLEAN:
            return ((Boolean_t*)obj)->value;
        case OBJECT_NULL:
            return false; 
        default:    
            return true;
    }
}

static bool isError(Object_t* obj) {
    if (obj) {
        return obj->type == OBJECT_ERROR;
    }

    return false;
}