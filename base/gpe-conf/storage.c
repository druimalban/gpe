/*
 * gpe-conf
 *
 * Copyright (C) 2002  Pierre TARDY <tardyp@free.fr>
 *	             2003, 2004  Florian Boor <florian.boor@kernelconcepts.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 *
 * GPE storage info and settings module.
 *
 * memory info reading taken from minisys - thanks
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <libintl.h>
#define _(x) gettext(x)

#include <gtk/gtk.h>

#include <gpe/errorbox.h>
#include <gpe/spacing.h>
#include <gpe/pixmaps.h>
#include <gpe/render.h>

#include "storage.h"
#include "applets.h"
#include "suid.h"

static GtkWidget *vbox;

tfs *filesystems = NULL;
int fs_count = 0;
tfs meminfo;

int
which_fs_type (char *fsline)
{
	if (strstr (fsline, "/dev/root"))
		return FS_T_FLASH;
	if (strstr (fsline, "mtdblock"))
		return FS_T_FLASH;
	if (strstr (fsline, "mmc"))
		return FS_T_MMC;
	if (strstr (fsline, "hd"))
		return FS_T_CF;
	if (strstr (fsline, "nfs"))
		return FS_T_NFS;
	return FS_T_UNKNOWN;
}

/* from minisys */
int
system_memory (void)
{
	u_int64_t my_mem_used, my_mem_max;
	u_int64_t my_swap_max;

	static int mem_delay = 0;
	FILE *mem;
	static u_int64_t aa, ab, ac, ad, ae, af, ag, ah;
	/* put this in permanent storage instead of stack */
	static char not_needed[2048];
	if (mem_delay-- <= 0)
	{
		mem = fopen ("/proc/meminfo", "r");
		fgets (not_needed, 2048, mem);

		fscanf (mem, "%*s %Ld %Ld %Ld %Ld %Ld %Ld", &aa, &ab, &ac,
			&ad, &ae, &af);
		fscanf (mem, "%*s %Ld %Ld", &ag, &ah);
		fclose (mem);
		mem_delay = 25;

		/* calculate it */
		my_mem_max = aa;	/* memory.total; */
		my_swap_max = ag;	/* swap.total; */

		my_mem_used = ah + ab - af - ae;

		meminfo.total = my_mem_max / 1024;
		meminfo.used = my_mem_used / 1024;
		meminfo.avail = (my_mem_max - my_mem_used) / 1024;
		return 0;
	}
	return 1;
}


void
toolbar_set_style (GtkWidget * bar, gint percent)
{
	GtkStyle *astyle;
	GtkRcStyle *rc_style;
	static const GdkColor blue = { 0, 0, 0x0000, 0xf000 };
	static const GdkColor red = { 0, 0xd000, 0x0000, 0x0000 };
	static const GdkColor green = { 0, 0x0000, 0xd000, 0x0000 };
	static const GdkColor white = { 0, 0xffff, 0xffff, 0xffff };
	static const GdkColor yellow = { 0, 0xffff, 0xeeee, 0x0000 };
	astyle = gtk_widget_get_style (bar);
	rc_style = gtk_rc_style_new ();
	if (astyle)
	{
		/* bar active */
		if (percent < 80)
		{
			rc_style->bg[GTK_STATE_PRELIGHT] = green;
			rc_style->base[GTK_STATE_PRELIGHT] = green;
			rc_style->base[GTK_STATE_SELECTED] = green;
			rc_style->bg[GTK_STATE_SELECTED] = green;
		}
		else if (percent < 95)
		{
			rc_style->bg[GTK_STATE_PRELIGHT] = yellow;
			rc_style->base[GTK_STATE_PRELIGHT] = yellow;
			rc_style->base[GTK_STATE_SELECTED] = yellow;
			rc_style->bg[GTK_STATE_SELECTED] = yellow;
		}
		else
		{
			rc_style->bg[GTK_STATE_PRELIGHT] = red;
			rc_style->base[GTK_STATE_PRELIGHT] = red;
			rc_style->base[GTK_STATE_SELECTED] = red;
			rc_style->bg[GTK_STATE_SELECTED] = red;
		}

		rc_style->color_flags[GTK_STATE_PRELIGHT] |= GTK_RC_BG;
		rc_style->color_flags[GTK_STATE_PRELIGHT] |= GTK_RC_BASE;
		rc_style->color_flags[GTK_STATE_SELECTED] |= GTK_RC_BASE;
		rc_style->color_flags[GTK_STATE_SELECTED] |= GTK_RC_BG;

		rc_style->fg[GTK_STATE_SELECTED] = white;
		rc_style->text[GTK_STATE_SELECTED] = white;
		rc_style->color_flags[GTK_STATE_SELECTED] |= GTK_RC_FG;
		rc_style->color_flags[GTK_STATE_SELECTED] |= GTK_RC_TEXT;
		rc_style->text[GTK_STATE_NORMAL] = white;
		rc_style->fg[GTK_STATE_NORMAL] = white;

		/* bar background */
		rc_style->base[GTK_STATE_NORMAL] = blue;
		rc_style->bg[GTK_STATE_NORMAL] = blue;

		rc_style->color_flags[GTK_STATE_NORMAL] |= GTK_RC_FG;
		rc_style->color_flags[GTK_STATE_NORMAL] |= GTK_RC_TEXT;
		rc_style->color_flags[GTK_STATE_NORMAL] |= GTK_RC_BASE;
		rc_style->color_flags[GTK_STATE_NORMAL] |= GTK_RC_BG;

		gtk_widget_modify_style (bar, rc_style);
		gtk_rc_style_unref(rc_style);					  
	}

	gtk_widget_ensure_style (bar);
}

