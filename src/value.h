#ifndef VALUE_H
#define VALUE_H

#include "env.h"

// Value types
typedef struct value_s *value;
typedef value (*function)(value args, env e);

//Value functions 
value value_new_string(char *str);
value value_new_bool(int b);
value value_new_closure(value params, value body, env e);
value value_new_symbol(const char *text);
value value_new_cons(value car, value cdr);
value value_new_int(int x);
value value_new_function(function f);
value value_new_nil();

//Utils until I write better 
void value_print(value v);



#endif
