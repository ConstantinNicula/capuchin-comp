#include <assert.h>
#include "compiler.h"
#include "builtin.h"
#include "gc.h"

IMPL_VECTOR_TYPE(CompilationScope, CompilationScope_t);

Compiler_t createCompiler() {
    VectorCompilationScope_t* scopes = createVectorCompilationScope();
    vectorCompilationScopeAppend(scopes, (CompilationScope_t) {
        .instructions = createSliceByte(0),
        .lastInstruction = {0},
        .previousInstruction = {0},
    });

    SymbolTable_t* symbolTable = createSymbolTable();
    BuiltinFunctionDef_t* builtinDefs = getBuiltinDefs();
    
    uint32_t i = 0;
    while (builtinDefs[i].name) {
        symbolTableDefineBuiltin(symbolTable, i, builtinDefs[i].name);
        i++;
    }

    return (Compiler_t) {
        .constants = createVectorObjects(),
        .symbolTable = symbolTable,
        .externalStorage = false,
        .scopes = scopes, 
        .scopeIndex = 0, 
    };
}

Compiler_t createCompilerWithState(SymbolTable_t* sym, VectorObjects_t* constants) {
    VectorCompilationScope_t* scopes = createVectorCompilationScope();
    vectorCompilationScopeAppend(scopes, (CompilationScope_t) {
        .instructions = createSliceByte(0),
        .lastInstruction = {0},
        .previousInstruction = {0},
    });
    return (Compiler_t) {
        .constants = constants,
        .symbolTable = sym,
        .externalStorage = true,
        .scopes = scopes, 
        .scopeIndex = 0, 
    };
}


void cleanupCompilationScope(CompilationScope_t* scope) {
    if (!scope) return;
    cleanupSliceByte(scope->instructions);
}

void cleanupCompiler(Compiler_t* comp) {
    if(!comp) return;

    cleanupVectorCompilationScope(&comp->scopes, cleanupCompilationScope);
    
    // cleanup symtable and constants only if owned 
    if (!comp->externalStorage) {
        cleanupSymbolTable(comp->symbolTable);
        cleanupVectorObjects(&comp->constants, NULL);
    }
}

void cleanupBytecode(Bytecode_t* bytecode) {
    cleanupSliceByte(bytecode->instructions);
    cleanupVectorObjects(&bytecode->constants, NULL);
}

static CompError_t compilerCompileProgram(Compiler_t* comp, Program_t* program);

static CompError_t compilerCompileStatement(Compiler_t* comp, Statement_t* statement);
static CompError_t compilerCompileExpressionStatement(Compiler_t* comp, ExpressionStatement_t* statement);
static CompError_t compilerCompileBlockStatement(Compiler_t* comp, BlockStatement_t* statement);
static CompError_t compilerCompileLetStatement(Compiler_t* comp, LetStatement_t* statement); 
static CompError_t compilerCompileReturnStatement(Compiler_t* comp, ReturnStatement_t* statement);

static CompError_t compilerCompileExpression(Compiler_t* comp, Expression_t* expression);
static CompError_t compilerCompileInfixExpression(Compiler_t* comp, InfixExpression_t* expression); 
static CompError_t compilerCompileIntegerLiteral(Compiler_t* comp, IntegerLiteral_t* expression); 
static CompError_t compilerCompileBooleanLiteral(Compiler_t* comp, BooleanLiteral_t* expression);
static CompError_t compilerCompilePrefixExpression(Compiler_t* comp, PrefixExpression_t* prefix); 
static CompError_t compilerCompileIfExpression(Compiler_t* comp, IfExpression_t* expression);
static CompError_t compilerCompileIdentifier(Compiler_t* comp, Identifier_t* ident); 
static CompError_t compilerCompileStringLiteral(Compiler_t* comp, StringLiteral_t* strLit); 
static CompError_t compilerCompileArrayLiteral(Compiler_t* comp, ArrayLiteral_t* arrayLit); 
static CompError_t compilerCompileHashLiteral(Compiler_t* comp, HashLiteral_t* hashLit); 
static CompError_t compilerCompileIndexExpression(Compiler_t* comp, IndexExpression_t* indExpr);
static CompError_t compilerCompileFunctionLiteral(Compiler_t* comp, FunctionLiteral_t* func);
static CompError_t compilerCompileCallExpression(Compiler_t* comp, CallExpression_t* expression);

