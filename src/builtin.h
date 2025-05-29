#ifndef BUILTIN_H
#define BUILTIN_H

#include "env.h"
#include "value.h"

int bool_isnil(value args, env e);
int bool_istrue(value args, env e);

env builtins_startup(env e);

#endif
