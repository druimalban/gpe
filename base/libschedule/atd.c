/*
 * Copyright (C) 2003 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <glib.h>

#include <sqlite.h>
#include <gpe/errorbox.h>

#include "gpe/schedule.h"

#define ATD_BASE "/var/spool/at"

#define TRIGGER ATD_BASE "/trigger"

static sqlite *sqliteh;
static const char *fname = "/.gpe/next_alarm_id";

char *no_export[] =
{
    "TERM", "_", "SHELLOPTS", "BASH_VERSINFO", "EUID", "GROUPS", "PPID", "UID"
};

extern char **environ;

int alarm_uid;

static const char *schema_str = 
"create table alarms (alarm_uid integer NOT NULL)";

static int
dbinfo_callback (void *arg, int argc, char **argv, char **names)
{
  if (argc == 1)
    {
      alarm_uid = atoi (argv[0]);
    }

  return 0;
}

gboolean
alarm_db_start (void)
{
  const char *home = g_get_home_dir ();
  char *buf;
  char *err;
  size_t len;

  len = strlen (home) + strlen (fname) + 1;
  buf = g_malloc (len);
  strcpy (buf, home);
  strcat (buf, fname);
  sqliteh = sqlite_open (buf, 0, &err);
  g_free (buf);
  if (sqliteh == NULL)
    {
      gpe_error_box (err);
      free (err);
      return FALSE;
    }

  sqlite_exec (sqliteh, schema_str, NULL, NULL, &err);
  
  if (sqlite_exec (sqliteh, "select alarm_uid from alarms", dbinfo_callback, NULL, &err))
    {
      alarm_uid=-1;
      gpe_error_box (err);
      free (err);
      return FALSE;
    }
    
  return TRUE;
}

static gboolean
trigger_atd (void)
{
  char pid_file[25];
  int fd;
  gboolean rc = TRUE;
   
  sprintf(pid_file, "/var/run/atd.pid");
  if (access (pid_file, F_OK) != 0)
    rc=FALSE;
  		
  if (rc)
  {
    fd = open (TRIGGER, O_WRONLY);
    if (fd < 0)
    {
      perror (TRIGGER);
      return FALSE;
    }
  
    if (write (fd, "\n", 1) < 0)
      rc = FALSE;
    if (close (fd) < 0)
      rc = FALSE;
  }
  return rc;
}

static gboolean
same_alarm (guint alarm_uid, guint id, guint start_time)
{
  char dline[256], call_at[256];
  int id_check, start_time_check;
  FILE *at_return;
  
  sprintf(call_at, "/usr/bin/at -c %d 2>&1", alarm_uid);
  if ((at_return=popen(call_at, "r")) != NULL)
    {
      while(fgets(dline,sizeof(dline),at_return)) {
        if (sscanf(dline,"#!# %i %i", &id_check, &start_time_check) == 2) {
	  if (id==id_check && start_time==start_time_check) {
	    pclose(at_return);
	    return TRUE;
	  }
	}
      }
    }
  pclose (at_return);
  return FALSE;
}

static gchar *
alarm_filename (guint id, time_t start)
{
  uid_t uid = getuid ();
  return g_strdup_printf ("%s/%ld.%d-%d", ATD_BASE, start, id, uid);
}

static gchar *
write_tail (guint id, time_t start)
{
  return g_strdup_printf ("#!# %d %ld", id, start);
}

static gchar *
tmp_alarm_filename (guint id, time_t start)
{
  return g_strdup_printf ("/tmp/atjob.txt");
}

static gboolean
cancel_alarm_uid(guint alarm_uid)
{
  int ret_val;
  char erase_at[256];
  
  sqlite_exec_printf (sqliteh, "delete from alarms where alarm_uid=%d", 
		      NULL, NULL, NULL, alarm_uid);
  sprintf(erase_at, "/usr/bin/atrm %d", alarm_uid);
  ret_val = system(erase_at);
  if (ret_val == 0) return TRUE;
  else return FALSE;
}  
  
static const char *boilerplate_1 = "#!/bin/sh\nexport DISPLAY=:0\n";
static const char *condensorplate_1 = "\nrm -f $0\n";

/**
 * schedule_set_alarm:
 * @id: Unique alarm identifier. 
 * @start: Time to start the alarm action.
 * @action: Command to become executed if alarm time is reached.
 * @calendar_alarm: A flag to assure that the gpe-calendar program does
 *     not erase someone else's at (alarm) command
 * @HAVE_AT: A compile-time flag to determine whether the scheduler should
 *    use a "traditional" at (TRUE) or the ipaq's atd (FALSE) 
 *
 * Set a new alert including an action which should become executed
 * at the alarm time. 
 *
 * Returns: TRUE on success, FALSE otherwise.
 */
