#ifndef _VECTOR_H_
#define _VECTOR_H_

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "utils.h"
/*
    Use the DEFINE_VECTOR_TYPE(NAME, TYPE) in .h files to construct custom Vector 
    for a given underlying datatype of your choosing. Add a single IMPL_VECTOR_TYPE(NAME, TYPE) in a .c file 
*/

#define _VEC_TYPE(NAME) Vector##NAME##_t
#define _VEC_CLEAN_FN(NAME) Vector##NAME##ElemCleanupFn_t
#define _VEC_COPY_FN(NAME)  Vector##NAME##ElemCopyFn_t

#define DEFINE_VECTOR_TYPE(NAME, TYPE)                                                         \
typedef struct Vector##NAME {                                                                  \
    uint32_t cap;                                                                              \
    uint32_t cnt;                                                                              \
    TYPE* buf;                                                                                 \
} _VEC_TYPE(NAME);                                                                             \
\
typedef void (*_VEC_CLEAN_FN(NAME)) (TYPE* elem);                                              \
typedef TYPE (*_VEC_COPY_FN(NAME)) (const TYPE);                                               \
\
_VEC_TYPE(NAME)* createVector##NAME();                                                         \
_VEC_TYPE(NAME)* copyVector##NAME( _VEC_TYPE(NAME) *vec, _VEC_COPY_FN(NAME) copyFn);           \
void cleanupVector##NAME##Contents(_VEC_TYPE(NAME) *vec, _VEC_CLEAN_FN(NAME) cleanupFn);       \
void cleanupVector##NAME(_VEC_TYPE(NAME) **vec, _VEC_CLEAN_FN(NAME) cleanupFn);                \
\
void vector##NAME##Append(_VEC_TYPE(NAME) *vec, const TYPE elem);                              \
TYPE vector##NAME##Pop(_VEC_TYPE(NAME) * vec);                                                 \
uint32_t vector##NAME##GetCount(_VEC_TYPE(NAME) *vec);                                         \
TYPE* vector##NAME##GetBuffer(_VEC_TYPE(NAME) *vec);                                           



#define IMPL_VECTOR_TYPE(NAME, TYPE)                                                           \
_VEC_TYPE(NAME) * createVector##NAME() {                                                       \
    _VEC_TYPE(NAME) * vec = mallocChk(sizeof(_VEC_TYPE(NAME) ));                               \
    *vec = (_VEC_TYPE(NAME)) {                                                                 \
        .cap = 0u,                                                                             \
        .cnt = 0u,                                                                             \
        .buf = NULL                                                                            \
    };                                                                                         \
    return vec;                                                                                \
}                                                                                              \
\
_VEC_TYPE(NAME) * copyVector##NAME(_VEC_TYPE(NAME) * vec, _VEC_COPY_FN(NAME) copyFn) {         \
    if (!vec || !copyFn) return NULL;                                                          \
\
    _VEC_TYPE(NAME) * newVec = createVector##NAME();                                           \
    newVec->buf = mallocChk( sizeof(TYPE) * vec->cap);                                         \
    newVec->cap = vec->cap;                                                                    \
    newVec->cnt = vec->cnt;                                                                    \
\
    for (uint32_t i = 0; i < vec->cnt; i++) {                                                  \
        newVec->buf[i] = copyFn(vec->buf[i]);                                                  \
    }                                                                                          \
    return newVec;                                                                             \
}                                                                                              \
\
void cleanupVector##NAME##Contents(_VEC_TYPE(NAME) *vec, _VEC_CLEAN_FN(NAME) cleanupFn) {      \
    if (!vec) return;                                                                          \
    if (cleanupFn) {                                                                           \
        for (uint32_t i = 0; i < vec->cnt; i++) {                                              \
            cleanupFn(&vec->buf[i]);                                                           \
        }                                                                                      \
    }                                                                                          \
    memset(vec->buf, 0, vec->cnt * sizeof(void*));                                             \
    vec->cnt = 0;                                                                              \
}                                                                                              \
\
void cleanupVector##NAME(_VEC_TYPE(NAME) ** vec, _VEC_CLEAN_FN(NAME) cleanupFn) {              \
    if (!*vec ) return;                                                                        \
\
    cleanupVector##NAME##Contents(*vec, cleanupFn);                                            \
    free((*vec)->buf);                                                                         \
    (*vec)->buf = NULL;                                                                        \
\
    free(*vec);                                                                                \
    *vec = NULL;                                                                               \
}                                                                                              \
\
void vector##NAME##Append(_VEC_TYPE(NAME) * vec, const TYPE elem) {                            \
    if (vec->cnt >= vec->cap) {                                                                \
        if (vec->buf != NULL) {                                                                \
            /* no more space, reallocate */                                                    \
            vec->cap = (3u * vec->cap) / 2 + 1;                                                \
            vec->buf = realloc(vec->buf, sizeof(TYPE) * vec->cap);                             \
            if (!vec->buf) HANDLE_OOM();                                                       \
        } else {                                                                               \
            /* initial allocation */                                                           \
            vec->cap = 1;                                                                      \
            vec->buf = malloc(sizeof(TYPE) * vec->cap);                                        \
            if (!vec->buf) HANDLE_OOM();                                                       \
        }                                                                                      \
    }                                                                                          \
    memmove(&vec->buf[vec->cnt], &elem, sizeof(elem));                                         \
    vec->cnt++;                                                                                \
}                                                                                              \
\
TYPE vector##NAME##Pop(_VEC_TYPE(NAME) * vec) {                                                \
    TYPE tmp = vec->buf[vec->cnt-1];                                                             \
    vec->cnt--;                                                                                \
    return tmp;                                                                                \
}                                                                                              \
\
uint32_t vector##NAME##GetCount(_VEC_TYPE(NAME) *vec) {                                        \
    return vec->cnt;                                                                           \
}                                                                                              \
\
TYPE* vector##NAME##GetBuffer(_VEC_TYPE(NAME) *vec) {                                          \
    return vec->buf;                                                                           \
}                                                                                              


/* Export default types */
DEFINE_VECTOR_TYPE(, void*);


#endif 