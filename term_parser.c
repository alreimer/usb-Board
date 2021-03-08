/* term_parser.c:
 *
 * Copyright (C) 2009-2012  Alexander Reimer <alex_raw@rambler.ru>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "include/httpd.h"
//#include "include/httpd_config.h" //was in this file RAW
//#include "include/httpd_sysconf.h"
//#include "parse_CGI.h"		//get_arg;
//#include "copy.h"		//get_var;

#include <sys/types.h>		//for regexpressions
#include <sys/stat.h>
#include <regex.h>
#include "term_parser.h"
#include "term_add.h"
#include "terminal.h"

#define DEBUG
#undef DEBUG

#define MIN(a,b) ((a) < (b) ? (a) : (b))

extern struct cfg_parse1 **cfg_p;
//extern struct cfg_parse2 cfg2[];

char *strstrcfg(char *d, char *c, int *len)	/*check c string in d string*/
{

    if(*c == '\0') return NULL;
    while (*c)
	{
	    if (*c != *d)  break;
	    if (!(*d))  return NULL;//this line is not needed!!!
	    c++;
	    d++;
	}

	if (!(*c)){
	    if(*d != '=' && *(d+1) != '\'') return NULL;	//it's "=\'"  -- fresch
	    d = d + 2;

	    c = d;
	    while((*c != '\'') && *c) c++;	//can be danger
	    *c = '\0';
	    *len = c - d + 1;	//size of value (incl. termin. NULL) on which is d pointed
	    return d;
	}
    return NULL;
}

int cfg_arg_changed(char *web_name){	//if not found - returns 0 (as not changed)

    struct cfg_parse1 *p;

    if(cfg_p){
    p = *cfg_p;

    while (p){		/* search the variable value  */
        if((p->str != NULL) && (p->size != 0) && (p->size != 1) && 
		(p->web_name != NULL) && strcmp(p->web_name, web_name) == 0 ){
	    return p->changed;
	}
	p = p->next;
    }
    }

    return 0;
}

/* parm = web_name:value */
int cfg_arg_strcmp(char *parm, char flag){	//if not found or ':' not found - returns -1 (as not same)

//    struct cfg_parse1 *p = cfg1;
    char *ptr;
#ifdef DEBUG
printf("cfg_arg_strcmp: %s\n", parm);
#endif
    ptr = w_strtok(&parm, ':');
//    if(!ptr || !(*parm)) return -1;
    if(!ptr) return -1;
//printf("%s    %s\n", ptr, parm);
    ptr = get_var(NULL, ptr);
//    ptr = get_cfg_value(NULL, ptr, 0);
    *(parm-1) = ':';
    if(flag == 0) {if(ptr && parsestr1_(ptr, parm)){ return 0;}}		//flag = 0 -if, flag =1 -not
    else {if(ptr && !parsestr1_(ptr, parm)){ return 0;}}
    return -1;
}

