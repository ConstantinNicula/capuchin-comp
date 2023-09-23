#ifndef _OBJECT_H_
#define _OBJECT_H_


#include <stdint.h>
#include <stdbool.h>
#include "ast.h"
#include "env.h"
#include "code.h"

typedef struct Object Object_t; 
DEFINE_VECTOR_TYPE(Objects, Object_t*);

typedef enum ObjectType{
    OBJECT_INTEGER,
    OBJECT_BOOLEAN, 
    OBJECT_NULL,
    OBJECT_RETURN_VALUE,
    OBJECT_ERROR,
    OBJECT_FUNCTION,
    OBJECT_COMPILED_FUNCTION,
    OBJECT_STRING,
    OBJECT_BUILTIN,
    OBJECT_ARRAY,
    OBJECT_HASH,
    _OBJECT_TYPE_CNT
} ObjectType_t;

const char* objectTypeToString(ObjectType_t type);

#define OBJECT_BASE_ATTRS \
    ObjectType_t type;

/************************************ 
 *     GENERIC OBJECT TYPE          *
 ************************************/

typedef struct Object {
    OBJECT_BASE_ATTRS;
} Object_t;

typedef char* (*ObjectInspectFn_t) (const void*);
typedef void* (*ObjectCopyFn_t) (const void*);

Object_t* copyObject(const Object_t* obj);

char* objectInspect(const Object_t* obj);
ObjectType_t objectGetType(const Object_t* obj);
char* objectGetHashKey(const Object_t* obj);
bool objectIsHashable(const Object_t* obj); 

/************************************ 
 *     INTEGER OBJECT TYPE          *
 ************************************/

typedef struct Integer {
    OBJECT_BASE_ATTRS;
    int64_t value;
}Integer_t;

Integer_t* createInteger(int64_t value);
Integer_t* copyInteger(const Integer_t* obj);

char* integerInspect(Integer_t* obj);


/************************************ 
 *     BOOLEAN OBJECT TYPE          *
 ************************************/

typedef struct Boolean {
    OBJECT_BASE_ATTRS;
    bool value;
}Boolean_t;

Boolean_t* createBoolean(bool value);
Boolean_t* copyBoolean(const Boolean_t* obj);

char* booleanInspect(Boolean_t* obj);

/************************************ 
 *     STRING OBJECT TYPE          *
 ************************************/

typedef struct String {
    OBJECT_BASE_ATTRS;
    char* value;
}String_t;

String_t* createString(const char* value);
String_t* copyString(const String_t* obj);

char* stringInspect(String_t* obj);


/************************************ 
 *        NULL OBJECT TYPE          *
 ************************************/

typedef struct Null {
    OBJECT_BASE_ATTRS;
}Null_t;

Null_t* createNull();
Null_t* copyNull(const Null_t* obj);

char* nulllInspect(Null_t* obj);


/************************************ 
 *      RETURN OBJECT TYPE          *
 ************************************/

typedef struct ReturnValue {
    OBJECT_BASE_ATTRS;
    Object_t* value;
}ReturnValue_t;

ReturnValue_t* createReturnValue(Object_t*);
ReturnValue_t* copyReturnValue(const ReturnValue_t* obj);

char* returnValueInspect(ReturnValue_t* obj);


/************************************ 
 *      ERROR OBJECT TYPE          *
 ************************************/

typedef struct Error {
    OBJECT_BASE_ATTRS;
    char* message;
}Error_t;

Error_t* createError(char* message);
Error_t* copyError(const Error_t* obj);

char* errorInspect(Error_t* obj);

/************************************ 
 *    FUNCTION OBJECT TYPE          *
 ************************************/
typedef struct Environment Environment_t;

typedef struct Function {
    OBJECT_BASE_ATTRS;
    VectorExpressions_t* parameters;
    BlockStatement_t* body;
    Environment_t* environment;
} Function_t;

Function_t* createFunction(VectorExpressions_t* params, BlockStatement_t* body, Environment_t* env);
Function_t* copyFunction(const Function_t* obj);

char* functionInspect(Function_t* obj);
uint32_t functionGetParameterCount(Function_t* obj);
Identifier_t** functionGetParameters(Function_t* obj);

/************************************ 
 *  COMP FUNCTION OBJECT TYPE       *
 ************************************/

typedef struct CompiledFunction {
    OBJECT_BASE_ATTRS;
    Instructions_t instructions;
} CompiledFunction_t;

CompiledFunction_t* createCompiledFunction(Instructions_t instr);
CompiledFunction_t* copyCompiledFunction(const CompiledFunction_t* obj);

char* compiledFunctionInspect(CompiledFunction_t* obj);

/************************************ 
 *       ARRAY OBJECT TYPE          *
 ************************************/

typedef struct Array {
    OBJECT_BASE_ATTRS;
    VectorObjects_t* elements;
}Array_t;

Array_t* createArray();
Array_t* copyArray(const Array_t* obj);

char* arrayInspect(Array_t* obj);
uint32_t arrayGetElementCount(Array_t* obj);
Object_t** arrayGetElements(Array_t* obj);
void arrayAppend(Array_t* arr, Object_t* obj);

/************************************ 
 *        HASH OBJECT TYPE          *
 ************************************/

typedef struct HashPair {
    Object_t* key;
    Object_t* value;
} HashPair_t;

HashPair_t* createHashPair(Object_t* key, Object_t* value);

typedef struct Hash {
    OBJECT_BASE_ATTRS;
    HashMap_t* pairs;
} Hash_t;

Hash_t* createHash();
Hash_t* copyHash(const Hash_t* obj);

char* hashInspect(Hash_t* obj);
void hashInsertPair(Hash_t* obj, HashPair_t* pair);
HashPair_t* hashGetPair(Hash_t* obj, Object_t* key);


/************************************ 
 *     BULITIN OBJECT TYPE          *
 ************************************/

typedef Object_t* (*BuiltinFunction_t) (VectorObjects_t*);

typedef struct Builtin {
    OBJECT_BASE_ATTRS;
    BuiltinFunction_t func;
} Builtin_t;

Builtin_t* createBuiltin(BuiltinFunction_t func);
Builtin_t* copyBuiltin(const Builtin_t* obj);

char* builtinInspect(Builtin_t* obj);


#endif 