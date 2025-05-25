#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <assert.h>
#include <string.h>
#include <gc/gc.h>


#include "value.h"
#include "repl.h"
#include "parser.h"

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

//Temp buffer for token data
char buf[1024];

// Simple token reader
token token_getnext(FILE *in) {

  if (token_pushed) {
    token_pushed = 0;
    return tok;
  }

  //TODO: More robust
  tok.text = buf;

  //Skip all whitespace
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

  //Number
  if (isdigit(c) || (c == '-' && isdigit(peek(in)))) {
    int i = 0;
    buf[i++] = c;
    while (isdigit(peek(in)))
      buf[i++] = fgetc(in);
    buf[i] = '\0';

    tok.type = TOK_INT;
    return tok;
  }

  // Symbol
  if (isalpha(c) || strchr("+-*/<=>!?_", c)) {
    int i = 1;
    buf[0] = c;
    
    while (isalnum(peek(in)) || strchr("+-*/<=>!?_", peek(in))) 
      buf[i++] = fgetc(in);

    
    buf[i] = '\0';

    tok.type = TOK_SYMBOL;
    return tok;
  }

  // String literal
  if (c == '\"') {
    int i = 0;

    // TODO: Add escapes
    c = fgetc(in);
    while (isprint(c) && '\"' != c) {
          buf[i] = c;
	  c = fgetc(in);
	  i += 1;
    }

    buf[i] = '\0';

    if(c != '\"')
      repl_error("Unterminated string literal: %s", buf);
     

    tok.type = TOK_STRING;
    return tok;
  }
  
  repl_error("Unexpected char: '%c'\n", c);
}

/**
 * Simple parser
 */

value parse_list(FILE *in, env e) {
  token t = token_getnext(in);

  if(t.type == TOK_EOF)
    repl_error("Unbalanced parenthesis");
  
  if (t.type == TOK_RPAREN)
    return value_new_nil();

  token_push(t);

  value car_val = parse_expression(in,e);
  value cdr_val = parse_list(in, e);

  return value_new_cons(car_val, cdr_val);
}

value parse_expression(FILE *in, env e) {
  token t = token_getnext(in);

  if(t.type == TOK_EOF) {
    return value_new_nil();
  }
  
  switch (t.type) {
  case TOK_LPAREN:
    return parse_list(in, e);

  case TOK_QUOTE:
    return value_new_cons(value_new_symbol("quote", e),
        value_new_cons(parse_expression(in,e), value_new_nil()));

  case TOK_BOOL:
    return value_new_bool(t.text[1] == 't');
    
  case TOK_INT:
    return value_new_int(atoi(t.text));

  case TOK_SYMBOL:
    return value_new_symbol(t.text, e);

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

