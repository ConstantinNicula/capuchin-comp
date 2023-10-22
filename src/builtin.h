#ifndef _BUILTIN_H_
#define _BUILTIN_H_

#include "object.h"

typedef Object_t* (*BuiltinFn_t) (VectorObjects_t*);

typedef struct BuiltinFunctionDef {
    const char* name; 
    BuiltinFn_t fn; 
} BuiltinFunctionDef_t;


BuiltinFn_t getBuiltinByName(const char* name);
BuiltinFn_t getBuiltinByIndex(uint8_t index); 
BuiltinFunctionDef_t* getBuiltinDefs();

#endif