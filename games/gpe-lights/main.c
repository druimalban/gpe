/*
 * Switch the light off !
 * A little brain-oriented game.
 *-----
 * To My angel.
 * December 2001.
 * Sed
 *-----
 * This file is in the public domain.
 *-----
 * the main file
 */

#include "interface.h"
#include "event.h"

int main(void)
{
  interface it;
  event ev;

  if (init_interface(&it) == -1)
    return -1;

  while (1) {
    if (wait_event(&ev, &it) == -1)
      goto end;
    if (handle_event(&ev, &it) != 0)
      goto end;
  }

end:
  close_interface(&it);
  return 0;
}
