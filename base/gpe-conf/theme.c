/*
 * gpe-conf
 *
 * Copyright (C) 2002  Pierre TARDY <tardyp@free.fr>
 *               2003  Florian Boor <florian.boor@kernelconcepts.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <ctype.h>
#include <pwd.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <X11/Xcms.h>
#include <X11/Xlib.h>
#include <libintl.h>
#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE		/* Pour GlibC2 */
#endif
#include <time.h>
#include "applets.h"
#include "theme.h"
#include "widgets/sp-color-slider.h"
#include "widgets/color.h"

#include <gpe/errorbox.h>
#include <gpe/question.h>
#include <gpe/spacing.h>
#include <xsettings-client.h>

#define KEY_MATCHBOX "MATCHBOX/"
#define KEY_THEME "Net/"

static XSettingsClient *client;

static struct
{
  GtkWidget *cbTheme;
  GtkWidget *eImage;
  GtkWidget *rbSolid, *rbGrad, *rbImage;
  GtkWidget *rbImgStr,*rbImgTiled,*rbImgCent;
  GtkWidget *rbH,*rbV;
  GtkWidget *bColor1,*bColor2;
  gchar *themename;
  GtkWidget *bOpen;
  GtkWidget *demolabel;
  GtkWidget *spFS, *bFont, *bColorFont;
}
self;

typedef struct
{
  SPColorSlider *s[3];
  GtkAdjustment *a[3];
  GtkWidget *previewbutton;
}
tcsel;

tcsel *csel;


static char *matchboxpath = "/usr/share/themes";

static GList *themeitems;
static GtkWidget *colorbox;

int updating = FALSE;

typedef struct _mbdesktop_bg {
  int type;

  union {
    char *filename;
    int cols[3];
    int gcols[6];
  } data;

} MBDesktopBG;

MBDesktopBG mb_back;

static void select_font_popup (GtkWidget *parent_button);
static void on_font_size_change(GtkSpinButton *spinbutton,GtkScrollType arg1);
static GtkWidget *popup_menu_button_new (const gchar *stock_id);


gboolean
mbbg_parse_spec(MBDesktopBG *mbbg, char *spec)
{
  /*
  img-stretched:filename>
  img-tiled:<filename>
  col-solid:<color definition>
  col-gradient-vertical:<start color>,<end color>
  col-gradient-horizontal:<start color>,<end color>
  */

  XColor tmpxcol;
  int i, mapping_cnt, spec_offset = 0, type = 0;
  char *bg_def = NULL, *p = NULL;
  Display *dpy;
  Screen *scr;
  Colormap colormap;
  	
  struct conf_mapping_t {
    char *name;
    int   id;
  } conf_mapping[] = {
    { "img-stretched:",           BG_STRETCHED_PXM  },
    { "img-tiled:",               BG_TILED_PXM      },
    { "img-centered:",            BG_CENTERED_PXM   },
    { "col-solid:",               BG_SOLID          },
    { "col-gradient-vertical:",   BG_GRADIENT_VERT  },
    { "col-gradient-horizontal:", BG_GRADIENT_HORIZ },
  };

  dpy = GDK_DISPLAY();
  scr =  gdk_x11_screen_get_xscreen(gdk_screen_get_default());
  colormap = gdk_x11_colormap_get_xcolormap(gdk_screen_get_default_colormap(gdk_screen_get_default()));
  
  mapping_cnt = (sizeof(conf_mapping)/sizeof(struct conf_mapping_t));

  memset(mbbg, 0, sizeof(mbbg));

  for (i=0; i<mapping_cnt; i++)
	{
	  spec_offset = strlen(conf_mapping[i].name);
	  if (spec_offset < strlen(spec)
	      && !strncmp(conf_mapping[i].name, spec, spec_offset))
	    {
	      type = conf_mapping[i].id;
	      break;
	    }
	}

    if (!type)
	{
	  /* Assume we've just been passed an image filename */
	  mbbg->type = BG_STRETCHED_PXM;
	  mbbg->data.filename = strdup(spec);
	  return True;
	} else bg_def = spec + spec_offset;
    

  mbbg->type = type;
  switch(type)
    {
    case BG_SOLID:
      XParseColor(dpy, colormap, 
		  bg_def, &tmpxcol);
      mbbg->data.cols[0] = tmpxcol.red   >> 8;
      mbbg->data.cols[1] = tmpxcol.green >> 8;
      mbbg->data.cols[2] = tmpxcol.blue  >> 8;
      break;
    case BG_TILED_PXM:
    case BG_CENTERED_PXM:
    case BG_STRETCHED_PXM:
      mbbg->data.filename = strdup(bg_def);
      break;
    case BG_GRADIENT_HORIZ:
    case BG_GRADIENT_VERT:
      p = bg_def;
      while(*p != ',' && *p != '\0') p++;
      if (*p == '\0')
	{
	  return False; 	/* XXX need to reset on fail */
	}
      *p = '\0';
      XParseColor(dpy, colormap, 
		  bg_def, &tmpxcol);
      mbbg->data.gcols[0] = (tmpxcol.red   >> 8);
      mbbg->data.gcols[2] = (tmpxcol.green >> 8);
      mbbg->data.gcols[4] = (tmpxcol.blue  >> 8);
      p++;
      XParseColor(dpy, colormap, 
		  p, &tmpxcol);
      mbbg->data.gcols[1] = (tmpxcol.red   >> 8);
      mbbg->data.gcols[3] = (tmpxcol.green >> 8);
      mbbg->data.gcols[5] = (tmpxcol.blue  >> 8);
      break;
    }    

  return True;
}


