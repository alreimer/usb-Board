#define ETC_PATH		"/etc"

#define SERVER_PORT		80
#define MOUNT_POINT		"/mnt/hd1"	//"/mnt"
//#define HTTPD_DOCUMENT_ROOT	"/httpd_nh221/web"	//used in httpd.
#define DOCUMENT_ROOT		"/httpd_nh221/web"	//used in httpd_scripts.c

//#define DO_DIR			//use to show dir content as html
//#define SP_OPTIONS			//to get feedback about no OPTIONS IPMLEMETATION
//#define HTTPD_MAX_ACCEPT
//#define USB		//enable cgi scripts for USB support

/* Methods */
#define M_GET		1
#define M_POST		2
#define M_OPTIONS	4

/* post transfer type */
#define    POST_NO 		0
#define    POST_BOUNDARY	1
#define    POST_WEB_ENCODED 	2

//typedef struct{
//    const char *name;	 /* The name of WEB field */
//    char *value;
//} WEB_VAR;

typedef int file_handler(FILE *out, char *name, char *type);
typedef struct {
    const char *ext;
    char *type;
    file_handler *handler;
} MIME_TYPES;

typedef struct{
    const char *name;  /* Argument name to look for */
    char *var;	       /* Variable to copy value into, if present */
    int len;	       /* Max length of string, including terminator */
} ARGUMENT;

/* Global variabels */
extern char referer[];
extern char user_agent[];
extern char boundary[];
extern char ip[];
extern char check_ip[];		//internal use. if="" so ip is released.
extern char port[];
extern char dns_name[];
extern char etc_save[];
extern char buf[];		//buffer for writting some information
extern unsigned long long buf_size;
/*version used in terminal*/
extern char version[];
extern char value[];

extern char hdused[];
extern char uptime[];
//extern char *arg;
extern char *file;
extern FILE *fdcr;//file discriptor of web socket
