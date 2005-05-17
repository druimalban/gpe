#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <sys/param.h>
#include <gtk/gtk.h>
#include <gpe/init.h>
#include <string.h>
#include <stdlib.h>

#include "interface.h"
#include "conf.h"
#include "confGUI.h"
#include "../applets.h"

static ipaq_conf_t *ISconf;

void 
Sleep_Save ()
{
  char		cmd[64];
  sprintf(cmd, "%s stop", ISconf->binCmd); 
  runProg(cmd);
  if(save_ISconf(ISconf, ISconf->confName)) {
    char homeConf[MAXPATHLEN];
    sprintf(homeConf, "%s/ipaq-sleep.conf", getenv("HOME"));
    if(!save_ISconf(ISconf, homeConf))
      strcpy(ISconf->confName, homeConf);
  }
  sprintf(cmd, "%s start", ISconf->binCmd);
  runProg(cmd);
}

void 
Sleep_Restore ()
{
  
}

GtkWidget *Sleep_Build_Objects()
{
  char cname[MAXPATHLEN];
  GtkWidget *GPE_Config_Sleep;

  sprintf(cname,"%s/.sleep.conf",g_get_home_dir());
  ISconf = load_ISconf(cname);
  if(ISconf == NULL) {
    strcpy(cname, "/etc/ipaq-sleep.conf");
    ISconf = load_ISconf(cname);
    if(ISconf == NULL) {
	ISconf = default_ISconf();
    }
  }
  strcpy(ISconf->binCmd, "/etc/init.d/ipaq-sleep");

  GPE_Config_Sleep = create_GPE_Config_Sleep (ISconf);

  set_conf_defaults(GPE_Config_Sleep, ISconf);
  check_configurable(ISconf);

  gtk_widget_show (GPE_Config_Sleep);

  return GPE_Config_Sleep;
}
