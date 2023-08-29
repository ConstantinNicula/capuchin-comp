#include "refcount.h"
#include "utils.h"
#include <stdint.h>
#include <malloc.h>

typedef struct RefCountHeader {
    uint32_t cnt;
} RefCountHeader_t;

void* createRefCountPtr(size_t size) {
    // allocate an additional RefCountHeader_t used for ref counting 
    // *-----*---------
    // | cnt |  data....
    // *-----*----------
    //       |-> return ptr 
    RefCountHeader_t* ptr = malloc(sizeof(RefCountHeader_t) + size);
    if (!ptr) HANDLE_OOM();
    ptr->cnt = 1;
    return (void*)((char*)ptr + sizeof(RefCountHeader_t));
}

static RefCountHeader_t* getHeader(void *ptr) {
    return (RefCountHeader_t*)((char*)ptr - sizeof(RefCountHeader_t));
}

void refCountPtrInc(void* ptr) {
    RefCountHeader_t* header = getHeader(ptr);
    header->cnt++;
}

uint32_t refCountPtrDec(void* ptr) {
    RefCountHeader_t* header = getHeader(ptr);
    header->cnt--;
    return header->cnt;
}

void cleanupRefCountedPtr(void* ptr) {
    if (!ptr) return;
    free((void*)getHeader(ptr));
}