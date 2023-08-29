#ifndef _EVALUATOR_H_
#define _EVALUATOR_H_

#include "object.h"
#include "env.h"
#include "ast.h"

Object_t* evalProgram(Program_t* prog, Environment_t* env);


#endif