static void compilerLoadSymbol(Compiler_t* comp, Symbol_t* sym);

static SliceByte_t* compilerCurrentInstructions(Compiler_t* comp);
static uint32_t compilerAddInstruction(Compiler_t* comp, SliceByte_t ins); 
static uint32_t compilerAddConstant(Compiler_t* comp, Object_t* obj); 

static void compilerSetLastInstruction(Compiler_t* comp, OpCode_t op, uint32_t pos); 
static bool compilerLastInstructionIs(Compiler_t* comp, OpCode_t op); 
static void compilerRemoveLastPop(Compiler_t* comp); 
static void compilerReplaceInstruction(Compiler_t* comp, uint32_t pos, SliceByte_t newInstruction);
static void compilerReplaceLastPopWithReturn(Compiler_t* comp); 
static void compilerChangeOperand(Compiler_t* comp, uint32_t pos, int operand); 

Bytecode_t compilerGetBytecode(Compiler_t* comp) {
    Bytecode_t bytecode = {
        .instructions = copySliceByte(*compilerCurrentInstructions(comp)),
        .constants = (!comp->externalStorage) ? copyVectorObjects(comp->constants, NULL) : comp->constants,
    };

    return bytecode;
}

void compilerEnterScope(Compiler_t* comp) {
    CompilationScope_t scope = (CompilationScope_t) {
        .instructions = createSliceByte(0), 
        .lastInstruction ={0},
        .previousInstruction = {0}, 
    };
    vectorCompilationScopeAppend(comp->scopes, scope); 
    comp->scopeIndex ++; 

    comp->symbolTable = createEnclosedSymbolTable(comp->symbolTable);
}

Instructions_t compilerLeaveScope(Compiler_t* comp) {
    Instructions_t instructions = *compilerCurrentInstructions(comp);   
    (void) vectorCompilationScopePop(comp->scopes);
    comp->scopeIndex--;
    
    SymbolTable_t* inner = comp->symbolTable; 
    comp->symbolTable = inner->outer;
    cleanupSymbolTable(inner);

    return instructions;
}


static void compilerLoadSymbol(Compiler_t* comp, Symbol_t* sym) {
    switch(sym->scope) {
        case SCOPE_LOCAL:
            compilerEmit(comp, OP_GET_LOCAL, (const int[]) {sym->index});
            break;
        case SCOPE_GLOBAL:
            compilerEmit(comp, OP_GET_GLOBAL, (const int[]) {sym->index});
            break;
        case SCOPE_BUILTIN:
            compilerEmit(comp, OP_GET_BUILTIN, (const int[]) {sym->index}); 
            break;
        default:
            break;
    }
}

static SliceByte_t* compilerCurrentInstructions(Compiler_t* comp) {
    return &(comp->scopes->buf[comp->scopeIndex].instructions);
}

static uint32_t compilerAddConstant(Compiler_t* comp, Object_t* obj) {
    gcSetRef(obj, GC_REF_COMPILE_CONSTANT);
    vectorObjectsAppend(comp->constants, obj);
    return vectorObjectsGetCount(comp->constants) - 1; 
}

static uint32_t compilerAddInstruction(Compiler_t* comp, SliceByte_t ins) {
    uint32_t posNewInstruction = sliceByteGetLen(*compilerCurrentInstructions(comp));
    sliceByteAppend(compilerCurrentInstructions(comp), ins, sliceByteGetLen(ins));
    return posNewInstruction;
}

static void compilerSetLastInstruction(Compiler_t* comp, OpCode_t op, uint32_t pos) {
    EmittedInstruction_t previous = comp->scopes->buf[comp->scopeIndex].lastInstruction;
    EmittedInstruction_t last = (EmittedInstruction_t) {.opcode = op, .position = pos}; 

    comp->scopes->buf[comp->scopeIndex].previousInstruction = previous; 
    comp->scopes->buf[comp->scopeIndex].lastInstruction = last; 
}

uint32_t compilerEmit(Compiler_t* comp, OpCode_t op,const int operands[]) {
    SliceByte_t ins = codeMake(op, operands);
    uint32_t pos = compilerAddInstruction(comp, ins);
    compilerSetLastInstruction(comp, op, pos);
    cleanupSliceByte(ins);
    return pos;
}


