#ifndef _TEST_HELPER_H_
#define _TEST_HELPER_H_
#include <stdint.h>
#include <stdbool.h>
#include "code.h"

typedef enum {
    EXPECT_INTEGER,
    EXPECT_BOOL, 
    EXPECT_STRING,
    EXPECT_NULL, 
    EXPECT_ARRAY,
    EXPECT_HASH,
    EXPECT_COMPILED_FUNCTION, 
    EXPECT_END,
} ExpectType_t;


typedef struct GenericExpect GenericExpect_t; 

typedef struct GenericHash {
    GenericExpect_t *keys;
    GenericExpect_t *values;
} GenericHash_t;

typedef struct GenericExpect {
    ExpectType_t type;
    union {
        int64_t il;
        bool bl;
        const char* sl;
        struct GenericExpect *al;
        GenericHash_t hl;
        SliceByte_t* fl;
    };
}GenericExpect_t;


#define _BOOL(x) (GenericExpect_t){.type=EXPECT_BOOL, .bl=(x)}
#define _INT(x) (GenericExpect_t){.type=EXPECT_INTEGER, .il=(x)}
#define _STRING(x) (GenericExpect_t){.type=EXPECT_STRING, .sl=(x)}
#define _ARRAY(...) (GenericExpect_t){.type=EXPECT_ARRAY, .al=(GenericExpect_t[]){__VA_ARGS__}}
#define _HASH(k, v) (GenericExpect_t){.type=EXPECT_HASH, .hl=(GenericHash_t){\
    .keys=(GenericExpect_t[])k, .values=(GenericExpect_t[])v,}}
#define _FUNC(...) (GenericExpect_t){.type=OBJECT_COMPILED_FUNCTION, .fl=((SliceByte_t []){__VA_ARGS__})}
#define _NIL (GenericExpect_t){.type=EXPECT_NULL}
#define _END (GenericExpect_t){.type=EXPECT_END}
#define _LIT(...) __VA_ARGS__

#define TEST_INT(expected, actual, message)\
    TEST_ASSERT_EQUAL_INT_MESSAGE(expected, actual, message)
#define TEST_BYTES(expected, actual, numElements, message) \
    TEST_ASSERT_EQUAL_UINT8_ARRAY_MESSAGE(expected, actual, numElements, message)

#define TEST_NOT_NULL(ptr, message) \
    TEST_ASSERT_NOT_NULL_MESSAGE(ptr, message)

#define TEST_STRING(exp, actual, message) \
    TEST_ASSERT_EQUAL_STRING_MESSAGE(exp, actual, message)

#endif