/*
void fill_all_cfg(void){

    struct cfg_parse1 *p = cfg1;
    char *tmp;
    regex_t preg;
    size_t nmatch;
    regmatch_t pmatch;

    while(p){
	if((p->str != NULL) && (p->size != 0) && (p->size != 1)){
	tmp = get_arg(p->web_name, NULL, 0);//NULL - only get value
	if(tmp){
	    if(strcmp(tmp, p->value) != 0){
printf("fill web:%s = %s\n", p->web_name, tmp);
		p->changed = 1;
		strncpy(p->new_value, tmp, p->size-1);
		p->new_value[p->size-1] = '\0';
				//hier p->new_value to pattern recognition needed
		    if(p->pattern && *(p->pattern)){
			if(regcomp(&preg, p->pattern, REG_EXTENDED|REG_NOSUB)){
			    *(p->new_value) = '\0';
			    p->changed = 0;
printf("Not compiled\n");
			    goto END;
			}
printf("For matching\n");
			if(regexec(&preg, p->new_value, nmatch, &pmatch, 0)){//REG_NOTEOL
			    *(p->new_value) = '\0';
			    p->changed = 0;
printf("Not matched\n");
			}
			regfree(&preg);
		    }
		END:;
	    }
	}
	}
	p = p->next;
    }
}
*/
//parm="parm1:parm2:parm3..."
void fill_cfg(char *parm){

    struct cfg_parse1 *p;
    char *tmp;
    regex_t preg;
    size_t nmatch;
    regmatch_t pmatch;

    if(cfg_p == NULL) return;
    while(tmp = w_strtok(&parm, ':')){
	if(*tmp){
	    p = *cfg_p;
	    while(p){
		if((p->str != NULL) && (p->size != 0) && (p->size != 1) && !strcmp(tmp, p->web_name)){
	
//		    tmp = get_arg(tmp, NULL, 0);//NULL - only get value
//		    if(tmp){
//		    if(strcmp(tmp, p->value) != 0){
		    ////////////////////////if(*(p->new_value) != '\0'){ //why i set it here??? -must be removed
			if(strcmp(p->new_value, p->value) != 0){
printf("fill web:%s = --%s--%s--\n", p->web_name, p->new_value, p->pattern);
			    p->changed = 1;
//			    strncpy(p->new_value, tmp, p->size-1);
			    p->new_value[p->size-1] = '\0';
			    if(p->pattern && *(p->pattern)){
			    	//hier p->new_value to pattern recognition needed
			    	if(regcomp(&preg, p->pattern, REG_EXTENDED|REG_NOSUB)){
//			    	    *(p->new_value) = '\0';
			    	    p->changed = 0;
printf("Not compiled\n");
			    	    break;
			    	}
			    	if(regexec(&preg, p->new_value, nmatch, &pmatch, 0)){
//			    	    *(p->new_value) = '\0';
			    	    p->changed = 0;
printf("Not matched --%s--\n", p->pattern);
			    	}
			    	regfree(&preg);
			    }
			}
		    //////////////////////////}
		    break;
		}
		p = p->next;
	    }
//	if(tmp != parm) *(parm-1) = ':'; //not last parm
	}
    }
}

struct cfg_parse1 *get_cfg(char *field_name){

    struct cfg_parse1 *p;
    struct page_n *n;
    char flag = 0, *ptr = field_name;

    if(cfg_p){
	p = *cfg_p;

	while(*ptr){
	    if(*ptr == '@'){
		*ptr = '\0';
		flag = 1;
		n = get_page(ptr+1);
		if(n) p = n->cfg;
		break;
	    }else if(*ptr == '%'){
		*ptr = '\0';
		flag = 2;
		n = get_last_page(ptr+1, 0);
		if(n) p = n->cfg;
		break;
	    }
	    ptr++;
	}

	while (p){		/* search the variable value  */
	    if((p->str != NULL) && (p->size != 0) && (p->size != 1) && (p->web_name != NULL) && strcmp(p->web_name, field_name) == 0 ){
/*p->size !=0 && !=1 -must be thinked about*/
		if(flag == 1) *ptr = '@';
		else if(flag == 2) *ptr = '%';

		return p;
	    }
	    p = p->next;
	}
	if(flag == 1) *ptr = '@';
	else if(flag == 2) *ptr = '%';
    }
    return NULL;
}

char *get_cfg_value(long long *size_ptr, char *field_name, int i){
    struct cfg_parse1 *p = get_cfg(field_name);

	if((p != NULL) && (p->str != NULL) && (p->size != 0) && (p->size != 1)){

		if(size_ptr) *size_ptr = p->size;	//if pointer is given, return value
		if(i == 0) return p->value;
		else if(i == 1) return p->new_value;
		else if(i == 2){ if(p->changed) return p->new_value;
				else return p->value;
				}
		else return NULL;
	}

    if(size_ptr) *size_ptr = 0; // if given but not matched, return 0
    return NULL;
}

//register new parameter called name, from value, size-lang.
//return 1=success and 0=false
int reg_par(char *name, char *value, long long size)
{
	struct cfg_parse1 *ptr = *cfg_p;
	while(ptr){
		if(ptr->type == CFG_TMP && !strcmp(name, ptr->web_name)){
			ptr->size = size;
			ptr->value = value;
			ptr->new_value = value;
			return 1;
		}
	ptr = ptr->next;
	}
	return 0;

}


