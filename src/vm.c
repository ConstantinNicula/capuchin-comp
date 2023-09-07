#include "vm.h"
#include "utils.h"


static VmError_t vmExecuteOpConstant(Vm_t* vm, uint32_t* ip); 
static VmError_t vmExecuteBinaryOperation(Vm_t *vm, OpCode_t op);
static VmError_t vmExecuteBinaryIntegerOperation(Vm_t *vm, OpCode_t op, Integer_t* left, Integer_t* right); 
static VmError_t vmExecuteOpBoolean(Vm_t* vm, OpCode_t op); 
static VmError_t vmExecuteOpPop(Vm_t* vm); 
static VmError_t vmPush(Vm_t* vm, Object_t* obj); 
static Object_t* vmPop(Vm_t* vm);


static Boolean_t True = (Boolean_t) {
    .type = OBJECT_BOOLEAN, 
    .value = true,
};

static Boolean_t False = (Boolean_t) {
    .type = OBJECT_BOOLEAN,
    .value = false,
};


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

Object_t* vmLastPoppedStackElem(Vm_t *vm) {
    return vm->stack[vm->sp];
}

VmError_t vmRun(Vm_t *vm) {
    VmError_t err = VM_NO_ERROR;
    uint32_t len = sliceByteGetLen(vm->instructions);
    
    for (uint32_t ip = 0; ip < len; ip++) {
        OpCode_t op = vm->instructions[ip];
         
        switch(op) {
            case OP_CONSTANT: 
                err = vmExecuteOpConstant(vm, &ip); 
                break;

            case OP_ADD:
            case OP_SUB:
            case OP_MUL:
            case OP_DIV:
                err = vmExecuteBinaryOperation(vm, op); 
                break;

            case OP_TRUE:
            case OP_FALSE: 
                err = vmExecuteOpBoolean(vm, op);
                break;
                
            case OP_POP: 
                err = vmExecuteOpPop(vm);
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

static VmError_t vmExecuteOpConstant(Vm_t* vm, uint32_t* ip) {
    uint16_t constIndex = (vm->instructions[*ip+1]) << 8 | vm->instructions[*ip+2];
    *ip += 2;

    Object_t* constObj = vectorObjectsGetBuffer(vm->constants)[constIndex]; 
    return vmPush(vm, constObj);
}


static VmError_t vmExecuteBinaryOperation(Vm_t *vm, OpCode_t op) {
    Object_t* right = vmPop(vm);
    Object_t* left = vmPop(vm);

    if (left->type == OBJECT_INTEGER && right->type == OBJECT_INTEGER) {
        return vmExecuteBinaryIntegerOperation(vm, op, (Integer_t*)left, (Integer_t*)right);
    }

    return VM_UNSUPPORTED_TYPES; 
}

static VmError_t vmExecuteBinaryIntegerOperation(Vm_t *vm, OpCode_t op, Integer_t* left, Integer_t* right) {
    int64_t leftValue = left->value;
    int64_t rightValue = right->value;

    int64_t result;
    switch(op) {
        case OP_ADD:
            result = leftValue + rightValue;
            break;
        case OP_SUB: 
            result = leftValue - rightValue;
            break;
        case OP_MUL: 
            result = leftValue * rightValue;
            break;
        case OP_DIV:
            result = leftValue / rightValue;
            break;
        default:
            return VM_UNSUPPORTED_OPERATOR;
    }

    return vmPush(vm, (Object_t*)createInteger(result));
}

static VmError_t vmExecuteOpBoolean(Vm_t* vm, OpCode_t op) {
    Boolean_t* objBool = (op == OP_TRUE) ? &True : &False;
    return vmPush(vm, (Object_t*)objBool);
}

static VmError_t vmExecuteOpPop(Vm_t* vm) {
    vmPop(vm); 
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

