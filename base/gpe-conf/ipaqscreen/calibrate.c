/*
 * gpe-conf
 *
 * Copyright (C) 2002 Moray Allan <moray@sermisy.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <libintl.h>
#include "gpe/errorbox.h"

#include "calibrate.h"
#include "../suid.h"
#include "../applets.h"
#ifndef _
# define _(x) gettext(x)
#endif

void calibrate ()
{

#ifdef __i386__
	gpe_error_box ("you are on 386 machine!!");
	return ; // xrandr doesnt exit on i386 dev machines!
#endif
	if (suid_exec("XCAL",""))
	  gpe_error_box(_("Sorry, wrong password."));

}
