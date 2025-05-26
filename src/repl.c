#include <stdio.h>
#include <setjmp.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>

#include <readline/readline.h>
#include <readline/history.h>


#include "repl.h"
#include "env.h"
#include "value.h"
#include "parser.h"
#include "eval.h"
#include "builtin.h"

//Error jmp point 
jmp_buf repl_env;


#define HISTORY_FILE "minilisp_history"

void repl_error(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);

  fprintf(stderr, "Error: ");
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");

  va_end(ap);
  longjmp(repl_env, 1);
}

value repl_eval(FILE *in, env e) {

  value expr = parse_all(in, e);

  if (bool_isnil(expr, e))
    return value_new_nil();

  //Prepend a begin for idiomatic REPL behaviour 
  expr = value_new_cons(value_new_symbol("begin",e),
			expr);
			
  return eval(expr, e);
}

value repl_eval_file(char *filename, env e) {
  value ret;

  FILE *fp = fopen(filename, "r");
  if (!fp)
    repl_error("error: could not open file '%s'\n", filename);

  ret = repl_eval(fp, e);

  fclose(fp);
  return ret;
}

void repl(env e) {
  //Load history 
  using_history();
  read_history(HISTORY_FILE);

  //Loop 
  for (;;) {
    if (setjmp(repl_env) != 0)
      printf("Error recovered.\n");


    //Read 
    char *input_string;
    input_string = readline("minilisp> ");
    
    if(!input_string) //EOF
      break;

    if (*input_string) 
      add_history(input_string);      

    FILE *input;
    input = fmemopen(input_string, strlen(input_string), "r");

    //Eval and Print
    value_print(repl_eval(input, e));
    printf("\n");

    fclose(input);
    free(input_string);

  }

  //Write history
  write_history(HISTORY_FILE);
  
  printf("bye.\n");
}
