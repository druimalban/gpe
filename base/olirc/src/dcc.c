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
#include "windows.h"
#include "prefs.h"
#include "histories.h"
#include "misc.h"
#include "network.h"
#include "ctcp.h"
#include "dcc.h"
#include "dialogs.h"

#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <string.h>

GList *DCC_List = NULL;		/* List of current DCC connections */

static gchar *DCC_Types[] = { "Chat", "Send", "Get" };
static gchar *DCC_Status[] = { "Listening", "Connecting", "Active", "Failed", "Finished", "Aborted" };

gchar dcc_tmp[1024];

static guint32 dc_id = 0;

/* ----- DCC connections -------------------------------------------------------------- */

void DCC_Disconnect(DCC *d, gint State)
{
	g_return_if_fail(d);

	#ifdef DEBUG_DCC
	fprintf(stdout, "DCC_Disconnect(%p, %s)\n", d, DCC_Status[State]);
	#endif

	if (d->gdk_tag) { gdk_input_remove(d->gdk_tag); d->gdk_tag = 0; }
	if (d->fd_socket!=-1) { close(d->fd_socket); d->fd_socket = -1; }
	if (d->fd_file!=-1) { close(d->fd_file); d->fd_file = -1; }

	d->State = State;
	d->Distant_IP = 0;
	d->Distant_Port = 0;
	d->End_time = time(NULL);

	if (d->time_label)
	{
		DCC_Box_Update(d);
		sprintf(dcc_tmp, "%s.", DCC_Status[State]);
		gtk_label_set(d->status_label, dcc_tmp);
	}

	if (d->vw) VW_Status(d->vw);
}

void DCC_Send_Bytes(DCC *d)
{
	gchar buf[4096];
	gint res;
	gchar *merr = "DCC Send error";
	gint err;

	res = read(d->fd_file, buf, 2048);

	if (res==-1)
	{
		err = errno;
		DCC_Disconnect(d, DCC_FAILED);
		sprintf(dcc_tmp, "Error while reading file %s\n(%s)", d->file, g_strerror(err));
		Message_Box(merr, dcc_tmp);
		return;
	}

	res = write(d->fd_socket, buf, res);

	if (res==-1)
	{
		err = errno;
		DCC_Disconnect(d, DCC_FAILED);
		sprintf(dcc_tmp, "Error while sending file %s\n(%s)", d->file, g_strerror(err));
		Message_Box(merr, dcc_tmp);
		return;
	}

	d->completed += res;

	/* FIXME if d->size is null -> floating point crash */
	gtk_progress_bar_update(d->bar, (gfloat) d->completed/d->size);
}

void DCC_Display_Message(DCC *d, gchar *msg)
{
	gchar *end;

	g_return_if_fail(d);
	g_return_if_fail(d->Type==DCC_CHAT);

	#ifdef DEBUG_DCC
	fprintf(stdout, "DCC_Display_Message(%p, %s)\n", d, msg);
	#endif

	end = msg + strlen(msg);

	/* We remove EOL chars */
	while (end > msg && (*(end-1) == '\n' || *(end-1) == '\r')) *--end = 0;

	if ((!strncmp(msg, "\001ACTION", 7)) && msg[strlen(msg)-1]=='\001')
	{
		msg += 8;
		msg[strlen(msg)-1] = 0;
		VW_output(d->vw, T_DCC_ACTION, "snudt", d->server, d->nick, d->user, d->my_nick, msg);
	}
	else VW_output(d->vw, T_DCC_MSG, "snudt", d->server, d->nick, d->user, d->my_nick, msg);
}

