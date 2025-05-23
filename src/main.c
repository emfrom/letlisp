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

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <setjmp.h>
#include <stdarg.h>

// Functions
__attribute__((noreturn))
void repl_error(const char *fmt, ...);

/**
 * Alloc tracking
 */
void **alloc_list = NULL;
size_t alloc_count = 0;

void *track_malloc(size_t size) {
    void *ptr = malloc(size);
    if (ptr) {
        void **new_list = realloc(alloc_list, (alloc_count + 1) * sizeof(void *));
        if (!new_list) {
            free(ptr);
            return NULL; // Handle out-of-memory
        }
        alloc_list = new_list;
        alloc_list[alloc_count++] = ptr;
    }
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
  TOK_INT
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

//Store for next token
static token tok = {0};
int token_pushed = 0;

void token_push(token to_push) {
  assert(!token_pushed);
  
  tok = to_push;
  token_pushed = 1;
}

// Simple token reader
token token_getnext(FILE *in) {

  if(token_pushed) {
    token_pushed = 0;
    return tok;
  }

  
  char buf[1024]; // TODO: Dynamic
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

  if (isdigit(c) || (c == '-' && isdigit(peek(in)))) {
    int i = 0;
    buf[i++] = c;
    while (isdigit(peek(in)))
      buf[i++] = fgetc(in);
    buf[i] = '\0';
    tok.type = TOK_INT;
    tok.text = strdup(buf);
    return tok;
  }

  if (isalpha(c) || strchr("+-*/<=>!?_", c)) {
    int i = 0;
    buf[i++] = c;
    while (isalnum(peek(in)) || strchr("+-*/<=>!?_", peek(in)))
      buf[i++] = fgetc(in);
    buf[i] = '\0';
    tok.type = TOK_SYMBOL;
    tok.text = strdup(buf);
    return tok;
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
  TYPE_FUNCTION
} valueType;

//Value types 
typedef struct value *value;
typedef value (*function)(value args);

//Value Struct
struct value {
  valueType type;
  union {
    struct {
      value car;
      value cdr;
    } cons;
    int i;
    char *sym;
    function fn;
  };
};

value value_alloc(valueType type) {
  value v = track_malloc(sizeof(value));
  if (!v) {
    fprintf(stderr, "Momery nada<zip><nothing><dead>\n");
    exit(EXIT_FAILURE);
  }
  v->type = type;
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

value value_new_sym(const char *s) {
  value v = value_alloc(TYPE_SYMBOL);
  v->sym = strdup(s);
  return v;
}

value value_new_function(function f) {
    value v = value_alloc(TYPE_FUNCTION);
    v->fn = f;
    return v;
}

value value_new_nil() {
  static struct value nil = {.type = TYPE_NIL};
  return &nil;
}

void value_print(value v) {
    switch (v->type) {
        case TYPE_INT:
            printf("%d", v->i);
            break;
        case TYPE_SYMBOL:
            printf("%s", v->sym);
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
        default:
            printf("<unknown>");
    }
}

/**
 * Environment
 */
value global_env = NULL;
value eval(value v);

void env_set(value sym, value val) {
    assert(sym->type == TYPE_SYMBOL);

    if(NULL == global_env) {
      global_env = value_new_nil();
    }

    for (value e = global_env; e->type == TYPE_CONS; e = e->cons.cdr) {
            value pair = e->cons.car;
            if (strcmp(pair->cons.car->sym, sym->sym) == 0) {
                pair->cons.cdr = val;
                return;
            }
    }

    // New
    value pair = value_new_cons(sym, val);
    global_env = value_new_cons(pair, global_env);
}



value env_lookup(const char *name) {
    for (value e = global_env; e->type == TYPE_CONS; e = e->cons.cdr) {
            value pair = e->cons.car;
            if (strcmp(pair->cons.car->sym, name) == 0)
                return pair->cons.cdr;
    }
    repl_error("Unbound symbol: %s\n", name);
}


/**
 *  Builtin functions
 */


value builtin_add(value args) {
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


value builtin_sub(value args) {
    if (args->type == TYPE_NIL)
      repl_error("sub: requires at least one argument\n");


    value first = args->cons.car;
    value rest  = args->cons.cdr;

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

struct builtin_functions {
  char *name;
  function fn;
};

struct builtin_functions startup[] = {
  { "add", builtin_add },
  { "sub", builtin_sub },
  { NULL, NULL }
};

void startup_load_builtins() {
  for(int i = 0; startup[i].name != NULL; i++) {
    value symbol = value_alloc(TYPE_SYMBOL);
    value function = value_alloc(TYPE_FUNCTION);

    symbol->sym = startup[i].name;
    function->fn = startup[i].fn;
    env_set(symbol, function);

  }
  
}


/**
 * Base evalation
 */

value eval_list(value lst) {
    if (lst->type == TYPE_NIL)
        return lst;
    
    return value_new_cons(eval(lst->cons.car), eval_list(lst->cons.cdr));
}


value eval_define(value args) {
    value sym = args->cons.car;

    if (sym->type != TYPE_SYMBOL)
        repl_error("define: first argument must be a symbol");

    value expr = args->cons.cdr->cons.car;
    value val = eval(expr);
    env_set(sym, val);
    return sym;
}

value eval_quote(value args) {
  return args->cons.car;
}


value eval(value v) {
    switch (v->type) {
    case TYPE_INT:
    case TYPE_NIL:
    case TYPE_FUNCTION:
        return v;

    case TYPE_SYMBOL:
        return env_lookup(v->sym);

    case TYPE_CONS: {
        value head = v->cons.car;

        if (head->type == TYPE_SYMBOL) {
            if (strcmp(head->sym, "define") == 0)
                    return eval_define(v->cons.cdr);
	    
            if (strcmp(head->sym, "quote") == 0)
                    return eval_quote(v->cons.cdr);
            // more 
        }

        value fn = eval(head);
        value args = eval_list(v->cons.cdr);
        if (fn->type == TYPE_FUNCTION)
            return fn->fn(args);

        repl_error("Not a function\n");
    }

    default:
        repl_error("Unknown type in eval\n");
        exit(1);
    }
}

/**
 * Simple parser
 */
value parse_expression(FILE *in);

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
    return value_new_cons(value_new_sym("quote"),
                          value_new_cons(parse_expression(in), value_new_nil()));

  case TOK_INT:
    return value_new_int(atoi(t.text));

  case TOK_SYMBOL:
    return value_new_sym(t.text);

  case TOK_RPAREN:
    repl_error("Unexpected ')' outside list\n");

  case TOK_EOF:
    repl_error("Unexpected EOF\n");
  
  default:
    repl_error("Unknown token type\n");
  }
}



jmp_buf repl_env;

void repl_error(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);

    fprintf(stderr, "Error: ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");

    va_end(ap);
    longjmp(repl_env, 1);
}


void repl() {
    char line[1024];

    for(;;) {
        if (setjmp(repl_env) != 0) {
            // After an error
            printf("Error recovered.\n");
        }

        printf("lispy> ");
        if (!fgets(line, sizeof(line), stdin)) break;

        FILE *input = fmemopen(line, strlen(line), "r");
        value expr = parse_expression(input);
        fclose(input);

        value result = eval(expr);
        value_print(result);
        printf("\n");
    }

    printf("bye.\n");
}



int main() {
  //Setup free on exit
  atexit(free_all_allocs);

  //Load builtins
  startup_load_builtins();

  //Go for it
  repl();
  
  return EXIT_SUCCESS;
}
