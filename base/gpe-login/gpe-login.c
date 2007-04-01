/*
 * Copyright (C) 2002, 2003, 2005 Philip Blundell <philb@gnu.org>
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
#include <dirent.h>
#include <fcntl.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <locale.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/XF86keysym.h>

#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <gdk/gdkkeysyms.h>

#include <gpe/errorbox.h>
#include <gpe/init.h>
#include <gpe/spacing.h>
#include <gpe/translabel.h>

#include "gpe-ownerinfo.h"

#define _(x) gettext(x)
#define N_(x) (x)

#define GPE_LOGIN_SETUP "/etc/X11/gpe-login.setup"
#define PRE_SESSION "/etc/X11/gpe-login.pre-session"
#define PASSWORD_FILE "/etc/passwd"
#define GROUP_FILE "/etc/group"
#define SHELL "/bin/sh"
#define GPE_LOGIN_CONF "/etc/gpe/gpe-login.conf"
#define GPE_LOCALE_ALIAS "/etc/gpe/locales/"
#define GPE_LOCALE_ALIAS_FALLBACK "/etc/gpe/locale.alias"
#define GPE_LOCALE_DEFAULT "/etc/gpe/locale.default"
#define GPE_LOCALE_USER_FILE ".gpe/locale"
#define GPE_OWNERINFO_DONTSHOW_FILE "/etc/gpe/gpe-ownerinfo.dontshow"
/* Number of milliseconds to hold the stylus down before recalibration is called */
#define RECALIBRATION_TIMEOUT 5000
#define XTSCAL_PATH "/usr/bin/xtscal"
#ifndef DEBUG
#define DEBUG 0
#endif

#define bin_to_ascii(c) ((c)>=38?((c)-38+'a'):(c)>=12?((c)-12+'A'):(c)+'.')

static GtkWidget *label_result;
static gboolean have_users;
static gboolean root_password_set;
static pid_t setup_pid;
static pid_t kbd_pid;
static const char *xkbd_path = "xkbd";
static const char *xkbd_str;
static gchar **add_groups;
static gboolean force_xkbd;
static const char *display;

static GtkWidget *entry_username, *entry_fullname;
static GtkWidget *entry_password, *entry_confirm;

static gboolean autolock_mode;
static gboolean hard_keys_mode;

static GtkWidget *window;
static GtkWidget *focus;
static GtkWidget *socket = NULL;
static GtkWidget *socket_box;
static GtkWidget *ownerinfo = NULL;

static guint xkbd_xid;

static Atom suspended_atom;

static int hard_key_length = 4;
static gchar *hard_key_buf;

static gchar *pango_lang_code;
static const char *current_username;

typedef struct
{
  gchar *name;
  gchar *locale;
  guint menu_pos;
} locale_item_t;

static locale_item_t *current_locale;
static locale_item_t *default_locale;
static locale_item_t *old_locale;
static GSList *locale_system_list;
static GtkWidget *language_menu;
GtkWidget *vbox2, *root_password_vbox;
gboolean locale_changed;

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
  
guint32 timer;
 
/* function protos */
static void cleanup_children (void);
static void add_menu_callback (GtkWidget * menu, const char *label,
			       void *func, gpointer data);
static void parse_xkbd_args (const char *cmd, char **argv);
static void spawn_xkbd (void);
static void cleanup_children_and_exit (int s);
static void set_username (GtkWidget * widget, gpointer data);
static void slurp_passwd (GtkWidget * menu);
static void move_callback (GtkWidget * widget, GtkWidget * entry);
static void pre_session (const char *name);
static void do_login (const char *name, uid_t uid, gid_t gid, char *dir,
		      char *shell);
static gboolean login_correct (const gchar * pwstr, struct passwd **pwe_ret);
static void enter_lock_callback (GtkWidget * widget, GtkWidget * entry);
static GdkFilterReturn filter (GdkXEvent * xevp, GdkEvent * ev, gpointer p);
static void enter_callback (GtkWidget * widget, GtkWidget * entry);
static void enter_newuser_callback (GtkWidget * widget, gpointer h);
static void hard_key_insert (gchar c);
static gboolean key_press_event (GtkWidget * window, GdkEventKey * event);

static locale_item_t *locale_parse_line (char *p);
static GSList *locale_get_list (const char *flocale);
static GSList *locale_get_files (void);
static void set_current_locale (GtkWidget * widget, gpointer data);
static void build_language_menu (void);
static locale_item_t *locale_read_user (const char *username);
static int locale_set_xprop (const char *locale);
static int locale_set (const char *locale);
static void locale_try_user (const char *user);
static void locale_update_menu (GtkOptionMenu *optionmenu, gpointer data);
static locale_item_t *locale_get_file_single (const char *fname);

static gboolean cal_countdown_button_press (GtkWidget *widget, GdkEventButton *event, gpointer data);
static gboolean cal_countdown_button_release (GtkWidget *widget, GdkEventButton *event, gpointer data);
static void add_recalibration_signals (GtkWidget *widget);

GtkWidget *build_new_user_box (void);

#define MAX_ARGS 8

