#ifndef VALUE_H
#define VALUE_H

#include <stdint.h>

#ifndef ENV_H
typedef struct env_s *env;
#endif 

// Value types
typedef struct value_s *value;
typedef value (*function)(value args, env e);

typedef enum {
  TYPE_CONS,
  TYPE_INT,
  TYPE_SYMBOL,
  TYPE_NIL,
  TYPE_FUNCTION,
  TYPE_SPECIAL,
  TYPE_CLOSURE,
  TYPE_BOOL,
  TYPE_STRING
} valueType;

typedef struct {
  value params; // list of symbols
  value body;   // list of expressions
  env e;    // captured environment
} closure;

// Value Struct
struct value_s {
  valueType type;
  union {
    struct {
      value car;
      value cdr;
    } cons;
    int64_t i;
    char *sym;
    char *string;
    function fn;
    closure clo;
    int boolean;
  };
};

// Macros
#define car(v) ((v)->cons.car)
#define cdr(v) ((v)->cons.cdr)
#define cadr(v) (car(cdr(v)))
#define caar(v) (car(car(v)))
#define cdar(v) (cdr(car(v)))
#define cddr(v) (cdr(cdr(v)))


// Value functions
value value_alloc(valueType type);
value value_new_string(char *str);
value value_new_bool(int b);
value value_new_closure(value params, value body, env e);
value value_new_symbol(const char *text, env e);
value value_new_special(const char *text);
value value_new_cons(value car, value cdr);
value value_new_int(int x);
value value_new_function(function f);
value value_new_nil();

//Utils until I write better 
void value_print(value v);



#endif
