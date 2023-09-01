#ifndef _COMPILER_H_
#define _COMPILER_H_

#include "ast.h"
#include "object.h"
#include "slice.h"
#include "code.h"
#include "vector.h"


typedef struct Compiler {
    Instructions_t instructions;
    VectorObjects_t* constants; 
} Compiler_t;

typedef enum CompError {
    COMP_NO_ERROR = 0,
} CompError_t;

typedef struct Bytecode {
    Instructions_t instructions;
    VectorObjects_t* constants; 
} Bytecode_t; 


Compiler_t createCompiler(); 
void cleanupCompiler(Compiler_t* comp); 

CompError_t compilerCompile(Compiler_t* comp, Program_t* program); 
Bytecode_t compilerGetBytecode(Compiler_t* comp);




#endif