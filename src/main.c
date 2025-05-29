
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
#include <gmp.h>
#include <setjmp.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// TODO: Parse with regex
// TODO: Continuations (for io as well?)
// TODO: Multiline input to readline
// TODO: Finish refactor into small files

// Stuff
#include "builtin.h"
#include "env.h"
#include "eval.h"
#include "memory.h"
#include "parser.h"
#include "repl.h"
#include "value.h"

// Functions
int is_special(const char *sym);

/**
 *  Numbers and utilities
 */
mpq_ptr num_exact_new() {
  mpq_ptr new = gcx_malloc(sizeof(mpq_t));

  mpq_init(new);

  return new;
}

/**
 * Base evalation
 */

value eval_list(value lst, env e) {
  if (lst->type == TYPE_NIL)
    return lst;

  value head = eval(car(lst), e);
  value tail = eval_list(cdr(lst), e);

  return value_new_cons(head, tail);
}

value eval_define(value args, env e) {
  value sym = car(args);

  // (define symbol expr)
  if (sym->type == TYPE_SYMBOL) {
    value expr = car(cdr(args));
    value val = eval(expr, e);

    env_set(e, sym, val);
    return sym;
  }

  // define function
  else if (sym->type == TYPE_CONS) {
    value func_name = car(sym);

    // TODO: This breaks renaming special forms (dont really care f.n.)
    if (func_name->type != TYPE_SYMBOL)
      repl_error("define: function name must be a symbol");

    value params = cdr(sym);
    value body = cdr(args);

    value lambda_sym = value_new_symbol("lambda", e);
    value lambda_expr =
        value_new_cons(lambda_sym, value_new_cons(params, body));

    value val = eval(lambda_expr, e);

    env_set(e, func_name, val);
    return func_name;
  }

  repl_error("define: invalid syntax");
}

value eval_quote(value args, env e) { return car(args); }

value eval_lambda(value args, env e) {
  value params = car(args); // first argument
  value body = cdr(args);   // rest of list

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
    result = eval(car(body), new_env);
    body = cdr(body);
  }

  return result;
}

value eval_if(value args, env env) {
  if (args->type != TYPE_CONS)
    repl_error("if: missing arguments");

  // Extract predicate
  value predicate = car(args);

  // Extract then branch
  if (cdr(args)->type != TYPE_CONS)
    repl_error("if: missing then branch");

  value then_branch = cadr(args);

  // Extract else branch (optional)
  value else_branch = value_new_nil();

  if (cddr(args)->type == TYPE_CONS)
    else_branch = car(cddr(args));

  // Evaluate predicate
  value cond = eval(predicate, env);

  if (bool_istrue(cond, env))
    return eval(then_branch, env);
  else
    return eval(else_branch, env);
}

const char *valueTypeNames[] = {"cons",    "int",      "symbol",
                                "nil",     "function", "special",
                                "closure", "bool",     "string"};

value eval(value v, env e) {
  switch (v->type) {
  case TYPE_NUM_EXACT:
  case TYPE_NIL:
  case TYPE_FUNCTION:
  case TYPE_BOOL:
  case TYPE_STRING:
  case TYPE_SPECIAL:
    return v;

  case TYPE_SYMBOL:
    return env_lookup(e, v->sym);

  case TYPE_CONS: {
    value head = car(v);

    if (head->type == TYPE_SPECIAL)
      return eval_special(head, cdr(v), e);

    value fn = eval(head, e);
    value args = cdr(v);

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

    // short
    if (result->type == TYPE_BOOL && !result->boolean)
      return result;

    args = cdr(args);
  }

  return value_new_bool(1);
}

value eval_eval(value args, env env) {
  if (args->type != TYPE_CONS)
    repl_error("eval: expected 1 argument");

  value expr = car(args);

  if (cdr(args)->type != TYPE_NIL)
    repl_error("eval: too many arguments");

  return eval(expr, env);
}

// Argument list in an unevaled sequence of sexps to eval
value eval_begin(value args, env e) {
  value result = value_new_nil();

  for (; args->type == TYPE_CONS; args = cdr(args))
    result = eval(car(args), e);

  return result;
}

value eval_cond(value args, env e) {
  for (; args->type == TYPE_CONS; args = cdr(args)) {
    value clause = car(args);

    if (clause->type != TYPE_CONS)
      repl_error("cond: invalid clause");

    value predicate = car(clause);

    if (predicate->type == TYPE_SYMBOL && strcmp(predicate->sym, "else") == 0) {
      // (else expr1 expr2 ...)
      return eval_begin(cdr(clause), e);
    }

    value result = eval(predicate, e);

    if (bool_istrue(result, e)) {
      // (predicate expr1 expr2 ...)
      return eval_begin(cdr(clause), e);
    }
  }

  return value_new_nil(); // no clause matched
}

value eval_let(value args, env e) {

  if (args->type != TYPE_CONS)
    repl_error("let: missing bindings and body");

  value bindings = car(args);
  value body = cdr(args);

  if (bindings->type != TYPE_CONS && bindings->type != TYPE_NIL)
    repl_error("let: bindings must be a list");

  env new_env = env_extend(e, value_new_nil(), value_new_nil());

  // Iterate over bindings
  for (; bindings->type == TYPE_CONS; bindings = cdr(bindings)) {
    value bind = car(bindings);

    if (bind->type != TYPE_CONS || cdr(bind)->type != TYPE_CONS ||
        cddr(bind)->type != TYPE_NIL)
      repl_error("let: each binding must be (var expr)");

    value var = car(bind);
    value expr = cadr(bind);

    if (var->type != TYPE_SYMBOL)
      repl_error("let: binding variable must be a symbol");

    value val = eval(expr, e); // old environment !!

    env_set(new_env, var, val);
  }

  return eval_begin(body, new_env);
}

value eval_np(value args, env e) { repl_error("Special form not implemented"); }

static const char *special_forms[] = {"lambda", "define", "quote", "if",
                                      "or",     "and",    "eval",  "begin",
                                      "cond",   "let",    NULL};

static const function special_handlers[] = {
    eval_lambda, eval_define, eval_quote, eval_if,   eval_or,
    eval_and,    eval_eval,   eval_begin, eval_cond, eval_let};

int is_special(const char *sym) {
  for (int i = 0; special_forms[i] != NULL; i++)
    if (strcmp(sym, special_forms[i]) == 0)
      return 1;

  return 0;
}

value eval_special(value head, value args, env e) {
  for (int i = 0; special_forms[i] != NULL; i++)
    if (strcmp(head->sym, special_forms[i]) == 0)
      return special_handlers[i](args, e);

  repl_error("Unknown special form: %s", head->sym);
}

env special_startup(env e) {
  for (int i = 0; special_forms[i] != NULL; i++)
    env_set(e, value_new_special(special_forms[i]),
            value_new_function(special_handlers[i]));

  return e;
}

int main() {
  // Memory
  mem_startup();

  // Load builtins
  env global_env = env_new(NULL);

  global_env = builtins_startup(global_env);
  global_env = special_startup(global_env);

  // Load lisp startup
  if (setjmp(repl_env) == 0)
    repl_eval_file("letlisp.lsp", global_env);
  else
    fprintf(stderr, "Error in startup file\n");

  // Go for it
  repl(global_env);

  return EXIT_SUCCESS;
}
