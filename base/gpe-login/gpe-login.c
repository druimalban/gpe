/*
 * Copyright (C) 2002 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <pwd.h>
#include <sys/types.h>
#include <shadow.h>
#include <crypt.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/wait.h>
#include <libintl.h>
#include <time.h>
#include <stdio.h>
#include <grp.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <ctype.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include <gtk/gtk.h>
#include <gdk_imlib.h>
#include <gdk/gdkx.h>
#include <gdk/gdkkeysyms.h>

#include <gpe/errorbox.h>
#include <gpe/init.h>
#include <gpe/pixmaps.h>
#include <gpe/picturebutton.h>
#include <gpe/render.h>

#include "rootpixmap.h"

#define _(x) gettext(x)

#define GPE_LOGIN_SETUP "/etc/X11/gpe-login.setup"
#define PRE_SESSION "/etc/X11/gpe-login.pre-session"
#define PASSWORD_FILE "/etc/passwd"
#define GROUP_FILE "/etc/group"
#define SHELL "/bin/sh"
#define GPE_LOGIN_CONF "/etc/gpe/gpe-login.conf"

#define bin_to_ascii(c) ((c)>=38?((c)-38+'a'):(c)>=12?((c)-12+'A'):(c)+'.')

static const char *current_username;
static GtkWidget *label_result;
static gboolean have_users;
static pid_t setup_pid;
static pid_t kbd_pid;
static const char *xkbd_path = "xkbd";
static const char *xkbd_str;
static gboolean force_xkbd;

static GtkWidget *entry_username, *entry_fullname;
static GtkWidget *entry_password, *entry_confirm;

static gboolean autolock_mode;
static gboolean hard_keys_mode;

static GtkWidget *window;
static GtkWidget *focus;
static GtkWidget *socket = NULL;
static GtkWidget *socket_box;

static guint xkbd_xid;

static Atom suspended_atom;

static int hard_key_length = 4;
static gchar *hard_key_buf;

static struct gpe_icon my_icons[] = {
  { "ok", "ok" },
  { NULL, NULL }
};

typedef struct
{
  const gchar *name;
  gchar map;
  guint keyval;
} key_map_t;

static key_map_t keymap[] = 
  {
    { "XF86Calendar", '1' },
    { "telephone", '2' },
    { "XF86Mail", '3' },
    { "XF86Start", '4' },
    { "Up", 'N' },
    { "Down", 'S' },
    { "Left", 'W' },
    { "Right", 'E' },
    { "1", '1' },
    { "2", '2' },
    { "3", '3' },
    { "4", '4' },
    { "N", 'N' },
    { "S", 'S' },
    { "W", 'W' },
    { "E", 'E' },
    { NULL }
  };

static void
cleanup_children (void)
{
  if (setup_pid)
    kill (-setup_pid, 15);

  if (kbd_pid)
    kill (kbd_pid, 15);
}

#define MAX_ARGS 8

static void
parse_xkbd_args (char *cmd, char **argv)
{
  char *p = cmd;
  char *buf = alloca (strlen (cmd) + 1), *bufp = buf;
  int nargs = 0;
  int escape = 0, squote = 0, dquote = 0;

  while (*p)
    {
      if (escape)
	{
	  *bufp++ = *p;
	  escape = 0;
	}
      else
	{
	  switch (*p)
	    {
	    case '\\':
	      escape = 1;
	      break;
	    case '"':
	      if (squote)
		*bufp++ = *p;
	      else
		dquote = !dquote;
	      break;
	    case '\'':
	      if (dquote)
		*bufp++ = *p;
	      else
		squote = !squote;
	      break;
	    case ' ':
	      if (!squote && !dquote)
		{
		  *bufp = 0;
		  if (nargs < MAX_ARGS)
		    argv[nargs++] = strdup (buf);
		  bufp = buf;
		  break;
		}
	    default:
	      *bufp++ = *p;
	      break;
	    }
	}
      p++;
    }
  
  if (bufp != buf)
    {
      *bufp = 0;
      if (nargs < MAX_ARGS)
	argv[nargs++] = strdup (buf);
    }

  argv[nargs] = NULL;
}

static void
spawn_xkbd (void)
{
  int fd[2];

  pipe (fd);
  kbd_pid = fork ();
  if (kbd_pid == 0)
    {
      char *xkbd_args[MAX_ARGS + 1];
      close (fd[0]);
      if (dup2 (fd[1], 1) < 0)
	perror ("dup2");
      close (fd[1]);
      if (fcntl (1, F_SETFD, 0))
	perror ("fcntl");
      xkbd_args[0] = (char *)xkbd_path;
      xkbd_args[1] = "-xid";
      if (xkbd_str)
	parse_xkbd_args (xkbd_str, xkbd_args + 2);
      else
	xkbd_args[2] = NULL;
      execvp (xkbd_path, xkbd_args);
      perror (xkbd_path);
      _exit (1);
    }
  
  close (fd[1]);
  
  {
    char buf[256];
    char c;
    int a = 0;
    size_t n;
    
    do {
      n = read (fd[0], &c, 1);
      if (n)
	{
	  buf[a++] = c;
	}
    } while (n && (c != 10) && (a < (sizeof (buf) - 1)));
    
    if (a)
      {
	buf[a] = 0;
	xkbd_xid = atoi (buf);
      }
  }
}

static void
cleanup_children_and_exit (int s)
{
  cleanup_children ();
  exit (0);
}

static void
set_username (GtkWidget *widget, gpointer data)
{
  current_username = (const char *)data;
}

static void
add_one_user (const char *name, GtkWidget *menu)
{
  GtkWidget *item;
  item = gtk_menu_item_new_with_label (name);
  gtk_signal_connect (GTK_OBJECT(item), "activate", 
		      GTK_SIGNAL_FUNC (set_username), (gpointer)name);
  gtk_menu_append (GTK_MENU (menu), item);
}

static void
slurp_passwd (GtkWidget *menu)
{
  struct passwd *pw;
  while (pw = getpwent (), pw != NULL)
    {
      const char *name;

      if (pw->pw_uid < 100 || pw->pw_uid >= 65534)
	continue;

      have_users = TRUE;
      name = g_strdup (pw->pw_name);
      add_one_user (name, menu);

      if (current_username == NULL)
	current_username = name;
    }

  add_one_user ("root", menu);
}

static void
move_callback (GtkWidget *widget, GtkWidget *entry)
{
  gtk_widget_grab_focus (entry);
}

static void
pre_session (const char *name)
{
  if (access (PRE_SESSION, X_OK) == 0)
    {
      pid_t spid = fork ();
      switch (spid)
	{
	case 0:
	  execl (PRE_SESSION, PRE_SESSION, name, NULL);
	  perror (PRE_SESSION);
	  _exit (1);
	  break;

	case -1:
	  perror ("fork");
	  break;

	default:
	  waitpid (spid, NULL, 0);
	  break;
	}
    }
}

static void
do_login (const char *name, uid_t uid, gid_t gid, char *dir, char *shell)
{
  int fd;

  cleanup_children ();

  pre_session (name);

  /* become session leader */
  if (setsid ())
    perror ("setsid");
  
  /* establish the user's environment */
  if (initgroups (name, gid))
    perror ("initgroups");

  if (setgid (gid))
    perror ("setgid");

  if (setuid (uid))
    perror ("setuid");
  
  setenv ("SHELL", shell, 1);
  setenv ("HOME", dir, 1);
  setenv ("USER", name, 1);
  chdir (dir);

  fd = open (".xsession-errors", O_WRONLY | O_CREAT | O_TRUNC, 0600);
  if (fd)
    {
      dup2 (fd, 1);
      dup2 (fd, 2);
      close (fd);
    }
  
  if (access (".xsession", X_OK) == 0)
    execl (".xsession", ".xsession", NULL);
  execl ("/etc/X11/Xsession", "/etc/X11/Xsession", NULL);
  execl ("/usr/bin/x-terminal-emulator", "/usr/bin/x-terminal-emulator", NULL);
  _exit (1);
}

