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
#include "network.h"
#include "windows.h"
#include "channels.h"
#include "queries.h"
#include "misc.h"
#include "servers.h"
#include "ctcp.h"
#include "dialogs.h"
#include "parse_msg.h"
#include "prefs.h"
#include "dns.h"

#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include <string.h>

unsigned long Our_IP = 0;	/* Our own IP address */

gchar net_tmp[4096];

/* ----- Server Input/Output ---------------------------------------------------------- */

gboolean Server_Output(Server *s, gchar *text, gboolean warn_if_fail)
{
	/* Send a message to a server */

	int res;

	if (s->State != SERVER_CONNECTED && s->State != SERVER_DISCONNECTING)
	{
		if (warn_if_fail) VW_output(s->vw_active, T_WARNING, "sF-", s, "%s", "Server not connected.");
		return FALSE;
	}

	if (s->fd==-1 || s->fd==0)
	{
		g_warning("No socket for server %s ?!?",s->fs->Name);
		return FALSE;
	}

	sprintf(net_tmp, "%s\r\n", text);
	res = send(s->fd, net_tmp, strlen(net_tmp), 0);

	if (res==-1)
	{
		sprintf(net_tmp,"write() failed (%s).", g_strerror(errno));
		VW_output(s->vw, T_ERROR, "sF-", s, "%s", net_tmp);
		Server_Disconnect(s, TRUE);
		return FALSE;
	}
	else if (s->Raw_Window) Server_Raw_Window_Output(s, net_tmp, TRUE);

	return TRUE;
}

void Server_Input(gpointer data, gint source, GdkInputCondition cond)
{
	/* Receive bytes from a server */

	int res;

	Server *s = (Server *) data;

	if (cond & GDK_INPUT_WRITE)
	{
		int option, size = sizeof(option);

		gdk_input_remove(s->gdk_tag);

		/* Check if the socket is correctly opened */

		res = getsockopt(s->fd, SOL_SOCKET, SO_ERROR, (void *) &option, &size);

		if (res == -1 || option)
		{
			if (s->State != SERVER_DISCONNECTING)
			{
				sprintf(net_tmp,"Unable to connect to the %sserver (%s).", (s->Flags & SERVER_SOCKS)? "SOCKS " : "", g_strerror(option));
				VW_output(s->vw, T_ERROR, "sF-", s, "%s", net_tmp);
			}
			Server_Disconnect(s, TRUE);
			return;
		}

		/* Now, we watch only the read and error flags of the socket */

		s->gdk_tag = gdk_input_add(s->fd, GDK_INPUT_READ | GDK_INPUT_EXCEPTION, Server_Input, (gpointer) s);

		if (s->Flags & SERVER_SOCKS)
		{
			gchar buf[128];
			guint32 ip;
			guint16 port;

			VW_output(s->vw, T_NORMAL, "sF-", s, "%s", "Sending Socks request...");

			ip = htonl(s->IP);
			port = Server_Get_Port(s);

			buf[0] = 4;		/* Version Number = Socks 4 */
			buf[1] = 1;		/* Command = Connect */
			memcpy(buf+2, &port, 2);
			memcpy(buf+4, &ip, 4);
			sprintf(buf+8, GPrefs.Socks_User);

			if (write(s->fd, buf, 9+strlen(GPrefs.Socks_User)) == -1)
			{
				sprintf(net_tmp,"write() failed when sending Socks request (%s).", g_strerror(errno));
				VW_output(s->vw, T_ERROR, "sF-", s, "%s", net_tmp);
				Server_Disconnect(s, TRUE);
				return;
			}
		}
		else
		{
			/* We are now really connected to the server */

			s->State = SERVER_CONNECTED; VW_Status(s->vw);
			VW_output(s->vw, T_NORMAL, "sF-", s, "%s", "Connected to the server.\n");

			/* Send login message */

			if (*(s->fs->Password)) sprintf(net_tmp, "PASS %s\r\nNICK %s\r\nUSER %s - - :%s", s->fs->Password, s->fs->Nick, s->fs->User, s->fs->Real);
			else sprintf(net_tmp, "NICK %s\r\nUSER %s - - :%s", s->fs->Nick, s->fs->User, s->fs->Real);
			Server_Output(s, net_tmp, FALSE);
		}
	}

	if (cond & GDK_INPUT_READ)
	{
		if (s->Flags & SERVER_SOCKS)
		{
			gchar buf[128];
			gint res;

			res = read(s->fd, buf, 8);
		
			if (res>0)
			{
				s->Flags &= ~SERVER_SOCKS;

				if (buf[1] != 90)
				{
					switch (buf[1])
					{
						case 91: sprintf(net_tmp, "SOCKS request rejected or failed."); break;
						case 92: sprintf(net_tmp, "SOCKS request rejected because the server was unable to connect to our identd"); break;
						case 93: sprintf(net_tmp, "SOCKS request rejected : client user-id was different from identd user-id"); break;
						default: sprintf(net_tmp, "SOCKS request failed - Unknown answer %d", (unsigned char) buf[1]);
					}

					VW_output(s->vw, T_ERROR, "sF-", s, "%s", net_tmp);
					Server_Disconnect(s, TRUE);
					return;
				}
			
				/* We are now really connected to the server */

				s->State = SERVER_CONNECTED; VW_Status(s->vw);
				VW_output(s->vw, T_NORMAL, "sF-", s, "%s", "Connected to the server.\n");

				/* Send login message */

				sprintf(net_tmp,"NICK %s\nUSER %s - - :%s\n", s->fs->Nick, s->fs->User, s->fs->Real);
				/* FIXME Send password if there is one !! Anyway, I have to rewrite all the SOCKS stuff ... */
				Server_Output(s,net_tmp, FALSE);
			}
			else if (res==0)
			{
				VW_output(s->vw, T_ERROR, "sF-", s, "%s", "Connection to the Socks server lost.");
				Server_Disconnect(s, TRUE);
				return;
			}
			else if (errno == EAGAIN)
			{
				/* fprintf(stdout, "--- AGAIN ---\n"); */
				return;
			}
			else
			{
				sprintf(net_tmp,"Unable to connect to the Socks server (%s).", g_strerror(errno));
				VW_output(s->vw, T_ERROR, "sF-", s, "%s", net_tmp);
				Server_Disconnect(s, TRUE);
				return;
			}
		}
		else if (Socket_Read_Messages(s->fd, s->Input_Buffer, &s->offset, SERVER_INPUT_BUFFER_SIZE, s->vw, (void *) Message_Parse, s))
		{
			Server_Disconnect(s, TRUE);
			return;
		}
	}

	if (cond & GDK_INPUT_EXCEPTION)
	{
		Server_Disconnect(s, TRUE);
		return;
	}
}

