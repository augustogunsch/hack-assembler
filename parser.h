#ifndef PARSER_H
#define PARSER_H
#include <stdio.h>

#define INST_LIMIT 1<<15

typedef struct lnls {
	char* content;
	int truen;
	struct lnls* next;
} LINELIST;

typedef struct {
	FILE* input;
	int maxwidth;
	LINELIST* output;
} PARSER;

PARSER* mkparser(FILE* input);
void parse(PARSER* p);
#endif
