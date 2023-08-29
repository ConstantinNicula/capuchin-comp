#include <stdio.h> 
#include <stdlib.h>
#include <stdbool.h>
#include <string.h> 

#include "../parser.h"
#include "../lexer.h"
#include "../utils.h"
#include "../evaluator.h"
#include "../env.h"
#include "../gc.h"

#define PROMPT ">> "

static void printParserErrors(const char**err, uint32_t cnt){
    printf("Woops! We ran into some monkey business here!\n");
    printf(" parser errors:\n");

    for (uint32_t i = 0; i < cnt ; i++) {
        printf ("\t%s\n", err[i]);
    }
}

void evalInput(const char* input, Environment_t* env) {
    Lexer_t* lexer = createLexer(input);
    Parser_t* parser = createParser(lexer);
    Program_t* program = parserParseProgram(parser);

    if (parserGetErrorCount(parser) != 0) {
        printParserErrors(parserGetErrors(parser), parserGetErrorCount(parser));
    } else {
        Object_t* evalRes = evalProgram(program, env);

        if (evalRes != NULL && evalRes->type != OBJECT_NULL) {
            char* inspect = objectInspect(evalRes);
            printf("%s\n", inspect);
            free(inspect);
        }

        gcFreeExtRef(evalRes);
    }

    cleanupParser(&parser);
    cleanupProgram(&program);
}

void replMode() {
    char inputBuffer[4096] = "";
    Environment_t* env = createEnvironment(NULL);
    while (true) {
        printf("%s", PROMPT);
        if(!fgets(inputBuffer, sizeof(inputBuffer), stdin))
            break;
        
        if (strcmp(inputBuffer, "quit\n") == 0) 
            break;
            
        evalInput(inputBuffer, env);
    }
    gcFreeExtRef(env);
}

char* readEntireFile(char* filename) {
    FILE* f = fopen(filename, "rb");
    if(!f) {
        perror("Failed to open file!");
        exit(1);
    }
    long fsize;
    fseek(f, 0, SEEK_END);
    fsize = ftell(f);
    rewind(f);

    char* ret = calloc(1, fsize + 2);
    if (!ret) HANDLE_OOM();
    fread(ret, fsize, 1, f);
    fclose(f);
    return ret;
}

void fileExecMode(char* filename) {
    char* input = readEntireFile(filename);
    Environment_t* env = createEnvironment(NULL);
    evalInput(input, env);
    gcFreeExtRef(env);
    free(input);
}


int main(int argc, char**argv) {
    if (argc == 1) {
        // no parameters provided
        replMode();
    } else {    
        fileExecMode(argv[1]);
    }
    return 0;
}