void
widget_set_color_rgb8 (GtkWidget * widget,float r, float g, float b)
{
//  static GdkColor white = {0,0xffff,0xffff,0xffff};
//  static GdkColor black = {0,0x0000,0x0000,0x0000};
  GtkStyle *astyle;
  GtkRcStyle *rc_style;
  GdkColor gcolor;
  GtkBin bbin;
  float gray;

  bbin =  (GtkBin)(GTK_BUTTON(widget)->bin);
  gray = 0.3*r + 0.59*g + 0.11*b;
  
  if (gray > 0.5)
  {
	  gtk_label_set_markup(GTK_LABEL(bbin.child),g_strdup_printf("<span foreground=\"#000000\"> %s </span>",_("Color")));
  }
  else
  {
	  gtk_label_set_markup(GTK_LABEL(bbin.child),g_strdup_printf("<span foreground=\"#ffffff\"> %s </span>",_("Color")));

  }
  gcolor.pixel = 0;
  gcolor.red = (int)(r*65534.0);
  gcolor.green = (int)(g*65534.0);
  gcolor.blue = (int)(b*65534.0);
	
  astyle = gtk_widget_get_style (widget);
  rc_style = gtk_rc_style_new ();
  if (astyle)
    {
      rc_style->base[GTK_STATE_NORMAL] = gcolor;
      rc_style->fg[GTK_STATE_NORMAL] = gcolor;
      rc_style->bg[GTK_STATE_NORMAL] = gcolor;
      rc_style->color_flags[GTK_STATE_NORMAL] |= GTK_RC_FG;
      rc_style->color_flags[GTK_STATE_NORMAL] |= GTK_RC_BASE;
      rc_style->color_flags[GTK_STATE_NORMAL] |= GTK_RC_BG;
//      rc_style->text[GTK_STATE_NORMAL] = xcolor;
//      rc_style->color_flags[GTK_STATE_NORMAL] |= GTK_RC_TEXT;
		
      gtk_widget_modify_style (widget, rc_style);
    }
  gtk_widget_ensure_style (widget);
}

//--------------------------
static void
sp_color_selector_update_sliders (SPColorSlider * csel1, guint channels)
{
  float r,g,b;
  if (updating)
    return;
  updating = TRUE;
  if ((channels != 3) && (channels != 2))
    {
      /* Update red */
      sp_color_slider_set_colors (SP_COLOR_SLIDER (csel->s[0]),
				  SP_RGBA32_F_COMPOSE (0.0, csel->a[1]->value,
						       csel->a[2]->value,
						       1.0),
				  SP_RGBA32_F_COMPOSE (1.0, csel->a[1]->value,
						       csel->a[2]->value,
						       1.0));
    }
  if ((channels != 3) && (channels != 1))
    {
      /* Update green */
      sp_color_slider_set_colors (SP_COLOR_SLIDER (csel->s[1]),
				  SP_RGBA32_F_COMPOSE (csel->a[0]->value, 0.0,
						       csel->a[2]->value,
						       1.0),
				  SP_RGBA32_F_COMPOSE (csel->a[0]->value, 1.0,
						       csel->a[2]->value,
						       1.0));
    }
  if ((channels != 1) && (channels != 2))
    {
      /* Update blue */
      sp_color_slider_set_colors (SP_COLOR_SLIDER (csel->s[2]),
				  SP_RGBA32_F_COMPOSE (csel->a[0]->value,
						       csel->a[1]->value, 0.0,
						       1.0),
				  SP_RGBA32_F_COMPOSE (csel->a[0]->value,
						       csel->a[1]->value, 1.0,
						       1.0));
    }
	
  r = csel->a[0]->value;
  g = csel->a[1]->value;
  b = csel->a[2]->value;
	
  widget_set_color_rgb8(csel->previewbutton,r,g,b);
  updating = FALSE;
}


//#--------------------------

/*******************/
/*   init stuff    */
/*******************/

void update_enabled_widgets()
{
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(self.rbSolid)))
	{
		gtk_widget_set_sensitive(self.bColor1,TRUE);
		gtk_widget_set_sensitive(self.bColor2,FALSE);
		gtk_widget_set_sensitive(self.eImage,FALSE);
		gtk_widget_set_sensitive(self.bOpen,FALSE);
		gtk_widget_set_sensitive(self.rbH,FALSE);
		gtk_widget_set_sensitive(self.rbV,FALSE);
		gtk_widget_set_sensitive(self.rbImgCent,FALSE);
		gtk_widget_set_sensitive(self.rbImgStr,FALSE);
		gtk_widget_set_sensitive(self.rbImgTiled,FALSE);
	}
	else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(self.rbGrad)))
	{
		gtk_widget_set_sensitive(self.bColor1,TRUE);
		gtk_widget_set_sensitive(self.bColor2,TRUE);
		gtk_widget_set_sensitive(self.eImage,FALSE);
		gtk_widget_set_sensitive(self.bOpen,FALSE);
		gtk_widget_set_sensitive(self.rbH,TRUE);
		gtk_widget_set_sensitive(self.rbV,TRUE);
		gtk_widget_set_sensitive(self.rbImgCent,FALSE);
		gtk_widget_set_sensitive(self.rbImgStr,FALSE);
		gtk_widget_set_sensitive(self.rbImgTiled,FALSE);
	}
	else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(self.rbImage)))
	{
		gtk_widget_set_sensitive(self.bColor1,FALSE);
		gtk_widget_set_sensitive(self.bColor2,FALSE);
		gtk_widget_set_sensitive(self.eImage,TRUE);
		gtk_widget_set_sensitive(self.bOpen,TRUE);
		gtk_widget_set_sensitive(self.rbH,FALSE);
		gtk_widget_set_sensitive(self.rbV,FALSE);
		gtk_widget_set_sensitive(self.rbImgCent,TRUE);
		gtk_widget_set_sensitive(self.rbImgStr,TRUE);
		gtk_widget_set_sensitive(self.rbImgTiled,TRUE);
	}
}

