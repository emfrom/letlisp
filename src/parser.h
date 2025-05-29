#ifndef PARSER_H
#define PARSER_H

#include <stdio.h>

#include "value.h"

value parse_expression(FILE *in, env e);
value parse_list(FILE *in, env e);
value parse_all(FILE *in, env e);
#endif
