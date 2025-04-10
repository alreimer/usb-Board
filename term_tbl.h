void parse_tbl(char *data, char clean);
struct rnd_tbl *find_tbl(char *name);
void show_tbl(char *var, FILE *out);
void show_tbl_chck(char *var, FILE *out);
unsigned long long show_tbl_str(char *var, char *buf, unsigned long long size);
char *get_tbl(char *var);
unsigned char tbl_changed(char *name);
void tbl_show(char *name);
void tbl_show_all(void);
unsigned int *get_tbl_begin(char *name);//name of table
void change_tbl_stat(char *data);	//data="flag:name:frase(forParsing)"

#define TAB_LEN 16

struct rnd_tbl {
    unsigned int	rnd_entry;	//name of cgi
    int			flag;		//=1 - not show, =0 - show
    int			p_flag;		//used for =1 print(), and =0 fprintf()
    struct rnd_tbl	*next;
    char		*entry;
    char		*entries[TAB_LEN];//if entry==NULL so use that entries
};

struct tbl {
    char		*name;	//name of cgi
    unsigned int	begin;	//begin of window!, scrolling throw table
    unsigned int	begin_old;	//old begin of window!
    char		changed;	//new: default is 0. set to 1 if table is changed.
    char		show;		//show table one time
    struct rnd_tbl	*ptr;
    struct tbl		*next;
};

void free_rnd_tbl(struct rnd_tbl **ptr);
void free_tbl(struct tbl *ptr);
extern struct tbl **tbl_name;	//begin of table