GtkWidget *
build_colorbox ()
{
  guint gpe_border = gpe_get_border ();
  guint gpe_boxspacing = gpe_get_boxspacing ();
  gchar *tstr;
  int i;
  GtkWidget *label, *button;
  GtkAttachOptions table_attach_left_col_x;
  GtkAttachOptions table_attach_left_col_y;
  GtkAttachOptions table_attach_right_col_x;
  GtkAttachOptions table_attach_right_col_y;
  GtkJustification table_justify_left_col;
  GtkJustification table_justify_right_col;

  table_attach_left_col_x = GTK_FILL;
  table_attach_left_col_y = 0;
  table_attach_right_col_x = GTK_SHRINK | GTK_EXPAND | GTK_FILL;
  table_attach_right_col_y = GTK_FILL;

  table_justify_left_col = GTK_JUSTIFY_LEFT;
  table_justify_right_col = GTK_JUSTIFY_RIGHT;

  colorbox = gtk_table_new (3, 3, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (colorbox), gpe_border);
  gtk_table_set_row_spacings (GTK_TABLE (colorbox), 0);
  gtk_table_set_col_spacings (GTK_TABLE (colorbox), gpe_boxspacing);
  gtk_widget_show (colorbox);

	button = gtk_button_new();
	csel->previewbutton = button;
    gtk_widget_set_size_request(button,30,30+6*gpe_boxspacing);
    gtk_table_attach (GTK_TABLE (colorbox), button, 2, 3, 0, 3,
			(GtkAttachOptions) (table_attach_left_col_x),
			(GtkAttachOptions) (table_attach_left_col_y), 0, 0);
    gtk_widget_show(button);

  for (i = 0; i < 3; i++)
    {
      label = gtk_label_new (NULL);
      gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
      gtk_widget_show (label);
      switch (i)
	{
	case 0:
	  tstr = g_strdup_printf ("<b>%s</b>", _("Red"));
	  gtk_label_set_markup (GTK_LABEL (label), tstr);
	  g_free (tstr);
	  break;
	case 1:
	  tstr = g_strdup_printf ("<b>%s</b>", _("Green"));
	  gtk_label_set_markup (GTK_LABEL (label), tstr);
	  g_free (tstr);
	  break;
	case 2:
	  tstr = g_strdup_printf ("<b>%s</b>", _("Blue"));
	  gtk_label_set_markup (GTK_LABEL (label), tstr);
	  g_free (tstr);
	  break;
	}
      gtk_table_attach (GTK_TABLE (colorbox), label, 0, 1, i, i + 1,
			(GtkAttachOptions) (table_attach_left_col_x),
			(GtkAttachOptions) (table_attach_left_col_y), 0, 0);
      csel->a[i] = GTK_ADJUSTMENT(gtk_adjustment_new (0.0, 0.0, 255.0, 1.0, 1.0, 10.0));
      label = sp_color_slider_new (csel->a[i]);
      gtk_widget_show (label);
      csel->s[i] = (SPColorSlider*)label;
      gtk_signal_connect (GTK_OBJECT (csel->s[i]), "changed",
			  GTK_SIGNAL_FUNC (sp_color_selector_update_sliders),
			  NULL);

      sp_color_slider_set_map ((SPColorSlider*)label, NULL);
      sp_color_slider_set_colors ((SPColorSlider*)label, 1.0, 65534.0);
      gtk_table_attach (GTK_TABLE (colorbox), label, 1, 2, i, i + 1,
			(GtkAttachOptions) (table_attach_left_col_x),
			(GtkAttachOptions) (table_attach_left_col_y), 0, 0);	    
    }
	
  return colorbox;
}

