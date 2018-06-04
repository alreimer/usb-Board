extern char *point[2];

char *parsestr1( char *a, char *b);
char *parsestr1_( char *a, char *b);
struct parsestr{
    char ch;
    char *zero;		//place, were ch was stored (for restoring)
    char *end;		//end of matched string
};
char *parsestr2(struct parsestr *ptr, char *a, char *b);
char *restore_str( struct parsestr *ptr);//return the end of matched string (struct parsestr->end)

char *get_var(unsigned long long *size_ptr, char *var_index);		//parse Varialbles

char *w_strtok(char **str, char mark);
unsigned long long strmycpy(char *tmp, char *tmp1, unsigned long long size);
unsigned long long strncpy_(char *tmp, char *tmp1, long long size);
int write_ppar(char *line);
int copy(FILE *read_f, FILE *write_f);

void write_system(char *b_in, long long s_in, char *buf, long long size, int mode, char *cmd);
void my_system(FILE *out, char *cmd);
void system_(char *cmd);


void shell(char *tmp);
void my_shell(FILE *out, char *tmp);
//if(buf_in == NULL) -> /dev/stdin is off. if(size_in == 0) -> strlen(buf_in)
void write_shell(char *buf_in, long long size_in, char *buf, long long size, int mode, char *cmd);

