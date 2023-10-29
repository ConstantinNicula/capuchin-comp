#include "vm.h"
#include "utils.h"
#include "gc.h"
#include "builtin.h"

#define MAX_FRAMES 1024 

static void cleanupGlobals(Vm_t *vm); 
static void cleanupStack(Vm_t* vm);
static void cleanupConstants(Vm_t *vm); 
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
static VmError_t vmBuildHash(Vm_t* vm, uint16_t numElements, Hash_t** hash); 

static VmError_t vmExecuteOpIndex(Vm_t* vm); 
static VmError_t vmExecuteArrayIndex(Vm_t* vm, Array_t*array, Integer_t* index);
static VmError_t vmExecuteHashIndex(Vm_t* vm, Hash_t* hash, Object_t* index); 

static VmError_t vmExecuteOpCall(Vm_t* vm, int32_t* ip);
static VmError_t vmCallClosure(Vm_t* vm, Closure_t* cl, uint8_t numArgs); 
static VmError_t vmCallBuiltin(Vm_t* vm,Builtin_t* builtin, uint8_t numArgs); 
static VmError_t vmExecuteOpReturnValue(Vm_t* vm); 
static VmError_t vmExecuteOpReturn(Vm_t* vm); 

static VmError_t vmExecuteOpSetLocal(Vm_t* vm, int32_t* ip);
static VmError_t vmExecuteOpGetLocal(Vm_t* vm, int32_t* ip);

static VmError_t vmExecuteOpGetBuiltin(Vm_t* vm, int32_t* ip);
static VmError_t vmExecuteOpClosure(Vm_t* vm, int32_t* ip);
static VmError_t vmExecuteOpGetFree(Vm_t* vm, int32_t* ip); 

static VmError_t vmExecuteOpCurrentClosure(Vm_t* vm);

static VmError_t vmPush(Vm_t* vm, Object_t* obj); 
static Object_t* vmPop(Vm_t* vm);

static bool vmIsTruthy(Object_t* obj);
static Object_t* nativeBoolToBooleanObject(bool val);
static uint16_t readUint16BigEndian(uint8_t* ptr); 

VmError_t createVmError(VmErrorCode_t code, char* str) {
    return (VmError_t) {
        .code = code, 
        .str = str
    };
}

void cleanupVmError(VmError_t* err){ 
    if (!err && !err->str) return;
    free(err->str);
}



Vm_t createVm(Bytecode_t* bytecode) {
    return createVmWithStore(bytecode, NULL);
} 

Vm_t createVmWithStore(Bytecode_t* bytecode, Object_t** s)  {
    Frame_t* frames = callocChk(MAX_FRAMES * sizeof(Frame_t));
    CompiledFunction_t* mainFunction = createCompiledFunction(bytecode->instructions, 0, 0);
    Closure_t* mainClosure = createClosure(mainFunction, createVectorObjects());
    gcSetRef(mainClosure, GC_REF_COMPILE_CONSTANT);
    frames[0] = createFrame(mainClosure, 0);

    Object_t** globals = (s == NULL) ? callocChk(GLOBALS_SIZE * sizeof(Object_t*)) : s;
    return (Vm_t) {
        .constants = bytecode->constants,

        .stack = callocChk(STACK_SIZE * sizeof(Object_t*)),
        .sp = 0, 
        .lastPopped = NULL, 

        .externalStorage = (s != NULL),
        .globals = globals,
        
        .frames = frames,
        .frameIndex = 1,
    };
}

void cleanupVm(Vm_t *vm) {
    if (!vm) return;

    if (!vm->externalStorage) {
        cleanupGlobals(vm); 
        cleanupConstants(vm);
    }

    cleanupStack(vm);
    cleanupFrames(vm);

    gcForceRun();
}

