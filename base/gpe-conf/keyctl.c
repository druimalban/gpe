#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <pwd.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#define _XOPEN_SOURCE /* Pour GlibC2 */
#include <time.h>
#include "gpe/pixmaps.h"
#include "gpe/render.h"

#include "applets.h"
#include "keyctl.h"

#include <gpe/spacing.h>


char buttons[5][1024];
static struct{

  GtkWidget *button[5];
  GdkPixbuf *p;  
}self;

char * default_keyctl_conf[] = {
  "/usr/bin/record",
  "/usr/bin/schedule",
  "/usr/bin/mingle",
  "/usr/bin/x-terminal-emulator",
  "/usr/bin/fmenu"
};
void FileSelected(char *file, gpointer data);

void init_buttons()
{
    FILE *fd;
    GtkButton *target;
    char buffer[1024];
    char *slash;
    char btext[16];
    int i;

    strcpy(buttons[0],"");

    /* read from configfile and set buttons */
    fd=fopen("/etc/keyctl.conf","r");
    if (fd==NULL) {
	/* defaults */
	for (i=0;i<5;i++) {
	    strcpy(buttons[i],default_keyctl_conf[i]);
	    slash=strrchr(buttons[i],'/')+1;
	    if (slash==NULL) { slash=buttons[i]; }
	    strncpy(btext,slash,15);
	    btext[15]='\x0';
	    target=GTK_BUTTON(self.button[i]);
	    gtk_label_set_text(GTK_LABEL(target->child),btext);
	}
    } else {
	/* load from configfile */
	for (i=0;i<5;i++) {
	    fgets(buffer, 1023, fd);
	    slash=strchr(buffer,'\n');
	    if (slash!=NULL) { *slash='\x0'; }
	    strcpy(buttons[i],buffer);
	    slash=strrchr(buffer,'/')+1;
	    if (slash==NULL) { slash=buffer; }
	    strncpy(btext,slash,15);
	    btext[15]='\x0';
	    target=GTK_BUTTON(self.button[i]);
	    gtk_label_set_text(GTK_LABEL(target->child),btext);
	}
        fclose(fd);
    }
}

#if 1 // an attempt to draw some nice arrows from the pixmaps to the buttons. 
// if anyone is more experienced on gdk and wan to do it..
gboolean expose_event (GtkWidget *widget, GdkEventExpose *event, gpointer data)
{			     

  gdk_draw_line (widget->window,
                widget->style->fg_gc[widget->state],
                184, 182,165, 145 
                 );
  return TRUE;
}
#endif

void
on_button_clicked                      (GtkButton       *button,
                                        gpointer         user_data)
{
    ask_user_a_file("/usr/bin",NULL,FileSelected,NULL,button);
}




GtkWidget *Keyctl_Build_Objects()
{
  guint gpe_border = gpe_get_border ();
  GtkWidget *layout1 = gtk_layout_new (NULL, NULL);
  GtkWidget *button_1 = gtk_button_new_with_label ("Record");
  GtkWidget *button_2 = gtk_button_new_with_label ("Calendar");
  GtkWidget *button_5 = gtk_button_new_with_label ("Menu");
  GtkWidget *button_3 = gtk_button_new_with_label ("Contacts");
  GtkWidget *button_4 = gtk_button_new_with_label ("Q");
  GtkWidget *scroll = gtk_scrolled_window_new(
					      GTK_ADJUSTMENT(gtk_adjustment_new(0,0,230,1,10,240)),
					      GTK_ADJUSTMENT(gtk_adjustment_new(0,0,220,1,10,240)));
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);
  self.p = gpe_find_icon ("ipaq");


  self.button[0]=button_1;
  self.button[1]=button_2;
  self.button[2]=button_3;
  self.button[3]=button_4;
  self.button[4]=button_5;
  
  gtk_widget_show (layout1);
  gtk_container_add (GTK_CONTAINER (scroll), layout1);
  gtk_layout_set_size (GTK_LAYOUT (layout1), 230, 220);

