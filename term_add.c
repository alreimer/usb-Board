/* term_add.c:  A very simple connection programm for USB-Board
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
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "include/httpd.h"

#include "term_add.h"
#include "term_tbl.h"
#include "term_cgi.h"
#include "term_parser.h"
#include "terminal.h"

char *point[2];		//0 - place where is ch_zero by /]; 1 - end of string
char ch_zero = '\0';

char *parsestr1( char *d, char *c)	//try identic strings!, "xxx*NULL" combination
{
	char *tmp, *tmp2, ch;
	unsigned int i;

	while (*c)
	{
	    
	    if(*c == '/' ){
		c++;
		switch (*c){
    		    case '*':/* printf("d1:%s\n", d);*/
			      c++; while(1){tmp = parsestr1(d, c); if (tmp) return tmp; if(! *d) break; d++;} return NULL;//depend on continues parsing
							//if * in b string -> some symbols between
//	!a*(b+c) = !(a+!(b+c))->	/!/Ba/\/!/Bb/\c/\/E/E
		    case 'B': c++; tmp = NULL; tmp2 = c; i = 0;
					while(*tmp2){
					    if(*tmp2 == '/' && *(tmp2+1) == '/') tmp2++; //tmp2+=2
					    else if(*tmp2 == '/' && *(tmp2+1) == 'B' && i <= 1024) i++; //if(/B)
					    else if(*tmp2 == '/' && *(tmp2+1) == 'E'){	//if(/E)	-optional
						if(i == 0){tmp = tmp2; break;}
						else i--;
					    }
					    tmp2++;
					}
				//tmp - end of compare-string
				//tmp2 - can be used,
				while(1){	//not matched
				    tmp2 = parsestr1(d, c);
				    if(tmp2) break;
				    while(1){
					if(*c == '/' && *(c+1) == '/') c++; //c+=2
					else if(*c == '/' && *(c+1) == '\\'){c = c + 2; break;} // if(/\) 
					else if((tmp && (c == tmp)) || (*c == '\0')) return NULL;	//end of compare-strings - no matches - return NULL
					c++;
				    }
				}
//printf("str:%s d=%s c=%s\n", tmp2,d,c);
				if(tmp) {d = tmp2; /*tmp2 is not always the end of string!! check it out*/
					c = tmp + 2; continue;}
				else return tmp2;

		    case '\\':
		    case 'E': point[1] = d;//NEW 20.03.2016
				return d;
	    /* skip zero or one character*/
		    case '0': tmp = d; while(tmp <= d+1){tmp = parsestr1(tmp, c+1); if (tmp) return tmp; tmp++;} return NULL;
	    /* skip one symbol in d, exept \0 */
		    case '|': c++; if(*d == '\0') return NULL; d++; continue;
	    /* by /.x. skip x-symbol all at once */
		    case '.': if(*c != *(c+2) || *(c+1)=='\0') return NULL; //error! - or make it with continue
				    c=c+1; while(*d == *c) d++; c=c+2; continue;
	    /* by /,x, skip x-symbol 0 or 1-time */
		    case ',': if(*c != *(c+2) || *(c+1)=='\0') return NULL; //error!
				    c=c+1; tmp = d; while((d <= tmp+1) && (*d == *c)) d++; c=c+2; continue; //used in *search[]
	    /* by /lxl get last x-symbol in string 1 or more times */
		    case 'l':
	    /* by /LxL get last x-symbol in string 0 or more times*/
		    case 'L': if(*c != *(c+2) || *(c+1)=='\0') return NULL; //error!
				    i=0;c=c+1; tmp=d; while(*tmp){if(*tmp == *c){ d=tmp+1;i=1;} tmp++;}
				    if(*(c+1)=='l'&&i==0)return NULL;c=c+2; continue; //used in serv-others_1.htm
	    /* by /NxN x-symbol is not in string */
		    case 'N': if(*c != *(c+2) || *(c+1)=='\0') return NULL; //error!
				    c=c+1; if(*c == *d) return NULL; c=c+2; d++; continue; //used in 
	    /* skip all blanks or tabs or enters in d */
