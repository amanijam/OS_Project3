void resetmem();
int mem_set_value(char *var_in, char *value_in);
char *mem_get_value(char *var_in);

typedef struct frame_struct FrameSlice;
void framestr_init();
void freeFrameStr();
int getLRUFrameNum();
void evictFrame();
int insert_framestr(int pid, int pn, char *line);
FrameSlice *mem_get_from_framestr(int i);
FrameSlice *mem_read_from_framestr(int i);
void mem_remove_from_framestr(int i);
int pageSize;