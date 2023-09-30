#ifndef _FRAME_H_
#define _FRAME_H_

#include "object.h"
#include "vector.h"

typedef struct Frame {
    CompiledFunction_t* fn;
    int32_t ip;
} Frame_t;


Frame_t createFrame(CompiledFunction_t* fn);
Instructions_t frameGetInstructions(Frame_t* frame);
void cleanupFrame(Frame_t* frame); 
#endif