void server_connect_dns_callback(struct olirc_hostip *oh, gpointer data)
{
	Server *s = (Server *) data;

	if (oh->h_error || oh->s_error)
	{
		sprintf(net_tmp, "Unable to resolve %s (%s).", s->fs->Name, dns_error(oh));
		VW_output(s->vw, T_ERROR, "sF-", s, "%s", net_tmp);
		Server_Disconnect(s, TRUE);
		return;
	}

	s->State = SERVER_CONNECTING; VW_Status(s->vw); /* FIXME */ /* Sync(); */

	if (s->Flags & SERVER_SOCKS)
	{
		VW_output(s->vw, T_NORMAL, "sF-", s, "%s", "Connecting to the Socks server...");
		if (Socket_Connect(&(s->fd), oh->ip, GPrefs.Socks_Port, Server_Input, &(s->gdk_tag), (gpointer) s, s->vw) < 0)
			Server_Disconnect(s, TRUE);
	}
	else
	{
		gint p = Server_Get_Port(s);
		s->IP = oh->ip;
		if (!s->IP) { Server_Disconnect(s, TRUE); return; }
		sprintf(net_tmp, "%s has address %s", s->fs->Name, Dump_IP(s->IP));
		VW_output(s->vw, T_NORMAL, "sF-", s, "%s", net_tmp);
		sprintf(net_tmp, "Connecting to port %d...", p);
		VW_output(s->vw, T_NORMAL, "sF-", s, "%s", net_tmp);
		if (Socket_Connect(&(s->fd), s->IP, p, Server_Input, &(s->gdk_tag), (gpointer) s, s->vw) < 0)
			Server_Disconnect(s, TRUE);
	}
}

