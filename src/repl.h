#ifndef REPL_H
#define REPL_H

#include "env.h"

// Return to repl prompt on error
__attribute__((noreturn)) void repl_error(const char *fmt, ...);

// Main repl
void repl(env e);

#endif
