#ifndef _VM_H_
#define _VM_H_

#include "object.h"
#include "code.h"
#include "compiler.h"
#include "frame.h"


#define STACK_SIZE 2048
#define GLOBALS_SIZE 65536

typedef enum VmError {
    VM_NO_ERROR = 0, 
    VM_STACK_OVERFLOW,
    VM_UNSUPPORTED_TYPES,
    VM_UNSUPPORTED_OPERATOR,
    VM_INVALID_KEY, 
} VmError_t;

typedef struct Vm {
// Compiled constants 
    VectorObjects_t* constants;

// Stack variables  
    Object_t** stack;
    uint16_t sp;

// Global storage  
    Object_t** globals;
    bool externalStorage;

// Call frames  
    Frame_t* frames;
    uint32_t frameIndex; 

} Vm_t;

Vm_t createVm(Bytecode_t* bytecode);
Vm_t createVmWithStore(Bytecode_t* bytecode, Object_t** s);
void cleanupVm(Vm_t *vm);


Object_t* vmStackTop(Vm_t *vm);
VmError_t vmRun(Vm_t *vm);
Object_t* vmLastPoppedStackElem(Vm_t *vm); 

#endif