static bool compilerLastInstructionIs(Compiler_t* comp, OpCode_t op) {
    if (sliceByteGetLen(*compilerCurrentInstructions(comp)) == 0) {
        return false;
    }
    return comp->scopes->buf[comp->scopeIndex].lastInstruction.opcode == op;
}

static void compilerRemoveLastPop(Compiler_t* comp) {
    EmittedInstruction_t last = comp->scopes->buf[comp->scopeIndex].lastInstruction;
    EmittedInstruction_t previous = comp->scopes->buf[comp->scopeIndex].previousInstruction;

    sliceByteResize(compilerCurrentInstructions(comp), last.position);
    comp->scopes->buf[comp->scopeIndex].lastInstruction = previous;
}

static void compilerReplaceInstruction(Compiler_t* comp, uint32_t pos, SliceByte_t newInstruction) {
    uint32_t len = sliceByteGetLen(newInstruction);
    for (uint32_t i = 0; i < len; i++) {
        (*compilerCurrentInstructions(comp))[pos + i] = newInstruction[i];
    } 
}

static void compilerReplaceLastPopWithReturn(Compiler_t* comp) {
    uint32_t lastPos = comp->scopes->buf[comp->scopeIndex].lastInstruction.position;
    SliceByte_t tmpInstr = codeMakeV(OP_RETURN_VALUE);
    compilerReplaceInstruction(comp, lastPos, tmpInstr);
    comp->scopes->buf[comp->scopeIndex].lastInstruction.opcode = OP_RETURN_VALUE;
    cleanupSliceByte(tmpInstr);
}

static void compilerChangeOperand(Compiler_t* comp, uint32_t pos, int operand) {
    OpCode_t op = (*compilerCurrentInstructions(comp))[pos];
    SliceByte_t newInstruction = codeMake(op, (const int[]) {operand});

    compilerReplaceInstruction(comp, pos, newInstruction);
    cleanupSliceByte(newInstruction);
}



CompError_t compilerCompile(Compiler_t* comp, Program_t* program) {
    return compilerCompileProgram(comp, program);
}

CompError_t compilerCompileProgram(Compiler_t* comp, Program_t* program) {
    uint32_t stmtCnt = programGetStatementCount(program);
    Statement_t** stmts = programGetStatements(program);
    for (uint32_t i = 0; i < stmtCnt; i++) {
        CompError_t ret = compilerCompileStatement(comp, stmts[i]);
        if (ret != COMP_NO_ERROR) {
            return ret;
        }
    }

    return COMP_NO_ERROR;
}


CompError_t compilerCompileStatement(Compiler_t* comp, Statement_t* statement) {
    CompError_t err = COMP_NO_ERROR;
    switch(statement->type) {
        case STATEMENT_EXPRESSION: 
            err = compilerCompileExpressionStatement(comp, (ExpressionStatement_t*) statement);
            break;
        case STATEMENT_BLOCK: 
            err = compilerCompileBlockStatement(comp, (BlockStatement_t*) statement);
            break;
        case STATEMENT_LET:
            err = compilerCompileLetStatement(comp, (LetStatement_t*) statement);
            break;
        case STATEMENT_RETURN:
            err = compilerCompileReturnStatement(comp, (ReturnStatement_t*) statement);
            break;
        default: 
            assert(0 && "Unreachable: Unhandled statement type"); 
    }
    return err;
}


static CompError_t compilerCompileExpressionStatement(Compiler_t* comp, ExpressionStatement_t* statement) {
    Expression_t* expression = statement->expression;
    CompError_t err = compilerCompileExpression(comp, expression);
    if (err != COMP_NO_ERROR) {
        return err;
    }
    compilerEmit(comp, OP_POP, NULL);
    return COMP_NO_ERROR;
}


static CompError_t compilerCompileBlockStatement(Compiler_t* comp, BlockStatement_t* statement) {
    Statement_t** stmts = blockStatementGetStatements(statement);
    uint32_t stmtCnt = blockStatementGetStatementCount(statement);

    for (uint32_t i = 0; i < stmtCnt; i++) {
        CompError_t err = compilerCompileStatement(comp, stmts[i]);
        if (err != COMP_NO_ERROR) {
            break;
        } 
    }

    return COMP_NO_ERROR;
}

