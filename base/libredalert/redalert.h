/*
  libredalert - a middle-layer between atd and apps
  Copyright (C) 2002  Robert Mibus <mibus@handhelds.org>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

/* 'redalert' is a temporary name.
   (unless no-one can come up with a better one.
   other possibilities discussed on IRC:
   mibus: libnoisy? libbeeper? libillgetrightbacktoyou? libalarmnotify? libalarm?
   PaxAnima: lib-wake-me-up-before-you-go-go, libbeep
*/

#include <time.h>
#include <glib.h>

void redalert_set_alarm (const char *program, guint alarm_id, time_t unixtime, const char *command);
void redalert_set_alarm_message (const char *program, guint alarm_id, time_t unixtime, const char *message);