void DCC_Input(gpointer data, gint source, GdkInputCondition cond)
{
	guint32 pos;

	DCC *d = (DCC *) data;
	gint res;
	gint err;

	#ifdef DEBUG_DCC
	fprintf(stdout, "DCC_Input(%p, %d)\n", d, cond);
	#endif

	if (cond & GDK_INPUT_WRITE)
	{
		int option, size = sizeof(option);

		gdk_input_remove(d->gdk_tag); d->gdk_tag = 0;

		/* Check if the socket is correctly opened */

		res = getsockopt(d->fd_socket, SOL_SOCKET, SO_ERROR, (void *) &option, &size);

		if (res == -1 || option)
		{
			DCC_Disconnect(d, DCC_FAILED);

			if (d->Type == DCC_CHAT) VW_output(d->vw, T_ERROR, "sF---", d->server, "Unable to chat with %s!%s (%s).", d->nick, d->user, g_strerror(option));
			else 
			{
				sprintf(dcc_tmp, "Error while setting up the transfer of file\n%s to/from %s\n\n(%s)", d->file, d->nick, g_strerror(option));
				Message_Box("DCC Error", dcc_tmp);
			}
			return;
		}

		/* Now, we watch only the read and error flags of the socket */

		d->gdk_tag = gdk_input_add(d->fd_socket, GDK_INPUT_READ | GDK_INPUT_EXCEPTION, DCC_Input, data);

		/* We are now really connected to our partner */

		d->State = DCC_ACTIVE;
		d->Start_time = time(NULL);

		sprintf(dcc_tmp,"Connected to %s:%u", Dump_IP(d->Distant_IP), d->Distant_Port);

		if (d->Type==DCC_CHAT && d->server) VW_output(d->vw, T_NORMAL, "st", d->server, dcc_tmp);
		else gtk_label_set(d->status_label, dcc_tmp);

		if (d->vw) VW_Status(d->vw);
	}

	if (cond & GDK_INPUT_READ)
	{
		if (d->Type == DCC_CHAT)
		{
			if (Socket_Read_Messages(d->fd_socket, d->Buffer, &d->offset, sizeof(d->Buffer), d->vw, (void *) DCC_Display_Message, d))
			{
				VW_output(d->vw, T_ERROR, "st", d->server, "DCC connection closed.");
				DCC_Disconnect(d, DCC_FINISHED);
				return;
			}
			else if (d->vw) VW_Status(d->vw);
		}
		else for (;;)
		{
			res = Socket_Read_Bytes(d->fd_socket, d->Buffer, &d->offset, sizeof(d->Buffer));

			if (res == -EWOULDBLOCK) break;

			if (res < 0)
			{
				DCC_Disconnect(d, DCC_FAILED);
				/* fprintf(stderr, "Read error on socket (%s)", g_strerror(-res)); */
				return;
			}

			if (res == 0)
			{
				DCC_Disconnect(d, DCC_FAILED);
				/* fprintf(stderr, "Connection lost for file %s.", d->file); */
				return;
			}

			if (d->Type == DCC_SEND)
			{
				for (;;)
				{
					if (d->offset < 4) break;
					memcpy(&pos, d->Buffer, 4); pos = ntohl(pos);
					memmove(d->Buffer, d->Buffer + 4, d->offset - 4); d->offset -= 4;
					if (pos>=d->size) { DCC_Disconnect(d, DCC_FINISHED); return; }
					else if (pos==d->completed) DCC_Send_Bytes(d);
				}
			}
			else
			{
				res = write(d->fd_file, d->Buffer, d->offset);

				if (res == -1)
				{
					err = errno;
					DCC_Disconnect(d, DCC_FAILED);
					/* fprintf(stderr, "Write error on file %s (%s)\n", d->file, g_strerror(err)); */
					return;
				}

				d->completed += d->offset;
				d->offset = 0;

				/* FIXME if d->size is null -> floating point crash */
				gtk_progress_bar_update(d->bar, (gfloat) d->completed/d->size);

				pos = htonl(d->completed);
				res = write(d->fd_socket, &pos, 4);

				if (res == -1)
				{
					err = errno;
					DCC_Disconnect(d, DCC_FAILED);
					/* fprintf(stderr, "Write error on socket (%s)\n", g_strerror(err)); */
					return;
				}

				if (d->completed>=d->size) { DCC_Disconnect(d, DCC_FINISHED); return; }
			}
		}
	}

	if (cond & GDK_INPUT_EXCEPTION)
	{
		DCC_Disconnect(d, DCC_FINISHED);
		if (d->Type == DCC_CHAT) VW_output(d->vw, T_ERROR, "st", d->server, "DCC connection closed.");
	}
}

void DCC_Accept(gpointer data, gint source, GdkInputCondition cond)
{
	/* Someone is connected to our socket */

	DCC *d = (DCC *) data;
	gint sd;
	struct sockaddr_in client_addr;
	gint len = sizeof(struct sockaddr_in);
	gint err;

	#ifdef DEBUG_DCC
	fprintf(stdout, "DCC_Accept(%p, %d)\n", d, cond);
	#endif

	gdk_input_remove(d->gdk_tag); d->gdk_tag = 0;

	sd = accept(d->fd_socket, (struct sockaddr *) &client_addr, &len);

	if (sd==-1)
	{
		err = errno;

		DCC_Disconnect(d, DCC_FAILED);

		if (d->Type == DCC_CHAT)
		{
			VW_output(d->vw, T_ERROR, "sF-", d->server, "DCC Chat failed on accept() (%s).", g_strerror(err));
		}
		else
		{
			sprintf(dcc_tmp, "Error while setting up the transfer of file\n%s with %s\n(%s)", d->file, d->nick, g_strerror(err));
			Message_Box("DCC Error", dcc_tmp);
		}

		return;
	}
	
	d->Distant_IP = ntohl(client_addr.sin_addr.s_addr);
	d->Distant_Port = ntohs(client_addr.sin_port);
	d->State = DCC_ACTIVE;
	d->Start_time = time(NULL);

	close(d->fd_socket); d->fd_socket = sd;
	d->gdk_tag = gdk_input_add(sd, GDK_INPUT_READ | GDK_INPUT_EXCEPTION, DCC_Input, data);

	if (d->Type == DCC_CHAT)
	{
		VW_output(d->vw, T_NORMAL, "sF--", d->server, "DCC Chat connection established (connnected to %s:%u).", Dump_IP(d->Distant_IP), d->Distant_Port);
		if (d->vw) VW_Status(d->vw);
	}
	else
	{
		sprintf(dcc_tmp, "Connnected to %s:%u", Dump_IP(d->Distant_IP), d->Distant_Port);
		gtk_label_set(d->status_label, dcc_tmp);
		DCC_Send_Bytes(d);
	}
}

