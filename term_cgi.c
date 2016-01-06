/* term_CGI.c:
 *
 * Copyright (C) 2015  Alexander Reimer <alex_raw@rambler.ru>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
//#include <errno.h>//errno
#include <string.h>
#include "term_cgi.h"
#include "term_tbl.h"
#include "term_add.h"
#include "term_parser.h"

char A1_BTN = 0;
struct cgi *cgi_name = NULL;
struct cgi *cgi_used = NULL;	//connect to cgi that is now in use. Used by print()

#define MIN(a,b) ((a) < (b) ? (a) : (b))

unsigned long long parse_cgi_name(char *data, char **tmp, char *buf){
    unsigned long long size = 0;

	char i;
	int digi;


    if(data == NULL) return 0;
    while(*data != '\0' && *data != '\n'){ //*data == '\0'used in boot_cgi "";
// "\"\"" parse
//	if( *data != '\\' && *(data+1) == '\"'){
//	    data +=2;
	if(*data == '\"'){
	    data++;
	    while(*data != '\0'){
		if(*data == '\"' && *(data-1) != '\\'){data++; break;}
		if(buf != NULL) buf[size] = *data;
		size++;
		data++;
	    }
	    continue;
	}
	if(*data == ' ' || *data == '\t'){data++; continue;}

//digits parse
	i = 0;
	digi = 0;
	while(data[i] >= '0' && data[i] <= '9' /*&& digi*10*/){
	    digi = digi*10 + (data[i] - '0');
	    i++;
	    if(i > 2) break;
	}
	if(i != 0){
//	    if(digi > 255){ printf("char #%d in file %s is > 255\n", data-ptr, file_name);}
	    if(buf != NULL) buf[size] = digi;// % 256;
	    size++;
//printf("-- %d --\n", digi);
	    data = data + i;
//	    if(*data != ' ' && *data != '\t' && *data != '\r' && *data != '\n') printf("char #%d is not correct separated!\n", data-ptr);
	    continue;
	}


	data++;
    }
    if(*data != '\0'){*tmp = data + 1;} else {*tmp = data;}
    return (size);// + 1);//'\0' must be included!!

}


char *search[] = {"print",		//1
			"boot_page",
			"savecfg",	//3
			"fill_cfg", //4
			"write_system", //5  par:cmd
			"cat_system", //6:  par:cmd
			"write_char",	//7
			"change_line",	//8
			"write_par",	//9
			"if_changed",	//10
			"not_changed",	//11
			"if",		//12
			"not",		//13
			"shell",	//14
			"my_shell",	//15
			"system",
			"my_system",	//17
			"clean_par",	//18
			"boot_cgi",	//19
			"set_tbl",		//20
			"exit_cgi",	//21
			"copy_ppar",	//22

			NULL
			};

