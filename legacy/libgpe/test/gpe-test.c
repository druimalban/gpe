#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <libgpe.h>

int main(int argc, char **argv)
{
char testcmd[256];
int i;

	strcpy(testcmd,"./testcmd -a -b -c arg -d");

	i=gpe_execute_async_cmd(testcmd);

	fprintf(stderr,"gpe_execute_async()=%d\n",i);

return 0;
}
