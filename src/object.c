#include <stdlib.h>
#include <assert.h>

#include "object.h"
#include "utils.h"
#include "sbuf.h"
#include "gc.h"

IMPL_VECTOR_TYPE(Objects, Object_t*);

const char* tokenTypeStrings[_OBJECT_TYPE_CNT] = {
    [OBJECT_INTEGER]="INTEGER",
    [OBJECT_BOOLEAN]="BOOLEAN", 
    [OBJECT_STRING]="STRING",
    [OBJECT_NULL]="NULL",
    [OBJECT_CLOSURE]="CLOSURE",
    [OBJECT_COMPILED_FUNCTION]="COMPILED_FUNCTION",
    [OBJECT_ERROR]="ERROR",
    [OBJECT_BUILTIN]="BUILTIN",
    [OBJECT_ARRAY]="ARRAY",
    [OBJECT_HASH]="HASH",
    [OBJECT_RETURN_VALUE]="RETURN_VALUE"
};

const char* objectTypeToString(ObjectType_t type) {
    if (0 <= type && type <= _OBJECT_TYPE_CNT) {
        return tokenTypeStrings[type];
    }
    return "";
}


/************************************ 
 *     GENERIC OBJECT TYPE          *
 ************************************/

static ObjectInspectFn_t objectInsepctFns[_OBJECT_TYPE_CNT] = {
    [OBJECT_INTEGER]=(ObjectInspectFn_t)integerInspect,
    [OBJECT_BOOLEAN]=(ObjectInspectFn_t)booleanInspect,
    [OBJECT_STRING]=(ObjectInspectFn_t)stringInspect,
    [OBJECT_NULL]=(ObjectInspectFn_t)nulllInspect,
    [OBJECT_RETURN_VALUE]=(ObjectInspectFn_t)returnValueInspect,
    [OBJECT_ERROR]=(ObjectInspectFn_t)errorInspect,
    [OBJECT_CLOSURE]=(ObjectInspectFn_t)closureInspect,
    [OBJECT_COMPILED_FUNCTION]=(ObjectInspectFn_t)compiledFunctionInspect,
    [OBJECT_BUILTIN]=(ObjectInspectFn_t)builtinInspect,
    [OBJECT_ARRAY]=(ObjectInspectFn_t)arrayInspect,
    [OBJECT_HASH]=(ObjectInspectFn_t)hashInspect
};

static ObjectCopyFn_t objectCopyFns[_OBJECT_TYPE_CNT] = {
    [OBJECT_INTEGER]=(ObjectCopyFn_t)copyInteger,
    [OBJECT_BOOLEAN]=(ObjectCopyFn_t)copyBoolean,
    [OBJECT_STRING]=(ObjectCopyFn_t)copyString,
    [OBJECT_NULL]=(ObjectCopyFn_t)copyNull,
    [OBJECT_RETURN_VALUE]=(ObjectCopyFn_t)copyReturnValue,
    [OBJECT_ERROR]=(ObjectCopyFn_t)copyError,
    [OBJECT_CLOSURE]=(ObjectCopyFn_t)copyClosure,
    [OBJECT_COMPILED_FUNCTION]=(ObjectCopyFn_t)copyCompiledFunction,
    [OBJECT_BUILTIN]=(ObjectCopyFn_t)copyBuiltin,
    [OBJECT_ARRAY]=(ObjectCopyFn_t)copyArray,
    [OBJECT_HASH]=(ObjectCopyFn_t)copyHash,
};


Object_t* copyObject(const Object_t* obj) {
    if (obj && 0 <= obj->type && obj->type < _OBJECT_TYPE_CNT) {
        ObjectCopyFn_t copyFn = objectCopyFns[obj->type];
        if (!copyFn) return (Object_t*)createNull();
        return copyFn(obj);
    }
    return (Object_t*)createNull();
}


char* objectInspect(const Object_t* obj) {
    if (obj && 0 <= obj->type && obj->type < _OBJECT_TYPE_CNT) {
        ObjectInspectFn_t inspectFn = objectInsepctFns[obj->type];
        if (!inspectFn) return cloneString(""); 
        return inspectFn(obj);
    }
    return cloneString(""); 
}


ObjectType_t objectGetType(const Object_t* obj) {
    return obj->type;
}

