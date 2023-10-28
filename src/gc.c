#include <malloc.h>

#include "object.h"
#include "utils.h"
#include "gc.h"

typedef struct GCHandle {
    void* first;
    uint32_t objCount;
}GCHandle_t;

static GCHandle_t gcHandle = {.first = NULL, .objCount = 0 };

// Additional header data used for GC 
// *--------*---------
// | header |  data....
// *--------*----------
//          |-> return ptr 

typedef struct GCDataHeader {
    uint32_t mark;
    void* next;
} GCDataHeader_t;

// Mark bits significance 
// *-----------*-----+-----+-----+
// | 31-4 SRC  | CRB | GRB | IRB |
// *-----------*-----+-----+-----+
// SRC - stack ref counter
// CRB - constant ref bit
// GRB - global ref bit
// IRB - internal ref bit

#define MARK_UNUSED 0x00
#define INTERNAL_REF_BIT 0x01 
#define GLOBAL_REF_BIT 0x02
#define CONSTANT_REF_BIT 0x04

#define STACK_REF_SHIFT 3u
#define STACK_REF_MASK 0xFFFFFFF8

// Used to check if stack, global, constant  references exist
#define EXTERNAL_REF_MASK 0xFFFFFFFE

/* External definitions */
extern void gcCleanupObject(Object_t** obj);
extern void gcMarkObject(Object_t* obj);


/* Create global table of destructors */

static void* createFatPtr(size_t size, void* next);
static GCDataHeader_t* getHeader(void *ptr);
static void *getPtr(GCDataHeader_t* header);

static inline void setBit(GCDataHeader_t* header, uint32_t bitmask);
static inline void clearBit(GCDataHeader_t* header, uint32_t bitmask);
static inline bool isBitSet(GCDataHeader_t* header, uint32_t bitmask);

static inline void incStackRef(GCDataHeader_t* header);
static inline void decStackRef(GCDataHeader_t* header);
static inline bool hasStackRef(GCDataHeader_t* header);

static void gcMark();
static void gcSweep();
static void gcDebugPrintChain(void* ptr) ;


/************************************ 
 *   Public function definitions    *
 ************************************/

void* gcMalloc(size_t size) {
    void* ptr = createFatPtr(size, gcHandle.first);

    GCDataHeader_t* header = getHeader(ptr);
    header->next = gcHandle.first;

    gcHandle.first = ptr;
    gcHandle.objCount++;
    return ptr;
}

void gcFree(void* ptr) {
    free(getHeader(ptr));
    gcHandle.objCount--;
}

void gcSetRef(void* ptr, GCRefType_t refType) {
    if (!ptr) return;
    GCDataHeader_t* header = getHeader(ptr);
    switch(refType) {
        case GC_REF_INTERNAL:
            setBit(header, INTERNAL_REF_BIT);
            break;
        case GC_REF_GLOBAL:
            setBit(header, GLOBAL_REF_BIT); 
            break;
        case GC_REF_COMPILE_CONSTANT:
            setBit(header, CONSTANT_REF_BIT);
            break;
        case GC_REF_STACK:
            incStackRef(header); 
            break; 
    }
}

void gcClearRef(void* ptr, GCRefType_t refType) {
    if (!ptr) return;
    GCDataHeader_t* header = getHeader(ptr);
    switch(refType) {
        case GC_REF_INTERNAL:
            clearBit(header, INTERNAL_REF_BIT);
            break;
        case GC_REF_GLOBAL:
            clearBit(header, GLOBAL_REF_BIT); 
            break;
        case GC_REF_COMPILE_CONSTANT:
            clearBit(header, CONSTANT_REF_BIT);
            break;
        case GC_REF_STACK:
            decStackRef(header); 
            break; 
    }
}

