#ifndef BUILTIN_H
#define BUILTIN_H

#include "value.h"
#include "env.h"

int bool_isnil(value args, env e);
int bool_istrue(value args, env e);

env builtins_startup(env e);

#endif
