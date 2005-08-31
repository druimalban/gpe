/*
 * no_defs.c - Thread definitions for systems without thread support.
 *
 * Copyright (C) 2002  Southern Storm Software, Pty Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "thr_defs.h"
#ifdef HAVE_UNISTD_H
	#include <unistd.h>
#endif

#ifdef IL_NO_THREADS

#ifdef	__cplusplus
extern	"C" {
#endif

void _ILThreadInitSystem(ILThread *mainThread)
{
	mainThread->handle = 0;
	mainThread->identifier = 0;
}

int _ILThreadCreateSystem(ILThread *thread)
{
	return 0;
}

void _ILThreadSetPriority(ILThread *thread, int priority)
{
}

int _ILThreadGetPriority(ILThread *thread)
{
	return IL_TP_NORMAL;
}

int _ILCondVarTimedWait(_ILCondVar *cond, _ILCondMutex *mutex, ILUInt32 ms)
{
	/* On a system without threads, we wait for the timeout
	   to expire but otherwise ignore the request.  We have
	   to do this because there are no other threads in the
	   system that could possibly signal us */
#ifdef HAVE_USLEEP
	if(ms != IL_MAX_UINT32)
	{
		/* Sleep for the specified timeout */
		while(ms >= 100000)
		{
			usleep(100000000);
			ms -= 100000;
		}
		if(ms > 0)
		{
			usleep(ms * 1000);
		}
	}
	else
	{
		/* Sleep forever */
		for(;;)
		{
			usleep(100000000);
		}
	}
	return 0;
#else
	/* We don't know how to sleep on this platform, so report "interrupted" */
	return -1;
#endif
}

#ifdef	__cplusplus
};
#endif

#endif	/* IL_NO_THREADS */
