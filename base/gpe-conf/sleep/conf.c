
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "conf.h"

static confCfg *findCfg(ipaq_conf_t *ISconf, const char *name)
{
  int i;
  confCfg *c = NULL;

  for(i = 0; i < ISconf->nConf; i++) {
    if(!strcmp(name, ISconf->clist[i].name)) {
      c = &ISconf->clist[i]; break;
    }
  }
  return c;
}


static confCfg *addConfigOpt(ipaq_conf_t *c, const char *name)
{
  confCfg *cfg;
  short flag = FALSE;
  if(*name == '#') { flag = TRUE; name++; }

  cfg = findCfg(c, name);

  if(cfg) return cfg;

  if(!(c->nConf % CONF_BLOCK)) {
    c->clist = (confCfg *)realloc(c->clist, ((c->nConf * CONF_BLOCK)+CONF_BLOCK)*sizeof(confCfg));
    memset(&c->clist[c->nConf], 0, CONF_BLOCK*sizeof(confCfg));
  }
  cfg = &c->clist[c->nConf++];
  cfg->name = strdup(name);

  return cfg;
}

static void setCfgInt(confCfg *cfg, int val, int add)
{
  int i;
  if(!cfg) return;
  for(i = 0; i < cfg->nval; i++) {
    if(cfg->val[i] == -1) {
      cfg->val[i] = (double)val;
      return;
    }
  }
  if(!(cfg->nval % CONF_BLOCK)) {
    cfg->val = (double *)realloc(cfg->val, (cfg->nval+CONF_BLOCK)*sizeof(double));
    memset(&cfg->val[cfg->nval], 0, CONF_BLOCK*sizeof(double));
  }
  i = (add ? cfg->nval : 0);
  cfg->val[i] = (double)val;
  if(!add) cfg->nval  = 1;
  else	   cfg->nval += 1;
}

void setConfigInt(ipaq_conf_t *c, const char *name, int val)
{
  confCfg *cfg = findCfg(c, name);  
  return setCfgInt(cfg, val, FALSE);
}

void addConfigInt(ipaq_conf_t *c, const char *name, int val)
{
  confCfg *cfg = findCfg(c, name);
  if(!cfg) cfg = addConfigOpt(c, name);
  return setCfgInt(cfg, val, TRUE);
}

int getConfigInt(ipaq_conf_t *c, const char *name)
{
  confCfg *cfg = findCfg(c, name);
  if(!cfg) return 0;
  cfg->configurable = TRUE;
  return (int)cfg->val[0];
}

int *getConfigIntL(ipaq_conf_t *c, int *num, const char *name)
{
  int	i, *ilist;

  confCfg *cfg = findCfg(c, name);
  if(!cfg) {*num = 0; return NULL;}
  cfg->configurable = TRUE;

  *num = cfg->nval; if(!cfg->nval) return NULL;
  ilist = (int *)calloc(cfg->nval, sizeof(int));
  for(i = 0; i < cfg->nval; i++) ilist[i] = (int)cfg->val[i];

  return ilist;
}

void delConfigInt(ipaq_conf_t *c, const char *name, int val)
{
  int i;
  confCfg *cfg = findCfg(c, name);

  for(i = 0; i < cfg->nval; i++) {
    if(cfg->val[i] == val) {
      cfg->val[i] = -1;
      break;
    }
  }
}

static void setCfgDbl(confCfg *cfg, double val, int add)
{
  int i;
  if(!cfg) return; cfg->flag = TRUE;
  for(i = 0; i < cfg->nval; i++) {
    if(cfg->val[i] == -1) {
      cfg->val[i] = val;
      return;
    }
  }
  if(!(cfg->nval % CONF_BLOCK)) {
    cfg->val = (double *)realloc(cfg->val, (cfg->nval+CONF_BLOCK)*sizeof(double));
    memset(&cfg->val[cfg->nval], 0, CONF_BLOCK*sizeof(double));
  }
  i = (add ? cfg->nval : 0);
  cfg->val[i] = val;
  if(!add) cfg->nval  = 1;
  else	   cfg->nval += 1;
}

void setConfigDbl(ipaq_conf_t *c, const char *name, double val)
{
  confCfg *cfg = findCfg(c, name);  
  return setCfgDbl(cfg, val, FALSE);
}

