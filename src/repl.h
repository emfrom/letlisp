#ifndef REPL_H
#define REPL_H

#include <stdio.h>

#include "env.h"

// Return to repl prompt on error
__attribute__((noreturn)) void repl_error(const char *fmt, ...);

// Main repl
void repl(env e);

// eval and print last resultj
void repl_eval_print(FILE *in, env e);

#endif
