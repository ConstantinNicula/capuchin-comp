#ifndef _GC_H_
#define _GC_H_

#include <stddef.h>
#include <stdbool.h>

typedef enum GCDataType{
    GC_DATA_OBJECT, 
    GC_DATA_ENVIRONENT,
    _GC_DATA_TYPE_CNT
} GCDataType_t;


void* gcMalloc(size_t size, GCDataType_t type);
void gcFree(void* ptr);
void gcForceRun();

void gcMarkUsed(void* ptr);
bool gcMarkedAsUsed(void*ptr);

void* gcGetExtRef(void* ptr);
void gcFreeExtRef(void* ptr);

#endif