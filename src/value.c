#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "env.h"
#include "memory.h"
#include "value.h"

int is_special(const char *sym);
int bool_isnil(value value, env e);

/**
 * LISP values
 */
value value_alloc(valueType type) {
  value v = gcx_malloc(sizeof(struct value_s));

  v->type = type;
  return v;
}

/**
 * new_string
 *
 * Arg is copied as reference
 */
value value_new_string(char *str) {
  value v = value_alloc(TYPE_STRING);

  // Pass in allocated string
  // v->string = strdup(str);

  v->string = str;

  return v;
}

// Trying to be idiomatic
const struct value_s bool_true_obj = {.type = TYPE_BOOL, .boolean = 1};

const struct value_s bool_false_obj = {.type = TYPE_BOOL, .boolean = 0};

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

  v->sym = (char *)text;

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

  if (!bool_isnil(v, e)) {
    // fprintf(stderr,"Reusing symbol %s: ( %s, %p )\n", text, car(v)->sym,
    // car(v));
    return car(v);
  }

  // All special forms are already added
  assert(!is_special(text));

  return value_new_symbol_nolookup(text);
}

value value_new_cons(value car, value cdr) {
  value v = value_alloc(TYPE_CONS);

  car(v) = car;
  cdr(v) = cdr;
  return v;
}

value value_new_exact(mpq_ptr number) {
  value v = value_alloc(TYPE_NUM_EXACT);

  v->num_exact = number;

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
  case TYPE_NUM_EXACT:
    // Leave this one as it, dont use string function
    if (mpz_cmp_ui(mpq_denref(v->num_exact), 1) == 0) {
      gmp_printf("%Zd", mpq_numref(v->num_exact));
    } else {
      gmp_printf("%Qd", v->num_exact);
    }
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
      value_print(car(v));
      v = cdr(v);
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
