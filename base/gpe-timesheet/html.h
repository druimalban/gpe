#ifndef _GPE_TIMESHEET_HTML_H
#define _GPE_TIMESHEET_HTML_H

#include "sql.h"

int journal_add_header(char* title, char ***buffer);
int journal_add_line(struct task atask, char ***buffer, int length);
int journal_add_footer(char ***buffer, int length);
int journal_to_file(char **buffer, char *filename);
int journal_show(char *filename);

#endif