static void
notify_func (const char *name,
	     XSettingsAction action,
	     XSettingsSetting * setting, void *cb_data)
{
  GtkRcStyle *astyle;
  GtkWidget *label;
	
   if (strncmp (name, KEY_THEME, strlen (KEY_THEME)) == 0)
    {
      char *p = (char *) name + strlen (KEY_THEME);

      if (!strcmp (p, "ThemeName"))
	{
	  if (setting->type == XSETTINGS_TYPE_STRING)
	    {
	      if (self.themename == NULL) self.themename = g_strdup (setting->data.v_string);
	      gtk_entry_set_text (GTK_ENTRY (GTK_COMBO (self.cbTheme)->entry),
				  setting->data.v_string);
	    }
	}
    }
  if (strncmp (name, KEY_MATCHBOX, strlen (KEY_MATCHBOX)) == 0)
    {
      char *p = (char *) name + strlen (KEY_MATCHBOX);

      if (!strcmp (p, "Background"))
	{
	  if (setting->type == XSETTINGS_TYPE_STRING)
	    {
			mbbg_parse_spec(&mb_back,g_strdup(setting->data.v_string));
			  switch(mb_back.type)
				{
				case BG_SOLID:
				  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(self.rbSolid),TRUE);
				  widget_set_color_rgb8(self.bColor1,(float)mb_back.data.cols[0]/255.0,(float)mb_back.data.cols[1]/255.0,(float)mb_back.data.cols[2]/255.0);
				  break;
				case BG_TILED_PXM:
				  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(self.rbImgTiled),TRUE);
				  gtk_entry_set_text(GTK_ENTRY(self.eImage),mb_back.data.filename);
				  break;
   				case BG_CENTERED_PXM:
 				  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(self.rbImgCent),TRUE);
				  gtk_entry_set_text(GTK_ENTRY(self.eImage),mb_back.data.filename);
				  break;
				case BG_STRETCHED_PXM:
 				  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(self.rbImgStr),TRUE);
				  gtk_entry_set_text(GTK_ENTRY(self.eImage),mb_back.data.filename);
				  break;
				case BG_GRADIENT_HORIZ:
				  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(self.rbH),TRUE);
				  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(self.rbGrad),TRUE);
				  widget_set_color_rgb8(self.bColor1,(float)mb_back.data.cols[0]/255.0,(float)mb_back.data.cols[2]/255.0,(float)mb_back.data.cols[4]/255.0);
				  widget_set_color_rgb8(self.bColor2,(float)mb_back.data.cols[1]/255.0,(float)mb_back.data.cols[3]/255.0,(float)mb_back.data.cols[5]/255.0);
				  break;
				case BG_GRADIENT_VERT:
				  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(self.rbV),TRUE);
				  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(self.rbGrad),TRUE);
				  widget_set_color_rgb8(self.bColor1,(float)mb_back.data.cols[0]/255.0,(float)mb_back.data.cols[2]/255.0,(float)mb_back.data.cols[4]/255.0);
				  widget_set_color_rgb8(self.bColor2,(float)mb_back.data.cols[1]/255.0,(float)mb_back.data.cols[3]/255.0,(float)mb_back.data.cols[5]/255.0);
				  break;
				}    
			}
 	    update_enabled_widgets();			
	}
      
	if (!strcmp (p, "DesktopFont"))
	{
	  if (setting->type == XSETTINGS_TYPE_STRING)
	    {
		   char* spos = NULL;
			
           astyle = gtk_rc_style_new ();
		   spos = strrchr(setting->data.v_string,'-'); // this should be the size divider
		   if ((spos != NULL) && strlen(spos) && isdigit(spos[1]))
			  spos[0] = ' ';		   
           astyle->font_desc = pango_font_description_from_string (setting->data.v_string);
           gtk_widget_modify_style (self.demolabel, astyle);
		   gtk_spin_button_set_value(GTK_SPIN_BUTTON(self.spFS),(float)pango_font_description_get_size(astyle->font_desc)/PANGO_SCALE);
		   label = g_object_get_data (G_OBJECT (self.bFont), "label");
		   gtk_label_set_text(GTK_LABEL(label),pango_font_description_get_family(astyle->font_desc));	
		}
	}
	
	if (!strcmp (p, "DesktopFontColor"))
	{
	  if (setting->type == XSETTINGS_TYPE_STRING)
	    {
           Display *dpy;
           Colormap colormap;
           XColor tmpxcol;
			
           dpy = GDK_DISPLAY();
           colormap = gdk_x11_colormap_get_xcolormap(gdk_screen_get_default_colormap(gdk_screen_get_default()));
		   XParseColor(dpy, colormap,setting->data.v_string, &tmpxcol);
           widget_set_color_rgb8(self.bColorFont,(float)(tmpxcol.red >> 8)/255.0,(float)(tmpxcol.green >> 8)/255.0,(float)(tmpxcol.blue >> 8)/255.0);
		}
	}
   }
 }

static GdkFilterReturn
xsettings_event_filter (GdkXEvent * xevp, GdkEvent * ev, gpointer p)
{
  if (xsettings_client_process_event (client, (XEvent *) xevp))
    return GDK_FILTER_REMOVE;

  return GDK_FILTER_CONTINUE;
}


static void
watch_func (Window window, Bool is_start, long mask, void *cb_data)
{
  GdkWindow *gdkwin;

  gdkwin = gdk_window_lookup (window);

  if (is_start)
    {
      if (!gdkwin)
	gdkwin = gdk_window_foreign_new (window);
      else
	g_object_ref (gdkwin);

      gdk_window_add_filter (gdkwin, xsettings_event_filter, NULL);
    }
  else
    {
      g_assert (gdkwin);
      g_object_unref (gdkwin);
      gdk_window_remove_filter (gdkwin, xsettings_event_filter, NULL);
    }
}

gboolean
mb_start_xsettings (void)
{
  Display *dpy = GDK_DISPLAY ();

  client =
    xsettings_client_new (dpy, DefaultScreen (dpy), notify_func, watch_func,
			  NULL);
  if (client == NULL)
    {
      fprintf (stderr, "Cannot create XSettings client.\n");
      return FALSE;
    }

  return TRUE;
}


/*******************/
/*  Changing stuff */
/*******************/

void
on_matchbox_entry_changed (GtkWidget * menu, gpointer user_data)
{
  gchar *tn = NULL;
  tn =
    gtk_editable_get_chars (GTK_EDITABLE (GTK_COMBO (self.cbTheme)->entry), 0,
			    -1);
  if ((tn) && (strlen (tn)))
    {
      system_printf ("xst write %s%s str %s", KEY_THEME, "ThemeName", tn);
    }
  if (tn)
    g_free (tn);
}

void
dialog_color_response (GtkDialog * dialog, gint response, gpointer caller)
{
  float r,g,b;
  if (response == GTK_RESPONSE_ACCEPT)
    {
      // grab color here
  	  r = csel->a[0]->value;
      g = csel->a[1]->value;
      b = csel->a[2]->value;
      widget_set_color_rgb8(caller,r,g,b);
    }

  gtk_container_remove (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), colorbox);
  gtk_widget_destroy (GTK_WIDGET (dialog));
}


void
on_color_select (GtkWidget * widget, GdkEvent * event)
{
  GtkWidget *w;
  GtkStyle *astyle = gtk_widget_get_style(widget);

  w = gtk_dialog_new_with_buttons (_("Select color"), GTK_WINDOW (mainw),
				   GTK_DIALOG_MODAL |
				   GTK_DIALOG_DESTROY_WITH_PARENT,
				   GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
				   GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT,
				   NULL);

  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (w)->vbox), build_colorbox ());
	
  // pass calling widget to handler
  g_signal_connect (GTK_OBJECT (w), "response",
		    (void *) dialog_color_response, (gpointer) widget);

  gtk_adjustment_set_value(csel->a[0],(float)astyle->base[GTK_STATE_NORMAL].red/65534.0);
  gtk_adjustment_set_value(csel->a[1],(float)astyle->base[GTK_STATE_NORMAL].green/65534.0);
  gtk_adjustment_set_value(csel->a[2],(float)astyle->base[GTK_STATE_NORMAL].blue/65534.0);
  gtk_signal_emit_by_name (GTK_OBJECT (csel->s[0]), "changed");

  gtk_dialog_run (GTK_DIALOG (w));
}