//arg=to_par:from_par
//to_par is par or area of actual page
void link_cfg(char *arg){
    char *tmp, *ptr, *err = "ERROR: cannot allocate memory for link\n";
    struct cfg_parse1 *cfg_pointer = NULL, *cfg_pointer1, **cfg;
    unsigned long str_size;//is size of to_par->web_name, if web_name not exist in page

    tmp = w_strtok(&arg, ':');
    if(tmp && *tmp && *arg){
    /* tmp = to_par, arg = from_par */
	cfg_pointer1 = get_cfg(arg);
	if(cfg_pointer1 == NULL){printf("NO parameter or area with name: %s\n", arg); *(arg-1) = ':'; return;}

	cfg = cfg_p;
	while(*cfg){
	    if(((*cfg)->web_name != NULL) && !strcmp((*cfg)->web_name, tmp)){
		cfg_pointer = (*cfg);
		break;
	    }
	    cfg = &((*cfg)->next);
	}
	if(cfg_pointer == NULL){
	    cfg_pointer = (struct cfg_parse1 *)malloc(sizeof(struct cfg_parse1));
	    if(cfg_pointer == NULL){ printf("%s", err); *(arg-1) = ':'; return;}
	    *cfg = cfg_pointer;

	    str_size = strlen(tmp);
	    ptr = (char*)malloc(str_size + 2);
	    if(ptr == NULL){
		printf("%s", err);
		*(arg-1) = ':';
		free(*cfg);
		*cfg = NULL;	//main criteria to abort moving in array.
		return;
	    }

	    strncpy(ptr, tmp, str_size);//web_name
	    ptr[str_size] = '\0';

	    cfg_pointer->str = ptr;
	    cfg_pointer->web_name = ptr;

	}
	cfg_pointer->type = cfg_pointer1->type;		//think about!!
	cfg_pointer->size = cfg_pointer1->size;
	cfg_pointer->changed = cfg_pointer1->changed;
	cfg_pointer->position = cfg_pointer1->position;
	cfg_pointer->saved = cfg_pointer1->saved;
	cfg_pointer->value = cfg_pointer1->value;
	cfg_pointer->new_value = cfg_pointer1->new_value;
	cfg_pointer->name = cfg_pointer1->name;
	cfg_pointer->pattern = cfg_pointer1->pattern;
	cfg_pointer->next = NULL;

	*(arg-1) = ':';
    }
}

void write_char(char *arg){

    char *tmp;
    struct cfg_parse1 *p;

    if(cfg_p == NULL) return;
    p = *cfg_p;
    tmp = w_strtok(&arg, ':');
    if(tmp && *tmp){
	while (p){		/* search the variable value  */
	    if((p->str != NULL) && (p->size != 0) && (p->size != 1) && (p->web_name != NULL) && strcmp(p->web_name, tmp) == 0 ){
		if(*arg){
		    if(p->position < ((p->size) - 1)){
			*(p->new_value + p->position) = *arg;
			(p->position)++;
			*(p->new_value + p->position) = '\0';
		    }
		}else{		//erase option - if nothing is given
		    if(p->position > 0){
			(p->position)--;
		    }
		    *(p->new_value + p->position) = '\0';
		}
	    break;
	    }
	    p = p->next;
	}
    }
}

/*
int ReadConfiguration1(void){

    FILE *fip;
    char LineBuf[256];
    char *ptr, *end, *config = ETC_PATH "/config";
    int i;
    struct cfg_parse2 *p;

    if((fip = fopen(config, "r")) == NULL){
	printf("Cannot find file: %s\n", config);
	exit (1);
	//return 0;
    }

    while(fgets(LineBuf,255,fip) != NULL){
	    for(i=0;i<255;i++){
		if((LineBuf[i] == ' ') || (LineBuf[i] == '\n') || (LineBuf[i] == '\r')){
		    LineBuf[i] = '\0';
		    i=256;
		}
	    }
	    
	    p = cfg2;
	    while(p->size != 0){
		if((ptr = strstrcfg(LineBuf, p->name, &i)) != NULL){
			p->value[(p->size)-1] = '\0';	//make it for shure
			strncpy(p->value, ptr, MIN(i, (p->size)-1));	//it seems to be OK, but is always p->value[(p->size)-1] == '\0'
//			strcpy(p->value, ptr);
		    printf("%s=%s %d\n", p->name, p->value, p->size);
		}
		p++;
	    }
//	    printf("IP=%s\n", CONFIG.IP);

    }
    fclose(fip);
    return 1;
}
*/
int ReadConfiguration(void){

    struct cfg_parse1 *p;
    FILE *fip;
    char LineBuf[1024];
    char *ptr, *end, *name, *config = ETC_PATH "/config";
    int i;

    if((fip = fopen(config, "r")) == NULL){
	printf("Cannot find file: %s\n", config);
	//exit (1);
	return 0;
    }

    if(cfg_p){
    while(fgets(LineBuf,1023,fip) != NULL){
	p = *cfg_p;
    
        while(p){
	    if((p->str != NULL) && (p->size != 0) && (p->size != 1) && (p->name) && *(p->name)){
		name = p->name;
		if(*name == '?'){
		    name++;
		    name = get_var(NULL, name);
		}
		if(name && *name && (ptr = strstrcfg(LineBuf, name, &i)) != NULL){
		strncpy(p->value, ptr, MIN(i, (p->size)-1));	//it seems to be OK, but is always p->value[(p->size)-1] == '\0'
		p->value[(p->size)-1] = '\0';	//make it for shure
//		strcpy(p->value, ptr);
		strncpy(p->new_value, ptr, MIN(i, (p->size)-1));
		p->new_value[(p->size)-1] = '\0';
		p->saved = 0;//new here 20180930
		printf("readcfg:%s=%s %lld  %d\n", name, p->value, p->size, p->changed);
		}
	    }
	    p = p->next;
	}
    }
    }
    fclose(fip);
    return 1;
}

