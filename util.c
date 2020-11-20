#include <string.h>
#include <stdlib.h>
#include "util.h"

char* heapstr(const char* str, int len) {
	int sz = sizeof(char) * (len + 1);
	char* outstr = (char*)malloc(sz);
	strcpy(outstr, str);
	return outstr;
}

int countplaces(int n) {
	int places = 1;
	int divisor = 1;
	if(n < 0) {
		n = -n;
		places++;
	}
	while(n / divisor >= 10) {
		places++;
		divisor *= 10;
	}
	return places;
}

void printlns(LINELIST* lns, FILE* stream) {
	LINELIST* curln = lns;
	while(curln != NULL) {
		fprintf(stream, "%s\n", curln->content);
		curln = curln->next;
	}
}
