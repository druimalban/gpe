#ifndef CONF_H
#define CONF_H

#include <stdio.h>
#include <sys/param.h>

#ifndef NewPtr
#define	NewPtr(x)	(x *)calloc(1, sizeof(x))
#endif

#define	CONF_BLOCK	5

#define DIM_STEP 1

#ifndef	TRUE
#define	TRUE	1
#define	FALSE	0
#endif

typedef struct _confCfg {
  int		nval;
  double	*val;
  char		*name;
  char		*desc;
  short		flag, configurable;
} confCfg;

typedef struct _ipaq_conf_t {
  char		binCmd[MAXPATHLEN];
  char		confName[MAXPATHLEN];

  int		nConf;
  confCfg	*clist;

  int		blFD;
} ipaq_conf_t;

void setConfigInt(ipaq_conf_t *c, const char *name, int val);
int getConfigInt(ipaq_conf_t *c, const char *name);
int *getConfigIntL(ipaq_conf_t *c, int *num, const char *name);
void addConfigInt(ipaq_conf_t *c, const char *name, int val);
void delConfigInt(ipaq_conf_t *c, const char *name, int val);

void setConfigDbl(ipaq_conf_t *c, const char *name, double val);
double getConfigDbl(ipaq_conf_t *c, const char *name);
double *getConfigDblL(ipaq_conf_t *c, int *num, const char *name);
void addConfigDbl(ipaq_conf_t *c, const char *name, double val);
void delConfigDbl(ipaq_conf_t *c, const char *name, double val);

void check_configurable(ipaq_conf_t *c);
ipaq_conf_t *load_ISconf(const char *fname);
ipaq_conf_t *default_ISconf();
int save_ISconf(ipaq_conf_t *c, const char *fname);

void Sleep_Save ();
void Sleep_Restore ();

#endif
