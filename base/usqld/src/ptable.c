#include <pthread.h> 

typedef struct ThreadEntry{
   pthread_t thread;
   
}ThreadEntryT; 

typedef struct ThreadTable
{
   
}ThreadTable;



ThreadTable * create_Threadtable(size_t max_procs)
{
   ThreadTable * tab;
   tab = malloc(sizeof(ThreadTable));
   
}
