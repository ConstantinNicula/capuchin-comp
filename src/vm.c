#include "vm.h"
#include "utils.h"
#include "gc.h"
#include "frame.h"
#define MAX_FRAMES 1024 

static void cleanupGlobals(Vm_t *vm); 
static void cleanupStack(Vm_t* vm);
static void cleanupFrames(Vm_t* vm);

static void vmPushFrame(Vm_t *vm, Frame_t *f);
static Frame_t* vmCurrentFrame(Vm_t *vm);
static Frame_t vmPopFrame(Vm_t *vm); 

static Instructions_t vmGetInstructions(Vm_t* vm);

static VmError_t vmExecuteOpConstant(Vm_t* vm, int32_t* ip); 
static VmError_t vmExecuteBinaryOperation(Vm_t *vm, OpCode_t op);
static VmError_t vmExecuteBinaryIntegerOperation(Vm_t *vm, OpCode_t op, Integer_t* left, Integer_t* right); 
static VmError_t vmExecuteBinaryStringOperation(Vm_t *vm, OpCode_t op, String_t* left, String_t* right); 
static VmError_t vmExecuteOpBoolean(Vm_t* vm, OpCode_t op); 
static VmError_t vmExecuteOpNull(Vm_t* vm); 

static VmError_t vmExecuteComparison(Vm_t* vm, OpCode_t op);
static VmError_t vmExecuteIntegerComparison(Vm_t* vm, OpCode_t op, Integer_t* left, Integer_t* right); 
static VmError_t vmExecuteBooleanComparison(Vm_t* vm, OpCode_t op, Boolean_t* left, Boolean_t* right);

static VmError_t vmExecuteBangOperator(Vm_t *vm);
static VmError_t vmExecuteMinusOperator(Vm_t *vm);

static VmError_t vmExecuteOpJump(Vm_t* vm, int32_t* ip);
static VmError_t vmExecuteOpJumpNotTruthy(Vm_t* vm, int32_t* ip);

static VmError_t vmExecuteOpPop(Vm_t* vm); 
static VmError_t vmExecuteOpSetGlobal(Vm_t* vm, int32_t* ip); 
static VmError_t vmExecuteOpGetGlobal(Vm_t* vm, int32_t* ip); 

static VmError_t vmExecuteOpArray(Vm_t* vm, int32_t* ip); 
static Array_t* vmBuildArray(Vm_t* vm, uint16_t numElements);

static VmError_t vmExecuteOpHash(Vm_t* vm, int32_t* ip); 
static Hash_t* vmBuildHash(Vm_t* vm, uint16_t numElements); 

static VmError_t vmExecuteOpIndex(Vm_t* vm); 
static VmError_t vmExecuteArrayIndex(Vm_t* vm, Array_t*array, Integer_t* index);
static VmError_t vmExecuteHashIndex(Vm_t* vm, Hash_t* hash, Object_t* index); 

static VmError_t vmExecuteOpCall(Vm_t* vm);
static VmError_t vmExecuteOpReturnValue(Vm_t* vm); 
static VmError_t vmExecuteOpReturn(Vm_t* vm); 

static VmError_t vmPush(Vm_t* vm, Object_t* obj); 
static Object_t* vmPop(Vm_t* vm);

static bool vmIsTruthy(Object_t* obj);
static Object_t* nativeBoolToBooleanObject(bool val);
static uint16_t readUint16BigEndian(uint8_t* ptr); 



Vm_t createVm(Bytecode_t* bytecode) {
    Frame_t* frames = callocChk(MAX_FRAMES * sizeof(Frame_t));
    frames[0] = createFrame(createCompiledFunction(bytecode->instructions));

    return (Vm_t) {
        .constants = bytecode->constants,

        .stack = callocChk(STACK_SIZE * sizeof(Object_t*)),
        .sp = 0, 

        .externalStorage = false,
        .globals = callocChk(GLOBALS_SIZE * sizeof(Object_t*)),

        .frames = frames,
        .frameIndex = 1,
    };
} 

Vm_t createVmWithStore(Bytecode_t* bytecode, Object_t** s)  {
    Frame_t* frames = callocChk(MAX_FRAMES * sizeof(Frame_t));
    frames[0] = createFrame(createCompiledFunction(bytecode->instructions));

    return (Vm_t) {
        .constants = bytecode->constants,

        .stack = callocChk(STACK_SIZE * sizeof(Object_t*)),
        .sp = 0, 
        
        .externalStorage = true,
        .globals = s,
        
        .frames = frames,
        .frameIndex = 1,
    };
}