bool objectIsHashable(const Object_t* obj) {
    switch(obj->type) {
        case OBJECT_BOOLEAN:
        case OBJECT_STRING:
        case OBJECT_INTEGER:
            return true;
        default:
            return false;
    }
}

char* objectGetHashKey(const Object_t* obj) {
   return objectIsHashable(obj) ? objectInspect(obj) : NULL;
}

void gcCleanupObject(Object_t** obj);
void gcMarkObject(Object_t* obj);


/************************************ 
 *     INTEGER OBJECT TYPE          *
 ************************************/

Integer_t* createInteger(int64_t value) {
    Integer_t* obj = gcMalloc(sizeof(Integer_t));
    
    *obj = (Integer_t){
        .type = OBJECT_INTEGER,
        .value = value
    };

    return obj;
}

Integer_t* copyInteger(const Integer_t* obj) {
    return createInteger(obj->value);
}

char* integerInspect(Integer_t* obj) {
    return strFormat("%d", obj->value);
}

void gcCleanupInteger(Integer_t** obj) {
    if (!(*obj)) return;
    gcFree(*obj);
    *obj = NULL; 
}

void gcMarkInteger(Integer_t* obj) {
    // no objects owned by gc
}


/************************************ 
 *     BOOLEAN OBJECT TYPE          *
 ************************************/

Boolean_t* createBoolean(bool value) {
    Boolean_t* ret = gcMalloc(sizeof(Boolean_t));
    *ret = (Boolean_t) {
        .type = OBJECT_BOOLEAN,
        .value = value
    };
    return ret;
}

Boolean_t* copyBoolean(const Boolean_t* obj) {
    return createBoolean(obj->value);
}

char* booleanInspect(Boolean_t* obj) {
    return obj->value ? cloneString("true") : cloneString("false");
}

void gcCleanupBoolean(Boolean_t** obj) {
    if (!(*obj)) return;
    gcFree(*obj);
    *obj = NULL; 
}

void gcMarkBoolean(Boolean_t* obj) {
    // no objects owned by gc
}


/************************************ 
 *     STRING OBJECT TYPE          *
 ************************************/

String_t* createString(const char* value) {
    String_t* ret = gcMalloc(sizeof(String_t));
    *ret = (String_t) {
        .type = OBJECT_STRING,
        .value = cloneString(value)
    };
    return ret;
}

String_t* copyString(const String_t* obj) {
    return createString(obj->value);
}

char* stringInspect(String_t* obj) {
    return cloneString(obj->value);
}

void gcCleanupString(String_t** obj) {
    if (!(*obj)) return;
    free((*obj)->value);
    gcFree(*obj);
    *obj = NULL; 
}

void gcMarkString(Boolean_t* obj) {
    // no objects owned by gc
}
/************************************ 
 *        NULL OBJECT TYPE          *
 ************************************/
Null_t* createNull() {
    Null_t* ret = gcMalloc(sizeof(Null_t));
    *ret = (Null_t) {.type = OBJECT_NULL};
    return ret;
}

Null_t* copyNull(const Null_t* obj) {
    return createNull();
}

char* nulllInspect(Null_t* obj) {
    return cloneString("null");
}


void gcCleanupNull(Null_t** obj) {
    if (!(*obj)) return;
    gcFree(*obj);
    *obj = NULL;    
}

void gcMarkNull(Null_t* obj) {
    // no objects owned by gc
}
/************************************ 
 *      RETURN OBJECT TYPE          *
 ************************************/

ReturnValue_t* createReturnValue(Object_t* value) {
    ReturnValue_t* ret = gcMalloc(sizeof(ReturnValue_t));
    *ret= (ReturnValue_t) {
        .type = OBJECT_RETURN_VALUE, 
        .value = value
    };

    return ret;
}

ReturnValue_t* copyReturnValue(const ReturnValue_t* obj) {
    return createReturnValue(obj->value);
}

char* returnValueInspect(ReturnValue_t* obj) {
    return objectInspect(obj->value);
}

void gcCleanupReturnValue(ReturnValue_t** obj) {
    if (!(*obj)) return;
    // Note: intentionally does not free inner obj. 
    gcFree(*obj);
    *obj = NULL;    
}

void gcMarkReturnValue(ReturnValue_t* obj) {
    gcMarkObject(obj->value);
}

