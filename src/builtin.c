#include <string.h>
#include "builtin.h"
#include "sbuf.h"
#include "utils.h"

Object_t* lenBuiltin(VectorObjects_t* args);
Object_t* firstBuiltin(VectorObjects_t* args);
Object_t* lastBuiltin(VectorObjects_t* args);
Object_t* restBuiltin(VectorObjects_t* args);
Object_t* pushBuiltin(VectorObjects_t* args);
Object_t* putsBuiltin(VectorObjects_t* args);
Object_t* printfBuiltin(VectorObjects_t* args);

static BuiltinFunctionDef_t builtinDefs[] = {
    {"len", lenBuiltin},
    {"puts", putsBuiltin},
    {"first", firstBuiltin},
    {"last", lastBuiltin},
    {"rest", restBuiltin},
    {"push", pushBuiltin},
    {"printf", printfBuiltin},
    {NULL, NULL}
};

BuiltinFn_t getBuiltinByName(const char* name) {
    uint32_t i = 0; 
    while (builtinDefs[i].name) {
        if (strcmp(name, builtinDefs[i].name) == 0) {
            return  builtinDefs[i].fn;
        }
        i++;
    }
    return NULL;
}

BuiltinFn_t getBuiltinByIndex(uint8_t index) {
    uint8_t numBuiltins = sizeof(builtinDefs) / sizeof(builtinDefs[0]);
    if (index >= numBuiltins) return NULL;
    return builtinDefs[index].fn;
}

BuiltinFunctionDef_t* getBuiltinDefs() {
    return builtinDefs;
}

Object_t* lenBuiltin(VectorObjects_t* args) {
    if (vectorObjectsGetCount(args) != 1) {
        char* err = strFormat("wrong number of arguments. got=%d, want=1", 
                                vectorObjectsGetCount(args));
        return (Object_t*)createError(err); 
    }
    
    Object_t** argBuf = vectorObjectsGetBuffer(args);
    switch(argBuf[0]->type) {
        case OBJECT_ARRAY: 
            return (Object_t*)createInteger(arrayGetElementCount((Array_t*)argBuf[0]));
        case OBJECT_STRING:
            return (Object_t*)createInteger(strlen(((String_t*)argBuf[0])->value));
        default:
            char* err = strFormat("argument to `len` not supported, got %s", 
                                    objectTypeToString(argBuf[0]->type));
            return (Object_t*)createError(err);
    }
    return (Object_t*)createNull();
}

Object_t* firstBuiltin(VectorObjects_t* args) {
    if (vectorObjectsGetCount(args) != 1) {
        char* err = strFormat("wrong number of arguments. got=%d, want=1", 
                                vectorObjectsGetCount(args));
        return (Object_t*)createError(err); 
    }
    
    Object_t** argBuf = vectorObjectsGetBuffer(args);
    if (argBuf[0]->type != OBJECT_ARRAY) {
        char* err = strFormat("argument to `first` must be ARRAY, got %s", 
                                objectTypeToString(argBuf[0]->type));
        return (Object_t*)createError(err);                      
    }

    Array_t* arr = (Array_t*)argBuf[0];
    if (arrayGetElementCount(arr) > 0) {
        return arrayGetElements(arr)[0];
    }

    return (Object_t*) createNull();
}

Object_t* lastBuiltin(VectorObjects_t* args) {
    if (vectorObjectsGetCount(args) != 1) {
        char* err = strFormat("wrong number of arguments. got=%d, want=1", 
                                vectorObjectsGetCount(args));
        return (Object_t*)createError(err); 
    }
    
    Object_t** argBuf = (Object_t**)vectorObjectsGetBuffer(args);
    if (argBuf[0]->type != OBJECT_ARRAY) {
        char* err = strFormat("argument to `last` must be ARRAY, got %s", 
                                objectTypeToString(argBuf[0]->type));
        return (Object_t*)createError(err);                      
    }

    Array_t* arr = (Array_t*)argBuf[0];
    uint32_t len = arrayGetElementCount(arr); 
    if (len > 0) {
        return arrayGetElements(arr)[len - 1];
    }

    return (Object_t*) createNull();
}


Object_t* restBuiltin(VectorObjects_t* args) {
    if (vectorObjectsGetCount(args) != 1) {
        char* err = strFormat("wrong number of arguments. got=%d, want=1", 
                                vectorObjectsGetCount(args));
        return (Object_t*)createError(err); 
    }
    
    Object_t** argBuf = vectorObjectsGetBuffer(args);
    if (argBuf[0]->type != OBJECT_ARRAY) {
        char* err = strFormat("argument to `rest` must be ARRAY, got %s", 
                                objectTypeToString(argBuf[0]->type));
        return (Object_t*)createError(err);                      
    }

    Array_t* arr = (Array_t*)argBuf[0];
    uint32_t len = arrayGetElementCount(arr); 
    Object_t** elems = arrayGetElements(arr);
    if (len > 0) {
        VectorObjects_t* newElements = createVectorObjects();
        for (uint32_t i = 1; i < len; i++) {
            vectorObjectsAppend(newElements, copyObject(elems[i]));
        } 
        Array_t* newArr = createArray();
        newArr->elements = newElements;
        return (Object_t*)newArr;
    }

    return (Object_t*) createNull();
}

