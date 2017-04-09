char *parse_cgi_script(char *data);
char *cgi_end(char *data);


#define GET_CGI_MAX   78
#define MAX_SIZE_OF_NAME   30
//#define GET_CGI_LEN 2048
/*for x_open_file */

struct cgi {
    unsigned char	name[30];	//name of cgi
    unsigned long long name_size;
    int		cmd[GET_CGI_MAX];
    char	*arg[GET_CGI_MAX];
    int		bb[GET_CGI_MAX];
    int 	a, b, c, d;	//used as variables
    char	*data_ptr;	//pointer for data-field
    char	*parse;		//pointer for data-field buf_parse_area
    struct cgi	*next;
};

extern unsigned char A1_BTN;
extern struct cgi *cgi_name;	//begin of cgi

//struct cgi *find_cgi(char *filename);	//find cgi in cgi-tree. if not found return NULL

int get_cgi(char ch, int num, unsigned char *buf);
void wr_shell(char *tmp1, char flag);
int print(FILE *out, char *text);//returns 1 on success
char *print_(FILE *out, char *data, int make, char _else, int loop);
int print_str(FILE *out, char *text);
//void shell(char *tmp);
//void my_shell(FILE *out, char *tmp);
//if(buf_in == NULL) -> /dev/stdin is off. if(size_in == 0) -> strlen(buf_in)
//void write_shell(char *buf_in, long long size_in, char *buf, long long size, int mode, char *cmd);
void free_cgi(struct cgi *ptr);
//void show_CGIs(FILE *out);

//unsigned long long  strmycpy(char *tmp, char *tmp1, unsigned long long size);
unsigned long long parse_cgi_name(char *data, char **tmp, char *buf);