static void
File_Selected (char *file, gpointer data)
{
  if (access(file,R_OK))
	gpe_error_box(_("You don't have read access\nto selected background image!"));
  else
	gtk_entry_set_text (GTK_ENTRY (self.eImage), file);
}

void
choose_file (GtkWidget * button, gpointer user_data)
{
  ask_user_a_file (getenv ("HOME"), NULL, File_Selected, NULL, NULL);
}

char* get_color_from_widget(GtkWidget* w)
{
  GdkColor acolor;
  GtkStyle* astyle;
  int r,g,b;
  gchar *result;
	
  astyle = gtk_widget_get_style (w);
  acolor = astyle->base[GTK_STATE_NORMAL];
  r = acolor.red >> 8;
  g = acolor.green >> 8;
  b = acolor.blue >> 8;
  result = g_strdup_printf("#%02x%02x%02x",r,g,b);
  return result;
}


void
Theme_Save ()
{
	char *confstr = NULL;
	const char* clabel;
	char *par1,*par2;
	int fs;
	GtkWidget* label;
	
    /* background settings */
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(self.rbSolid)))
	{
		par1 = get_color_from_widget(self.bColor1);
		confstr = g_strdup_printf("col-solid:%s",par1);
		free(par1);
	}
	else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(self.rbGrad)))
	{
		par1 = get_color_from_widget(self.bColor1);
		par2 = get_color_from_widget(self.bColor2);
		if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(self.rbV)))
			confstr = g_strdup_printf("col-gradient-vertical:%s,%s",par1,par2);
		else
			confstr = g_strdup_printf("col-gradient-horizontal:%s,%s",par1,par2);
		free(par1);
		free(par2);		
	}
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(self.rbImage)))
	{
		par1 = gtk_editable_get_chars(GTK_EDITABLE(self.eImage),0,-1);
		if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(self.rbImgCent)))
			confstr = g_strdup_printf("img-centered:%s",par1);
		else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(self.rbImgStr)))
			confstr = g_strdup_printf("img-stretched:%s",par1);
		else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(self.rbImgTiled)))
			confstr = g_strdup_printf("img-tiled:%s",par1);
		if (access(par1,R_OK))
		{
			gpe_error_box(_("You don't have read access\nto selected background image!"));
			free(confstr);
			confstr = NULL;
		}
		free(par1);
	}
  	
	if (confstr)
	{
		printf ("xst write %s%s str %s\n", KEY_MATCHBOX, "Background", confstr);
		system_printf ("xst write %s%s str %s", KEY_MATCHBOX, "Background", confstr);
		free(confstr);
	}

	/* font type and size */
	label = g_object_get_data (G_OBJECT (self.bFont), "label");
	clabel = gtk_label_get_text(GTK_LABEL(label));
	fs = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(self.spFS));
	confstr = g_strdup_printf("xst write %s%s str \"%s-%i\"", KEY_MATCHBOX, "DesktopFont", clabel,fs);
	printf ("xst write %s%s str \"%s-%i\"\n", KEY_MATCHBOX, "DesktopFont", clabel,fs);
	system(confstr);
	free(confstr);
	/* font color */
	confstr = get_color_from_widget(self.bColorFont);
	printf ("xst write %s%s str \"%s\"\n", KEY_MATCHBOX, "DesktopFontColor", confstr);
	system_printf ("xst write %s%s str \"%s\"", KEY_MATCHBOX, "DesktopFontColor", confstr);
	free(confstr);
}

void
Theme_Restore ()
{
	system_printf ("xst write %s%s str %s", KEY_THEME, "ThemeName", self.themename);
}

/****************/
/*  interface   */
/****************/

