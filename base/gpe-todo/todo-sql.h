extern int sql_start (void);
extern void add_new_item (struct todo_list *list, time_t t, const char *what, 
			  item_state state, const char *summary, int id);
