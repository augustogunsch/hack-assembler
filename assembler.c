#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdbool.h>
#include "tables.h"
#include "assembler.h"
#include "util.h"

void expandsymbols(SYMBOLARRAY* a, int toaddn);
void pushsymbol(SYMBOLARRAY* a, SYMBOL* s);
void freesymbol(SYMBOL* s);
SYMBOL* mksymbol(char* name, int namesize, int val);
int getsymbol(ASSEMBLER* a, char* name);
void skipln(ASSEMBLER* a);
void readrest(ASSEMBLER* a, int trueln);
int isvar(char* var);
void initsymbols(SYMBOLARRAY* s);
ASSEMBLER* mkassembler(FILE* input);
void populatevars(ASSEMBLER* a);
SYMBOL* readlabel(ASSEMBLER* a, int trueln);
void chop(ASSEMBLER* a);
void replacevar(SYMBOL* ln, int val);
void preprocess(ASSEMBLER* a);
void transa(SYMBOL* ln);
char* lookctable(TABLE* t, bool cond, char* token, const char* fieldname, int trueln);
void transb(SYMBOL* ln);
void translate(ASSEMBLER* a);
void gatherinfo(ASSEMBLER* a);
void freeassembler(ASSEMBLER* a);

void expandsymbols(SYMBOLARRAY* a, int toaddn) {
	int sum = a->count + toaddn;
	if(sizeof(SYMBOL*) * sum > a->size) {
		a->size = sizeof(SYMBOL*) * sum * 3;
		a->items = (SYMBOL**)realloc(a->items, a->size);
	}
}

void pushsymbol(SYMBOLARRAY* a, SYMBOL* s) {
	expandsymbols(a, 1);
	a->items[a->count] = s;
	a->count++;
}

void freesymbol(SYMBOL* s) {
	free(s->name);
	free(s);
}

void freesymbols(SYMBOLARRAY* a) {
	for(int i = 0; i < a->count; i++)
		freesymbol(a->items[i]);
	free(a->items);
	free(a);
}

SYMBOL* mksymbol(char* name, int namesize, int val) {
	SYMBOL* s = (SYMBOL*)malloc(sizeof(SYMBOL));
	char* heapname = (char*)malloc(namesize);
	strcpy(heapname, name);
	s->name = heapname;
	s->value = val;
	return s;
}

int getsymbol(ASSEMBLER* a, char* name) {
	for(int i = 0; i < a->vars->count; i++)
		if(strcmp(a->vars->items[i]->name, name)  == 0)
			return a->vars->items[i]->value;

	for(int i = 0; i < a->labels->count; i++)
		if(strcmp(a->labels->items[i]->name, name)  == 0)
			return a->labels->items[i]->value;
	
	return -1;
}

void skipln(ASSEMBLER* a) {
	char c;
	while(c = fgetc(a->input), c != -1)
		if(c == '\n')
			break;
}

void readrest(ASSEMBLER* a, int trueln) {
	char c;
	while(c = fgetc(a->input), c != -1) {
		if(c == '\n')
			break;
		if(isspace(c))
			continue;
		if(c == '/') {
			char nc = fgetc(a->input);
			if(nc == '/') {
				skipln(a);
				break;
			}
			ungetc(nc, a->input);
		}
		fprintf(stderr, "Unexpected '%c' at line '%i'\n", c, trueln);
		exit(1);
	}
}

int isvar(char* var) {
	int i = 0;
	while(1) {
		if(var[i] == '\0')
			break;
		if(!isdigit(var[i]))
			return 1;
		i++;
	}
	return 0;
}

void initsymbols(SYMBOLARRAY* s) {
	s->size = s->count * sizeof(SYMBOL*);
	s->items = (SYMBOL**)malloc(s->size);
	s->count = 0;
}

ASSEMBLER* mkassembler(FILE* input) {
	ASSEMBLER* a = (ASSEMBLER*)malloc(sizeof(ASSEMBLER));
	a->lns = (SYMBOLARRAY*)malloc(sizeof(SYMBOLARRAY));
	a->labels = (SYMBOLARRAY*)malloc(sizeof(SYMBOLARRAY));
	a->vars = (SYMBOLARRAY*)malloc(sizeof(SYMBOLARRAY));
	a->input = input;

	gatherinfo(a);

	initsymbols(a->lns);
	initsymbols(a->labels);
	a->vars->count = 80; // arbitrary number for initial size
	initsymbols(a->vars);

	populatevars(a);
	chop(a);

	return a;
}