static gboolean
login_correct (const gchar *pwstr, struct passwd **pwe_ret)
{
  struct passwd *pwe;
  struct spwd *spe;
  char *p;

  pwe = getpwnam (current_username);
  if (pwe == NULL)
    return FALSE;

  spe = getspnam (current_username);
  if (spe)
    pwe->pw_passwd = spe->sp_pwdp;

  p = crypt (pwstr, pwe->pw_passwd);

  if (strcmp (p, pwe->pw_passwd))
    return FALSE;

  if (pwe_ret)
    *pwe_ret = pwe;

  return TRUE;
}

static void
enter_lock_callback (GtkWidget *widget, GtkWidget *entry)
{
  gchar *pwstr = gtk_editable_get_chars (GTK_EDITABLE (entry), 0, -1);
  gtk_entry_set_text (GTK_ENTRY (entry), "");

  if (login_correct (pwstr, NULL))
    {
      gdk_keyboard_ungrab (GDK_CURRENT_TIME);
      gtk_widget_hide (window);
      kill (kbd_pid, 15);
    }
  else
    gtk_label_set_text (GTK_LABEL (label_result), _("Login incorrect"));

  memset (pwstr, 0, strlen (pwstr));
  g_free (pwstr);
}

static GdkFilterReturn
filter (GdkXEvent *xevp, GdkEvent *ev, gpointer p)
{
  XEvent *xev = (XEvent *)xevp;
  
  if (xev->type == PropertyNotify)
    {
      if (xev->xproperty.atom == suspended_atom)
	{
	  spawn_xkbd ();
	  gtk_label_set_text (GTK_LABEL (label_result), "");
	  gtk_widget_set_usize (window, gdk_screen_width (), gdk_screen_height ());
	  gtk_widget_show_all (window);
	  gtk_widget_grab_focus (focus);
	  gdk_keyboard_grab (focus->window, TRUE, GDK_CURRENT_TIME);
	  if (xkbd_xid)
	    {
	      socket = gtk_socket_new ();
	      gtk_widget_show (socket);
	      gtk_container_add (GTK_CONTAINER (socket_box), socket);
	      gtk_socket_steal (GTK_SOCKET (socket), xkbd_xid);
	    }
	}
    }

  return GDK_FILTER_CONTINUE;
}

