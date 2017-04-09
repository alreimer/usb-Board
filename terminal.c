/* terminal.c:  A very simple connection programm for USB-Board
 *
 * Copyright (c) 2015 Alexander Reimer <alex_raw@rambler.ru>
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
//#include "include/httpd_config.h"	//cfg_parse1
#include "term_parser.h"
#include "term_add.h"
#include "term_cgi.h"

FILE *fp;
char buffer[2078];

/* Global variables */
char etc_save[2];
//char buf[16384];
char buf[65537];//65536 + 1
unsigned long long buf_size = 0;
char *referrer = NULL, *ref_ = NULL;
char version[256];
char value[16];

#define MIN(a,b) ((a) < (b) ? (a) : (b))


int copy_file(char *file, FILE *out)
{
  FILE *fp;
  int n;
  int wrote;
  char *err_str="[ERR] File: %s cannot open\n";

  if((fp=fopen(file,"r")) == NULL){
    printf(err_str, file);
    fprintf(out, err_str, file);
    return 1;
  }
  else{
    while ((n = fread(buffer,1,sizeof(buffer),fp)) != 0) {
	wrote = fwrite(buffer,n,1,out);
	if (wrote < 1){
	    fclose(fp);
	    return 1;
	}
    }
    fclose(fp);
  }
  return 0;
}

struct cfg_parse1 *cfg1 = NULL;	/*new config via web*/
struct cfg_parse1 **cfg_p = &cfg1;


//clear all parameters
void free_par(struct cfg_parse1 **ptr){
	if(!ptr || !*ptr) return;
	free_par(&((*ptr)->next));
printf("FREEpar %s\n", (*ptr)->str);
	if((*ptr)->str) free((*ptr)->str);
	free(*ptr);
	*ptr = NULL;
}

void free_page_mem(void){
    free_par(&cfg1);
    cfg_p = &cfg1;

    free_cgi(cgi_name);
    cgi_name = NULL;
    free_tbl();
}