static CompError_t compilerCompileLetStatement(Compiler_t* comp, LetStatement_t* statement) {
    Symbol_t* symbol = symbolTableDefine(comp->symbolTable, statement->name->value);

    CompError_t err = compilerCompileExpression(comp, statement->value);
    if (err != COMP_NO_ERROR) {
        return err;
    }

    if (symbol->scope == SCOPE_GLOBAL) {
        compilerEmit(comp, OP_SET_GLOBAL, (const int[]) {symbol->index});    
    } else {
        compilerEmit(comp, OP_SET_LOCAL, (const int[]) {symbol->index});    
    }

    return COMP_NO_ERROR;
}

static CompError_t compilerCompileReturnStatement(Compiler_t* comp, ReturnStatement_t* statement) {
    CompError_t err = compilerCompileExpression(comp, statement->returnValue);
    if (err != COMP_NO_ERROR) {
        return err;
    }

    compilerEmit(comp, OP_RETURN_VALUE, NULL);
    return COMP_NO_ERROR;
}

CompError_t compilerCompileExpression(Compiler_t* comp, Expression_t* expression) {
    CompError_t err = COMP_NO_ERROR;

    switch(expression->type) {
        case EXPRESSION_INFIX_EXPRESSION: 
            err = compilerCompileInfixExpression(comp, (InfixExpression_t*) expression);
            break;
        case EXPRESSION_PREFIX_EXPRESSION:
            err = compilerCompilePrefixExpression(comp, (PrefixExpression_t*) expression);
            break;
        case EXPRESSION_INTEGER_LITERAL: 
            err = compilerCompileIntegerLiteral(comp, (IntegerLiteral_t*) expression);
            break;
        case EXPRESSION_BOOLEAN_LITERAL: 
            err = compilerCompileBooleanLiteral(comp, (BooleanLiteral_t*) expression);
            break;
        case EXPRESSION_IF_EXPRESSION:
            err = compilerCompileIfExpression(comp, (IfExpression_t*) expression);
            break;
        case EXPRESSION_IDENTIFIER:
            err = compilerCompileIdentifier(comp, (Identifier_t*) expression);
            break;
        case EXPRESSION_STRING_LITERAL:
            err = compilerCompileStringLiteral(comp, (StringLiteral_t*) expression);
            break;
        case EXPRESSION_ARRAY_LITERAL:
            err = compilerCompileArrayLiteral(comp, (ArrayLiteral_t*) expression);
            break;
        case EXPRESSION_HASH_LITERAL: 
            err = compilerCompileHashLiteral(comp, (HashLiteral_t*) expression);
            break;
        case EXPRESSION_INDEX_EXPRESSION:
            err = compilerCompileIndexExpression(comp, (IndexExpression_t*)expression);
            break;
        case EXPRESSION_FUNCTION_LITERAL:
            err = compilerCompileFunctionLiteral(comp, (FunctionLiteral_t*)expression);
            break;
        case EXPRESSION_CALL_EXPRESSION:
            err = compilerCompileCallExpression(comp, (CallExpression_t*)expression);
            break;
        default: 
            assert(0 && "Unreachable: Unhandled expression type");    
    }

    return err;
}

static CompError_t compilerCompileInfixExpression(Compiler_t* comp, InfixExpression_t* infix) {
    TokenType_t operator = infix->token->type; 
    Expression_t* operandLeft = infix->left;
    Expression_t* operandRight = infix->right;

    // special handling of LT operator at compile time.
    if (operator == TOKEN_LT) {
        Expression_t*tmp = operandLeft;
        operandLeft = operandRight;
        operandRight = tmp;
        operator = TOKEN_GT;
    }

    CompError_t ret = compilerCompileExpression(comp, operandLeft);
    if (ret != COMP_NO_ERROR) {
        return ret;
    }

    ret = compilerCompileExpression(comp, operandRight);
    if (ret != COMP_NO_ERROR) {
        return ret;
    }

    switch(operator) { 
        case TOKEN_PLUS: 
            compilerEmit(comp, OP_ADD, NULL);
            break;
        case TOKEN_MINUS: 
            compilerEmit(comp, OP_SUB, NULL);
            break;
        case TOKEN_ASTERISK: 
            compilerEmit(comp, OP_MUL, NULL);
            break;
        case TOKEN_SLASH: 
            compilerEmit(comp, OP_DIV, NULL);
            break;
        case TOKEN_EQ: 
            compilerEmit(comp, OP_EQUAL, NULL);
            break;
        case TOKEN_NOT_EQ:
            compilerEmit(comp, OP_NOT_EQUAL, NULL);
            break;
        case TOKEN_GT: 
            compilerEmit(comp, OP_GREATER_THAN, NULL);
            break;
        default:
            return COMP_UNKNOWN_OPERATOR;
    }

    return COMP_NO_ERROR;
}