void Server_Connect(Server *s)
{
	if (s->State != SERVER_IDLE)
	{
		g_warning("Server_Connect(%s): Attempt to connect a non-idle server !?",s->fs->Name);
		return;
	}

	if (!(s->Input_Buffer)) s->Input_Buffer = (gchar *) g_malloc0(SERVER_INPUT_BUFFER_SIZE);
	if (!(s->Output_Buffer)) s->Output_Buffer = (gchar *) g_malloc0(SERVER_OUTPUT_BUFFER_SIZE);

	s->State = SERVER_RESOLVING; VW_Status(s->vw); /* FIXME */ /* Sync(); */

	if (GPrefs.Flags & PREF_USE_SOCKS)
	{
		s->Flags |= SERVER_SOCKS;
		VW_output(s->vw, T_NORMAL, "sF-", s, "%s", "Resolving SOCKS server...");
		/* Should test is GPrefs.Socks_Host is not an IP (i.e. a string like "10.20.30.40") */
		dns_resolve(0, GPrefs.Socks_Host, server_connect_dns_callback, (gpointer) s);
	}
	else
	{
		VW_output(s->vw, T_NORMAL, "sF-", s, "%s", "Resolving server...");
		/* Should test is s->fs->Name is not an IP (i.e. a string like "10.20.30.40") */
		dns_resolve(0, s->fs->Name, server_connect_dns_callback, (gpointer) s);
	}

}

/* ----- Network stuff ---------------------------------------------------------------- */

gint Socket_Connect(int *fd, unsigned long adr, int port, void callback(), gint *tag, gpointer data, Virtual_Window *vw)
{
	/* Create a socket and try to connect it to adr:port */

	int sd, option = 1, res;
	struct sockaddr_in my_addr;
	gint err;

	#ifdef DEBUG_NETWORK
	fprintf(stdout, "Socket_Connect(%s:%d, %p, %p)\n", Dump_IP(adr), port, callback, data);
	#endif

	sd = socket(AF_INET, SOCK_STREAM, 0);

	if (sd==-1)
	{
		err = errno;
		sprintf(net_tmp, "Unable to create a socket (%s).",g_strerror(err));
		VW_output(vw, T_ERROR, "F-", "%s", net_tmp);
		return -(err);
	}

	setsockopt(sd, SOL_SOCKET, SO_KEEPALIVE, (void *) &option, sizeof(option));
	setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, (void *) &option, sizeof(option));

	fcntl(sd, F_SETFL, O_NONBLOCK);	/* Set the file descriptor non blocking */

	my_addr.sin_family = AF_INET;
	my_addr.sin_addr.s_addr = htonl(adr);
	my_addr.sin_port = htons(port);

	res = connect(sd, (struct sockaddr *) &my_addr, sizeof(struct sockaddr_in));

	if (res==-1 && errno!=EINPROGRESS)
	{
		err = errno;
		sprintf(net_tmp,"Unable to connect (%s).",g_strerror(err));
		VW_output(vw, T_ERROR, "F-", "%s", net_tmp);
		close(sd);
		return -(err);
	}

	*fd = sd;
	*tag = gdk_input_add(sd, GDK_INPUT_READ | GDK_INPUT_WRITE | GDK_INPUT_EXCEPTION, callback, data);

	if (!Our_IP)	/* If we don't know our own IP yet, we get it now */
	{
		int len = sizeof(struct sockaddr_in);
		if (getsockname(sd, (struct sockaddr *) &my_addr, &len) == -1)
		{
			sprintf(net_tmp, "Unable to get our own IP (%s).", g_strerror(errno));
			VW_output(vw, T_WARNING, "F-", "%s", net_tmp);
		}
		else Our_IP = ntohl(my_addr.sin_addr.s_addr);
	}

	return 0;
}

