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

void repl(env e) {
  //Load history 
  using_history();
  read_history(HISTORY_FILE);

  //Loop 
  for (;;) {
    if (setjmp(repl_env) != 0) {

      // After an error
      printf("Error recovered.\n");
    }

    //Read 
    char *input_string;
    input_string = readline("minilisp> ");
    
    if(!input_string) //EOF
      break;

    if (*input_string) 
      add_history(input_string);      

    //    printf("%s\n", input_string);

    FILE *input;
    input = fmemopen(input_string, strlen(input_string), "r");
    
    value expr = parse_expression(input);
    fclose(input);
    free(input_string);
    
    //Eval 
    value result = eval(expr, e);

    //Print 
    value_print(result);
    printf("\n");
  }

  //Write history
  write_history(HISTORY_FILE);
  
  printf("bye.\n");
}
