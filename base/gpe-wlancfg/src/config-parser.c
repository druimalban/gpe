#include <stdio.h>
#include <stdlib.h>

#include "config-parser.h"

int parse_input(char* cfgname)
{
	FILE 	*inputfile;

	memset(schemelist, 0, sizeof(schemelist));

	inputfile=fopen(cfgname, "r");
		if (!inputfile)
		{
			fprintf(stderr, "Could not open input file %s\n", cfgname);
			return(0);
		}
		wl_set_inputfile(inputfile);
		yyparse();

	printf("Found %i schemes...\n", schemecount);
	return(schemecount);
}