static void
parse_xkbd_args (const char *cmd, char **argv)
{
  const char *p = cmd;
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
add_menu_callback (GtkWidget * menu, const char *label, void *func, gpointer data)
{
  GtkWidget *entry;
  entry = gtk_menu_item_new_with_label (label);
  add_recalibration_signals (entry);
  g_signal_connect (G_OBJECT (entry), "activate", G_CALLBACK (func), data);
  gtk_menu_append (GTK_MENU (menu), entry);
}

static void
cleanup_children (void)
{
  if (setup_pid)
    kill (-setup_pid, 15);

  if (kbd_pid)
    kill (kbd_pid, 15);
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
slurp_passwd (GtkWidget *menu)
{
  struct passwd *pw;

  setpwent ();

  while (pw = getpwent (), pw != NULL)
    {
      const char *name;

      if (!strcmp (pw->pw_name, "root") 
	  && strcmp (pw->pw_passwd, "*"))
	root_password_set = TRUE;

      if (pw->pw_uid < 100 || pw->pw_uid >= 65534)
	continue;

      have_users = TRUE;
      name = g_strdup (pw->pw_name);
      add_menu_callback (menu, name, &set_username, (gpointer)name);

      if (current_username == NULL)
	current_username = name;
    }

  endpwent ();

  add_menu_callback (menu, "root", &set_username, "root");
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
  int ngroups, naddgroups, ngroups_max = sysconf (_SC_NGROUPS_MAX);
  struct group *grp;
  gchar **g;
  gid_t *groups;
  cleanup_children ();

  pre_session (name);

  /* become session leader */
  if (setsid ())
    perror ("setsid");
  
  /* establish the user's environment */
  if (initgroups (name, gid))
    perror ("initgroups");

  /* add supplementary group membership */
  if (add_groups)
    {
      ngroups = getgroups (0, NULL);
      naddgroups = g_strv_length (add_groups);
      groups = (gid_t *) malloc ((ngroups + naddgroups) * sizeof (gid_t));
      if (!groups)
	perror ("malloc");
      if (!getgroups (ngroups, groups))
	perror ("getgroups");
      g = add_groups;
      while ((ngroups_max > ngroups + 1) && (*g))
	{
	  if ((grp = getgrnam (*g++)))
	    groups[ngroups++] = grp->gr_gid;
	}
      if (setgroups (ngroups, groups))
	perror ("setgroups");
    }

  if (setgid (gid)) 
    {
      perror ("setgid");
      exit(1);
    }

  if (setuid (uid))
    {
      perror ("setuid");
      exit(1);
    }

  clearenv ();
  setenv ("SHELL", shell, 1);
  setenv ("HOME", dir, 1);
  setenv ("USER", name, 1);
  setenv ("LANG", current_locale->locale, 1);
  setenv ("DISPLAY", display, 1);

  chdir (dir);

  if (locale_changed && current_locale != old_locale)
    {
      FILE *fp;
      fp = fopen (GPE_LOCALE_USER_FILE, "w");
      if (fp)
	{
	  fprintf (fp, "%s\n", current_locale->locale);
	  fclose (fp);
	}
    }

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
    gtk_label_set_markup (GTK_LABEL (label_result),
			  g_strdup_printf ("<span lang='%s' foreground='red'><b>%s</b></span>",
					   pango_lang_code,
					   _("Login incorrect")));
  
  memset (pwstr, 0, strlen (pwstr));
  g_free (pwstr);
}

static GdkFilterReturn
power_button_filter (GdkXEvent *xevp, GdkEvent *ev, gpointer p)
{
  XEvent *xev = (XEvent *)xevp;

  if (xev->type == KeyPress || xev->type == KeyRelease)
    {
      KeySym ks = XKeycodeToKeysym (xev->xany.display, xev->xkey.keycode, 0);

      if (ks == XF86XK_PowerDown)
	{
	  xev->xkey.window = DefaultRootWindow (xev->xany.display);
	  XSendEvent (xev->xany.display, DefaultRootWindow (xev->xany.display),
		      True, KeyPressMask | KeyReleaseMask, xev);
	}
    }

  return GDK_FILTER_CONTINUE;
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
	  gtk_label_set_markup (GTK_LABEL (label_result), "");
	  gtk_widget_set_usize (window, gdk_screen_width (), gdk_screen_height ());
	  gtk_widget_show_all (window);
      if (ownerinfo)
        {  
          if (access(GPE_OWNERINFO_DONTSHOW_FILE, F_OK))
            gtk_widget_show(ownerinfo);
          else
            gtk_widget_hide(ownerinfo);
         }
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
    gtk_label_set_markup (GTK_LABEL (label_result),
			  g_strdup_printf ("<span lang='%s' foreground='red'>%s</span>",
					   pango_lang_code,
					   _("Login incorrect")));

  memset (pwstr, 0, strlen (pwstr));
  g_free (pwstr);
}

static void
my_error_box (gchar *text)
{
  GtkWidget *w, *f;
  GtkWidget *ok, *vbox, *hbox, *icon, *label;

  w = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  f = gtk_frame_new (NULL);
  gtk_container_add (GTK_CONTAINER (w), f);
  vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (f), vbox);
  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);
  icon = gtk_image_new_from_stock (GTK_STOCK_DIALOG_ERROR, GTK_ICON_SIZE_DIALOG);
  gtk_box_pack_start (GTK_BOX (hbox), icon, FALSE, FALSE, 0);
  label = gtk_label_new (text);
  gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);
  gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
  ok = gtk_button_new_from_stock (GTK_STOCK_OK);
  gtk_box_pack_end (GTK_BOX (vbox), ok, FALSE, FALSE, 2);
  gtk_window_set_position (GTK_WINDOW (w), GTK_WIN_POS_CENTER);

  gtk_widget_show_all (w);

  add_recalibration_signals (ok);
  g_signal_connect (G_OBJECT (ok), "clicked", gtk_main_quit, NULL);

  gtk_main ();

  gtk_widget_destroy (w);
}

