/* usefull_functions.h, by Michael Berg
 * A collection of functions I wrote or was shown that I find usefull for
 * writing most programs
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "usefull_funcs.h"


/* Function implementations */

int rand_from_to (int min, int max)
{
  int result;

  /* Note: This line doesn't work on many Microsoft platforms, but works
     very well on most variants of Unix (especially Linux) */ 
  result = ((rand() % time(0)) % (max - min + 1)) + min;
  return (result);
}
