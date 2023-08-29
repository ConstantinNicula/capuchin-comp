#ifndef _VECTOR_H_
#define _VECTOR_H_

#include <stdint.h>
#include <stddef.h>


typedef void (*VectorElemCleanupFn_t) (void** elem);
typedef void* (*VectorElemCopyFn_t) (void* elem);

typedef struct Vector {
    uint32_t cap;
    uint32_t cnt;
    void** buf;
} Vector_t;


Vector_t* createVector();
Vector_t* copyVector(Vector_t* vec, VectorElemCopyFn_t copyFn);
void cleanupVectorContents(Vector_t*vec, VectorElemCleanupFn_t cleanupFn);
void cleanupVector(Vector_t** vec, VectorElemCleanupFn_t cleanupFn);

void vectorAppend(Vector_t* vec, void* elem);
uint32_t vectorGetCount(Vector_t* vec);
void** vectorGetBuffer(Vector_t* vec);
#endif 