static void
enter_root_callback (GtkWidget *widget, gpointer h)
{
  const gchar *password, *confirm;
  GtkWidget *new_box;
  struct passwd *pwe;
  time_t tm;
  char salt[2];
  FILE *fp;

  password = gtk_entry_get_text (GTK_ENTRY (entry_password));
  confirm = gtk_entry_get_text (GTK_ENTRY (entry_confirm));

  if (strcmp (password, confirm))
    {
      my_error_box (_("Passwords don't match"));
      gtk_entry_set_text (GTK_ENTRY (entry_password), "");
      gtk_entry_set_text (GTK_ENTRY (entry_confirm), "");
      gtk_widget_grab_focus (entry_password);
      return;
    }

  if (password[0] == 0)
    {
      my_error_box (_("Empty password not allowed"));
      gtk_widget_grab_focus (entry_password);
      return;
    }

  fp = fopen ("/etc/passwd.new", "w");
  if (! fp)
    {
      my_error_box ("Couldn't open new password file for writing");
      exit (1);
    }

  setpwent ();
  while (pwe = getpwent (), pwe != NULL)
    {
      if (! strcmp (pwe->pw_name, "root"))
	{
	  time (&tm);
	  salt[0] = bin_to_ascii (tm & 0x3f);
	  salt[1] = bin_to_ascii ((tm >> 6) & 0x3f);
	  pwe->pw_passwd = crypt (password, salt);
	}

      putpwent (pwe, fp);
    }

  endpwent ();
  fclose (fp);
  unlink ("/etc/passwd");
  rename ("/etc/passwd.new", "/etc/passwd");
 
  gtk_container_remove (GTK_CONTAINER (vbox2), root_password_vbox);

  new_box = build_new_user_box ();
  gtk_widget_show_all (new_box);
  gtk_box_pack_start (GTK_BOX (vbox2), new_box, TRUE, TRUE, 0);
  gtk_widget_grab_focus (focus);
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
      my_error_box (_("User already exists"));

      gtk_entry_set_text (GTK_ENTRY (entry_username), "");
      gtk_entry_set_text (GTK_ENTRY (entry_password), "");
      gtk_entry_set_text (GTK_ENTRY (entry_confirm), "");
      gtk_widget_grab_focus (entry_username);
      return;
    }

  if (strcmp (password, confirm))
    {
      my_error_box (_("Passwords don't match"));
      gtk_entry_set_text (GTK_ENTRY (entry_password), "");
      gtk_entry_set_text (GTK_ENTRY (entry_confirm), "");
      gtk_widget_grab_focus (entry_password);
      return;
    }

  if (password[0] == 0)
    {
      my_error_box (_("Empty password not allowed"));
      gtk_widget_grab_focus (entry_password);
      return;
    }

  time (&tm);
  salt[0] = bin_to_ascii (tm & 0x3f);
  salt[1] = bin_to_ascii ((tm >> 6) & 0x3f);
  cryptstr = crypt (password, salt);

  setpwent ();
  while (pwe = getpwent (), pwe != NULL)
    {
      if (pwe->pw_uid < 60000 && pwe->pw_uid >= uid)
	uid = pwe->pw_uid + 1;
    }
  endpwent ();

  setgrent ();
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

static int
locale_set_xprop (const char *locale)
{
  Atom atom;
  Display *dpy;
  Window root;

  dpy = GDK_DISPLAY ();

  root = RootWindow (dpy, 0);
  atom = XInternAtom (dpy, "_GPE_LOCALE", 0);

  XChangeProperty (dpy, root, atom, XA_STRING, 8, PropModeReplace,
		   locale, strlen (locale));

  return 0;
}

static int
locale_set (const char *locale)
{
  setlocale (LC_ALL, locale);
  return locale_set_xprop (locale);
}  

/* Try and set the best locale for a user to, in order:
 * - A valid user supplied locale
 * - The system default locale
 * If neither are found, the current_locale is left unchanged.
 */
static void
locale_try_user (const char *user)
{
  locale_item_t *item = NULL;
  
  if (DEBUG)
    fprintf(stderr,"locale_try_user: user &%p\n",user);
 
  /* Check for a valid user supplied item */ 
  item = locale_read_user (user);

  /* Otherwise, the default_locale */
  if (item == NULL)
    item = default_locale;

  if (item)
    {
      if (DEBUG) 
	{
	  fprintf(stderr,"locale_try_user: got locale %s",item->name);
	  fprintf(stderr," menu_pos: %d\n",item->menu_pos);
	}      
  
      /* update state */
      locale_set (item->locale);
  
      current_locale = item;
      old_locale = item;
      locale_changed = FALSE;

      gtk_option_menu_set_history (GTK_OPTION_MENU (language_menu), current_locale->menu_pos);
    }
}

/* Attempt to parse the given locale line and return an appropriate
 * locale_item_t *. Caller is responsible for freeing the locale_item_t.
 * line format is (described as regex): 
 * 	^"\(.*\)" \(.*\)$
 * Where:
 * The first match is a user-friendly name, (locale_item_t *)->name
 * The second match is a system locale name, (locale_item_t *)->locale
 */  
locale_item_t *
locale_parse_line (char *p)
{

  locale_item_t *item = NULL;
  char *name = NULL, *locale = NULL;

  while (*p && isspace (p[strlen (p) - 1]))
    p[strlen (p) - 1] = 0;
  while (isspace (*p))
    p++;
  if (*p != '"')
    return NULL;
  else
    {
      int i = 0;
      p++;
      /* quoted string is the name */
      while (*(p + i) != '\0' && *(p + i) != '"')
	{
	  i++;
	}
      if (i)
	{
	  name = (gchar *) malloc (i + 1);
	  memcpy (name, p, i);
	  name[i] = '\0';
	  //printf("p: %s got name %s\n",p,name);
	}
      /* point past the " */
      p = &p[i + 1];
      i = 0;

      /* chomp whitespace */
      while (isspace (*p))
	p++;
      /* remaining text is locale */
      while (*(p + i))
	{
	  i++;
	}
      if (i)
	{
	  locale = (gchar *) malloc (i + 1);
	  memcpy (locale, p, i);
	  locale[i] = '\0';
	  //printf("got locale %s\n",locale);
	}
      if (name && locale)
	{
	  item = (locale_item_t *) calloc (1, sizeof (locale_item_t));
	  item->name = (gchar *) strdup (name);
	  item->locale = (gchar *) strdup (locale);

	}
      if (name)
	{
	  free (name);
	  name = NULL;
	}
      if (locale)
	{
	  free (locale);
	  locale = NULL;
	}
    }
  return item;
}