bool gcHasRef(void* ptr, GCRefType_t refType) {
    if (!ptr) false;
    GCDataHeader_t* header = getHeader(ptr);
    
    switch(refType) {
        case GC_REF_INTERNAL:
            return isBitSet(header, INTERNAL_REF_BIT);
        case GC_REF_GLOBAL:
            return isBitSet(header, GLOBAL_REF_BIT); 
        case GC_REF_COMPILE_CONSTANT:
            return isBitSet(header, CONSTANT_REF_BIT);
        case GC_REF_STACK:
            return hasStackRef(header); 
    }
    return false;
}


void gcForceRun() {
    // perform mark & sweep round
    gcMark();
    gcSweep();
}
/************************************ 
 *   Static function definitions    *
 ************************************/

static void* createFatPtr(size_t size, void* next) {
    GCDataHeader_t* ptr = malloc(sizeof(GCDataHeader_t) + size);
    if (!ptr) HANDLE_OOM();
    *ptr = (GCDataHeader_t) {
        .mark = 0u, 
        .next = next
    };

    return (void*)((char*)ptr + sizeof(GCDataHeader_t));
}

static GCDataHeader_t* getHeader(void *ptr) {
    if(!ptr) return NULL;
    return (GCDataHeader_t*)((char*)ptr - sizeof(GCDataHeader_t));
}

static void *getPtr(GCDataHeader_t* header) {
    if (!header) return NULL;
    return (void*)((char*)header + sizeof(GCDataHeader_t));
}

static void gcMark() {
    void* ptr = gcHandle.first;
    while (ptr) {
        GCDataHeader_t* header = getHeader(ptr);
        if (isBitSet(header, EXTERNAL_REF_MASK)) {
            gcMarkObject(ptr);
        }
        ptr = header->next;
    }
}


static void gcSweep() {
    if (!gcHandle.first) return;

    GCDataHeader_t sentinel = {.next = gcHandle.first};
    GCDataHeader_t* prev = &sentinel;
    GCDataHeader_t* curr = getHeader(gcHandle.first);
    
    while (curr) {
        if (!isBitSet(curr, INTERNAL_REF_BIT)) {
            // remove element 
            prev->next = curr->next;
            void* cptr = getPtr(curr);

            // cleanup element              
            Object_t **tmp = (Object_t**)&cptr;
            gcCleanupObject(tmp);

            curr = getHeader(prev->next);
        } else  {
            clearBit(curr, INTERNAL_REF_BIT);
            prev = curr; 
            curr = getHeader(curr->next);
        }
    }

    gcHandle.first = sentinel.next;
}


static void gcDebugPrintChain(void* ptr) {
    //printf("-----START-----\n");
    while (ptr) {
        printf("ptr(%d|%d)@0x%p\n",
            isBitSet(getHeader(ptr), GLOBAL_REF_BIT),
            isBitSet(getHeader(ptr), INTERNAL_REF_BIT),
            ptr);
        ptr = getHeader(ptr)->next;
    }
    //printf("-----END-----\n");
}


static inline void setBit(GCDataHeader_t* header, uint32_t bitmask) {
    header->mark |= bitmask;
}
static inline void clearBit(GCDataHeader_t* header, uint32_t bitmask) {
    header->mark &= (~bitmask);
}
static inline bool isBitSet(GCDataHeader_t* header, uint32_t bitmask){
    return (header->mark & bitmask) != 0;
}

static inline void incStackRef(GCDataHeader_t* header) {
    uint32_t count = (header->mark >> STACK_REF_SHIFT) + 1;
    header->mark = (header->mark & ~STACK_REF_MASK) | (count << STACK_REF_SHIFT); 
}

static inline void decStackRef(GCDataHeader_t* header) {
    uint32_t count = (header->mark >> STACK_REF_SHIFT);
    if (count) {
        count --;
        header->mark = (header->mark & ~STACK_REF_MASK) | (count << STACK_REF_SHIFT); 
    }
}

static inline bool hasStackRef(GCDataHeader_t* header) {
    return (header->mark >> STACK_REF_SHIFT) != 0; 
}

