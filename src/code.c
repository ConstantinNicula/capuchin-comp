#include <stddef.h>
#include <stdarg.h>
#include <assert.h>
#include "sbuf.h"
#include "code.h"
#include "utils.h"


static char* fmtInstruction(const OpDefinition_t* def, SliceInt_t operands);

static OpDefinition_t definitions[_OP_COUNT] = {
    [OP_CONSTANT] = {"OpConstant", .argCount=1, .argWidths={2}},

    [OP_ADD] = {"OpAdd", .argCount=0, .argWidths={0}},
    [OP_SUB] = {"OpSub", .argCount=0, .argWidths={0}},
    [OP_MUL] = {"OpMul", .argCount=0, .argWidths={0}},
    [OP_DIV] = {"OpDiv", .argCount=0, .argWidths={0}},

    [OP_TRUE] = {"OpTrue", .argCount=0, .argWidths={0}},
    [OP_FALSE] = {"OpFalse", .argCount=0, .argWidths={0}},
    [OP_NULL] = {"OpNull", .argCount=0, .argWidths={0}},

    [OP_EQUAL] = {"OpEqual", .argCount=0, .argWidths={0}},
    [OP_NOT_EQUAL] = {"OpNotEqual", .argCount=0, .argWidths={0}},
    [OP_GREATER_THAN] = {"OpGreaterThan", .argCount=0, .argWidths={0}},

    [OP_MINUS] = {"OpMinus", .argCount=0, .argWidths={0}},
    [OP_BANG] = {"OpBang", .argCount=0, .argWidths={0}},

    [OP_JUMP_NOT_TRUTHY] = {"OpJumpNotTruthy", .argCount=1, .argWidths={2}},
    [OP_JUMP] = {"OpJump", .argCount=1, .argWidths={2}},

    [OP_GET_GLOBAL] = {"OpGetGlobal", .argCount=1, .argWidths={2}},
    [OP_SET_GLOBAL] = {"OpSetGlobal", .argCount=1, .argWidths={2}},

    [OP_ARRAY] = {"OpArray", .argCount=1, .argWidths={2}},
    [OP_HASH] = {"OpHash", .argCount=1, .argWidths={2}},
    [OP_INDEX] = {"OpIndex", .argCount=0, .argWidths={0}},
    
    [OP_CALL] = {"OpCall", .argCount=1, .argWidths={1}},
    [OP_RETURN_VALUE] = {"OpReturnValue", .argCount=0, .argWidths={0}},
    [OP_RETURN] = {"OpReturn", .argCount=0, .argWidths={0}},

    [OP_SET_LOCAL] = {"OpSetLocal", .argCount=1, .argWidths={1}},
    [OP_GET_LOCAL] = {"OpGetLocal", .argCount=1, .argWidths={1}},

    [OP_GET_BUILTIN] = {"OpGetBuiltin", .argCount=1, .argWidths={1}},
    [OP_CLOSURE] = {"OpClosure", .argCount=2, .argWidths={2, 1}},
    [OP_GET_FREE] = {"OpGetFree", .argCount=1, .argWidths={1}},
    [OP_POP] = {"OpPop", .argCount=0, .argWidths={0}},
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

SliceByte_t codeMake(OpCode_t op, const int operands[]) {
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
                assert(0 && "Unreachable"); 
                break;
        }
        offset += width;
    }

    return instruction;
}


char* instructionsToString(Instructions_t ins) {
    Strbuf_t* sbuf = createStrbuf(); 
    uint32_t insLen = sliceByteGetLen(ins); 
    uint32_t i = 0; 
    
    while (i < insLen) {
        const OpDefinition_t* def = opLookup(ins[i]);
        if (!def) {
            strbufConsume(sbuf, 
                strFormat("ERROR: opcode %d undefined\n", ins[i]));
            break;
        }

        uint8_t bytesRead = 0;
        SliceInt_t operands = codeReadOperands(def, &ins[i+1], &bytesRead);
        char *operandsStr = fmtInstruction(def, operands);
        strbufConsume(sbuf, strFormat("%04d %s\n", i, operandsStr));
    
        cleanupSliceInt(operands);
        free(operandsStr);
        i += 1 + bytesRead;
    }

    return detachStrbuf(&sbuf);
}

SliceInt_t codeReadOperands(const OpDefinition_t* def, Instructions_t ins, uint8_t* bytesRead) {
    SliceInt_t operands = createSliceInt(def->argCount);
    uint8_t offset = 0;

    for (uint8_t i = 0; i < def->argCount; i++) {
        uint8_t width = def->argWidths[i];
        switch(width) {
            case 2: 
                operands[i] = (ins[offset] << 8) | ins[offset+1];
                break;
            case 1: 
                operands[i] = ins[offset];
                break;
            default: 
                assert(0 && "Unreachable");
                break;
        }
        offset += width;
    }
    *bytesRead = offset;
    return operands;
}

static char* fmtInstruction(const OpDefinition_t* def, SliceInt_t operands) {
    if (def->argCount != sliceIntGetLen(operands)) {
        return strFormat("ERROR: operand len %d does not match defined %d\n",
            sliceIntGetLen(operands), def->argCount);
    }

    switch (def->argCount) {
        case 0: 
            return cloneString(def->name);
        case 1: 
            return strFormat("%s %d", def->name, operands[0]);
        case 2: 
            return strFormat("%s %d %d", def->name, operands[0], operands[1]);
    }

    return strFormat("ERROR: unhandled operandCount for %s\n", def->name);
}

