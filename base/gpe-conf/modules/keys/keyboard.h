#ifndef GPE_KEYBOARD_H
#define GPE_KEYBOARD_H

GtkWidget *Keyboard_Build_Objects();
void Keyboard_Free_Objects();
void Keyboard_Save();
void Keyboard_Restore();

void keyboard_save(char *type, char *port);
void keyboard_get_config(const char **model, const char **port);

#endif
