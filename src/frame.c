#include "frame.h"
#include "gc.h"

Frame_t createFrame(CompiledFunction_t *fn, uint32_t basePointer)
{
    return (Frame_t){
        .fn = gcGetExtRef(fn),
        .ip = -1,
        .basePointer = basePointer,
    };
}

Instructions_t frameGetInstructions(Frame_t *frame)
{
    return frame->fn->instructions;
}

void cleanupFrame(Frame_t* frame) {
    gcFreeExtRef(frame->fn); 
}