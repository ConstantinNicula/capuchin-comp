#include "unity.h"
#include "hmap.h"
#include "utils.h"

void setUp(void) {
    // set stuff up here
}

void tearDown(void) {
    // clean stuff up here
}

static void cleanupStr(char** str){
    if (!(*str)) return;
    free(*str);
    *str = NULL;
}

static char* copyStr(char* str) {
    return cloneString(str);
}

void hashMapTestBasic() {
    HashMap_t* map = createHashMap();
    cleanupHashMap(&map, (HashMapElemCleanupFn_t)cleanupStr);    
}

void hashMapTestInsert() {
    HashMap_t* map = createHashMap();

    hashMapInsert(map, "hello", cloneString("my value"));
    hashMapInsert(map, "test1", cloneString("my value 1"));
    hashMapInsert(map, "test1", cloneString("my value 4"));
    hashMapInsert(map, "test2", cloneString("my value 2"));   
    hashMapInsert(map, "test3", cloneString("my value 3"));

    TEST_ASSERT_EQUAL_INT_MESSAGE(4, map->itemCnt, "Wrong item count");  
    TEST_ASSERT_EQUAL_INT_MESSAGE(8, map->numBuckets, "Wrong number of buckets");  
    TEST_ASSERT_EQUAL_STRING("my value", hashMapGet(map, "hello")); 
    TEST_ASSERT_EQUAL_STRING("my value 4", hashMapGet(map, "test1")); 
    TEST_ASSERT_EQUAL_STRING("my value 2", hashMapGet(map, "test2")); 
    TEST_ASSERT_EQUAL_STRING("my value 3", hashMapGet(map, "test3")); 

    HashMapIter_t iter = createHashMapIter(map);
    HashMapEntry_t* entry = hashMapIterGetNext(map, &iter);
    while(entry) {

        TEST_MESSAGE(entry->key);
        entry = hashMapIterGetNext(map, &iter);
    }

    HashMap_t* mapCopy = copyHashMap(map, (HashMapElemCopyFn_t)copyStr);
    hashMapInsert(mapCopy, "test7", cloneString("my value 2"));   
    hashMapInsert(mapCopy, "test5", cloneString("my value 3"));
    hashMapInsert(mapCopy, "x", cloneString("my value 3"));
    hashMapInsert(mapCopy, "test7", cloneString("my value 3"));


    cleanupHashMap(&map, (HashMapElemCleanupFn_t)cleanupStr);
    cleanupHashMap(&mapCopy,(HashMapElemCleanupFn_t)cleanupStr);
}

void hashMapTestSetInsert() {

    HashMap_t* map = createHashMap();
    hashMapInsert(map, "hello", NULL);
    hashMapInsert(map, "world", NULL);
    cleanupHashMap(&map, NULL);
}

// not needed when using generate_test_runner.rb
int main(void) {
   UNITY_BEGIN();
   RUN_TEST(hashMapTestBasic);
   RUN_TEST(hashMapTestInsert);
   RUN_TEST(hashMapTestSetInsert);
   return UNITY_END();
}