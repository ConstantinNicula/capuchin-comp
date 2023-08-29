#ifndef _SBUF_H_
#define _SBUF_H_

#include <stdint.h>

typedef struct Strbuf {
    char* str;
    uint32_t len;
    uint32_t cap;
} Strbuf_t;

Strbuf_t* createStrbuf();
void cleanupStrbuf(Strbuf_t** sbuf);
char* detachStrbuf(Strbuf_t** sbuf);

void strbufWrite(Strbuf_t* sbuf, const char* str);
void strbufWriteChar(Strbuf_t* sbuf, char c);
void strbufConsume(Strbuf_t* sbuf, char* str);



#endif