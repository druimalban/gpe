#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <pwd.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <gtk/gtk.h>
#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE /* Pour GlibC2 */
#endif
#include <time.h>
#include "applets.h"
#include "unimplemented.h"



GtkWidget *Unimplemented_Build_Objects()
{  
  return gtk_label_new("This applet is not yet implemented.\nPlease wait for the next update.\n");;

}
void Unimplemented_Free_Objects()
{
}

void Unimplemented_Save()
{
}
void Unimplemented_Restore()
{

}
