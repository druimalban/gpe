#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "nsqlc.h"

int
callback (void *arg, int argc, char *argv[])
{
  int i;

  fprintf (stderr, "argc=%d\n", argc);

  for (i = 0; i < argc; i++)
    fprintf (stderr, "argv[%d] = '%s'\n", i, argv[i]);

  return 0;
}

main()
{
  nsqlc *ctx;
  char *err;
  time_t t = 0;

  ctx = nsqlc_open_ssh ("localhost:test", 0, &err);
  if (ctx == NULL)
    {
      fprintf (stderr, "Couldn't open: %s\n", err);
      exit (1);
    }

  nsqlc_get_time (ctx, &t, &err);
  printf ("%d %d\n", t, time (NULL));

  if (nsqlc_exec (ctx, "select * from test", callback, NULL, &err))
    {
      fprintf (stderr, "Couldn't run query: %s\n", err);
      exit (1);
    }

  if (nsqlc_exec_printf (ctx, "insert into test values('%s')", callback, NULL, &err, "hello"))
    {
      fprintf (stderr, "Couldn't run query: %s\n", err);
      exit (1);
    }

  nsqlc_close (ctx);

  exit (0);
}