/*
 * In order to filter not installed locales we check if the locale
 * directory is present in the filesystem. Currently we check for the 
 * directory with the exact name and with the .utf8 extension only.
 * This is expected to work in most environments.
 */
gboolean
locale_install_check(const char* locale)
{
    gboolean result = FALSE;
    char *dir = g_strdup_printf(PREFIX "/lib/locale/%s", locale);

    /* check for equal locale */    
    if (!access(dir, F_OK))
        result = TRUE;

    /* check for the .utf8 type directory */
    if (strstr(locale, ".UTF-8"))
      {
        int l1, l2, lm;
        char *cu = strstr(locale, ".UTF-8");
        cu [0] = 0;
        char *dir_alt = g_strconcat (PREFIX "/lib/locale/", locale, ".utf8", NULL);
        
        if (!access(dir_alt, F_OK))
          result = TRUE;
        g_free(dir_alt);
      }
    g_free(dir);
      
    return result;
}


/* Read the given file name and parse it with locale_parse_line,
 * returning a GSList of the (locale_item_t *)'s found
 */
static GSList *
locale_get_list (const char *flocale)
{

  FILE *fp;
  GSList *list=NULL;
  locale_item_t *item = NULL;
  gchar lbuf[256], *p;   

  if (!(fp = fopen (flocale, "r")))
    {
      return NULL;
    }

  if (DEBUG)
    fprintf(stderr,"locale_get_list: flocale: %s\n",flocale);

  while (!feof (fp))
    {
      if ((p = fgets (lbuf, sizeof (lbuf), fp)))
        {
          item = locale_parse_line (p);
          if (item)
            {
              if (locale_install_check(item->locale))
                list = g_slist_append (list, item);
              else
                {
                  g_free(item->locale);
                  g_free(item->name);
                  g_free(item);
                }
            }
        }
    }    
  fclose (fp);
  return list;
}

/* Return a list of system locale files as determined from 
 * the GPE_LOCALE_ALIAS define. For directories, this will be a list of
 * all regular files within that directory.
 */
static GSList *
locale_get_files (void)
{
  struct stat st;
  GSList *flist = NULL;
  DIR *dir;
  struct dirent *dent;
  gboolean fallback = FALSE;

  if (stat(GPE_LOCALE_ALIAS,&st))
    {
      perror("locale_get_files: could not stat, trying fallback");
      fallback = TRUE;
      if (stat(GPE_LOCALE_ALIAS_FALLBACK, &st))
        {
          perror("locale_get_files: no fallback, giving up");
          return NULL;
        }
    }
  
  /* read all file entries if it is a directory */  
  if (S_ISDIR (st.st_mode))
    {
      int dirlen;
      
      dirlen = strnlen(GPE_LOCALE_ALIAS,NAME_MAX);
    
      if (! (dir=opendir(GPE_LOCALE_ALIAS)))
        {
          perror("locale_get_list: could not opendir!");
          return NULL;
        }
      while ( (dent=readdir(dir)) )
        {
          gchar *fname;
          gint len;
          
          len = strnlen(dent->d_name,NAME_MAX) + dirlen + 1;
          
          if ( !(fname = (gchar *) malloc(len)) )
            {
              perror("locale_get_files: could not malloc!");
              exit(1);
            }
            
          snprintf(fname,len,"%s%s",GPE_LOCALE_ALIAS,dent->d_name);
          
          if ( stat(fname,&st) || !S_ISREG(st.st_mode) )
            {
              if (DEBUG)
                fprintf(stderr,"locale_get_files: continue on %s\n",fname);
              continue;
            }
              
          if (fname)
            {              
              flist = g_slist_append(flist,fname);
              if (DEBUG)
                {
                  fprintf(stderr,"locale_get_files: dent %s",dent->d_name);
                  fprintf(stderr," fname: %s\n",fname);
                }
            }
        }
    }
  else if (S_ISREG (st.st_mode))
    /* just one file. copy the filename cause we dont own it */
    {
      gchar *fname;
      if (fallback) 
          fname = g_strdup (GPE_LOCALE_ALIAS_FALLBACK);
      else
          fname = g_strdup (GPE_LOCALE_ALIAS);
      if (fname)
        flist = g_slist_append (flist, fname);
    }
  else
    /* strange.. */
    {
      if (DEBUG)
        fprintf(stderr,"locale_get_list: strange.. not a file or a dir\n");
      return NULL;
    }
  return flist;   
}    

/* Set current_locale based on entry selected by user */
static void
set_current_locale (GtkWidget * widget, gpointer data)
{
  locale_item_t *item = (locale_item_t *) data;
 
  if (item)
    {
      current_locale = item;
      locale_set (item->locale);
      locale_changed = TRUE;
    }
}

/* Initialise locale handling and build the language drop-down menu */
static void
build_language_menu (void)
{
  GtkWidget *m = gtk_menu_new ();
  locale_item_t *c_locale, *item;
  GSList *filelist;
  GSList *iter;
  int i;

  language_menu = gtk_option_menu_new ();

  c_locale = g_malloc (sizeof (locale_item_t));
  c_locale->name = "English";
  c_locale->locale = "C";
  locale_system_list = g_slist_append (NULL, c_locale);

  /* get list of locale files */
  filelist = locale_get_files ();
  if (filelist)
    {
      GSList *iter;
      
      for (iter = filelist; iter; iter = iter->next)
        {
          gchar *file;
	  GSList *tmplist;

	  file = iter->data;
          tmplist = locale_get_list (file);
          if (tmplist)
	    locale_system_list = g_slist_concat (locale_system_list, tmplist);

          /* no longer need the file name */
          g_free (file);
        }

      g_slist_free (filelist);
    }
    
  /* initialise the default_locale, if it exists */
  default_locale = locale_get_file_single (GPE_LOCALE_DEFAULT);
  if (default_locale == NULL)
    default_locale = c_locale;

  for (i = 0, iter = locale_system_list; iter; iter = iter->next, i++)
    {
      item = iter->data;
      item->menu_pos = i;
      add_menu_callback (m, item->name, set_current_locale, item);
    }
    
  gtk_option_menu_set_menu (GTK_OPTION_MENU (language_menu), m);    
}

