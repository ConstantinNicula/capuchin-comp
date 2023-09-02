#ifndef _TEST_HELPER_H_
#define _TEST_HELPER_H_
#include <stdint.h>

typedef enum {
    EXPECT_INTEGER,
    EXPECT_BOOL, 
    EXPECT_STRING,
    EXPECT_NULL
} ExpectType_t;

typedef struct GenericExpect {
    ExpectType_t type;
    union {
        int64_t il;
        bool bl;
        const char* sl;
    };
}GenericExpect_t;

#define _BOOL(x) (GenericExpect_t){.type=EXPECT_BOOL, .bl=(x)}
#define _INT(x) (GenericExpect_t){.type=EXPECT_INTEGER, .il=(x)}
#define _STRING(x) (GenericExpect_t){.type=EXPECT_STRING, .sl=(x)}
#define _NIL (GenericExpect_t){.type=EXPECT_NULL}


#define TEST_INT(expected, actual, message)\
    TEST_ASSERT_EQUAL_INT_MESSAGE(expected, actual, message)
#define TEST_BYTES(expected, actual, numElements, message) \
    TEST_ASSERT_EQUAL_UINT8_ARRAY_MESSAGE(expected, actual, numElements, message)

#define TEST_NOT_NULL(ptr, message) \
    TEST_ASSERT_NOT_NULL_MESSAGE(ptr, message)

#define TEST_STRING(exp, actual, message) \
    TEST_ASSERT_EQUAL_STRING_MESSAGE(exp, actual, message)

#endif