static void
enter_callback (GtkWidget *widget, GtkWidget *entry)
{
  struct passwd *pwe;
  gchar *pwstr = gtk_editable_get_chars (GTK_EDITABLE (entry), 0, -1);
  gtk_entry_set_text (GTK_ENTRY (entry), "");

  if (current_username == NULL)
    return;

  if (login_correct (pwstr, &pwe))
    {
      do_login (current_username, pwe->pw_uid, pwe->pw_gid, pwe->pw_dir, pwe->pw_shell);
      gtk_main_quit ();
    }
  else
    gtk_label_set_text (GTK_LABEL (label_result), _("Login incorrect"));

  memset (pwstr, 0, strlen (pwstr));
  g_free (pwstr);
}

static void
enter_newuser_callback (GtkWidget *widget, gpointer h)
{
  const gchar *username, *fullname, *password, *confirm;
  char *cryptstr;
  struct passwd *pwe;
  struct group *gre;
  time_t tm;
  char salt[2];
  char buf[256];
  char home[80];
  gid_t gid = 100;
  uid_t uid = 100;
  FILE *fp;

  username = gtk_entry_get_text (GTK_ENTRY (entry_username));
  fullname = gtk_entry_get_text (GTK_ENTRY (entry_fullname));
  password = gtk_entry_get_text (GTK_ENTRY (entry_password));
  confirm = gtk_entry_get_text (GTK_ENTRY (entry_confirm));

  pwe = getpwnam (username);
  if (pwe)
    {
      gpe_error_box (_("User already exists"));

      gtk_entry_set_text (GTK_ENTRY (entry_username), "");
      gtk_entry_set_text (GTK_ENTRY (entry_password), "");
      gtk_entry_set_text (GTK_ENTRY (entry_confirm), "");
      gtk_widget_grab_focus (entry_username);
      return;
    }

  if (strcmp (password, confirm))
    {
      gpe_error_box (_("Passwords don't match"));
      gtk_entry_set_text (GTK_ENTRY (entry_password), "");
      gtk_entry_set_text (GTK_ENTRY (entry_confirm), "");
      gtk_widget_grab_focus (entry_password);
      return;
    }

  if (password[0] == 0)
    {
      gpe_error_box (_("Empty password not allowed"));
      gtk_widget_grab_focus (entry_password);
      return;
    }

  time (&tm);
  salt[0] = bin_to_ascii (tm & 0x3f);
  salt[1] = bin_to_ascii ((tm >> 6) & 0x3f);
  cryptstr = crypt (password, salt);

  while (pwe = getpwent (), pwe != NULL)
    {
      if (pwe->pw_uid < 60000 && pwe->pw_uid >= uid)
	uid = pwe->pw_uid + 1;
    }
  endpwent ();

  while (gre = getgrent (), gre != NULL)
    {
      if (gre->gr_gid >= gid)
	gid = gre->gr_gid + 1;
    }
  endgrent ();

  snprintf (home, sizeof (home), "/home/%s", username);
  mkdir (home, 0770);
  chown (home, uid, gid);

  fp = fopen (PASSWORD_FILE, "a");
  if (fp == NULL)
    {
      gpe_perror_box (PASSWORD_FILE);
      exit (1);
    }
  snprintf (buf, sizeof (buf), "%s:%s:%d:%d:%s:%s:%s\n",
	    username, cryptstr, uid, gid, fullname, home, SHELL);
  fputs (buf, fp);
  fclose (fp);

  fp = fopen (GROUP_FILE, "a");
  if (fp == NULL)
    {
      gpe_perror_box (GROUP_FILE);
      exit (1);
    }
  snprintf (buf, sizeof (buf), "%s:x:%d:\n", username, gid);
  fputs (buf, fp);
  fclose (fp);

  do_login (username, uid, gid, home, "/bin/sh");

  gtk_main_quit ();
}

