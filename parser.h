#ifndef PARSER_H
#define PARSER_H

#include <stdio.h>
#include "util.h"

#define INST_LIMIT 1<<15


typedef struct {
	FILE* input;
	int maxwidth;
	LINELIST* output;
} PARSER;

PARSER* mkparser(FILE* input);
void parse(PARSER* p);
void freeparser(PARSER* p);
#endif