int
update_status ()
{
	gchar *fstr = NULL;
	FILE *pipe;
	static char cur[256];
	static char cnew[255], cnew2[255], cnew3[255];
	int fs_pos = 0;
	int i;


/* first get memory info - this is fast - our users want feedback! */
	if (!system_memory ())
	{
		fstr = g_strdup_printf ("%s%s %s",
					"<b><span foreground=\"black\">",
					_("System Memory"), "</span></b>");
		gtk_label_set_markup (GTK_LABEL (meminfo.label), fstr);
		g_free (fstr);

		gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (meminfo.bar),
					       (float) meminfo.used /
					       (float) meminfo.total);
    /* TRANSLATORS: MB == Mega Bytes*/
		sprintf (cnew2, "%s : <i>%4.1f</i> %s", _("Free memory"),
			 ((float) meminfo.total -
			  (float) meminfo.used) / 1024.0, _("MB"));
		gtk_label_set_markup (GTK_LABEL (meminfo.label3), cnew2);

		fstr = g_strdup_printf ("%s %s %4.1f %s %s",
					"<span foreground=\"black\">",
					_("Total:"),
					(float) meminfo.total / 1024.0,
					_("MB"), "</span>");
		gtk_label_set_markup (GTK_LABEL (meminfo.label2), fstr);
		g_free (fstr);
		toolbar_set_style (meminfo.bar,
				   (int) ((float) meminfo.used /
					  (float) meminfo.total * 100.0) + 20);
	}
	
