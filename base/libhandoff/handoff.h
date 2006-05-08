/* handoff.h - Handoff Interface.
   Copyright (C) 2006 Neal H. Walfield <neal@walfield.org>

   This file is part of GPE.

   GPE is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   GPE is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
   License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111, USA. */

#ifndef HANDOFF_H
#define HANDOFF_H

#include <glib-object.h>

G_BEGIN_DECLS

#define TYPE_HANDOFF (handoff_get_type ())
#define HANDOFF(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_HANDOFF, Handoff))
#define HANDOFF_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), TYPE_HANDOFF, HandoffClass))
#define IS_HANDOFF(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_HANDOFF))
#define IS_HANDOFF_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), TYPE_HANDOFF))
#define HANDOFF_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_HANDOFF, HandoffClass))

struct _Handoff;
typedef struct _Handoff Handoff;

struct _HandoffClass;
typedef struct _HandoffClass HandoffClass;

extern GType handoff_get_type (void) G_GNUC_CONST;

/* The "handoff" signal is triggered when another instance successful
   negotiates a handoff to this instance.  DATA is the data sent by
   other instance, normally a serialization of state this instance
   should use.  */
typedef void (*HandoffCallback) (Handoff *handoff, char *data);

/* Create a new passive handoff.  Before calling handoff_handoff, the
   caller should connect a handler to the "handoff" signal.  */
extern Handoff *handoff_new (void);

/* Try to handoff this instance to an already running process.

   RENDEZVOUS is the address which all instances of this program
   rendezvous with.

   If another instance of the program is contacted then: if
   DISPLAY_RELEVANT is true, the value of the environment variable
   DISPLAY is sent; and if DATA is non-NULL, its value is sent.  If
   the handoff request (DISPLAY is either not sent or they are
   identical), this function returns TRUE and this instance should
   proceed to exit.  Otherwise, if the other instance punts, it sends
   a serialization of its state and the "handoff" signal is fired and
   TRUE is returned.

   If no running instance is found to be listening on RENDEZVOUS or
   the instance punts, this instance attempts to bind to RENDEZVOUS so
   that subsequent instances can find this instance and FALSE is
   returned.

   After connecting, this instance waits on RENDEZVOUS for other
   instances to connect.  When an instance connects and hands off its
   state, the "handoff" signal is fired.  If this instance decides to
   yield (based on the above criteria), then SERIALIZE is called to
   serialize any state.  Once the handoff completes, EXIT is called
   and this instance should proceed to do any final clean up and
   exit.  */
extern gboolean handoff_handoff (Handoff *handoff, const char *rendezvous,
				 const char *data,
				 gboolean display_relevant,
				 char *(*serialize) (Handoff *),
				 void (*exit)(Handoff *));

G_END_DECLS

#endif /* HANDOFF_H  */