void populatevars(ASSEMBLER* a) {
	const int firstamnt = 5;
	const int ramvamnt = 16; //max: 19
	const int specialamt = 2;
	const int sum = firstamnt + ramvamnt + specialamt;

	// realloc if necessary
	expandsymbols(a->vars, sum);
	
	// update varscount to new index
	a->vars->count = sum;

	// first five
	char* labels[] = { "SP", "LCL", "ARG", "THIS", "THAT" };
	for(int i = 0; i < firstamnt; i++) {
		a->vars->items[i] = mksymbol(labels[i], strlen(labels[i])+1, i);
	}

	// RAM variables (R0-R15)
	const int asciioff = 48;
	char ramvname[4];
	ramvname[0] = 'R';
	ramvname[2] = '\0';
	int tmptarg = (ramvamnt/10)*10;
	for(int i = 0; i < tmptarg; i++) {
		ramvname[1] = (char)(i+asciioff);
		a->vars->items[firstamnt+i] = mksymbol(ramvname, 3, i);
	}
	ramvname[1] = '1';
	ramvname[3] = '\0';
	for(int i = 10; i < ramvamnt; i++) {
		ramvname[2] = (char)((i%10)+asciioff);
		a->vars->items[firstamnt+i] = mksymbol(ramvname, 4, i);
	}

	// SCREEN var
	a->vars->items[firstamnt+ramvamnt] = mksymbol("SCREEN", 7, 16384);
	// KBD var
	a->vars->items[firstamnt+ramvamnt+1] = mksymbol("KBD", 4, 24576);
}

SYMBOL* readlabel(ASSEMBLER* a, int trueln) {
	char* name = (char*)malloc(sizeof(char)*(a->maxwidth-1));
	int i = 0;
	char c;
	int maxind = a->maxwidth-2;
	while(c = fgetc(a->input), c != -1) {
		if(c == ')')
			break;
		if(i == maxind) {
			fprintf(stderr, "Label width bigger than the maximum (%i characters); line %i\n", 
				maxind, trueln+1);
			exit(1);
		}
		if(c == '\n') {
			fprintf(stderr, "Unexpected end of line; line %i\n", trueln+1);
			exit(1);
		}
		if(isspace(c) || c == '(') {
			fprintf(stderr, "Unexpected '%c'; line %i\n", c, trueln+1);
			exit(1);
		}
		name[i] = c;
		i++;
	}
	name[i] = '\0';
	readrest(a, trueln);
	SYMBOL* l = (SYMBOL*)malloc(sizeof(SYMBOL));
	l->name = name;
	l->value = a->lns->count;
	return l;
}

