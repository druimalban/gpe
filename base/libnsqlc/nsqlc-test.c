#include <stdio.h>
#include <stdlib.h>

#include "nsqlc.h"

void
callback (int argc, char *argv[], void *arg)
{
}

main()
{
  nsqlc *ctx;
  char *err;

  ctx = nsqlc_open ("localhost:test", 0, &err);
  if (ctx == NULL)
    {
      fprintf (stderr, "Couldn't open: %s\n", err);
      exit (1);
    }

  if (nsqlc_exec (ctx, "select * from test", callback, NULL, &err))
    {
      fprintf (stderr, "Couldn't run query: %s\n", err);
      exit (1);
    }

  nsqlc_close (ctx);

  exit (0);
}