char *cgi_loop(char *data, int *i, int *k, struct cgi *ptr){
    int j, len;
    char *tmp, *tmp1;

    while(data && *data){
	if(*data == '/'){
	    data++;
	    if(*data == '/') {do{ data++; if(*data == '\n'){ data+=1; break;}} while(*data); continue;}	//comment //...
	    if(*data == '*') {do{ data++; if(*data == '*'&& *(data+1) == '/'){ data+=2; break;}} while(*data); continue;}	//comment /*...*/
	    data--;
	}
	if(*data == ' ' || *data == '\t' || *data == '\n' || *data == '\r') {data++;continue;}

	j = 0;
	while(search[j]){
	    len = strlen(search[j]);
	    if(!strncmp(data, search[j], len)){
		data = data + len;
		if(ptr->cmd[*i] == 0) ptr->cmd[*i] = j + 1;
		else printf("ERR:Overwrite '%s' line: %d\n", search[(ptr->cmd[*i])], *i + 1);
//printf("cmd: %s:%d, len %d\n", search[j], *i, len);
//		strncpy(ptr->arg[*i], "", 1);
		break;//see data++;
	    }
	    j++;
	}

	if(*data == '\"') {data++;
			    tmp = data;
			    while(*tmp){
				if(*tmp == '\"' && *(tmp-1) != '\\'){
//				    *tmp = '\0';
				    len = tmp-data;
				    if(ptr->arg[*i] == NULL){
				    ptr->arg[*i] = (char *)malloc(len+2);
				    if(ptr->arg[*i] != NULL){
					strncpy(ptr->arg[*i], data, len);
					*(ptr->arg[*i] + len) = '\0';
				    }
				    }else printf("ERR: Overwrite to \"\" line: %d\n", *i + 1);
//printf("arg: %s", ptr->arg[*i]);
				    tmp++;
				    break;
				}
			    tmp++;
			    }
	    data = tmp;
	    continue;
	}	//string

	if(*data == '[') {data++;
			    tmp = data;
			    while(*tmp){
				if(*tmp == '\"'){
				    tmp++;
				    while(*tmp){if(*tmp == '\"' && *(tmp-1) != '\\'){tmp++; break;} tmp++;}
				    continue;
				}
				if(tmp1 = parsestr1(tmp, "table:/*#end_table")){//table entry
				    tmp = tmp1;
				    continue;
				}
				if(*tmp == ']'){
//				    *tmp = '\0';
				    len = tmp-data;
				    if((ptr->arg[*i]) == NULL){
				    ptr->arg[*i] = (char *)malloc(len+2);
				    if(ptr->arg[*i] != NULL){
					strncpy(ptr->arg[*i], data, len);
					*(ptr->arg[*i] + len) = '\0';
				    }
				    }else printf("ERR: Overwrite to [] line: %d\n", *i + 1);
				    tmp++;
//printf("arg: %s", ptr->arg[*i]);
				    break;
				}
			    tmp++;
			    }
	    data = tmp;
	    continue;
	}	//string []
	if(*data == ';') {data++; (*i)++;if(*i >= GET_CGI_MAX) return NULL; ptr->cmd[*i] = 0; continue;}

	if(*data == 'd' && *(data+1) == 'o'){		// do{ }
	    j = 2; while(*(data+j) == ' ' || *(data+j) == '\t' || *(data+j) == '\n'){ j++; }
	    if(*(data+j) == '{') {data=data+j+1; j = *i; tmp = cgi_loop(data, i, k, ptr);
		if(tmp) ptr->bb[*i] = j; //tmp replace via data
//printf("bb: %d, to %d\n",*i, ptr->bb[*i]);
		data = tmp;
		if(*k == 0) continue; else return data;
	    }
	}
	
	if(*data == '{') {data++; j = *i; (*i)++; if(*i >= GET_CGI_MAX) return NULL; tmp = cgi_loop(data, i, k, ptr); //return NULL- is a problem!!
	    if(tmp) ptr->bb[j] = *i; //tmp replace via data
//printf("bb: %d, to %d\n",j, ptr->bb[j]);
	    data = tmp;
	    if(*k == 0) continue; else return data;
	}
	if(*data == '}'){data++; return data;
	}
	if(!strncmp(data, "end", 3)){data+=3; break;}
#ifdef DEBUG
putchar(*data);
#endif
	data++;
    }
*k = 1;
return data;
}