static void
locale_update_menu (GtkOptionMenu *optionmenu, gpointer data)
{
  if (DEBUG)
    fprintf(stderr,"locale_update: changed, current user: %s\n",
            current_username);

  locale_try_user (current_username);
}

static locale_item_t *
locale_get_file_single (const char *fname)
{
  FILE *fp;
  locale_item_t *item = NULL;
  
  fp = fopen (fname, "r");

  if (!fp)
    {
      perror (fname);
      return NULL;
    }

  while (!item && !feof (fp))
    {
      char *p;
      char lbuf[80];

      p = fgets (lbuf, sizeof (lbuf), fp);
      if (p)
	{
	  GSList *iter;

	  while (isspace (*p))
	    p++;
	  while (p[0] && isspace (p[strlen (p) - 1]))
	    p[strlen (p) - 1] = 0;
	  
	  for (iter = locale_system_list; iter; iter = iter->next)
	    {
	      locale_item_t *ip = iter->data;
	      
	      if (!strcmp (ip->locale, lbuf))
		{
		  item = ip;
		  break;
		}
	    }
	}
    }

  fclose (fp);

  return item;
}

static locale_item_t *
locale_read_user (const char *username)
{
  struct passwd *pwe;
  struct stat st;
  char *fname = NULL;
  locale_item_t *item = NULL;

  pwe = getpwnam (username);
  if (! pwe)
    return NULL;

  if (lstat (pwe->pw_dir, &st))
    {
      perror (pwe->pw_dir);
      return NULL;
    }

  if (! S_ISDIR (st.st_mode))
    {
      fprintf (stderr, "locale_read_user: %s not a directory\n", pwe->pw_dir);
      return NULL;
    }

  fname = g_strdup_printf ("%s/%s", pwe->pw_dir, GPE_LOCALE_USER_FILE);

  if (lstat (fname, &st))
    {
      perror (fname);
      g_free (fname);
      return NULL;
    }

  if (! S_ISREG (st.st_mode))
    {
      fprintf (stderr, "read_user_locale: %s not a regular file!\n", fname);
      g_free (fname);
      return NULL;
    }

  item = locale_get_file_single (fname);
  
  g_free (fname);

  if (DEBUG)
    fprintf(stderr,"locale_read_user: got locale %s\n",item->name);

  return item;
}

static void
calibrate_hint_hook (GtkWidget *w, gpointer data)
{
  gchar *p = g_strdup_printf ("<span lang='%s'>%s <i>%s</i> %s <i>%s</i> %s</span>",
			      pango_lang_code,
			      _("Holding down the stylus for greater than"),
			      _("5 seconds"),
			      _(", or holding"),
			      _("record"),
			      _(", recalibrates the touchscreen."));

  gtk_label_set_markup (GTK_LABEL (w), p);

  g_free (p);
}

GtkWidget *
build_root_password_box (void)
{
  GtkWidget *label_prompt, *label_title;
  GtkWidget *label_password, *label_confirm;
  GtkWidget *table;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *ok_button;
  guint gpe_boxspacing = gpe_get_boxspacing ();
  gchar *s;

  ok_button = gtk_button_new_from_stock (GTK_STOCK_OK);
 
  label_title = gtk_label_new (NULL);
  s = g_strdup_printf ("<b>%s</b>", _("System setup"));
  gtk_label_set_markup (GTK_LABEL (label_title), s);
  g_free (s);

  label_prompt = gtk_label_new (NULL);
  gtk_label_set_markup (GTK_LABEL (label_prompt), _("Please choose a password for the\n<i>root</i> account.\n\nThis will be needed when performing\nsystem administration.\n"));
  label_password = gtk_label_new (_("Password"));
  label_confirm = gtk_label_new (_("Confirm"));

  gtk_misc_set_alignment (GTK_MISC (label_password), 0.0, 0.5);
  gtk_misc_set_alignment (GTK_MISC (label_confirm), 0.0, 0.5);

  entry_password = gtk_entry_new ();
  entry_confirm = gtk_entry_new ();
  
  gtk_entry_set_visibility (GTK_ENTRY (entry_password), FALSE);
  gtk_entry_set_visibility (GTK_ENTRY (entry_confirm), FALSE);

  table = gtk_table_new (2, 2, FALSE);
  gtk_table_attach (GTK_TABLE (table), label_password,
		    0, 1, 0, 1, GTK_EXPAND, 0, gpe_boxspacing, gpe_boxspacing);
  gtk_table_attach (GTK_TABLE (table), label_confirm, 
		    0, 1, 1, 2, GTK_EXPAND, 0, gpe_boxspacing, gpe_boxspacing);

  gtk_table_attach (GTK_TABLE (table), entry_password,
		    1, 2, 0, 1, GTK_EXPAND | GTK_FILL, 0, 0, gpe_boxspacing);
  gtk_table_attach (GTK_TABLE (table), entry_confirm, 
		    1, 2, 1, 2, GTK_EXPAND | GTK_FILL, 0, 0, gpe_boxspacing);
  
  add_recalibration_signals (entry_password);
  add_recalibration_signals (entry_confirm);
  add_recalibration_signals (ok_button);
  g_signal_connect (G_OBJECT (entry_password), "activate",
		    G_CALLBACK (move_callback), entry_confirm);
  g_signal_connect (G_OBJECT (entry_confirm), "activate",
		    G_CALLBACK (enter_root_callback), NULL);
  g_signal_connect (G_OBJECT (ok_button), "clicked",
		    G_CALLBACK (enter_root_callback), NULL);
  
  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_end (GTK_BOX (hbox), ok_button, FALSE, FALSE, 0);
  
  vbox = gtk_vbox_new (FALSE, gpe_boxspacing);
  gtk_box_pack_start (GTK_BOX (vbox), label_title, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), label_prompt, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), table, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  
  focus = entry_password;

  return vbox;
}

