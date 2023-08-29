#ifndef _UTILS_H_
#define _UTILS_H_

#include <stdint.h>
#include <stdbool.h> 
#include <stdlib.h>

char* cloneString(const char* str);
char* cloneSubstring(const char* str, uint32_t len);

char* strFormat(const char* fmt, ...);
bool strToInteger(const char* str, int64_t* val);


void* mallocChk(size_t size);
#define HANDLE_OOM() {\
    perror("HMAP ERROR: Failed to allocate memory!");\
    exit(1);\
} while(0)

#endif