char *parse_cgi_script(char *data){


    int i = 0, k = 0;
    char *tmp, tmp1[30], *end;
    struct cgi **ptr;
    unsigned long long size;

size = parse_cgi_name(data, &end, NULL);
//printf("%lld\n", size);

//    tmp = w_strtok(&data, '\n');
//    if(!tmp){ printf("Unable to def. length\n"); return NULL;}
    if( size == 0 || size > MAX_NAME_SIZE ){ printf("Length of cgi name: %lld\n", size); return NULL;}
    parse_cgi_name(data, &end, tmp1);
    ptr = &cgi_name;
    while(*ptr){
	if((*ptr)->name && !strncmp((*ptr)->name, tmp1, size))return NULL;	//if name exist - skip parsing!
	ptr = &((*ptr)->next);		//add cgi struct to the end
    }
    *ptr = (struct cgi *)malloc(sizeof(struct cgi));
    if(*ptr == NULL){
	printf("ERR: allocate memory\n");
	return NULL;
    }
    strncpy((*ptr)->name, tmp1, size);
    (*ptr)->name_size = size;
    (*ptr)->next = NULL;
//printf("script %s %d\n", (*ptr)->name, *((*ptr)->name+1));

	while(i < GET_CGI_MAX){		//clear all cgi_lines
	    (*ptr)->cmd[i] = 0;
	    (*ptr)->arg[i] = NULL;
	    //strncpy((*ptr)->arg[i], "", 1);
	    (*ptr)->bb[i] = i;
	    i++;
	}
//	(*ptr)->data_ptr = NULL;

	i = 0;
	if((tmp = cgi_loop(end, &i, &k, *ptr)) == NULL){	//if something wrong - erase entry of this cgi-script
	    i = 0;
	    while(i < GET_CGI_MAX){		//clear all cgi_lines
		if((*ptr)->arg[i] != NULL){ free((*ptr)->arg[i]); /*(*ptr)->arg[i] = NULL;*/}
		i++;
	    }
	    free(*ptr);
	    *ptr = NULL;
	}else return tmp;
    return NULL;
}

char *cgi_end(char *data){
    while(*data != '\0'){
	if(*data != '\\' && *(data+1) == '\"'){
	    data +=2;
	    while(*data != '\0'){
		if(*data == '\"' && *(data-1) != '\\'){break;}
		data++;
	    }
	} //else
	if(!strncmp(data, "end", 3)){data+=3; return data;}

	data++;
    }
    return data;
}

int print(FILE *out, char *data){

    static int loop_print = 0;	// used if loops exissted
    if(loop_print > 10){
	fprintf(out, "max loop counter reached\n");
	return 1;
    }
    loop_print++;

    print_(out, data, 1, 0);//make = 1, loop = 0

    loop_print--;
}

char *print_(FILE *out, char *data, int make, int loop){

	char *tmp, *ptr = data, *ptr1, *a, ch, i;
	char *name, *digit, *p, *ptr2;
	int mk;
	struct parsestr strct;
	unsigned long long digi, str_size;

    while((data != NULL) && *data){

	if(*data == '/'){
	    data++;
	    if(*data == '/') {do{ data++; if(*data == '\n'){ data+=1; break;}} while(*data); continue;}	//comment //...
	    if(*data == '*') {do{ data++; if(*data == '*'&& *(data+1) == '/'){ data+=2; break;}} while(*data); continue;}	//comment /*...*/
	    data--;
	}
	if(*data == ' ' || *data == '\t' || *data == '\n' || *data == '\r') {data++;continue;}

//digits parse
	i = 0;
	digi = 0;
	while(data[i] >= '0' && data[i] <= '9' /*&& digi*10*/){
	    digi = digi*10 + (data[i] - '0');
	    i++;
	    if(i > 2) break;
	}
	if(i != 0){
	    if(digi > 255){ printf("char #%d is > 255\n", data-ptr);}
	    if(make) putc(digi % 256, out);
//printf("-- %d --\n", digi);
	    data = data + i;
	    if(*data != ' ' && *data != '\t' && *data != '\r' && *data != '\n') printf("char #%d is not correct separated!\n", data-ptr);
	    continue;
	}

	ch = *data;
	if(*data == '>') ch = 27; //0x1b = ESC

	else if(*data == '\"') {data++;		//string
			    tmp = data;
			    while(*tmp){
				if(*tmp == '\"' && *(tmp-1) != '\\'){	//not \"
//				    tmp++;
				    break;
				}
//printf("%c", *tmp);
//			    putc(*tmp, out);
			    tmp++;
			    }
	    i = *tmp; if(i != '\0'){ if(make){*tmp = '\0'; print_str(out, data); *tmp = i;} tmp++;}
	    data = tmp;
	    continue;

	}else if(*data == '$'){			//insert a string parameter
	    data++;
	    tmp = data;
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
				a = parsestr2(&strct, a, ptr2);
				if(a != NULL) i = 1;//used for restoring of string after
			    }

			    if(a != NULL){//printout!
				if(str_size != 0){
				    p = a + str_size;
				    while(*a && a < p){
					putc(*a, out);
					a++;
				    }
				}else fprintf(out, "%s", a);
			    if(i) restore_str(&strct);
			    }else printf("parsestring: %s not match!\n", ptr1);
			}else printf("Name: %s not found!\n", name);
		        free(name);
		    }
		//show variables value
