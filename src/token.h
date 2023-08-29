#ifndef _TOKEN_H_
#define _TOKEN_H_

#include <stdio.h>
#include <stdint.h> 

typedef enum TokenType{
    TOKEN_ILLEGAL, 
    TOKEN_EOF, 

    // Identifieres + literals 
    TOKEN_IDENT, // add, foobar, x, y, ..
    TOKEN_INT,  // 1343456
    TOKEN_STRING, // "hello"

    // Operators
    TOKEN_ASSIGN,
    TOKEN_PLUS, 
    TOKEN_MINUS,
    TOKEN_BANG,
    TOKEN_ASTERISK,
    TOKEN_SLASH,    

    TOKEN_LT, 
    TOKEN_GT,
    TOKEN_EQ,
    TOKEN_NOT_EQ,

    // Delimiters
    TOKEN_COMMA, 
    TOKEN_SEMICOLON, 
    TOKEN_COLON,

    TOKEN_LPAREN, 
    TOKEN_RPAREN, 
    TOKEN_LBRACE, 
    TOKEN_RBRACE, 
    TOKEN_LBRACKET,
    TOKEN_RBRACKET,

    // Keywords, 
    TOKEN_FUNCTION, 
    TOKEN_LET,
    TOKEN_TRUE,
    TOKEN_FALSE,
    TOKEN_IF, 
    TOKEN_ELSE,
    TOKEN_RETURN,

    _TOKEN_TYPE_CNT
} TokenType_t; 

typedef struct Token {
    TokenType_t type;
    char* literal; 
} Token_t;


Token_t* createToken(TokenType_t type, const char* literal, uint16_t len);
Token_t* copyToken(const Token_t* tok);
void cleanupToken(Token_t** token);


TokenType_t lookupIdent(const char* ident, uint32_t len);
const char * tokenTypeToStr(TokenType_t token);




#endif