/* ----- DCC Send / Get --------------------------------------------------------------- */

void DCC_Box_destroy(GtkWidget *w, gpointer data)
{
	DCC *d = (DCC *) data;

	if (d->State != DCC_FAILED && d->State != DCC_FINISHED) DCC_Disconnect(d, DCC_FAILED);

	DCC_List = g_list_remove(DCC_List, d);

	g_free(d->nick);
	g_free(d->user);
	g_free(d->file);
	g_free(d);
}

gchar *DCC_Time_Format(time_t t, gchar *s)
{
	/* used for nice times display in by DCC_Box_Update, written by Khalimero, may 1, 1999 */

	static const gchar *hour_string = " hour";
	static const gchar *minute_string = " minute";
	static const gchar *second_string = " second";
	
	if(t<60) /* seconds only */
	{
		if(t==1)
			sprintf(s,"1%s",second_string);
		else
			sprintf(s,"%d%ss",(int)t,second_string);
	}
	else if(t<3600) /* minutes and 5 seconds */
	{
		div_t d=div(t,60);
		d.rem-=d.rem%5;
		if(d.rem)
		{
			if(d.quot==1)
				sprintf(s,"%d%s %d%ss",d.quot,minute_string,d.rem,second_string);
			else
				sprintf(s,"%d%ss %d%ss",d.quot,minute_string,d.rem,second_string);
		}
		else /* minutes only, seconds equal zero */
		{
			if(d.quot==1)
				sprintf(s,"%d%s",d.quot,minute_string);
			else
				sprintf(s,"%d%ss",d.quot,minute_string);
		}
	}
	else /* hours and 5 minutes */
	{
		div_t d=div(t/60,60);
		d.rem-=d.rem%5;
		if(d.rem)
		{
			if(d.quot==1)
				sprintf(s,"%d%s %d%ss",d.quot,hour_string,d.rem,minute_string);
			else
				sprintf(s,"%d%ss %d%ss",d.quot,hour_string,d.rem,minute_string);
		}
		else /* hours only, minuts equal zero */
		{
			if(d.quot==1)
				sprintf(s,"%d%s",d.quot,hour_string);
			else
				sprintf(s,"%d%ss",d.quot,hour_string);
		}
	}
	
	return(s);
}

void DCC_Box_Update(DCC *d)
{
	/* completely rewritten by Khalimero, may 2, 1999, still with the old format */

	static gchar dcc_dbu_tmp[64];
	static gchar dcc_time_tmp1[24];
	static gchar dcc_time_tmp2[24];
	time_t elapsed, remaining; /* elapsed and remaining time*/
	static const gchar *comp_string = "Completed: ";
	static const gchar *average_string = "Average: ";
	static const gchar *elapsed_string = "Elapsed: ";
	static const gchar *left_string = "Left: ";
	static const gchar *speed_string = " k/s";
	
	g_return_if_fail(d);

	#ifdef DEBUG_DCC
	fprintf(stdout, "DCC_Box_Update(%p)\n", d);
	#endif
	
	elapsed = ((d->End_time) ? d->End_time : time(NULL)) - d->Start_time; /* End_time=0 until transfer ends */

	if(d->Start_time) /* Start_time=0 until transfer begins */
	{
		if(d->size)
		{
			/* percentage achieved and average speed evaluation */
			if(elapsed>=5 || (d->End_time && elapsed)) /* first evaluate average speed after 5 seconds */
			{
				if(elapsed-d->Elapsed_time>=5 || d->End_time) /* refresh average speed every 5 seconds */
				{
					d->Average_speed = ((float)d->completed)/(1024*elapsed);
					d->Elapsed_time = elapsed;
				}
				sprintf(dcc_dbu_tmp, "%s%lld%%    %s%.1f%s", comp_string, (d->completed*100)/d->size, average_string, d->Average_speed, speed_string);
			}
			else
				sprintf(dcc_dbu_tmp,"%s%lld%%", comp_string, (d->completed*100)/d->size);
			gtk_label_set(d->completed_label, dcc_dbu_tmp);
			/* elapsed and evaluated remaing time */
			if(d->End_time || !d->completed || elapsed<5) /* already finished or not yet begun */
				sprintf(dcc_dbu_tmp, "%s%s", elapsed_string, DCC_Time_Format(elapsed,dcc_time_tmp1));
			else
			{
				remaining = elapsed*((float)d->size/d->completed-1); /* evaluation at average speed */
				sprintf(dcc_dbu_tmp, "%s%s    %s%s",elapsed_string,DCC_Time_Format(elapsed,dcc_time_tmp1),left_string,DCC_Time_Format(remaining,dcc_time_tmp2));
			}
			gtk_label_set(d->time_label, dcc_dbu_tmp);
		}
		else /* empty file */
		{
			sprintf(dcc_dbu_tmp, "%s100%%", comp_string);
			gtk_label_set(d->completed_label, dcc_dbu_tmp);
			gtk_label_set(d->time_label, "Warning! File was empty.");
		}
	}
	else /* still waiting */
	{
		sprintf(dcc_dbu_tmp, "%s0%%", comp_string);
		gtk_label_set(d->completed_label, dcc_dbu_tmp);
		gtk_label_set(d->time_label, "");
	}
	
}

