#ifndef _CONF_PARSER_H
#define _CONF_PARSER_H
/* some very modest parser functions.. */

int parse_file(char *file,char *format,...);
int parse_pipe(char *cmd,char *format,...);
int parse_file_and_gfree(char *file,char *format,...);
int parse_pipe_and_gfree(char *cmd,char *format,...);

#endif
