#include <pwd.h>
#include <sys/types.h>

#if MACHINE == ipaq
#endif

#ifndef __arm__
#define MINUSERUID 500
#else
#define MINUSERUID 100
#endif
typedef struct pwlist_s
{
  struct passwd pw;

  struct pwlist_s *next;
}pwlist;

extern pwlist *pwroot;

typedef struct userw_s
{
  GtkWidget *w;
  GtkWidget *username;
  GtkWidget *gecos;
  GtkWidget *shell;
  GtkWidget *home;
  pwlist    *cur;
}userw;

typedef struct passw_s
{
  GtkWidget *w;
  GtkWidget *oldpasswd;
  GtkWidget *newpasswd;
  GtkWidget *newpasswd2;
  pwlist    *cur;
}passw;


GtkWidget* create_userchange (pwlist *init,GtkWidget *parent);

GtkWidget* create_passwindow (pwlist *init,GtkWidget *parent);

void ReloadList(void);
int IsHidden(pwlist *t);
