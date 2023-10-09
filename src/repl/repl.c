#include <stdio.h> 
#include <stdlib.h>
#include <stdbool.h>
#include <string.h> 

#include "../parser.h"
#include "../lexer.h"
#include "../utils.h"
#include "../compiler.h"
#include "../vm.h"
#include "../gc.h"

#define PROMPT ">> "

static void printParserErrors(const char**err, uint32_t cnt){
    printf("Woops! We ran into some monkey business here!\n");
    printf(" parser errors:\n");

    for (uint32_t i = 0; i < cnt ; i++) {
        printf ("\t%s\n", err[i]);
    }
}

void evalInput(const char* input, SymbolTable_t* symTable, VectorObjects_t* constants,  Object_t** globals) {
    Lexer_t* lexer = createLexer(input);
    Parser_t* parser = createParser(lexer);
    Program_t* program = parserParseProgram(parser);

    if (parserGetErrorCount(parser) != 0) {
        printParserErrors(parserGetErrors(parser), parserGetErrorCount(parser));
        goto parser_err;
    }
    
    Compiler_t comp = createCompilerWithState(symTable, constants);
    CompError_t compErr = compilerCompile(&comp, program);        
    if (compErr != COMP_NO_ERROR) {
        printf("Woops! Compilation failed:\n %d\n", compErr);
        goto comp_err;
    }

    Bytecode_t bytecode = compilerGetBytecode(&comp);
    Vm_t vm = createVmWithStore(&bytecode, globals);
    VmError_t vmErr = vmRun(&vm);
    if (vmErr.code != VM_NO_ERROR) {
        printf("Woops! Executing bytecode failed:\n %s\n", vmErr.str);
        goto vm_err;
    } 

    Object_t* stackTop = vmLastPoppedStackElem(&vm);
    printf("%s\n", objectInspect(stackTop));
    // TO DO: fix memory leak (nothing is gc'd at the moment)
    //gcForceRun();

vm_err: 
    cleanupVm(&vm);
comp_err: 
    cleanupCompiler(&comp);
parser_err: 
    cleanupParser(&parser);
    cleanupProgram(&program);
}

void replMode() {
    char inputBuffer[4096] = "";
    Object_t** globals = callocChk(GLOBALS_SIZE * sizeof(Object_t*));
    VectorObjects_t* constants = createVectorObjects();
    SymbolTable_t* symTable = createSymbolTable(); 
    while (true) {
        printf("%s", PROMPT);
        if(!fgets(inputBuffer, sizeof(inputBuffer), stdin))
            break;
        
        if (strcmp(inputBuffer, "quit\n") == 0) 
            break;
            
        evalInput(inputBuffer, symTable, constants, globals);
    }

    cleanupSymbolTable(symTable);
    // FIXME: cleanup does not remove allocate objects 
    cleanupVectorObjects(&constants, NULL);
    free(globals);
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
    Object_t** globals = mallocChk(GLOBALS_SIZE * sizeof(Object_t*));
    VectorObjects_t* constants = createVectorObjects();
    SymbolTable_t* symTable = createSymbolTable();

    evalInput(input, symTable, constants, globals);
    
    cleanupSymbolTable(symTable);
    // FIXME: cleanup does not remove allocate objects 
    cleanupVectorObjects(&constants, NULL);
    free(globals);
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