static CompError_t compilerCompileIntegerLiteral(Compiler_t* comp, IntegerLiteral_t* intLit) {
    Integer_t* integer = createInteger(intLit->value);
    
    const int operands[] = {compilerAddConstant(comp, (Object_t*)integer)};
    compilerEmit(comp, OP_CONSTANT, operands);

    return COMP_NO_ERROR;
}

static CompError_t compilerCompileBooleanLiteral(Compiler_t* comp, BooleanLiteral_t* boolLit) {
    if (boolLit->value) {
        compilerEmit(comp, OP_TRUE, NULL); 
    } else {
        compilerEmit(comp, OP_FALSE, NULL); 
    }

    return COMP_NO_ERROR;
}

static CompError_t compilerCompilePrefixExpression(Compiler_t* comp, PrefixExpression_t* prefix) {
    CompError_t err = compilerCompileExpression(comp, prefix->right);
    if (err != COMP_NO_ERROR) {
        return err;
    }

    switch(prefix->token->type) {
        case TOKEN_BANG: 
            compilerEmit(comp, OP_BANG, NULL);
            break;
        case TOKEN_MINUS:
            compilerEmit(comp, OP_MINUS, NULL);
            break;
        default:
            return COMP_UNKNOWN_OPERATOR;
    }

    return COMP_NO_ERROR;
}


static CompError_t compilerCompileIfExpression(Compiler_t* comp, IfExpression_t* expression) {
    CompError_t err = compilerCompileExpression(comp, expression->condition);
    if (err != COMP_NO_ERROR) {
        return err;
    }

    // Emit an `OpJumpNotTruthy` with a bogus value
    uint32_t jumpNotTruthyPos = compilerEmit(comp, OP_JUMP_NOT_TRUTHY, (const int[]) {9999});

    err = compilerCompileBlockStatement(comp, expression->consequence);
    if (err != COMP_NO_ERROR) {
        return err; 
    }

    if (compilerLastInstructionIs(comp, OP_POP)) {
        compilerRemoveLastPop(comp);
    }

    // Emit an `OpJump` with a bogus value 
    uint32_t jumpPos = compilerEmit(comp, OP_JUMP, (const int[]) {9999});
    
    uint32_t afterConsequencePos = sliceByteGetLen(*compilerCurrentInstructions(comp));
    compilerChangeOperand(comp, jumpNotTruthyPos, afterConsequencePos);

    // Emit OP_NULL in case alternative branch does not exist
    if (expression->alternative == NULL) {
        compilerEmit(comp, OP_NULL, NULL);
    } else {
        err = compilerCompileBlockStatement(comp, expression->alternative);
        if (err != COMP_NO_ERROR) {
            return err;
        }

        if (compilerLastInstructionIs(comp, OP_POP)) {
            compilerRemoveLastPop(comp);
        }

    }

    uint32_t afterAlternativePos = sliceByteGetLen(*compilerCurrentInstructions(comp));
    compilerChangeOperand(comp, jumpPos, afterAlternativePos);   

    return COMP_NO_ERROR;
}

static CompError_t compilerCompileIdentifier(Compiler_t* comp, Identifier_t* ident) {
    Symbol_t* symbol = symbolTableResolve(comp->symbolTable, ident->value);
    if (!symbol) return COMP_UNDEFINED_VARIABLE;
    compilerLoadSymbol(comp, symbol); 
    return COMP_NO_ERROR;
}