/*		    if(a = get_var(NULL, ptr1)){
			//fprintf(out, "%s", a);
			print_1(out, a);
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
		//show variables value
		    if(a = get_var(NULL, ptr1)){
			print(out, a);//!!!need recursive limit!!!!
		    }else printf("Name: %s not found!\n", ptr1);
		    free(ptr1);
		}
	    }
	    data = tmp;
	    continue;
/*** big IF configuration ***/
	}else if(tmp = parsestr2(&strct, data, "if/t\"/[/*/]\"")){
	    if(make == 0) mk = 0;
	    else mk = 1;
	    if(cfg_arg_strcmp(tmp, 0)){//par -not exist or not match
		mk = 0;
	    }
	    data = restore_str(&strct);
	    data = print_(out, data, mk, loop + 1);
	    continue;
	}else if(tmp = parsestr2(&strct, data, "ifnot/t\"/[/*/]\"")){
	    if(make == 0) mk = 0;
	    else mk = 1;
	    if(cfg_arg_strcmp(tmp, 1)){//par -not exist or match
		mk = 0;
	    }
	    data = restore_str(&strct);
	    data = print_(out, data, mk, loop + 1);
	    continue;
	}else if(*data == 'f' && *(data+1) == 'i'){
	    data += 2;
	    if(loop) return data;
	    else {
		printf("\"if\" not present!\n");
		continue;
	    }
//	    make = 1;
//	    continue;
/*** end of IF configuration ***/
	}else if(tmp = parsestr2(&strct, data, "table:/t/[/*/]#end_table")){
	    if(make == 1){
		parse_tbl(tmp, 1);//with clean
	    }
	    data = restore_str(&strct);
	    continue;
	}


	if(make) putc(ch, out);
	data++;
    }

    if(loop) printf("\"fi\" not present!\n");
    return data;
}

