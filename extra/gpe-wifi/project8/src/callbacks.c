#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <sys/socket.h>
#include <net/if.h>

#include <sys/ioctl.h>
#include <arpa/inet.h>

#include <gtk/gtk.h>

#include <iwlib.h>

#include "callbacks.h"
#include "interface.h"
#include "support.h"

#define IF_WIFI "eth0"

static int Wfd;

static struct wireless_info WInfo;

static int check_essid(gpointer user_data)
{

	GtkWidget *entry6 = lookup_widget(GTK_WIDGET(user_data), "entry6");
	char dline[64], func[12], value[64];
	FILE *f;

	f = fopen("/etc/network/interfaces", "r");

	if (!f)
		fprintf(stderr, "problem opening /etc/network/interfaces\n");
	else {
		while (fgets(dline, sizeof(dline), f)) {
			if (sscanf(dline, "%s %s", func, value) == 2) {
				if (strcmp(func, "wireless-essid") == 0) {
					gtk_entry_set_text(GTK_ENTRY(entry6), _(value));
				}
			}
		}
	}
}

static int get_addr(int sock, char *ifname, struct sockaddr *ifaddr)
{
	struct ifreq *ifr;
	struct ifreq ifrr;

	ifr = &ifrr;
	ifrr.ifr_addr.sa_family = AF_INET;

	strncpy(ifrr.ifr_name, ifname, sizeof(ifrr.ifr_name));
	if (ioctl(sock, SIOCGIFADDR, ifr) < 0) {
		return -1;
	}
	*ifaddr = ifrr.ifr_addr;

	return 0;
}

guint update_wireless(gpointer user_data)
{
	GtkWidget *entry3 = lookup_widget(GTK_WIDGET(user_data), "entry3");
	GtkWidget *entry6 = lookup_widget(GTK_WIDGET(user_data), "entry6");
	GtkWidget *entry9 = lookup_widget(GTK_WIDGET(user_data), "entry9");
	GtkWidget *entry10 = lookup_widget(GTK_WIDGET(user_data), "entry10");
	GtkWidget *entry12 = lookup_widget(GTK_WIDGET(user_data), "entry12");
	GtkWidget *entry13 = lookup_widget(GTK_WIDGET(user_data), "entry13");
	char *status;
	int sockfd;
	struct sockaddr ifa;
	struct sockaddr_in sa;
	unsigned long ad, host_ad;
	struct iwreq wrq;
	gchar tmp_str[64];
	int sig_level = 0;

	if (iw_get_basic_config(Wfd, IF_WIFI, &WInfo.b) < 0) {
		fprintf(stderr, "mb-applet-wireless: unable to read wireless config\n");
		return FALSE;
	}

	if (iw_get_range_info(Wfd, IF_WIFI, &(WInfo.range)) >= 0)
		WInfo.has_range = 1;

	if (iw_get_stats(Wfd, IF_WIFI, &(WInfo.stats), &(WInfo.range), WInfo.has_range) >= 0)
		WInfo.has_stats = 1;

	iw_get_ext(Wfd, IF_WIFI, SIOCGIWAP, &wrq);

	status = iw_sawap_ntop(&wrq.u.ap_addr, tmp_str);
	if (strcmp("Not-Associated", status) == 0) {
		check_essid(user_data);
		gtk_entry_set_text(GTK_ENTRY(entry3), _(status));
		gtk_entry_set_text(GTK_ENTRY(entry9), _("N/A"));
		gtk_entry_set_text(GTK_ENTRY(entry10), _("N/A"));
		gtk_entry_set_text(GTK_ENTRY(entry12), _("N/A"));
		gtk_entry_set_text(GTK_ENTRY(entry13), _("N/A"));
	} else {
		gtk_entry_set_text(GTK_ENTRY(entry3), _("Connected"));
		gtk_entry_set_text(GTK_ENTRY(entry6), _(WInfo.b.essid));
		gtk_entry_set_text(GTK_ENTRY(entry13), _(status));


		sig_level = (int) WInfo.stats.qual.level;
		if (sig_level >= 64)
			sig_level -= 0x100;

		sprintf(tmp_str, "%d dBm", sig_level);
		gtk_entry_set_text(GTK_ENTRY(entry9), _(tmp_str));
		sprintf(tmp_str, "%d%%", WInfo.stats.qual.qual);
		gtk_entry_set_text(GTK_ENTRY(entry10), _(tmp_str));


		if (0 > (sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP))) {
			fprintf(stderr, "Cannot open socket.\n");
			exit(EXIT_FAILURE);
		}

		get_addr(sockfd, IF_WIFI, &ifa);
		gtk_entry_set_text(GTK_ENTRY(entry12),
			_(inet_ntoa(*(struct in_addr *) &ifa.sa_data[sizeof sa.sin_port])));
	}

	close(sockfd);


}