void DCC_Create_Box(DCC *d)
{
	GtkWidget *wtmp;
	GtkWidget *vbox;
	gchar *seek;

	g_return_if_fail(d);
	g_return_if_fail(!d->dialog);

	d->dialog = (GtkDialog *) gtk_dialog_new();

	gtk_signal_connect(GTK_OBJECT(d->dialog), "destroy", GTK_SIGNAL_FUNC(DCC_Box_destroy), (gpointer) d);

	vbox = gtk_vbox_new(FALSE, 10);
	gtk_box_pack_start((GtkBox *) d->dialog->vbox, vbox, TRUE, TRUE, 0);
	gtk_container_border_width((GtkContainer *) vbox, 15);

	sprintf(dcc_tmp, "DCC %s", DCC_Types[d->Type]);
	gtk_window_set_title((GtkWindow *) d->dialog, dcc_tmp);

	seek = strrchr(d->file, '/'); if (seek) seek++; else seek = d->file;

	if (d->Type == DCC_SEND)
		sprintf(dcc_tmp, " Sending file %s to %s ", seek, d->nick);
	else
		sprintf(dcc_tmp, " Getting file %s from %s ", seek, d->nick);

	wtmp = gtk_label_new(dcc_tmp);
	gtk_box_pack_start(GTK_BOX(vbox), wtmp, TRUE, TRUE, 0);

	d->completed_label = (GtkLabel *) gtk_label_new("");
	gtk_box_pack_start((GtkBox *) vbox, (GtkWidget *) d->completed_label, TRUE, TRUE, 0);

	d->bar = (GtkProgressBar *) gtk_progress_bar_new();
	gtk_box_pack_start(GTK_BOX(vbox), (GtkWidget *) d->bar, TRUE, TRUE, 0);

	d->time_label = (GtkLabel *) gtk_label_new("");
	gtk_box_pack_start((GtkBox *) vbox, (GtkWidget *) d->time_label, TRUE, TRUE, 0);

	sprintf(dcc_tmp, "%s...", DCC_Status[d->State]);
	d->status_label = (GtkLabel *) gtk_label_new(dcc_tmp);
	gtk_box_pack_start((GtkBox *) vbox, (GtkWidget *) d->status_label, TRUE, TRUE, 0);

	wtmp = gtk_button_new_with_label(" Close ");
	GTK_WIDGET_UNSET_FLAGS(wtmp, GTK_CAN_FOCUS);
	gtk_signal_connect_object((GtkObject *) wtmp, "clicked", (GtkSignalFunc) gtk_widget_destroy, (GtkObject *) d->dialog);
	gtk_box_pack_start((GtkBox *) d->dialog->action_area, wtmp, TRUE, TRUE, 0);

	DCC_Box_Update(d);

	gtk_widget_show_all((GtkWidget *) d->dialog);
}

DCC *DCC_Send_New(gchar *partner, Server *s, gchar *file)
{
	DCC *d;
	gint fd;
	struct stat st;
	guint port;
	gchar *s1, *s2;
	gchar *merr = "DCC Send error";

	g_return_val_if_fail(partner, NULL);
	g_return_val_if_fail(s, NULL);
	g_return_val_if_fail(file, NULL);

	#ifdef DEBUG_DCC
	fprintf(stdout, "DCC_Send_New(%s, %s, %s)\n", partner, s->fs->Name, file);
	#endif

	if (stat(file, &st)==-1)
	{
		if (errno == ENOENT) sprintf(dcc_tmp, "File %s not found.", file);
		else sprintf(dcc_tmp, "Can't stat() file %s\n(%s)", file, g_strerror(errno));
		Message_Box(merr, dcc_tmp);
		return NULL;
	}

	if (!S_ISREG(st.st_mode))
	{
		sprintf(dcc_tmp, "%s is not a regular file.", file);
		Message_Box(merr, dcc_tmp);
		return NULL;
	}

	if (!st.st_size)
	{
		sprintf(dcc_tmp, "%s is empty.", file);
		Message_Box(merr, dcc_tmp);
		return NULL;
	}

	fd = open(file, O_RDONLY);

	if (fd==-1)
	{
		sprintf(dcc_tmp, "Can't open file %s\n(%s)", file, g_strerror(errno));
		Message_Box(merr, dcc_tmp);
		return NULL;
	}

	d = (DCC *) g_malloc0(sizeof(DCC));

	d->object_id = dc_id++;
	d->Type = DCC_SEND;
	d->fd_socket = -1;
	d->fd_file = fd;
	d->server = s;
	d->size = (guint32) st.st_size;
	d->State = DCC_LISTENING;
	d->file = g_strdup(file);
	d->nick = g_strdup(nick_part(partner));

 	DCC_List = g_list_append(DCC_List, d);

	DCC_Create_Box(d);

	port = Socket_Listen(&(d->fd_socket), DCC_Accept, &(d->gdk_tag), (gpointer) d, s->vw_active);

	if (port < 0)
	{
		DCC_Disconnect(d, DCC_FAILED);
		sprintf(dcc_tmp, "Error while creating the socket\n(%s)", g_strerror(-port));
		Message_Box(merr, dcc_tmp);
		return NULL;
	}
		
	s1 = strrchr(file, '/'); if (s1) s1++; else s1 = file;
	for (s2 = s1; *s2; s2++) if (*s2 == ' ') *s2 = '_';
	sprintf(dcc_tmp,"%s %lu %d %llu", s1, Our_IP, port, d->size);

	CTCP_Send(s, partner, FALSE, "DCC SEND", dcc_tmp, FALSE);

	fcntl(d->fd_socket, F_SETFL, O_NONBLOCK);

	return d;
}

