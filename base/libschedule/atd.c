#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include <stdio.h>

#include <glib.h>

#define ATD_BASE "/var/spool/at"

#define TRIGGER ATD_BASE "/trigger"

static gboolean
trigger_atd (void)
{
  int fd = open (TRIGGER, O_WRONLY);
  gboolean rc = TRUE;
  if (fd < 0)
    {
      perror (TRIGGER);
      return FALSE;
    }
  
  if (write (fd, "\n", 1) < 0)
    rc = FALSE;
  if (close (fd) < 0)
    rc = FALSE;

  return rc;
}

static gchar *
alarm_filename (guint id, time_t start)
{
  uid_t uid = getuid ();
  return g_strdup_printf ("%s/%ld.%ld-%ld", ATD_BASE, start, id, uid);
}

gboolean
schedule_set_alarm (guint id, time_t start, const gchar *action)
{
  int fd;
  gchar *filename = alarm_filename (id, start);

  fd = open (filename, O_WRONLY | O_CREAT, 0700);
  if (fd < 0)
    {
      perror (filename);
      g_free (filename);
      return FALSE;
    }

  g_free (filename);

  return trigger_atd ();
}

gboolean
unschedule_alarm (guint id, time_t start)
{
  gchar *filename = alarm_filename (id, start);

  if (unlink (filename))
    {
      perror (filename);
      g_free (filename);
      return FALSE;
    }

  g_free (filename);

  return trigger_atd ();
}

