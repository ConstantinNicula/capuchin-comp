#include <malloc.h> 
#include <string.h> 
#include "token.h"
#include "utils.h"


Token_t* createToken(TokenType_t type, const char* literal, uint16_t len) {
    Token_t* token = mallocChk(sizeof(Token_t));

    token->type = type;    
    token->literal = cloneSubstring(literal, len); 
    if (token->literal == NULL)
        return NULL;
        
    return token;
}

Token_t* copyToken(const Token_t* tok) {
    return createToken(tok->type, tok->literal, strlen(tok->literal));
}

void cleanupToken(Token_t** token)
{
    if (!*token)
        return;
    free((*token)->literal);
    free(*token);
    *token = NULL;
}

TokenType_t lookupIdent(const char* ident, uint32_t len)
{
    if(strlen("let") == len && strncmp(ident, "let", len) == 0)
        return TOKEN_LET;
    else if (strlen("fn") == len && strncmp(ident, "fn", len) == 0) 
        return TOKEN_FUNCTION;
    else if (strlen("true") == len && strncmp(ident, "true", len) == 0) 
        return TOKEN_TRUE;
    else if (strlen("false") == len && strncmp(ident, "false", len) == 0) 
        return TOKEN_FALSE;
    else if (strlen("if") == len && strncmp(ident, "if", len) == 0) 
        return TOKEN_IF;
    else if (strlen("else") == len && strncmp(ident, "else", len) == 0) 
        return TOKEN_ELSE;
    else if (strlen("return") == len && strncmp(ident, "return", len) == 0) 
        return TOKEN_RETURN;
    return TOKEN_IDENT;
}
/* C99 designated initializer abuse :) */
static const char* TokenTypeStrings[32] = {
    [TOKEN_ILLEGAL]="TOKEN_ILLEGAL", [TOKEN_EOF]="TOKEN_EOF",
    [TOKEN_IDENT]="TOKEN_IDENT", [TOKEN_INT]="TOKEN_INT",
    [TOKEN_STRING]="TOKEN_STRING",
    [TOKEN_ASSIGN]="=", [TOKEN_PLUS]="+", 
    [TOKEN_MINUS]="-", [TOKEN_BANG]="!",
    [TOKEN_ASTERISK]="*", [TOKEN_SLASH]="?",    
    [TOKEN_LT]="<", [TOKEN_GT]=">",
    [TOKEN_EQ]="=", [TOKEN_NOT_EQ]="!=",
    [TOKEN_COMMA]=",", [TOKEN_SEMICOLON]=";", 
    [TOKEN_LPAREN]="(", [TOKEN_RPAREN]=")", 
    [TOKEN_LBRACE]="{", [TOKEN_RBRACE]="}", 
    [TOKEN_LBRACKET]="[", [TOKEN_RBRACKET]="]", 
    [TOKEN_FUNCTION]="TOKEN_FUNCTION", [TOKEN_LET]="TOKEN_LET",
    [TOKEN_TRUE]="TOKEN_TRUE", [TOKEN_FALSE]="TOKEN_FALSE",
    [TOKEN_IF]="TOKEN_IF",  [TOKEN_ELSE]="TOKEN_ELSE",
    [TOKEN_RETURN]="TOKEN_RETURN", 
};

const char * tokenTypeToStr(TokenType_t tokType)
{
    if (tokType < 0 || tokType >= _TOKEN_TYPE_CNT) {
        return "INVLAID_TOKEN_TYPE";
    }
    return TokenTypeStrings[tokType];
}