// Splits the stream into an array of strings, stripping comments, white spaces and labels
// Requires vars array to check for duplicate symbols, but doesn't modify it
void chop(ASSEMBLER* a) {
	char c;
	char tmpln[a->maxwidth];
	int lnind = 0;
	int lnscount = 0;
	int truelnscount = 1;

	bool comment = false;
	bool spacedln = false;
	while(c = fgetc(a->input), c != -1) {
		if(c == '\n') {
			if(comment) {
				comment = false;
				ungetc(c, a->input);
				continue;
			}
			truelnscount++;
			if(!lnind)
				continue;

			tmpln[lnind] = '\0';

			pushsymbol(a->lns, mksymbol(tmpln, lnind+1, truelnscount));

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

		if(c == '(') {
			if(lnind) {
				fprintf(stderr, "Unexpected char '%c'; line %i:%i\n", c, truelnscount, lnind+1);
				exit(1);
			}

			SYMBOL* l = readlabel(a, truelnscount);
			if(getsymbol(a, l->name) != -1) {
				fprintf(stderr, "Already defined symbol '%s'; line %i\n", l->name, truelnscount);
				exit(1);
			}

			pushsymbol(a->labels, l);
			truelnscount++;
			continue;
		}
		
		if(c == '/') {
			char nc = fgetc(a->input);
			if(nc == '/') {
				comment = true;
				continue;
			}
			ungetc(nc, a->input);
		}

		if(spacedln) {
			fprintf(stderr, "Unexpected char '%c'; line %i:%i\n", c, lnscount+1, lnind+1);
			exit(1);
		}
		
		tmpln[lnind] = c;
		lnind++;
	}
	fclose(a->input);
}

void replacevar(SYMBOL* ln, int val) {
	free(ln->content);
	int size = sizeof(char)*(countplaces(val) + 2);
	char* newln = (char *)malloc(size);
	snprintf(newln, size, "@%i", val);
	ln->content = newln;
}

void preprocess(ASSEMBLER* a) {
	int varsramind = BOTTOM_VAR;
	for(int i = 0; i < a->lncount; i++) {
		if(a->lns->items[i]->content[0] == '@') {
			char* afterat = a->lns->items[i]->content+sizeof(char);
			if(isvar(afterat)) {
				int val = getsymbol(a, afterat);
				if(val == -1) {
					if(varsramind == RAM_LIMIT) {
						fprintf(stderr, "Variable amount reached RAM limit (%i); line %i\n", RAM_LIMIT, a->lns->items[i]->truen);
						exit(1);
					}
					SYMBOL* var = mksymbol(afterat, strlen(afterat)+1, varsramind);
					varsramind++;
					pushsymbol(a->vars, var);
					val = var->value;
				}
				replacevar(a->lns->items[i], val);
			}
		}
	}
}

void transa(SYMBOL* ln) {
	int add = atoi(ln->content+sizeof(char));

	if(add >= INST_LIMIT) {
		fprintf(stderr, "'A' instruction cannot reference addresses bigger than %i; line %i\n", INST_LIMIT-1, ln->truen);
		exit(1);
	}

	char* out = (char*)malloc(sizeof(char) * INST_SIZE);

	int lastbit = 1 << 15;
	for(int i = INST_SIZE-2; i > 0; i--) {
		if(add & (lastbit >> i))
			out[i] = '1';
		else
			out[i] = '0';
	}

	out[INST_SIZE-1] = '\0';
	out[0] = '0';

	free(ln->content);
	ln->content = out;
}

char* lookctable(TABLE* t, bool cond, char* token, const char* fieldname, int trueln) {
	char* out = (char*)malloc(t->instsize);

	if(!cond) {
		int targsize = t->instsize - 1;
		for(int i = 0; i < targsize; i++)
			out[i] = '0';
		out[t->instsize-1] = '\0';
		return out;
	}
	for(int i = 0; i < t->size; i++)
		if(strcmp(t->table[2*i], token) == 0) {
			strcpy(out, t->table[(2*i)+1]);
			return out;
		}

	fprintf(stderr, "Unexpected token '%s' for %s field; line %i\n", token, fieldname, trueln);
	exit(1);
}

void transb(SYMBOL* ln) {
	bool hasjmp = false;
	bool hasdest = false;
	bool hascmp = false;
	int i = 0;
	int tmpi = 0;
	char tmp[C_TOKEN_SIZE], dest[C_TOKEN_SIZE], cmp[C_TOKEN_SIZE], jmp[C_TOKEN_SIZE];

	while(1) {
		if(ln->content[i] == '\0') {
			tmp[tmpi] = '\0';
			if(hasjmp)
				strcpy(jmp, tmp);
			else
				strcpy(cmp, tmp);
			break;
		}
		
		if(tmpi == C_TOKEN_SIZE-1) {
			fprintf(stderr, "Unexpected char '%c'; line %i:%i;\n", ln->content[i], ln->truen, i+1);
			exit(1);
		}

		if(ln->content[i] == '=' && !hasdest && hascmp) {
			hascmp = false;
			hasdest = true;
			tmp[tmpi] = '\0';
			strcpy(dest, tmp);
			tmpi = 0;
			i++;
			continue;
		}
		if(ln->content[i] == ';' && !hasjmp && hascmp) {
			hascmp = false;
			hasjmp = true;
			tmp[tmpi] = '\0';
			strcpy(cmp, tmp);
			tmpi = 0;
			i++;
			continue;
		}
		
		hascmp = 1;
		tmp[tmpi] = ln->content[i];
		tmpi++;
		i++;
	}
	
	char* rawdest = lookctable(&desttable, hasdest, dest, "dest", ln->truen);
	char* rawjmp = lookctable(&jmptable, hasjmp, jmp, "jump", ln->truen);
	char* rawcmp = lookctable(&cmptable, 1, cmp, "comp", ln->truen);

	int sz = sizeof(char) * INST_SIZE;
	char* out = (char*)malloc(sz);
	snprintf(out, sz, "111%s%s%s", rawcmp, rawdest, rawjmp);

	free(ln->content);
	ln->content = out;
	free(rawdest);
	free(rawjmp);
	free(rawcmp);
}

void translate(ASSEMBLER* a) {
	for(int i = 0; i < a->lns->count; i++)
		if(a->lns->items[i]->content[0] == '@')
			transa(a->lns->items[i]);
		else
			transb(a->lns->items[i]);
}

void gatherinfo(ASSEMBLER* a) {
	char c;
	bool readsmt = false;
	bool comment = false;
	int lnwidth = 1;

	a->truelnscount = 1;
	a->maxwidth = 0;
	a->labels->count = 0;
	a->lns->count = 0;

	while(c = fgetc(a->input), c != -1) {
		if(c == '\n') {
			a->truelnscount++;
			comment = false;
			if(lnwidth > a->maxwidth)
				a->maxwidth = lnwidth;
			if(readsmt) {
				if(a->lns->count == INST_LIMIT) {
					fprintf(stderr, "Reached instruction limit (%i); line %i\n", INST_LIMIT, a->truelnscount);
					exit(1);
				}
				a->lns->count++;
			}
			readsmt = false;
			lnwidth = 1;
			continue;
		}
		if(comment)
			continue;
		if(c == '(') {
			a->labels->count++;
			comment = true;
			continue;
		}
		if(c == '/') {
			char nc = fgetc(a->input);
			if(nc == '/') {
				comment = true;
				continue;
			}
			ungetc(nc, a->input);
		}
		if(isspace(c)) {
			continue;
		}
		readsmt = true;
		lnwidth++;
	}
	rewind(a->input);
	a->lncount = a->lns->count;
}

void freeassembler(ASSEMBLER* a) {
	freesymbols(a->lns);
	freesymbols(a->vars);
	freesymbols(a->labels);
	free(a);
}