//		    case ' ': tmp = d; while(*d == ' ' || *d == '\t' || *d == '\n' || *d == '\r') d++; if(tmp == d) return NULL; c++; continue;
		    case ' ': while(*d == ' ' || *d == '\t' || *d == '\n' || *d == '\r') d++; c++; continue;
	    /* skip all blanks and tabs */
		    case 't': while(*d == ' ' || *d == '\t') d++; c++; continue;
	    /* /<xCHARS/> matches CHARS with d, continue if matches and return null if not */
		    case '<': c++;
			switch(*c){
			    case '-': //  /<-CHARS/> matches chars 1 or more times
			    case '*': //  /<*CHARS/> matches chars zero or more times
				c++; tmp = c; i=0; while((*c != '/'|| c[1] != '>') && *c){
				if(*c == '\\'){
				    c++;
				    switch(*c){
				    case '\\':	ch = '\\';break;
				    case 't':	ch = '\t';break;
				    case 'n':	ch = '\n';break;
				    default: c--; ch = *c; break;
				    }
				} else ch = *c;
				if(ch==*d){ do{d++;}while(ch==*d);
				    c = tmp; i=1; continue;
				}
				c++;}
				if(*(tmp-1)=='-' && i==0) return NULL;
				if(*c == '/') c=c+2; continue;
			    case '0': //  /<0CHARS/> matches chars zero or 1 time
			    case '1': //  /<1CHARS/> matches chars one time
			    case 'N': //  /<NCHARS/> all chars not matches one time
				i = 0;tmp = c; c++; while((*c != '/'|| c[1] != '>') && *c){
				if(*c == '\\'){
				    c++;
				    switch(*c){
				    case '\\':	ch = '\\';break;
				    case 't':	ch = '\t';break;
				    case 'n':	ch = '\n';break;
				    case '0':	ch = '\0';break;//check if *d == '\0'.
							//In /* is set until zero, inclusive zero!
				    default: c--; ch = *c; break;
				    }
				} else ch = *c;
				if(ch == *d) i = 1; c++;}
				if(*(tmp)=='1' && i==0) return NULL;
				if(*(tmp)=='N' && i==1) return NULL;
				if(*c == '/') c=c+2; if(*d != '\0' && (*(tmp)=='1' || *(tmp)=='N' || (*(tmp)=='0' &&  i==1))) d++; continue;
			    default:
			    case '\0': c--; continue; //  rest='<\0'
			}
		    case '[': if(parsestr1(d, c+1)){ /*point[0] = d;*/ return d;} return NULL;		//if [ in b so this pointer will be returned, and c is move forward. BEGINNofSTR

		    case ']':/* printf("d:%s\n", d);*/
			      tmp = parsestr1(d, c+1);
			      if(!tmp) return NULL;
			      //case tmp=d (*(c+1) = '/0')[ end braces in end of string]
			      if(tmp == d && *tmp){ tmp++; point[1] = tmp;}
			      /*if(*d) point[0] = d+1; else */
			      point[0] = d;	// point[0] points on character to be zeroed
			      ch_zero = *d;
			      *d = '\0'; return tmp;//depend on continues(if /[ goes after /] - begin)
							// make end of string hier!, check rest as (]...) and return next to end char.
	    /* second '/' charakter is to match */
		    case '/': break;
