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
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

// TODO: Readline for input
// TODO: Parse with regex
// TODO: Continuations (for io as well?)

// Stuff
#include "repl.h"
#include "env.h"
#include "value.h"
#include "parser.h"
#include "eval.h"

// Functions
int is_special(const char *sym);


/**
 * Alloc tracking
 */
void **alloc_list = NULL;
size_t alloc_count = 0;

void *track_malloc(size_t size) {
  void *ptr = malloc(size);
  if (!ptr) {
    fprintf(stderr, "Out of memory\n");
    exit(EXIT_FAILURE);
  }

  alloc_list = realloc(alloc_list, (alloc_count + 1) * sizeof(void *));
  if (!alloc_list) {
    fprintf(stderr, "Out of memory\n");
    exit(EXIT_FAILURE);
  }
  alloc_list[alloc_count++] = ptr;

  return ptr;
}

void free_all_allocs(void) {
  for (size_t i = 0; i < alloc_count; ++i)
    free(alloc_list[i]);
  free(alloc_list);
}

/**
 * Minimalist lexer
 */
typedef enum {
  TOK_EOF,
  TOK_LPAREN,
  TOK_RPAREN,
  TOK_QUOTE,
  TOK_SYMBOL,
  TOK_INT,
  TOK_BOOL,
  TOK_STRING
} tokenType;

typedef struct {
  tokenType type;
  char *text;
} token;

int peek(FILE *in) {
  int c = fgetc(in);
  if (c != EOF)
    ungetc(c, in);
  return c;
}

void skip_ws(FILE *in) {
  while (isspace(peek(in)))
    fgetc(in);
}

// Store for next token
static token tok = {0};
int token_pushed = 0;

void token_push(token to_push) {
  assert(!token_pushed);

  tok = to_push;
  token_pushed = 1;
}

// Simple token reader
token token_getnext(FILE *in) {

  if (token_pushed) {
    token_pushed = 0;
    return tok;
  }

  char buf[1024];
  tok.text = buf;
  skip_ws(in);
  int c = fgetc(in);

  if (c == EOF) {
    tok.type = TOK_EOF;
    return tok;
  }

  if (c == '(') {
    tok.type = TOK_LPAREN;
    return tok;
  }

  if (c == ')') {
    tok.type = TOK_RPAREN;
    return tok;
  }

  if (c == '\'') {
    tok.type = TOK_QUOTE;
    return tok;
  }

  if (c == '#') {
    tok.type = TOK_BOOL;
    buf[0] = '#';
    buf[1] = fgetc(in);
    buf[2] = '\0';

    if(!strchr("tf",buf[1])) 
      repl_error("Malformated input: %s\n", buf);

    //tok.text = strdup(buf);

    return tok;
  }
  
  if (isdigit(c) || (c == '-' && isdigit(peek(in)))) {
    int i = 0;
    buf[i++] = c;
    while (isdigit(peek(in)))
      buf[i++] = fgetc(in);
    buf[i] = '\0';
    tok.type = TOK_INT;

    //tok.text = strdup(buf);

    return tok;
  }

  if (isalpha(c) || strchr("+-*/<=>!?_\"", c)) {
    int i = 0;
    buf[i++] = c;
    while (isalnum(peek(in)) || strchr(":+-*/<=>!?_", peek(in)))
      buf[i++] = fgetc(in);
    buf[i] = '\0';

    tok.type = TOK_SYMBOL;
    if (buf[0] == '\"' && buf[i - 1] == '\"') {
      tok.type = TOK_STRING;
      tok.text = track_malloc(i);
      buf[i - 1] = '\0';
      strcpy(tok.text, buf + 1);
    }

    if (!strchr(tok.text, '\"'))
      return tok;
    repl_error("Stray \" in input");
  }

  repl_error("Unexpected char: '%c'\n", c);
}

/**
 * LISP values
 */
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

// Value types
typedef value (*function)(value args, env e);

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
    int bool;
  };
};

value value_alloc(valueType type) {
  value v = track_malloc(sizeof(struct value_s));
  v->type = type;
  return v;
}

value value_new_string(char *str) {
  value v = value_alloc(TYPE_STRING);
  v->string = str;
  return v;
}

