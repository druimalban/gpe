#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>
#include <unistd.h>

#include "callbacks.h"
#include "interface.h"
#include "support.h"

static gchar return_cmd[64];

gint security=0,ipmethod=0,wpamethod;

void
save_settings(gpointer user_data){

    FILE *f;
    GtkWidget * entry15 = lookup_widget(GTK_WIDGET(user_data), "entry15");
    GtkWidget * entry11 = lookup_widget(GTK_WIDGET(user_data), "entry11");
    GtkWidget * entry12 = lookup_widget(GTK_WIDGET(user_data), "entry12");
    GtkWidget * entry13 = lookup_widget(GTK_WIDGET(user_data), "entry13");
    GtkWidget * entry14 = lookup_widget(GTK_WIDGET(user_data), "entry14");
    GtkWidget * entry7 = lookup_widget(GTK_WIDGET(user_data), "entry7");
    GtkWidget * entry8 = lookup_widget(GTK_WIDGET(user_data), "entry8");
    GtkWidget * entry9 = lookup_widget(GTK_WIDGET(user_data), "entry9");
    GtkWidget * entry10 = lookup_widget(GTK_WIDGET(user_data), "entry10");
    GtkWidget * spinbutton1 = lookup_widget(GTK_WIDGET(user_data), "spinbutton1");
    gchar *pathname = g_strdup_printf ("/etc/wifi/%s.cfg",gtk_entry_get_text(GTK_ENTRY(entry15)));

        f = fopen(pathname, "w");

	if (ipmethod) 
    	    fprintf(f,"ipmethod = DHCP\n");
	else {
    	    fprintf(f,"ipmethod = Static\n");
    	}
    	
    	if (security==1) {
    	    fprintf(f,"security = WEP\n");
    	    fprintf(f,"default_key = %d\n",gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(spinbutton1)));
    	    fprintf(f,"key_0 = %s\n",gtk_entry_get_text(GTK_ENTRY(entry7)));
    	    fprintf(f,"key_1 = %s\n",gtk_entry_get_text(GTK_ENTRY(entry8)));
    	    fprintf(f,"key_2 = %s\n",gtk_entry_get_text(GTK_ENTRY(entry9)));
    	    fprintf(f,"key_3 = %s\n",gtk_entry_get_text(GTK_ENTRY(entry10)));
    	}
    	else if (security==2) {
    	    fprintf(f,"security = WPA-PSK\n");
	    if (wpamethod==1)
    		fprintf(f,"wpa_encryption = AES-CCMP\n");
    	    else
    		fprintf(f,"wpa_encryption = TKIP\n");
    		
    	    fprintf(f,"passphrase = %s\n",gtk_entry_get_text(GTK_ENTRY(entry11)));
    	}
    	else if (security==3) {
    	    fprintf(f,"security = WPA-EAP\n");
	    if (wpamethod==3) {
    		fprintf(f,"wpa_encryption = PEAP\n");
    		fprintf(f,"username = %s\n",gtk_entry_get_text(GTK_ENTRY(entry12)));
    		fprintf(f,"password = %s\n",gtk_entry_get_text(GTK_ENTRY(entry13)));
    	    }
    	    else if (wpamethod==4) {
    		fprintf(f,"wpa_encryption = TLS\n");
    		fprintf(f,"username = %s\n",gtk_entry_get_text(GTK_ENTRY(entry12)));
    		fprintf(f,"passphrase = %s\n",gtk_entry_get_text(GTK_ENTRY(entry14)));
    	    }
    	    else {
    		fprintf(f,"wpa_encryption = TTLS\n");
    		fprintf(f,"username = %s\n",gtk_entry_get_text(GTK_ENTRY(entry12)));
    		fprintf(f,"password = %s\n",gtk_entry_get_text(GTK_ENTRY(entry13)));
    	    }
    		
    	}
    	else
    	    fprintf(f,"security = NONE\n");
    		
        fclose(f);


}

