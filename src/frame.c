#include "frame.h"
#include "gc.h"

Frame_t createFrame(Closure_t *cl, uint32_t basePointer)
{
    return (Frame_t) {
        .cl = cl,
        .ip = -1,
        .basePointer = basePointer,
    };
}

Instructions_t frameGetInstructions(Frame_t *frame)
{
    return frame->cl->fn->instructions;
}