//		    case '\0': continue;//need to check it!!!!!!

	    /* end of string is to match */
		    case '\0': if(*d != '\0') return NULL; return d;
		
		    case '!':		//Negotiation		NEW
					//  \!\*ab	-no more "ab" in rest of string
			    tmp = parsestr1(d, c+1);
			    if(tmp)	return NULL;
			    else { point[1] = d; return d;}
		    case '?':		// /?variable?/
		    c++;
		    tmp = c;
		    tmp2 = NULL;
		    while(*tmp){
			if(*tmp == '?' && *(tmp+1) == '/'){
			    *tmp = '\0';
			    tmp2 = get_var(NULL, c);		//get_var and get_variable
			    *tmp = '?';
			    c=tmp+2;
			    break;
			}
			tmp++;
		    }
		    
		    if(tmp2 && *tmp2){
			i = strlen(tmp2);
			if(i && strncmp(d, tmp2, i)) return NULL;
			d = d + i;
		    }
		    else return NULL;//new here!!!!

		    continue;		//it's or c++ or c=tmp+2

		    default: c--;
		}
	    } else if(*c == '\\'){
		c++;
		switch(*c){
	    /* new string is to match */
		    case 'n': if(*d != '\n') return NULL; c++; d++; continue;
		    case 't': if(*d != '\t') return NULL; c++; d++; continue;
		    case 'r': if(*d != '\r') return NULL; c++; d++; continue;
	    /* \" string is to match */
		    case '\"': if(*d != '\"') return NULL; c++; d++; continue;	//need to change it
	    /* \0 the same as "bla/" */
		    case '0': if(*d != '\0') return NULL; return d;
		    case '\\': break;
		    default: c--;
		}
	    }

//	    if (tolower(*c) != tolower(*d))  break;
	    if (*c != *d) return NULL;
	    if (!(*d))	  {printf("PARSESTR1<<<<<<</n");break;}	//not happens *d=*c=='\0'

	    c++;
	    d++;
	}

	    point[1] = d;
	    return (char *) d;//ENDofSTR
}

char *parsestr1_( char *d, char *c){		//push and pop the pointers
    char *tmp, *a, *b;

    a = point[0];
    b = point[1];

    tmp = parsestr1(d, c);

    point[0] = a;
    point[1] = b;

    return tmp;
}

char *parsestr2( struct parsestr *ptr, char *d, char *c){		//use the pointers in struct
    char *tmp;

    point[0] = NULL;
    if(tmp = parsestr1(d, c) /*&& (ptr != NULL)*/){
	ptr->ch = ch_zero;
	ptr->zero = point[0];	//if NULL -> restore_str is not made
	ptr->end = point[1];
    }

    return tmp;
}

char *restore_str( struct parsestr *ptr){	//note: in parsestring MUST BE-> "/]"
    if(ptr->zero){
	*(ptr->zero) = ptr->ch;
    }
    return ptr->end;
}


/*find in *str character mark and replace them throw /0, set *str to next charakter and return pointer to begining of *str
 if charakter in string not found return NULL*/
char *w_strtok(char **str, char mark)
{

    char *begin = *str;

    if(*str)
    if(**str){
	while (**str != '\0'){
	    if (**str == mark){
		**str='\0';
		(*str)++;
		return begin;
	    }
	    (*str)++;
	}
    return begin;
    }
    return NULL;
}

//copy tmp1 to tmp and break if strlen of tmp1 is smoller as size
//size is including end of str.
unsigned long long strmycpy(char *tmp, char *tmp1, unsigned long long size){
	if(size) size--;
	int s = size;
	while(size){
		*tmp = *tmp1;
		if(!(*tmp1)) return (s-size);
		tmp++; tmp1++; size--;
	}
	*tmp = '\0';
	return s;
}

//line="from_par:to_par:parsestr"
int write_ppar(char *line){
	char *par[2], *tmp, *tmp1;
	int i = 0, jump = 0;
	unsigned long long k, size;

	while(i <= 1){
		tmp = w_strtok(&line, ':');
		if(!tmp || !(*tmp) || !(*line)) {printf("[ERR]: write_ppar step: %d\n", i); return 0;}
//printf("%s , %d\n", tmp, i);
		par[i] = get_var((i==1)? &size : NULL, tmp);
	i++;
	}
	if(!par[0]) {printf("[ERR]: parameter not found\n"); return 0;}

// par[1]=to_par, par[0]=from_par, line=parsestr, size=to_par_size
	if(par[1]){
	    if(size){
		k = strlen(par[0])+1;
		//size == 1
		tmp = (char *)malloc(k);
		if(tmp){
		    strncpy(tmp, par[0], k);
		    if(tmp1 = parsestr1(tmp, line)){
			strmycpy(par[1], tmp1, size);
			jump = 1;//everything is OK - so jump
		    }
		free(tmp);
		}
	    }
	}else{
	    //tmp[1] == NULL
	    if(tmp1 = parsestr1(par[0], line)){	//par[0] - can be zeroed!! by /[/*/]
		jump = reg_par(tmp, tmp1, strlen(tmp1)+1);	//tmp = to_par, on success = 1
	    }
	}
	return jump;
}