static void cleanupConstants(Vm_t *vm) {
    uint32_t count = vectorObjectsGetCount(vm->constants);
    Object_t** objs = vectorObjectsGetBuffer(vm->constants);
    for(uint32_t i = 0; i < count; i++) {
        gcClearRef(objs[i], GC_REF_COMPILE_CONSTANT);
    }
    cleanupVectorObjects(&vm->constants, NULL);
}

static void cleanupGlobals(Vm_t *vm) {
    uint16_t i = 0; 
    while (vm->globals[i] != NULL && i < GLOBALS_SIZE) {
        gcClearRef(vm->globals[i], GC_REF_GLOBAL);
        i++;
    }
    free(vm->globals);
}

static void cleanupStack(Vm_t *vm) {
    while (vm->sp) { 
        vmPop(vm);
    }
    if(vm->lastPopped) gcClearRef(vm->lastPopped, GC_REF_STACK);
    free(vm->stack);
}

static void cleanupFrames(Vm_t* vm) {
    // first frame is created internally, so fn ref must be cleared manually.
    //gcClearRef(vm->frames[0].cl->fn, GC_REF_COMPILE_CONSTANT); 
    gcClearRef(vm->frames[0].cl, GC_REF_COMPILE_CONSTANT); 
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
    return vm->lastPopped;
}

VmError_t vmRun(Vm_t *vm) {
    VmError_t err = createVmError(VM_NO_ERROR, NULL);
    
    while (vmCurrentFrame(vm)->ip < (int32_t)sliceByteGetLen(frameGetInstructions(vmCurrentFrame(vm))) - 1) {
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
                err = vmExecuteOpCall(vm, &vmCurrentFrame(vm)->ip);
                break;

            case OP_RETURN_VALUE:
                err = vmExecuteOpReturnValue(vm);
                break;

            case OP_RETURN:
                err = vmExecuteOpReturn(vm);
                break;

            case OP_SET_LOCAL:
                err = vmExecuteOpSetLocal(vm, &vmCurrentFrame(vm)->ip);
                break;

            case OP_GET_LOCAL:
                err = vmExecuteOpGetLocal(vm, &vmCurrentFrame(vm)->ip);
                break;

            case OP_GET_BUILTIN:
                err = vmExecuteOpGetBuiltin(vm, &vmCurrentFrame(vm)->ip);
                break;

            case OP_CLOSURE:
                err = vmExecuteOpClosure(vm, &vmCurrentFrame(vm)->ip);
                break;

            case OP_GET_FREE:
                err = vmExecuteOpGetFree(vm, &vmCurrentFrame(vm)->ip); 
                break;
            
            case OP_CURRENT_CLOSURE:
                err = vmExecuteOpCurrentClosure(vm);
                break;

            default:
                break;
        }

        if (err.code != VM_NO_ERROR) {
            break;
        }

        //gcForceRun();
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

    return createVmError(VM_UNSUPPORTED_TYPES, strFormat("unsupported types for binary operation: %s %s", 
                objectTypeToString(left->type), 
                objectTypeToString(right->type))); 
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
            return createVmError(VM_UNSUPPORTED_OPERATOR, strFormat("unknown integer operator: %d", op));
    }

    return vmPush(vm, (Object_t*)createInteger(result));
}

static VmError_t vmExecuteBinaryStringOperation(Vm_t *vm, OpCode_t op, String_t* left, String_t* right) {
    if (op != OP_ADD) {
        return createVmError(VM_UNSUPPORTED_OPERATOR, strFormat("unknown string operator: %d", op));
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

    return createVmError(VM_UNSUPPORTED_TYPES, strFormat("unknown operator: %d (%s %s)", 
        op, objectTypeToString(left->type), objectTypeToString(right->type))); 
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
            return createVmError(VM_UNSUPPORTED_OPERATOR, strFormat("unknown operator: %d", op));
    }
}

