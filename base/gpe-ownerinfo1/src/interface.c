/*
 * DO NOT EDIT THIS FILE - it is generated by Glade.
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include <gpe/pixmaps.h>
#include <gpe/render.h>

#include "callbacks.h"
#include "interface.h"
#include "support.h"

GtkWidget*
create_GPE_Ownerinfo (void)
{
  GtkWidget *GPE_Ownerinfo;
  GtkWidget *frame1;
  GtkWidget *notebook;
  GtkWidget *table1;
  GtkWidget *owner_name_label;
  GtkWidget *name;
  GtkWidget *owner_email_label;
  GtkWidget *email;
  GtkWidget *owner_phone_label;
  GtkWidget *phone;
  GtkWidget *scrolledwindow1;
  GtkWidget *viewport1;
  GtkWidget *address;
  GtkWidget *vbox1;
  GtkWidget *owner_address_label;
  GtkWidget *smallphotobutton;
  GtkWidget *smallphoto;
  GtkWidget *label3;
  GtkWidget *bigphotobutton;
  GtkWidget *bigphoto;
  GtkWidget *label4;
  // GtkStyle* style;
  GtkStyle *style = gtk_style_new();
  GdkColor myGdkColor = {0, 0xAFFF, 0xFFFF, 0xCFFF};
	  
  GPE_Ownerinfo = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_widget_set_name (GPE_Ownerinfo, "GPE_Ownerinfo");
  gtk_object_set_data (GTK_OBJECT (GPE_Ownerinfo), "GPE_Ownerinfo", GPE_Ownerinfo);
  gtk_window_set_title (GTK_WINDOW (GPE_Ownerinfo), _("GPE Owner Info"));
  gtk_window_set_default_size (GTK_WINDOW (GPE_Ownerinfo), 240, 120);
  gtk_window_set_policy (GTK_WINDOW (GPE_Ownerinfo), TRUE, TRUE, TRUE);

  frame1 = gtk_frame_new (_("Owner Information"));
  gtk_widget_set_name (frame1, "frame1");
  gtk_widget_ref (frame1);
  gtk_object_set_data_full (GTK_OBJECT (GPE_Ownerinfo), "frame1", frame1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (frame1);
  gtk_container_add (GTK_CONTAINER (GPE_Ownerinfo), frame1);
  gtk_container_set_border_width (GTK_CONTAINER (frame1), 5);

  notebook = gtk_notebook_new ();
  gtk_widget_set_name (notebook, "notebook");
  gtk_widget_ref (notebook);
  gtk_object_set_data_full (GTK_OBJECT (GPE_Ownerinfo), "notebook", notebook,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (notebook);
  gtk_container_add (GTK_CONTAINER (frame1), notebook);
  GTK_WIDGET_UNSET_FLAGS (notebook, GTK_CAN_FOCUS);
  gtk_notebook_set_show_tabs (GTK_NOTEBOOK (notebook), FALSE);
  gtk_notebook_set_show_border (GTK_NOTEBOOK (notebook), FALSE);

  table1 = gtk_table_new (4, 2, FALSE);
  gtk_widget_set_name (table1, "table1");
  gtk_widget_ref (table1);
  gtk_object_set_data_full (GTK_OBJECT (GPE_Ownerinfo), "table1", table1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (table1);
  gtk_container_add (GTK_CONTAINER (notebook), table1);

  owner_name_label = gtk_label_new (_("Name"));
  gtk_widget_set_name (owner_name_label, "owner_name_label");
  gtk_widget_ref (owner_name_label);
  gtk_object_set_data_full (GTK_OBJECT (GPE_Ownerinfo), "owner_name_label", owner_name_label,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (owner_name_label);
  gtk_table_attach (GTK_TABLE (table1), owner_name_label, 0, 1, 0, 1,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_label_set_justify (GTK_LABEL (owner_name_label), GTK_JUSTIFY_RIGHT);
  gtk_misc_set_alignment (GTK_MISC (owner_name_label), 1, 0.5);
  gtk_misc_set_padding (GTK_MISC (owner_name_label), 5, 0);

  name = gtk_label_new (_("Foo Bar"));
  gtk_widget_set_name (name, "name");
  gtk_widget_ref (name);
  gtk_object_set_data_full (GTK_OBJECT (GPE_Ownerinfo), "name", name,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (name);
  gtk_table_attach (GTK_TABLE (table1), name, 1, 2, 0, 1,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_label_set_justify (GTK_LABEL (name), GTK_JUSTIFY_LEFT);
  gtk_misc_set_alignment (GTK_MISC (name), 0, 0.5);
  gtk_misc_set_padding (GTK_MISC (name), 5, 0);

  owner_email_label = gtk_label_new (_("E-Mail"));
  gtk_widget_set_name (owner_email_label, "owner_email_label");
  gtk_widget_ref (owner_email_label);
  gtk_object_set_data_full (GTK_OBJECT (GPE_Ownerinfo), "owner_email_label", owner_email_label,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (owner_email_label);
  gtk_table_attach (GTK_TABLE (table1), owner_email_label, 0, 1, 1, 2,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_label_set_justify (GTK_LABEL (owner_email_label), GTK_JUSTIFY_RIGHT);
  gtk_misc_set_alignment (GTK_MISC (owner_email_label), 1, 0.5);
  gtk_misc_set_padding (GTK_MISC (owner_email_label), 5, 0);

  email = gtk_label_new (_("nobody@localhost"));
  gtk_widget_set_name (email, "email");
  gtk_widget_ref (email);
  gtk_object_set_data_full (GTK_OBJECT (GPE_Ownerinfo), "email", email,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (email);
  gtk_table_attach (GTK_TABLE (table1), email, 1, 2, 1, 2,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_label_set_justify (GTK_LABEL (email), GTK_JUSTIFY_LEFT);
  gtk_misc_set_alignment (GTK_MISC (email), 0, 0.5);
  gtk_misc_set_padding (GTK_MISC (email), 5, 0);

  owner_phone_label = gtk_label_new (_("Phone"));
  gtk_widget_set_name (owner_phone_label, "owner_phone_label");
  gtk_widget_ref (owner_phone_label);
  gtk_object_set_data_full (GTK_OBJECT (GPE_Ownerinfo), "owner_phone_label", owner_phone_label,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (owner_phone_label);
  gtk_table_attach (GTK_TABLE (table1), owner_phone_label, 0, 1, 2, 3,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_label_set_justify (GTK_LABEL (owner_phone_label), GTK_JUSTIFY_RIGHT);
  gtk_misc_set_alignment (GTK_MISC (owner_phone_label), 1, 0.5);
  gtk_misc_set_padding (GTK_MISC (owner_phone_label), 5, 0);

  phone = gtk_label_new (_("+99 (9999) 999-9999"));
  gtk_widget_set_name (phone, "phone");
  gtk_widget_ref (phone);
  gtk_object_set_data_full (GTK_OBJECT (GPE_Ownerinfo), "phone", phone,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (phone);
  gtk_table_attach (GTK_TABLE (table1), phone, 1, 2, 2, 3,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_label_set_justify (GTK_LABEL (phone), GTK_JUSTIFY_LEFT);
  gtk_misc_set_alignment (GTK_MISC (phone), 0, 0.5);
  gtk_misc_set_padding (GTK_MISC (phone), 5, 0);

  scrolledwindow1 = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_set_name (scrolledwindow1, "scrolledwindow1");
  gtk_widget_ref (scrolledwindow1);
  gtk_object_set_data_full (GTK_OBJECT (GPE_Ownerinfo), "scrolledwindow1", scrolledwindow1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (scrolledwindow1);
  gtk_table_attach (GTK_TABLE (table1), scrolledwindow1, 1, 2, 3, 4,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 0);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow1), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

  viewport1 = gtk_viewport_new (NULL, NULL);
  gtk_widget_set_name (viewport1, "viewport1");
  gtk_widget_ref (viewport1);
  gtk_object_set_data_full (GTK_OBJECT (GPE_Ownerinfo), "viewport1", viewport1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (viewport1);
  gtk_container_add (GTK_CONTAINER (scrolledwindow1), viewport1);
  gtk_viewport_set_shadow_type (GTK_VIEWPORT (viewport1), GTK_SHADOW_NONE);

  address = gtk_label_new (_("Nosuch Lane 42\nX-12345 Quux\nFoobarland"));
  gtk_widget_set_name (address, "address");
  gtk_widget_ref (address);
  gtk_object_set_data_full (GTK_OBJECT (GPE_Ownerinfo), "address", address,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (address);
  gtk_container_add (GTK_CONTAINER (viewport1), address);
  gtk_label_set_justify (GTK_LABEL (address), GTK_JUSTIFY_LEFT);
  gtk_misc_set_alignment (GTK_MISC (address), 0, 0);
  gtk_misc_set_padding (GTK_MISC (address), 5, 0);

  vbox1 = gtk_vbox_new (FALSE, 0);
  gtk_widget_set_name (vbox1, "vbox1");
  gtk_widget_ref (vbox1);
  gtk_object_set_data_full (GTK_OBJECT (GPE_Ownerinfo), "vbox1", vbox1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (vbox1);
  gtk_table_attach (GTK_TABLE (table1), vbox1, 0, 1, 3, 4,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (GTK_FILL), 0, 0);

  owner_address_label = gtk_label_new (_("Address"));
  gtk_widget_set_name (owner_address_label, "owner_address_label");
  gtk_widget_ref (owner_address_label);
  gtk_object_set_data_full (GTK_OBJECT (GPE_Ownerinfo), "owner_address_label", owner_address_label,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (owner_address_label);
  gtk_box_pack_start (GTK_BOX (vbox1), owner_address_label, FALSE, FALSE, 1);
  gtk_label_set_justify (GTK_LABEL (owner_address_label), GTK_JUSTIFY_RIGHT);
  gtk_misc_set_alignment (GTK_MISC (owner_address_label), 1, 0);
  gtk_misc_set_padding (GTK_MISC (owner_address_label), 5, 0);

  smallphotobutton = gtk_button_new ();
  gtk_widget_set_name (smallphotobutton, "smallphotobutton");
  gtk_widget_ref (smallphotobutton);
  gtk_object_set_data_full (GTK_OBJECT (GPE_Ownerinfo), "smallphotobutton", smallphotobutton,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (smallphotobutton);
  gtk_box_pack_end (GTK_BOX (vbox1), smallphotobutton, TRUE, TRUE, 0);
  gtk_button_set_relief (GTK_BUTTON (smallphotobutton), GTK_RELIEF_NONE);

  smallphoto = create_pixmap (GPE_Ownerinfo, "ownerphoto");
  gtk_widget_set_name (smallphoto, "smallphoto");
  gtk_widget_ref (smallphoto);
  gtk_object_set_data_full (GTK_OBJECT (GPE_Ownerinfo), "smallphoto", smallphoto,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (smallphoto);
  gtk_container_add (GTK_CONTAINER (smallphotobutton), smallphoto);

  label3 = gtk_label_new (_("label3"));
  gtk_widget_set_name (label3, "label3");
  gtk_widget_ref (label3);
  gtk_object_set_data_full (GTK_OBJECT (GPE_Ownerinfo), "label3", label3,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label3);
  gtk_notebook_set_tab_label (GTK_NOTEBOOK (notebook), gtk_notebook_get_nth_page (GTK_NOTEBOOK (notebook), 0), label3);

  bigphotobutton = gtk_button_new ();
  gtk_widget_set_name (bigphotobutton, "bigphotobutton");
  gtk_widget_ref (bigphotobutton);
  gtk_object_set_data_full (GTK_OBJECT (GPE_Ownerinfo), "bigphotobutton", bigphotobutton,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (bigphotobutton);
  gtk_container_add (GTK_CONTAINER (notebook), bigphotobutton);
  GTK_WIDGET_UNSET_FLAGS (bigphotobutton, GTK_CAN_FOCUS);
  gtk_button_set_relief (GTK_BUTTON (bigphotobutton), GTK_RELIEF_NONE);

  //  style->bg[GTK_STATE_PRELIGHT] = myGdkColor;
  style->bg[GTK_STATE_PRELIGHT] = style->bg[GTK_STATE_NORMAL];
  style->bg[GTK_STATE_NORMAL] = myGdkColor;
  gtk_widget_set_style (GTK_WIDGET (bigphotobutton), style);
  
  bigphoto = create_pixmap_big (GPE_Ownerinfo, "ownerphoto");
  gtk_widget_set_name (bigphoto, "bigphoto");
  gtk_widget_ref (bigphoto);
  gtk_object_set_data_full (GTK_OBJECT (GPE_Ownerinfo), "bigphoto", bigphoto,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (bigphoto);
  gtk_container_add (GTK_CONTAINER (bigphotobutton), bigphoto);

  label4 = gtk_label_new (_("label4"));
  gtk_widget_set_name (label4, "label4");
  gtk_widget_ref (label4);
  gtk_object_set_data_full (GTK_OBJECT (GPE_Ownerinfo), "label4", label4,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label4);
  gtk_notebook_set_tab_label (GTK_NOTEBOOK (notebook), gtk_notebook_get_nth_page (GTK_NOTEBOOK (notebook), 1), label4);

  gtk_signal_connect (GTK_OBJECT (GPE_Ownerinfo), "destroy",
                      GTK_SIGNAL_FUNC (gtk_main_quit),
                      NULL);
  gtk_signal_connect (GTK_OBJECT (smallphotobutton), "clicked",
                      GTK_SIGNAL_FUNC (on_smallphotobutton_clicked),
                      NULL);
  gtk_signal_connect (GTK_OBJECT (bigphotobutton), "clicked",
                      GTK_SIGNAL_FUNC (on_bigphotobutton_clicked),
                      NULL);

  gtk_widget_grab_focus (smallphotobutton);
  return GPE_Ownerinfo;
}