void addConfigDbl(ipaq_conf_t *c, const char *name, double val)
{
  confCfg *cfg = findCfg(c, name);
  if(!cfg) cfg = addConfigOpt(c, name);
  return setCfgDbl(cfg, val, TRUE);
}


double getConfigDbl(ipaq_conf_t *c, const char *name)
{
  confCfg *cfg = findCfg(c, name);
  if(!cfg) return 0.0;
  cfg->configurable = TRUE;
  return cfg->val[0];
}

double *getConfigDblL(ipaq_conf_t *c, int *num, const char *name)
{
  int	i;
  double *dlist;

  confCfg *cfg = findCfg(c, name);
  if(!cfg) {*num = 0; return NULL;}
  cfg->configurable = TRUE;

  *num = cfg->nval; if(!cfg->nval) return NULL;
  dlist = (double *)calloc(cfg->nval, sizeof(double));
  for(i = 0; i < cfg->nval; i++) dlist[i] = cfg->val[i];

  return dlist;
}

void delConfigDbl(ipaq_conf_t *c, const char *name, double val)
{
  int i;
  confCfg *cfg = findCfg(c, name);

  for(i = 0; i < cfg->nval; i++) {
    if(cfg->val[i] == val) {
      cfg->val[i] = -1;
      break;
    }
  }
}

void check_configurable(ipaq_conf_t *c)
{
  int i;
  for(i = 0; i < c->nConf; i++) {
    if(!c->clist[i].configurable)
      fprintf(stderr, "WARNING: %s flag not configurable\n", c->clist[i].name);
  }
}

ipaq_conf_t *load_ISconf(const char *fname)
{
  FILE 		*fp;
  char		line[BUFSIZ];
  char		*c, *s, *cmt;
  confCfg	*cfg = NULL;
  ipaq_conf_t	*conf = NewPtr(ipaq_conf_t);

  strcpy(conf->confName, fname);
  fp = fopen(conf->confName, "r");
  if(!fp) { perror(conf->confName); return NULL; }
  s = fgets(line, BUFSIZ, fp);
  while (!feof(fp)) {
    if((c = strchr(line, '\n')) != NULL) *c = (char)NULL;
    if(!strlen(s)) { s = fgets(line, BUFSIZ, fp); continue; }
    if(*s == '#') { s = fgets(line, BUFSIZ, fp); continue; }
    c = s; while(c && (*c != (char)NULL) && !isspace(*c)) c++;
    *c = (char)NULL; cfg = addConfigOpt(conf, s); s = ++c;
    while(s && (*s != (char)NULL) && !isdigit(*s)) s++; c = s;		/* find start of num */
    while(c && (*c != (char)NULL) && !isspace(*c)) c++; *c = (char)NULL; cmt = ++c;	/* find end of num */
    if((c = strchr(s, '.')) != NULL) setCfgDbl(cfg, atof(s), FALSE); else setCfgInt(cfg, atoi(s), FALSE);
    if(cmt && (*cmt != (char)NULL)) {
      if((c = strchr(cmt, '#')) != NULL) *c = (char)NULL;
      cmt = ++c;
    }
    while(cmt && (*cmt != (char)NULL) && isspace(*cmt)) cmt++;
    if(cmt && *cmt) cfg->desc = strdup(cmt);
    s = fgets(line, BUFSIZ, fp);
  }
  fclose(fp);
  return conf;
}


ipaq_conf_t *default_ISconf()
{
	printf("Defaults are not implemented.\n" \
	       "Please copy a valid ipaqsleep.conf to your home directory.\n");
	exit(1);
	return 0;
}
		

int save_ISconf(ipaq_conf_t *c, const char *fname)
{
  int		i, j;
  FILE		*fp;
  confCfg	*cfg = NULL;

  if((fp = fopen(fname, "w")) == NULL) {
    perror(fname); return 1;
  }

  for(i = 0; i < c->nConf; i++) {
    cfg = &c->clist[i];
    for(j = 0; j < cfg->nval; j++) {
      if(cfg->val[j] == -1) continue;
      fprintf(fp, "%-16.16s = ", cfg->name);
      if(cfg->flag)	fprintf(fp, "%-5.2f ", cfg->val[j]);
      else		fprintf(fp, "%-5d ", (int)cfg->val[j]);
      fprintf(fp, "# %s\n", ((cfg->desc) ? cfg->desc : ""));
    }
  }
  fclose(fp);
  return 0;
}
