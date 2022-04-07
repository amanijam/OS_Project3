void mem_init_frame();
void mem_init();
void varStore_init();
char *mem_get_value(char *var);
int mem_set_value(char *var, char *value);
int insert(char *var_in, char *value_in);
char *mem_get_value_from_position(int i);
void mem_remove_by_position(int i);
