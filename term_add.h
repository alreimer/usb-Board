extern unsigned char *point[3];
extern unsigned long number;
extern unsigned long value_;

unsigned char *parsestr(unsigned char *a, unsigned char *b);
unsigned char *parsestr1( unsigned char *a, unsigned char *b);
char *parsestr1_( char *a, char *b);
struct parsestr{
//in parameters
    FILE *fd;
    char *arg;
    char flags;
//end of in
//out parameters
    char *begin;	//will be set by /[
    unsigned long num;
    unsigned long val;
    char *end;		//end of matched string
//end of out
//internal parameters
    char ch;
    char *zero;		//place, were ch was stored (for restoring) the "ch"
//end of internals
};

struct cascade{
    unsigned long s;		//stack
    unsigned long e;		//exec
    unsigned long c;		//case number
    unsigned long n;		//number
    unsigned long v;		//value
    struct cascade *next;
};
void free_cascade(void);

struct strctexec{
    unsigned long exec;
    char *begin;
    char *end;	//zero pointer (for restoring)
    unsigned long value;
    char ch;	//zero char
    struct strctexec *next;
};
typedef void exec_fnctn(struct strctexec *ptr);
void free_strctexec(void);

char *parsestr2(struct parsestr *ptr, exec_fnctn *fn, char *a, char *b);
char *restore_str( struct parsestr *ptr);//return the end of matched string (struct parsestr->end)
char *parsestr2_s( struct parsestr *ptr, exec_fnctn *fn, char *d, char *c);

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