#warning FIXME: this aint right... :)
  /* gtk_container_set_border_width (GTK_CONTAINER (scroll), gpe_border); */
  
  GTK_ADJUSTMENT (GTK_LAYOUT (layout1)->hadjustment)->step_increment = 10;
  GTK_ADJUSTMENT (GTK_LAYOUT (layout1)->vadjustment)->step_increment = 10;

  gtk_widget_show (button_1);
  gtk_layout_put (GTK_LAYOUT (layout1), button_1, 0, 56);
  gtk_widget_set_usize (button_1, 50, 20);

  gtk_widget_show (button_2);
  gtk_layout_put (GTK_LAYOUT (layout1), button_2, 8, 182);
  gtk_widget_set_usize (button_2, 50, 20);

  gtk_widget_show (button_5);
  gtk_layout_put (GTK_LAYOUT (layout1), button_5, 184, 182);
  gtk_widget_set_usize (button_5, 50, 20);

  gtk_widget_show (button_3);
  gtk_layout_put (GTK_LAYOUT (layout1), button_3, 64, 196);
  gtk_widget_set_usize (button_3, 50, 20);

  gtk_widget_show (button_4);
  gtk_layout_put (GTK_LAYOUT (layout1), button_4, 128, 196);
  gtk_widget_set_usize (button_4, 50, 20);

  if(self.p)
    {
      GtkWidget *pixmap1 = gpe_render_icon (wstyle, self.p);
      gtk_widget_show (pixmap1);
      gtk_layout_put (GTK_LAYOUT (layout1), pixmap1, 50, 0);
      gtk_widget_set_usize (pixmap1, 144, 208);


      gtk_signal_connect_after (GTK_OBJECT (pixmap1), "expose_event",GTK_SIGNAL_FUNC (expose_event),NULL);


      
    }
  
  gtk_signal_connect_object (GTK_OBJECT (button_1), "clicked",
                             GTK_SIGNAL_FUNC (on_button_clicked),
                             GTK_OBJECT (button_1));
  gtk_signal_connect_object (GTK_OBJECT (button_2), "clicked",
                             GTK_SIGNAL_FUNC (on_button_clicked),
                             GTK_OBJECT (button_2));
  gtk_signal_connect_object (GTK_OBJECT (button_5), "clicked",
                             GTK_SIGNAL_FUNC (on_button_clicked),
                             GTK_OBJECT (button_5));
  gtk_signal_connect_object (GTK_OBJECT (button_3), "clicked",
                             GTK_SIGNAL_FUNC (on_button_clicked),
                             GTK_OBJECT (button_3));
  gtk_signal_connect_object (GTK_OBJECT (button_4), "clicked",
                             GTK_SIGNAL_FUNC (on_button_clicked),
                             GTK_OBJECT (button_4));
  init_buttons();
  return scroll;

}
void Keyctl_Free_Objects();
void Keyctl_Save()
{
    /* save new config, force keyctl to reload, and exit */
    int i;
    FILE *fd;

    fd=fopen("/etc/keyctl.conf","w");
    if (fd==NULL) {
	printf("ERROR: Can't open /etc/keyctl.conf for writing!\n");
	return;
	//        exit(1);
    }
    for (i=0;i<5;i++) {
#ifdef DEBUG
	printf("button #%d => %s\n",i,buttons[i]);
#endif
        fprintf(fd, "%s\n",buttons[i]);
    }
    fclose(fd);

}
void Keyctl_Restore()
{

  init_buttons();

}


void FileSelected(char *file, gpointer data)
{
  GtkButton *target;
  char btext[16];
  int len;
  char *slash;
  int button_nr;

  target=data;
  for(button_nr = 0; button_nr<5 && self.button[button_nr]!=GTK_WIDGET(target) ;button_nr++);

  if(button_nr==5)
    {
      printf("cant find button\n");
      return;
    }
	

  strncpy(buttons[button_nr],file,1023);
  buttons[button_nr][1023]='\x0';

#ifdef DEBUG
  printf("button %d changed to %s\n",button_nr,file);
#endif
  len=strlen(file);
  slash=strrchr(file,'/')+1;
  if (slash==NULL) { slash=file; }
  strncpy(btext,slash,15);
  btext[15]='\x0';
#ifdef DEBUG
  printf("setting label to %s\n",btext);
#endif
  gtk_label_set_text(GTK_LABEL(target->child),btext);
}
 
