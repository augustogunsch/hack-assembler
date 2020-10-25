#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#define RAM_LIMIT 24577
#define TOP_VAR 16383
#define BOT_VAR 16
#define ADD_STR_SIZE 7
#define INST_SIZE 17
#define C_TOKEN_SIZE 4
#define INST_LIMIT 32768

#define CMP_SIZE 8
#define CMP_TABLE_SIZE 27
const char* cmptable[] = 
{
	"0", "0101010",
	"1", "0111111",
	"-1", "0111010",
	"D", "0001100",
	"A", "0110000",
	"!D", "0001101",
	"!A", "0110001",
	"-D", "0001111",
	"-A", "0110011",
	"D+1", "0011111",
	"A+1", "0110111",
	"D-1", "0001110",
	"A-1", "0110010",
	"D+A", "0000010",
	"D-A", "0010011",
	"A-D", "0000111",
	"D&A", "0000000",
	"D|A", "0010101",
	"M", "1110000",
	"!M", "1110001",
	"M+1", "1110111",
	"M-1", "1110010",
	"D+M", "1000010",
	"D-M", "1010011",
	"M-D", "1000111",
	"D&M", "1000000",
	"D|M", "1010101"
};

#define DEST_SIZE 4
#define DEST_TABLE_SIZE 7
const char* desttable[] =
{
	"M", "001",
	"D", "010",
	"MD", "011",
	"A", "100",
	"AM", "101",
	"AD", "110",
	"AMD", "111"
};

#define JMP_SIZE 4
#define JMP_TABLE_SIZE 7
const char* jmptable[] =
{
	"JGT", "001",
	"JEQ", "010",
	"JGE", "011",
	"JLT", "100",
	"JNE", "101",
	"JLE", "110",
	"JMP", "111"
};

struct symbol {
	char* name;
	int value;
};

struct line {
	char* ln;
	int truen;
};

void freesymbols(struct symbol** symbols, int symbolsind) {
	for(int i = 0; i < symbolsind; i++) {
		free(symbols[i]->name);
		free(symbols[i]);
	}
}

struct symbol* mksymb(char* name, int namesize, int val) {
	struct symbol* s = (struct symbol*)malloc(sizeof(struct symbol));
	char* heapname = (char*)malloc(namesize);
	strcpy(heapname, name);
	s->name = heapname;
	s->value = val;
	return s;
}

int getsymb(char* symb, struct symbol** vars, int varscount, struct symbol** labels, int labelscount) {
	for(int i = 0; i < varscount; i++)
		if(strcmp(vars[i]->name, symb)  == 0)
			return vars[i]->value;

	for(int i = 0; i < labelscount; i++)
		if(strcmp(labels[i]->name, symb)  == 0)
			return labels[i]->value;
	
	return -1;
}

void skipln(FILE* input) {
	char c;
	while(c = fgetc(input), c != -1)
		if(c == '\n')
			break;
}

