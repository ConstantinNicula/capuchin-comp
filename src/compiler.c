#include <assert.h>
#include "compiler.h"

Compiler_t createCompiler() {
    return (Compiler_t) {
        .instructions = createSliceByte(0),
        .constants = createVectorObjects(),
    };
}

void cleanupCompiler(Compiler_t* comp) {
    if(!comp) return;
    cleanupSliceByte(comp->instructions);
    // Objects are GC'd no need for clenaup function
    cleanupVectorObjects(&comp->constants, NULL); 
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
static CompError_t compilerCompileExpression(Compiler_t* comp, Expression_t* expression);

static uint32_t compilerAddConstant(Compiler_t* comp, Object_t* obj); 
static uint32_t compilerEmit(Compiler_t* comp, OpCode_t op, int operands[]);
static uint32_t compileAddInstruction(Compiler_t* comp, SliceByte_t ins); 

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
    switch(statement->type) {
        case STATEMENT_EXPRESSION: {
            Expression_t* expression = ((ExpressionStatement_t*) statement)->expression;
            CompError_t ret = compilerCompileExpression(comp, expression);
            if (ret != COMP_NO_ERROR) {
                return ret;
            }
            break;
        }
        default: 
            assert(0 && "Unreachable"); 
    }
    return COMP_NO_ERROR;
}

CompError_t compilerCompileExpression(Compiler_t* comp, Expression_t* expression) {
    switch(expression->type) {
        case EXPRESSION_INFIX_EXPRESSION: {
            InfixExpression_t* infix = (InfixExpression_t*) expression;
            CompError_t ret = compilerCompileExpression(comp, infix->left);
            if (ret != COMP_NO_ERROR) {
                return ret;
            }

            ret = compilerCompileExpression(comp, infix->right);
            if (ret != COMP_NO_ERROR) {
                return ret;
            }

            break;
        }

        case EXPRESSION_INTEGER_LITERAL: {
            IntegerLiteral_t* intLit = (IntegerLiteral_t*) expression;
            Integer_t* integer = createInteger(intLit->value);
            
            int operands[] = {compilerAddConstant(comp, (Object_t*)integer)};
            compilerEmit(comp, OP_CONSTANT, operands);
            
            break;
        }    

        default: 
            assert(0 && "Unreachable");    
    }

    return COMP_NO_ERROR;
}


uint32_t compilerAddConstant(Compiler_t* comp, Object_t* obj) {
    // TO DO GC add external ref :)
    vectorObjectsAppend(comp->constants, obj);
    return vectorObjectsGetCount(comp->constants) - 1; 
}


uint32_t compilerEmit(Compiler_t* comp, OpCode_t op,  int operands[]) {
    SliceByte_t ins = codeMake(op, operands);
    uint32_t pos = compileAddInstruction(comp, ins);
    cleanupSliceByte(ins);
    return pos;
}

uint32_t compileAddInstruction(Compiler_t* comp, SliceByte_t ins) {
    uint32_t posNewInstruction = sliceByteGetLen(comp->instructions);
    sliceByteAppend(&comp->instructions, ins, sliceByteGetLen(ins));
    return posNewInstruction;
}