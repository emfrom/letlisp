/*
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#include <assert.h>
#include <ctype.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <gc/gc.h>

// TODO: segv in the repl

// TODO: Parse with regex
// TODO: Continuations (for io as well?)
// TODO: (define (func a b c) ( .. )) form
// TODO: Multiline input to readline
// TODO: Finish refactor into small files



// Stuff
#include "env.h"
#include "value.h"
#include "parser.h"
#include "eval.h"
#include "repl.h"
#include "builtin.h"

// Functions
int is_special(const char *sym);


/**
 * LISP values
 */
value value_alloc(valueType type) {
  value v = GC_MALLOC(sizeof(struct value_s));
  v->type = type;
  return v;
}

value value_new_string(char *str) {
  value v = value_alloc(TYPE_STRING);
  v->string = strdup(str);
  return v;
}


// Trying to be idiomatic
const struct value_s bool_true_obj = {
  .type = TYPE_BOOL,
  .boolean = 1
};

const struct value_s bool_false_obj = {
    .type = TYPE_BOOL,
    .boolean = 0
};

value value_new_bool(int b) {
  value bool_true = (value)&bool_true_obj;
  value bool_false = (value)&bool_false_obj;

  return b ? bool_true : bool_false;
}

value value_new_closure(value params, value body, env e) {
  value v = value_alloc(TYPE_CLOSURE);
  v->clo.params = params;
  v->clo.body = body;
  v->clo.e = e;
  return v;
}

value value_new_special(const char *text) {
  value v = value_alloc(TYPE_SPECIAL);
  
  v->sym = (char *) text;
  
  return v;
}

value value_new_symbol_nolookup(const char *text) {
  value v = value_alloc(TYPE_SYMBOL);


  v->sym = strdup(text);


  /* fprintf(stderr, "New symbol %s: ( %s , %p)\n",
	  text,
	  v->sym,
	  v);
  */
  
  return v;
}

value value_new_symbol(const char *text, env e) {

  value v = env_exists(e, text);

  if(!bool_isnil(v,e)) {
    //fprintf(stderr,"Reusing symbol %s: ( %s, %p )\n", text, car(v)->sym, car(v));
    return car(v);
  }
  //All special forms are already added 
  assert(!is_special(text));

  return value_new_symbol_nolookup(text);
}

value value_new_cons(value car, value cdr) {
  value v = value_alloc(TYPE_CONS);
  v->cons.car = car;
  v->cons.cdr = cdr;
  return v;
}

value value_new_int(int x) {
  value v = value_alloc(TYPE_INT);
  v->i = x;
  return v;
}

value value_new_function(function f) {
  value v = value_alloc(TYPE_FUNCTION);
  v->fn = f;
  return v;
}

value value_new_nil() {
  static struct value_s nil = {.type = TYPE_NIL};
  return &nil;
}

void value_print(value v) {
  switch (v->type) {
  case TYPE_INT:
    printf("%li", v->i);
    break;
    
  case TYPE_SYMBOL:
    printf("%s", v->sym);
    break;
  case TYPE_STRING:
    printf("%s", v->string);
    break;
    
  case TYPE_NIL:
    printf("()");
    break;
    
  case TYPE_CONS:
    printf("(");
    while (v->type == TYPE_CONS) {
      value_print(v->cons.car);
      v = v->cons.cdr;
      if (v->type == TYPE_CONS)
        printf(" ");
    }
    if (v->type != TYPE_NIL) {
      printf(" . ");
      value_print(v);
    }
    printf(")");
    break;
    
  case TYPE_BOOL:
    if (v->boolean)
      printf("#t");
    else
      printf("#f");
    break;
    
  case TYPE_CLOSURE:
  case TYPE_FUNCTION:
   case TYPE_SPECIAL:
    printf("<function>");
        break;

  default:
    printf("<unknown>");
  }
}

/**
 * Environment
 *
 * All env modifications are done by prepending, first occurence of symbol in the alist is
 *  the symbols value in that context
 *
 * Todo: Do I need to remove duplicates? (later, later)
 */
struct env_s {
    env parent;   // Outer scope environment (NULL for global)
    value bindings;       // alist of (symbol . value) pairs for this scope
};

env env_new(env parent) {
    env e = GC_MALLOC(sizeof(struct env_s));
    e->parent = parent;
    e->bindings = value_new_nil(); 
    return e;
}

void env_set(env e, value sym, value val) {
  // New
  value pair = value_new_cons(sym, val);
  e->bindings = value_new_cons(pair, e->bindings);
}

env env_extend(env parent, value params, value args) {
  env e = env_new(parent);
  while (params->type == TYPE_CONS && args->type == TYPE_CONS) {
    env_set(e, params->cons.car, args->cons.car);
    params = params->cons.cdr;
    args = args->cons.cdr;
  }
  return e;
}

value env_exists(env e, const char *name) {
  for (; e != NULL; e = e->parent)
    for (value bind = e->bindings; bind->type == TYPE_CONS;
         bind = cdr(bind)) {
      value pair = car(bind);

      if (strcmp(car(pair)->sym, name) == 0) {
        return pair;
      }
    }

  return value_new_nil();
}

value env_lookup(env e, const char *name) {
  value v = env_exists(e, name);

  if(bool_isnil(v, e))
    repl_error("Unbound symbol: %s\n", name);

  return cdr(v);
}

void env_dump(env e) {
  //Debug
  value v = e->bindings;

  while(!bool_isnil(v, e)) {
    value p = car(v);

    printf("%s(%s): %p\n", car(p)->sym, car(p)->type == TYPE_SPECIAL ? "spec" : "sym", cdr(p));

    v = cdr(v);
  }
}

