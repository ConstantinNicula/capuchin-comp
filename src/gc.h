#ifndef _GC_H_
#define _GC_H_

#include <stddef.h>
#include <stdbool.h>

typedef enum GCRefType{
    GC_REF_INTERNAL, 
    GC_REF_COMPILE_CONSTANT, 
    GC_REF_GLOBAL, 
    GC_REF_STACK,
} GCRefType_t;

void* gcMalloc(size_t size);
void gcFree(void* ptr);

void gcSetRef(void* ptr, GCRefType_t refType);
void gcClearRef(void* ptr, GCRefType_t refType);
bool gcHasRef(void* ptr, GCRefType_t refType);

void gcForceRun();
#endif