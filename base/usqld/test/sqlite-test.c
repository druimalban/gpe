#include <stdio.h>
#include <sqlite.h> 
#include <unistd.h>
#include <stdlib.h>

int cb(void * c,int nf,char ** row,char ** head)
{
   int i;
   
   for(i = 0;i<nf;i++)
     {
	printf("%s\t|",head[i]);
     }
   printf("\n");
   for(i = 0;i<nf;i++)
     {
	printf("%s\t|",row[i]);
     }
   printf("\n");
   return 0;
}


int main(int argc, char ** argv)
{
   sqlite* s;
   char * errmsg;
   s = sqlite_open("/tmp/test",0644, &errmsg);
   if(s==NULL)
     {
	fprintf(stderr,"cannot open database: %s",errmsg);
	exit(1);
     }
   
   if(SQLITE_OK!=sqlite_exec(s,"CREATE TABLE foo(x int,y int);",
			     NULL,
			     NULL,
			     &errmsg))
     {
	fprintf(stderr,"cannot create table:%s",errmsg);
	
     }
   
   if(SQLITE_OK!=sqlite_exec(s,"INSERT into foo VALUES(1,2);",
			     NULL,
			     NULL,
			     &errmsg))
     {
	fprintf(stderr,"cannot insert into table:%s",errmsg);
	
     }

   if(SQLITE_OK!=sqlite_exec(s,"INSERT into foo VALUES(3,4);",
			     NULL,
			     NULL,
			     &errmsg))
     {
	fprintf(stderr,"cannot insert into table:%s",errmsg);
	
     }
   
   if(SQLITE_OK!=sqlite_exec(s,"SELECT * from foo;",
			     cb,
			     NULL,
			     &errmsg))
     {
	fprintf(stderr,"cannot select from table:%s",errmsg);
	
     }

   return 0;
}