DCC *DCC_Get_New(gchar *partner, Server *s, unsigned long IP, guint Port, gchar *file, guint32 bytes)
{
	DCC *d;
	gint fd;
	struct stat st;
	gchar *merr = "DCC Get error";

	g_return_val_if_fail(IP, NULL);
	g_return_val_if_fail(Port, NULL);
	g_return_val_if_fail(file, NULL);

	#ifdef DEBUG_DCC
	fprintf(stdout, "DCC_Get_New(%s, %s, %s:%u, %s, %d)\n", partner, s->fs->Name, Dump_IP(IP), Port, file, bytes);
	#endif

	if (stat(file, &st)!=-1)
	{
		if (S_ISDIR(st.st_mode))
			sprintf(dcc_tmp, "Please choose a file, not a directory.");
		else
			sprintf(dcc_tmp, "The file %s already exists (%d bytes).\nPlease choose another name.", file, (guint32) st.st_size);
		Message_Box(merr, dcc_tmp);
		return NULL;
	}

	fd = creat(file, 0600);

	if (fd==-1)
	{
		sprintf(dcc_tmp, "Can't creat file %s\n(%s)", file, g_strerror(errno));
		Message_Box(merr, dcc_tmp);
		return NULL;
	}

	d = (DCC *) g_malloc0(sizeof(DCC));

	d->object_id = dc_id++;
	d->Type = DCC_GET;
	d->fd_socket = -1;
	d->fd_file = fd;
	d->server = s;
	d->size = bytes;
	d->State = DCC_CONNECTING;
	d->file = g_strdup(file);
	d->Distant_IP = IP;
	d->Distant_Port = Port;
	d->nick = g_strdup(nick_part(partner));
	d->user = g_strdup(userhost_part(partner));

 	DCC_List = g_list_append(DCC_List, d);

	DCC_Create_Box(d);

	if (Socket_Connect(&(d->fd_socket), IP, Port, DCC_Input, &(d->gdk_tag), (gpointer) d, s->vw_active) < 0)
		DCC_Disconnect(d, DCC_FAILED);

	return d;
}

/* ----- DCC Chat --------------------------------------------------------------------- */

void DCC_Chat_Input(gpointer data, gchar *text)
{
	DCC *d = (DCC *) data;

	g_return_if_fail(d->Type==DCC_CHAT);

	#ifdef DEBUG_DCC
	fprintf(stdout, "DCC_Input(%p, %s)\n", d, text);
	#endif

	if (d->State==DCC_ACTIVE)
	{
		if (d->server && d->server->current_nick && strcmp(d->server->current_nick, d->my_nick))
		{
			g_free(d->my_nick); d->my_nick = g_strdup(d->server->current_nick);
		}

		if (strncmp(text, "\001ACTION", 7) || text[strlen(text)-1] != '\001')
		{
			VW_output(d->vw, T_OWN_DCC_MSG, "snudt", d->server, d->my_nick, "userhost", d->nick, text);
		}

		sprintf(dcc_tmp, "%s\n", text);

		if (write(d->fd_socket, dcc_tmp, strlen(dcc_tmp))==-1)
		{
			VW_output(d->vw, T_ERROR, "sF-", d->server, "write() failed (%s).",g_strerror(errno));
			DCC_Disconnect(d, DCC_FAILED);
		}

	}
	else VW_output(d->vw, T_WARNING, "st", d->server, "DCC not active.");
}

void DCC_Chat_Close(gpointer data)
{
	DCC *d = (DCC *) data;

	g_return_if_fail(d);
	g_return_if_fail(d->Type == DCC_CHAT);

	#ifdef DEBUG_DCC
	fprintf(stdout, "DCC_Chat_Close(%p)\n");
	#endif

	DCC_Disconnect(d, DCC_FINISHED);

	if (d->vw->rw->vw_active == d->vw)
		if (!GW_Activate_Last_VW(d->vw->rw) && d->server) VW_Activate(d->server->vw);

 	DCC_List = g_list_remove(DCC_List, d);
 	VW_Remove_From_GW(d->vw, FALSE);

	g_free(d->my_nick);
	g_free(d);
}

void DCC_Chat_Infos(gpointer data)
{
	DCC *d = (DCC *) data;

	g_return_if_fail(d);

	#ifdef DEBUG_DCC
	fprintf(stdout, "DCC_Chat_Infos(%s)\n", d->nick);
	#endif

	if (d->server)
		gtk_label_set((GtkLabel *) d->vw->WFL[0], d->server->fs->Name);
	else
		gtk_label_set((GtkLabel *) d->vw->WFL[0], "(server closed)");

	sprintf(dcc_tmp, "DCC Chat with %s", d->nick);
		gtk_label_set((GtkLabel *) d->vw->WFL[1], dcc_tmp);

	if (d->user)
	{
		gtk_label_set((GtkLabel *) d->vw->WFL[2], d->user);
		gtk_widget_show(d->vw->WF[2]);
	}
	else gtk_widget_hide(d->vw->WF[2]);

	gtk_label_set((GtkLabel *) d->vw->WFL[3], DCC_Status[d->State]);
}

