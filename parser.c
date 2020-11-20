#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "parser.h"

void pushln(LINELIST** curln, char* tmpln, int lnind, int truen) {
	int size = (lnind+1)*sizeof(char);
	char* newcontent = (char*)malloc(size);
	strcpy(newcontent, tmpln);
	(*curln)->content = newcontent;
	(*curln)->truen = truen;
	LINELIST* nextln = (LINELIST*)malloc(sizeof(LINELIST));
	(*curln)->next = nextln;
	(*curln) = nextln;
}

void chop(PARSER* p) {
	char c;
	char tmpln[p->maxwidth];
	int lnind = 0;
	int lnscount = 0;
	int truelnscount = 0;
	
	LINELIST* firstln = (LINELIST*)malloc(sizeof(LINELIST));
	LINELIST* lastln;
	LINELIST* curln = firstln;

	bool comment = false;
	bool spacedln = false;
	while(c = fgetc(p->input), c != -1) {
		if(c == '\n') {
			if(comment) {
				comment = false;
				ungetc(c, p->input);
				continue;
			}
			truelnscount++;
			if(!lnind)
				continue;

			tmpln[lnind] = '\0';

			lastln = curln;
			pushln(&curln, tmpln, lnind, truelnscount);

			lnind = 0;
			spacedln = false;
			lnscount++;
			continue;
		}

		if(comment)
			continue;

		if(isspace(c)) {
			if(lnind)	
				spacedln = true;
			continue;
		}
		
		if(c == '/') {
			char nc = fgetc(p->input);
			if(nc == '/') {
				comment = true;
				continue;
			}
			ungetc(nc, p->input);
		}

		if(spacedln) {
			fprintf(stderr, "Unexpected char '%c'; line %i:%i\n", c, lnscount+1, lnind+1);
			exit(1);
		}
		
		tmpln[lnind] = c;
		lnind++;
	}
	fclose(p->input);
	free(curln);
	lastln->next = NULL;
	p->output = firstln;
}

void gatherinfo(PARSER* p) {
	char c;
	bool readsmt = false;
	bool comment = false;
	int lnwidth = 1;

	int lnscount = 0;
	int truelnscount = 1;

	p->maxwidth = 0;

	while(c = fgetc(p->input), c != -1) {
		if(c == '\n') {
			truelnscount++;
			comment = false;
			if(lnwidth > p->maxwidth)
				p->maxwidth = lnwidth;
			if(readsmt) {
				if(lnscount == INST_LIMIT) {
					fprintf(stderr, "Reached instruction limit (%i); line %i\n", INST_LIMIT, truelnscount);
					exit(1);
				}
				lnscount++;
			}
			readsmt = false;
			lnwidth = 1;
			continue;
		}
		if(comment)
			continue;
		if(c == '/') {
			char nc = fgetc(p->input);
			if(nc == '/') {
				comment = true;
				continue;
			}
			ungetc(nc, p->input);
		}
		if(isspace(c)) {
			continue;
		}
		readsmt = true;
		lnwidth++;
	}
	rewind(p->input);
}

PARSER* mkparser(FILE* input) {
	PARSER* p = (PARSER*)malloc(sizeof(PARSER));

	p->input = input;
};

void parse(PARSER* p) {
	gatherinfo(p);
	chop(p);
}