//return 0 -exec in braces, 1 - jump
/*used in get_cgi as print "text to \n be ??_%var?? "*/
int print_str(FILE *out, char *tmp){
    char *tmp1, *tmp2;//tmp2 can be replaced via tmp1!!
    struct cgi *ptr;
    struct parsestr strct;
    ptr = cgi_used;
    static int loop_print_1 = 0;	// used if loops exissted
    if(loop_print_1 > 10){
	fprintf(out, "max loop counter reached\n");
	return 1;
    }
    loop_print_1++;

	    while(*tmp){
		tmp1 = tmp;
		if(*tmp == '\\'){
		    if(*(tmp+1) == '\"'){ tmp++; tmp1 = tmp;}
		    else if(*(tmp+1) == 'n'){tmp++; tmp1 = "\n";}
		    else if(*(tmp+1) == 't'){tmp++; tmp1 = "\t";}
		    else if(*(tmp+1) == '\\'){tmp++; tmp1 = "\\";}
		    else if(*(tmp+1) == '?' || *(tmp+1) == '{' || *(tmp+1) == '['){tmp++; tmp1 = tmp;}
		} else
		if(tmp2 = parsestr2(&strct, tmp, "??/[/N?N/*/]??")){		//??variable??
			tmp2 = get_var(NULL, tmp2);		//get_var and get_variable
			if(tmp2) print_str(out, tmp2); /*fprintf(out, "%s", tmp3);*/
			tmp = restore_str(&strct);
			continue;		//it's or tmp=tmp+2 or tmp=tmp2+2
		} else
		if(tmp2 = parsestr2(&strct, tmp, "?@/[/N@N/*/]@?")){		//?@variable@?
			if(*tmp2 == '0'){		//?@0variable@?	-	fill with \0 the rest of variable
				long long size, s;
				tmp2 = get_var(&size, tmp2+1);
				if(tmp2){
				    s = strlen(tmp2);
				    if(s < size)	fprintf(out, "%s", tmp2);
				    else{		fwrite(tmp2, size-1, 1, out); s = size-1;}//make zero at end
				    while(s < size){
					putc('\0', out);
					s++;
				    }
				}
			}
			tmp = restore_str(&strct);
			continue;		//it's or tmp=tmp+2 or tmp=tmp2+2
		} else
		if(tmp2 = parsestr2(&strct, tmp, "{?/[/N?N/*/]?}")){	//{?condition?}	-switched on if running from cgi-script
//needed more thinking about
			if(ptr && ptr->data_ptr){
				tmp2 = parsestr1(ptr->data_ptr, tmp2);		//parse tmp in data-buffer
				if(tmp2){
				    fprintf(out, "%s", tmp2);
				    ptr->data_ptr = point[1];
				}//else ptr->data_ptr = NULL;		//be carefull- this mean that no more parses 
			}
			tmp = restore_str(&strct);
			continue;		//it's or tmp=tmp+2 or tmp=tmp2+2
		} else
		if(tmp2 = parsestr2(&strct, tmp, "[?/[/N?N/*/]?]")){	//[?condition?]	-switched on if running from cgi-script
//needed more thinking about
			if(ptr && ptr->data_ptr){
				tmp2 = parsestr1(ptr->data_ptr, tmp2);		//parse tmp in data-buffer
				if(!tmp2){
				    restore_str(&strct);
				    loop_print_1--;
				    return 0;		//if tmp is not matched with data_ptr - break printing
				}
			}
			tmp = restore_str(&strct);
			continue;		//it's or tmp=tmp+2 or tmp=tmp2+2
		}
		putc(*tmp1, out);
	    tmp++;
	    }
	loop_print_1--;
	return 1;
}


void rename_(char *old_name, char *new_name){
//    char LineBuf[256];
    FILE *fp, *fop;
	if((fp = fopen(old_name,"r")) != NULL && (fop = fopen(new_name, "w")) != NULL){
/*	    while(fgets(LineBuf,255,fp) != NULL){
	        fputs(LineBuf,fop);
	    }
*/	    copy(fp, fop);
	    fclose(fp);
	    fclose(fop);
	    remove(old_name);
	}
}

//return 0 -exec in braces, 1 - jump
int change_line(char *line){ //line="/etc/file:/ nameserver: nameserver ??var??" or line="/etc/file:/*/?var?/: nameserver ??_%var??"
    char *file_name, *cmpstr, LineBuf[256];//, *tmp;
    char *file ="/var/temp1234";
    char *ptr = cgi_used->data_ptr; //!!!!use only in cgi-scripts!!!!

    FILE *fp, *fop;
    int flag = 0;//exec in braces
        file_name = w_strtok(&line, ':');
        if(!file_name || !(*file_name)){ printf("change_line: file name is empty\n");return 1;}//jump the braces
        cmpstr = w_strtok(&line, ':');
        if(!cmpstr || !(*cmpstr)){ printf("change_line: compare_string is empty\n");return 1;}

	if((fp = fopen(file_name,"r")) == NULL) return flag;//cannot open file -> exec. in braces

	if((fop = fopen(file, "w")) != NULL){
	    while(fgets(LineBuf,255,fp) != NULL){

		cgi_used->data_ptr = LineBuf;
		if(parsestr1(LineBuf, cmpstr)){
		    if(print_str(fop, line)){
//printf("%s   :: %sline: %s\n", cmpstr, LineBuf, line);
		    flag = 1;//jump braces
		    }else{
			fputs(LineBuf,fop);
		    }
		}else{
		    fputs(LineBuf,fop);
		}
	    }
	    fclose(fop);
	    if(flag == 1){
		rename_(file, file_name);//found and changed - rename file
	    }else{
		remove(file);//not found - remove file
	    }
	}
	fclose(fp);
	cgi_used->data_ptr = ptr;
	return flag;

}

