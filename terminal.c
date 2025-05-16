/* terminal.c:  A very simple connection programm for USB-Board
 *
 * Copyright (c) 2015-2021 Alexander Reimer <alex_raw@rambler.ru>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/stat.h>
#include <dirent.h>
#include <signal.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
//#include "include/httpd_config.h"	//cfg_parse1
#include "term_parser.h"
#include "term_add.h"
#include "term_tbl.h"
#include "term_cgi.h"
#include "t_config.h"
#include "terminal.h"

FILE *fp;
char buffer[2078];

/* Global variables */
char etc_save[2];
//char buf[16384];
char buf[65537];//65536 + 1
unsigned long long buf_size = 0;
char version[256];
//char value[1024];
char auth[5];


#define MIN(a,b) ((a) < (b) ? (a) : (b))



int copy_file(char *file, FILE *out)
{
  FILE *fp;
  int n;
  int wrote;
  char buf_fer[2078];
  char *err_str="[ERR] File: %s cannot open\n";

  if((fp=fopen(file,"r")) == NULL){
    printf(err_str, file);
    fprintf(out, err_str, file);
    return 1;
  }
  else{
    while ((n = fread(buf_fer,1,sizeof(buf_fer),fp)) != 0) {
	wrote = fwrite(buf_fer,n,1,out);
	if (wrote < 1){
	    fclose(fp);
	    return 1;
	}
    }
    fclose(fp);
  }
  return 0;
}


//struct cfg_parse1 *cfg1 = NULL;	/*new config via web*/
struct cfg_parse1 **cfg_p = NULL;
struct page_n *p_n = NULL;

//clear all parameters
void free_par(struct cfg_parse1 *ptr){
	if(!ptr) return;
	free_par(ptr->next);
printf("FREEpar %s\n", ptr->str);
	if(ptr->str) free(ptr->str);
	free(ptr);
}
/*
void free_page_mem(void){
    free_tbl();
}
*/
void free_page_n(struct page_n *ptr){
    if(ptr == NULL) return;
    free_page_n(ptr->next);
    if(ptr->name){
	printf("Free PAGE:%s\n", ptr->name);
	free(ptr->name);
    }
    if(ptr->cfg){
	free_par(ptr->cfg);
	ptr->cfg = NULL;//maybe not needed
    }
    if(ptr->tbl_name){
        free_tbl(ptr->tbl_name);
        ptr->tbl_name = NULL;//here too
    }
    if(ptr->cgi_name){
	free_cgi(ptr->cgi_name);
	ptr->cgi_name = NULL;//maybe not needed
    }
    free(ptr);
}

struct page_n *get_last_p(struct page_n *ptr, int digi, int *i, int flag){
    struct page_n *p;
    if(ptr == NULL){
	*i = 0;
	return NULL;
    }
    if((flag == 1) && (&(ptr->cfg) == cfg_p)){	//for flag == 1 -> relative to actual page (_referrer)
	if(digi == 0) return ptr;
	*i = 1;
	return NULL;
    }

    p = get_last_p(ptr->next, digi, i, flag);
//printf("loop: %s   %d\n",ptr->name, *i);
    if(p != NULL) return p;
    if(*i == digi) return ptr;
/*
    if((flag == 1) && (&(ptr->cfg) == cfg_p)){	//for flag == 1 -> relative to actual page (_referrer)
	*i = 1;
	return ptr;
    }
*/
    (*i)++;
    return NULL;

}

struct page_n *get_last_page(char *num, int flag){
    int digi = 1, i = 0;	//if digi = 1 -> get page "forlast". if digi = 0 -> get last page

    if(num != NULL && *num != '\0'){
	digi = 0;
	while(num[i] >= '0' && num[i] <= '9'){
	    digi = digi*10 + (num[i] - '0');
	    i++;
	    if(i > 3) break;
	}
    }

    return get_last_p(p_n, digi, &i, flag);
}

struct page_n *get_page(char *name){
    struct page_n *ptr = p_n;