void
set_ipmethod                  (gpointer value, gpointer user_data)
{

    GtkWidget * entry2 = lookup_widget(GTK_WIDGET(user_data), "entry2");
    GtkWidget * entry3 = lookup_widget(GTK_WIDGET(user_data), "entry3");
    GtkWidget * entry4 = lookup_widget(GTK_WIDGET(user_data), "entry4");
    GtkWidget * entry5 = lookup_widget(GTK_WIDGET(user_data), "entry5");
    GtkWidget * label5 = lookup_widget(GTK_WIDGET(user_data), "label5");
    GtkWidget * label6 = lookup_widget(GTK_WIDGET(user_data), "label6");
    GtkWidget * label7 = lookup_widget(GTK_WIDGET(user_data), "label7");
    GtkWidget * label8 = lookup_widget(GTK_WIDGET(user_data), "label8");

    
    if (strcmp(value,"DHCP")==0) {
	    ipmethod=1;
	    gtk_widget_hide(entry2);
	    gtk_widget_hide(entry3);
	    gtk_widget_hide(entry4);
	    gtk_widget_hide(entry5);

	    gtk_widget_hide(label5);
	    gtk_widget_hide(label6);
	    gtk_widget_hide(label7);
	    gtk_widget_hide(label8);
    }	
    else {
	    ipmethod=0;
	    gtk_widget_show(entry2);
	    gtk_widget_show(entry3);
	    gtk_widget_show(entry4);
	    gtk_widget_show(entry5);
	
	    gtk_widget_show(label5);
	    gtk_widget_show(label6);
	    gtk_widget_show(label7);
	    gtk_widget_show(label8);
	    
    }
}

void
set_wpamethod                  (gpointer value, gpointer user_data)
{

    GtkWidget * entry12 = lookup_widget(GTK_WIDGET(user_data), "entry12");
    GtkWidget * entry13 = lookup_widget(GTK_WIDGET(user_data), "entry13");
    GtkWidget * entry14 = lookup_widget(GTK_WIDGET(user_data), "entry14");
    GtkWidget * label22 = lookup_widget(GTK_WIDGET(user_data), "label22");
    GtkWidget * label23 = lookup_widget(GTK_WIDGET(user_data), "label23");
    GtkWidget * label24 = lookup_widget(GTK_WIDGET(user_data), "label24");


    
    if (strcmp(value,"TLS")==0) {
	    wpamethod=4;
	    gtk_widget_hide(entry13);
	    gtk_widget_hide(label24);
	    gtk_widget_show(entry14);
	    gtk_widget_show(label23);
    }	
    else if (strcmp(value,"PEAP")==0) {
	    wpamethod=3;
	    gtk_widget_hide(entry14);
	    gtk_widget_hide(label23);
	    gtk_widget_show(entry13);
	    gtk_widget_show(label24);
	    
    }
    else if (strcmp(value,"TTLS")==0) {
	    wpamethod=5;
	    gtk_widget_hide(entry14);
	    gtk_widget_hide(label23);
	    gtk_widget_show(entry13);
	    gtk_widget_show(label24);
	    
    }
}



static void set_radio_button(gpointer local_button, gpointer user_data) {
    GList *child = gtk_container_children(GTK_CONTAINER(local_button));
    if (!strcmp(gtk_label_get_text(GTK_LABEL(child->data)),user_data)){
	gtk_toggle_button_set_active(local_button,TRUE);
    }

}

void
set_return_cmd(gchar *return_str){

sprintf(return_cmd,"%s",return_str);

}