GtkWidget *
build_new_user_box (void)
{
  GtkWidget *label_username, *label_fullname;
  GtkWidget *label_password, *label_confirm;
  GtkWidget *hbox_username, *hbox_fullname;
  GtkWidget *hbox_password, *hbox_confirm;
  GtkWidget *table;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *label;
  GtkWidget *ok_button;
  GtkWidget *label_language, *hbox_language;
  guint gpe_boxspacing = gpe_get_boxspacing ();
  gchar *s;

  ok_button = gtk_button_new_from_stock (GTK_STOCK_OK);

  label_username = gtk_label_new_with_translation (PACKAGE, N_("Username"));
  label_fullname = gtk_label_new_with_translation (PACKAGE, N_("Full name"));
  label_password = gtk_label_new_with_translation (PACKAGE, N_("Password"));
  label_confirm = gtk_label_new_with_translation (PACKAGE, N_("Confirm password"));
  label_language = gtk_label_new_with_translation (PACKAGE, N_("Language"));

  entry_username = gtk_entry_new ();
  entry_fullname = gtk_entry_new ();
  entry_password = gtk_entry_new ();
  entry_confirm = gtk_entry_new ();
  
  hbox_username = gtk_hbox_new (0, FALSE);
  hbox_fullname = gtk_hbox_new (0, FALSE);
  hbox_password = gtk_hbox_new (0, FALSE);
  hbox_confirm = gtk_hbox_new (0, FALSE);
  hbox_language = gtk_hbox_new (0, FALSE);

  gtk_entry_set_visibility (GTK_ENTRY (entry_password), FALSE);
  gtk_entry_set_visibility (GTK_ENTRY (entry_confirm), FALSE);

  gtk_box_pack_start (GTK_BOX (hbox_username), label_username, 
		      FALSE, FALSE, gpe_boxspacing);
  gtk_box_pack_start (GTK_BOX (hbox_fullname), label_fullname, 
		      FALSE, FALSE, gpe_boxspacing);
  gtk_box_pack_start (GTK_BOX (hbox_password), label_password, 
		      FALSE, FALSE, gpe_boxspacing);
  gtk_box_pack_start (GTK_BOX (hbox_confirm), label_confirm,
		      FALSE, FALSE, gpe_boxspacing);
  gtk_box_pack_start (GTK_BOX (hbox_language), label_language,
		      FALSE, FALSE, gpe_boxspacing);

  table = gtk_table_new (5, 2, FALSE);
  gtk_table_attach_defaults (GTK_TABLE (table), hbox_username, 
			     0, 1, 0, 1);
  gtk_table_attach_defaults (GTK_TABLE (table), hbox_fullname, 
			     0, 1, 1, 2);
  gtk_table_attach_defaults (GTK_TABLE (table), hbox_password,
			     0, 1, 2, 3);
  gtk_table_attach_defaults (GTK_TABLE (table), hbox_confirm, 
			     0, 1, 3, 4);
  gtk_table_attach_defaults (GTK_TABLE (table), hbox_language,
			     0, 1, 4, 5);
  
  gtk_table_attach_defaults (GTK_TABLE (table), entry_username, 
			     1, 2, 0, 1);
  gtk_table_attach_defaults (GTK_TABLE (table), entry_fullname, 
			     1, 2, 1, 2);
  gtk_table_attach_defaults (GTK_TABLE (table), entry_password,
			     1, 2, 2, 3);
  gtk_table_attach_defaults (GTK_TABLE (table), entry_confirm, 
			     1, 2, 3, 4);
  gtk_table_attach_defaults (GTK_TABLE (table), language_menu,
			     1, 2, 4, 5);
  
  add_recalibration_signals (entry_username);
  add_recalibration_signals (entry_fullname);
  g_signal_connect (G_OBJECT (entry_username), "activate",
		    G_CALLBACK (move_callback), entry_fullname);
  g_signal_connect (G_OBJECT (entry_fullname), "activate",
		    G_CALLBACK (move_callback), entry_password);
  g_signal_connect (G_OBJECT (entry_password), "activate",
		    G_CALLBACK (move_callback), entry_confirm);
  g_signal_connect (G_OBJECT (entry_confirm), "activate",
		    G_CALLBACK (enter_newuser_callback), NULL);
  g_signal_connect (G_OBJECT (ok_button), "clicked",
		    G_CALLBACK (enter_newuser_callback), NULL);
  
  label = gtk_label_new (NULL);
  s = g_strdup_printf ("<b>%s</b>", _("New user details"));
  gtk_label_set_markup (GTK_LABEL (label), s);
  g_free (s);
  
  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_end (GTK_BOX (hbox), ok_button, FALSE, FALSE, 0);
  
  vbox = gtk_vbox_new (FALSE, gpe_boxspacing);
  gtk_box_pack_start (GTK_BOX (vbox), label, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), table, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  
  focus = entry_username;
  current_locale = default_locale;

  return vbox;
}

/* Following three functions detect pressing down of stylus for recalibration */
static gboolean
check_time(guint32 *time, guint32 current_time)
{
  guint32 time_elapsed = current_time - *time;
  if (*time == 0)
    return FALSE;
  *time = 0;
	
  if (time_elapsed >= RECALIBRATION_TIMEOUT)
    {
      system (XTSCAL_PATH);
      return TRUE;
    }
	
  return FALSE;
}