/* now look at mounted filesystems */

	pipe = popen ("/bin/df -k", "r");

	if (pipe > 0)
	{
		fgets (cur, 255, pipe);
		while (!feof (pipe))
		{
			fgets (cur, 255, pipe);
			if ((which_fs_type (cur) != FS_T_UNKNOWN)
			    && (!feof (pipe)))
			{

				fs_pos++;
				// we create another set of widgets                       
				if (fs_pos > fs_count)
				{
					fs_count++;
					filesystems =
						realloc (filesystems,
							 fs_count * sizeof (tfs));
					filesystems[fs_count - 1].name = NULL;
					filesystems[fs_count - 1].mountpoint = NULL;
					filesystems[fs_count - 1].label =
						gtk_label_new (NULL);
					gtk_misc_set_alignment (GTK_MISC
								(filesystems[fs_count - 1].label),
								0, 0.5);

					gtk_box_pack_start (GTK_BOX (vbox),
							    filesystems
							    [fs_count -
							     1].label, FALSE,
							    FALSE, 0);

					filesystems[fs_count - 1].bar =
						gtk_progress_bar_new ();
					gtk_widget_set_sensitive (filesystems
								  [fs_count - 1].bar,
								  TRUE);
					toolbar_set_style (filesystems
							   [fs_count - 1].bar,
							   (int) ((float)
								  filesystems
								  [fs_count - 1].used /
								  (float)filesystems[fs_count - 1].total *
								  100.0));
					gtk_box_pack_start (GTK_BOX (vbox),
							    filesystems[fs_count - 1].bar, FALSE,
							    FALSE, 0);
					filesystems[fs_count - 1].label3 =
						gtk_label_new (NULL);
					gtk_misc_set_alignment (GTK_MISC
								(filesystems[fs_count - 1].label3),
								0.0, 0.5);
					gtk_box_pack_start (GTK_BOX (vbox),
							    filesystems
							    [fs_count - 1].label3, FALSE,
							    FALSE, 0);

					filesystems[fs_count - 1].label2 =
						gtk_label_new (NULL);
					gtk_misc_set_alignment (GTK_MISC
								(filesystems[fs_count - 1].label2),
								0.0, 0.5);
					gtk_box_pack_start (GTK_BOX (vbox),
							    filesystems
							    [fs_count - 1].label2, FALSE,
							    FALSE, 0);
								
					filesystems[fs_count - 1].dummy = gtk_label_new (NULL);
					gtk_widget_set_size_request(filesystems[fs_count - 1].dummy,-1,3);
					gtk_box_pack_start (GTK_BOX (vbox), filesystems[fs_count - 1].dummy, FALSE, FALSE, 0);
								
					gtk_widget_show_all (vbox);
				}
				// mark current fs
				filesystems[fs_pos - 1].present = TRUE;
				filesystems[fs_pos - 1].type =
					which_fs_type (cur);
				sscanf (cur, "%s %i %i %i %s %s", cnew,
					&filesystems[fs_pos - 1].total,
					&filesystems[fs_pos - 1].used,
					&filesystems[fs_pos - 1].avail, cnew2,
					cnew3);
				if (filesystems[fs_pos - 1].name)
					free (filesystems[fs_pos - 1].name);
				if (filesystems[fs_pos - 1].mountpoint)
					free (filesystems[fs_pos - 1].
					      mountpoint);
				filesystems[fs_pos - 1].name =
					g_strdup (cnew);
				filesystems[fs_pos - 1].mountpoint =
					g_strdup (cnew3);

				switch (filesystems[fs_pos - 1].type)
				{
				case FS_T_FLASH:
					fstr = g_strdup_printf ("%s%s %s",
								"<b><span foreground=\"black\">",
								_("Internal Flash"),
								"</span></b>");
					break;
				case FS_T_MMC:
					fstr = g_strdup_printf ("%s%s %s",
								"<b><span foreground=\"black\">",
								_("MMC"),
								"</span></b>");
					break;
				case FS_T_CF:
					fstr = g_strdup_printf ("%s%s %s",
								"<b><span foreground=\"black\">",
								_("Compact Flash"),
								"</span></b>");
					break;
				case FS_T_NFS:
					fstr = g_strdup_printf ("%s%s %s",
								"<b><span foreground=\"black\">",
								_("Network Filesystem"),
								"</span></b>");
					break;
				}

				gtk_label_set_markup (GTK_LABEL
						      (filesystems[fs_pos - 1].label),
						      fstr);
				g_free (fstr);

				gtk_progress_bar_set_fraction
					(GTK_PROGRESS_BAR
					 (filesystems[fs_pos - 1].bar),
					 (float) filesystems[fs_pos - 1].used /
					 (float) filesystems[fs_pos - 1].total);
				fstr = g_strdup_printf
					("%s%s %4.1f %s, %s <i>%s</i> %s",
					 "<span foreground=\"black\">",
					 _("Total:"),
					 (float) filesystems[fs_pos - 1].total / 1024.0, _("MB"),
					_("Mounted at "),
					 filesystems[fs_pos - 1].mountpoint,
					 "</span>");
				gtk_label_set_markup (GTK_LABEL
						      (filesystems[fs_pos - 1].label2),
						      fstr);
				g_free (fstr);
				sprintf (cnew2, "%s: <i>%4.1f</i> %s",
					 _("Free space"),
					 ((float) filesystems[fs_pos - 1].
					  total - (float) filesystems[fs_pos - 1].used) / 1024.0, _("MB"));
				gtk_label_set_markup (GTK_LABEL
						      (filesystems[fs_pos - 1].label3),
						      cnew2);
				toolbar_set_style (filesystems[fs_pos - 1].
						   bar, (int) ((float)filesystems[fs_pos - 1].used /
							  (float) filesystems[fs_pos - 1].total * 100.0));
			}
		}
		pclose (pipe);
	}

	// remove widgets for storage not present anymore
	for (fs_pos = 0; fs_pos < fs_count; fs_pos++)
	{
		if (filesystems[fs_pos].present != TRUE)
		{
			gtk_widget_destroy (filesystems[fs_pos].bar);
			gtk_widget_destroy (filesystems[fs_pos].label2);
			gtk_widget_destroy (filesystems[fs_pos].label3);
			gtk_widget_destroy (filesystems[fs_pos].label);
			gtk_widget_destroy (filesystems[fs_pos].dummy);
			fs_count--;
			for (i = fs_pos; i < fs_count; i++)
				filesystems[i] = filesystems[i + 1];
			if (fs_pos > 0)
				fs_pos--;
		}
		else
		{
			filesystems[fs_pos].present = FALSE;
		}
	}

	return TRUE;
}

