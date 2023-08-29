#include "utils.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

char* cloneString(const char* str) {
    return cloneSubstring(str, strlen(str));
}

char* cloneSubstring(const char* str, uint32_t len) {
    char* newStr = (char*) calloc(len + 1u, sizeof(char));
    if (newStr == NULL) {
        return NULL;
    } 
    memmove(newStr, str, len);
    return newStr;
}


bool strToInteger(const char* str, int64_t* val) { 
    char* endPtr = NULL;
    *val = strtol(str, &endPtr, 10);

    if (endPtr == str)
        return false;

    // TO DO: conversion can also fail due to overflow, not handled here :)
    return true;
}

char* strFormat(const char* fmt, ...) {
    va_list argp, argp2;
 
    va_start(argp, fmt);
    va_copy(argp2, argp);

    int len = vsnprintf(NULL, 0, fmt, argp) + 1;
    char* str = malloc(len);
    vsnprintf(str, len, fmt, argp2);    
   
    va_end(argp);
    va_end(argp2);

    return str;
}


void* mallocChk(size_t size) {
    void* ptr = malloc(size);
    if (!ptr) HANDLE_OOM();
    return ptr;
}