GtkWidget *
Theme_Build_Objects ()
{
  GtkWidget *table, *rg_background, *rg_hv, *rg_img;
  GtkWidget *label;
  GtkWidget *hbox;
  GtkWidget *notebook;
  GtkAttachOptions table_attach_left_col_x;
  GtkAttachOptions table_attach_left_col_y;
  GtkAttachOptions table_attach_right_col_x;
  GtkAttachOptions table_attach_right_col_y;
  GtkJustification table_justify_left_col;
  GtkJustification table_justify_right_col;
  guint gpe_border = gpe_get_border ();
  guint gpe_boxspacing = gpe_get_boxspacing ();
  gchar *tstr; 
  GtkRcStyle* astyle;

  table_attach_left_col_x = GTK_FILL;
  table_attach_left_col_y = 0;
  table_attach_right_col_x = GTK_SHRINK | GTK_EXPAND | GTK_FILL;
  table_attach_right_col_y = GTK_FILL;

  table_justify_left_col = GTK_JUSTIFY_LEFT;
  table_justify_right_col = GTK_JUSTIFY_RIGHT;

  csel = malloc (sizeof (tcsel));
  self.themename = NULL;
/* ------------------------------------------------------------------------ */

  notebook  = gtk_notebook_new();
  
  label = gtk_label_new (_("Theme"));
  table = gtk_table_new (3, 2, FALSE);
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook),table,label);
  gtk_container_set_border_width (GTK_CONTAINER (table), gpe_border);
  gtk_table_set_row_spacings (GTK_TABLE (table), gpe_boxspacing);
  gtk_table_set_col_spacings (GTK_TABLE (table), gpe_boxspacing);

  label = gtk_label_new("none");
  gtk_misc_set_alignment(GTK_MISC(label),0.0,0.5);
  tstr = g_strdup_printf ("<b>%s</b>", _("Matchbox Theme"));
  gtk_label_set_markup (GTK_LABEL (label), tstr);
  g_free (tstr);
  gtk_table_attach (GTK_TABLE (table), label, 0, 2, 0, 1,
		    (GtkAttachOptions) (table_attach_left_col_x),
		    (GtkAttachOptions) (table_attach_left_col_y), 0, 0);
  
  hbox = gtk_hbox_new (FALSE, gpe_boxspacing);
  gtk_table_attach (GTK_TABLE (table), hbox, 0, 1, 1, 2,
		    (GtkAttachOptions) (table_attach_left_col_x),
		    (GtkAttachOptions) (table_attach_left_col_y), 0, 0);
  
  label = gtk_label_new (_("Name"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);

  self.cbTheme = gtk_combo_new ();
  themeitems = make_items_from_dir (matchboxpath);
  gtk_combo_set_popdown_strings (GTK_COMBO (self.cbTheme), themeitems);
  gtk_combo_set_value_in_list (GTK_COMBO (self.cbTheme), TRUE, FALSE);
  gtk_widget_set_size_request (self.cbTheme, 120, -1);
  gtk_box_pack_start (GTK_BOX (hbox), self.cbTheme, FALSE, TRUE, 0);

  gtk_signal_connect (GTK_OBJECT (GTK_COMBO (self.cbTheme)->entry), "changed",
		      GTK_SIGNAL_FUNC (on_matchbox_entry_changed), NULL);
 /*---------------------------------------------*/

  label = gtk_label_new (_("Background"));
  table = gtk_table_new (3, 2, FALSE);
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook),table,label);

  gtk_widget_set_name (table, "table");
  gtk_container_set_border_width (GTK_CONTAINER (table), gpe_border);
  gtk_table_set_row_spacings (GTK_TABLE (table), gpe_boxspacing);
  gtk_table_set_col_spacings (GTK_TABLE (table), gpe_boxspacing);

  label = gtk_label_new(NULL);
  gtk_misc_set_alignment(GTK_MISC(label),0.0,0.5);
  tstr = g_strdup_printf ("<b>%s</b>", _("Background Settings"));
  gtk_label_set_markup (GTK_LABEL (label), tstr);
  g_free (tstr);
  gtk_table_attach (GTK_TABLE (table), label, 0, 2, 0, 1,
		    (GtkAttachOptions) (table_attach_left_col_x),
		    (GtkAttachOptions) (table_attach_left_col_y), 0, 0);

  label = gtk_radio_button_new_with_label (NULL, _("Solid color"));
  rg_background = label;
  self.rbSolid = label;
  g_signal_connect (GTK_OBJECT (label), "toggled",
		       (update_enabled_widgets), NULL);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2,
		    (GtkAttachOptions) (table_attach_left_col_x),
		    (GtkAttachOptions) (table_attach_left_col_y), 0, 0);
			
  // box for radio buttons
  hbox = gtk_hbox_new(FALSE,gpe_boxspacing);
  gtk_table_attach (GTK_TABLE (table), hbox, 0, 3, 5, 6,
		    (GtkAttachOptions) (table_attach_left_col_x),
		    (GtkAttachOptions) (table_attach_left_col_y), 0, 0);
  label = gtk_radio_button_new_with_label (NULL, _("Horizontal"));
  rg_hv = label;
  self.rbH = label;
  gtk_box_pack_start(GTK_BOX(hbox),label,FALSE,TRUE,0);
  label = gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON(rg_hv), _("Vertical"));
  self.rbV = label;
  gtk_box_pack_start(GTK_BOX(hbox),label,FALSE,TRUE,0);
  
  self.bColor1 = gtk_button_new();
  gtk_button_set_label(GTK_BUTTON(self.bColor1),_("Color"));
  //gtk_button_new_from_stock (GTK_STOCK_SELECT_COLOR);
  g_signal_connect (GTK_OBJECT (self.bColor1), "clicked",
		    G_CALLBACK (on_color_select), NULL);
  gtk_table_attach (GTK_TABLE (table), self.bColor1, 1, 2, 1, 2,
		    (GtkAttachOptions) (table_attach_left_col_x),
		    (GtkAttachOptions) (table_attach_left_col_y), 0, 0);

  label =
    gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON
						 (rg_background),
						 _("Color gradient"));
  g_signal_connect (GTK_OBJECT (label), "toggled",
		       (update_enabled_widgets), NULL);
  self.rbGrad = label;
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 2, 3,
		    (GtkAttachOptions) (table_attach_left_col_x),
		    (GtkAttachOptions) (table_attach_left_col_y), 0, 0);

  self.bColor2 = gtk_button_new();
  gtk_button_set_label(GTK_BUTTON(self.bColor2),_("Color"));
  g_signal_connect (GTK_OBJECT (self.bColor2), "clicked",
		    G_CALLBACK (on_color_select), NULL);
  //gtk_button_new_from_stock (GTK_STOCK_SELECT_COLOR);
  gtk_table_attach (GTK_TABLE (table), self.bColor2, 1, 2, 2, 3,
		    (GtkAttachOptions) (table_attach_left_col_x),
		    (GtkAttachOptions) (table_attach_left_col_y), 0, 0);


  label =
    gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON
						 (rg_background), _("Image"));
  g_signal_connect (GTK_OBJECT (label), "toggled",
		       (update_enabled_widgets), NULL);
  self.rbImage = label;
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 6, 7,
		    (GtkAttachOptions) (table_attach_left_col_x),
		    (GtkAttachOptions) (table_attach_left_col_y), 0, 0);

  self.eImage = gtk_entry_new ();
  gtk_table_attach (GTK_TABLE (table), self.eImage, 0, 1, 7, 8,
		    (GtkAttachOptions) (table_attach_left_col_x),
		    (GtkAttachOptions) (table_attach_left_col_y), 0, 0);
  gtk_widget_set_size_request (self.eImage, 150, -1);

  self.bOpen = gtk_button_new_from_stock (GTK_STOCK_OPEN);
  gtk_signal_connect (GTK_OBJECT (self.bOpen), "pressed",
		      GTK_SIGNAL_FUNC (choose_file), NULL);
  gtk_table_attach (GTK_TABLE (table), self.bOpen, 1, 2, 7, 8,
		    (GtkAttachOptions) (table_attach_left_col_x),
		    (GtkAttachOptions) (table_attach_left_col_y), 0, 0);

  hbox = gtk_hbox_new(False,gpe_boxspacing);
  gtk_table_attach (GTK_TABLE (table), hbox, 0, 3, 8, 9,
		    (GtkAttachOptions) (table_attach_left_col_x),
		    (GtkAttachOptions) (table_attach_left_col_y), 0, 0);
			
  label = gtk_radio_button_new_with_label (NULL, _("Centered"));
  rg_img = label;
  self.rbImgCent = label;
  gtk_box_pack_start(GTK_BOX(hbox),label,FALSE,TRUE,0);
  label = gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON(rg_img), _("Tiled"));
  self.rbImgTiled = label;
  gtk_box_pack_start(GTK_BOX(hbox),label,FALSE,TRUE,0);
  label = gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON(rg_img), _("Stretched"));
  self.rbImgStr = label;
  gtk_box_pack_start(GTK_BOX(hbox),label,FALSE,TRUE,0);
			
 /*---------------------------------------------*/
  label = gtk_label_new (_("Font"));
  table = gtk_table_new (2, 2, FALSE);
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook),table,label);
  gtk_container_set_border_width (GTK_CONTAINER (table), gpe_border);
  gtk_table_set_row_spacings (GTK_TABLE (table), gpe_boxspacing);
  gtk_table_set_col_spacings (GTK_TABLE (table), gpe_boxspacing);
 
  label = gtk_label_new("none");
  gtk_misc_set_alignment(GTK_MISC(label),0.0,0.5);
  tstr = g_strdup_printf ("<b>%s</b>", _("Desktop Font"));
  gtk_label_set_markup (GTK_LABEL (label), tstr);
  g_free (tstr);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1,
		    (GtkAttachOptions) (table_attach_left_col_x),
		    (GtkAttachOptions) (table_attach_left_col_y), 0, 0);

  label = gtk_label_new(_("Family"));
  gtk_misc_set_alignment(GTK_MISC(label),0.0,0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2,
		    (GtkAttachOptions) (table_attach_left_col_x),
		    (GtkAttachOptions) (table_attach_left_col_y), 0, 0);

  self.bFont = GTK_WIDGET(popup_menu_button_new (GTK_STOCK_SELECT_FONT));
  g_object_set_data (G_OBJECT (self.bFont), "active", FALSE);
  gtk_signal_connect (GTK_OBJECT (self.bFont), "pressed",
		      GTK_SIGNAL_FUNC (select_font_popup), NULL);
  gtk_table_attach (GTK_TABLE (table), self.bFont, 0, 3, 2, 3,
		    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
		    (GtkAttachOptions) (table_attach_left_col_y), 0, 0);
  self.demolabel = gtk_label_new(_("GPE rulez!"));  
  gtk_table_attach (GTK_TABLE (table), self.demolabel, 0, 3, 5, 6,
		    (GtkAttachOptions) (table_attach_left_col_x),
		    (GtkAttachOptions) (table_attach_left_col_y), 0, 0);
  
  label = gtk_label_new(_("Size"));
  gtk_misc_set_alignment(GTK_MISC(label),0.0,0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 3, 4,
		    (GtkAttachOptions) (table_attach_left_col_x),
		    (GtkAttachOptions) (table_attach_left_col_y), 0, 0);
  self.spFS = gtk_spin_button_new_with_range(1,30,1);
  gtk_table_attach (GTK_TABLE (table), self.spFS, 0, 1, 4, 5,
		    (GtkAttachOptions) (table_attach_left_col_x),
		    (GtkAttachOptions) (table_attach_left_col_y), 0, 0);
  g_signal_connect(G_OBJECT(self.spFS),"value-changed",G_CALLBACK(on_font_size_change),NULL);	
  gtk_widget_set_size_request(self.spFS,20,-1);
  self.bColorFont = gtk_button_new();
  gtk_button_set_label(GTK_BUTTON(self.bColorFont),_("Color"));
  g_signal_connect (GTK_OBJECT (self.bColorFont), "clicked",
		    G_CALLBACK (on_color_select), NULL);
  gtk_table_attach (GTK_TABLE (table), self.bColorFont, 2, 3, 4, 5,
		    (GtkAttachOptions) (table_attach_left_col_x),
		    (GtkAttachOptions) (table_attach_left_col_y), 0, 0);
 gtk_widget_set_size_request(self.bColorFont,20,-1);
  