void cleanupVm(Vm_t *vm) {
    if (!vm) return;

    cleanupVectorObjects(&vm->constants, NULL);
    cleanupStack(vm);
    if (!vm->externalStorage) cleanupGlobals(vm); 
    cleanupFrames(vm);
}

static void cleanupGlobals(Vm_t *vm) {
    uint16_t i = 0; 
    while (vm->globals[i] != NULL && i < GLOBALS_SIZE) {
        gcFreeExtRef(vm->globals[i]);
        i++;
    }
    free(vm->globals);
}

static void cleanupStack(Vm_t *vm) {
    while (vm->sp) {
        vmPop(vm);
    }
    free(vm->stack);
}

static void cleanupFrames(Vm_t* vm) {
    for (uint32_t i = 0; i < vm->frameIndex; i++) {
        cleanupFrame(&(vm->frames[i]));
    }
    free(vm->frames);
}

static Frame_t* vmCurrentFrame(Vm_t *vm) {
    return &(vm->frames[vm->frameIndex-1]);
}

static void vmPushFrame(Vm_t *vm, Frame_t *f) {
    vm->frames[vm->frameIndex] = *f;
    vm->frameIndex++;
}

static Frame_t vmPopFrame(Vm_t *vm) {
    vm->frameIndex--;
    return vm->frames[vm->frameIndex];
} 