int parse_file(char *file_name, char flag){
//copy_file(file_name, fp);
	FILE *f;
	char *ptr, *ptr1, *data, *tmp, *a, ch, i;//ptr - is busy
	unsigned long long digi, str_size;
	int make = 1;
	struct stat stbuf;
	struct cfg_parse1 *cfg_pointer;
	struct parsestr parser;
	char *name, *digit, *p, *ptr2;
//	struct parsestr strct;



//limit size of file to 64kb
	if(stat(file_name, &stbuf) || stbuf.st_size > 1024*64 || (f=fopen(file_name, "r")) == NULL){
	    printf("Error open file: %s\n", file_name);
	    return 1;
	}
	ptr = (char *)malloc(stbuf.st_size+1);
	if(ptr == NULL){
	    printf("ERR: Allocate memory\n");
	    fclose(f);
	    return 1;
	}

	data = ptr;
	fread(data, stbuf.st_size, 1, f);
	data[stbuf.st_size] = '\0';
	fclose(f);



    if(flag == 0){
	if(referrer) free(referrer);
	referrer = ref_;
	str_size = strlen(file_name) + 1;
	ref_ = malloc(str_size + 1);
	if(ref_) strncpy(ref_, file_name, str_size);

	free_page_mem();	//flag == 0
    }

    while((data != NULL) && *data){

	ch = *data;
	if(*data == '/'){
	    data++;
	    if(*data == '/') {do{ data++; if(*data == '\n'){ data+=1; break;}} while(*data); continue;}	//comment //...
	    if(*data == '*') {do{ data++; if(*data == '*'&& *(data+1) == '/'){ data+=2; break;}} while(*data); continue;}	//comment /*...*/
	    data--;
	}
	if(*data == ' ' || *data == '\t' || *data == '\n' || *data == '\r') {data++;continue;}

/*** par -parameter ***/
	else if(tmp = parsestr1(data, "par:/t")){
	    data = tmp;
	    while(*tmp && *tmp != '\n' && *tmp != '\r'){tmp++;}
//size=tmp-data
//insert
	i = 0;
	digi = 0;
	while(data[i] >= '0' && data[i] <= '9' /*&& digi*10*/){
	    digi = digi*10 + (data[i] - '0');
	    i++;
//	    if(i > 3) break;
	}
	if(!digi || digi > 300) {data = tmp; continue;}	//size is in impassable range - so skip it all (whole parameter string will be skipped!)

	*cfg_p = (struct cfg_parse1 *)malloc(sizeof(struct cfg_parse1));

	cfg_pointer = *cfg_p;
	if(cfg_pointer == NULL){ data = tmp; continue;}

	str_size = tmp - data - i - 1;		//   v - (2chars) is for shore
	ptr1 = (char*)malloc(str_size + 1 + 2*digi + 2);//str = "[part of string][value][new_value]"
	if(ptr1 == NULL){
	    printf("ERROR allocate memory\n");
	    if(cfg_p && *cfg_p){
		free(*cfg_p);
		*cfg_p = NULL;	//main criteria to abort moving in array.
	    }
	    data = tmp;
	    continue;
	}

	strncpy(ptr1, data + i + 1, str_size);//256:web_name:name:pattern
	ptr1[str_size] = '\0';

	cfg_pointer->type = CFG_PAR;
	cfg_pointer->str = ptr1;
	cfg_pointer->size = digi;
	cfg_pointer->changed = 0;
	cfg_pointer->position = 0;
	cfg_pointer->saved = 0;
	cfg_pointer->value = ptr1 + str_size+1;
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
	cfg_p = &(cfg_pointer->next);
//insert



	    data = tmp;
	    continue;
	}//else if(!strncmp)
/** area:size:web_name **/
	else if(tmp = parsestr1(data, "area:/t")){
	    data = tmp;
	    while(*tmp && *tmp != '\n' && *tmp != '\r'){tmp++;}
//size=tmp-data
//insert
	i = 0;
	digi = 0;
	while(data[i] >= '0' && data[i] <= '9' /*&& digi*10*/){
	    digi = digi*10 + (data[i] - '0');
	    i++;
//	    if(i > 3) break;
	}
	if(digi > 64*1024) {data = tmp; continue;}	//size is in impassable range - so skip it all (whole parameter string will be skipped!)

	*cfg_p = (struct cfg_parse1 *)malloc(sizeof(struct cfg_parse1));

	cfg_pointer = *cfg_p;
	if(cfg_pointer == NULL){ data = tmp; continue;}

	str_size = tmp - data - i - 1;		// v - (2chars) is for shore
	ptr1 = (char*)malloc(str_size + 1 + digi + 2);//str = "[part of string][value]"
	if(ptr1 == NULL){
	    printf("ERROR allocate memory\n");
	    if(cfg_p && *cfg_p){
		free(*cfg_p);
		*cfg_p = NULL;	//main criteria to abort moving in array.
	    }
	    data = tmp;
	    continue;
	}

	strncpy(ptr1, data + i + 1, str_size);//web_name
	ptr1[str_size] = '\0';

	cfg_pointer->type = (digi != 0) ? CFG_AREA:CFG_TMP;
	cfg_pointer->str = ptr1;
	cfg_pointer->size = digi;
	cfg_pointer->changed = 0;
	cfg_pointer->position = 0;
	cfg_pointer->saved = 0;
	if(digi != 0){
	cfg_pointer->value = ptr1 + str_size+1;
//	*(cfg_pointer->value) = '\0';
	cfg_pointer->new_value = ptr1 + str_size + 1 + digi;
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

printf("Collected parameter: --%s--%lld--\n", cfg_pointer->web_name, cfg_pointer->size);
	cfg_p = &(cfg_pointer->next);
//insert
	    data = tmp;
	    continue;
	}//else if(!strncmp)

	else if(tmp = parsestr1(data, "readcfg")){
//	else if(!strncmp(data, "readcfg", 7)){
//	    data += 7;
	    data = tmp;
	    ReadConfiguration();
	    continue;
	}


//digits parse
	i = 0;
	digi = 0;
	while(data[i] >= '0' && data[i] <= '9' /*&& digi*10*/){
	    digi = digi*10 + (data[i] - '0');
	    i++;
	    if(i > 2) break;
	}
	if(i != 0){
	    if(digi > 255){ printf("char #%d in file %s is > 255\n", data-ptr, file_name);}
	    if(make) putc(digi % 256, fp);
//printf("-- %d --\n", digi);
	    data = data + i;
	    if(*data != ' ' && *data != '\t' && *data != '\r' && *data != '\n' && *data != '\0') printf("char #%d in file: %s is not correct separated!\n", data-ptr, file_name);
	    continue;
	}

	if(*data == '>') ch = 27; //0x1b = ESC

	else if(*data == '\"') {data++;		//string:  "??variable?? \"text\""
			    tmp = data;
			    while(*tmp){
				if(*tmp == '\"' && *(tmp-1) != '\\'){	//not \"
//				    tmp++;
				    break;
				}
//printf("%c", *tmp);
//			    if(make) putc(*tmp, fp);
			    tmp++;
			    }
	    i = *tmp; if(i != '\0'){ if(make){*tmp = '\0'; print_str(fp, data); *tmp = i;} tmp++;}
	    data = tmp;
	    continue;

	}else if(*data == '$'){			//insert a string parameter
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
			    if((ptr2 = parsestr1(ptr1, "\"/[/*/N\\N/]\"/")) != NULL){
				a = parsestr2(&parser, a, ptr2);
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
	}else if(tmp = parsestr2(&parser, data, "if/t\"/[/*/]\"")){
	    if(cfg_arg_strcmp(tmp, 0)){//par -not exist or not match
		make = 0;
	    }
	    data = restore_str(&parser);
	    continue;
	}else if(tmp = parsestr2(&parser, data, "ifnot/t\"/[/*/]\"")){
	    if(cfg_arg_strcmp(tmp, 1)){//par -not exist or match
		make = 0;
	    }
	    data = restore_str(&parser);
	    continue;
	}else if(*data == 'f' && *(data+1) == 'i'){
	    data += 2;
	    make = 1;
	    continue;
/*** end of IF configuration ***/
	}else if(*data == '<'){
	    data++;
	    if(make == 1 && (tmp = parse_cgi_script(data)) != NULL){
		//tmp = functon(data);
		data = tmp;
	    }else{
		data = cgi_end(data);
	    }
	    continue;
	}
	else if(a = parsestr1(data, "shell:/t")){
//	    while(*a == ' ' || *a == '\t'){a++;}
	    tmp = a;
	    while(*tmp && *tmp != '\n' && *tmp != '\r'){tmp++;}
	    str_size = tmp - a;
	    if(str_size != 0){
		ptr1 = (char *)malloc(str_size + 2);
		if(ptr1){
		    strncpy(ptr1, a, str_size);
		    ptr1[str_size] = '\0';
//printf("shell--%s--\n", ptr1);
		    my_shell(fp, ptr1);
		    free(ptr1);
		}
	    }else{ printf("WARN: shell line, at char %d, is empty!\n", data-ptr);}

	    data = tmp;
	    continue;
	}
	else if(a = parsestr1(data, "cgi:/t")){
//	    while(*a == ' ' || *a == '\t'){a++;}
//	    tmp = a;
	    str_size = parse_cgi_name(a, &ptr1, NULL);//ptr1 - is end(\n) of cgi-string
//printf("-%s--%d->--%s--<<\n", a, str_size, ptr1);
//printf("%lld\n", str_size);
	    if( str_size == 0 || str_size > MAX_SIZE_OF_NAME ){ printf("WARN:terminal: Length of cgi name: %lld\n", str_size); data = ptr1; continue;}
	    tmp = (char *)malloc(str_size+2);
	    if(tmp != NULL){
		parse_cgi_name(a, &ptr1, tmp);
		get_cgi(0, str_size, tmp);
		free(tmp);

	    }
	    data = ptr1;
	    continue;
	}else if(tmp = parsestr2(&parser, data, "include:/t/[/*/]\n")){
	    if(make == 1){
		parse_file(tmp, 1);	//parse file w/o erease of other parameters
	    }
	    data = restore_str(&parser);
	    continue;
	}else if(tmp = parsestr2(&parser, data, "wr_shell:/t/[/*/]\n")){
	    if(make == 1){
		wr_shell(tmp, 0);	//write to shell
	    }
	    data = restore_str(&parser);
	    continue;
	} else if(tmp = parsestr2(&parser, data, "wr_par:/t/[/*/]\n")){	//wr_par: par:value

		if(make == 1){
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
		}
	    data = restore_str(&parser);
	    continue;
	}else if(tmp = parsestr2(&parser, data, "table:/t/[/*/]#end_table")){
	    if(make == 1){
		parse_tbl(tmp, 0);//without clean
	    }
	    data = restore_str(&parser);
	    continue;
	}



if(make) putc(ch, fp);
	data++;
    }


	free(ptr);
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
	break;
    default: printf("SIgnal %d\n", signo);
//#endif
    }
    fclose(fp);
    free_page_mem();	//clear all memory used for page
    if(referrer) free(referrer);
    if(ref_) free(ref_);
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
    int i = 0, k;
    char brk[] = "\n\n\rbreAk\r\rbReaK ";

    while(1){

    if(line[n] != NULL && line[n][i] == '\0'){
	printf("Func: %d\n", n);

	switch(n){
	    case 0:
		    //print ">SV";
		    //get dspl-version "<V,len,string"
		    parse_file("/term/index.term", 0);
		    break;
	    case 1: //fprintf(fp, "07121035142015");//%m%d%H%M%S20%Y
		    my_system(fp, "date \"+%m%d%H%M%S%Y\"");
		    break;
	    case 2:
		    //time set %m%d%H%M%S20%Y -> 'date %m%d%H%M20%Y.%S`
		    k = 0;	//14 chars length -is parameter +1char='\n'
		    while(k < 15){
			buff[k] = getc(fp);
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
	}//switch
	n = 0; i = 0;		//clear search parameters
    }

    buff[i] = getc(fp);
//printf("%c %d\n", buf[i], i);

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
	    if(k == i+1){		//after loop in n is matched line
		i++;
		continue;
	    }
	}
    }

/* begin of not-recognized, just print out! */
    k = 0;
    do{
	switch(buff[k]){
	    case 27:	printf("ESC ");
			break;
	    default:	printf(" %d", buff[k]);
	}
	k++;
    }while(k <= i);
//    if(i >= 255) i = 0;
i = 0; n = 0;
    }
}

int main(int argc, char *argv[]){

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
    buf_size = 0;

    chdir("/term/");
//    chdir(CONFIG.WEB_ROOT);		//just use it!!

	if((fp=fopen(dev,"r+")) == NULL){
	    printf("Error open device: %s\n", dev);
	    exit(5);
	}

	read_buf__(buffer);
}