/* insert some defaults */
  astyle = gtk_rc_style_new ();
  astyle->font_desc = pango_font_description_from_string ("Sans 9");
  gtk_widget_modify_style (self.demolabel, astyle);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(self.spFS),(float)9.0);
  label = g_object_get_data (G_OBJECT (self.bFont), "label");
  gtk_label_set_text(GTK_LABEL(label),"Sans");	
  
  mb_start_xsettings ();

  return notebook;
}

//------font stuff------------from gpe-word
static GtkWidget *
popup_menu_button_new (const gchar *stock_id)
{
  GtkWidget *button, *arrow, *hbox, *image, *label;
  GtkRequisition requisition;
  gint width = 0, height;

  button = gtk_button_new ();
  arrow = gtk_arrow_new (GTK_ARROW_DOWN, GTK_SHADOW_NONE);
  hbox = gtk_hbox_new (FALSE, 0);
  image = gtk_image_new_from_stock (stock_id, GTK_ICON_SIZE_SMALL_TOOLBAR);
  label = gtk_label_new(NULL);

  gtk_box_set_homogeneous (GTK_BOX (hbox), FALSE);

  g_object_set_data (G_OBJECT (button), "active", FALSE);
  g_object_set_data (G_OBJECT (button), "hbox", hbox);
  g_object_set_data (G_OBJECT (button), "image", image);
  g_object_set_data (G_OBJECT (button), "arrow", arrow);
  g_object_set_data (G_OBJECT (button), "label", label);

  gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), arrow, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

  gtk_widget_size_request (image, &requisition);
  width = width + requisition.width;
  height = requisition.height;

  gtk_widget_size_request (arrow, &requisition);
  width = (width + requisition.width) - 5;

  gtk_widget_set_size_request (hbox, width, height);
  gtk_container_add (GTK_CONTAINER (button), hbox);
  gtk_widget_show_all (button);

  return button;
}