static void
hard_key_insert (gchar c)
{
  struct passwd *pwe;

  if (current_username == NULL)
    return;

  memmove (hard_key_buf, hard_key_buf + 1, hard_key_length - 1);
  hard_key_buf[hard_key_length - 1] = c;

  if (login_correct (hard_key_buf, &pwe))
    {
      if (autolock_mode)
	{
	  gdk_keyboard_ungrab (GDK_CURRENT_TIME);
	  gtk_widget_hide (window);
	  memset (hard_key_buf, ' ', hard_key_length);
	}
      else
	{
	  do_login (current_username, pwe->pw_uid, pwe->pw_gid, pwe->pw_dir, pwe->pw_shell);
	  gtk_main_quit ();
	}
    }
}

static gboolean
key_press_event (GtkWidget *window, GdkEventKey *event)
{
  key_map_t *k = keymap;
  while (k->name)
    {
      if (k->keyval == event->keyval)
	{
	  hard_key_insert (k->map);
	  return TRUE;
	}
      k++;
    }
  printf ("No match for %d\n", event->keyval);
  return FALSE;
}

static void
mapped (GtkWidget *window)
{
  Pixmap rmap = GetRootPixmap (GDK_DISPLAY ());
  if (rmap != None)
    {
      Pixmap pmap;
      pmap = CutWinPixmap (GDK_DISPLAY(), GDK_WINDOW_XWINDOW (window->window), rmap, 
			   GDK_GC_XGC (window->style->black_gc));
      if (pmap != None)
	{
	  GdkPixmap *gpmap = gdk_pixmap_foreign_new (pmap);      
	  window->style->bg_pixmap[GTK_STATE_NORMAL] = gpmap;
	  gtk_widget_set_style (window, window->style);
	}
    }
}

