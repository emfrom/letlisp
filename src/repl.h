#ifndef REPL_H
#define REPL_H

#include <stdio.h>

#include "env.h"
#include "value.h"

// Return to repl prompt on error
__attribute__((noreturn)) void repl_error(const char *fmt, ...);

// Main repl
void repl(env e);

// eval and print last resultj
value repl_eval(FILE *in, env e);

#endif
