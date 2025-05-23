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

// Simple token reader
token next_token(FILE *in) {
  char buf[1024]; // TODO: Dynamic
  skip_ws(in);
  int c = fgetc(in);
  token tok = {0};

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

  fprintf(stderr, "Unexpected char: '%c'\n", c);
  exit(EXIT_FAILURE);
}



/**
 * LISP values
 */
typedef enum {
  TYPE_CONS,
  TYPE_INT,
  TYPE_SYM,
  TYPE_NIL,
} valueType;

typedef struct value *value;

struct value {
  valueType type;
  union {
    struct {
      value car;
      value cdr;
    } cons;
    int i;
    char *sym;
  };
};

value value_alloc(valueType type) {
  value v = malloc(sizeof(value));
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
  value v = value_alloc(TYPE_SYM);
  v->sym = strdup(s);
  return v;
}

value value_new_nil() {
  static struct value nil = {.type = TYPE_NIL};
  return &nil;
}

#define car(v) ((v)->cons.car)
#define cdr(v) ((v)->cons.cdr)


void value_print(value v) {
    switch (v->type) {
        case TYPE_INT:
            printf("%d", v->i);
            break;
        case TYPE_SYM:
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
 * Simple parser
 */
value parse_expression(FILE *in);

value parse_list(FILE *in) {
  token t = next_token(in);
  
  if (t.type == TOK_RPAREN)
    return value_new_nil();

  ungetc(' ', in); // hack 
  
  value car_val = parse_expression(in);
  value cdr_val = parse_list(in);
  
  return value_new_cons(car_val, cdr_val);
}

value parse_expression(FILE *in) {
  token t = next_token(in);
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
  default:
    fprintf(stderr, "Unexpected token\n");
    exit(1);
  }
}
int main() {
    printf("Enter a Lisp expression:\n");
    value expr = parse_expression(stdin);
    printf("You entered: ");
    value_print(expr);
    printf("\n");
    return 0;
}