/************************************ 
 *      ERROR OBJECT TYPE          *
 ************************************/

Error_t* createError(char* message) {
    Error_t* err = gcMalloc(sizeof(Error_t));

    *err = (Error_t) {
        .type = OBJECT_ERROR,
        .message = message
    };

    return err;
}

Error_t* copyError(const Error_t* obj) {
    return createError(cloneString(obj->message));
}

char* errorInspect(Error_t* err) {
    return strFormat("ERROR: %s", err->message);
}

void gcCleanupError(Error_t** err) {
    if (!(*err))
        return;
    free((*err)->message);
    gcFree(*err);
    *err = NULL;
}

void gcMarkError(Error_t* err) {
    // no objects owned by gc
}


/************************************ 
 *  COMP FUNCTION OBJECT TYPE       *
 ************************************/
CompiledFunction_t* createCompiledFunction(Instructions_t instr, uint32_t numLocals, uint32_t numParameters) {
    CompiledFunction_t* obj = gcMalloc(sizeof(CompiledFunction_t));
    *obj = (CompiledFunction_t) {
        .type = OBJECT_COMPILED_FUNCTION,
        .instructions = instr,
        .numLocals = numLocals,
        .numParameters = numParameters
    };
    return obj;
}

void gcCleanupCompiledFunction(CompiledFunction_t** obj) {
    if(!(*obj)) return;
    
    // cleanup owned attr.
    cleanupSliceByte((*obj)->instructions);
   
    gcFree(*obj);
    *obj = NULL;
}
void gcMarkCompiledFunction(CompiledFunction_t* obj) { 
    // nothing to mark
}

CompiledFunction_t* copyCompiledFunction(const CompiledFunction_t* obj) {
    return createCompiledFunction(copySliceByte(obj->instructions), obj->numLocals, obj->numParameters);
}

char* compiledFunctionInspect(CompiledFunction_t* obj) {
    return strFormat("CompileFunction[%p]", obj);
}

/************************************ 
 *      CLOSURE OBJECT TYPE         *
 ************************************/

Closure_t* createClosure(CompiledFunction_t *fn) {
    Closure_t* obj = gcMalloc(sizeof(Closure_t));
    *obj = (Closure_t) {
        .type = OBJECT_CLOSURE,
        .fn = fn,
        .free = createVectorObjects()
    };
    return obj;
}

Closure_t* copyClosure(const Closure_t* obj) {
    Closure_t* newObj = gcMalloc(sizeof(Closure_t)); 
    *newObj = (Closure_t) {
        .type = OBJECT_CLOSURE,
        .fn = copyCompiledFunction(obj->fn),
        .free = copyVectorObjects(obj->free, copyObject),
    };
    return newObj;    
}

void gcCleanupClosure(Closure_t** obj) {
    if(!(*obj)) return;

    gcCleanupObject((Object_t**)&(*obj)->fn);
    cleanupVectorObjects(&(*obj)->free, gcCleanupObject); 

    gcFree(*obj);
    *obj = NULL;
}
void gcMarkClosure(Closure_t* obj) { 
    gcMarkObject((Object_t*)obj->fn);
    uint32_t freeCnt = vectorObjectsGetCount(obj->free);
    Object_t** freeObjs = vectorObjectsGetBuffer(obj->free);
    for (uint32_t i = 0; i < freeCnt; i++) {
        gcMarkObject(freeObjs[i]);
    }
}

char* closureInspect(Closure_t* obj) {
    return strFormat("Closure[%p]", obj);
}


/************************************ 
 *       ARRAY OBJECT TYPE          *
 ************************************/

Array_t* createArray() {
    Array_t* arr = gcMalloc(sizeof(Array_t));
    *arr = (Array_t) {
        .type = OBJECT_ARRAY,
        .elements = NULL
    };
    return arr;
}

Array_t* copyArray(const Array_t* obj) {
    Array_t* newArr = createArray();
    newArr->elements = copyVectorObjects(obj->elements, copyObject);
    return newArr;
}

