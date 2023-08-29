#ifndef _REFCOUNT_H_
#define _REFCOUNT_H_

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h> 

void* createRefCountPtr(size_t size);
void cleanupRefCountedPtr(void* ptr);

void refCountPtrInc(void* ptr);
uint32_t refCountPtrDec(void* ptr);

#endif