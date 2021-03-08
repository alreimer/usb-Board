//********** struct define *************
//int type;

#define CFG_PAR		1
#define CFG_AREA	2
#define CFG_TMP		3

struct cfg_parse1 {	//for web config
	int type;
	char *str;	//is whole string(will be cuted by replacing of ':' to '\0')
	char *name;	//pointer to begin of name
	char *web_name;	//pointer to begin of web_name
	char *value;	//pointer to begin of value in str
	char *new_value;	//pointer to begin of new_value in str
	char *pattern;	//pointer to begin of pattern in str
	unsigned long long size;		//max string size(is applied to value string!) with \0 char, 
						//if == 0 ->used as "temp"-parameters
	unsigned long long position;	//position of char in new_value to be written.
	int changed;		//by init - is 0	in cfg_parseargs will be changed (fill_cfg, fill_cfg_all)
	char saved;		//by init - 0 and in SaveConfig will be set to 1
	struct cfg_parse1 *next;
};

//***************************************

char *strstrcfg(char *a, char *b, int *len);
int  ReadConfiguration(void);
int  SaveConfiguration(void);
void recover_saved(char *par);
int cfg_arg_strcmp(char *parm, char flag);
int cfg_arg_changed(char *web_name);
char *get_cfg_value(long long *size, char *field_name, int i);	//i is 0-value, 1-new_value, 2-fresh_value
struct cfg_parse1 *get_cfg(char *field_name);
//void fill_all_cfg(void);
void fill_cfg(char *parm);//parm="parm1:parm2:parm3..."
int reg_par(char *name, char *value, long long size);//registering parameter with 'name'
void link_cfg(char *arg);
void write_char(char *arg);
