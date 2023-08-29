#ifndef _ENVIRONMENT_H_
#define _ENVIRONMENT_H_

#include "hmap.h"
#include "object.h"


typedef struct Object Object_t;

typedef struct Environment {
    HashMap_t* store;
    struct Environment* outer;
} Environment_t;

Environment_t* createEnvironment(Environment_t* outer);
Environment_t* copyEnvironment(const Environment_t* env);

Object_t* environmentGet(Environment_t* env, const char* name);
Object_t* environmentSet(Environment_t* env, const char* name, Object_t* obj);

#endif