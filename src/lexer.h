#ifndef _LEXER_H_
#define _LEXER_H_

#include <stdint.h>
#include "token.h"

typedef struct Lexer {
    const char* input;
    int32_t inputLength; // lenght of input string 
    int32_t position; // current position in input (points to current char)
    int32_t readPosition; // current reading position in input (affter current char)
    char ch; // curernt char under examination 
} Lexer_t;

// Relevant API 
Token_t* lexerNextToken(Lexer_t* lexer);

// Create and cleanup 
Lexer_t* createLexer(const char* input);
void cleanupLexer(Lexer_t** lexer);


#endif 