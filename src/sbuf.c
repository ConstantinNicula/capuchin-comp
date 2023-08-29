#include <malloc.h>
#include <string.h> 

#include "sbuf.h"
#include "utils.h"

Strbuf_t* createStrbuf() {
    Strbuf_t* sbuf = mallocChk(sizeof(Strbuf_t));
    *sbuf = (Strbuf_t) {
        .str = NULL,
        .len = 0, 
        .cap = 0
    }; 
    return sbuf;
}
void cleanupStrbuf(Strbuf_t** sbuf) {
    if (!(*sbuf)) 
        return;
    
    if ((*sbuf)->str != NULL)
        free((*sbuf)->str);

    (*sbuf)->len = 0u;
    free(*sbuf);
}

char* detachStrbuf(Strbuf_t** sbuf) {
    if (*sbuf == NULL) 
        return NULL;

    // no more refs 
    char* str = (*sbuf)->str;
    (*sbuf)->len = 0u;
    free(*sbuf);
    
    return str;
}

void strbufWrite(Strbuf_t* sbuf, const char* str) {
    uint32_t tlen = sbuf->len + strlen(str);
    
    if (sbuf->str == NULL) {
        sbuf->str = (char*) malloc(tlen + 1u);  
        sbuf->str[0]= '\0';
        sbuf->cap = tlen + 1u; 
    } else if (sbuf->cap < tlen + 1) {
        while (sbuf->cap < tlen + 1) {
            sbuf->cap = (3 * sbuf->cap) / 2 + 1;
        }
        sbuf->str = (char*) realloc(sbuf->str, sbuf->cap);
    }
    
    if (sbuf->str == NULL){
        HANDLE_OOM();
    }
    sbuf->len = tlen;
    strcat(sbuf->str, str);
}

void strbufConsume(Strbuf_t* sbuf, char* str) {
    if (str == NULL)
        return;
    strbufWrite(sbuf, str);
    free(str);
}

void strbufWriteChar(Strbuf_t*sbuf, char c) {
    char str[2] = {c, '\0'};
    strbufWrite(sbuf, str);
}