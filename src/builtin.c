#include <string.h>
#include "builtin.h"
#include "sbuf.h"
#include "utils.h"


Object_t* lenBuiltin(Vector_t* args);
Object_t* firstBuiltin(Vector_t* args);
Object_t* lastBuiltin(Vector_t* args);
Object_t* restBuiltin(Vector_t* args);
Object_t* pushBuiltin(Vector_t* args);
Object_t* putsBuiltin(Vector_t* args);
Object_t* printfBuiltin(Vector_t* args);


void registerBuiltinFunctions(Environment_t* env) {
    environmentSet(env, "len", (Object_t*)createBuiltin(lenBuiltin));    
    environmentSet(env, "first", (Object_t*)createBuiltin(firstBuiltin));    
    environmentSet(env, "last", (Object_t*)createBuiltin(lastBuiltin));    
    environmentSet(env, "rest", (Object_t*)createBuiltin(restBuiltin));    
    environmentSet(env, "push", (Object_t*)createBuiltin(pushBuiltin));    
    environmentSet(env, "puts", (Object_t*)createBuiltin(putsBuiltin));    
    environmentSet(env, "printf", (Object_t*)createBuiltin(printfBuiltin));    
}

Object_t* lenBuiltin(Vector_t* args) {
    if (vectorGetCount(args) != 1) {
        char* err = strFormat("wrong number of arguments. got=%d, want=1", 
                                vectorGetCount(args));
        return (Object_t*)createError(err); 
    }
    
    Object_t** argBuf = (Object_t**)vectorGetBuffer(args);
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

Object_t* firstBuiltin(Vector_t* args) {
    if (vectorGetCount(args) != 1) {
        char* err = strFormat("wrong number of arguments. got=%d, want=1", 
                                vectorGetCount(args));
        return (Object_t*)createError(err); 
    }
    
    Object_t** argBuf = (Object_t**)vectorGetBuffer(args);
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

Object_t* lastBuiltin(Vector_t* args) {
    if (vectorGetCount(args) != 1) {
        char* err = strFormat("wrong number of arguments. got=%d, want=1", 
                                vectorGetCount(args));
        return (Object_t*)createError(err); 
    }
    
    Object_t** argBuf = (Object_t**)vectorGetBuffer(args);
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


Object_t* restBuiltin(Vector_t* args) {
    if (vectorGetCount(args) != 1) {
        char* err = strFormat("wrong number of arguments. got=%d, want=1", 
                                vectorGetCount(args));
        return (Object_t*)createError(err); 
    }
    
    Object_t** argBuf = (Object_t**)vectorGetBuffer(args);
    if (argBuf[0]->type != OBJECT_ARRAY) {
        char* err = strFormat("argument to `rest` must be ARRAY, got %s", 
                                objectTypeToString(argBuf[0]->type));
        return (Object_t*)createError(err);                      
    }

    Array_t* arr = (Array_t*)argBuf[0];
    uint32_t len = arrayGetElementCount(arr); 
    Object_t** elems = arrayGetElements(arr);
    if (len > 0) {
        Vector_t* newElements = createVector();
        for (uint32_t i = 1; i < len; i++) {
            vectorAppend(newElements, copyObject(elems[i]));
        } 
        Array_t* newArr = createArray();
        newArr->elements = newElements;
        return (Object_t*)newArr;
    }

    return (Object_t*) createNull();
}

Object_t* pushBuiltin(Vector_t* args) {
    if (vectorGetCount(args) != 2) {
        char* err = strFormat("wrong number of arguments. got=%d, want=2", 
                                vectorGetCount(args));
        return (Object_t*)createError(err); 
    }
    
    Object_t** argBuf = (Object_t**)vectorGetBuffer(args);
    if (argBuf[0]->type != OBJECT_ARRAY) {
        char* err = strFormat("argument to `push` must be ARRAY, got %s", 
                                objectTypeToString(argBuf[0]->type));
        return (Object_t*)createError(err);                      
    }

    Array_t* arr = (Array_t*)argBuf[0];
    Object_t* obj = (Object_t*)argBuf[1];

    uint32_t len = arrayGetElementCount(arr); 
    Object_t** elems = arrayGetElements(arr);
    
    Vector_t* newElements = createVector();
    for (uint32_t i = 0; i < len; i++) {
        vectorAppend(newElements, copyObject(elems[i]));
    } 
    // add new element 
    vectorAppend(newElements, obj);
    Array_t* newArr = createArray();
    newArr->elements = newElements;
    return (Object_t*)newArr;

}

Object_t* putsBuiltin(Vector_t* args) {
    uint32_t argCnt = vectorGetCount(args);
    Object_t** argBuf = (Object_t**)vectorGetBuffer(args);
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

Object_t* printfBuiltin(Vector_t* args) {
    Object_t* retValue = (Object_t*) createNull();
    uint32_t argCnt = vectorGetCount(args);
    Object_t** argBuf = (Object_t**)vectorGetBuffer(args);

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
