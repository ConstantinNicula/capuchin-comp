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

CompError_t compilerCompile(Compiler_t* comp, Program_t* program) {
    return COMP_NO_ERROR;
} 

Bytecode_t compilerGetBytecode(Compiler_t* comp) {
    return (Bytecode_t) {
        .instructions = comp->instructions,
        .constants = comp->constants,
    };
}

