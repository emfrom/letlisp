#ifndef EVAL_H
#define EVAL_H

#include "env.h"
#include "value.h"

value eval_special(value head, value args, env e);
value eval(value v, env e);

#endif
