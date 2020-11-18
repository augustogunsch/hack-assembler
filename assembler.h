#ifndef ASSEMBLER_H
#define ASSEMBLER_H
#include <stdio.h>

#define RAM_LIMIT 24577
#define TOP_VAR 16383
#define BOTTOM_VAR 16
#define INST_SIZE 17
#define C_TOKEN_SIZE 4
#define INST_LIMIT 1<<15

typedef struct {
	union {
		char* name;
		char* content;
	};
	union {
		int value;
		int truen;
	};
} SYMBOL;

typedef struct {
	SYMBOL** items;
	int count;
	int size;
} SYMBOLARRAY;

typedef struct {
	FILE* input;

	int maxwidth;
	int truelnscount;
	int lncount;

	SYMBOLARRAY* lns;

	SYMBOLARRAY* labels;

	SYMBOLARRAY* vars;
} ASSEMBLER;

ASSEMBLER* mkassembler(FILE* input);
void preprocess(ASSEMBLER* a);
void translate(ASSEMBLER* a);
void freeassembler(ASSEMBLER* a);
#endif
