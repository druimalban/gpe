/* Preferences handling for GPE
 *
 * Copyright (C) 2003 Luc Pionchon
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 */

//Test program

#include <glib.h>
//#include "gpe/preferences.h"
#include "preferences.h"
int main(int argc, char **argv){

  gpe_prefs_init("test-prefs");

  {
    gint    pref_int    = 0;
    gfloat  pref_float  = 0.0;
    gchar * pref_string = NULL;

    //retrieve saved perfs
    gpe_prefs_get("p-int",   G_TYPE_INT,    &pref_int);
    gpe_prefs_get("p-float", G_TYPE_FLOAT,  &pref_float);
    gpe_prefs_get("p-string",G_TYPE_STRING, &pref_string);

    /**/g_print("get: p-int    < %d\n",   pref_int);
    /**/g_print("get: p-float  < %g\n",   pref_float);
    /**/g_print("get: p-string < '%s'\n", pref_string);

    //change the prefs
    pref_int++;
    pref_float += 1.1;
    if(pref_string) g_free(pref_string);
    pref_string = g_strdup_printf("int: %d float: %g", pref_int, pref_float);

    gpe_prefs_set("p-int",   G_TYPE_INT,    &pref_int);
    gpe_prefs_set("p-float", G_TYPE_FLOAT,  &pref_float);
    gpe_prefs_set("p-string",G_TYPE_STRING, &pref_string);

    /**/g_print("set: p-int    > %d\n",   pref_int);
    /**/g_print("set: p-float  > %g\n",   pref_float);
    /**/g_print("set: p-string > '%s'\n", pref_string);
  }

  gpe_prefs_exit();

  return 0;
}