unsigned long long strncpy_(char *tmp, char *tmp1, long long size){
    //tmp -is out, tmp1 -is in. and return the size of string!
    //if tmp == NULL -> works as counter of string tmp1
    //'\0' -sign is not counted!! -> strncpy_() + 1 
    char ch, *tmp2;
    struct parsestr strct;
    unsigned long long s = 0;

    static int loop_strncpy = 0;	// used if loops exissted
    static char *ptr = NULL;//used for breaking loops like: "area:text to loop ??area??"

    if(loop_strncpy == 0){
	ptr = tmp;
    } else if(loop_strncpy > 10){
	printf("max loop counter in strncpy_ reached\n");
	goto end;
    }

    if(tmp1 == ptr || tmp1 == NULL){
	goto end;
    }
    loop_strncpy++;

    while ((!tmp || s < size) && *tmp1){
	ch = *tmp1;
	if(ch == '\\'){
	    tmp1++;
	    switch (*tmp1){
		case '\"':	ch = '\"';break;
		case '\\':	ch = '\\';break;
		case 'n':	ch = '\n';break;
		case 't':	ch = '\t';break;
		case '?':	ch = '?';break;
		default :	tmp1--;
	    }
	} else if(tmp2 = parsestr2(&strct, tmp1, "??/[/N?N/*/]??")){		//??variable??
		tmp2 = get_var(NULL, tmp2);		//get_var and get_variable
		if(tmp2) s += strncpy_(tmp ? (tmp+s) : NULL, tmp2, size-s);
		tmp1 = restore_str(&strct);
		continue;
	}
	if(tmp) tmp[s] = ch;
	s++; tmp1++;
	if(s == 0) break;//is overload!
    }

    loop_strncpy--;
end:
    if(tmp) tmp[s] = '\0';
    return s;
}

int copy(FILE *read_f, FILE *write_f)	//used by rename_ aswell
{
  int n;
  int wrote;
  char copybuf[16384];

//  alarm(TIMEOUT);
  while ((n = fread(copybuf,1,sizeof(copybuf),read_f)) != 0) {
////    alarm(TIMEOUT);
    wrote = fwrite(copybuf,n,1,write_f);
////    alarm(TIMEOUT);
    if (wrote < 1)
	return -1;
  }
//  alarm(0);
  return 0;
}


//b_in(buffer) and s_in(size) are /dev/stdin. If (b_in == NULL) ->/dev/stdin is OFF!
//buf and size are std(out, err) buffers.
void write_system(char *b_in, long long s_in, char *buf, long long size, int mode, char *cmd)
{
	int pid, status;
	int pipe_fd[2], pipe_[2]; //pipe_fd - is for read(stdout stderr) and pipe_ - is for write(stdin)
	int wrote;
	int err = 0;
	long long i = 0;

if(!size) return;

if(mode){	//mode==1 -> cat mode
	while(buf[i] && (size - 1 - i)){
	    i++;
	}
}

    if (pipe(pipe_fd) == 0) {
      if (pipe(pipe_) == 0) {
	if (!(pid=fork())){	//child
	    close(pipe_fd[0]);	//read end

//	    close(fd);	    	//for inetd
//	    close(sockfd);

	    close(1);
	    close(2);
	    dup2(pipe_fd[1], 1);	//1 -stdout
	    dup2(pipe_fd[1], 2);	//2 -stderr
	    close(pipe_fd[1]);

	    close(pipe_[1]);	//write end
	    close(0);
	    dup2(pipe_[0], 0);	//1 -stdin
	    close(pipe_[0]);

//	    setsid();
	    execl("/bin/sh", "sh", "-c", cmd, (char *) 0);
//	    execl("/bin/sh", "sh", cmd, (char *) 0);

	    exit(1);
	}else{			//parent
	    close(pipe_[0]);	//read end
	    close(pipe_fd[1]);	//write end
	    if (pid > 0) {	// fork -- parent

		if(b_in != NULL){
		    if(s_in == 0) s_in = strlen(b_in);
		    if(s_in != 0) write(pipe_[1], b_in, s_in);
		}
		close(pipe_[1]);

		while(i<(size-1) && read(pipe_fd[0], (buf+i), 1) > 0){
//printf("n=%d w=%d",n,wrote);
//		    if(i >= (size-1)) break;
		    i++;
		}
		buf[i] = '\0';
		wait(&status);
	    } else {	//fork failed
		err = 2;
		close(pipe_[1]);
	    }

	    close(pipe_fd[0]);	//close rest
	}
      }else{
        err = 1;
        close(pipe_fd[0]);
        close(pipe_fd[1]);
      }

    } else err = 1;	//make a pipe failed
    if(err)	fprintf(stderr, "my_system: Unable to run child process %s, errno %d\n", cmd, err);
}