    while(ptr){
	if(ptr->name && parsestr(ptr->name, name)){
		return ptr;	//matched name with entry_name
	 }
	ptr = ptr->next;
    }
    return NULL;

}

struct page_n *get_page_cmp(char *name){
    struct page_n *ptr = p_n;
    while(ptr){
	if(ptr->name && !strcmp(ptr->name, name))	return ptr;	//matched name with entry_name
	ptr = ptr->next;
    }
    return NULL;

}
struct page_n *get_page_cmp_next(char *name, int flag){
    struct page_n *ptr = p_n;
    int next;
    if(flag == 1) next = 0;	//if flag == 1 -> name mast be nexter as actual page
    else next = 1;

    while(ptr){
	if(flag && !next && (&(ptr->cfg) == cfg_p)) next = 1;
	if(next && ptr->next && ptr->next->name && !strcmp(ptr->next->name, name))	return ptr;	//matched name with entry_name
	ptr = ptr->next;
    }
    return NULL;

}

struct file_n {
    char		*file_name;	//name of file
    char		*begin;	//begin of file
};

char *parse_file_(char *data, struct file_n *f_n, int make, int loop){
	char *ptr1, *tmp, *a, i;
	unsigned int *pt;
	unsigned long long digi, str_size;
	struct cfg_parse1 *cfg_pointer,/* *cfg_pointer1,*/ **cfg;
	struct parsestr parser;
	char *name, *digit, *p, *ptr2, mk;



    while((data != NULL) && *data){

	if(*data == ' ' || *data == '\t' || *data == '\n' || *data == '\r') {data++;continue;}

	else if(tmp = parsestr2(&parser, NULL, data, "/n0N/ /B//*/**///\\/////*\n"
				"/\\/sV/n22n"
				"/\\>/n23n"
				"/\\par:/t/sV:/[/*/]\n/n1n"
				"/\\area:/t/sV:/[/*/]\n/n2n"
				"/\\readcfg/n3n"
				"/\\link:/t/[/*/]\n/n4n"
				"/\\include:/t/[/*/]\n/n5n"
				"/\\wr_shell:/t/[/*/]\n/n6n"
				"/\\wr_par:/t/[/*/]\n/n7n"
				"/\\shell:/t/[/*/]\n/n8n"
				"/\\cgi:/t/[/*/]\n/n9n"
				"/\\table:/t/[/*\\n/]#end_table\\n/n20n"
				"/\\\"/[/*/N\\N/]\"/n21n")){
	    if(make == 1){
	    switch(parser.num){
		case 1:
/*** par -parameter ***/
	digi = value_;
	if(!digi || digi > 300 || (cfg_p == NULL)) {break;}	//size is in impassable range - so skip it all (whole parameter string will be skipped!)

	cfg_pointer = (struct cfg_parse1 *)malloc(sizeof(struct cfg_parse1));

	if(cfg_pointer == NULL){ break;}

	cfg = cfg_p;
	while(*cfg){
	    cfg = &((*cfg)->next);
	}
	*cfg = cfg_pointer;

	str_size = parser.zero - tmp;			//   v - (2chars) is for shore
	ptr1 = (char*)malloc(str_size + 1 + 2*digi + 2);//str = "[part of string][value][new_value]"
	if(ptr1 == NULL){
	    printf("ERROR allocate memory\n");
	    free(*cfg);
	    *cfg = NULL;	//main criteria to abort moving in array.
	    break;
	}

	strncpy(ptr1, tmp, str_size);//256:web_name:name:pattern
	ptr1[str_size] = '\0';

//printf("par-->%s<--\n", ptr1);

	cfg_pointer->type = CFG_PAR;
	cfg_pointer->str = ptr1;
	cfg_pointer->size = digi;
	cfg_pointer->changed = 0;
	cfg_pointer->position = 0;
	cfg_pointer->saved = 0;
	cfg_pointer->value = ptr1 + str_size + 1;
//	*(cfg_pointer->value) = '\0';
	cfg_pointer->new_value = ptr1 + str_size + 1 + digi;
//	*(cfg_pointer->new_value) = '\0';
	memset(cfg_pointer->value, 0, 2*digi);//fill with zeros
	cfg_pointer->name = NULL;
	cfg_pointer->pattern = NULL;
	cfg_pointer->web_name = NULL;
	cfg_pointer->next = NULL;

	    i = 0;
	    while(i < 2){
		a = w_strtok(&ptr1, ':');
	        if(a)
		    switch(i){
		    case 0:	cfg_pointer->web_name = a; break;
		    case 1:	cfg_pointer->name = a; cfg_pointer->pattern = ptr1; break;
		    }
		else {
		    printf("par: not full, broken by %d\n", i+1);
		    break;
		}
	    i++;
	    }
printf("Collected parameter: --%s--%s--%lld--%s--\n", cfg_pointer->web_name, cfg_pointer->name, cfg_pointer->size, cfg_pointer->pattern);
//insert
		    break;
		case 2:
/** area:size:web_name **/
	digi = value_;
	if(digi > 64*1024 || (cfg_p == NULL)) {break;}	//size is in impassable range - so skip it all (whole parameter string will be skipped!)

	cfg_pointer = (struct cfg_parse1 *)malloc(sizeof(struct cfg_parse1));

	if(cfg_pointer == NULL){ break;}

	cfg = cfg_p;
	while(*cfg){
	    cfg = &((*cfg)->next);
	}
	*cfg = cfg_pointer;

	str_size = parser.zero - tmp;			// v - (2chars) is for shore
	ptr1 = (char*)malloc(str_size + 1 + digi + 2);//str = "[part of string][value]"
	if(ptr1 == NULL){
	    printf("ERROR allocate memory\n");
	    free(*cfg);
	    *cfg = NULL;	//main criteria to abort moving in array.
	    break;
	}

	strncpy(ptr1, tmp, str_size);//web_name
	ptr1[str_size] = '\0';

//printf("area-->%s<--\n", ptr1);

	cfg_pointer->type = (digi != 0) ? CFG_AREA:CFG_TMP;
	cfg_pointer->str = ptr1;
	cfg_pointer->size = digi;
	cfg_pointer->changed = 0;
	cfg_pointer->position = 0;
	cfg_pointer->saved = 0;
	if(digi != 0){
	cfg_pointer->value = ptr1 + str_size + 1;
//	*(cfg_pointer->value) = '\0';
	cfg_pointer->new_value = cfg_pointer->value;
//	cfg_pointer->new_value = ptr1 + str_size + 1 + digi;
//	*(cfg_pointer->new_value) = '\0';
	memset(cfg_pointer->value, 0, digi);//fill with zeros
	}else{//AREA=0:temp_name
	cfg_pointer->value = NULL;
	cfg_pointer->new_value = NULL;
	}
	cfg_pointer->name = NULL;
	cfg_pointer->pattern = NULL;
	cfg_pointer->next = NULL;
	cfg_pointer->web_name = ptr1;

printf("Collected area: --%s--%lld--\n", cfg_pointer->web_name, cfg_pointer->size);
//insert
		    break;
		case 3:
		    ReadConfiguration();
		    break;
		case 4:
		    link_cfg(tmp);/** link:to_name:from_name **/
		    break;
		case 5:
		    parse_file(tmp, 1);	//parse file w/o erease of other parameters
		    break;
		case 6:
		    wr_shell(tmp, 0);	//write to shell  "from_par:to_par:cmd"
		    break;
		case 7:
		// write_par  par:value		this is used in copy_CGI.c - the same code!
		    ptr1 = w_strtok(&tmp, ':');//ptr1 = par, tmp = value
		    if(ptr1 && *ptr1 && *tmp){
			a = get_var(&str_size, ptr1);
			if(a && str_size){
			    strncpy_(a, tmp, str_size-1);
//			    tmp[str_size-1] = '\0';
			}
			if(ptr1 != tmp) *(tmp-1) = ':';
		    }
		    break;
		case 8:
		    my_shell(fp, tmp);
/*			str_size = parser.zero - tmp;
			if(str_size != 0){
			ptr1 = (char *)malloc(str_size + 2);
			if(ptr1){
			    strncpy(ptr1, tmp, str_size);
			    ptr1[str_size] = '\0';
//printf("shell--%s--\n", ptr1);
			    my_shell(fp, ptr1);
			    free(ptr1);
			}
			}else{ printf("WARN: shell line of file: %s, at char %ld, is empty!\n", f_n->file_name, data-(f_n->begin));}
*/
		    break;
		case 9:
			str_size = parse_cgi_name(tmp, &ptr1, NULL);//ptr1 - is end(\n or \0) of cgi-string
//printf("-%s--%d->--%s--<<\n", tmp, str_size, ptr1);
//printf("%lld\n", str_size);
			if( str_size == 0 || str_size > MAX_SIZE_OF_NAME ){ printf("WARN:terminal: Length of cgi name: %lld\n", str_size); break;}
			a = (char *)malloc(str_size+2);
			if(a != NULL){
			    parse_cgi_name(tmp, &ptr1, a);
			    get_cgi(0, str_size, a);
			    free(a);
			}
		    
		    break;
		case 20:
		    parse_tbl(tmp, 0);//without clean
		    break;
		case 21:
		    print_str(fp, tmp);//string:  "??variable?? \"text\""
		    break;
		case 22:
		    //digits parse
		    digi = value_;
		    if(digi > 255){ printf("char #%ld is > 255 in file %s\n", tmp-(f_n->begin), f_n->file_name);}
		    putc(digi % 256, fp);
		    break;
		case 23:
		    putc(27, fp);	//0x1b = ESC
		    break;

	    }//switch(num)
	    }//if(make)
	    data = restore_str(&parser);
	    continue;
	}//if(parsestr)


	if(*data == '$'){			//insert a string parameter
	    data++;
	    tmp = data;
//	    while(*tmp && *tmp != ' ' && *tmp != '\t' && *tmp != '\n' && *tmp != '\r' && *tmp != '\"'){tmp++;}
	    while(*tmp && *tmp != ' ' && *tmp != '\t' && *tmp != '\n' && *tmp != '\r' && *tmp != '\"'){
		if(*tmp == ':' && *(tmp+1) == '\"') {
			    tmp+=2;		//string
			    while(*tmp){
				if(*tmp == '\"' && *(tmp-1) != '\\'){	//not \"
				    tmp++;
				    break;
				}
			    tmp++;
			    }
		break;
		}
		tmp++;
	    }

//function(data, tmp-data);
	    if(make != 0){
		str_size = tmp-data;
		if((ptr1 = malloc(str_size+10)) != NULL){
		    strncpy(ptr1, data, str_size);
		    ptr1[str_size] = '\0';
		//show variables value ->  $name:size:"parse_str" next_parameter
		    if((name = w_strtok(&ptr1, ':')) != NULL){
			if(a = get_var(&str_size, name)){
			    digi = 0;
			    if((digit = w_strtok(&ptr1, ':')) != NULL){
				//digits parse
				i = 0;
				while(digit[i] >= '0' && digit[i] <= '9' /*&& digi*10*/){
				    digi = digi*10 + (digit[i] - '0');
				    i++;
				    if(i > 2) break;
				}
			    }
			    if(str_size != 0) str_size = MIN(digi, str_size-1);
			    i = 0;
			    if((ptr2 = parsestr(ptr1, "\"/[/*/N\\N/]\"/")) != NULL){
				a = parsestr2(&parser, NULL, a, ptr2);
				if(a != NULL) i = 1;//used for restoring of string after
			    }

			    if(a != NULL){//printout!
				if(str_size != 0){
				    p = a + str_size;
				    while(*a && a < p){
					putc(*a, fp);
					a++;
				    }
				}else fprintf(fp, "%s", a);
			    if(i) restore_str(&parser);
			    }else printf("parsestring: %s not match!\n", ptr1);
			}else printf("Name: %s not found!\n", name);
		        free(name);
		    }
		//show variables value
/*		    if(a = get_var(NULL, ptr1)){
			//fprintf(fp, "%s", a);
			print_1(fp, a);
		    }else printf("Name: %s not found!\n", ptr1);
		    free(ptr1);
*/
		}
	    }
	    data = tmp;
	    continue;

	}else if(*data == '#'){			//insert a digit parameter like: > 100 "HALLOw" 0 > "AL" 0 0
	    data++;
	    tmp = data;
	    while(*tmp && *tmp != ' ' && *tmp != '\t' && *tmp != '\n' && *tmp != '\r' && *tmp != '\"'){tmp++;}
	    if(make != 0){
		str_size = tmp-data;
		if((ptr1 = malloc(str_size+10)) != NULL){
		    strncpy(ptr1, data, str_size);
		    ptr1[str_size] = '\0';
		    //if(a = get_cfg_value(NULL, ptr1, 0)){		//give 'value' and show
		//show variables value
		    if(a = get_var(NULL, ptr1)){
			print(fp, a);
		    }else printf("Name: %s not found!\n", ptr1);
		    free(ptr1);
		}
	    }
	    data = tmp;
	    continue;
/*** big IF configuration ***/
	}else if(tmp = parsestr2(&parser, NULL, data, "if/t\"/[/*/N\\N/]\"")){
	    mk = 1;
	    if(make){
		if(cfg_arg_strcmp(tmp, 0)){//par -not exist or not match
		    mk = 0;
		}
	    }
	    data = restore_str(&parser);
	    data = parse_file_(data, f_n, (make && mk), loop + 1);
	    if(tmp = parsestr(data, "else/ ")){
		if(mk) mk = 0;
		else mk = 1;
		data = parse_file_(tmp, f_n, (make && mk), loop + 1);
	    }
	    continue;
	}else if(tmp = parsestr2(&parser, NULL, data, "ifnot/t\"/[/*/N\\N/]\"")){
	    mk = 1;
	    if(make){
		if(cfg_arg_strcmp(tmp, 1)){//par -not exist or match
		    mk = 0;
		}
	    }
	    data = restore_str(&parser);
	    data = parse_file_(data, f_n, (make && mk), loop + 1);
	    if(tmp = parsestr(data, "else/ ")){
		if(mk) mk = 0;
		else mk = 1;
		data = parse_file_(tmp, f_n, (make && mk), loop + 1);
	    }
	    continue;
	}else if(*data == 'f' && *(data+1) == 'i'){
	    data += 2;
	    if(loop) return data;
	    else {
		printf("main: \"if\" not present!\n");
		continue;
	    }
	} else if(tmp = parsestr(data, "else")){	//return for inversing make.
	    if(loop) return data;
	    else {
		printf("main: else without if\n");
		data = tmp;
		continue;
	    }
/*** end of IF configuration ***/
	}else if(*data == '<'){
	    data++;
	    if(make == 1 && (tmp = parse_cgi_script(data)) != NULL){
		//tmp = functon(data);
		data = tmp;
	    }else{
		tmp = parsestr(data, "/{*/B\"\"/\\\"/*/N\\N\"/\\"
					"[/{*/B\"\"/\\\"/*/N\\N\"/\\table:/*\n#end_table/\\//*/**///\\/////*\n/\\/|/E/}]"
					"/\\//*/**///\\/////*\n/--/\\/|/E/}\nend");
		if(tmp){data = tmp;}
		else {printf("WARN: cgi at char: %ld in file: %s is not complet\n", data - (f_n->begin), f_n->file_name);
		//maybe return
		}
	    }
	    continue;
	}else if(tmp = parsestr2(&parser, NULL, data, "tbl_size:/t\"/[/*/]\"")){
	    if(make == 1){
		pt = get_tbl_begin_size(tmp, 1);//show count of begin parameter of tbl
		if(pt) fprintf(fp, "%d", *pt);//show as digit
		else fprintf(fp, "NE");//not exist
	    }
	    data = restore_str(&parser);
	    continue;
	}

	putc(*data, stderr);//check the cleaning
	data++;
    }

    if(loop) printf("main: \"fi\" not present!\n");
    return data;
}