DCC *DCC_Chat_New(gchar *partner, Server *s, unsigned long IP, gint Port)
{
	DCC *d;
	
	g_return_val_if_fail(partner, NULL);

	#ifdef DEBUG_DCC
	fprintf(stdout, "DCC_Chat_New(%s, %s, %s:%d)\n", partner, s->fs->Name, Dump_IP(IP), Port);
	#endif

	d = (DCC *) g_malloc0(sizeof(DCC));
	d->object_id = dc_id++;
	d->Type = DCC_CHAT;
	d->fd_socket = -1;
	d->fd_file = -1;
	d->server = s;
	d->my_nick = g_strdup("me");

	if (IP) /* We need to connect to our partner */
	{
		d->Distant_IP = IP;
		d->Distant_Port = Port;
		d->nick = g_strdup(nick_part(partner));
		d->user = g_strdup(userhost_part(partner));
		d->State = DCC_CONNECTING;

		d->vw = VW_new();
		d->vw->pmask.w_type = W_DCC;
		d->vw->Name = d->nick;
		d->vw->Title = d->nick;
		d->vw->Resource = (gpointer) d;
		d->vw->Infos = DCC_Chat_Infos;
		d->vw->Close = DCC_Chat_Close;
		d->vw->Input = DCC_Chat_Input;

		VW_Init(d->vw, s->vw->rw, 4);

		VW_Activate(d->vw);

		if (Socket_Connect(&(d->fd_socket), IP, Port, DCC_Input, &(d->gdk_tag), (gpointer) d, d->vw) < 0)
		{
			g_free(d->nick);
			g_free(d->user);
			g_free(d);
			return NULL;
		}
		
		if (d->vw) VW_Status(d->vw);
	}
	else /* Our partner will connect to our socket */
	{
		gint port;

		d->nick = g_strdup(nick_part(partner));
		d->State = DCC_LISTENING;

		d->vw = VW_new();
		d->vw->pmask.w_type = W_DCC;
		d->vw->Name = d->nick;
		d->vw->Title = d->nick;
		d->vw->Resource = (gpointer) d;
		d->vw->Infos = DCC_Chat_Infos;
		d->vw->Close = DCC_Chat_Close;
		d->vw->Input = DCC_Chat_Input;

		VW_Init(d->vw, s->vw->rw, 4);

		VW_Activate(d->vw);

		port = Socket_Listen(&(d->fd_socket), DCC_Accept, &(d->gdk_tag), (gpointer) d, d->vw);

		if (port < 0)
		{
			d->State = DCC_FAILED;
			VW_output(d->vw, T_ERROR, "st", d->server, "DCC Chat failed (can't creat the socket).");
		}
		else
		{
			VW_output(d->vw, T_NORMAL, "sF--", d->server, "Created socket %s:%u", Dump_IP(Our_IP), port);
			VW_output(d->vw, T_NORMAL, "sF-", d->server, "Offering DCC chat to %s...", d->nick);

			sprintf(dcc_tmp, "chat %lu %d", Our_IP, port);
			CTCP_Send(s, partner, FALSE, "DCC CHAT", dcc_tmp, FALSE);
		}

		if (d->vw) VW_Status(d->vw);
	}

	d->my_nick = g_strdup(s->current_nick);

 	DCC_List = g_list_append(DCC_List, d);

	return d;
}

/* ----- DCC Send Fileselector  ------------------------------------------------------- */

struct DCC_Send_Queued
{
	Server *server;
	gchar *nick;
	GtkFileSelection *filesel;
};

void DCC_Send_Filesel_destroy(GtkWidget *w, gpointer data)
{
	struct DCC_Send_Queued *dsq = (struct DCC_Send_Queued *) data;
	g_free(dsq->nick);
	g_free(dsq);
}

void DCC_Send_Filesel_Ok(GtkWidget *w, gpointer data)
{
	struct DCC_Send_Queued *dsq = (struct DCC_Send_Queued *) data;
	DCC_Send_New(dsq->nick, dsq->server, Remember_Filepath(dsq->filesel, &Olirc->dcc_send_path));
	gtk_widget_destroy((GtkWidget *) dsq->filesel);
}

