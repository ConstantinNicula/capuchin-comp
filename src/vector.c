#include <malloc.h>
#include <string.h>
#include "vector.h"
#include "utils.h"


Vector_t* createVector() {
    Vector_t* vec = mallocChk(sizeof(Vector_t));
    vec->cap = 0u;
    vec->cnt = 0u;
    vec->buf = NULL;
    return vec;
}

Vector_t* copyVector(Vector_t* vec, VectorElemCopyFn_t copyFn) {
    if (!vec || !copyFn) 
        return NULL;

    Vector_t* newVec = createVector();
    newVec->buf = mallocChk( sizeof(void*) * vec->cap);
    newVec->cap = vec->cap;
    newVec->cnt = vec->cnt;

    for (uint32_t i = 0; i < vec->cnt; i++){
        newVec->buf[i] = copyFn(vec->buf[i]);
    }
    return newVec;
}

void cleanupVectorContents(Vector_t*vec, VectorElemCleanupFn_t cleanupFn) {
    if (!vec) 
        return;
        
    for (uint32_t i = 0; i < vec->cnt; i++)
    {
        if (cleanupFn)
            cleanupFn((void**) &vec->buf[i]);
    }
    memset(vec->buf, 0, vec->cnt * sizeof(void*));
    vec->cnt = 0;
}

void cleanupVector(Vector_t** vec, VectorElemCleanupFn_t cleanupFn) {
    if (!*vec )
        return;
    
    cleanupVectorContents(*vec, cleanupFn);
    free((*vec)->buf);
    (*vec)->buf = NULL;
    
    free(*vec);
    *vec = NULL;
}


void vectorAppend(Vector_t* vec, void* elem) {
    if (vec->cnt >= vec->cap)
    {
        if (vec->buf != NULL) {
            // no more space, reallocate
            vec->cap = (3u * vec->cap) / 2 + 1;
            vec->buf = (void*)realloc(vec->buf, sizeof(void*) * vec->cap);
            if (!vec->buf) HANDLE_OOM();
        }
        else 
        {
            // initial allocation
            vec->cap = 1;
            vec->buf = (void*)malloc(sizeof(void*) * vec->cap);
            if (!vec->buf) HANDLE_OOM();
        }
    }
    vec->buf[vec->cnt] = elem;
    vec->cnt++;
}

uint32_t vectorGetCount(Vector_t* vec) {   
    return vec->cnt;
}

void** vectorGetBuffer(Vector_t* vec) {
    return vec->buf;
}