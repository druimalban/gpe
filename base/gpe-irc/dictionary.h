#ifndef __DICTIONARY_H__
#define __DICTIONARY_H__
#include <glib.h>


struct _dictionary {

    GList* list;
    GHashTable* lookup_hash;
    char* last_word;
    GList* last_node;

};
typedef struct _dictionary Dictionary;


Dictionary* dictionary_new(); 
Dictionary* dictionary_new_from_file( char* filename ); 
Dictionary* dictionary_new_from_list( GList* list ); 
void dictionary_destroy( Dictionary* dict ); 
void dictionary_add_word( Dictionary* dict, const char* word, gboolean rehash ); 
void dictionary_remove_word( Dictionary* dict, const char* word, gboolean rehash ); 
void dictionary_add_word_list( Dictionary* dictionary, GList* list ); 
void dictionary_remove_word_list( Dictionary* dictionary, GList* list ); 
void dictionary_load( Dictionary* dict, char* filename ); 
char* dictionary_predict_word( Dictionary* dict, char* word ); 
void dictionary_predict_reset( Dictionary* dict ); 

#endif