value value_new_bool(int b) {
  value v = value_alloc(TYPE_BOOL);
  v->bool = b;
  return v;
}

value value_new_closure(value params, value body, env e) {
  value v = value_alloc(TYPE_CLOSURE);
  v->clo.params = params;
  v->clo.body = body;
  v->clo.e = e;
  return v;
}

value value_new_symbol(const char *text) {
  value v = value_alloc(TYPE_SYMBOL);

  if (is_special(text))
    v->type = TYPE_SPECIAL;

  v->sym = strdup(text);

  return v;
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
    if (v->bool)
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
    env e = track_malloc(sizeof(struct env_s));
    e->parent = parent;
    e->bindings = value_new_nil(); 
    return e;
}

void env_set(env e, value sym, value val) {
  assert(sym->type == TYPE_SYMBOL);

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

value env_lookup(env e, const char *name) {
  for (; e != NULL; e = e->parent)
    for (value bind = e->bindings;
	 bind->type == TYPE_CONS;
         bind = bind->cons.cdr) {
      value pair = bind->cons.car;

      if (strcmp(pair->cons.car->sym, name) == 0)
        return pair->cons.cdr;
    }

  repl_error("Unbound symbol: %s\n", name);
}

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


int bool_istrue(value args, env e) {
    if (args->type == TYPE_BOOL)
        return args->bool;
    if (args->type == TYPE_NIL)
        return 0;
    
    return 1; // everything else true
}

value builtin_istrue(value args, env e) {
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

struct builtin_functions {
  char *name;
  function fn;
};

struct builtin_functions startup[] = {
    {"+", builtin_add},
    {"-", builtin_sub},
    {"*", builtin_mult},
    {"true?", builtin_istrue},
    {"<=", builtin_lequ}, 
    {NULL, NULL}};

env startup_load_builtins() {
  env e = env_new(NULL);
  
  for (int i = 0; startup[i].name != NULL; i++) {
    value symbol = value_alloc(TYPE_SYMBOL);
    value function = value_alloc(TYPE_FUNCTION);

    symbol->sym = startup[i].name;
    function->fn = startup[i].fn;
    env_set(e, symbol, function);
  }

  return e;
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
    value sym = args->cons.car;

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


value eval(value v, env e) {
        switch (v->type) {
        case TYPE_INT:
        case TYPE_NIL:
        case TYPE_FUNCTION:
	case TYPE_BOOL:
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
        repl_error("Unknown type in eval\n");
        exit(1);
        }
}

/**
 * Special forms
 */

value eval_np(value args, env e) {
  repl_error("Special form not implemented");
}

static const char *special_forms[] = {"lambda", "define", "quote",
                                      "if",     "let",    NULL};

static const function special_handlers[] = {eval_lambda, eval_define,
                                            eval_quote, eval_if, eval_np};

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

/**
 * Simple parser
 */

value parse_list(FILE *in) {
  token t = token_getnext(in);

  if (t.type == TOK_RPAREN)
    return value_new_nil();

  token_push(t);

  value car_val = parse_expression(in);
  value cdr_val = parse_list(in);

  return value_new_cons(car_val, cdr_val);
}

value parse_expression(FILE *in) {
  token t = token_getnext(in);
  switch (t.type) {
  case TOK_LPAREN:
    return parse_list(in);

  case TOK_QUOTE:
    return value_new_cons(
        value_new_symbol("quote"),
        value_new_cons(parse_expression(in), value_new_nil()));

  case TOK_BOOL:
    return value_new_bool(t.text[1] == 't');
    
  case TOK_INT:
    return value_new_int(atoi(t.text));

  case TOK_SYMBOL:
    return value_new_symbol(t.text);

  case TOK_STRING:
    return value_new_string(t.text);

  case TOK_RPAREN:
    repl_error("Unexpected ')' outside list");

  case TOK_EOF:
    repl_error("Unexpected EOF");

  default:
    repl_error("Unknown token type");
  }
}


int main() {
  // Setup free on exit
  atexit(free_all_allocs);

  // Load builtins
  env global_env;
  global_env = startup_load_builtins();

  // Go for it
  repl(global_env);

  return EXIT_SUCCESS;
}
