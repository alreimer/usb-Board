/* Global variabels */
extern char etc_save[];
extern char buf[];		//buffer for writting some information

int parse_file(char *file_name, char flag); //flag == 0 ->free parsed file
int copy_file(char *file, FILE *out);

struct page_n {
    char		*name;	//name of page (of file with path)
    unsigned int	size;	//size of name
    struct cfg_parse1	*cfg;	//for parameters
    struct tbl		*tbl_name;	//tables
    struct cgi		*cgi_name;	//for cgi's
    struct page_n	*next;
};
extern struct cfg_parse1 **cfg_p;

void free_page_n(struct page_n *ptr);
struct page_n *get_last_page(char *num);//num="0" or "1" or ... "5"
struct page_n *get_page(char *name);
struct page_n *get_page_cmp(char *name);