// tmp1 = "from_par:to_par:cmd"
//or ":to_par:cmd"
void wr_shell(char *tmp1, char flag){
    char *tmp, *tmp2, *tmp3;
    unsigned long long size, size1;
	tmp = w_strtok(&tmp1, ':');
	if(tmp){
		tmp3 = tmp1;
		if(*tmp)  tmp = get_var(&size1, tmp);
		else tmp = NULL;
		tmp2 = w_strtok(&tmp1, ':');
		if(tmp2 && *tmp2 && *tmp1){
		    tmp2 = get_var(&size, tmp2);
		    if(tmp2 && size){
		    write_shell(tmp, size1, tmp2, size, flag, tmp1);
//			else write_shell(tmp, size, 1, tmp1);
		    }
		if(tmp2 != tmp1) *(tmp1-1) = ':';
		}
		*(tmp3-1) = ':';
	}
}

#define skipspaces(p) while(isspace(*p)) p++;

int set_2(char *arg){
    int result=0;
    skipspaces(arg);
    if(isdigit(*arg)){
	while(isdigit(*arg))
	    result=result*10 + *arg++ - '0';
	return result;
    }
    else return 0;
}

int set_1(int *var, char *arg){
    skipspaces(arg);
    if(*arg == '\0') return 0;
    else if(*arg == '=' && *(arg+1) == '=') return *var == set_2(arg+2);
    else if(*arg == '!' && *(arg+1) == '=') return *var != set_2(arg+2);
    else if(*arg == '<' && *(arg+1) == '=') return *var <= set_2(arg+2);
    else if(*arg == '>' && *(arg+1) == '=') return *var >= set_2(arg+2);
    else if(*arg == '<') return *var < set_2(arg+1);
    else if(*arg == '>') return *var > set_2(arg+1);
    else if(*arg == '-' && *(arg+1) == '=') return *var -= set_2(arg+2);
    else if(*arg == '+' && *(arg+1) == '=') return *var += set_2(arg+2);
    else if(*arg == '='){ *var = set_2(++arg); return 1;}
    else if(*arg == ':' && *(arg+1) == '='){
	    char *tmp;		// set "a := _#val";
	    arg=arg+2;
	    skipspaces(arg); tmp=get_var(NULL, arg);
	    if(tmp){*var = set_2(tmp); return 1;}
	    else {*var = 0; return 0;}
	    }
    else return(*var);
}

int set_tbl(char *arg){
    char *tmp, ch = '\0', *st, *string = " =!<>-+:";
    unsigned int *in = NULL;

    skipspaces(arg);
    if(*arg == '\0') return 0;
    tmp = arg;
    while(*tmp){
	st = string;
	while(*st){
	    if(*tmp == *st){
		ch = *st;
		*tmp = '\0';
		in = get_tbl_begin(arg);
		*tmp = ch;
		if(in != NULL) return set_1(in, ++tmp);
		else return 0;
	    }
	    st++;
	}
	tmp++;
    }
    return 0;
}


extern FILE *fp;

