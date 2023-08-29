#include <malloc.h>
#include <string.h> 
#include <stdio.h> 
#include <stdint.h>
#include <stdbool.h> 


#include "lexer.h"
#include "token.h"
#include "utils.h"

static void lexerReadChar(Lexer_t* lexer);
static char lexerPeekChar(Lexer_t* lexer);
static void lexerReadIdentifier(Lexer_t* lexer, uint32_t* len);
static void lexerReadString(Lexer_t* lexer, uint32_t*len);
static void lexerReadDigit(Lexer_t* lexer, uint32_t* len);
static void lexerSkipWhiteSpace(Lexer_t* lexer);

static bool isLetter(char ch);
static bool isWhiteSpace(char ch);
static bool isDigit(char ch);

Lexer_t* createLexer(const char* input)
{
    if (input == NULL)
        return NULL;

    Lexer_t* lexer = mallocChk(sizeof(Lexer_t));
    
    lexer->input = input;
    lexer->position = 0;
    lexer->readPosition = 0;
    lexer->inputLength = strlen(lexer->input);

    lexerReadChar(lexer);
    return lexer;
}

void cleanupLexer(Lexer_t** lexer)
{
    if (!(*lexer))
        return;
        
    free(*lexer);
    *lexer = NULL;
}


Token_t* lexerNextToken(Lexer_t* lexer) {
    Token_t *tok = NULL;
    const char* tokStart = NULL;
    uint32_t tokLen = 0u;

    lexerSkipWhiteSpace(lexer);
    tokStart = &lexer->input[lexer->position]; 

    switch(lexer->ch) {
        case '=': 
            if (lexerPeekChar(lexer) == '=') {
                lexerReadChar(lexer); // consume next char 
                tok = createToken(TOKEN_EQ, tokStart, 2u);
            }
            else {
                tok = createToken(TOKEN_ASSIGN, tokStart, 1u);
            }
            break;
        case '+':
            tok = createToken(TOKEN_PLUS, tokStart, 1u);
            break;
        case '-': 
            tok = createToken(TOKEN_MINUS, tokStart, 1u);
            break;
        case '!': 
            if (lexerPeekChar(lexer) == '=') {
                lexerReadChar(lexer);
                tok = createToken(TOKEN_NOT_EQ, tokStart, 2u);
            }
            else {
                tok = createToken(TOKEN_BANG, tokStart, 1u);
            }
            break;
        case '*': 
            tok = createToken(TOKEN_ASTERISK, tokStart, 1u);
            break;
        case '/': 
            tok = createToken(TOKEN_SLASH, tokStart, 1u);
            break;
        case '<': 
            tok = createToken(TOKEN_LT, tokStart, 1u);
            break;
        case '>': 
            tok = createToken(TOKEN_GT, tokStart, 1u);
            break;
        case ',': 
            tok = createToken(TOKEN_COMMA, tokStart, 1u);
            break;
        case ';':
            tok = createToken(TOKEN_SEMICOLON, tokStart, 1u);
            break;
        case '(':
            tok = createToken(TOKEN_LPAREN, tokStart, 1u);
            break;
        case ')':
            tok = createToken(TOKEN_RPAREN, tokStart, 1u);
            break;
        case '{':
            tok = createToken(TOKEN_LBRACE, tokStart, 1u);
            break;
        case '}':
            tok = createToken(TOKEN_RBRACE, tokStart, 1u);
            break;
        case '[':
            tok = createToken(TOKEN_LBRACKET, tokStart, 1u);
            break;
        case ']':
            tok = createToken(TOKEN_RBRACKET, tokStart, 1u);
            break;
        case '\0':
            tok = createToken(TOKEN_EOF, tokStart, 1u);
            break;  
        case ':': 
            tok = createToken(TOKEN_COLON, tokStart, 1u);
            break;
        case '"':
            // now sitting on '"', skip it
            lexerReadChar(lexer);
            tokStart++;
            
            lexerReadString(lexer, &tokLen);
            if (lexer->ch != '"')
                tok = createToken(TOKEN_ILLEGAL, tokStart, tokLen);
            else 
                tok = createToken(TOKEN_STRING, tokStart, tokLen);
            break;
        default: 
            if (isLetter(lexer->ch)) {
                lexerReadIdentifier(lexer, &tokLen);
                tok = createToken(lookupIdent(tokStart, tokLen), tokStart, tokLen);
                return tok;
            } else if(isDigit(lexer->ch)) {
                lexerReadDigit(lexer, &tokLen);
                tok = createToken(TOKEN_INT, tokStart, tokLen);
                return tok;
            } else { 
                tok = createToken(TOKEN_ILLEGAL, tokStart, tokLen);
            }

    }

    lexerReadChar(lexer);
    return tok;
}

static void lexerReadChar(Lexer_t* lexer) {
    if (lexer->readPosition >= lexer->inputLength)
        lexer->ch = '\0'; // Empty string return NULL character 
    else 
        lexer->ch = lexer->input[lexer->readPosition];

    lexer->position = lexer->readPosition;
    lexer->readPosition++;
}

static char lexerPeekChar(Lexer_t*lexer) {
    if (lexer->readPosition >= lexer->inputLength) 
        return '\0';
    else 
        return lexer->input[lexer->readPosition];
}

static void lexerReadIdentifier(Lexer_t* lexer, uint32_t* len) {
    *len = 0u;
    while (isLetter(lexer->ch)) {
        lexerReadChar(lexer);
        (*len)++;
    }

}

static void lexerReadString(Lexer_t* lexer, uint32_t*len) {
    while (lexer->ch != '"' && lexer->ch != '\0'){
        lexerReadChar(lexer);
        (*len)++;
    }
}

static void lexerReadDigit(Lexer_t* lexer, uint32_t* len) {
    *len = 0u;
    while(isDigit(lexer->ch)) {
        lexerReadChar(lexer);
        (*len)++;
    }
}

static void lexerSkipWhiteSpace(Lexer_t* lexer) {
    while (isWhiteSpace(lexer->ch)) {
        lexerReadChar(lexer);
    }
}


static bool isLetter(char ch) {
    return (('a' <= ch) && (ch <='z')) || (('A' <= ch) && (ch <= 'Z')) || (ch == '_');
}

static bool isWhiteSpace(char ch) {
    return (ch == ' ') || (ch == '\t') || (ch == '\n') || (ch == '\r');
}

static bool isDigit(char ch) {
    return ('0' <= ch) && (ch <= '9');
}