gboolean
schedule_set_alarm (guint id, time_t start, const gchar *action, gboolean calendar_alarm)
{
  int fd;
#ifdef HAVE_AT
  gchar *filename = tmp_alarm_filename (id, start);
  struct tm tm;
  char call_at[256];
  gchar *atjob_tail = write_tail (id, start);
  FILE *at_return;
#else
  char *ap;
  char **atenv;
  gchar *filename = alarm_filename (id, start);
#endif
  FILE *fp;

  fd = open (filename, O_WRONLY | O_CREAT, 0700);
  if (fd < 0)
    {
      perror (filename);
      g_free (filename);
      return FALSE;
    }
  if ((fp = fdopen(fd, "w")) == NULL)
      {
	printf("Cannot reopen atjob file\n");
	return FALSE;
      }
	
  fwrite (boilerplate_1, sizeof(char), strlen (boilerplate_1), fp);

#ifndef HAVE_AT
    for (atenv = environ; *atenv != NULL; atenv++) {
    int export = 1;
    char *eqp;

    eqp = strchr(*atenv, '=');
    if (ap == NULL)
  	eqp = *atenv;
    else {
  	unsigned int i;
  	for (i = 0; i < sizeof(no_export) / sizeof(no_export[0]); i++) {
  	    export = export
  		&& (strncmp(*atenv, no_export[i],
  			    (size_t) (eqp - *atenv)) != 0);
  	}
  	eqp++;
    }
	
    if (export) {
  	fwrite(*atenv, sizeof(char), eqp - *atenv, fp);
  	for (ap = eqp; *ap != '\0'; ap++) {
  	    if (*ap == '\n')
  		fprintf(fp, "\"\n\"");
  	    else {
  		if (!isalnum(*ap)) {
  		    switch (*ap) {
  		    case '%':
  		    case '/':
  		    case '{':
  		    case '[':
  		    case ']':
  		    case '=':
  		    case '}':
  		    case '@':
  		    case '+':
  		    case '#':
  		    case ',':
  		    case '.':
  		    case ':':
  		    case '-':
  		    case '_':
  			break;
  		    default:
  			fputc('\\', fp);
  			break;
  		    }
  		}
  		fputc(*ap, fp);
  	    }
  	}
  	fputs("; export ", fp);
  	if (eqp - *atenv>0) fwrite(*atenv, sizeof(char), eqp - *atenv - 1, fp);
  	fputc('\n', fp);
    }
  }
#endif
    
  fwrite (action, sizeof(char), strlen (action), fp);
  fwrite ("\n", 1, 1, fp);

#ifndef HAVE_AT
  fwrite (condensorplate_1, sizeof(char), strlen (condensorplate_1), fp);
#else
  fwrite (atjob_tail, sizeof(char), strlen (atjob_tail), fp);
#endif
  
  fclose (fp);

  g_free (filename);

#ifndef HAVE_AT
  return trigger_atd ();  
#else
  if (calendar_alarm)
    {
      if(alarm_db_start () == FALSE)
        return FALSE;
      if (same_alarm(alarm_uid, id, start)) return TRUE;
      else if (alarm_uid>0) cancel_alarm_uid (alarm_uid);
    }
    
  localtime_r (&start, &tm);
  
  sprintf(call_at, "/usr/bin/at -q g -f /tmp/atjob.txt %02d:%02d %02d.%02d.%02d 2>&1", tm.tm_hour, tm.tm_min, tm.tm_mday, tm.tm_mon+1, tm.tm_year-100);
  
  if (calendar_alarm)
  {
    if (!access("/usr/bin/at", X_OK) && ((at_return=popen(call_at, "r")) != NULL))
      {
        char erase_tmpat[32];
        char junk[4];
	do
	{
          if(!fscanf(at_return, "%s ", &junk))
	  	break;
	} while (strcmp(junk, "job")!=0); 
	if (fscanf(at_return, "%i ", &alarm_uid))
	{
	  char *err;
	  if (sqlite_exec_printf (sqliteh, 
	  		    "insert into alarms (alarm_uid) values (%d)", 
	  		    NULL, NULL, &err, alarm_uid))
	  {
	    gpe_error_box (err);
	    free (err);
	    return FALSE;
	  }
	}
        pclose (at_return);
        sprintf(erase_tmpat, "rm /tmp/atjob.txt");
        system(erase_tmpat);
     }
     else
     {
        fprintf(stderr, "ERROR: Date string was invalid or could not run 'at'.  Is 'atd' running?/n");
        return FALSE;
     }
  }
  else
  {
    char erase_tmpat[32];
        
    system(call_at);
    sprintf(erase_tmpat, "rm /tmp/atjob.txt");
    system(erase_tmpat);
  }
  return TRUE;
#endif
}

/**
 * schedule_cancel_alarm:
 * @id: Unique alarm identifier. 
 * @start: Alarm time.
 * @HAVE_AT: A compile-time flag to determine whether the scheduler should
 *    use a "traditional" at (TRUE) or the ipaq's atd (FALSE) 
 *
 * Cancel a schedule alarm event.
 *
 * Returns: TRUE on success, FALSE otherwise.
 */
gboolean
schedule_cancel_alarm (guint id, time_t start)
{
#ifndef HAVE_AT
  gchar *filename = alarm_filename (id, start);

  if (unlink (filename))
    {
      perror (filename);
      g_free (filename);
      return FALSE;
    }

  g_free (filename);

  return trigger_atd ();
#else
  if (alarm_db_start () == FALSE)
    return FALSE;
  
  return cancel_alarm_uid(alarm_uid);
#endif
}