//return value:
//0	-error
//1	-ok
//2	-go out now!
int get_cgi_body(struct cgi *ptr){

    int i = 0, j = 0, jump = 0, allocated, ret;
    unsigned long long size, size1, size2;//size2 - for copy_ppar
    char *tmp, *tmp1, *tmp2, *arg = NULL;
    FILE *f;

//ptr->arg[0] = "privet";
	    while(i < GET_CGI_MAX && ptr->cmd[i]){
//printf("[%d]: %s\n", i, ptr->arg[i]);
		arg = "";
		allocated = 0;
		if(ptr->arg[i]){	//allocate memory & copy ptr->arg[i] to arg
		    j = strlen(ptr->arg[i]) + 1;
		    arg = (char *)malloc(j);
		    if(arg){
			strncpy(arg, ptr->arg[i], j);
			allocated = 1;
		    }else{
			printf("[ERR] Unable allocate memory\n");
			return 0;
		    }
//		} else {
//		    printf("ptr->arg[i] is empty\n");
//		    return 0;
		}
		switch (ptr->cmd[i]){
		    case 1: //print
			    print(fp, arg);
			//    jump = print(out, arg); //run in braces if print returns 0;
			    break;
		    case 2:
		    /* after parse_file - quit now !!! */
			    size = strncpy_(NULL, arg, 0)+1;	//this is max. size of arg-string
			    if(size > 1 && (tmp = malloc(size))){
				strncpy_(tmp, arg, size);
//printf("parse_file:%s %ld\n", tmp, size);
				if(*tmp != '\0') parse_file(tmp, 0);//return from this function now!!

//				if(!copy_file_include(tmp, out)) jump = 1; //exec in braces if file not found 
									//jump if error memory allocate
				free(tmp);
			    }//else jump = 1;//error allocate memory
			    if(allocated) free(arg);
			    return 2;		//go out now!!
			    break;
		    case 3:	//savecfg;
			    SaveConfiguration();
			    break;
		    case 4:
			    fill_cfg(arg);
			    break;
		    case 5:	// write_system  par:cmd
		    case 6:	// cat_system  par:cmd
			    wr_shell(arg, (ptr->cmd[i] == 5) ? 0 : 1);
			    break;
		    case 7:
			    write_char(arg);
			    break;
		    case 8:				//change_line
			    jump = change_line(arg);
			    break;
		    case 9:	// write_par  par:value
			    tmp1 = arg;
			    tmp2 = w_strtok(&tmp1, ':');
			    if(tmp2 && *tmp2 && *tmp1){
			    	tmp = get_var(&size, tmp2);
				if(tmp && size){
				    strncpy_(tmp, tmp1, size-1);
//				    tmp[size-1] = '\0';
				}
//				if(tmp2 != tmp1) *(tmp1-1) = ':';
			    }
			    break;
		    case 10:				//if_changed
//printf("if_changed: %s; i=%d; bb=%d\n", ptr->arg[i], i, ptr->bb[i]);
			    if(!cfg_arg_changed(arg)){ jump = 1;}
			    break;
		    case 11:				//not_changed or not found
			    if(cfg_arg_changed(arg)) { jump = 1;}
			    break;
		    case 12:				//if
//printf("if: %s; i=%d; bb=%d\n", ptr->arg[i], i, ptr->bb[i]);
			    if(cfg_arg_strcmp(arg, 0)) { jump = 1;}
			    break;
		    case 13:				//not
			    if(cfg_arg_strcmp(arg, 1)) { jump = 1;}
			    break;
		    case 14:shell(arg);		//shell
			    break;
		    case 15:my_shell(fp, arg);		//my_shell
			    break;
		    case 16: system_(arg);break;
		    case 17: my_system(fp, arg);break;
		    case 18://clean_par arg="par:par1:par2.."
			    tmp1 = arg;
			    while(tmp2 = w_strtok(&tmp1, ':')){
				tmp2 = get_var(&size, tmp2);
				if(tmp2 && size){
				    memset(tmp2, '\0', size);
				}
			    }
			    break;
		    case 19:	//boot_cgi
			    size = parse_cgi_name(arg, &tmp1, NULL);//ptr1 - is end(\n) of cgi-string
//printf("-%s--%d->--%s--<<\n", arg, size, tmp1);
			    if( size == 0 || size > MAX_NAME_SIZE ){
				printf("WARN: Length of cgi name: %d\n", size);
				if(allocated) free(arg);
				return 0;
			    }
			    tmp = (char *)malloc(size+2);
			    if(tmp != NULL){
				parse_cgi_name(arg, &tmp1, tmp);
				ret = get_cgi(0, size, tmp);
				free(tmp);
				if(ret == 2){
				if(allocated) free(arg);
				return 2;	//go out now!!
				}
			    }
			    break;
		    case 20:					//if_set
			    if(!set_tbl(arg)){ jump = 1;}
			    break;
		    case 21: //exit_cgi
			    if(allocated) free(arg);
			    return 1;
		    case 22: //copy_ppar
			    //arg="from_par:to_par:offset"
			    tmp = arg;//offset
			    tmp1 = w_strtok(&tmp, ':');//from_par
			    tmp2 = w_strtok(&tmp, ':');//to_par
			    if(tmp2 && *tmp2 && tmp1 && *tmp1 && *tmp){
				size2 = atoll(tmp);
				tmp1 = get_var(&size, tmp1);//from_par
				if(tmp1){
				    if(size == 0) size = strlen(tmp1) + 1;//read only from_par
				    tmp = get_var(&size1, tmp2);//to_par
				    if(tmp){
					if(size1){
						if(size2 < size){//offset < from_par
						    size = size-size2;
						    strncpy(tmp, tmp1+size2, (size < size1) ? size : (size1-1));
						    if(size >= size1) tmp[size1-1] = '\0';
						    jump = 1;
						}
					}
//				    }else{
//					if(size2 < size) jump = reg_par(tmp2 ,tmp1+size2 , size-size2);//to_par
				    }
				}
			    }
			    break;


		}//switch end
	    if(allocated) free(arg);
//printf("check: [%d]: %s a=%d b=%d\n", i, ptr->arg[i], ptr->a, ptr->b);
	    if(jump == 1){
		jump = 0;
		if(i!=ptr->bb[i]) i = ptr->bb[i];
		else i++;
	    }else i++;
	    }//while
return 1;
}