void my_system(FILE *out, char *cmd)
{
	int pid, status;
	int pipe_fd[2];
	int wrote;
	int err = 0;
	int buf;
	
    if (pipe(pipe_fd) == 0) {

	if (!(pid=fork())){	//child
	    close(pipe_fd[0]);	//read end

//	    close(fd);	    	//}for inetd
//	    close(sockfd);

	    close(1);
	    close(2);
	    dup2(pipe_fd[1], 1);	//1 -stdout
	    dup2(pipe_fd[1], 2);	//2 -stderr
	    close(pipe_fd[1]);

	    close(0);	//	/dev/stdin is off

//	    setsid();
	    execl("/bin/sh", "sh", "-c", cmd, (char *) 0);
//	    execl("/bin/sh", "sh", cmd, (char *) 0);
	    exit(1);
	}else{			//parent
	    close(pipe_fd[1]);	//write end
	    if (pid > 0) {	// fork -- parent
		while(read(pipe_fd[0], &buf, 1) > 0){
		    wrote = fwrite(&buf, 1, 1, out);
//printf("n=%d w=%d",n,wrote);
		    if(wrote < 1){
			err = 3;	//write failed
			break;
		    }
		}
//		waitpid(pid, &status, 0);
		wait(&status);
	    } else	//fork failed
		err = 2;

	    close(pipe_fd[0]);	//close rest
	
	}

    } else err = 1;	//make a pipe failed
    if(err)	fprintf(out, "my_system: Unable to run child process %s, errno %d\n", cmd, err);
}

void system_(char *cmd)
{
	int pid, status;
	int err = 0;
	
	if (!(pid=fork())){	//child

//	    close(fd);	    	//for inetd
//	    close(sockfd);

	    close(1);
	    close(2);

	    close(0);	//	/dev/stdin is off

//	    setsid();
	    execl("/bin/sh", "sh", "-c", cmd, (char *) 0);
//	    execl("/bin/sh", "sh", cmd, (char *) 0);
	    exit(1);
	}else{			//parent
	    if (pid > 0) {	// fork -- parent
		wait(&status);
	    } else	//fork failed
		err = 1;

	}

    if(err)	fprintf(stderr, "system: Unable to run child process %s, errno %d\n", cmd, err);
}

void write_shell(char *buf_in, long long size_in, char *buf, long long size_1, int mode, char *cmd){ //tmp- is max. 2048byte  if runned from CGI-script
	unsigned long long size = strncpy_(NULL, cmd, 0)+1;	//this is max. size of cmd-string
	char *arg;

	if(arg = malloc(size)){
	    strncpy_(arg, cmd, size);
	    write_system(buf_in, size_in, buf, size_1, mode, arg);
	    free(arg);
	}
}

void my_shell(FILE *out, char *tmp){		//tmp- is max. 2048byte  if runned from CGI-script
	unsigned long long size = strncpy_(NULL, tmp, 0)+1;	//this is max. size of arg-string
	char *arg;

	if(arg = malloc(size)){
	    strncpy_(arg, tmp, size);
	    my_system(out, arg);
	    free(arg);
	}
}

