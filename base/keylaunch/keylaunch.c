/* 
 * Oroborus Window Manager - KeyLaunch
 *
 * Copyright (C) 2001 Ken Lynch
 * Copyright (C) 2002 Stefan Pfetzing
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  
 */

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/stat.h>

typedef struct _Key Key;

struct _Key
{
  int keycode, modifier;
  char *command;
  Key *next;
};

Display *dpy;
Window root;
Key *key;
int NumLockMask, CapsLockMask, ScrollLockMask;
time_t last_update;

/*
 *
 * General functions
 *
 */
 
void print_error(char *error, int critical)
{
#ifdef DEBUG
  printf("print_error\n");
#endif

  fprintf(stderr, "KEYLAUNCH: %s", error);
  if(critical) exit(1);
}

time_t get_last_update(char *dir, char *file)
{
  char *file_name;
  struct stat stat_buf;

  if((file_name=malloc(strlen(dir)+strlen(file)+2))==NULL)
    print_error("Memory allocation failed.\n", True);

  sprintf(file_name, "%s/%s", dir, file);

  /* If no rc file then set time equal to last_update */
  if(stat(file_name, &stat_buf))
    stat_buf.st_mtime=last_update;
  free(file_name);
  return stat_buf.st_mtime;
}

/*
 *
 * Key functions
 *
 */

void init_keyboard()
{
  XModifierKeymap *xmk=NULL;
  KeyCode *map;
  int m, k;

#ifdef DEBUG
  printf("init_keyboard\n");
#endif

  xmk=XGetModifierMapping(dpy);
  if(xmk)
  {
    map=xmk->modifiermap;
    for(m=0;m<8;m++)
      for(k=0;k<xmk->max_keypermod; k++, map++)
      {
        if(*map==XKeysymToKeycode(dpy, XK_Num_Lock))
          NumLockMask=(1<<m);
        if(*map==XKeysymToKeycode(dpy, XK_Caps_Lock))
          CapsLockMask=(1<<m);
        if(*map==XKeysymToKeycode(dpy, XK_Scroll_Lock))
          ScrollLockMask=(1<<m);
      }
    XFreeModifiermap(xmk);
  }
}

void grab_key(int keycode, unsigned int modifiers, Window w)
{
  if(keycode)
  {
    XGrabKey(dpy, keycode, modifiers, w, False, GrabModeAsync, GrabModeAsync);
    XGrabKey(dpy, keycode, modifiers|NumLockMask, w, False, GrabModeAsync, GrabModeAsync); 
    XGrabKey(dpy, keycode, modifiers|CapsLockMask, w, False, GrabModeAsync, GrabModeAsync);
    XGrabKey(dpy, keycode, modifiers|ScrollLockMask, w, False, GrabModeAsync, GrabModeAsync);
    XGrabKey(dpy, keycode, modifiers|NumLockMask|CapsLockMask, w, False, GrabModeAsync, GrabModeAsync);
    XGrabKey(dpy, keycode, modifiers|NumLockMask|ScrollLockMask, w, False, GrabModeAsync, GrabModeAsync);
    XGrabKey(dpy, keycode, modifiers|NumLockMask|CapsLockMask|ScrollLockMask, w, False, GrabModeAsync, GrabModeAsync);
  }
}

void create_new_key(char *key_string)
{
  Key *k;
  char *key_str, *command=NULL;

#ifdef DEBUG
  printf("create_new_key\n");
#endif

  key_str=strtok(key_string, ":");
  if(key_str!=NULL)
    command=strtok(NULL, "\n");
  
  if(key_str==NULL || command==NULL) return;

  if(strlen(key_str)<4) return;
  
  if((k=malloc(sizeof *k))==NULL)
    return;
  k->next=key;
  key=k;

  k->modifier=0;
  k->keycode=0;

  if(key_str[0]=='*') k->modifier=k->modifier|ShiftMask;
  if(key_str[1]=='*') k->modifier=k->modifier|ControlMask;
  if(key_str[2]=='*') k->modifier=k->modifier|Mod1Mask;

  k->keycode=XKeysymToKeycode(dpy, XStringToKeysym(key_str+3));

  grab_key(k->keycode, k->modifier, root);
  k->command=strdup(command);
  return;
}