void readrest(FILE* input, int trueln) {
	char c;
	while(c = fgetc(input), c != -1) {
		if(c == '\n')
			break;
		if(isspace(c))
			continue;
		if(c == '/') {
			char nc = fgetc(input);
			if(nc == '/') {
				skipln(input);
				break;
			}
			ungetc(nc, input);
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

void populatevars(struct symbol** vars, int* varscount) {
	// First five
	const int firstamnt = 5;
	char* labels[] = { "SP", "LCL", "ARG", "THIS", "THAT" };
	for(int i = 0; i < firstamnt; i++) {
		vars[i] = mksymb(labels[i], strlen(labels[i])+1, i);
	}

	// RAM variables (R0-R15)
	const int ramvamnt = 16; //max: 19
	const int asciioff = 48;
	char ramvname[4];
	ramvname[0] = 'R';
	ramvname[2] = '\0';
	for(int i = 0; i < (ramvamnt/10)*10; i++) {
		ramvname[1] = (char)(i+asciioff);
		vars[firstamnt+i] = mksymb(ramvname, 3, i);
	}
	ramvname[1] = '1';
	ramvname[3] = '\0';
	for(int i = 10; i < ramvamnt; i++) {
		ramvname[2] = (char)((i%10)+asciioff);
		vars[firstamnt+i] = mksymb(ramvname, 4, i);
	}

	// SCREEN var
	vars[firstamnt+ramvamnt] = mksymb("SCREEN", 7, 16384);
	// KBD var
	vars[firstamnt+ramvamnt+1] = mksymb("KBD", 4, 24576);
	// update varscount to new index
	*varscount = firstamnt+ramvamnt+2;
}

void gatherinfo(FILE* input, int* lnscount, int* labelscount, int* maxwidth) {
	char c;
	unsigned char readsmt = 0;
	unsigned char comment = 0;
	int truelnscount = 1;
	int lnwidth = 1;
	while(c = fgetc(input), c != -1) {
		if(c == '\n') {
			truelnscount++;
			comment = 0;
			if(lnwidth > *maxwidth)
				*maxwidth = lnwidth;
			if(readsmt) {
				if(*lnscount == INST_LIMIT) {
					fprintf(stderr, "Reached instruction limit (%i); line %i\n", INST_LIMIT, truelnscount);
					exit(1);
				}
				(*lnscount)++;
			}
			readsmt = 0;
			lnwidth = 1;
			continue;
		}
		if(comment)
			continue;
		if(c == '(') {
			(*labelscount)++;
			comment = 1;
			continue;
		}
		if(c == '/') {
			char nc = fgetc(input);
			if(nc == '/') {
				comment = 1;
				continue;
			}
			ungetc(nc, input);
		}
		if(isspace(c)) {
			continue;
		}
		readsmt = 1;
		lnwidth++;
	}
	rewind(input);
}

struct symbol* readlabel(FILE* input, int ln, int trueln, int lnwidth) {
	char* name = (char*)malloc(sizeof(char)*(lnwidth-1));
	int i = 0;
	char c;
	while(c = fgetc(input), c != -1) {
		if(c == ')')
			break;
		if(i == lnwidth-2) {
			fprintf(stderr, "Label width bigger than the maximum (%i characters); line %i\n", 
				lnwidth-2, trueln+1);
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
	readrest(input, trueln);
	struct symbol* l = (struct symbol*)malloc(sizeof(struct symbol));
	l->name = name;
	l->value = ln;
	return l;
}

// Splits the stream into an array of strings, stripping comments, white spaces and labels
// Requires vars array to check for duplicate symbols, but doesn't modify it
int chop(FILE* input, struct symbol** vars, int varscount, struct symbol** labels, int* labelscount, struct line** lns, int lnwidth) {
	char c;
	char tmpln[lnwidth];
	int lnscount = 0;
	int truelnscount = 1;
	int lnind = 0;
	int comment = 0;
	int spacedln = 0;
	while(c = fgetc(input), c != -1) {
		if(c == '\n') {
			if(comment) {
				comment = 0;
				ungetc(c, input);
				continue;
			}
			truelnscount++;
			if(!lnind)
				continue;

			tmpln[lnind] = '\0';

			char* newln = (char*)malloc(sizeof(char)*(lnind+1));
			strcpy(newln, tmpln);
			struct line* s = (struct line*)malloc(sizeof(struct line));
			s->ln = newln;
			s->truen = truelnscount;
			lns[lnscount] = s;

			lnscount++;
			lnind = 0;
			spacedln = 0;
			continue;
		}

		if(comment)
			continue;

		if(isspace(c)) {
			if(lnind)	
				spacedln = 1;
			continue;
		}

		if(c == '(') {
			if(lnind != 0) {
				fprintf(stderr, "Unexpected char '%c'; line %i:%i\n", c, truelnscount, lnind+1);
				exit(1);
			}

			struct symbol* l = readlabel(input, lnscount, truelnscount, lnwidth);
			if(getsymb(l->name, vars, varscount, labels, *labelscount) != -1) {
				fprintf(stderr, "Already defined symbol '%s'; line %i\n", l->name, truelnscount);
				exit(1);
			}

			labels[*labelscount] = l;
			(*labelscount)++;
			truelnscount++;
			continue;
		}
		
		if(c == '/') {
			char nc = fgetc(input);
			if(nc == '/') {
				comment = 1;
				continue;
			}
			ungetc(nc, input);
		}

		if(spacedln) {
			fprintf(stderr, "Unexpected char '%c'; line %i:%i\n", c, lnscount+1, lnind+1);
			exit(1);
		}
		
		if(lnwidth-1 == lnind) {
			fprintf(stderr, "Reached line width limit (%i); line %i\n", lnwidth, lnscount+1);
			exit(1);
		}
		
		tmpln[lnind] = c;
		lnind++;
	}
	return lnscount;
}

void replacevar(struct line* ln, int val) {
	free(ln->ln);
	char* newln = (char *)malloc(sizeof(char)*ADD_STR_SIZE);
	snprintf(newln, ADD_STR_SIZE, "@%i", val);
	ln->ln = newln;
}

void stripvars(struct symbol** vars, int* varscount, struct symbol** labels, int* labelscount, struct line** lns, int lnscount) {
	int varsramind = BOT_VAR;
	for(int i = 0; i < lnscount; i++) {
		if(lns[i]->ln[0] == '@') {
			char* afterat = lns[i]->ln+sizeof(char);
			if(isvar(afterat)) {
				int val = getsymb(afterat, vars, *varscount, labels, *labelscount);
				if(val == -1) {
					if(varsramind == RAM_LIMIT) {
						fprintf(stderr, "Variable amount reached RAM limit (%i); line %i\n", RAM_LIMIT, lns[i]->truen);
						exit(1);
					}
					struct symbol* var = mksymb(afterat, strlen(afterat)+1, varsramind);
					vars[*varscount] = var;
					(*varscount)++;
					varsramind++;
					val = var->value;
				}
				replacevar(lns[i], val);
			}
		}
	}
}

void transa(char* in, char* out, int trueln) {
	int add = atoi(in+sizeof(char));
	if(add >= INST_LIMIT) {
		fprintf(stderr, "'A' instruction cannot reference addresses bigger than %i; line %i\n", INST_LIMIT-1, trueln);
		exit(1);
	}
	int lastbit = 1 << 15;
	for(int i = INST_SIZE-2;i > 0; i--) {
		if(add & (lastbit >> i))
			out[i] = '1';
		else
			out[i] = '0';
	}
	out[INST_SIZE-1] = '\0';
	out[0] = '0';
}

void lookctable(char* out, int outsize, const char** table, int tablesize, int cond, char* token, const char* fieldname, int trueln) {
	if(!cond) {
		for(int i = 0; i < outsize-1; i++)
			out[i] = '0';
		out[outsize-1] = '\0';
		return;
	}
	for(int i = 0; i < tablesize; i++)
		if(strcmp(table[2*i], token) == 0) {
			strcpy(out, table[(2*i)+1]);
			return;
		}
	fprintf(stderr, "Unexpected token '%s' for %s field; line %i\n", token, fieldname, trueln);
	exit(1);
}

void transb(char* in, char* out, int trueln) {
	int hasjmp = 0;
	int hasdest = 0;
	int hascmp = 0;
	int i = 0;
	int tmpi = 0;
	char tmp[C_TOKEN_SIZE], dest[C_TOKEN_SIZE], cmp[C_TOKEN_SIZE], jmp[C_TOKEN_SIZE];

	while(1) {
		if(in[i] == '\0') {
			tmp[tmpi] = '\0';
			if(hasjmp)
				strcpy(jmp, tmp);
			else
				strcpy(cmp, tmp);
			break;
		}
		
		if(tmpi == C_TOKEN_SIZE-1) {
			fprintf(stderr, "Unexpected char '%c'; line %i:%i;\n", in[i], trueln, i+1);
			exit(1);
		}

		if(in[i] == '=' && !hasdest && hascmp) {
			hascmp = 0;
			hasdest = 1;
			tmp[tmpi] = '\0';
			strcpy(dest, tmp);
			tmpi = 0;
			i++;
			continue;
		}
		if(in[i] == ';' && !hasjmp && hascmp) {
			hascmp = 0;
			hasjmp = 1;
			tmp[tmpi] = '\0';
			strcpy(cmp, tmp);
			tmpi = 0;
			i++;
			continue;
		}
		
		hascmp = 1;
		tmp[tmpi] = in[i];
		tmpi++;
		i++;
	}
	
	char rawdest[DEST_SIZE];
	lookctable(rawdest, DEST_SIZE, desttable, DEST_TABLE_SIZE, hasdest, dest, "dest", trueln);
	char rawjmp[JMP_SIZE];
	lookctable(rawjmp, JMP_SIZE, jmptable, JMP_TABLE_SIZE, hasjmp, jmp, "jump", trueln);
	char rawcmp[CMP_SIZE];
	lookctable(rawcmp, CMP_SIZE, cmptable, CMP_TABLE_SIZE, 1, cmp, "comp", trueln);
	sprintf(out, "111%s%s%s", rawcmp, rawdest, rawjmp);
}

char** translate(struct line** lns, int lnscount) {
	char** assembled = (char**)malloc(sizeof(char*)*lnscount);
	for(int i = 0; i < lnscount; i++)
		assembled[i] = (char*)malloc(sizeof(char)*INST_SIZE);

	for(int i = 0; i < lnscount; i++)
		if(lns[i]->ln[0] == '@')
			transa(lns[i]->ln, assembled[i], lns[i]->truen);
		else
			transb(lns[i]->ln, assembled[i], lns[i]->truen);
	return assembled;
}

void getoutname(char* fname, int fnamelen, char* outf) {
	strcpy(outf, fname);
	strcpy(outf+(fnamelen*sizeof(char))-(3*sizeof(char)), "hack");
}

int main(int argc, char* argv[]) {
	if(argc < 2) {
		printf("Usage: %s {input}\n", argv[0]);
		return 1;
	}

	int fnamelen = strlen(argv[1]);
	int invalidext = strcmp(argv[1]+(fnamelen*sizeof(char)-(4*sizeof(char))), ".asm");
	if(invalidext) {
		fprintf(stderr, "Invalid extension (must be *.asm)\n");
		return 1;
	}
	FILE* input = fopen(argv[1], "r");

	if(input == NULL) {
		fprintf(stderr, "%s\n", strerror(errno));
		return errno;
	}
	
	// info gathering
	int lnscount = 0;
	int labelscount = 0;
	int lnwidth = 0;
	gatherinfo(input, &lnscount, &labelscount, &lnwidth);
	struct line** lns = (struct line**)malloc(sizeof(struct line*)*lnscount); // has to be on the heap; can be huge and cause a stack overflow

	// line chopping
	struct symbol** labels = (struct symbol**)malloc(sizeof(struct symbol*)*labelscount); // same for this one
	labelscount = 0;

	struct symbol* vars[TOP_VAR - BOT_VAR];
	int varscount = 0;
	populatevars(vars, &varscount);

	lnscount = chop(input, vars, varscount, labels, &labelscount, lns, lnwidth);
	fclose(input);

	// variable substitution
	stripvars(vars, &varscount, labels, &labelscount, lns, lnscount);
	freesymbols(vars, varscount);
	freesymbols(labels, labelscount);
	free(labels);
	
	// actual translation
	char** bin = translate(lns, lnscount);
	for(int i = 0; i < lnscount; i++) {
		free(lns[i]->ln);
		free(lns[i]);
	}
	free(lns);

	// file output
	char outf[fnamelen+2];
	getoutname(argv[1], fnamelen, outf);

	FILE* output = fopen(outf, "w");

	if(output == NULL) {
		fprintf(stderr, "%s\n", strerror(errno));
		return errno;
	}

	for(int i = 0; i < lnscount; i++) {
		fprintf(output, "%s\n", bin[i]);
		free(bin[i]);
	}
	free(bin);
	fclose(output);
	return 0;
}
