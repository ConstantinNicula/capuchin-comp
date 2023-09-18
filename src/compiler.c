#include <assert.h>
#include "compiler.h"

Compiler_t createCompiler() {
    return (Compiler_t) {
        .instructions = createSliceByte(0),
        .constants = createVectorObjects(),
        .symbolTable = createSymbolTable(),
        .externalStorage = false,
    };
}

Compiler_t createCompilerWithState(SymbolTable_t* s) {
    return (Compiler_t) {
        .instructions = createSliceByte(0),
        .constants = createVectorObjects(),
        .symbolTable = s,
        .externalStorage = true,
    };
}

void cleanupCompiler(Compiler_t* comp) {
    if(!comp) return;
    cleanupSliceByte(comp->instructions);
    // Objects are GC'd no need for cleanup function
    cleanupVectorObjects(&comp->constants, NULL); 
    // cleanup symtable only if owned 
    if (!comp->externalStorage) {
        cleanupSymbolTable(comp->symbolTable);
    }
    // Stack allocated no need for free :) 
}


Bytecode_t compilerGetBytecode(Compiler_t* comp) {
    return (Bytecode_t) {
        .instructions = comp->instructions,
        .constants = comp->constants,
    };
}

static CompError_t compilerCompileProgram(Compiler_t* comp, Program_t* program);

static CompError_t compilerCompileStatement(Compiler_t* comp, Statement_t* statement);
static CompError_t compilerCompileExpressionStatement(Compiler_t* comp, ExpressionStatement_t* statement);
static CompError_t compilerCompileBlockStatement(Compiler_t* comp, BlockStatement_t* statement);
static CompError_t compilerCompileLetStatement(Compiler_t* comp, LetStatement_t* statement); 

static CompError_t compilerCompileExpression(Compiler_t* comp, Expression_t* expression);
static CompError_t compilerCompileInfixExpression(Compiler_t* comp, InfixExpression_t* expression); 
static CompError_t compilerCompileIntegerLiteral(Compiler_t* comp, IntegerLiteral_t* expression); 
static CompError_t compilerCompileBooleanLiteral(Compiler_t* comp, BooleanLiteral_t* expression);
static CompError_t compilerCompilePrefixExpression(Compiler_t* comp, PrefixExpression_t* prefix); 
static CompError_t compilerCompileIfExpression(Compiler_t* comp, IfExpression_t* expression);
static CompError_t compilerCompileIdentifier(Compiler_t* comp, Identifier_t* ident); 
static CompError_t compilerCompileStringLiteral(Compiler_t* comp, StringLiteral_t* strLit); 
static CompError_t compilerCompileArrayLiteral(Compiler_t* comp, ArrayLiteral_t* arrayLit); 

static uint32_t compilerAddConstant(Compiler_t* comp, Object_t* obj); 
static uint32_t compilerEmit(Compiler_t* comp, OpCode_t op, const int operands[]);
static uint32_t compilerAddInstruction(Compiler_t* comp, SliceByte_t ins); 

static void compilerSetLastInstruction(Compiler_t* comp, OpCode_t op, uint32_t pos); 
static bool compilerLastInstructionIsPop(Compiler_t* comp); 
static void compilerRemoveLastPop(Compiler_t* comp); 
static void compilerReplaceInstruction(Compiler_t* comp, uint32_t pos, SliceByte_t newInstruction); 
static void compilerChangeOperand(Compiler_t* comp, uint32_t pos, int operand); 

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
    CompError_t err = compilerCompileExpression(comp, statement->value);
    if (err != COMP_NO_ERROR) {
        return err;
    }

    Symbol_t* symbol = symbolTableDefine(comp->symbolTable, statement->name->value);
    compilerEmit(comp, OP_SET_GLOBAL, (const int[]) {symbol->index});    


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

    if (compilerLastInstructionIsPop(comp)) {
        compilerRemoveLastPop(comp);
    }

    // Emit an `OpJump` with a bogus value 
    uint32_t jumpPos = compilerEmit(comp, OP_JUMP, (const int[]) {9999});
    
    uint32_t afterConsequencePos = sliceByteGetLen(comp->instructions);
    compilerChangeOperand(comp, jumpNotTruthyPos, afterConsequencePos);

    // Emit OP_NULL in case alternative branch does not exist
    if (expression->alternative == NULL) {
        compilerEmit(comp, OP_NULL, NULL);
    } else {
        err = compilerCompileBlockStatement(comp, expression->alternative);
        if (err != COMP_NO_ERROR) {
            return err;
        }

        if (compilerLastInstructionIsPop(comp)) {
            compilerRemoveLastPop(comp);
        }

    }

    uint32_t afterAlternativePos = sliceByteGetLen(comp->instructions);
    compilerChangeOperand(comp, jumpPos, afterAlternativePos);   

    return COMP_NO_ERROR;
}

static CompError_t compilerCompileIdentifier(Compiler_t* comp, Identifier_t* ident) {
    Symbol_t* symbol = symbolTableResolve(comp->symbolTable, ident->value);
    if (!symbol) return COMP_UNDEFINED_VARIABLE;
    compilerEmit(comp, OP_GET_GLOBAL, (const int[]){symbol->index});
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

static uint32_t compilerAddConstant(Compiler_t* comp, Object_t* obj) {
    // TO DO GC add external ref :)
    vectorObjectsAppend(comp->constants, obj);
    return vectorObjectsGetCount(comp->constants) - 1; 
}


static uint32_t compilerEmit(Compiler_t* comp, OpCode_t op,const int operands[]) {
    SliceByte_t ins = codeMake(op, operands);
    uint32_t pos = compilerAddInstruction(comp, ins);
    compilerSetLastInstruction(comp, op, pos);
    cleanupSliceByte(ins);
    return pos;
}

static uint32_t compilerAddInstruction(Compiler_t* comp, SliceByte_t ins) {
    uint32_t posNewInstruction = sliceByteGetLen(comp->instructions);
    sliceByteAppend(&comp->instructions, ins, sliceByteGetLen(ins));
    return posNewInstruction;
}

static void compilerSetLastInstruction(Compiler_t* comp, OpCode_t op, uint32_t pos) {
    comp->previousInstruction = comp->lastInstruction; 
    comp->lastInstruction = (EmittedInstruction_t) {.opcode = op, .position = pos};
}

static bool compilerLastInstructionIsPop(Compiler_t* comp) {
    return comp->lastInstruction.opcode == OP_POP;
}

static void compilerRemoveLastPop(Compiler_t* comp) {
    sliceByteResize(&comp->instructions, comp->lastInstruction.position);
    comp->lastInstruction = comp->previousInstruction;
}

static void compilerReplaceInstruction(Compiler_t* comp, uint32_t pos, SliceByte_t newInstruction) {
    uint32_t len = sliceByteGetLen(newInstruction);
    for (uint32_t i = 0; i < len; i++) {
        comp->instructions[pos + i] = newInstruction[i];
    } 
}

static void compilerChangeOperand(Compiler_t* comp, uint32_t pos, int operand) {
    OpCode_t op = comp->instructions[pos];
    SliceByte_t newInstruction = codeMake(op, (const int[]) {operand});

    compilerReplaceInstruction(comp, pos, newInstruction);
    cleanupSliceByte(newInstruction);
}