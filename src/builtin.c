#include <stdlib.h>
#include <stdio.h>

#include "builtin.h"
#include "value.h"
#include "env.h"
#include "repl.h"

/**
 *  Builtin functions
 */

value builtin_add(value args, env e) {
  int sum = 0;
  while (args->type == TYPE_CONS) {
    value v = args->cons.car;
    if (v->type != TYPE_INT)
      repl_error("Expected int\n");

    sum += v->i;
    args = args->cons.cdr;
  }
  return value_new_int(sum);
}

value builtin_sub(value args, env e) {
  if (args->type == TYPE_NIL)
    repl_error("sub: requires at least one argument\n");

  value first = args->cons.car;
  value rest = args->cons.cdr;

  if (rest->type == TYPE_NIL) {
    return value_new_int(0 - first->i); // unary: negate
  }

  int result = first->i;
  for (; rest->type != TYPE_NIL; rest = rest->cons.cdr) {
    value v = rest->cons.car;
    result -= v->i;
  }

  return value_new_int(result);
}

value builtin_mult(value args, env e) {
  int product = 1;

  while (args->type == TYPE_CONS) {
    value v = args->cons.car;
    if (v->type != TYPE_INT)
      repl_error("Expected int");

    product *= v->i;
    if(product < v->i)
      repl_error("Multiplication overflow");
    args = args->cons.cdr;
  }

  return value_new_int(product);
}

int bool_isnil(value args, env e) {
  //Needed since empty list is sometimes evaled to nil
  if(args->type == TYPE_NIL) 
    return 1;

  if(args->type == TYPE_CONS &&
     args->cons.car->type == TYPE_NIL)
     return 1;
  
  return 0;
}

value builtin_null_pred(value args, env e) {
  return value_new_bool(bool_isnil(args, e)); 
}

int bool_istrue(value args, env e) {
  if (args->type != TYPE_CONS)
    repl_error("true? needs one argument");

  if (args->cons.car->type == TYPE_BOOL)
    return args->cons.car->boolean;

  return 1; // everything else true
}

value builtin_true_pred(value args, env e) {
  return value_new_bool(bool_istrue(args,e));
}

int bool_isnumber(value args, env e) {
  if(args->type == TYPE_INT)
    return 1;

  return 0;
}

value builtin_isnumber(value args, env e) {
  return value_new_bool(bool_isnumber(args,e));
}

value builtin_lequ(value args, env e) {
    if (args->type == TYPE_NIL) {
        // Combosable logic dictates
        return value_new_bool(1);
    }

    
    value prev = args->cons.car;
    if (!bool_isnumber(prev,e)) {
        repl_error("<=: arguments must be numbers");
    }

    args = args->cons.cdr;

    while (args->type != TYPE_NIL) {
        value curr = args->cons.car;
        if (!bool_isnumber(curr,e)) {
            repl_error("<=: arguments must be numbers");
        }

        // Compare prev <= curr
        if (prev->i > curr->i) 
            return value_new_bool(0);

        prev = curr;
        args = args->cons.cdr;
    }

    return value_new_bool(1);
}


value builtin_load(value args, env e) {
    if (args->type != TYPE_CONS)
        repl_error("load need at least one argument");

    do {
        value file = args->cons.car;
        if (file->type != TYPE_STRING)
            repl_error("load takes strings as arguments");

	repl_eval_file(file->string, e);


        args = args->cons.cdr;
    } while (args->type == TYPE_CONS);

    return value_new_bool(1);
}

value builtin_cons(value args, env e) {
  if(args->type != TYPE_CONS ||
     args->cons.cdr->type != TYPE_CONS)
    repl_error("cons takes two arguments");
  
  value first = args->cons.car;
    value second = args->cons.cdr->cons.car;

    // Allocate new cons cell
    value new_cons = value_alloc(TYPE_CONS);
    new_cons->cons.car = first;
    new_cons->cons.cdr = second;

    return new_cons;
}

value builtin_car(value args, env e) {
  if(args->type != TYPE_CONS ||
     args->cons.car->type != TYPE_CONS)
    repl_error("Argument to car not a pair");
  
  return args->cons.car->cons.car;
}

value builtin_cdr(value args, env e) {
  if(args->type != TYPE_CONS ||
     args->cons.car->type != TYPE_CONS)
    repl_error("Argument to cdr not a pair");
  
  return args->cons.car->cons.cdr;
}

//Fist lisp voodo :)
value builtin_list(value args, env e) {
  return args;
}

value builtin_display(value args, env e) {
    if (args->type != TYPE_CONS)
        repl_error("display need at least one argument");

    do {
        value text = args->cons.car;
        if (text->type != TYPE_STRING)
            repl_error("display takes strings as arguments");

        printf("%s", text->string);

        args = args->cons.cdr;
    } while (args->type == TYPE_CONS);

    return value_new_bool(1);
}

value builtin_newline(value args, env e) {
  printf("\n");

  return value_new_bool(1);
}

value builtin_debug(value args, env e) {
  printf("\n0x%p\n", args);

  if(args->type == TYPE_CONS)
    printf("0x%p\n", args->cons.car);
  
  value_print(args);
  printf("\n\n");
  
  return value_new_bool(1);
}

value builtin_debugenv(value args, env e) {
  env_dump(e);

  return value_new_bool(1);
}

value builtin_eq_pred(value args, env e) {
  if(args->type != TYPE_CONS ||
     cdr(args)->type != TYPE_CONS ||
     cddr(args)->type != TYPE_NIL)
    repl_error("eq? takes exactly two arguments");
  
  return car(args) == cadr(args) ? value_new_bool(1) : value_new_bool(0);
}

value builtin_pair_pred(value args, env e) {
  if(args->type != TYPE_CONS ||
     cdr(args)->type != TYPE_NIL)
    repl_error("pair? takes exactly one argument");

  return car(args)->type == TYPE_CONS ? value_new_bool(1) : value_new_bool(0);
}


struct builtin_functions {
  char *name;
  function fn;
};

struct builtin_functions startup[] = {
    {"+", builtin_add},
    {"-", builtin_sub},
    {"*", builtin_mult},
    {"true?", builtin_true_pred},
    {"null?", builtin_null_pred},
    {"pair?", builtin_pair_pred},
    {"eq?", builtin_eq_pred},
    {"<=", builtin_lequ},
    {"load", builtin_load},
    {"cons", builtin_cons},
    {"car", builtin_car},
    {"cdr", builtin_cdr},
    {"list", builtin_list},
    {"display", builtin_display},
    {"newline", builtin_newline},
    {"debug", builtin_debug},
    {"debugenv", builtin_debugenv},
    {NULL, NULL}};

env builtins_startup(env e) {
  
  for (int i = 0; startup[i].name != NULL; i++) {

    //To void #include loop's 
    value symbol = value_alloc(TYPE_SYMBOL);
    value function = value_alloc(TYPE_FUNCTION);

    symbol->sym = startup[i].name;
    function->fn = startup[i].fn;
    env_set(e, symbol, function); 
  }

  return e;
}