//return: 0 - OK, 1 - Err open file, 2 - Err allocate
int parse_file(char *file_name, char flag){
	FILE *f;
	char *ptr;
	unsigned long long str_size;
	struct stat stbuf;
	struct page_n **p1;
	struct file_n f_n;


//limit size of file to 64kb
	if(stat(file_name, &stbuf) || stbuf.st_size > 1024*64 || (f=fopen(file_name, "r")) == NULL){
	    printf("Error open file: %s\n", file_name);
	    return 1;
	}
	ptr = (char *)malloc(stbuf.st_size+1);
	if(ptr == NULL){
	    printf("ERR: Allocate memory\n");
	    fclose(f);
	    return 2;
	}

	fread(ptr, stbuf.st_size, 1, f);
	ptr[stbuf.st_size] = '\0';
	fclose(f);

	f_n.file_name = file_name;
	f_n.begin = ptr;//set the begin of file
	A1_BTN = 0;//for test here


/*
flag == 0 - call parse_file, or boot_page(CGI)
		clear all parameters, tables. Clear CGI's and Pages up from matched name of file and register new CGI's,
		pars and tables.
flag == 1 - call include(parse_file)
		do not clear parameters and tables. Ignor registring new Page. Register new CGI's, pars and tables.
flag == 2 - call include(CGI)
		do not clear parameters and tables. Register new Page, do not remove last Page.
		If Page exist - clear all Pages next of that. and register new CGI's and pars and tables
*/
    if(flag != 1){
	p1 = &p_n;
	while(1){
	    if(*p1 == NULL){
		*p1 = (struct page_n *)malloc(sizeof(struct page_n));
		if(*p1 != NULL){
		    str_size = strlen(file_name) + 1;
		    (*p1)->name = malloc(str_size + 1);
		    if((*p1)->name){
			strncpy((*p1)->name, file_name, str_size);
			(*p1)->size = str_size;
			(*p1)->next = NULL;
			(*p1)->cfg = NULL;
			cfg_p = &((*p1)->cfg);
			(*p1)->tbl_name = NULL;
			tbl_name = &((*p1)->tbl_name);
			(*p1)->cgi_name = NULL;
			cgi_name = &((*p1)->cgi_name);
			break;
		    }
		    free(*p1);
		    *p1 = NULL;
		}
		printf("Error allocate memory_page\n");
		free(ptr);
		return 1;
	    }
	    if((*p1)->name && !strcmp((*p1)->name, file_name)){	//name is existing
		free_page_n((*p1)->next);	//clear next pages
		(*p1)->next = NULL;
		free_par((*p1)->cfg);		//clear all parameters
		(*p1)->cfg = NULL;
		cfg_p = &((*p1)->cfg);
		free_tbl((*p1)->tbl_name);	//clear all tabs
		(*p1)->tbl_name = NULL;
		tbl_name = &((*p1)->tbl_name);
		free_cgi((*p1)->cgi_name);	//clear all cgi's
		(*p1)->cgi_name = NULL;
		cgi_name = &((*p1)->cgi_name);
		break;
	    }
	    if(flag == 0 && (*p1)->next == NULL){	//entries of names still remain
		free_par((*p1)->cfg);		//clear all parameters
		(*p1)->cfg = NULL;
		free_tbl((*p1)->tbl_name);	//clear all tabs
		(*p1)->tbl_name = NULL;
		free_cgi((*p1)->cgi_name);	//clear all cgi's
		(*p1)->cgi_name = NULL;
//		cgi_name = &((*p1)->cgi_name);
	    }
	p1 = &((*p1)->next);
	}
    }

	parse_file_(ptr, &f_n, 1, 0);
	free(ptr);
	return 0;	//all ok
}


