#ifndef _COMPILER_H_
#define _COMPILER_H_

#include "ast.h"
#include "object.h"
#include "slice.h"
#include "code.h"
#include "vector.h"
#include "symbol_table.h"

typedef struct EmittedInstruction {
    OpCode_t opcode;
    uint32_t position; 
} EmittedInstruction_t; 

typedef struct CompilationScope {
    Instructions_t instructions;
    EmittedInstruction_t lastInstruction;
    EmittedInstruction_t previousInstruction;
} CompilationScope_t;

DEFINE_VECTOR_TYPE(CompilationScope, CompilationScope_t)

typedef struct Compiler {
    VectorObjects_t* constants; 

    bool externalStorage; 
    SymbolTable_t* symbolTable;

    VectorCompilationScope_t* scopes;
    uint32_t scopeIndex; 
} Compiler_t;

typedef enum CompError {
    COMP_NO_ERROR = 0,
    COMP_UNKNOWN_OPERATOR,
    COMP_UNDEFINED_VARIABLE,
} CompError_t;

typedef struct Bytecode {
    Instructions_t instructions;
    VectorObjects_t* constants; 
} Bytecode_t; 

void cleanupBytecode(Bytecode_t* bytecode);

Compiler_t createCompiler(); 
Compiler_t createCompilerWithState(SymbolTable_t* s, VectorObjects_t* constants); 
void cleanupCompiler(Compiler_t* comp); 

CompError_t compilerCompile(Compiler_t* comp, Program_t* program); 
Bytecode_t compilerGetBytecode(Compiler_t* comp);

uint32_t compilerEmit(Compiler_t* comp, OpCode_t op,const int operands[]);
void compilerEnterScope(Compiler_t* comp);
Instructions_t compilerLeaveScope(Compiler_t* comp);

#endif