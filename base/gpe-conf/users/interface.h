#include <pwd.h>
#include <sys/types.h>

#ifdef __i386__
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
  GtkWidget *username;
  GtkWidget *gecos;
  GtkWidget *shell;
  GtkWidget *home;
  pwlist    *cur;
}userw;

typedef struct passw_s
{
  GtkWidget *oldpasswd;
  GtkWidget *newpasswd;
  GtkWidget *newpasswd2;
  pwlist    *cur;
}passw;


GtkWidget* create_userchange (pwlist *init,GtkWindow *parent);

GtkWidget* create_passwindow (pwlist *init,GtkWindow *parent);

void ReloadList(void);
int IsHidden(pwlist *t);
