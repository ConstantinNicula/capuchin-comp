#include "code.h"
#include  <stddef.h>

static OpDefinition_t definitions[_OP_COUNT] = {
    {"OpConstant", .argCount=1, .argWidths={2}},
};


const OpDefinition_t*  opLookup(OpCode_t op) {
    if (op < 0 || op >= _OP_COUNT) return NULL;
    return &definitions[op]; 
}