void DCC_Send_Filesel(Server *s, gchar *nick)
{
	struct DCC_Send_Queued *dsq;

	g_return_if_fail(nick);
	g_return_if_fail(s);

	dsq = g_malloc0(sizeof(struct DCC_Send_Queued));

	dsq->server = s;
	dsq->nick = g_strdup(nick);

	sprintf(dcc_tmp, "Sending a file to %s", nick);

	dsq->filesel = (GtkFileSelection *) gtk_file_selection_new(dcc_tmp);
	Add_Home_Button(dsq->filesel);

	if (Olirc->dcc_send_path)
		gtk_file_selection_set_filename(dsq->filesel, Olirc->dcc_send_path);

	gtk_signal_connect(GTK_OBJECT(dsq->filesel->ok_button), "clicked", GTK_SIGNAL_FUNC(DCC_Send_Filesel_Ok), (gpointer) dsq);
	gtk_signal_connect_object(GTK_OBJECT(dsq->filesel->cancel_button), "clicked", GTK_SIGNAL_FUNC(gtk_widget_destroy), GTK_OBJECT(dsq->filesel));

	gtk_signal_connect(GTK_OBJECT(dsq->filesel), "destroy", GTK_SIGNAL_FUNC(DCC_Send_Filesel_destroy), (gpointer) dsq);

	gtk_widget_show((GtkWidget *) dsq->filesel);
}

/* ----- DCC Request Dialog box ------------------------------------------------------- */

struct DCC_Req_Queued
{
	GtkDialog *dialog;
	GtkFileSelection *filesel;
	guint type;
	gchar *partner;
	Server *server;
	unsigned long IP;
	guint Port;
	gchar *file;
	guint32 bytes;
};

void DCC_Request_destroy(GtkWidget *w, gpointer data)
{
	struct DCC_Req_Queued *drq = (struct DCC_Req_Queued *) data;
	if (drq->filesel) gtk_widget_destroy((GtkWidget *) drq->filesel);
	g_free(drq->partner);
	g_free(drq);
}

void DCC_Request_Filesel_Ok(GtkWidget *w, gpointer data)
{
	struct DCC_Req_Queued *drq = (struct DCC_Req_Queued *) data;
	if (DCC_Get_New(drq->partner, drq->server, drq->IP, drq->Port, Remember_Filepath(drq->filesel, &Olirc->dcc_get_path), drq->bytes))
		gtk_widget_destroy((GtkWidget *) drq->dialog);
}

void DCC_Request_Accept(GtkWidget *w, gpointer data)
{
	struct DCC_Req_Queued *drq = (struct DCC_Req_Queued *) data;
	if (drq->type == DCC_CHAT)
	{
		DCC_Chat_New(drq->partner, drq->server, drq->IP, drq->Port);
		gtk_widget_destroy((GtkWidget *) drq->dialog);
	}
	else
	{
		gchar *nick = nick_part(drq->partner);

		gtk_widget_hide((GtkWidget *) drq->dialog);

		sprintf(dcc_tmp, "Getting file %s from %s", drq->file, nick);

		drq->filesel = (GtkFileSelection *) gtk_file_selection_new(dcc_tmp);
		Add_Home_Button(drq->filesel);

		if (Olirc->dcc_get_path)
			gtk_file_selection_set_filename(drq->filesel, Olirc->dcc_get_path);

		gtk_signal_connect(GTK_OBJECT(drq->filesel->ok_button), "clicked", GTK_SIGNAL_FUNC(DCC_Request_Filesel_Ok), (gpointer) drq);
		gtk_signal_connect_object(GTK_OBJECT(drq->filesel->cancel_button), "clicked", GTK_SIGNAL_FUNC(gtk_widget_destroy), GTK_OBJECT(drq->dialog));

		gtk_file_selection_set_filename(drq->filesel, drq->file);

		gtk_widget_show((GtkWidget *) drq->filesel);
	}
}

void DCC_Request(guint type, gchar *partner, Server *s, unsigned long IP, guint Port, gchar *file, guint32 bytes)
{
	GtkWidget *wtmp;
	struct DCC_Req_Queued *drq;
	gchar *n, *u;

	g_return_if_fail(type==DCC_CHAT || type==DCC_GET);

	drq = g_malloc0(sizeof(struct DCC_Req_Queued));

	drq->type = type;
	drq->partner = g_strdup(partner);
	drq->server = s;
	drq->IP = IP;
	drq->Port = Port;
	drq->file = file;
	drq->bytes = bytes;

	drq->dialog = GTK_DIALOG(gtk_dialog_new());

	if (type==DCC_CHAT) sprintf(dcc_tmp, "DCC Chat request");
	else sprintf(dcc_tmp, "DCC Send request");

	gtk_window_set_title(GTK_WINDOW(drq->dialog), dcc_tmp);

	n = nick_part(partner); u = userhost_part(partner);

	if (type==DCC_CHAT)
		sprintf(dcc_tmp,"\n %s (%s) is sending a DCC Chat request \n to you (%s:%u). \n\n Do you want to chat with %s ? \n", n, u, Dump_IP(IP), Port, n);	
	else
		sprintf(dcc_tmp,"\n %s (%s) wants to send you the file \n %s (%d bytes). \n\n Do you want to get this file ? \n", n, u, file, bytes);

	wtmp = gtk_label_new(dcc_tmp);
	gtk_box_pack_start(GTK_BOX(drq->dialog->vbox), wtmp, FALSE, FALSE, 0);

	wtmp = gtk_button_new_with_label(" Accept ");
	gtk_signal_connect(GTK_OBJECT(wtmp), "clicked", GTK_SIGNAL_FUNC(DCC_Request_Accept), (gpointer) drq);
	GTK_WIDGET_UNSET_FLAGS(wtmp, GTK_CAN_FOCUS);
	gtk_box_pack_start(GTK_BOX(drq->dialog->action_area), wtmp, FALSE, FALSE, 0);

	wtmp = gtk_button_new_with_label(" Reject ");
	gtk_signal_connect_object(GTK_OBJECT(wtmp), "clicked", GTK_SIGNAL_FUNC(gtk_widget_destroy), GTK_OBJECT(drq->dialog));
	GTK_WIDGET_UNSET_FLAGS(wtmp, GTK_CAN_FOCUS);
	gtk_box_pack_start(GTK_BOX(drq->dialog->action_area), wtmp, FALSE, FALSE, 0);

	gtk_signal_connect(GTK_OBJECT(drq->dialog), "destroy", GTK_SIGNAL_FUNC(DCC_Request_destroy), (gpointer) drq);

	gtk_widget_show_all((GtkWidget *) drq->dialog);
}