void
Storage_Free_Objects ()
{
}

void
Storage_Save ()
{

}

void
Storage_Restore ()
{
	return;
}

GtkWidget *
Storage_Build_Objects (void)
{
	static GtkWidget *label1;
	gchar *fstr = NULL;
	static char cnew2[255];
	static GtkWidget *bar_flash;
	GtkWidget *viewport = gtk_viewport_new (NULL, NULL);
	GtkWidget *sw = gtk_scrolled_window_new (NULL, NULL);
	
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw),
				  GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	gtk_container_add (GTK_CONTAINER (sw),viewport);
	gtk_viewport_set_shadow_type (GTK_VIEWPORT (viewport), GTK_SHADOW_NONE);

	vbox = gtk_vbox_new (FALSE, 1);
	gtk_container_set_border_width (GTK_CONTAINER (vbox),
					gpe_get_border ());
	gtk_container_add (GTK_CONTAINER (viewport), vbox);

/* first get memory info - this is fast - our users want feedback! */

	while (system_memory ()) /* why does this fail? */
	{
		usleep(10000);
	}
	{
		fstr = g_strdup_printf ("%s%s %s",
					"<b><span foreground=\"black\">",
					_("System Memory"), "</span></b>");
		label1 = gtk_label_new (NULL);
		meminfo.label = label1;
		gtk_label_set_markup (GTK_LABEL (label1), fstr);
		g_free (fstr);
		gtk_label_set_justify (GTK_LABEL (label1), GTK_JUSTIFY_LEFT);
		gtk_misc_set_alignment (GTK_MISC (label1), 0, 0.5);
		gtk_box_pack_start (GTK_BOX (vbox), label1, FALSE, FALSE, 0);

		bar_flash = gtk_progress_bar_new ();
		meminfo.bar = bar_flash;
		gtk_widget_set_sensitive (bar_flash, TRUE);
		toolbar_set_style (bar_flash,
				   (int) ((float) meminfo.used /
					  (float) meminfo.total * 100.0) + 20);
		gtk_box_pack_start (GTK_BOX (vbox), bar_flash, FALSE, FALSE,
				    0);
		gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (bar_flash),
					       (float) meminfo.used /
					       (float) meminfo.total);
		sprintf (cnew2, "%s: <i>%4.1f</i> %s", _("Free memory"),
			 ((float) meminfo.total -
			  (float) meminfo.used) / 1024.0, _("MB"));
		label1 = gtk_label_new (NULL);
		gtk_misc_set_alignment (GTK_MISC (label1), 0.0, 0.5);
		meminfo.label3 = label1;
		gtk_label_set_markup (GTK_LABEL (label1), cnew2);
		gtk_box_pack_start (GTK_BOX (vbox), label1, FALSE, FALSE, 0);

		label1 = gtk_label_new (NULL);
		meminfo.label2 = label1;
		fstr = g_strdup_printf ("%s%s %4.1f %s %s",
					"<span foreground=\"black\">",
					_("Total:"),
					(float) meminfo.total / 1024.0,
					_("MB"), "</span>");
		gtk_label_set_markup (GTK_LABEL (label1), fstr);
		g_free (fstr);
		gtk_misc_set_alignment (GTK_MISC (label1), 0.0, 0.5);
		gtk_box_pack_start (GTK_BOX (vbox), label1, FALSE, FALSE, 0);
		
		meminfo.dummy = gtk_label_new (NULL);
		gtk_widget_set_size_request(meminfo.dummy,-1,4);
		gtk_box_pack_start (GTK_BOX (vbox), meminfo.dummy, FALSE, FALSE, 0);
	}

	gtk_widget_show_all(sw);
	
	gtk_timeout_add (1600, (GtkFunction) update_status, NULL);
	return sw;
}