void sighup(int signo){		//not done !!
//    if(fd != -1) close(fd);
//    close(sockfd);
//    ReadConfiguration1();
//    main();
}

void sig_handler(int signo){
    switch (signo){
    case SIGINT:
    case SIGABRT:
    case SIGTERM:
        printf("TERMINATE or ABORT\n");
	break;
//#ifdef DEBUG
    case SIGALRM:
    /* got an alarm, exit & recycle */
	printf("Alarm!!\n");
	auth[0] = '1';
	auth[1] = '\0';
	return;
	//break;
    default: printf("SIgnal %d\n", signo);
//#endif
    }
    fclose(fp);
//    free_page_mem();	//clear all memory used for page
    free_page_n(p_n);
    exit(0);
}

char *line[] = {
			"##index\n",		//0
			"##getTime\n",		//1
			"##setTime: ",		//2
			"button",		//3
			"timeout",		//4
			"break",
			"paSsWord",		//6
			"script",		//7
			"##setBuffer",		//8
			"##getBuffer",		//9
			(char []) {27, 'A', 1, '\0'},	//10
			(char []) {27, 'H', 3, '\0'},	//11
			(char []) {27, 'V', '\0'},	//12
			"getCard",		//13
			(char []) {27, 'B', 2, '\0'},	//14
			NULL };