/* ------DCC Connections -------------------------------------------------------------- */
/*
GtkWidget *DCC_Connections_Window = NULL;
GtkWidget *DCC_CList;

void DCC_Connections_destroy(GtkWidget *w, gpointer data)
{
	DCC_Connections_Window = NULL;
}

void DCC_Connections_Update(DCC *d, gboolean unknown)
{
	if (!DCC_Connections_Window) return;
}

void DCC_Display_Connections()
{
	GtkWidget *vbox, *hbox;
	GtkWidget *wtmp;

	static char *tdc[] = { "Type", "Status", "To/From", "IP:Port", "Time", "File", "Size", "Position" };

	if (DCC_Connections_Window) return;

	DCC_Connections_Window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_policy(GTK_WINDOW(DCC_Connections_Window), TRUE, TRUE, FALSE);
	gtk_widget_set_usize(DCC_Connections_Window, 630, 320);
	gtk_container_border_width(GTK_CONTAINER(DCC_Connections_Window), 4);

	gtk_window_set_title(GTK_WINDOW(DCC_Connections_Window), "DCC Connections");

	vbox = gtk_vbox_new(FALSE, 8);
	gtk_container_add(GTK_CONTAINER(DCC_Connections_Window), vbox);

	DCC_CList = gtk_clist_new_with_titles(8, tdc);
	gtk_clist_column_titles_passive(GTK_CLIST(DCC_CList));
	gtk_clist_set_column_width(GTK_CLIST(DCC_CList), 0, 60);
	gtk_clist_set_column_width(GTK_CLIST(DCC_CList), 1, 60);
	gtk_clist_set_column_width(GTK_CLIST(DCC_CList), 2, 60);
	gtk_clist_set_column_width(GTK_CLIST(DCC_CList), 3, 100);
	gtk_clist_set_column_width(GTK_CLIST(DCC_CList), 4, 60);
	gtk_clist_set_column_width(GTK_CLIST(DCC_CList), 5, 120);
	gtk_clist_set_column_width(GTK_CLIST(DCC_CList), 6, 50);
	gtk_clist_set_column_width(GTK_CLIST(DCC_CList), 7, 50);
	wtmp = gtk_scrolled_window_new(NULL, NULL);
	gtk_container_add(GTK_CONTAINER(wtmp), DCC_CList);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(wtmp), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	GTK_WIDGET_UNSET_FLAGS(GTK_SCROLLED_WINDOW(wtmp)->vscrollbar, GTK_CAN_FOCUS);
	gtk_box_pack_start(GTK_BOX(vbox), wtmp, TRUE, TRUE, 0);

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_end(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

	wtmp = gtk_button_new_with_label(" Abort connection ");
	GTK_WIDGET_UNSET_FLAGS(wtmp, GTK_CAN_FOCUS);
	gtk_box_pack_start(GTK_BOX(hbox), wtmp, FALSE, FALSE, 0);
	gtk_widget_set_sensitive(wtmp, FALSE);

	wtmp = gtk_button_new_with_label(" Close this window ");
	gtk_signal_connect_object(GTK_OBJECT(wtmp), "clicked", GTK_SIGNAL_FUNC(gtk_widget_destroy), GTK_OBJECT(DCC_Connections_Window));
	GTK_WIDGET_UNSET_FLAGS(wtmp, GTK_CAN_FOCUS);
	gtk_box_pack_end(GTK_BOX(hbox), wtmp, FALSE, FALSE, 0);

	{
		gchar *texts[10];
		GList *l = DCC_List;
		DCC *d;

		while(l)
		{
			d = (DCC *) l->data;
			texts[0] = DCC_Types[d->Type];
			texts[1] = Dcc_Status[d->State];
			texts[2] = "";
			sprintf(dcc_tmp, "%s:%u", Dump_IP(d->Distant_IP), d->Distant_Port);
			texts[3] = dcc_tmp;
			texts[4] = "";
			texts[5] = "";
			texts[6] = "";
			texts[7] = "?";

			gtk_clist_append((GtkCList *) DCC_CList, texts);

			l = l->next;
		}
	}


	gtk_signal_connect(GTK_OBJECT(DCC_Connections_Window), "destroy", GTK_SIGNAL_FUNC(DCC_Connections_destroy), NULL);

	gtk_widget_show_all(DCC_Connections_Window);
}
*/	

/* vi: set ts=3: */