gint Socket_Listen(int *fd, void callback(), gint *tag, gpointer data, Virtual_Window *vw)
{
	/* Create a socket, then bind() and listen() it */

	int sd, option = 1;
	unsigned int l=sizeof(struct sockaddr_in);
	struct sockaddr_in my_addr;
	gint err;

	#ifdef DEBUG_NETWORK
	fprintf(stdout, "Socket_Listen(%p, %p, %s)\n", callback, data, vw->Title);
	#endif

	sd = socket(AF_INET, SOCK_STREAM, 0);

	if (sd==-1)
	{
		err = errno;
		sprintf(net_tmp,"Unable to create a socket (%s).",g_strerror(err));
		VW_output(vw, T_ERROR, "F-", "%s", net_tmp);
		return -(err);
	}

	setsockopt(sd, SOL_SOCKET, SO_KEEPALIVE, (void *) &option, sizeof(option));
	setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, (void *) &option, sizeof(option));

	fcntl(sd, F_SETFL, O_NONBLOCK);	/* Set the file descriptor non blocking */

	my_addr.sin_family = AF_INET;
	my_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	my_addr.sin_port = 0;

	/* Bind() the socket */

	if (bind(sd,(struct sockaddr *) &my_addr, l)==-1)
	{
		err = errno;
		sprintf(net_tmp,"Unable to bind() the socket (%s).",g_strerror(err));
		VW_output(vw, T_ERROR, "F-", "%s", net_tmp);
		close(sd);
		return -(err);
	}

	/* Listen() the socket */

	if (listen(sd,1)==-1)
	{
		err = errno;
		sprintf(net_tmp,"Unable to listen() the socket (%s).",g_strerror(err));
		VW_output(vw, T_ERROR, "F-", "%s", net_tmp);
		close(sd);
		return -(err);
	}

	/* Get the port of our socket */

	option = sizeof(struct sockaddr_in);

	if (getsockname(sd, (struct sockaddr *) &my_addr, &option) == -1)
	{
		err = errno;
		sprintf(net_tmp,"Unable to get the port of our socket: getsockname() failed (%s).",g_strerror(err));
		VW_output(vw, T_WARNING, "F-", "%s", net_tmp);
		close(sd);
		return -(err);
	}

	*fd = sd;
	*tag = gdk_input_add(sd, GDK_INPUT_READ | GDK_INPUT_EXCEPTION, callback, data);

	return ntohs(my_addr.sin_port);
}

gint Socket_Read_Bytes(int socket, gchar *buffer, gint *offset, gint bsize)
{
	/* Read bytes on a socket and copy them into the buffer.
		Does nothing if the buffer is full.
	   Returns the number of bytes read or -(errno) if an error occured
	*/

	gint res;

	g_return_val_if_fail(socket > 0, -EBADF);

	#ifdef DEBUG_NETWORK
	fprintf(stdout, "Socket_Read_Bytes(%d, %p, %d, %d)", socket, buffer, *offset, bsize);
	fflush(stdout);
	#endif

	/* FIXME Doing an fcntl() on the socket before each read() is ugly. 
		But it is the only way fcntl() seems to work (why ?) */

	if (!(fcntl(socket, F_GETFL) & O_NONBLOCK)) fcntl(socket, F_SETFL, O_NONBLOCK);

	res = read(socket, buffer + *offset, bsize - *offset);

	if (res == -1)
	{
		#ifdef DEBUG_NETWORK
		fprintf(stdout, " -> error %d\n", errno);
		#endif
		return -(errno);
	}

	*offset += res;

	#ifdef DEBUG_NETWORK
	fprintf(stdout, " = %d\n", res);
	#endif

	return res;
}

gint Socket_Read_Messages(int socket, gchar *buffer, gint *offset, gint bsize, Virtual_Window *vw, void (*parse_function)(gpointer, gchar *), gpointer data)
{
	/* Read bytes on a socket, split them into messages,
		and call a parser function for each message found.
	*/

	gint res;
	gchar *msg_end;
	gchar *buf_end;
	gint msg_size;

	#ifdef DEBUG_NETWORK
	fprintf(stdout, "Socket_Read_Messages(%d, %p, %d, %d, %s)\n", socket, buffer, *offset, bsize, vw->Title);
	#endif

	res = Socket_Read_Bytes(socket, buffer, offset, bsize);

	if (res == -EWOULDBLOCK) return 0;
	if (!res) return -1;
	if (res < 0)
	{
		sprintf(net_tmp, "Read error on socket (%s)", g_strerror(-res));
		VW_output(vw, T_ERROR, "F-", "%s", net_tmp);
		return -1;
	}

	/* Translate incoming bytes into messages */

	for(;;)
	{
		buf_end = buffer + *offset;

		msg_end = (gchar *) memchr(buffer, '\n', *offset);	/* Search for the end of a message */

		if (!msg_end) break; /* If there are no complete messages in the buffer, keep on waiting */

		msg_end++;

		msg_size = (gint) ((gint) msg_end) - ((gint) buffer);

		memset(net_tmp, 0, sizeof(net_tmp));

		memcpy(net_tmp, buffer, msg_size);

		memmove(buffer, msg_end, *offset - msg_size);

		*offset -= msg_size;

		parse_function(data, net_tmp);
	}

	if (*offset >= bsize)	/* ?? TODO Should check this ... */
	{
		g_warning("Reveive buffer overflow");
		VW_output(vw, T_ERROR, "F-", "%s", "*** INTERNAL ERROR : Reveive buffer overflow");
		return -1;
	}

	return 0;
}

/* vi: set ts=3: */