/**
 * Base evalation
 */

value eval_list(value lst, env e) {
    if (lst->type == TYPE_NIL)
        return lst;

    value head = eval(lst->cons.car, e);
    value tail = eval_list(lst->cons.cdr, e);
    return value_new_cons(head, tail);
}

value eval_define(value args, env e) {
  value sym = car(args);

    if (sym->type != TYPE_SYMBOL)
        repl_error("define: first argument must be a symbol (for now)");

    value expr = args->cons.cdr->cons.car;
    value val = eval(expr, e);

    env_set(e, sym, val);
    
    return sym;
}


value eval_quote(value args, env e) {
    return args->cons.car;
}

value eval_lambda(value args, env e){
        value params = args->cons.car;             // first argument
        value body = args->cons.cdr;               // rest of list

        return value_new_closure(params, body, e);
}

value eval_apply_closure(value fn, value arg_exprs, env calling_env) {
  assert(fn->type == TYPE_CLOSURE);
  value params = fn->clo.params;
  value body = fn->clo.body;
  env closure_env = fn->clo.e;

  value args = eval_list(arg_exprs, calling_env); // eval args in calling env

  // Bind params to args
  env new_env = env_extend(closure_env, params, args);

  // Evaluate body in new_env
  value result = value_new_nil();
  
  while (body->type == TYPE_CONS) {
    result = eval(body->cons.car, new_env);
    body = body->cons.cdr;
  }
  
  return result;
}

value eval_if(value args, env env) {
    if (args->type != TYPE_CONS)
        repl_error("if: missing arguments");

    // Extract predicate
    value predicate = args->cons.car;

    // Extract then branch
    if (args->cons.cdr->type != TYPE_CONS)
        repl_error("if: missing then branch");
    
    value then_branch = args->cons.cdr->cons.car;

    // Extract else branch (optional)
    value else_branch = value_new_nil();
    if (args->cons.cdr->cons.cdr->type == TYPE_CONS)
        else_branch = args->cons.cdr->cons.cdr->cons.car;

    // Evaluate predicate
    value cond = eval(predicate, env);

    if (bool_istrue(cond,env))
        return eval(then_branch, env);
    else
        return eval(else_branch, env);
}

const char *valueTypeNames[] = {
    "cons",
    "int",
    "symbol",
    "nil",
    "function",
    "special",
    "closure",
    "bool",
    "string"
};

value eval(value v, env e) {
        switch (v->type) {
        case TYPE_INT:
        case TYPE_NIL:
        case TYPE_FUNCTION:
	case TYPE_BOOL:
	case TYPE_STRING:
	case TYPE_SPECIAL:
        return v;

        case TYPE_SYMBOL:
        return env_lookup(e, v->sym);

        case TYPE_CONS: {
        value head = v->cons.car;

        if (head->type == TYPE_SPECIAL)
          return eval_special(head, v->cons.cdr, e);

        value fn = eval(head, e);
        value args = v->cons.cdr;

        if (fn->type == TYPE_CLOSURE)
          return eval_apply_closure(fn, args, e);

        if (fn->type == TYPE_FUNCTION)
          return fn->fn(eval_list(args, e), e);

        repl_error("Not a function");
        }

        default:
	  repl_error("Unknown type in eval: %s\n", valueTypeNames[v->type]);
        exit(1);
        }
}

/**
 * Special forms
 */

value eval_or(value args, env e) {

  while (args->type == TYPE_CONS) {
    value current = car(args);
    value result = eval(current, e);

    // Short 
    if (result->type != TYPE_BOOL || result->boolean) 
      return result;
    

    args = cdr(args);
  }

  return value_new_bool(0);
}

value eval_and(value args, env e) {
  while (args->type == TYPE_CONS) {
    value current = car(args);
    value result = eval(current, e);

    //short
    if (result->type == TYPE_BOOL && !result->boolean)
      return result;

    args = cdr(args);
  }

  return value_new_bool(1);
}

value eval_eval(value args, env env) {
    if (args->type != TYPE_CONS)
        repl_error("eval: expected 1 argument");

    value expr = args->cons.car;

    if (args->cons.cdr->type != TYPE_NIL)
        repl_error("eval: too many arguments");

    return eval(expr, env);
}


value eval_np(value args, env e) {
  repl_error("Special form not implemented");
}

static const char *special_forms[] = {
  "lambda", "define", "quote",
  "if", "or", "and", "eval", NULL};

static const function special_handlers[] = {eval_lambda, eval_define,
                                            eval_quote, eval_if, eval_or,
					    eval_and, eval_eval };

int is_special(const char *sym) {
  for (int i = 0; special_forms[i] != NULL; i++)
    if (strcmp(sym, special_forms[i]) == 0)
      return 1;

  return 0;
}

value eval_special(value head, value args,env e) {
  for (int i = 0; special_forms[i] != NULL; i++)
    if (strcmp(head->sym, special_forms[i]) == 0)
      return special_handlers[i](args,e);

  repl_error("Unknown special form: %s", head->sym);
}

env special_startup(env e) {
  for (int i = 0; special_forms[i] != NULL; i++)
    env_set(e,
	    value_new_special(special_forms[i]),
	    value_new_function(special_handlers[i]));

  return e;
}

int main() {

  // Load builtins
  env global_env = env_new(NULL);
  global_env = builtins_startup(global_env);
  global_env = special_startup(global_env);

  // Load lisp startup
  if(setjmp(repl_env) == 0)
    repl_eval_file("minilisp.lsp", global_env);
  else
    fprintf(stderr,"Error in startup file\n");
  
  // Go for it
  repl(global_env);

  return EXIT_SUCCESS;
}
