#ifndef UTIL_H
#define UTIL_H

#include <stdio.h>

char* heapstr(const char* str, int len);
int countplaces(int n);

typedef struct lnls {
	char* content;
	int truen;
	struct lnls* next;
} LINELIST;

void printlns(LINELIST* lns, FILE* stream);
#endif