static Instructions_t vmGetInstructions(Vm_t* vm) { 
    return frameGetInstructions(vmCurrentFrame(vm));
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
    
    while (vmCurrentFrame(vm)->ip < (int32_t)sliceByteGetLen(vmCurrentFrame(vm)->fn->instructions) - 1) {
        vmCurrentFrame(vm)->ip++;

        Instructions_t ins = vmGetInstructions(vm);
        OpCode_t op = ins[vmCurrentFrame(vm)->ip];
         
        switch(op) {
            case OP_CONSTANT: 
                err = vmExecuteOpConstant(vm, &vmCurrentFrame(vm)->ip); 
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

            case OP_NULL:
                err = vmExecuteOpNull(vm);
                break;

            case OP_EQUAL:
            case OP_NOT_EQUAL:
            case OP_GREATER_THAN:
                err = vmExecuteComparison(vm, op);
                break;

            case OP_BANG:
                err = vmExecuteBangOperator(vm);
                break;

            case OP_MINUS: 
                err = vmExecuteMinusOperator(vm);
                break;

            case OP_POP: 
                err = vmExecuteOpPop(vm);
                break;

            case OP_JUMP: 
                err = vmExecuteOpJump(vm, &vmCurrentFrame(vm)->ip);
                break;
            
            case OP_JUMP_NOT_TRUTHY:
                err = vmExecuteOpJumpNotTruthy(vm, &vmCurrentFrame(vm)->ip);
                break;

            case OP_SET_GLOBAL:
                err = vmExecuteOpSetGlobal(vm, &vmCurrentFrame(vm)->ip);
                break;
        
            case OP_GET_GLOBAL:
                err = vmExecuteOpGetGlobal(vm, &vmCurrentFrame(vm)->ip);
                break;

            case OP_ARRAY:
                err = vmExecuteOpArray(vm, &vmCurrentFrame(vm)->ip);
                break;

            case OP_HASH:
                err = vmExecuteOpHash(vm , &vmCurrentFrame(vm)->ip);
                break;

            case OP_INDEX:
                err = vmExecuteOpIndex(vm);
                break;

            case OP_CALL:
                err = vmExecuteOpCall(vm);
                break;

            case OP_RETURN_VALUE:
                err = vmExecuteOpReturnValue(vm);
                break;

            case OP_RETURN:
                err = vmExecuteOpReturn(vm);
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

static VmError_t vmExecuteOpConstant(Vm_t* vm, int32_t* ip) {
    Instructions_t ins = vmGetInstructions(vm);
    uint16_t constIndex = readUint16BigEndian(&ins[*ip + 1]);
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

    if (left->type == OBJECT_STRING && right->type == OBJECT_STRING) {
        return vmExecuteBinaryStringOperation(vm, op, (String_t*)left, (String_t*)right);
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

static VmError_t vmExecuteBinaryStringOperation(Vm_t *vm, OpCode_t op, String_t* left, String_t* right) {
    if (op != OP_ADD) {
        return VM_UNSUPPORTED_OPERATOR;
    }

    char* concatStr = strFormat("%s%s", left->value, right->value);
    String_t* strObj = createString(concatStr);
    free(concatStr);
    return vmPush(vm, (Object_t*) strObj);
}

static VmError_t vmExecuteComparison(Vm_t* vm, OpCode_t op) {
    Object_t* right = vmPop(vm);
    Object_t* left = vmPop(vm);

    if (left->type == OBJECT_INTEGER && right->type == OBJECT_INTEGER) {
        return vmExecuteIntegerComparison(vm, op, (Integer_t*)left, (Integer_t*)right);
    }

    if (left->type == OBJECT_BOOLEAN && right->type == OBJECT_BOOLEAN) {
        return vmExecuteBooleanComparison(vm, op, (Boolean_t*)left, (Boolean_t*)right);
    }

    return VM_UNSUPPORTED_TYPES; 
}

static VmError_t vmExecuteIntegerComparison(Vm_t* vm, OpCode_t op, Integer_t* left, Integer_t* right) {
    switch(op) {
        case OP_EQUAL:
            return vmPush(vm, nativeBoolToBooleanObject(right->value == left->value));
        case OP_NOT_EQUAL:
            return vmPush(vm, nativeBoolToBooleanObject(right->value != left->value));
        case OP_GREATER_THAN:
            return vmPush(vm, nativeBoolToBooleanObject(left->value > right->value));
        default:
            return VM_UNSUPPORTED_OPERATOR;
    }
}

static VmError_t vmExecuteBooleanComparison(Vm_t* vm, OpCode_t op, Boolean_t* left, Boolean_t* right) {
    switch(op) {
        case OP_EQUAL:
            return vmPush(vm, nativeBoolToBooleanObject(left->value == right->value));
        case OP_NOT_EQUAL:
            return vmPush(vm, nativeBoolToBooleanObject(left->value != right->value));
        default:
            return VM_UNSUPPORTED_OPERATOR; 
    }
}

static VmError_t vmExecuteBangOperator(Vm_t *vm) {
    Object_t* operand = vmPop(vm);

    switch(operand->type) {
        case OBJECT_BOOLEAN:    
            bool value = ((Boolean_t*)operand)->value; 
            return vmPush(vm, nativeBoolToBooleanObject(!value));
        case OBJECT_NULL:
            return vmPush(vm, nativeBoolToBooleanObject(true));
        default:
            return vmPush(vm, nativeBoolToBooleanObject(false));
    }
}

static VmError_t vmExecuteMinusOperator(Vm_t *vm) {
    Object_t* operand = vmPop(vm);
    if (operand->type != OBJECT_INTEGER) {
        return VM_UNSUPPORTED_TYPES;
    }

    int64_t value = ((Integer_t*)operand)->value;
    return vmPush(vm, (Object_t*)createInteger(-value));
}


static VmError_t vmExecuteOpArray(Vm_t* vm, int32_t* ip) {
    Instructions_t ins = vmGetInstructions(vm);
    uint16_t numElements = readUint16BigEndian(&ins[*ip + 1]);
    *ip += 2;

    Array_t* array = vmBuildArray(vm, numElements);
    return vmPush(vm, (Object_t*)array);
}

static Array_t* vmBuildArray(Vm_t* vm, uint16_t numElements) {
    // create array object using stack elements  
    Array_t* arr = createArray();
    arr->elements = createVectorObjects();
    for (uint16_t i = vm->sp - numElements; i< vm->sp; i++) {
        arrayAppend(arr, vm->stack[i]);
    }

    // remove elements from stack
    for (uint16_t i = vm->sp-numElements; i < vm->sp; i++) {
        vmPop(vm);
    }

    return arr; 
}

static VmError_t vmExecuteOpHash(Vm_t* vm, int32_t* ip) {
    Instructions_t ins = vmGetInstructions(vm);
    uint16_t numElements = readUint16BigEndian(&ins[*ip + 1]);
    *ip += 2;

    Hash_t* hash = vmBuildHash(vm, numElements);
    if (!hash) {
        return VM_INVALID_KEY;
    }
    return vmPush(vm, (Object_t*)hash);
}

static Hash_t* vmBuildHash(Vm_t* vm, uint16_t numElements) {
    Hash_t* hash = createHash();
    for(uint16_t i = vm->sp - numElements; i < vm->sp; i+= 2) {
        Object_t* key = vm->stack[i];
        Object_t* value = vm->stack[i+1];

        // check if key is hashable 
        // FIXME: this should emit a proper error code.  
        if (!objectIsHashable(key)) {
            return NULL;
        }

        HashPair_t* pair = createHashPair(key, value);
        hashInsertPair(hash, pair);
    }

    return hash;
}

static VmError_t vmExecuteOpIndex(Vm_t* vm) {
    Object_t* index = vmPop(vm);
    Object_t* left = vmPop(vm);

    if (left->type == OBJECT_ARRAY && index->type == OBJECT_INTEGER) {
        return vmExecuteArrayIndex(vm, (Array_t*)left, (Integer_t*)index);
    } else if (left->type == OBJECT_HASH) {
        return vmExecuteHashIndex(vm, (Hash_t*)left, index);
    }

    return VM_UNSUPPORTED_TYPES; 
}

static VmError_t vmExecuteArrayIndex(Vm_t* vm, Array_t*array, Integer_t* index) {
    uint32_t max = arrayGetElementCount(array);
    int64_t i = index->value;

    if (i < 0 || i >= max) {
        return vmPush(vm, (Object_t*) createNull());
    }

    Object_t** elems = arrayGetElements(array);
    return vmPush(vm, elems[i]);
}

static VmError_t vmExecuteHashIndex(Vm_t* vm, Hash_t* hash, Object_t* index) {
    if (!objectIsHashable(index)) {
        return VM_INVALID_KEY;
    }

    HashPair_t* pair = hashGetPair(hash, index);
    if (!pair) {
        return vmPush(vm, (Object_t*) createNull());
    }

    return vmPush(vm, pair->value);
}


static Object_t* nativeBoolToBooleanObject(bool val) {
    return (Object_t*)createBoolean(val);
}

static VmError_t vmExecuteOpBoolean(Vm_t* vm, OpCode_t op) {
    return vmPush(vm, nativeBoolToBooleanObject(op == OP_TRUE));
}

static VmError_t vmExecuteOpPop(Vm_t* vm) {
    vmPop(vm); 
    return VM_NO_ERROR;
}

static VmError_t vmExecuteOpJump(Vm_t* vm, int32_t* ip) {
    Instructions_t ins = vmGetInstructions(vm);
    uint16_t pos = readUint16BigEndian(&(ins[*ip + 1]));
    *ip = pos - 1;

    return VM_NO_ERROR; 
}

static VmError_t vmExecuteOpJumpNotTruthy(Vm_t* vm, int32_t* ip) {
    Instructions_t ins = vmGetInstructions(vm);
    uint16_t pos = readUint16BigEndian(&(ins[*ip + 1]));
    *ip += 2;

    Object_t* condition = vmPop(vm);
    if (!vmIsTruthy(condition)) {
        *ip = pos - 1;
    }

    return VM_NO_ERROR;
}

static VmError_t vmExecuteOpNull(Vm_t* vm) {
    return vmPush(vm, (Object_t*) createNull());
}

static VmError_t vmExecuteOpSetGlobal(Vm_t* vm, int32_t* ip) {
    Instructions_t ins = vmGetInstructions(vm);
    uint16_t globalIndex = readUint16BigEndian(&(ins[*ip + 1]));
    *ip += 2;

    if (vm->globals[globalIndex] != NULL)
        gcFreeExtRef(vm->globals[globalIndex]);
    vm->globals[globalIndex] = gcGetExtRef(vmPop(vm));
    return VM_NO_ERROR;
}

static VmError_t vmExecuteOpGetGlobal(Vm_t* vm, int32_t* ip) {
    Instructions_t ins = vmGetInstructions(vm);
    uint16_t globalIndex = readUint16BigEndian(&(ins[*ip + 1]));
    *ip += 2;

    return vmPush(vm, vm->globals[globalIndex]);
}


static VmError_t vmExecuteOpCall(Vm_t* vm) {
    Object_t* fn = vm->stack[vm->sp - 1];
    if (fn->type != OBJECT_COMPILED_FUNCTION) {
        return VM_CALL_NON_FUNCTION; 
    }
    Frame_t frame = createFrame((CompiledFunction_t*) fn);
    vmPushFrame(vm, &frame);

    return VM_NO_ERROR;
}

static VmError_t vmExecuteOpReturnValue(Vm_t* vm) {
    Object_t* returnValue = vmPop(vm);

    vmPopFrame(vm);
    vmPop(vm);

    return vmPush(vm, returnValue);
}

static VmError_t vmExecuteOpReturn(Vm_t* vm) {
    vmPopFrame(vm);
    vmPop(vm);

    return vmPush(vm, (Object_t*)createNull());
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

static bool vmIsTruthy(Object_t* obj) {
    switch(obj->type) {
        case OBJECT_BOOLEAN:
            return ((Boolean_t*)obj)->value;
        case OBJECT_NULL:
            return false;
        default:
            return true;
    }
}

static uint16_t readUint16BigEndian(uint8_t* ptr) {
    return (ptr[0] << 8) | ptr[1];
}
