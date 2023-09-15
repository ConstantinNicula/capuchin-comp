#ifndef _SLICE_H_
#define _SLICE_H_

#include <stddef.h>
#include <stdint.h>

#define _SLICE_TYPE(NAME) Slice##NAME##_t

#define DEF_SLICE_TYPE(NAME, TYPE)                                       \
typedef TYPE* _SLICE_TYPE(NAME);                                         \
_SLICE_TYPE(NAME) createSlice##NAME(size_t sz);                          \
void cleanupSlice##NAME(_SLICE_TYPE(NAME) slice);                        \
size_t slice##NAME##GetLen(_SLICE_TYPE(NAME) slice);                     \
void slice##NAME##Resize(_SLICE_TYPE(NAME) *slice, size_t len);          \
void slice##NAME##Append(_SLICE_TYPE(NAME) *slice, TYPE* buf, size_t len);


#define IMPL_SLICE_TYPE(NAME, TYPE)                               \
_SLICE_TYPE(NAME) createSlice##NAME(size_t sz) {                  \
    extern void* allocSlicePtr(size_t cnt, size_t elemSize);      \
    return (_SLICE_TYPE(NAME)) allocSlicePtr(sz, sizeof(TYPE));   \
}                                                                 \
\
void cleanupSlice##NAME(_SLICE_TYPE(NAME) slice) {     \
    extern void freeSlicePtr(void*);                   \
    freeSlicePtr(slice);                               \
}                                                      \
\
size_t slice##NAME##GetLen(_SLICE_TYPE(NAME) slice) { \
    extern size_t slicePtrGetLen(void*);              \
    return slicePtrGetLen(slice);                     \
}                                                     \
\
void slice##NAME##Resize(_SLICE_TYPE(NAME) *slicePtr, size_t len) {  \
    extern void sliceResize(void**, size_t, size_t);                 \
    sliceResize((void**)slicePtr, len, sizeof(TYPE));                \
}                                                                    \
\
void slice##NAME##Append(_SLICE_TYPE(NAME) *slicePtr, TYPE* buf, size_t len) {  \
    extern void slicePtrAppend(void**, void*, size_t, size_t);                  \
    slicePtrAppend((void**)slicePtr, buf, len, sizeof(TYPE));                   \
}

/* Create a few standard definitions */
DEF_SLICE_TYPE(Byte, uint8_t);
DEF_SLICE_TYPE(Int, int);
#define NULL_SLICE (NULL)

#endif
