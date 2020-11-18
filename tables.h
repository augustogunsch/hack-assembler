#ifndef TABLES
#define TABLES

typedef struct {
	const char** table;
	const int size;
	const int instsize;
} TABLE;

const char* cmptablestrs[] = {
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
	"1+D", "0011111",
	"A+1", "0110111",
	"1+A", "0110111",
	"D-1", "0001110",
	"A-1", "0110010",
	"D+A", "0000010",
	"A+D", "0000010",
	"D-A", "0010011",
	"A-D", "0000111",
	"D&A", "0000000",
	"A&D", "0000000",
	"D|A", "0010101",
	"A|D", "0010101",
	"M", "1110000",
	"!M", "1110001",
	"-M", "1110011",
	"M+1", "1110111",
	"1+M", "1110111",
	"M-1", "1110010",
	"D+M", "1000010",
	"M+D", "1000010",
	"D-M", "1010011",
	"M-D", "1000111",
	"D&M", "1000000",
	"M&D", "1000000",
	"D|M", "1010101",
	"M|D", "1010101"
};
TABLE cmptable =
{
	.table = cmptablestrs,
	.size = sizeof(cmptablestrs) / sizeof(char*) / 2,
	.instsize = 8 * sizeof(char)
};

const char* desttablestrs[] = {
	"M", "001",
	"D", "010",
	"MD", "011",
	"A", "100",
	"AM", "101",
	"AD", "110",
	"AMD", "111"
};
TABLE desttable = 
{
	.table = desttablestrs,
	.size = sizeof(desttablestrs) / sizeof(char*) / 2,
	.instsize = 4 * sizeof(char)
};

const char* jmptablestrs[] = {
	"JGT", "001",
	"JEQ", "010",
	"JGE", "011",
	"JLT", "100",
	"JNE", "101",
	"JLE", "110",
	"JMP", "111"
};
TABLE jmptable =
{
	.table = jmptablestrs,
	.size = sizeof(jmptablestrs) / sizeof(char*) / 2,
	.instsize = 4 * sizeof(char)
};

#endif