static gboolean
cal_countdown_button_press (GtkWidget *widget, GdkEventButton *event, gpointer data)
{
  if (event->button == 1)
    {
      if (*((guint32*)data) == 0)
      *((guint32*)data) = event->time;
    }
  return FALSE;
}

static gboolean
cal_countdown_button_release (GtkWidget *widget, GdkEventButton *event, gpointer data)
{
  guint32 time_elapsed;
	
  time_elapsed = event->time - *((guint32*)data);

  return check_time ((guint32 *)data, event->time);
}

/* Add button press/release signals to a widget for recalibration */
static void
add_recalibration_signals (GtkWidget *widget)
{
  gtk_widget_add_events (GTK_WIDGET (widget), GDK_BUTTON_PRESS_MASK |
  				 GDK_BUTTON_RELEASE_MASK | GDK_POINTER_MOTION_MASK |
  				  GDK_BUTTON_MOTION_MASK);
  g_signal_connect (G_OBJECT (widget), "button-press-event",
		    G_CALLBACK (cal_countdown_button_press), &timer);
  g_signal_connect (G_OBJECT (widget), "button-release-event",
		    G_CALLBACK (cal_countdown_button_release), &timer);
}

int
main (int argc, char *argv[])
{
  GtkWidget *option, *menu;
  GtkWidget *calibrate_hint;
  Display *dpy;
  Window root;
  gboolean geometry_set = FALSE;
  int i;
  char *geometry_str = FALSE;
  gboolean flag_geom = FALSE;
  gboolean flag_transparent = FALSE;
  gboolean flag_xkbd = FALSE;
  gboolean flag_force_new_user = FALSE;
  FILE *cfp;
  GdkCursor *cursor;
  GtkWidget *socket_frame;

  /* gchar *gpe_catindent = gpe_get_catindent ();  */
  /* guint gpe_catspacing = gpe_get_catspacing (); */
  guint gpe_boxspacing = gpe_get_boxspacing ();
  guint gpe_border     = gpe_get_border ();
  
  /* Initialise timer to 0 for recalibration */
  timer = 0;

  gtk_init (&argc, &argv);
  gtk_set_locale ();

  display = getenv ("DISPLAY");
  
  /* (LanguageCode) */
  /* TRANSLATORS: please replace this with your own Pango language code: */
  pango_lang_code = g_strdup (_("en"));
  
  setlocale (LC_ALL, "");

  bindtextdomain (PACKAGE, PACKAGE_LOCALE_DIR);
  textdomain (PACKAGE);
  bind_textdomain_codeset (PACKAGE, "UTF-8");

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
		  while (isspace (*q))
		    q++;
		  force_xkbd = TRUE;
		  if (*q && *q != '\n')
		    xkbd_str = q;
		}
	      else if (!strncmp (p, "addgroups", 9))
		{
		  char *q = p + 9;
		  while (isspace (*q))
		    q++;
		  if (*q && *q != '\n')
		    add_groups = g_strsplit (q, ",", 0);
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
      else if (strcmp (argv[i], "--force-new-user") == 0)
	flag_force_new_user = TRUE;
    }

  signal (SIGCHLD, SIG_IGN);

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

  cursor = gdk_cursor_new (GDK_LEFT_PTR);
  gdk_window_set_cursor (window->window, cursor);

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

	      /* Set larger fonts on big screens */
	      if (gdk_screen_width () >= 800)
		gtk_rc_parse_string ("gtk-font-name = \"Sans 14\"");
	      else if (gdk_screen_width () >= 640)
		gtk_rc_parse_string ("gtk-font-name = \"Sans 12\"");
	      else if (gdk_screen_width () >= 480)
		gtk_rc_parse_string ("gtk-font-name = \"Sans 10\"");
	    }
	}

      if (geometry_set == FALSE)
	gtk_window_set_default_size (GTK_WINDOW (window), 240, 320);
    }

  menu = gtk_menu_new ();
  slurp_passwd (menu);

  if (flag_force_new_user)
    {
      have_users = FALSE;
      //root_password_set = FALSE;
    }

  vbox2 = gtk_vbox_new (FALSE, gpe_boxspacing);

  calibrate_hint = gtk_label_new (NULL);
  gtk_widget_add_translation_hook (calibrate_hint, calibrate_hint_hook, NULL);

  add_recalibration_signals (window);
  g_signal_connect (G_OBJECT (window), "delete_event",
		    G_CALLBACK (gtk_main_quit), NULL);

  socket_frame = gtk_aspect_frame_new (NULL, 0, 0, 3.0, FALSE);
  gtk_frame_set_shadow_type (GTK_FRAME (socket_frame), GTK_SHADOW_NONE);
  gtk_container_set_border_width (GTK_CONTAINER (socket_frame), 0);

  if (autolock_mode || have_users)
    {
      GtkWidget *hbox_password;
      GtkWidget *login_label, *lock_label, *password_label;
      GtkWidget *entry = NULL, *table;
      guint xpad = gpe_boxspacing, ypad = 1;
      GtkWidget *ok_button;
      GtkWidget *vbox;

      ok_button = gtk_button_new_from_stock (GTK_STOCK_OK);

      GTK_WIDGET_UNSET_FLAGS (ok_button, GTK_CAN_FOCUS);

      if (autolock_mode) 
	{
	  lock_label = gtk_label_new (NULL);
	  gtk_label_set_markup (GTK_LABEL (lock_label),
				g_strdup_printf ("<span lang='%s'><b>%s</b></span>",
						 pango_lang_code,
						 _("Screen locked")));
	  gtk_box_pack_start (GTK_BOX (vbox2), lock_label, FALSE, FALSE, 0);
	}
      
      login_label = gtk_label_new_with_translation (PACKAGE, N_("Username"));
      label_result = gtk_label_new (NULL);

      if (autolock_mode)
	{
	  option = gtk_label_new (current_username);
	  gtk_misc_set_alignment (GTK_MISC (option), 0.0, 0.5);
	}
      else
	{
	  option = gtk_option_menu_new ();
	  gtk_option_menu_set_menu (GTK_OPTION_MENU (option), menu);
        add_recalibration_signals (option);
  	  g_signal_connect (G_OBJECT (option), "changed", 
	                    G_CALLBACK (locale_update_menu),NULL);
	}

      table = gtk_table_new (3, 2, FALSE);

      if (! hard_keys_mode)
	{
	  entry = gtk_entry_new ();
	  gtk_entry_set_visibility (GTK_ENTRY (entry), FALSE);
	  password_label = gtk_label_new_with_translation (PACKAGE, N_("Password"));
	  hbox_password = gtk_hbox_new (FALSE, gpe_boxspacing);
	  gtk_box_pack_start (GTK_BOX (hbox_password), entry, TRUE, TRUE, 0);
	  gtk_box_pack_end (GTK_BOX (hbox_password), ok_button, FALSE, FALSE, 0);

	  gtk_table_attach (GTK_TABLE (table), password_label, 0, 1, 1, 2, 0, GTK_EXPAND | GTK_FILL, xpad, ypad);
	  gtk_table_attach (GTK_TABLE (table), hbox_password, 1, 2, 1, 2, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, xpad, ypad);

	  gtk_label_set_justify (GTK_LABEL (password_label), GTK_JUSTIFY_LEFT);
	  gtk_misc_set_alignment (GTK_MISC (password_label), 0, 0.5);
	}

      if (! autolock_mode)
	{
	  GtkWidget *language_label = gtk_label_new_with_translation (PACKAGE, N_("Language"));
	  build_language_menu ();
	  locale_try_user (current_username);

	  gtk_table_attach (GTK_TABLE (table), language_label, 0, 1, 2, 3, 0, GTK_EXPAND | GTK_FILL, xpad, ypad);
	  gtk_table_attach (GTK_TABLE (table), language_menu, 1, 2, 2, 3, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, xpad, ypad);
	}

      gtk_table_attach (GTK_TABLE (table), login_label, 0, 1, 0, 1, 0, GTK_EXPAND | GTK_FILL, xpad, ypad);
      gtk_table_attach (GTK_TABLE (table), option, 1, 2, 0, 1, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, xpad, ypad);

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
	{
	  gtk_container_add (GTK_CONTAINER (socket_frame), socket);
	  gtk_box_pack_start (GTK_BOX (vbox), socket_frame, TRUE, TRUE, 0);
      gtk_widget_show_all(socket_frame);
	}
      else
	{
	  socket_box = gtk_event_box_new ();
	  gtk_widget_show (socket_box);
	  gtk_container_add (GTK_CONTAINER (socket_box), socket);
	  gtk_box_pack_start (GTK_BOX (vbox), socket_box, TRUE, FALSE, 0);
	}

      gtk_container_set_border_width (GTK_CONTAINER (vbox), 0);
      
      if (! hard_keys_mode)
	{
        add_recalibration_signals (entry);
        add_recalibration_signals (ok_button);
	  if (autolock_mode)
	    {
	      g_signal_connect (G_OBJECT (entry), "activate",
				  G_CALLBACK (enter_lock_callback), entry);
	      g_signal_connect (G_OBJECT (ok_button), "clicked",
				  G_CALLBACK (enter_lock_callback), entry);
	    }
	  else
	    {
	      g_signal_connect (G_OBJECT (entry), "activate",
				  G_CALLBACK (enter_callback), entry);
	      g_signal_connect (G_OBJECT (ok_button), "clicked",
				  G_CALLBACK (enter_callback), entry);
	    }
	  focus = entry;
	}
      else
	{
	  g_signal_connect (G_OBJECT (window), "key-press-event",
			      G_CALLBACK (key_press_event), window);
	  focus = window;
	}

      gtk_box_pack_start (GTK_BOX (vbox2), vbox, TRUE, TRUE, 0);

#if 0
      if (! autolock_mode)
	gtk_box_pack_start (GTK_BOX (vbox2), calibrate_hint, FALSE, FALSE, 0);
#endif
    }
  else
    {
      GtkWidget *vbox;

      build_language_menu ();

      if (! root_password_set)
	{
	  vbox = build_root_password_box ();
	  root_password_vbox = vbox;
	}
      else
	vbox = build_new_user_box ();

      gtk_box_pack_start (GTK_BOX (vbox2), vbox, FALSE, FALSE, 0);

      if (socket)
	{
	  gtk_container_add (GTK_CONTAINER (socket_frame), socket);
	  gtk_box_pack_end (GTK_BOX (vbox2), socket_frame, TRUE, TRUE, 0);
	}
      
#if 0
      gtk_box_pack_end (GTK_BOX (vbox2), calibrate_hint, FALSE, FALSE, 0);
#endif
    }

  gtk_container_set_border_width (GTK_CONTAINER (vbox2), gpe_border);

  if (autolock_mode || have_users)
    {
      ownerinfo = gpe_owner_info ();
      gtk_box_pack_start (GTK_BOX (vbox2), ownerinfo, TRUE, TRUE, 0);
      if (!access(GPE_OWNERINFO_DONTSHOW_FILE, F_OK))
		gtk_widget_hide(ownerinfo);  
    }

  gtk_container_add (GTK_CONTAINER (window), vbox2);

  gtk_widget_add_events (GTK_WIDGET (window), GDK_BUTTON_PRESS_MASK);

  if (autolock_mode)
    {
      XSelectInput (dpy, root, PropertyChangeMask);
      gdk_window_add_filter (GDK_ROOT_PARENT (), filter, 0);
      gdk_window_add_filter (focus->window, power_button_filter, 0);
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
