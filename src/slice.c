#include "slice.h"
#include "utils.h"
#include <string.h>

/* Implement definitions */
IMPL_SLICE_TYPE(Byte, uint8_t);
IMPL_SLICE_TYPE(Int, int);

size_t* slicePtrGetHeader(void* ptr) {
    return (size_t*)(((char*) ptr) - sizeof(size_t));
}

void* slicePtrGetData(size_t* header) {
    return (void*)(((char*) header) + sizeof(size_t));
}

void* allocSlicePtr(size_t cnt, size_t elemSize) {
    size_t* header = mallocChk(sizeof(size_t) + cnt * elemSize);
    *header = cnt;
    return slicePtrGetData(header);  
}

void freeSlicePtr(void* ptr) {
    free(slicePtrGetHeader(ptr));
}

size_t slicePtrGetLen(void *ptr) {
    return *slicePtrGetHeader(ptr);
}

void slicePtrAppend(void** ptrOut, void* buf, size_t len, size_t elemSize) {
    size_t* header = slicePtrGetHeader(*ptrOut);
    header = realloc(header, sizeof(size_t) + (*header) + len * elemSize);
    if (!header) 
        HANDLE_OOM();

    char* data = slicePtrGetData(header); 
    memmove(data + (*header) * elemSize, buf, len * elemSize);
    *header += len;
    (*ptrOut) = slicePtrGetData(header);
}

void sliceResize(void** ptrOut, size_t len, size_t elemSize) {
    size_t* header = slicePtrGetHeader(*ptrOut);
    header = realloc(header, sizeof(size_t) + len * elemSize);
    if (!header) 
        HANDLE_OOM();
    *header = len;
    (*ptrOut) = slicePtrGetData(header);
}

void* copySlicePtr(void*src, size_t elemSize) {
    size_t elemCnt = slicePtrGetLen(src);
    void *ret = allocSlicePtr(elemCnt, elemSize);
    memmove(ret, src, elemCnt * elemSize);
    return ret;
} 