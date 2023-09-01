#include "code.h"
#include <stddef.h>
#include <stdarg.h>

static OpDefinition_t definitions[_OP_COUNT] = {
    {"OpConstant", .argCount=1, .argWidths={2}},
};


const OpDefinition_t*  opLookup(OpCode_t op) {
    if (op < 0 || op >= _OP_COUNT) return NULL;
    return &definitions[op]; 
}


SliceByte_t codeMakeV(OpCode_t op, ...) {
    const OpDefinition_t* def = opLookup(op);
    if(!def) return createSliceByte(0);

    // declare pointer to arg list 
    va_list ptr; 
    va_start(ptr, op);

    // create slice from var args 
    SliceInt_t argv = createSliceInt(def->argCount);
    for (uint8_t i = 0; i < def->argCount; i++) {
        argv[i] = va_arg(ptr, int);
    }
    SliceByte_t ret = codeMake(op, argv);
    
    // cleanup
    cleanupSliceInt(argv);  
    va_end(ptr);

    return ret;
}

SliceByte_t codeMake(OpCode_t op, int* operands) {
    const OpDefinition_t* def = opLookup(op);
    if (!def) {
        // return empty slice 
        return createSliceByte(0);
    }

    uint8_t instructionLen = 1; 
    for (uint8_t i = 0; i < def->argCount; i++) {
        instructionLen += def->argWidths[i];
    }

    // create slice of correct size 
    SliceByte_t instruction = createSliceByte(instructionLen);
    instruction[0] = (uint8_t)op;

    uint8_t offset = 1;
    for (uint8_t i = 0; i < def->argCount; i++) {
        uint8_t width = def->argWidths[i];
        switch(width) {
            case 2:
                instruction[offset] = (operands[i] & 0xFF00) >> 8;
                instruction[offset + 1] = (operands[i] & 0x00FF);  
                break;
            case 1:
                instruction[offset] = operands[i];
                break;
            default:
                // nothing 
                break;
        }
        offset += width;
    }

    return instruction;
}