static CompError_t compilerCompileStringLiteral(Compiler_t* comp, StringLiteral_t* strLit) {
    String_t* str = createString(strLit->value);
    int constIdx = compilerAddConstant(comp, (Object_t*)str);
    compilerEmit(comp, OP_CONSTANT, (const int[]){constIdx});
    return COMP_NO_ERROR;
}

static CompError_t compilerCompileArrayLiteral(Compiler_t* comp, ArrayLiteral_t* arrayLit) {
    Expression_t** elems = arrayLiteralGetElements(arrayLit);
    uint32_t elemCnt = arrayLiteralGetElementCount(arrayLit);

    for (uint32_t i = 0; i < elemCnt; i++) {
        CompError_t err = compilerCompileExpression(comp, elems[i]);
        if (err != COMP_NO_ERROR) {
            return err;
        }
    }

    compilerEmit(comp, OP_ARRAY, (const int[]) {elemCnt});
    return COMP_NO_ERROR;
}

static CompError_t compilerCompileHashLiteral(Compiler_t* comp, HashLiteral_t* hashLit) {
    uint32_t pairsCount = hashLiteralGetPairsCount(hashLit);
    for (uint32_t i = 0; i < pairsCount; i++) {
        Expression_t* key = NULL;
        Expression_t* value = NULL;

        CompError_t err = COMP_NO_ERROR;
        hashLiteralGetPair(hashLit, i, &key, &value);

        err = compilerCompileExpression(comp, key);
        if (err != COMP_NO_ERROR) {
            return err;
        } 

        err = compilerCompileExpression(comp, value); 
        if (err != COMP_NO_ERROR) {
            return err;
        } 
    }
    
    compilerEmit(comp, OP_HASH, (const int[]) {2 * pairsCount});
    return COMP_NO_ERROR;
}

static CompError_t compilerCompileIndexExpression(Compiler_t* comp, IndexExpression_t* indExpr) {
    CompError_t err = compilerCompileExpression(comp, indExpr->left);
    if (err != COMP_NO_ERROR) {
        return err;
    }

    err = compilerCompileExpression(comp, indExpr->right);
    if (err != COMP_NO_ERROR) {
        return err;
    }

    compilerEmit(comp, OP_INDEX, NULL);

    return COMP_NO_ERROR;
}

static CompError_t compilerCompileFunctionLiteral(Compiler_t* comp, FunctionLiteral_t* func) {
    compilerEnterScope(comp);

    uint32_t numParams = functionLiteralGetParameterCount(func);
    Identifier_t** params = functionLiteralGetParameters(func);
    for (uint32_t i = 0; i < numParams; i++) {
        symbolTableDefine(comp->symbolTable, params[i]->value);
    }

    CompError_t err = compilerCompileBlockStatement(comp, func->body);
    if (err != COMP_NO_ERROR) {
        return err;
    }

    if (compilerLastInstructionIs(comp, OP_POP)) {
        compilerReplaceLastPopWithReturn(comp);
    }

    if (!compilerLastInstructionIs(comp, OP_RETURN_VALUE)) {
        compilerEmit(comp, OP_RETURN, NULL);
    }

    uint32_t numLocals = comp->symbolTable->numDefinitions; 
    Instructions_t instr = compilerLeaveScope(comp);
     
    CompiledFunction_t* compiledFn = createCompiledFunction(instr, numLocals, numParams);
    const int args[] = {compilerAddConstant(comp, (Object_t*) compiledFn), 0}; 
    compilerEmit(comp, OP_CLOSURE, args);
    
    return COMP_NO_ERROR;
}

static CompError_t compilerCompileCallExpression(Compiler_t* comp, CallExpression_t* expression) {
    CompError_t err = compilerCompileExpression(comp, expression->function); 
    if (err != COMP_NO_ERROR) {
        return err;
    }
    
    uint32_t numArgs = callExpresionGetArgumentCount(expression);
    Expression_t** args = callExpressionGetArguments(expression);
    for (uint32_t i = 0; i < numArgs; i++) {
        err = compilerCompileExpression(comp, args[i]);
        if (err != COMP_NO_ERROR) {
            return err;
        }
    }

    compilerEmit(comp, OP_CALL, (const int[]) {numArgs});
    return COMP_NO_ERROR; 
}