static VmError_t vmExecuteBooleanComparison(Vm_t* vm, OpCode_t op, Boolean_t* left, Boolean_t* right) {
    switch(op) {
        case OP_EQUAL:
            return vmPush(vm, nativeBoolToBooleanObject(left->value == right->value));
        case OP_NOT_EQUAL:
            return vmPush(vm, nativeBoolToBooleanObject(left->value != right->value));
        default:
            return createVmError(VM_UNSUPPORTED_OPERATOR, strFormat("unknown operator: %dd", op)); 
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
        return createVmError(VM_UNSUPPORTED_TYPES, strFormat("unsupported type for negation: %s", 
            objectTypeToString(operand->type)));
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
    for (uint16_t i = 0; i < numElements; i++) {
        vmPop(vm);
    }

    return arr; 
}

static VmError_t vmExecuteOpHash(Vm_t* vm, int32_t* ip) {
    Instructions_t ins = vmGetInstructions(vm);
    uint16_t numElements = readUint16BigEndian(&ins[*ip + 1]);
    *ip += 2;

    Hash_t* hash;
    VmError_t err = vmBuildHash(vm, numElements, &hash);
    if (err.code != VM_NO_ERROR) {
        return err;
    }
    return vmPush(vm, (Object_t*)hash);
}

static VmError_t vmBuildHash(Vm_t* vm, uint16_t numElements, Hash_t** hash) {
    *hash = createHash();
    for(uint16_t i = vm->sp - numElements; i < vm->sp; i+= 2) {
        Object_t* key = vm->stack[i];
        Object_t* value = vm->stack[i+1];

        // check if key is hashable 
        if (!objectIsHashable(key)) {
            return createVmError(VM_INVALID_KEY, strFormat("unusable as hash key: %s", 
                objectTypeToString(key->type)));
        }

        HashPair_t* pair = createHashPair(key, value);
        hashInsertPair(*hash, pair);
    }
    
    // cleanup stack vars 
    for (uint16_t i = 0; i < numElements; i++) {
        vmPop(vm);
    }
    return createVmError(VM_NO_ERROR, NULL);
}

static VmError_t vmExecuteOpIndex(Vm_t* vm) {
    Object_t* index = vmPop(vm);
    Object_t* left = vmPop(vm);

    if (left->type == OBJECT_ARRAY && index->type == OBJECT_INTEGER) {
        return vmExecuteArrayIndex(vm, (Array_t*)left, (Integer_t*)index);
    } else if (left->type == OBJECT_HASH) {
        return vmExecuteHashIndex(vm, (Hash_t*)left, index);
    }

    return createVmError(VM_UNSUPPORTED_TYPES, strFormat("index operator not supported: %s", 
        objectTypeToString(left->type))); 
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
        return createVmError(VM_INVALID_KEY, strFormat("unusable as hash key: %s", 
            objectTypeToString(index->type)));
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
    return createVmError(VM_NO_ERROR, NULL);
}

static VmError_t vmExecuteOpJump(Vm_t* vm, int32_t* ip) {
    Instructions_t ins = vmGetInstructions(vm);
    uint16_t pos = readUint16BigEndian(&(ins[*ip + 1]));
    *ip = pos - 1;

    return createVmError(VM_NO_ERROR, NULL); 
}

static VmError_t vmExecuteOpJumpNotTruthy(Vm_t* vm, int32_t* ip) {
    Instructions_t ins = vmGetInstructions(vm);
    uint16_t pos = readUint16BigEndian(&(ins[*ip + 1]));
    *ip += 2;

    Object_t* condition = vmPop(vm);
    if (!vmIsTruthy(condition)) {
        *ip = pos - 1;
    }

    return createVmError(VM_NO_ERROR, NULL);
}

static VmError_t vmExecuteOpNull(Vm_t* vm) {
    return vmPush(vm, (Object_t*) createNull());
}

static VmError_t vmExecuteOpSetGlobal(Vm_t* vm, int32_t* ip) {
    Instructions_t ins = vmGetInstructions(vm);
    uint16_t globalIndex = readUint16BigEndian(&(ins[*ip + 1]));
    *ip += 2;

    if (vm->globals[globalIndex] != NULL)
        gcClearRef(vm->globals[globalIndex], GC_REF_GLOBAL);

    vm->globals[globalIndex] = vmPop(vm);
    gcSetRef(vm->globals[globalIndex], GC_REF_GLOBAL);
    return createVmError(VM_NO_ERROR, NULL);
}

static VmError_t vmExecuteOpGetGlobal(Vm_t* vm, int32_t* ip) {
    Instructions_t ins = vmGetInstructions(vm);
    uint16_t globalIndex = readUint16BigEndian(&(ins[*ip + 1]));
    *ip += 2;

    return vmPush(vm, vm->globals[globalIndex]);
}


static VmError_t vmExecuteOpCall(Vm_t* vm, int32_t* ip) {
    uint8_t numArgs = vmGetInstructions(vm)[*ip+1];
    *ip += 1;

    Object_t* callee = vm->stack[vm->sp - 1 - numArgs];

    if (!callee) {
        return createVmError(VM_CALL_NON_FUNCTION, strFormat("calling non function object: NULL")); 
    }

    switch(callee->type) {
        case OBJECT_CLOSURE:
            return vmCallClosure(vm, (Closure_t*) callee, numArgs);
        case OBJECT_BUILTIN:
            return vmCallBuiltin(vm, (Builtin_t*)callee, numArgs);
        default:
            return createVmError(VM_CALL_NON_FUNCTION, strFormat("calling non function object: %s", 
                objectTypeToString(callee->type))); 
    }
}

static VmError_t vmCallClosure(Vm_t* vm, Closure_t* cl, uint8_t numArgs) {
    if (numArgs != cl->fn->numParameters) {
        return createVmError(VM_CALL_WRONG_PARAMS, strFormat("wrong number of arguments: want=%d, got=%d", 
            cl->fn->numParameters, numArgs));
    }

    Frame_t frame = createFrame(cl, vm->sp - numArgs);
    vmPushFrame(vm, &frame);

    // zero out stack values and update stack pointer
    uint16_t newSp = frame.basePointer + cl->fn->numLocals;
    for(uint16_t i = vm->sp; i < newSp; i++) {
        vm->stack[i] = NULL;
    }     
    vm->sp = newSp;

    return createVmError(VM_NO_ERROR, NULL);
}

static VmError_t vmCallBuiltin(Vm_t* vm,Builtin_t* builtin, uint8_t numArgs) {
    VectorObjects_t* args = createVectorObjects();
    for (uint16_t i = 0; i < numArgs; i++) {
        vectorObjectsAppend(args, vm->stack[vm->sp - numArgs + i]);
    }

    Object_t* result = builtin->func(args);

    // cleanup stack (flag for deletion), update stack pointer
    uint16_t newSp =  vm->sp - numArgs - 1;
    for (uint16_t i = newSp; i < vm->sp; i++) {
        gcClearRef(vm->stack[i], GC_REF_STACK);
    }
    vm->sp = newSp;
    cleanupVectorObjects(&args, NULL);

    return vmPush(vm, result);
}

static VmError_t vmExecuteOpReturnValue(Vm_t* vm) {
    Object_t* returnValue = vmPop(vm);

    // cleanup stack (flag for deletion), update stack pointer
    Frame_t frame = vmPopFrame(vm);
    for (uint16_t i = frame.basePointer-1; i < vm->sp; i++) {
        gcClearRef(vm->stack[i], GC_REF_STACK);
    }
    vm->sp = frame.basePointer - 1;

    return vmPush(vm, returnValue);
}

static VmError_t vmExecuteOpReturn(Vm_t* vm) {
    // cleanup stack (flag for deletion), update stack pointer
    Frame_t frame = vmPopFrame(vm);
    for (uint16_t i = frame.basePointer-1; i < vm->sp; i++) {
        gcClearRef(vm->stack[i], GC_REF_STACK);
    }
    vm->sp = frame.basePointer - 1;

    return vmPush(vm, (Object_t*)createNull());
}

static VmError_t vmExecuteOpSetLocal(Vm_t* vm, int32_t* ip) {
    Instructions_t ins = vmGetInstructions(vm);
    uint8_t localIndex = ins[*ip + 1];
    *ip += 1;

    Frame_t* frame = vmCurrentFrame(vm);
    uint16_t stackIndex = frame->basePointer + localIndex;
    vm->stack[stackIndex] = vmPop(vm);
    gcSetRef(vm->stack[stackIndex], GC_REF_STACK);

    return createVmError(VM_NO_ERROR, NULL); 
}

static VmError_t vmExecuteOpGetLocal(Vm_t* vm, int32_t* ip) {
    Instructions_t ins = vmGetInstructions(vm);
    uint8_t localIndex = ins[*ip + 1];
    *ip += 1;

    Frame_t* frame = vmCurrentFrame(vm);
    return vmPush(vm, vm->stack[frame->basePointer + localIndex]);
}

static VmError_t vmExecuteOpGetBuiltin(Vm_t* vm, int32_t* ip) {
    Instructions_t ins = vmGetInstructions(vm);
    uint8_t builtinIndex = ins[*ip + 1];
    *ip += 1;

    Builtin_t* builtin = createBuiltin(getBuiltinByIndex(builtinIndex));
    return vmPush(vm, (Object_t*)builtin);
}

static VmError_t vmExecuteOpClosure(Vm_t* vm, int32_t* ip) {
    Instructions_t ins = vmGetInstructions(vm);
    uint16_t constIndex = readUint16BigEndian(&(ins[*ip + 1])); 
    uint8_t numFree = ins[*ip + 3];
    *ip += 3;

    Object_t* constant = vectorObjectsGetBuffer(vm->constants)[constIndex];
    if (constant->type != OBJECT_COMPILED_FUNCTION) {
        return createVmError(VM_CALL_NON_FUNCTION, strFormat("not a function: %d", constant->type));
    }

    VectorObjects_t* freeVars = createVectorObjects();
    for (uint8_t i = 0; i < numFree; i++) {
        vectorObjectsAppend(freeVars, vm->stack[vm->sp - numFree + i]);
    }

    // cleanup stack 
    for (uint8_t i = 0; i < numFree; i++) {
        vmPop(vm);
    }

    Closure_t* closure = createClosure((CompiledFunction_t*)constant, freeVars);
    return vmPush(vm, (Object_t*)closure);
}

static VmError_t vmExecuteOpGetFree(Vm_t* vm, int32_t* ip) {
    Instructions_t ins = vmGetInstructions(vm);
    uint8_t freeIndex = ins[*ip + 1];
    *ip += 1;

    Closure_t* currentClosure = vmCurrentFrame(vm)->cl;
    return vmPush(vm, currentClosure->free->buf[freeIndex]);
}

static VmError_t vmExecuteOpCurrentClosure(Vm_t* vm) {
    Closure_t* currentClosure = vmCurrentFrame(vm)->cl;
    return vmPush(vm, (Object_t*)currentClosure);
}

static VmError_t vmPush(Vm_t* vm, Object_t* obj) {
    if (vm->sp >= STACK_SIZE) {
        return createVmError(VM_STACK_OVERFLOW, strFormat("stack overflow sp(%d)", vm->sp));
    }
    gcSetRef(obj, GC_REF_STACK);
    vm->stack[vm->sp] = obj;
    vm->sp++;
    return createVmError(VM_NO_ERROR, NULL);
}

static Object_t* vmPop(Vm_t* vm) {
    if (vm->lastPopped) gcClearRef(vm->lastPopped, GC_REF_STACK);
    vm->lastPopped = vm->stack[vm->sp-1]; 
    vm->sp--;
    return vm->lastPopped;
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
