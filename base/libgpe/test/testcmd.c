#include <stdio.h>

int main(int argc, char **argv)
{
int i;

	fprintf(stderr,"'%s' called with args:\n",argv[0]);
	for (i=1; i<argc; i++)
		fprintf(stderr,"\t%d: '%s'\n",i,argv[i]);

return 0;
}