Object_t* pushBuiltin(VectorObjects_t* args) {
    if (vectorObjectsGetCount(args) != 2) {
        char* err = strFormat("wrong number of arguments. got=%d, want=2", 
                                vectorObjectsGetCount(args));
        return (Object_t*)createError(err); 
    }
    
    Object_t** argBuf = vectorObjectsGetBuffer(args);
    if (argBuf[0]->type != OBJECT_ARRAY) {
        char* err = strFormat("argument to `push` must be ARRAY, got %s", 
                                objectTypeToString(argBuf[0]->type));
        return (Object_t*)createError(err);                      
    }

    Array_t* arr = (Array_t*)argBuf[0];
    Object_t* obj = (Object_t*)argBuf[1];

    uint32_t len = arrayGetElementCount(arr); 
    Object_t** elems = arrayGetElements(arr);
    
    VectorObjects_t* newElements = createVectorObjects();
    for (uint32_t i = 0; i < len; i++) {
        vectorObjectsAppend(newElements, copyObject(elems[i]));
    } 
    // add new element 
    vectorObjectsAppend(newElements, obj);
    Array_t* newArr = createArray();
    newArr->elements = newElements;
    return (Object_t*)newArr;

}

Object_t* putsBuiltin(VectorObjects_t* args) {
    uint32_t argCnt = vectorObjectsGetCount(args);
    Object_t** argBuf = (Object_t**)vectorObjectsGetBuffer(args);
    for(uint32_t i = 0; i < argCnt; i++) {
        char* inspectStr = objectInspect(argBuf[i]);
        puts(inspectStr);
        free(inspectStr);
    }

    return (Object_t*) createNull();
}


bool isDigit(char c) {
    return '0' <= c && c <= '9';
}

bool readInteger(const char* format, uint32_t* pos, int64_t* out) {
    if (!strToInteger(&format[*pos], out)) 
        return false;
    while (isDigit(format[*pos+1])) 
        (*pos)++; 
    return true;
}

char getEscapeChar(const char* format, uint32_t* pos) {
    switch(format[*pos]) {
        case 'n': 
            return '\n';
        case 't':
            return '\t';
        case 'r': 
            return '\r';
        default:
            int64_t escChar;
            if (readInteger(format, pos, &escChar)) {
                return escChar;
            }
            return format[*pos];
    }
}

static char* formatPrint(const char* format, uint32_t argsCount, char**args) {
    Strbuf_t* sbuf = createStrbuf(); 
    uint32_t pos = 0;
    uint32_t len = strlen(format);

    while (pos < len) {
        switch(format[pos]) {
            case '\\': 
                pos++;
                strbufWriteChar(sbuf, getEscapeChar(format, &pos));
                break;
            case '{':
                pos++;
                int64_t argIdx = 0;
                if (!readInteger(format, &pos, &argIdx)) {
                    cleanupStrbuf(&sbuf);
                    return NULL;         
                }
                if (argIdx < 0 || argIdx > argsCount) {
                    cleanupStrbuf(&sbuf);
                    return NULL;
                }
                
                strbufWrite(sbuf,args[argIdx]);
                pos++; 
                break;
            default:
                strbufWriteChar(sbuf, format[pos]);
        }
        pos++;
    }
    return detachStrbuf(&sbuf);
}

Object_t* printfBuiltin(VectorObjects_t* args) {
    Object_t* retValue = (Object_t*) createNull();
    uint32_t argCnt = vectorObjectsGetCount(args);
    Object_t** argBuf = (Object_t**)vectorObjectsGetBuffer(args);

    char* format = objectInspect(argBuf[0]);
    char** argStrBuf = malloc(sizeof(char*) * (argCnt - 1));
    for(uint32_t i = 1; i < argCnt; i++) {
        argStrBuf[i - 1] = objectInspect(argBuf[i]);
    }

    char* output = formatPrint(format, argCnt-1, argStrBuf);
    if(output) {
        fputs(output, stdout);
        free(output);
    } else {
        char* err = strFormat("invalid format string: %s", format);
        retValue = (Object_t*) createError(err);
    }

    for (uint32_t i = 0 ; i < argCnt-1; i++) {
        free(argStrBuf[i]);
    }
    free(argStrBuf);
    free(format);

    return retValue;
}