void
on_window1_show                        (GtkWidget       *widget,
                                        gpointer         user_data)
{

 GtkWidget * notebook1 = lookup_widget(GTK_WIDGET(user_data), "notebook1");
 GtkWidget * entry15 = lookup_widget(GTK_WIDGET(user_data), "entry15");
 GtkWidget * radiobutton6 = lookup_widget(GTK_WIDGET(user_data), "radiobutton6");
 GtkWidget * entry6 = lookup_widget(GTK_WIDGET(user_data), "entry6");
 gchar *pathname = g_strdup_printf ("/etc/wifi/%s.cfg",gtk_entry_get_text(GTK_ENTRY(entry15)));
 char dline[64], func[32], value[64];
 FILE *f;

gtk_notebook_remove_page(GTK_NOTEBOOK(notebook1),-1);
gtk_notebook_remove_page(GTK_NOTEBOOK(notebook1),-1);
gtk_notebook_remove_page(GTK_NOTEBOOK(notebook1),-1);

 if (g_file_test (pathname, G_FILE_TEST_EXISTS)) {
	f=fopen(pathname, "r");
	
	if (f)
	{
		while(fgets(dline,sizeof(dline),f)) {
			if (sscanf(dline,"%s = %s", func, value) == 2) {
				if (strcmp(func, "ipmethod")==0) {
					GtkWidget * radiobutton5 = lookup_widget(GTK_WIDGET(user_data), "radiobutton5");
					GSList *buttonlist = gtk_radio_button_get_group(GTK_RADIO_BUTTON(radiobutton5));
					g_slist_foreach(buttonlist,set_radio_button,&value);
					set_ipmethod(&value,user_data);
				}
				if (strcmp(func, "security")==0) {
					if (strcmp(value,"WEP")==0) {
					    GtkWidget * radiobutton6 = lookup_widget(GTK_WIDGET(user_data), "radiobutton6");
					    GtkWidget * fixed4 = lookup_widget(GTK_WIDGET(user_data), "fixed4");
					    GtkWidget * label3 = lookup_widget(GTK_WIDGET(user_data), "label3");
					    GSList *buttonlist = gtk_radio_button_get_group(GTK_RADIO_BUTTON(radiobutton6));
					    security=1;
					    gtk_notebook_insert_page(GTK_NOTEBOOK(notebook1),GTK_WIDGET(fixed4),GTK_WIDGET(label3),-1);
					    g_slist_foreach(buttonlist,set_radio_button,&value);
					}
					else if (strcmp(value,"WPA-PSK")==0) {
					    GtkWidget * radiobutton6 = lookup_widget(GTK_WIDGET(user_data), "radiobutton6");
					    GSList *buttonlist = gtk_radio_button_get_group(GTK_RADIO_BUTTON(radiobutton6));
					    GtkWidget * fixed5 = lookup_widget(GTK_WIDGET(user_data), "fixed5");
					    GtkWidget * label12 = lookup_widget(GTK_WIDGET(user_data), "label12");
					    security=2;
					    gtk_notebook_insert_page(GTK_NOTEBOOK(notebook1),GTK_WIDGET(fixed5),GTK_WIDGET(label12),-1);
					    g_slist_foreach(buttonlist,set_radio_button,&value);
					}
					else if (strcmp(value,"WPA-EAP")==0) {
					    GtkWidget * radiobutton6 = lookup_widget(GTK_WIDGET(user_data), "radiobutton6");
					    GtkWidget * fixed6 = lookup_widget(GTK_WIDGET(user_data), "fixed6");
					    GtkWidget * label13 = lookup_widget(GTK_WIDGET(user_data), "label13");
					    GSList *buttonlist = gtk_radio_button_get_group(GTK_RADIO_BUTTON(radiobutton6));
					    security=3;
					    gtk_notebook_insert_page(GTK_NOTEBOOK(notebook1),GTK_WIDGET(fixed6),GTK_WIDGET(label13),-1);
					    g_slist_foreach(buttonlist,set_radio_button,&value);
					}
					else {
					    GtkWidget * radiobutton6 = lookup_widget(GTK_WIDGET(user_data), "radiobutton6");
					    GSList *buttonlist = gtk_radio_button_get_group(GTK_RADIO_BUTTON(radiobutton6));
					    security=0;
					    g_slist_foreach(buttonlist,set_radio_button,_("None"));
					}
				}
				if (security==1) {
				    if (strcmp(func, "default_key")==0) {
					GtkWidget * spinbutton1 = lookup_widget(GTK_WIDGET(user_data), "spinbutton1");
					gtk_spin_button_set_value(GTK_SPIN_BUTTON(spinbutton1),atoi(value));
				    }
				    if (strcmp(func, "key_0")==0) {
					GtkWidget * entry7 = lookup_widget(GTK_WIDGET(user_data), "entry7");
					gtk_entry_set_text(GTK_ENTRY(entry7),value);
				    }
				    if (strcmp(func, "key_1")==0) {
					GtkWidget * entry8 = lookup_widget(GTK_WIDGET(user_data), "entry8");
					gtk_entry_set_text(GTK_ENTRY(entry8),value);
				    }
				    if (strcmp(func, "key_2")==0) {
					GtkWidget * entry9 = lookup_widget(GTK_WIDGET(user_data), "entry9");
					gtk_entry_set_text(GTK_ENTRY(entry9),value);
				    }
				    if (strcmp(func, "key_3")==0) {
					GtkWidget * entry10 = lookup_widget(GTK_WIDGET(user_data), "entry10");
					gtk_entry_set_text(GTK_ENTRY(entry10),value);
				    }
				}
				else if (security==2) {
				    if (strcmp(func, "passphrase")==0) {
					GtkWidget * entry11 = lookup_widget(GTK_WIDGET(user_data), "entry11");
					gtk_entry_set_text(GTK_ENTRY(entry11),value);
				    }
				    if (strcmp(func, "wpa_encryption")==0) {
					GtkWidget * radiobutton10 = lookup_widget(GTK_WIDGET(user_data), "radiobutton10");
					GSList *buttonlist = gtk_radio_button_get_group(GTK_RADIO_BUTTON(radiobutton10));
					if (strcmp(value,"AES-CCMP")==0) {
					    wpamethod=1;
					    g_slist_foreach(buttonlist,set_radio_button,_("AES-CCMP"));
					}
					else {
					    wpamethod=2;
					    g_slist_foreach(buttonlist,set_radio_button,_("TKIP"));
					}
				    }
				}
				else if (security==3) {
				    if (strcmp(func, "passphrase")==0) {
					GtkWidget * entry14 = lookup_widget(GTK_WIDGET(user_data), "entry14");
					gtk_entry_set_text(GTK_ENTRY(entry14),value);
				    }
				    if (strcmp(func, "username")==0) {
					GtkWidget * entry12 = lookup_widget(GTK_WIDGET(user_data), "entry12");
					gtk_entry_set_text(GTK_ENTRY(entry12),value);
				    }
				    if (strcmp(func, "password")==0) {
					GtkWidget * entry13 = lookup_widget(GTK_WIDGET(user_data), "entry13");
					gtk_entry_set_text(GTK_ENTRY(entry13),value);
				    }
				    if (strcmp(func, "wpa_encryption")==0) {
					GtkWidget * radiobutton12 = lookup_widget(GTK_WIDGET(user_data), "radiobutton12");
					GSList *buttonlist = gtk_radio_button_get_group(GTK_RADIO_BUTTON(radiobutton12));
					if (strcmp(value,"PEAP")==0) {
					    g_slist_foreach(buttonlist,set_radio_button,&value);
					    set_wpamethod(&value,user_data);
					    wpamethod=3;
					}
					if (strcmp(value,"TLS")==0) {
					    g_slist_foreach(buttonlist,set_radio_button,&value);
					    set_wpamethod(&value,user_data);
					    wpamethod=4;
					}
					if (strcmp(value,"TTLS")==0) {
					    g_slist_foreach(buttonlist,set_radio_button,&value);
					    set_wpamethod(&value,user_data);
					    wpamethod=5;
					}
				    }
				}
			}
		}
		fclose(f);
	}


	gtk_entry_set_text(GTK_ENTRY(entry6),gtk_entry_get_text(GTK_ENTRY(entry15)));

 }



}


