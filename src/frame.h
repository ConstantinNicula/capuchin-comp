#ifndef _FRAME_H_
#define _FRAME_H_

#include "object.h"
#include "vector.h"

typedef struct Frame {
    CompiledFunction_t* fn;
    int32_t ip;
    uint32_t basePointer;
} Frame_t;


Frame_t createFrame(CompiledFunction_t* fn, uint32_t basePointer);
Instructions_t frameGetInstructions(Frame_t* frame);
void cleanupFrame(Frame_t* frame); 
#endif