void free_keys()
{
  Key *k;

#ifdef DEBUG
  printf("free_keys\n");
#endif

  XUngrabKey(dpy, AnyKey, AnyModifier, root);
  while(key)
  {
    free(key->command);
    k=key->next;
    free(key);
    key=k;
  }
}

/*
 *
 * Rc file functions
 *
 */

void parse_rc(char *dir, char *file)
{
  FILE *rc;
  char *rc_file, buf[1024], *lvalue, *rvalue;

#ifdef DEBUG
  printf("parse_rc\n");
#endif

  if((rc_file=malloc(strlen(dir)+strlen(file)+2))==NULL)
    return;
 
  sprintf(rc_file, "%s/%s", dir, file); 

  if((rc=fopen(rc_file, "r")))
  {
    while(fgets(buf, sizeof buf, rc))
    {
      lvalue=strtok(buf, "=");
      if(lvalue)
      {
        if(!strcmp(lvalue, "key"))
        {
          rvalue=strtok(NULL, "\n");
          if(rvalue)
            create_new_key(rvalue);
        }
      }
    }
  }
  last_update=get_last_update(getenv("HOME"), RCFILE);
}

/*
 *
 * Execute command
 *
 */

void fork_exec(char *cmd)
{
  pid_t pid=fork();

#ifdef DEBUG
  printf("fork_exec\n");
  printf("  executing %s\n", cmd);
#endif

  switch(pid)
  {
  case 0:
    execlp("/bin/sh", "sh", "-c", cmd, NULL);
    fprintf(stderr, "Exec failed.\n");
    exit(0);
    break;
  case -1:
    fprintf(stderr, "Fork failed.\n");
    break;
  }
}

/*
 *
 * Initialize and quit functions
 *
 */

void quit()
{
#ifdef DEBUG
  printf("quit\n");
#endif

  free_keys();
  XCloseDisplay(dpy);
  exit(0);
}

void signal_handler(int signal)
{
#ifdef DEBUG
  printf("signal_handler\n");
#endif

  switch(signal)
  {
  case SIGINT:
  case SIGTERM:
  case SIGHUP:
    quit();
    break;
  case SIGCHLD:
    wait(NULL);
    break;
  }
}

void initialize()
{
  struct sigaction act;
  
  act.sa_handler=signal_handler;
  act.sa_flags=0;
  sigaction(SIGTERM, &act, NULL);
  sigaction(SIGINT, &act, NULL);
  sigaction(SIGHUP, &act, NULL);
  sigaction(SIGCHLD, &act, NULL);
  
  if(!(dpy=XOpenDisplay(NULL)))
    print_error("Can't open display, X may not be running.\n", True);

  root=XDefaultRootWindow(dpy);
  
  init_keyboard();  
  parse_rc(getenv("HOME"), RCFILE);
}

/*
 *
 * Main
 *
 */

int main(int argc, char *argv[])
{
  initialize();

  while(1)
  {
    while(XPending(dpy))
    {
      XEvent ev;
      Key *k;

      XNextEvent(dpy, &ev);
      if(ev.type==KeyPress)
      {
        ev.xkey.state=ev.xkey.state&(Mod1Mask|ControlMask|ShiftMask);
        for(k=key;k!=NULL;k=k->next)
          if(k->keycode==ev.xkey.keycode && k->modifier==ev.xkey.state)
            fork_exec(k->command);
      }
    }
    if(get_last_update(getenv("HOME"), RCFILE)!=last_update)
    {
      free_keys();
      parse_rc(getenv("HOME"), RCFILE);
    }
    usleep(1);
  }
  return 0;
}