int get_cgi(char ch, int num, char *buf){

    int ret = 0;
    struct cgi *ptr;
//printf("char:%c %d\n", ch, *buf);
    ptr = cgi_name;
    while(ptr){
//printf("cgi: %c %d\n", *(ptr->name), *((ptr->name) +1));
//printf("SHOW:%s: %d - %d \n", ptr->name,num, ptr->name_size);
	if(ch == 'A'){
	    if(ch == *(ptr->name) && num == 1 && ( ((num+1) == ptr->name_size && *buf == *((ptr->name)+1)) 
				|| ((num+2) == ptr->name_size && *buf >= *((ptr->name)+1) && *buf<= *((ptr->name)+2)) // "A" 0 10 -> 0>=x<=10
				)
	    ){		//!strcmp(ptr->name, filename)
printf("!!MATCH!!\n");
		cgi_used = ptr;
		A1_BTN = *buf;
		ret = get_cgi_body(ptr);
		A1_BTN = 0;	//means not defined!!!
		cgi_used = NULL;
		return ret;	//ok cgi is found
	    }
	    //return 0;//A- match but other is not match
	}else if(ch == 'H'){
printf("H PARAM\n");
		return 1;
	}else if((ptr->name_size == num) && !memcmp(ptr->name, buf, num)){//rest of ch, for example: ch == 0
//think about...
//		cgi_used = ptr;
		ret = get_cgi_body(ptr);
//		cgi_used = NULL;
		return ret;	//ok cgi is found
	}
	ptr = ptr->next;
    }//while(ptr)
    return 0;
}


void free_cgi(struct cgi *ptr){
    if(!ptr) return;
    free_cgi(ptr->next);
//printf("free %s\n", ptr->name);
    int i = 0;
    while(i < GET_CGI_MAX){		//clear all cgi_lines
	if(ptr->arg[i] != NULL){ free(ptr->arg[i]);}
	i++;
    }

    free(ptr);	//at hier action
}
