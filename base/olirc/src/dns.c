/*
 * ol-irc - A small irc client using GTK+
 *
 * Copyright (C) 1998, 1999 Yann Grossel [Olrick]
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */

#include "olirc.h"
#include "dns.h"

#include <unistd.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <errno.h>
#include <netinet/in.h>
#include <signal.h>
#include <string.h>

#include <arpa/inet.h>

/* Asynchronous DNS (using fork()ed child processes) */

struct dns_tmp
{
	struct olirc_hostip oh;
	gint input_tag;
	gint timer_tag;
	gpointer data;
	gint fd;
	DNS_Callback(callback);
	gchar buffer[1024];
	guint offset;
	pid_t child;
};

/* -------------------------------------------------------------------------------------------------- */

gchar *dns_error(struct olirc_hostip *oh)
{
	g_return_val_if_fail(oh, "Internal error");

	if (oh->s_error) return g_strerror(oh->v_errno);

	if (oh->h_error)
	{
		/* the function hstrerror() exists and works in Linux, but is not in the man ! */

		switch (oh->v_errno)
		{
			case HOST_NOT_FOUND: return "Unknown host";
			case TRY_AGAIN: return "Host name lookup failure";
			case NO_RECOVERY: return "Unknown server error";
			case NO_DATA: return "No address associated with name";
			default: return "Unknown resolver error";
		}
	}

	return "No error";
}

/* ----- Timeout function --------------------------------------------------------------------------- */

gboolean dns_timeout(gpointer data)
{
	struct dns_tmp *dt = (struct dns_tmp *) data;
	gint state;

	gtk_timeout_remove(dt->timer_tag);
	gdk_input_remove(dt->input_tag);

	dt->oh.s_error = TRUE;
	dt->oh.v_errno = ETIMEDOUT;
	dt->callback(&(dt->oh), dt->data);

	close(dt->fd);

	kill(dt->child, 9);
	waitpid(dt->child, &state, 0);

	g_free(dt);

	return FALSE;
}

/* ----- Resolver ---------------------------------------------------------------------- */

void dns_resolve_callback(gpointer data, gint source, GdkInputCondition cond)
{
	struct dns_tmp *dt = (struct dns_tmp *) data;
	gint r = 0;

	if (cond & GDK_INPUT_READ)
	{
		r = read(dt->fd, dt->buffer + dt->offset, sizeof(dt->buffer) - dt->offset);
		dt->offset += r;
	}

	if (r>0 && !(cond & GDK_INPUT_EXCEPTION)) return;

	gtk_timeout_remove(dt->timer_tag);
	gdk_input_remove(dt->input_tag);
	close(dt->fd);

	if (r == -1)
	{
		dt->oh.s_error = TRUE;
		dt->oh.v_errno = errno;
	}
	else
	{
		gpointer rp = (gpointer) dt->buffer;

		if (dt->offset < 2 * sizeof(gint))
		{
			dt->oh.s_error = TRUE;
			dt->oh.v_errno = EIO;
		}
		else
		{
			gint h_error, s_error;

			h_error = *((unsigned long int *) rp);
			dt->offset -= sizeof(gint); rp += sizeof(gint);
			s_error = *((unsigned long int *) rp);
			dt->offset -= sizeof(gint); rp += sizeof(gint);

			if (h_error || s_error)
			{
				if (h_error == -1)
				{
					dt->oh.s_error = TRUE;
					dt->oh.v_errno = s_error;
				}
				else
				{
					dt->oh.h_error = TRUE;
					dt->oh.v_errno = h_error;
				}
			}
			else if (dt->offset < sizeof(unsigned long int))
			{
				dt->oh.s_error = TRUE;
				dt->oh.v_errno = EIO;
			}
			else
			{
				dt->oh.ip = *((unsigned long int *) rp);
				dt->offset -= sizeof(unsigned long int);
				rp += sizeof(unsigned long int);
				dt->oh.host = rp;
				*((gchar *) rp + dt->offset) = 0;
			}
		}
	}

	dt->callback(&(dt->oh), dt->data);

	waitpid(dt->child, &r, 0);
	g_free(dt);
}

/*	Format of the stream of chars sent on the pipe :
 *
 *	gint h_error;	0 if no error
 *	gint s_error;	0 if no error
 * unsigned long ip;
 * string of chars : host name resolved, ended by \0
 *
 */

/* For dns_resolve(), ip must be in host byte order !! */

void dns_resolve(unsigned long int ip, gchar *name, DNS_Callback(callback), gpointer data)
{
	struct dns_tmp *dt;
	pid_t pid;
	int fd[2];

	g_return_if_fail(callback);

	dt = (struct dns_tmp *) g_malloc0(sizeof(struct dns_tmp));

	dt->data = data;
	dt->callback = callback;

	if (pipe(fd) == -1) /* Unable to create the pipe */
	{
		dt->oh.s_error = TRUE;
		dt->oh.v_errno = errno;
		callback(&(dt->oh), data);
		g_free(dt);
		return;
	}

	pid = fork();

	if (pid == -1) /* Unable to fork */
	{
		dt->oh.s_error = TRUE;
		dt->oh.v_errno = errno;
		callback(&(dt->oh), dt->data);
		g_free(dt);
		return;
	}
	else if (pid == 0) /* We are the child */
	{
		struct hostent *h = NULL;
		gint ret;

		close(fd[0]);

		if (name)	/* We have to resolve a name to an IP */
		{
			h = gethostbyname(name);

			if (h)
			{
				unsigned long int adr = ntohl(*(unsigned long int *) h->h_addr_list[0]);
				ret = 0;
				write(fd[1], &ret, sizeof(gint));
				write(fd[1], &ret, sizeof(gint));
				write(fd[1], &adr, sizeof(unsigned long int));
				write(fd[1], name, strlen(name));
			}
		}
		else	/* We have to resolve an IP to a name */
		{
			ip = htonl(ip);
			h = gethostbyaddr((const char *) &ip, sizeof(unsigned long int), AF_INET);

			if (h)
			{
				ret = 0;
				write(fd[1], &ret, sizeof(gint));
				write(fd[1], &ret, sizeof(gint));
				write(fd[1], &ip, sizeof(unsigned long int));
				write(fd[1], h->h_name, strlen(h->h_name));
			}
		}

		if (!h)	/* If unresolved */
		{
			ret = h_errno;
			write(fd[1], &ret, sizeof(gint));
			ret = errno;
			write(fd[1], &ret, sizeof(gint));
		}

		close(fd[1]);

		/* exit(0) abort()s the father (gtk error). Why ? Does g_atexit() has something
		 * to do with such a behaviour ?
		 * Then for now we terminate the child with a SIGKILL ... awful but it works
		 * Is the raise() function available on all unixes ? */

		kill(getpid(), SIGKILL);
	}
	else /* We are the parent */
	{
		close(fd[1]);
		dt->fd = fd[0];
		dt->child = pid;
		dt->input_tag = gdk_input_add(fd[0], GDK_INPUT_READ | GDK_INPUT_EXCEPTION, dns_resolve_callback, (gpointer) dt);
		dt->timer_tag = gtk_timeout_add(DNS_TIMEOUT * 1000, (GtkFunction) dns_timeout, (gpointer) dt);
	}
}

/* vi: set ts=3: */

