#include "vm.h"
#include "utils.h"


static VmError_t vmProcessOpConstant(Vm_t* vm, uint32_t* ip); 
static VmError_t vmProcessOpAdd(Vm_t *vm);
static VmError_t vmPush(Vm_t* vm, Object_t* obj); 
static Object_t* vmPop(Vm_t* vm);

Vm_t createVm(Bytecode_t* bytecode) {
    return (Vm_t) {
        .constants = bytecode->constants,
        .instructions = bytecode->instructions, 
        .sp = 0, 
        .stack = mallocChk(STACK_SIZE * sizeof(Object_t*))
    };
} 

void cleanupVm(Vm_t *vm) {
    if (!vm) return;
    free(vm->stack);
}

Object_t* vmStackTop(Vm_t *vm) {
    if (vm->sp == 0) return NULL;
    return vm->stack[vm->sp-1];
}

VmError_t vmRun(Vm_t *vm) {
    VmError_t err = VM_NO_ERROR;
    uint32_t len = sliceByteGetLen(vm->instructions);
    
    for (uint32_t ip = 0; ip < len; ip++) {
        OpCode_t op = vm->instructions[ip];
         
        switch(op) {
            case OP_CONSTANT: 
                err = vmProcessOpConstant(vm, &ip); 
                break;
            case OP_ADD: 
                err = vmProcessOpAdd(vm); 
                break;
            default:
                break;
        }

        if (err != VM_NO_ERROR) {
            break;
        }
    }
    return err;
}

static VmError_t vmProcessOpConstant(Vm_t* vm, uint32_t* ip) {
    uint16_t constIndex = (vm->instructions[*ip+1]) << 8 | vm->instructions[*ip+2];
    *ip += 2;

    Object_t* constObj = vectorObjectsGetBuffer(vm->constants)[constIndex]; 
    return vmPush(vm, constObj);
}

static VmError_t vmProcessOpAdd(Vm_t *vm) {
    Integer_t* right = (Integer_t*)vmPop(vm);
    Integer_t* left = (Integer_t*)vmPop(vm);
    int64_t leftValue = left->value;
    int64_t rightValue = right->value;

    int64_t result = leftValue + rightValue;
    vmPush(vm, (Object_t*)createInteger(result));

    return VM_NO_ERROR;
}

static VmError_t vmPush(Vm_t* vm, Object_t* obj) {
    if (vm->sp >= STACK_SIZE) {
        return VM_STACK_OVERFLOW;
    }

    vm->stack[vm->sp] = obj;
    vm->sp++;

    return VM_NO_ERROR;
}

static Object_t* vmPop(Vm_t* vm) {
    Object_t* obj = vm->stack[vm->sp-1]; 
    vm->sp--;
    return obj;
}