int SaveConfiguration(void){	//new function to write to config direct from struct

    FILE *fip, *fop;
    struct cfg_parse1 *p;
    int i;
    char LineBuf[1024], *name;	//maybe buffer is to small

    if(cfg_p == NULL) return 1;

    if((fip = fopen(ETC_PATH "/config","r")) == NULL || (fop = fopen(ETC_PATH "/config1","w+")) == NULL){
#ifdef DEBUG
        fprintf( stderr, "Save: Cannot open one of config files\n" );
#endif
	return 0;
    }else{
//#ifdef DEBUG
#if 1
	fprintf( stdout, "*****Save: Success open config file *****\n" );
#endif
    while(fgets(LineBuf,1023,fip) != NULL){
	    p = *cfg_p;		//need be more selective!
	    while(p){
		if((p->str != NULL) && (p->size != 0) && (p->size != 1) && 
				p->name && *(p->name) && p->changed && (! p->saved)){
		name = p->name;
//printf("SAVEDDDDD\n", name);
		if(*name == '?'){
		    name++;
		    name = get_var(NULL, name);
		}
		if(name && *name && strstrcfg(LineBuf, name, &i)){
		    fprintf(fop,"%s=\'%s\'\n", name, p->new_value);
printf("par__%s=\'%s\' changed=%d\n", name, p->new_value, p->changed);
		p->saved=1;
		    etc_save[0] = '1';		//set etc_save=1 - so /etc must be saved in memory
		    etc_save[1] = '\0';

	        goto next;
		}
		}
		p = p->next;
	    }
	    fputs(LineBuf,fop);
    next:	;
    }
    //save the rest of parameters which are not in file
    p = *cfg_p;
    while(p){
	if((p->str != NULL) && (p->size != 0) && (p->size != 1) && p->name && *(p->name) && p->changed && (!p->saved)){
		name = p->name;
		if(*name == '?'){
		    name++;
		    name = get_var(NULL, name);
		}
		if(name && *name){

	    fprintf(fop,"%s=\'%s\'\n", name, p->new_value);
printf("%s=\'%s\' changed=%d\n", name, p->new_value, p->changed);
	    p->saved=1;
		etc_save[0] = '1';
		etc_save[1] = '\0';

		}
	}
	p = p->next;
    }

    fclose(fip);
    fclose(fop);
    rename(ETC_PATH "/config1", ETC_PATH "/config");
    chmod(ETC_PATH "/config", S_IRUSR|S_IWUSR);
    }
#if 0 //RAW
	  system("/etc/rc.d/save_config.sh &");
#endif
    return 1;
}

//recover the 'par'-saved flag to 0
void recover_saved(char *par){
    struct cfg_parse1 *p;
    if(cfg_p == NULL) return;
    p = *cfg_p;
    if(par == NULL || *par == '\0'){
	while(p){
	    if((p->str != NULL) && (p->size != 0) && (p->size != 1) && p->name && *(p->name) && p->changed && (p->saved)){
//printf("recover: %s\n", p->name);
		p->saved = 0;
	    }
	    p = p->next;
	}
    }else printf("par is not empty\n");//test

}