int read_buf__(char *buff){
//    printf("%d\n", sizeof(buf));

    int n = 0;
    char size = 0;
    unsigned long long buf_s = 0;
    int i = 0, k, time;
    char brk[] = "\n\n\rbreAk\r\rbReaK ";

    while(1){

    if(line[n] != NULL && line[n][i] == '\0'){


	printf("Func: %d\n", n);
	switch(n){
	    case 0:
		    //print ">SV";
		    //get dspl-version "<V,len,string"
		    parse_file(INDEX_NAME, 0);
		    break;
	    case 1: //fprintf(fp, "07121035142015");//%m%d%H%M%S20%Y
		    my_system(fp, "date \"+%m%d%H%M%S%Y\"");
		    break;
	    case 12:
		    size = getc(fp);
		    memset(version, 0, 256);
		    if(size == 0){ printf("size zero!!\n"); break;}
		    i = 0;
		    do{
			version[i] = getc(fp);
			i++;
			size--;
		    }while(size);
		    printf("version: %s\n", version);
		    get_cgi(0, 7, "version");		//< "version" -will be started
		    break;
	}

	if(auth[0] >= '0' && auth[0] < '4' && auth[1] == '\0')
	switch(n){

	    case 2:
		    //time set %m%d%H%M%S20%Y -> 'date %m%d%H%M20%Y.%S`
		    k = 0;	//14 chars length -is parameter +1char='\n'
		    while(k < 15){
			buff[k] = getc(fp);
			if((buff[k] < '0' || buff[k] > '9') && (k != 14 || buff[k] != '\n')){
			    printf("set time - not in range: 0-9\n");// k = 0; continue;
			}
			k++;
		    }
		    sprintf(buff, "date %c%c%c%c%c%c%c%c%c%c%c%c.%c%c", buff[0], buff[1], buff[2], buff[3], buff[4], buff[5],
			    buff[6], buff[7], buff[10], buff[11], buff[12], buff[13], buff[8], buff[9]);
		    system_(buff);
		    break;
	    case 3:
	    case 4:
	    case 5:
	    case 7:
	    case 13:
//printf("CT:%s - %d\n", line[n], strlen(line[n]));
		    get_cgi(0, strlen(line[n]), line[n]);	//('A', 1, buf+1)
		    break;
	    case 6:
	    case 8:
		    buf_s = (getc(fp) << 8) + getc(fp);
		    if(buf_s == 0) {
			if( n == 8) buf_s = 65536;
			else{ buf_size = 0; break;}
		    }
		    printf("here must be a password:%lld\n", buf_s);
		    buf_size = buf_s;
		    i = 0;
		    do{
			buff[0] = getc(fp);
			buf[buf_size - buf_s] = buff[0];
			if(buff[0] == 0){
			    if (n != 8) { buf_size = buf_size - buf_s; break;}
			    else{
				//n = 8
				i++;//at i=1
				do{
				    buff[i] = getc(fp);
				    if(buff[i] != brk[i-1]){// if not "\0BbrREeaK" - put out to buffer!!
					k = 1;
					while(k <= i){
					    if(buf_size - buf_s + k > 65536){ buf_s = 1; break;}
					    buf[buf_size - buf_s + k] = buff[k];
					    k++;
					}
					if((k > i) && (buf_s > i)) buf_s = buf_s - i;
					break;
				    }
				    i++;
				}while(brk[i-1] != '\0');
				if(brk[i-1] == '\0'){ printf("break copy\n"); buf_size = 0; break;}	//clear and goout
				else { i = 0;}
			    }
			}
			buf_s--;
		    }while(buf_s != 0);
		    if(n == 8 && i == 0) get_cgi(0, 9, "setBuffer");	//('A', 1, buf+1)
		    break;
	    case 10:
	    case 11:
	    case 14:
		    //size = getc(fp);
		    size = buff[i-1];
		    //if(size == 0){ printf("size zero!!\n"); break;}
		    k = i;
		    do{
			buff[i] = getc(fp);
			i++;
			size--;
		    }while(size);
		    printf("button: %d\n", buff[k]);
		    get_cgi(buff[k-2], buff[k-1], buff+k);		//char, num, buf: ('A', 1, buf+1)
		    break;
	}//switch
	n = 0; i = 0;	//clear search parameters
	fflush(fp);
	if(auth[0] == '4' && auth[1] == '\0'){
	    time = alarm(0);
	    if(time) alarm(time);
	    else alarm(30);
	}
    }

    buff[i] = getc(fp);
//let it here for check of inputs, see bugfix for details
printf("%c i=%d, n=%d size of buff:%d\n", buff[i], i, n, sizeof(buff));

    if(line[n] != NULL){
	if(buff[i] == line[n][i]){
	    i++;
	    continue;
	}else{
	    n++;
	    k = 0;
	    while(k <= i && line[n] != NULL){
		if(buff[k] != line[n][k]){
		    n++;
		    k = 0;
		    continue;
		}
	    k++;
	    }

	    if(line[n] == NULL){//empty (not found in line[]) - go to begining
		i = 0; 
		n = 0;
		continue;
	    }

	    if(k == i+1){	//buff match to line - increase i.
		i++;
		continue;
	    }
	}
    }

/* begin of not-recognized, just print out! */
/*    k = 0;
    do{
	switch(buff[k]){
	    case 27:	printf("ESC ");
			break;
	    default:	printf(" %d", buff[k]);
	}
	k++;
    }while(k <= i);
*/
//    if(i >= 255) i = 0;
printf("!!!never happens!!!\n");
i = 0; n = 0;
    }
}