guint switch_to_status(gpointer user_data)
{
	GtkWidget *notebook1 = lookup_widget(GTK_WIDGET(user_data), "notebook1");
	GtkWidget *fixed1 = lookup_widget(GTK_WIDGET(user_data), "fixed1");
	GtkWidget *fixed3 = lookup_widget(GTK_WIDGET(user_data), "fixed3");
	GtkWidget *label2 = lookup_widget(GTK_WIDGET(user_data), "label2");
	GtkWidget *label5 = lookup_widget(GTK_WIDGET(user_data), "label5");

	gtk_notebook_remove_page(GTK_NOTEBOOK(notebook1), -1);
	gtk_notebook_insert_page(GTK_NOTEBOOK(notebook1), GTK_WIDGET(fixed1), GTK_WIDGET(label2), -1);
	gtk_notebook_insert_page(GTK_NOTEBOOK(notebook1), GTK_WIDGET(fixed3), GTK_WIDGET(label5), -1);

	return FALSE;
}

guint start_connection(gpointer user_data)
{
	GtkWidget *label4 = lookup_widget(GTK_WIDGET(user_data), "label4");
	gchar sys_cmd[128];
	system("ifdown eth0");
	sprintf(sys_cmd, "/usr/bin/net-interfaces %s",
			gtk_label_get_text(GTK_LABEL(label4)));
	system(sys_cmd);
	system("ifup eth0");
	update_wireless(user_data);
	g_timeout_add_full(G_PRIORITY_DEFAULT, 1250, switch_to_status,
					   user_data, NULL);

	return FALSE;
}

void on_window1_show(GtkWidget * widget, gpointer user_data)
{

	GtkWidget *label4 = lookup_widget(GTK_WIDGET(user_data), "label4");
	GtkWidget *notebook1 =
		lookup_widget(GTK_WIDGET(user_data), "notebook1");

	Wfd = iw_sockets_open();

	if (strcmp("None", gtk_label_get_text(GTK_LABEL(label4))) == 0) {
		gtk_notebook_remove_page(GTK_NOTEBOOK(notebook1), 0);
		update_wireless(user_data);
	} else {
		gtk_notebook_remove_page(GTK_NOTEBOOK(notebook1), -1);
		gtk_notebook_remove_page(GTK_NOTEBOOK(notebook1), -1);
		g_timeout_add_full(G_PRIORITY_DEFAULT, 250, start_connection,
						   user_data, NULL);
	}

}

gboolean on_button2_button_release_event(GtkWidget * widget, GdkEventButton * event, gpointer user_data)
{
	GtkWidget *window1 = lookup_widget(GTK_WIDGET(user_data), "window1");
	GtkWidget *notebook1 = lookup_widget(GTK_WIDGET(user_data), "notebook1");
	GtkWidget *label1 = lookup_widget(GTK_WIDGET(user_data), "label1");
	GtkWidget *fixed2 = lookup_widget(GTK_WIDGET(user_data), "fixed2");

	gtk_notebook_remove_page(GTK_NOTEBOOK(notebook1), -1);
	gtk_notebook_remove_page(GTK_NOTEBOOK(notebook1), -1);
	gtk_notebook_insert_page(GTK_NOTEBOOK(notebook1), GTK_WIDGET(fixed2),  GTK_WIDGET(label1), -1);
	g_timeout_add_full(G_PRIORITY_DEFAULT, 250, start_connection, window1, NULL);

	return FALSE;
}