void shell(char *tmp){		//tmp- is max. 2048byte  if runned from CGI-script
	unsigned long long size = strncpy_(NULL, tmp, 0)+1;	//this is max. size of arg-string
	char *arg;

	if(arg = malloc(size)){
	    strncpy_(arg, tmp, size);
	    system(arg);
	    free(arg);
	}

}


char *get_var(unsigned long long *size_ptr, char *var_index){

    char *ptr = NULL;
    unsigned long long size = 0;	//if Zero - not write able
    char copybuf[16384];

    if(*var_index == '_'){	/* global variables */
	var_index++;

        if(*var_index == '_'){	//??__variable?? - show environment variable
	    var_index++;
	    ptr = getenv(var_index);
/*	}else if(*var_index == '#'){	//??_#variable?? - show command variable index.html?variable=5
	    var_index++;
	    if(*var_index == '#'){	//??_##variable?? - show local command variable index.html?variable=5
		var_index++;
		return get_arg(var_index, size_ptr, 1);
	    }
	    return get_arg(var_index, size_ptr, 0);
//	    if(ptr) size = strlen(ptr) + 1;//for remove_show_chars needed
*/	}else if(*var_index == '%'){	//??_%variable?? - show new_variable
	    var_index++;
	    return get_cfg_value(size_ptr, var_index, 1);
	}else if(*var_index == '&'){	//??_&variable?? - show fresh_variable
	    var_index++;
	    return get_cfg_value(size_ptr, var_index, 2);
	}else if(*var_index == '@'){	//??_@variable?? - show variable from rnd table
	    var_index++;
	    ptr = get_tbl(var_index);
	    if(ptr != NULL) size = strlen(ptr) + 1;		//for limit by show str.
	}else if(*var_index == '?'){	//??_?file|expression?? -> in file this expression
	    var_index++;

	FILE *fip;
	char *ptr1;
	
	
	ptr1 = w_strtok(&var_index, '|');
	if (ptr1){
	
	    if((fip = fopen(ptr1,"r")) == NULL)
		printf("No file: %s\n", ptr1);
	    else{
	    while(fgets(copybuf,sizeof(copybuf),fip) != NULL){
		if((ptr1 = parsestr1_(copybuf,var_index)) != NULL){
		    ptr = ptr1;
		    break;
		}
	    }
	    fclose(fip);
	    }
	}
	return ptr;
	} else if(!strncmp(var_index,"referrer", 8)){	//last .term file
	    struct page_n *p;
	    p = get_last_page(var_index+8);
	    if(p){
		ptr = p->name;
		size = p->size;
	    }
/*	} else if(!strcmp(var_index,"short_referer")){
		ptr = parsestr1_(referer, "/L/L");	//get the last '/'
	}else if(!strcmp(var_index,"user_agent")){
	    ptr = user_agent;
	}else if(!strcmp(var_index,"ip")){
	    ptr = ip;
	    size = 20;
	}else if(!strcmp(var_index,"port")){
	    ptr = port;
	    size = 10;
	}else if(!strcmp(var_index,"srv_ip")){
	    ptr = CONFIG.IP;
	}else if(!strcmp(var_index,"srv_port")){
	    ptr = CONFIG.ADMIN_PORT;
	}else if(!strcmp(var_index,"dns_name")){
	    ptr = dns_name;
*/	}else if(!strcmp(var_index,"etc_save")){
	    ptr = etc_save;
	    size = 2;
	}else if(!strcmp(var_index,"value")){
	    snprintf(value, 15, "%d", A1_BTN);
	    ptr = value;
	}else if(!strcmp(var_index,"version")){
	    ptr = version;
	    size = 256;
	}else if(!strcmp(var_index,"buf")){	//whole buffer
	    ptr = buf;
	    size = 65537;
	}else if(!strcmp(var_index,"buffer")){
	    ptr = buf;
	    if(buf_size >= 65537 /*|| buf_size == 0*/) size = 65537;//i think, it is right here!!
	    else size = buf_size;
	}

    }/*end of global variables*/
    else  return get_cfg_value(size_ptr, var_index, 0); /*if not found - return NULL*/

    if(size_ptr) *size_ptr = size; 
    return ptr;
}

