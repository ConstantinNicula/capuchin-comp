#ifndef _CODE_H_
#define _CODE_H_
#include "slice.h"
#include <stdint.h> 

/* List of supported in instructions (check c file for op structure)*/
typedef enum OpCode {
    OP_CONSTANT = 0,

    OP_ADD,
    OP_SUB,
    OP_MUL, 
    OP_DIV,
    
    OP_TRUE,
    OP_FALSE,
    OP_NULL,

    OP_EQUAL,
    OP_NOT_EQUAL,
    OP_GREATER_THAN,

    OP_MINUS,
    OP_BANG,

    OP_JUMP_NOT_TRUTHY,
    OP_JUMP,

    OP_GET_GLOBAL,
    OP_SET_GLOBAL,
    
    OP_ARRAY,
    OP_HASH,
    
    OP_POP,
    _OP_COUNT,
} OpCode_t;

/* Maximum number of operands any instruction for a given instruction */
#define OP_MAX_ARGS 3 

/* Defines the structure a of a given instruction*/
typedef struct OpDefinition {
    const char* name; 
    uint8_t argCount;
    uint8_t argWidths[OP_MAX_ARGS];
} OpDefinition_t;


/* Container for bytecode */
typedef SliceByte_t Instructions_t; 

char* instructionsToString(Instructions_t ins); 

/* External API */

const OpDefinition_t*  opLookup(OpCode_t op);
SliceByte_t codeMake(OpCode_t op, const int operands[]);
SliceByte_t codeMakeV(OpCode_t op, ...);
SliceInt_t codeReadOperands(const OpDefinition_t*def, Instructions_t ins, uint8_t* bytesRead);
#endif