char* arrayInspect(Array_t* obj) {
    Strbuf_t* sbuf = createStrbuf();
    
    strbufWrite(sbuf, "[");
    uint32_t cnt = arrayGetElementCount(obj);
    Object_t** elems = arrayGetElements(obj);
    for (uint32_t i = 0; i < cnt; i++) {
        strbufConsume(sbuf, objectInspect(elems[i]));
        if (i != (cnt - 1)) {
            strbufWrite(sbuf, ", ");
        }
    }
    strbufWrite(sbuf, "]");
    return detachStrbuf(&sbuf);
}

uint32_t arrayGetElementCount(Array_t* obj) {
    return vectorObjectsGetCount(obj->elements);
}

Object_t** arrayGetElements(Array_t* obj) {
    return vectorObjectsGetBuffer(obj->elements);
}

void arrayAppend(Array_t* arr, Object_t* obj) {
    vectorObjectsAppend(arr->elements, (void*) obj);
}

void gcCleanupArray(Array_t** arr) {
    if (!(*arr)) return;
    cleanupVectorObjects(&(*arr)->elements, NULL);
    gcFree(*arr);
    *arr = NULL;
}

void gcMarkArray(Array_t* arr) {
    uint32_t cnt = arrayGetElementCount(arr);
    Object_t** elems = arrayGetElements(arr);
    for (uint32_t i = 0; i < cnt; i++) {
        gcSetRef(elems[i], GC_REF_INTERNAL);
    } 
}

/************************************ 
 *        HASH OBJECT TYPE          *
 ************************************/

HashPair_t* createHashPair(Object_t* key, Object_t* value) {
    HashPair_t* pair = mallocChk(sizeof(HashPair_t));
    *pair = (HashPair_t) {
        .key = key,
        .value = value
    };
    return pair;
}

void cleanupHashPair(HashPair_t** pair) {
    if (!(*pair)) return;
    free(*pair);
    *pair = NULL;
}

Hash_t* createHash() {
    Hash_t* hash = gcMalloc(sizeof(Hash_t));
    *hash = (Hash_t) {
        .type = OBJECT_HASH,
        .pairs = createHashMap()
    };
    return hash;
}

Hash_t* copyHash(const Hash_t* obj) {
    Hash_t* newHash = gcMalloc(sizeof(Hash_t));
    *newHash = (Hash_t) {
        .type = OBJECT_HASH,
        .pairs = copyHashMap(obj->pairs, (HashMapElemCopyFn_t) copyObject)
    };
    return newHash;
}

char* hashInspect(Hash_t* obj) {
    Strbuf_t* sbuf = createStrbuf();
    strbufWrite(sbuf, "{");
    
    HashMapIter_t iter = createHashMapIter(obj->pairs);
    HashMapEntry_t* entry = hashMapIterGetNext(obj->pairs, &iter);
    while(entry) {
        HashPair_t* pair = (HashPair_t*)entry->value; 
        strbufConsume(sbuf, objectInspect(pair->key));
        strbufWrite(sbuf, ":");
        strbufConsume(sbuf, objectInspect(pair->value));

        entry = hashMapIterGetNext(obj->pairs, &iter);
        if (entry)
            strbufWrite(sbuf, ", ");
    }

    strbufWrite(sbuf, "}");
    return detachStrbuf(&sbuf);
}

void hashInsertPair(Hash_t* obj, HashPair_t* pair) {
    char* hashKey = objectGetHashKey(pair->key);
    hashMapInsert(obj->pairs, hashKey, pair);
    free(hashKey);
}

HashPair_t* hashGetPair(Hash_t* obj, Object_t* key) {
    char* hashKey = objectGetHashKey(key); 
    HashPair_t* ret = (HashPair_t*)hashMapGet(obj->pairs, hashKey);
    free(hashKey);
    return ret;
}

void gcCleanupHash(Hash_t** obj) {
    if(!(*obj)) return;
    cleanupHashMap(&(*obj)->pairs, (HashMapElemCleanupFn_t) cleanupHashPair);
    gcFree(*obj);
    *obj = NULL; 
}

void gcMarkHash(Hash_t* obj) {
    HashMapIter_t iter = createHashMapIter(obj->pairs);
    HashMapEntry_t* entry = hashMapIterGetNext(obj->pairs, &iter);
    while(entry){
        HashPair_t* pair = (HashPair_t*)entry->value; 
        gcMarkObject(pair->key); 
        gcMarkObject(pair->value);
        entry = hashMapIterGetNext(obj->pairs, &iter);
    }
}