int
main (int argc, char *argv[])
{
  GtkWidget *option, *menu;
  GtkWidget *vbox, *vbox2;
  GtkWidget *ok_button;
  GtkWidget *frame;
  GtkWidget *calibrate_hint;
  Display *dpy;
  Window root;
  gboolean geometry_set = FALSE;
  int i;
  char *geometry_str = FALSE;
  gboolean flag_geom = FALSE;
  gboolean flag_transparent = FALSE;
  gboolean flag_xkbd = FALSE;
  FILE *cfp;

  if (gpe_application_init (&argc, &argv) == FALSE)
    exit (1);

  setlocale (LC_ALL, "");

  bindtextdomain (PACKAGE, PACKAGE_LOCALE_DIR);
  textdomain (PACKAGE);

  cfp = fopen (GPE_LOGIN_CONF, "r");
  if (cfp)
    {
      char buf[256];
      char *p;
      while (!feof (cfp))
	{
	  if (p = fgets (buf, sizeof (buf), cfp), p != NULL)
	    {
	      while (*p && isspace (p[strlen (p) - 1]))
		p[strlen (p) - 1] = 0;

	      while (isspace (*p))
		p++;

	      if (*p == '#' || *p == 0)
		continue;

	      if (!strcmp (p, "transparent"))
		flag_transparent = TRUE;
	      else if (!strcmp (p, "hard-keys"))
		hard_keys_mode = TRUE;
	      else if (!strncmp (p, "xkbd", 4))
		{
		  char *q = p + 4;
		  while (isspace (*p))
		    p++;
		  force_xkbd = TRUE;
		  if (*q && *q != '\n')
		    xkbd_str = q;
		}
	    }
	}
      fclose (cfp);
    }

  for (i = 1; i < argc; i++)
    {
      if (flag_geom)
	{
	  geometry_str = argv[i];
	  flag_geom = FALSE;
	}
      else if (flag_xkbd)
	{
	  xkbd_str = argv[i];
	  flag_xkbd = FALSE;
	}
      else if (strcmp (argv[i], "--autolock") == 0)
	autolock_mode = TRUE;
      else if (strcmp (argv[i], "--geometry") == 0 || strncmp (argv[i], "-g", 2) == 0)
	flag_geom = TRUE;
      else if (strcmp (argv[i], "--transparent") == 0)
	flag_transparent = TRUE;
      else if (strcmp (argv[i], "--hard-keys") == 0)
	hard_keys_mode = TRUE;
      else if (strcmp (argv[i], "--xkbd") == 0)
	{
	  force_xkbd = TRUE;
	  flag_xkbd = TRUE;
	}
    }

  signal (SIGCHLD, SIG_IGN);

  gpe_load_icons (my_icons);

  if (! autolock_mode)
    {
      if (access (GPE_LOGIN_SETUP, X_OK) == 0)
	{
	  setup_pid = fork ();
	  switch (setup_pid)
	    {
	    case 0:
	      {
		pid_t mypid = getpid ();
		setpgid (0, mypid);
		system (GPE_LOGIN_SETUP);
		_exit (0);
	      }
	      
	    case -1:
	      perror ("fork");
	      break;
	      
	    default:
	      waitpid (setup_pid, NULL, 0);
	      break;
	    }
	}
      
      signal (SIGINT, cleanup_children_and_exit);
      signal (SIGQUIT, cleanup_children_and_exit);
      signal (SIGTERM, cleanup_children_and_exit);
    }

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

  gtk_window_set_default_size (GTK_WINDOW (window), 240, 320);

  if (hard_keys_mode)
    {
      key_map_t *k = keymap;
      while (k->name)
	{
	  k->keyval = gdk_keyval_from_name (k->name);
	  k++;
	}

      hard_key_buf = g_malloc (hard_key_length + 1);
      memset (hard_key_buf, ' ', hard_key_length);
      hard_key_buf[hard_key_length] = 0;
    }

  if (!autolock_mode && (!hard_keys_mode || force_xkbd))
    {
      socket = gtk_socket_new ();
      spawn_xkbd ();
    }

  gtk_widget_realize (window);

  dpy = GDK_DISPLAY ();
  root = GDK_ROOT_WINDOW ();

  if (autolock_mode)
    {
      suspended_atom = XInternAtom (dpy, "GPE_DISPLAY_LOCKED", 0);
      gdk_window_set_override_redirect (window->window, TRUE);

      current_username = getenv ("USER");
      if (current_username == NULL)
	{
	  fprintf (stderr, "USER not set\n");
	  exit (1);
	}
    }
  else
    {
      if (geometry_str)
	{
	  int x = -1, y = -1, h, w;
	  int val = XParseGeometry (geometry_str, &x, &y, &w, &h);
	  if ((val & (HeightValue | WidthValue)) == (HeightValue | WidthValue))
	    {
	      gtk_widget_set_usize (window, w, h);
	      geometry_set = TRUE;
	    }
	  if (val & (XValue | YValue))
	    gtk_widget_set_uposition (window, x, y);
	}

      if (geometry_set == FALSE)
	{
	  FILE *fp = fopen ("/etc/X11/gpe-login.geometry", "r");
	  if (fp)
	    {
	      char buf[1024];
	      if (fgets (buf, sizeof (buf), fp))
		{
		  int val;
		  int x = -1, y = -1, h, w;
		  buf[strlen(buf)-1] = 0;
		  val = XParseGeometry (buf, &x, &y, &w, &h);
		  if ((val & (HeightValue | WidthValue)) == (HeightValue | WidthValue))
		    {
		      gtk_widget_set_usize (window, w, h);
		      geometry_set = TRUE;
		    }
		  if (val & (XValue | YValue))
		    gtk_widget_set_uposition (window, x, y);
		}
	      fclose (fp);
	    }
	}

      if (geometry_set == FALSE)
	{
	  /* If no window manager is running, set size to full-screen */
	  /*
	    <mallum> pb: request the 'SubstructureRedirect' event mask on the root window
	    <mallum> pb: if that fails, theres already a window manager running
	  */
	  GdkEventMask ev = gdk_window_get_events (window->window);
	  int r;
	  gdk_error_trap_push ();
	  XSelectInput (GDK_WINDOW_XDISPLAY (window->window),
			RootWindow (GDK_WINDOW_XDISPLAY (window->window), 0),
			SubstructureRedirectMask);
	  gdk_flush ();
	  r = gdk_error_trap_pop ();
	  gdk_window_set_events (window->window, ev);
	  if (r == 0)
	    {
	      gtk_widget_set_usize (window, gdk_screen_width (), gdk_screen_height ());
	      geometry_set = TRUE;
	    }
	}
    }

  menu = gtk_menu_new ();
  slurp_passwd (menu);

  ok_button = gpe_picture_button (window->style, _("OK"), "ok");

  vbox2 = gtk_vbox_new (FALSE, 0);

  calibrate_hint = gtk_label_new (_("Press Record to recalibrate touchscreen"));
  gtk_label_set_line_wrap (GTK_LABEL (calibrate_hint), TRUE);

  if (autolock_mode || have_users)
    {
      GtkWidget *hbox_password;
      GtkWidget *login_label, *password_label;
      GtkWidget *entry = NULL, *table;

      frame = gtk_frame_new (autolock_mode ? _("Screen locked") : _("Log in"));

      login_label = gtk_label_new (_("Username"));
      label_result = gtk_label_new ("");
      gtk_rc_parse_string ("widget '*login_result_label*' style 'gpe_login_result'");
      gtk_widget_set_name (label_result, "login_result_label");

      if (autolock_mode)
	{
	  option = gtk_label_new (current_username);
	  gtk_misc_set_alignment (GTK_MISC (option), 0.0, 0.5);
	}
      else
	{
	  option = gtk_option_menu_new ();
	  gtk_option_menu_set_menu (GTK_OPTION_MENU (option), menu);
	}

      gtk_signal_connect (GTK_OBJECT (window), "delete_event",
			  GTK_SIGNAL_FUNC (gtk_main_quit), NULL);
      
      table = gtk_table_new (2, 2, FALSE);

      if (! hard_keys_mode)
	{
	  entry = gtk_entry_new ();
	  gtk_entry_set_visibility (GTK_ENTRY (entry), FALSE);
	  password_label = gtk_label_new (_("Password"));
	  hbox_password = gtk_hbox_new (FALSE, 5);
	  gtk_box_pack_start (GTK_BOX (hbox_password), entry, TRUE, TRUE, 0);
	  gtk_box_pack_end (GTK_BOX (hbox_password), ok_button, FALSE, FALSE, 0);
	  gtk_label_set_justify (GTK_LABEL (password_label), GTK_JUSTIFY_LEFT);
	  gtk_misc_set_alignment (GTK_MISC (password_label), 0, 0.5);

	  gtk_table_attach_defaults (GTK_TABLE (table), password_label, 0, 1, 1, 2);
	  gtk_table_attach_defaults (GTK_TABLE (table), hbox_password, 1, 2, 1, 2);
	}

      gtk_table_attach_defaults (GTK_TABLE (table), login_label, 0, 1, 0, 1);
      gtk_table_attach_defaults (GTK_TABLE (table), option, 1, 2, 0, 1);

      gtk_label_set_justify (GTK_LABEL (login_label), GTK_JUSTIFY_LEFT);
      gtk_misc_set_alignment (GTK_MISC (login_label), 0, 0.5);

      vbox = gtk_vbox_new (FALSE, 0);

      gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);
      gtk_box_pack_start (GTK_BOX (vbox), label_result, FALSE, FALSE, 0);
      
      if (hard_keys_mode)
	{
	  GtkWidget *keys_hint = gtk_label_new (_("Enter password with hard keys"));
	  gtk_widget_show (keys_hint);
	  gtk_box_pack_start (GTK_BOX (vbox), keys_hint, FALSE, FALSE, 0);
	}

      if (socket)
	gtk_box_pack_start (GTK_BOX (vbox), socket, TRUE, TRUE, 0);
      else
	{
	  socket_box = gtk_event_box_new ();
	  gtk_widget_show (socket_box);
	  gtk_box_pack_start (GTK_BOX (vbox), socket_box, TRUE, TRUE, 0);
	}

      gtk_container_add (GTK_CONTAINER (frame), vbox);
      gtk_container_set_border_width (GTK_CONTAINER (frame), 5);
      gtk_container_set_border_width (GTK_CONTAINER (vbox), 5);
      
      if (! hard_keys_mode)
	{
	  if (autolock_mode)
	    {
	      gtk_signal_connect (GTK_OBJECT (entry), "activate",
				  GTK_SIGNAL_FUNC (enter_lock_callback), entry);
	      gtk_signal_connect (GTK_OBJECT (ok_button), "clicked",
				  GTK_SIGNAL_FUNC (enter_lock_callback), entry);
	    }
	  else
	    {
	      gtk_signal_connect (GTK_OBJECT (entry), "activate",
				  GTK_SIGNAL_FUNC (enter_callback), entry);
	      gtk_signal_connect (GTK_OBJECT (ok_button), "clicked",
				  GTK_SIGNAL_FUNC (enter_callback), entry);
	    }
	  focus = entry;
	}
      else
	{
	  gtk_signal_connect (GTK_OBJECT (window), "key-press-event",
			      GTK_SIGNAL_FUNC (key_press_event), window);
	  focus = window;
	}

      gtk_box_pack_start (GTK_BOX (vbox2), frame, FALSE, FALSE, 0);

      if (! autolock_mode)
	gtk_box_pack_start (GTK_BOX (vbox2), calibrate_hint, FALSE, FALSE, 0);
    }
  else
    {
      GtkWidget *label_username, *label_fullname;
      GtkWidget *label_password, *label_confirm;
      GtkWidget *hbox_username, *hbox_fullname;
      GtkWidget *hbox_password, *hbox_confirm;
      GtkWidget *table;
      GtkWidget *vbox;
      GtkWidget *hbox = gtk_hbox_new (FALSE, 0);

      label_username = gtk_label_new (_("Username"));
      label_fullname = gtk_label_new (_("Full name"));
      label_password = gtk_label_new (_("Password"));
      label_confirm = gtk_label_new (_("Confirm password"));

      entry_username = gtk_entry_new ();
      entry_fullname = gtk_entry_new ();
      entry_password = gtk_entry_new ();
      entry_confirm = gtk_entry_new ();

      hbox_username = gtk_hbox_new (0, FALSE);
      hbox_fullname = gtk_hbox_new (0, FALSE);
      hbox_password = gtk_hbox_new (0, FALSE);
      hbox_confirm = gtk_hbox_new (0, FALSE);

      gtk_entry_set_visibility (GTK_ENTRY (entry_password), FALSE);
      gtk_entry_set_visibility (GTK_ENTRY (entry_confirm), FALSE);

      gtk_box_pack_start (GTK_BOX (hbox_username), label_username, 
			  FALSE, FALSE, 4);
      gtk_box_pack_start (GTK_BOX (hbox_fullname), label_fullname, 
			  FALSE, FALSE, 4);
      gtk_box_pack_start (GTK_BOX (hbox_password), label_password, 
			  FALSE, FALSE, 4);
      gtk_box_pack_start (GTK_BOX (hbox_confirm), label_confirm,
			  FALSE, FALSE, 4);

      table = gtk_table_new (4, 2, FALSE);
      gtk_table_attach_defaults (GTK_TABLE (table), hbox_username, 
				 0, 1, 0, 1);
      gtk_table_attach_defaults (GTK_TABLE (table), hbox_fullname, 
				 0, 1, 1, 2);
      gtk_table_attach_defaults (GTK_TABLE (table), hbox_password,
				 0, 1, 2, 3);
      gtk_table_attach_defaults (GTK_TABLE (table), hbox_confirm, 
				 0, 1, 3, 4);

      gtk_table_attach_defaults (GTK_TABLE (table), entry_username, 
				 1, 2, 0, 1);
      gtk_table_attach_defaults (GTK_TABLE (table), entry_fullname, 
				 1, 2, 1, 2);
      gtk_table_attach_defaults (GTK_TABLE (table), entry_password,
				 1, 2, 2, 3);
      gtk_table_attach_defaults (GTK_TABLE (table), entry_confirm, 
				 1, 2, 3, 4);

      gtk_signal_connect (GTK_OBJECT (entry_username), "activate",
			 GTK_SIGNAL_FUNC (move_callback), entry_fullname);
      gtk_signal_connect (GTK_OBJECT (entry_fullname), "activate",
			 GTK_SIGNAL_FUNC (move_callback), entry_password);
      gtk_signal_connect (GTK_OBJECT (entry_password), "activate",
			 GTK_SIGNAL_FUNC (move_callback), entry_confirm);
      gtk_signal_connect (GTK_OBJECT (entry_confirm), "activate",
			 GTK_SIGNAL_FUNC (enter_newuser_callback), NULL);
      gtk_signal_connect (GTK_OBJECT (ok_button), "clicked",
			 GTK_SIGNAL_FUNC (enter_newuser_callback), NULL);

      frame = gtk_frame_new (_("New user"));

      vbox = gtk_vbox_new (FALSE, 0);
      gtk_box_pack_start (GTK_BOX (vbox), table, TRUE, TRUE, 0);
      if (socket)
	gtk_box_pack_start (GTK_BOX (vbox), socket, TRUE, TRUE, 0);
      
      gtk_container_add (GTK_CONTAINER (frame), vbox);
      gtk_container_set_border_width (GTK_CONTAINER (frame), 5);
      gtk_container_set_border_width (GTK_CONTAINER (table), 5);

      focus = entry_username;

      gtk_box_pack_end (GTK_BOX (hbox), ok_button, FALSE, FALSE, 5);
      gtk_box_pack_start (GTK_BOX (vbox2), frame, FALSE, FALSE, 0);
      gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 5);
      gtk_box_pack_start (GTK_BOX (vbox2), calibrate_hint, FALSE, FALSE, 0);
    }

  gtk_container_add (GTK_CONTAINER (window), vbox2);

  gtk_widget_add_events (GTK_WIDGET (window), GDK_BUTTON_PRESS_MASK);

  if (flag_transparent)
    gtk_signal_connect (GTK_OBJECT (window), "map-event",
			GTK_SIGNAL_FUNC (mapped), NULL);

  if (autolock_mode)
    {
      XSelectInput (dpy, root, PropertyChangeMask);
      gdk_window_add_filter (GDK_ROOT_PARENT (), filter, 0);
    }
  else
    {
      gtk_widget_show_all (window);
      gtk_widget_grab_focus (focus);
      if (xkbd_xid && socket)
	gtk_socket_steal (GTK_SOCKET (socket), xkbd_xid);
    }

  gtk_main ();

  cleanup_children ();

  return 0;
}
