#ifndef PARSER_H
#define PARSER_H

#include <stdio.h>

#include "value.h"

value parse_expression(FILE *in);
value parse_list(FILE *in);

#endif
