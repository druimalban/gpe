/*
 *  callbacks.c - functions to handle GUI events.
 *	part of galculator
 *  	(c) 2002 Simon Floery (simon.floery@gmx.at)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
 
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "calc_basic.h"
#include "galculator.h"
#include "math_functions.h"
#include "general_functions.h"
#include "display.h"

#include "callbacks.h"
#include "interface.h"
#include "support.h"

s_function_list function_list[] = {\
	{"sin", {sin, asin, sinh, sin}},\
	{"cos", {cos, acos, cosh, cos}},\
	{"tan", {tan, atan, tanh, tan}},\
	{"log", {log10, pow10y, log10, log10}},\
	{"ln", {log, exp, log, log}},\
	{"1/x", {reciprocal, idx, reciprocal, reciprocal}},\
	{"x^2", {powx2, sqrt, powx2, powx2}},\
	{"SQRT", {sqrt, powx2, sqrt, sqrt}},\
	{"n!", {factorial, factorial, factorial, factorial}},\
	{}\
};

s_constant_list constant_list[] = {\
	{"e", G_E},\
	{"PI", G_PI},\
	{}\
};

s_gfunc_list gfunc_list[] = {\
	{"+/-", display_result_toggle_sign},\
	{"<-", display_result_backspace},\
	{"EE", display_append_e},\
	{"C", clear},\
	{"AC", all_clear},\
	{"MS", memory_save},\
	{"MR", memory_read},\
	{"M+", memory_add2mem},\
	{}\
};

/* File */

void
on_quit1_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	gtk_main_quit();
}

/* this callback is called if a button for entering a number is clicked. There are two
 * cases: either starting a new number or appending a digit to the existing number.
 * The decimal point leads to some specialities.
 */

void
on_number_button_clicked               (GtkButton       *button,
                                        gpointer         user_data)
{	
	display_result_add_digit (*(gtk_button_get_label (button)));
}

/* this callback is called if a button for doing one of the arithmetic operations plus, minus, 
 * multiply, divide or power is clicked. it is mainly an interface to the calc_basic code.
 */

void
on_operation_button_clicked            (GtkButton       *button,
                                        gpointer         user_data)
{
	s_calc_token	current_token;
	
	/* current number is get from the display! */
	current_token.number = display_result_get_as_double ();
	current_token.operator = *(gtk_button_get_label (button));
	/* this is not the nice way, but ... */
	if (current_token.operator == *("x^y")) current_token.operator = '^';
	/* and display is set to return_value! */
	display_result_set_double (calc_tree_add_token (current_token));
	calc_entry_start_new = TRUE;
}

/* this callback is called if a button for a function manipulating the current entry directly
 * is clicked. the array function_list knows the relation between button label and function to
 * call.
 */

void
on_function_button_clicked             (GtkButton       *button,
                                        gpointer         user_data)
{
	int		counter=0;
	
	/* at first find the corresponding function */
	while (function_list[counter].button_label != NULL) 
	{
		if (strcmp (function_list[counter].button_label, gtk_button_get_label (button)) == 0) break;
		counter++;
	}
	if (function_list[counter].button_label != NULL) 
	{
		display_result_set_double (\
			function_list[counter].func[current_status.fmod](display_result_get_as_double()));
		/* I tried to avoid successive calls to lookup_widget, but ... */		
		gtk_toggle_button_set_active ((GtkToggleButton *)lookup_widget (((GtkWidget *)button)->parent, "tbutton_inv"), FALSE);
		gtk_toggle_button_set_active ((GtkToggleButton *)lookup_widget (((GtkWidget *)button)->parent, "tbutton_hyp"), FALSE);
		calc_entry_start_new = TRUE;	
	}
	else error_message ("unknown button <-> function relation");
}


gboolean
on_main_window_key_press_event         (GtkWidget       *widget,
                                        GdkEventKey     *event,
                                        gpointer         user_data)
{
	/* the problem: NUMLOCK counts as a modifier for gdk, like Shift or Control or Alt.
	 * If NUMLOCK is activated, an accelerator defined e.g. with CTRL-q won't work as they
	 * are caught as CTRL-NUMLOCK-q. As a matter of fact, we can't set the NUMLOCK modifier
	 * in GLADE. In order to not modify interface.c everytime the GUI changed, I catch here
	 * any key pressed and delete, if not 0-9 or decimal on keypad, the NUMLOCK flag.
	 * 0-9 and decimal on the keypad need the NUMLOCK flag, otherwise they are interpreted
	 * like Pgup and all the funny staff found on the deactivated keypad.
	 */
	
	/*printf ("%i %s\n", event->keyval, gdk_keyval_name (event->keyval));*/
	if (((event->keyval < GDK_KP_0) || (event->keyval > GDK_KP_9)) && \
		(event->keyval != GDK_KP_Decimal)) event->state &= ~GDK_MOD2_MASK;

	return FALSE;
}

void
on_constant_button_clicked             (GtkButton       *button,
                                        gpointer         user_data)
{
	int		counter=0;
	
	/* at first find the corresponding function */
	while (constant_list[counter].button_label != NULL) 
	{
		if (strcmp (constant_list[counter].button_label, gtk_button_get_label (button)) == 0) break;
		counter++;
	}
	if (constant_list[counter].button_label != NULL) display_result_set_double (constant_list[counter].constant);
	else error_message ("unknown button <-> constant relation");
}

void
on_tbutton_fmod_clicked                (GtkButton       *button,
                                        gpointer         user_data)
{
	if (strcmp (gtk_button_get_label (button), "INV") == 0)
	{
		current_status.fmod ^= 1<<CS_FMOD_FLAG_INV;
	}
	else if (strcmp (gtk_button_get_label (button), "HYP") == 0)
	{
		current_status.fmod ^= 1<<CS_FMOD_FLAG_HYP;
	}
	else error_message ("unknown function modifier (INV/HYP)");
}

void
on_gfunc_button_clicked                (GtkButton       *button,
                                        gpointer         user_data)
{
	int		counter=0;
	
	while (gfunc_list[counter].button_label != NULL) 
	{
		if (strcmp (gfunc_list[counter].button_label, gtk_button_get_label (button)) == 0) break;
		counter++;
	}
	if (gfunc_list[counter].button_label != NULL) gfunc_list[counter].func();
	else error_message ("unknown button <-> gfunc relation");
}