/************************************ 
 *     BUILTIN OBJECT TYPE          *
 ************************************/

Builtin_t* createBuiltin(BuiltinFunction_t func) {
    Builtin_t* builtin = gcMalloc(sizeof(Builtin_t));
    *builtin = (Builtin_t) {
        .type = OBJECT_BUILTIN, 
        .func = func
    };
    return builtin;
}

Builtin_t* copyBuiltin(const Builtin_t* obj) {
    return createBuiltin(obj->func);
}

char* builtinInspect(Builtin_t* obj) {
    return cloneString("builtin function");
}

void gcCleanupBuiltin(Builtin_t** obj) {
    if(!(*obj)) return;
    gcFree(*obj);
    *obj = NULL;
}

void gcMarkBuiltin(Builtin_t *obj) {
    // no internal objects to mark 
}

/************************************ 
 *      GARBAGE COLLECTION          *
 ************************************/

typedef void (*ObjectCleanupFn_t) (void**);
typedef void (*ObjectGcMarkFn_t) (void*);

static ObjectCleanupFn_t objectCleanupFns[_OBJECT_TYPE_CNT] = {
    [OBJECT_INTEGER]=(ObjectCleanupFn_t)gcCleanupInteger,
    [OBJECT_BOOLEAN]=(ObjectCleanupFn_t)gcCleanupBoolean,
    [OBJECT_STRING]=(ObjectCleanupFn_t)gcCleanupString,
    [OBJECT_NULL]=(ObjectCleanupFn_t)gcCleanupNull,
    [OBJECT_RETURN_VALUE]=(ObjectCleanupFn_t)gcCleanupReturnValue,
    [OBJECT_ERROR]=(ObjectCleanupFn_t)gcCleanupError,
    [OBJECT_CLOSURE]=(ObjectCleanupFn_t)gcCleanupClosure,
    [OBJECT_COMPILED_FUNCTION]=(ObjectCleanupFn_t)gcCleanupCompiledFunction,
    [OBJECT_BUILTIN]=(ObjectCleanupFn_t)gcCleanupBuiltin,
    [OBJECT_ARRAY]=(ObjectCleanupFn_t)gcCleanupArray,
    [OBJECT_HASH]=(ObjectCleanupFn_t)gcCleanupHash,
};

static ObjectGcMarkFn_t objectMarkFns[_OBJECT_TYPE_CNT] = {
    [OBJECT_INTEGER]=(ObjectGcMarkFn_t)gcMarkInteger,
    [OBJECT_BOOLEAN]=(ObjectGcMarkFn_t)gcMarkBoolean,
    [OBJECT_STRING]=(ObjectGcMarkFn_t)gcMarkString,
    [OBJECT_NULL]=(ObjectGcMarkFn_t)gcMarkNull,
    [OBJECT_RETURN_VALUE]=(ObjectGcMarkFn_t)gcMarkReturnValue,
    [OBJECT_ERROR]=(ObjectGcMarkFn_t)gcMarkError,
    [OBJECT_CLOSURE]=(ObjectGcMarkFn_t)gcMarkClosure,
    [OBJECT_COMPILED_FUNCTION]=(ObjectGcMarkFn_t)gcMarkCompiledFunction,
    [OBJECT_BUILTIN]=(ObjectGcMarkFn_t)gcMarkBuiltin,
    [OBJECT_ARRAY]=(ObjectGcMarkFn_t)gcMarkArray,
    [OBJECT_HASH]=(ObjectGcMarkFn_t)gcMarkHash,
};


void gcCleanupObject(Object_t** obj) {
    if (!(*obj)) return;
    if (0 <= (*obj)->type && (*obj)->type < _OBJECT_TYPE_CNT) {
        ObjectCleanupFn_t cleanupFn = objectCleanupFns[(*obj)->type];
        if (!cleanupFn) return;
        cleanupFn((void**)obj);
    }
}

void gcMarkObject(Object_t* obj) {
    if (obj && 0 <= obj->type && obj->type < _OBJECT_TYPE_CNT) {
        ObjectGcMarkFn_t markFn = objectMarkFns[obj->type];
        if ( (!markFn) || (gcHasRef(obj, GC_REF_INTERNAL))) return;
        gcSetRef(obj, GC_REF_INTERNAL);
        markFn(obj);
    }   
}
