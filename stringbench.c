#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/time.h>

#define START gettimeofday(&tv1, 0);
#define END(x) gettimeofday(&tv2, 0); t1 = ((double)tv1.tv_sec)+((double)tv1.tv_usec)/1000000.0; t2 = ((double)tv2.tv_sec)+((double)tv2.tv_usec)/1000000.0; printf("%s: %f seconds\n", x, t2-t1)

int main(int argc __unused, char **argv __unused) {
	struct timeval tv1, tv2;
	double t1, t2;
	char * const s = (char * const) malloc(16);
	char * const l = (char * const) malloc(15001);
	char * const tmp = (char * const) malloc(15001);

	for(int i=0; i<15; i++)
		s[i]='a';
	s[15]=0;

	START;
	for(int i=0; i<10000000; i++) {
		memset(l, 'a', 15000);
	}
	END("10000000 * memset 15000*'a'");

	l[15000]=0;

	START;
	for(int i=0; i<500000000; i++)
		if(strlen(s) != 15)
			fprintf(stderr, "strlen broken\n");
	END("500000000 * strlen(15*a)");

	START;
	for(int i=0; i<1000000; i++)
		if(strlen(l) != 15000)
			fprintf(stderr, "strlen broken\n");
	END("1000000 * strlen(15000*a)");

	START;
	for(int i=0; i<10000000; i++)
		if(memchr(l, 'b', 15000))
			fprintf(stderr, "bogus memchr\n");
	END("10000000 * memchr not finding anything in 15000 chars");

	START;
	for(int i=0; i<10000000; i++)
		if(strchr(l, 'b'))
			fprintf(stderr, "bogus strchr\n");
	END("10000000 * strchr not finding anything in 15000 chars");

	l[5000]='b';
	START;
	for(int i=0; i<10000000; i++) {
		char *b=memchr(l, 'b', 15000);
		if(!b || *b != 'b')
			fprintf(stderr, "bogus memchr II\n");
	}
	END("10000000 * memchr finding match at 5000");

	START;
	for(int i=0; i<10000000; i++) {
		char *b=strchr(l, 'b');
		if(!b || *b != 'b')
			fprintf(stderr, "bogus strchr II\n");
	}
	END("10000000 * strchr finding match at 5000");

	START;
	for(int i=0; i<10000000; i++) {
		memcpy(tmp, l, 15000);
	}
	END("10000000 * memcpy of 15000 bytes");

	START;
	for(int i=0; i<900000000; i++) {
		memcpy(tmp, s, 15);
	}
	END("900000000 * memcpy of 15 bytes");

	START;
	for(int i=0; i<1000000; i++) {
		strcpy(tmp, l);
	}
	END("1000000 * strcpy of 15000 bytes");

	START;
	for(int i=0; i<100000000; i++) {
		strcpy(tmp, s);
	}
	END("100000000 * strcpy of 15 bytes");

	START;
	for(int i=0; i<15000; i++) {
		memset(tmp, 0, 15000);
		for(int j=0; j<15000; j++) {
			strcat(tmp, "a");
		}
	}
	END("15000 * 15000 * strcat + 1 byte");

	memset(l, 0, 15000);
	memset(l, 'a', 1000);
	START;
	for(int i=0; i<15000; i++) {
		memset(tmp, 0, 15000);
		for(int j=0; j<15; j++) {
			strcat(tmp, l);
		}
	}
	END("15000 * 15 * strcat + 1000 bytes");

	return 0;
}
