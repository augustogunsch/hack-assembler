#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include "assembler.h"

char* getoutname(char* fname, int fnamelen) {
	char* outf = (char*)malloc(sizeof(char) * fnamelen + 2);
	strcpy(outf, fname);
	// .hack
	strcpy( (outf + ((fnamelen * sizeof(char)) - (3 * sizeof(char)))), "hack");
	return outf;
}

int main(int argc, char* argv[]) {
	if(argc < 2) {
		printf("Usage: %s {input}\n", argv[0]);
		return 1;
	}

	int fnamelen = strlen(argv[1]);
	int invalidext = strcmp(argv[1]+(fnamelen*sizeof(char)-(4*sizeof(char))), ".asm");
	if(invalidext) {
		fprintf(stderr, "Invalid extension (must be named lide Xxx.asm)\n");
		return 1;
	}
	FILE* input = fopen(argv[1], "r");

	if(input == NULL) {
		fprintf(stderr, "%s\n", strerror(errno));
		return errno;
	}
	
	PARSER* p = mkparser(input);
	parse(p);

	ASSEMBLER* a = mkassembler(p->output);

	// variable substitution
	preprocess(a);
	
	// actual translation
	translate(a);

	// file output
	char* outf = getoutname(argv[1], fnamelen);
	FILE* output = fopen(outf, "w");

	if(output == NULL) {
		fprintf(stderr, "%s\n", strerror(errno));
		return errno;
	}
	
	printlns(a->lns, output);

	free(outf);
	freeassembler(a);
	fclose(output);
	return 0;
}
