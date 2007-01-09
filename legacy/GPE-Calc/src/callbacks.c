#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>

#include <stdio.h>
#include <string.h>

#include "callbacks.h"
#include "interface.h"
#include "support.h"

#define DISP_MAX 64

float Operand=0.0;
float Operator=0.0;
float *Number=&Operand;
char DispVal[DISP_MAX]="0";

typedef enum {NOP, Plus, Minus, Div, Times} OperationT;
OperationT Operation=NOP;

void UpdateDisplay(GtkWidget *widget)
{
GtkWidget *Result;

	Result=lookup_widget(widget,"Result");
	gtk_entry_set_text(GTK_ENTRY(Result),DispVal);
}

void
on_Clear_clicked                       (GtkButton       *button,
                                        gpointer         user_data)
{
	*Number=0.0;
	strcpy(DispVal,"0");
	UpdateDisplay(GTK_WIDGET(button));
}


void
on_AllClear_clicked                    (GtkButton       *button,
                                        gpointer         user_data)
{
	Number = &Operand;
	Operand = 0;
	Operator = 0;
	strcpy(DispVal,"0");
	UpdateDisplay(GTK_WIDGET(button));
}


void Calculate(GtkWidget *widget)
{
	if (Operation == NOP)
		return;
	sscanf(DispVal,"%f",&Operator);
	switch (Operation) {
		case Plus:
			Operand += Operator;
			Number = &Operand;
			snprintf(DispVal,DISP_MAX - 1,"%.0f",*Number);
			UpdateDisplay(widget);
			break;
		case Minus:
			Operand -= Operator;
			Number = &Operand;
			snprintf(DispVal,DISP_MAX - 1,"%f",*Number);
			UpdateDisplay(widget);
			break;
		case Times:
			Operand *= Operator;
			Number = &Operand;
			snprintf(DispVal,DISP_MAX - 1,"%f",*Number);
			UpdateDisplay(widget);
			break;
		case Div:
			if (Operator != 0) {
				Operand /= Operator;
				Number = &Operand;
				snprintf(DispVal,DISP_MAX - 1,"%f",*Number);
				UpdateDisplay(widget);
			}
			break;
		default:
			return;
			break;
	}
	Operation = NOP;
}

void
on_Plus_clicked                        (GtkButton       *button,
                                        gpointer         user_data)
{
	Calculate(GTK_WIDGET(button));
	sscanf(DispVal,"%f",Number);
	Number = &Operator;
	Operation = Plus;
	strcpy(DispVal,"0");
}


void
on_Minus_clicked                       (GtkButton       *button,
                                        gpointer         user_data)
{
	Calculate(GTK_WIDGET(button));
	sscanf(DispVal,"%f",Number);
	Number = &Operator;
	Operation = Minus;
	strcpy(DispVal,"0");
}


void
on_Div_clicked                         (GtkButton       *button,
                                        gpointer         user_data)
{
	Calculate(GTK_WIDGET(button));
	sscanf(DispVal,"%f",Number);
	Number = &Operator;
	Operation = Div;
	strcpy(DispVal,"0");
}


void
on_Times_clicked                       (GtkButton       *button,
                                        gpointer         user_data)
{
	Calculate(GTK_WIDGET(button));
	sscanf(DispVal,"%f",Number);
	Number = &Operator;
	Operation = Times;
	strcpy(DispVal,"0");
}


void
on_Equals_clicked                      (GtkButton       *button,
                                        gpointer         user_data)
{
	Calculate(GTK_WIDGET(button));
	Operation=NOP;
	strcpy(DispVal,"0");
}


void
on_Point_clicked                       (GtkButton       *button,
                                        gpointer         user_data)
{
	size_t s = strlen(DispVal);
	if (s > 0
	    && s < (DISP_MAX - 1)
	    && DispVal[s-1] != '.') {
		strcat(DispVal, ".");
		UpdateDisplay(GTK_WIDGET(button));
	}
}


void
on_number_clicked                       (GtkButton       *button,
					 gpointer         user_data)
{
	size_t s = strlen(DispVal);
	if (s < (DISP_MAX - 1)) {
		if (s==1 && DispVal[0]=='0')
			DispVal[0]='\0';
		strcat(DispVal, (char *)user_data);
		UpdateDisplay(GTK_WIDGET(button));
	}
}