gboolean
on_radiobutton7_button_press_event     (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{

GtkWidget * notebook1 = lookup_widget(GTK_WIDGET(user_data), "notebook1");
GtkWidget * fixed4 = lookup_widget(GTK_WIDGET(user_data), "fixed4");
GtkWidget * label3 = lookup_widget(GTK_WIDGET(user_data), "label3");
gint num_pages=0;

num_pages=gtk_notebook_get_n_pages(GTK_NOTEBOOK(notebook1));

if (num_pages==4)
    gtk_notebook_remove_page(GTK_NOTEBOOK(notebook1),-1);

gtk_notebook_insert_page(GTK_NOTEBOOK(notebook1),GTK_WIDGET(fixed4),GTK_WIDGET(label3),-1);
security=1;
  return FALSE;
}


gboolean
on_radiobutton6_button_press_event     (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{

GtkWidget * notebook1 = lookup_widget(GTK_WIDGET(user_data), "notebook1");
gint num_pages=0;
num_pages=gtk_notebook_get_n_pages(GTK_NOTEBOOK(notebook1));
if (num_pages==4)
    gtk_notebook_remove_page(GTK_NOTEBOOK(notebook1),-1);
security=0;

  return FALSE;
}


gboolean
on_radiobutton8_button_press_event     (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
GtkWidget * notebook1 = lookup_widget(GTK_WIDGET(user_data), "notebook1");
GtkWidget * fixed = lookup_widget(GTK_WIDGET(user_data), "fixed5");
GtkWidget * label = lookup_widget(GTK_WIDGET(user_data), "label12");
gint num_pages=0;


num_pages=gtk_notebook_get_n_pages(GTK_NOTEBOOK(notebook1));

if (num_pages==4)
    gtk_notebook_remove_page(GTK_NOTEBOOK(notebook1),-1);

gtk_notebook_insert_page(GTK_NOTEBOOK(notebook1),GTK_WIDGET(fixed),GTK_WIDGET(label),-1);
security=2;

  return FALSE;
}


gboolean
on_radiobutton9_button_press_event     (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
GtkWidget * notebook1 = lookup_widget(GTK_WIDGET(user_data), "notebook1");
GtkWidget * fixed = lookup_widget(GTK_WIDGET(user_data), "fixed6");
GtkWidget * label = lookup_widget(GTK_WIDGET(user_data), "label13");
gint num_pages=0;


num_pages=gtk_notebook_get_n_pages(GTK_NOTEBOOK(notebook1));

if (num_pages==4)
    gtk_notebook_remove_page(GTK_NOTEBOOK(notebook1),-1);

gtk_notebook_insert_page(GTK_NOTEBOOK(notebook1),GTK_WIDGET(fixed),GTK_WIDGET(label),-1);
security=3;

  return FALSE;
}


gboolean
on_button1_button_release_event        (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{

    save_settings(user_data);
        
    if (return_cmd[0]=='\0')
	gtk_main_quit();
    else {

	pid_t pID = fork();
	if (pID == 0)              
	{
	    execl(return_cmd,return_cmd,(char *)0);
	}
	else
	{
	    sleep(3);
	    gtk_main_quit();
	}    
    }
	

  return FALSE;
}

void
on_radiobutton5_toggled                (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    int active;
    gchar *value;


    GList *child = gtk_container_children(GTK_CONTAINER(user_data));

    active=gtk_toggle_button_get_active(user_data);
    sprintf(value,"%s",gtk_label_get_text(GTK_LABEL(child->data)));
    if (active) {
	    set_ipmethod(value, user_data);
    }
}


void
on_radiobutton12_toggled               (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    int active;
    gchar *value;


    GList *child = gtk_container_children(GTK_CONTAINER(user_data));

    active=gtk_toggle_button_get_active(user_data);
    sprintf(value,"%s",gtk_label_get_text(GTK_LABEL(child->data)));
    if (active) {
	    set_wpamethod(value, user_data);
    }

}