static void
on_font_select(GtkWidget * widget, gpointer style)
{
	GtkWidget * label;
	gtk_widget_modify_style(self.demolabel,GTK_RC_STYLE(style));
	pango_font_description_set_size(GTK_RC_STYLE(style)->font_desc,gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(self.spFS))*PANGO_SCALE);
	label = g_object_get_data(G_OBJECT(self.bFont),"label");
	gtk_label_set_text(GTK_LABEL(label),pango_font_description_get_family(GTK_RC_STYLE(style)->font_desc));	
}


static void
on_font_size_change(GtkSpinButton *spinbutton,GtkScrollType arg1)
{
	GtkRcStyle *astyle = gtk_rc_style_new();
	
	astyle->font_desc = gtk_widget_get_style(self.demolabel)->font_desc;
	pango_font_description_set_size(astyle->font_desc,gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(self.spFS))*PANGO_SCALE);
	gtk_widget_modify_style(self.demolabel,GTK_RC_STYLE(astyle));
	g_free(astyle);
}


static int
select_font_popup_cmp_families (const void *a, const void *b)
{
  const char *a_name = pango_font_family_get_name (*(PangoFontFamily **)a);
  const char *b_name = pango_font_family_get_name (*(PangoFontFamily **)b);
  
  return g_utf8_collate (a_name, b_name);
}

static void
select_font_popup (GtkWidget *parent_button)
{
  GtkWidget *popup_window, *frame, *vbox, *button, *alignment, *scrolled_window, *button_label;
  GtkWidget *parent_arrow;
  GtkRequisition frame_requisition, parent_button_requisition;
  GtkRcStyle *button_label_rc_style;
  PangoFontFamily **families;
  gint n_families, i;
  gint x, y;
  gint screen_width;
  gint screen_height;

  parent_arrow = g_object_get_data (G_OBJECT (parent_button), "arrow");
  if (g_object_get_data (G_OBJECT (parent_button), "active") == TRUE)
  {
    gtk_widget_destroy (g_object_get_data (G_OBJECT (parent_button), "window"));
    g_object_set_data (G_OBJECT (parent_button), "active", FALSE);
    gtk_arrow_set (GTK_ARROW (parent_arrow), GTK_ARROW_DOWN, GTK_SHADOW_NONE);
  }
  else
  {
    g_object_set_data (G_OBJECT (parent_button), "active", TRUE);
    gtk_arrow_set (GTK_ARROW (parent_arrow), GTK_ARROW_UP, GTK_SHADOW_NONE);

    popup_window = gtk_window_new (GTK_WINDOW_POPUP);
    vbox = gtk_vbox_new (FALSE, 0);
    frame = gtk_frame_new (NULL);
    gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_OUT);
    scrolled_window = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

    pango_context_list_families (gtk_widget_get_pango_context (GTK_WIDGET (parent_button)), &families, &n_families);
    qsort (families, n_families, sizeof (PangoFontFamily *), select_font_popup_cmp_families);

    for (i=0; i<n_families; i++)
    {
      const gchar *font_name = pango_font_family_get_name (families[i]);

      button_label = gtk_label_new (font_name);
      button_label_rc_style = gtk_rc_style_new ();
      button_label_rc_style->font_desc = pango_font_description_from_string (g_strdup_printf ("%s 9", font_name));
      gtk_widget_modify_style (button_label, button_label_rc_style);

      alignment = gtk_alignment_new (0, 0, 0, 0);
      button = gtk_button_new ();
      gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
      gtk_container_add (GTK_CONTAINER (alignment), button_label);
      gtk_container_add (GTK_CONTAINER (button), alignment);
      gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
	  g_signal_connect(G_OBJECT(button),"clicked",G_CALLBACK(on_font_select),button_label_rc_style);
    }

    g_free (families);

    gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scrolled_window), vbox);
    gtk_container_add (GTK_CONTAINER (frame), scrolled_window);
    gtk_container_add (GTK_CONTAINER (popup_window), frame);
    gtk_widget_show_all (frame);

    gtk_widget_size_request (parent_button, &parent_button_requisition);
    gtk_widget_size_request (frame, &frame_requisition);

    gdk_window_get_position (GDK_WINDOW (mainw->window), &x, &y);

    screen_width = gdk_screen_width ();
    screen_height = gdk_screen_height ();
      
    x = CLAMP (x + parent_button->allocation.x, 0, MAX (0, screen_width - frame_requisition.width));
    y += parent_button->allocation.y;
    y += parent_button_requisition.height;

    gtk_widget_set_size_request (scrolled_window, -1, (screen_height - y) - 10);

    gtk_widget_set_uposition (popup_window, x, y);
      
    g_object_set_data (G_OBJECT (parent_button), "window", popup_window);

    gtk_widget_show (popup_window);
  }
}

