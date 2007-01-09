
#include "dictionary.h"

void irc_input_create( char* dict_filename, 
                             GtkWidget* entry, 
                             GtkWidget* quick_button, char* quick_filename,
                             GtkWidget* smiley_button, char* smiley_filename ); 
void irc_input_set_nick_dictionary( Dictionary* dict );