int main(int argc, char *argv[]){
    FILE *filep;
    char *dev;

    if (argc == 1) {
	dev = "/dev/ttyUSB0";
    } else dev = argv[1];

//    char *dev = "/dev/ttyUSB0";

    printf("Starting main()!! %s\n", dev);

    signal(SIGCHLD, SIG_IGN);	//ignore
    signal(SIGPIPE, SIG_IGN);	//ignore
    signal(SIGALRM, sig_handler);	//hang functions on signals

    signal(SIGTERM, sig_handler);	//catch all signals of termination for freeing memory
    signal(SIGABRT, sig_handler);
    signal(SIGKILL, sig_handler);//mayby not used
    signal(SIGINT, sig_handler);
//andy
    signal(SIGHUP, sighup);
//end


//    ReadConfiguration1();

    etc_save[0] = '0';		//set etc_save=0
    etc_save[1] = '\0';

    if((filep=fopen("/tmp/termlock","r")) != NULL){
	auth[0] = '4';
	fclose(filep);
    } else auth[0] = '1';

    auth[1] = '\0';
    buf_size = 0;

    chdir( ROOT_PATH );
//    chdir(CONFIG.WEB_ROOT);		//just use it!!

	if((fp=fopen(dev,"r+")) == NULL){
	    printf("Error open device: %s\n", dev);
	    exit(5);
	}

	read_buf__(buffer);
}

