#ifndef _FRAME_H_
#define _FRAME_H_

#include "object.h"
#include "vector.h"

typedef struct Frame {
    Closure_t* cl;
    int32_t ip;
    uint32_t basePointer;
} Frame_t;


Frame_t createFrame(Closure_t* cl, uint32_t basePointer);
Instructions_t frameGetInstructions(Frame_t* frame);
#endif