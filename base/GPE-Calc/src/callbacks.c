#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>

#include <stdio.h>
#include <string.h>

#include "callbacks.h"
#include "interface.h"
#include "support.h"


float Operand=0.0;
float Operator=0.0;
float *Number=&Operand;
char DispVal[64]="0";

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
			sprintf(DispVal,"%.0f",*Number);
			UpdateDisplay(widget);
			break;
		case Minus:
			Operand -= Operator;
			Number = &Operand;
			sprintf(DispVal,"%f",*Number);
			UpdateDisplay(widget);
			break;
		case Times:
			Operand *= Operator;
			Number = &Operand;
			sprintf(DispVal,"%f",*Number);
			UpdateDisplay(widget);
			break;
		case Div:
			if (Operator != 0) {
				Operand /= Operator;
				Number = &Operand;
				sprintf(DispVal,"%f",*Number);
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
	if (strlen(DispVal) > 0)
		if (DispVal[strlen(DispVal)-1] != '.') {
			strcat(DispVal,".");
			UpdateDisplay(GTK_WIDGET(button));
		}
}


void
on_Three_clicked                       (GtkButton       *button,
                                        gpointer         user_data)
{
	if (strlen(DispVal)==1 && DispVal[0]=='0')
		DispVal[0]='\0';
	strcat(DispVal,"3");
	UpdateDisplay(GTK_WIDGET(button));
}


void
on_Six_clicked                         (GtkButton       *button,
                                        gpointer         user_data)
{
	if (strlen(DispVal)==1 && DispVal[0]=='0')
		DispVal[0]='\0';
	strcat(DispVal,"6");
	UpdateDisplay(GTK_WIDGET(button));
}


void
on_Nine_clicked                        (GtkButton       *button,
                                        gpointer         user_data)
{
	if (strlen(DispVal)==1 && DispVal[0]=='0')
		DispVal[0]='\0';
	strcat(DispVal,"9");
	UpdateDisplay(GTK_WIDGET(button));
}


void
on_Zero_clicked                        (GtkButton       *button,
                                        gpointer         user_data)
{
	if (strlen(DispVal)==1 && DispVal[0]=='0')
		DispVal[0]='\0';
	strcat(DispVal,"0");
	UpdateDisplay(GTK_WIDGET(button));
}


void
on_Two_clicked                         (GtkButton       *button,
                                        gpointer         user_data)
{
	if (strlen(DispVal)==1 && DispVal[0]=='0')
		DispVal[0]='\0';
	strcat(DispVal,"2");
	UpdateDisplay(GTK_WIDGET(button));
}


void
on_Five_clicked                        (GtkButton       *button,
                                        gpointer         user_data)
{
	if (strlen(DispVal)==1 && DispVal[0]=='0')
		DispVal[0]='\0';
	strcat(DispVal,"5");
	UpdateDisplay(GTK_WIDGET(button));
}


void
on_Eight_clicked                       (GtkButton       *button,
                                        gpointer         user_data)
{
	if (strlen(DispVal)==1 && DispVal[0]=='0')
		DispVal[0]='\0';
	strcat(DispVal,"8");
	UpdateDisplay(GTK_WIDGET(button));
}


void
on_Seven_clicked                       (GtkButton       *button,
                                        gpointer         user_data)
{
	if (strlen(DispVal)==1 && DispVal[0]=='0')
		DispVal[0]='\0';
	strcat(DispVal,"7");
	UpdateDisplay(GTK_WIDGET(button));
}


void
on_Four_clicked                        (GtkButton       *button,
                                        gpointer         user_data)
{
	if (strlen(DispVal)==1 && DispVal[0]=='0')
		DispVal[0]='\0';
	strcat(DispVal,"4");
	UpdateDisplay(GTK_WIDGET(button));
}


void
on_One_clicked                         (GtkButton       *button,
                                        gpointer         user_data)
{
	if (strlen(DispVal)==1 && DispVal[0]=='0')
		DispVal[0]='\0';
	strcat(DispVal,"1");
	UpdateDisplay(GTK_WIDGET(button));
}

