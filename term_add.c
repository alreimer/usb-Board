/* term_add.c:  A very simple connection programm for USB-Board
 *
 * Copyright (c) 2015 - 2020 Alexander Reimer <alex_raw@rambler.ru>
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
#include <sys/types.h>
#include <sys/wait.h>

#include "include/httpd.h"

#include "term_add.h"
#include "term_tbl.h"
#include "term_cgi.h"
#include "term_parser.h"
#include "terminal.h"

void print_pstr(FILE *out, char *tmp){
	char ch;
	while(*tmp){
	    ch = *tmp;
	    if(*tmp == '\\'){
		tmp++;
		switch(*tmp){
		    case 't':	ch = '\t'; break;
		    case 'n':	ch = '\n'; break;
		    case '\"':
		    case '\\': ch = *tmp; break;
		    case '0': return;
		}
	    }
	    putc(ch, out);
	    tmp++;
	}
}

unsigned char *point[2];		//0 - place where is ch_zero by /]; 1 - end of string
unsigned char ch_zero = '\0';
unsigned long number = 0;
unsigned long value_ = 0;
unsigned long stack_ = 0;

unsigned char *parsestr1(unsigned char *d, unsigned char *c)	//try identic strings!, "xxx*NULL" combination
{
	unsigned char *tmp, *tmp2, *tmp3, ch;
	unsigned int i;
	unsigned long digi;

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
					else if(*c == '/' && *(c+1) == 'B' && i <= 1024) {i++; c++;}
					else if(i != 0 && *c == '/' && *(c+1) == 'E'){ i--; c++;}
					else if(i == 0 && *c == '/' && *(c+1) == '\\'){c = c + 2; break;} // if(/\) 
					else if((tmp && (c == tmp)) || (*c == '\0')) return NULL;	//end of compare-strings - no matches - return NULL
					c++;
				    }
				}
//printf("str:%s d=%s c=%s\n", tmp2,d,c);
				if(tmp && *tmp2) {d = tmp2; /*tmp2 is not always the end of string!! check it out*/
					c = tmp + 2; continue;}
				else return tmp2;
		    case '{': c++; ch = *c; c++; tmp = NULL; tmp2 = c; i = 0;
					while(*tmp2){
					    if(*tmp2 == '/' && *(tmp2+1) == '/') tmp2++; //tmp2+=2
					    else if(*tmp2 == '/' && *(tmp2+1) == '{' && i <= 1024) i++; //if(/{)
					    else if(*tmp2 == '/' && *(tmp2+1) == '}'){
						if(i == 0){tmp = tmp2 + 2; break;}
						else i--;
					    }
					    tmp2++;
					}
				//tmp - end of compare-string
				//tmp2 - can be used,
				
				//  /t/{*/*="/*/N\\N"/t/}-->
				//  /t/{-/*="/*/N\\N"/t/}-->
				//	 |		 |
				//	 c		tmp
				if(ch == '*'){//  /{*..../}  - 0 Times or more
					while(1){
					    if(tmp){
						tmp2 = parsestr1(d, tmp);
						if(tmp2) return tmp2;
					    }
					    tmp2 = parsestr1(d, c);
					    if(tmp2 == NULL) return NULL;
					    if(*tmp2 == '\0') return tmp2;
					    d = tmp2;
					}
				}
				if(ch == '-'){//  /{-..../}  - 1 Time or more
					while(1){
					    tmp2 = parsestr1(d, c);
					    if(tmp2 == NULL) return NULL;
					    if(*tmp2 == '\0') return tmp2;
					    d = tmp2;
					    if(tmp){
						tmp2 = parsestr1(d, tmp);
						if(tmp2) return tmp2;
					    }
					}
				}
				if(ch == 'R' || ch == 'r'){//  /{R..../}  - Repead 1 Time or more
					i = 0;			// /{r..../}  - Repead 0 Time or more
					while(1){
					    tmp2 = parsestr1(d, c);
					    if(tmp2 == NULL) break;
					    if(tmp2 == d) break;	//no progress
					    d = tmp2; i = 1;
					}
					if(ch == 'R' && i == 0) return NULL;
					c = tmp; continue;
				}
				if(ch == 'C'){//  /{C..../}  - Compare witch pointer is bigger
					tmp2 = parsestr1(d, c);
					if(tmp2 == NULL) return NULL;
					tmp3 = parsestr1(d, tmp);
					if(tmp3 == NULL) return NULL;
					if(tmp3 > tmp2) return tmp3;
					return NULL;
				}
				return NULL;	//default, if not '*' and not '-'
		    case '(': c++; ch = *c; c++; tmp = c; i = 0;
					while(1){
					    if(*tmp == '\0') return NULL;
					    else if(*tmp == '/' && *(tmp+1) == '/') tmp++; //tmp+=2
					    else if(*tmp == '/' && *(tmp+1) == '(' && i <= 1024) i++; //if( /( )
					    else if(*tmp == '/' && *(tmp+1) == ')'){	//if( /) )
						if(i == 0){tmp = tmp + 2; break;}//think about!! this is /) before /: found
						else i--;
					    }
					    else if(i == 0 && *tmp == '/' && *(tmp+1) == ':'){
						tmp = tmp + 2;
						break;	//if( /) )
					    }
					    tmp++;
					}
				// in c - plus, and in tmp - minus
					if(ch == 'S'){		///(S.../:.../) - use Stack with !manual! inc- decrimintation
					i = 0; tmp2 = d; tmp3 = d;
					while(1){
						tmp2 = parsestr1(d, c);
						if(tmp2){
						    if(tmp2 != d) d = tmp2;	//progress!!
						    else tmp2 = NULL;
						}
						if(i && stack_ == 0) break;
						if(tmp2 == NULL && tmp3 == NULL) return NULL;

						tmp3 = parsestr1(d, tmp);
						if(tmp3){
						    if(tmp3 != d) d = tmp3;	//progress!!
						    else tmp3 = NULL;
						}
						if(i && stack_ == 0) break;
						if(tmp2 == NULL && tmp3 == NULL) return NULL;

						i = 1;
					}
					} else return NULL;
				// in d - end of parsed string
					i = 0;
					while(1){
					    if(*tmp == '\0') return NULL;
					    else if(*tmp == '/' && *(tmp+1) == '/') tmp++; //tmp+=2
					    else if(*tmp == '/' && *(tmp+1) == '(' && i <= 1024) i++; //if( /( )
					    else if(*tmp == '/' && *(tmp+1) == ')'){	//if( /) )
						if(i == 0){c = tmp + 2; break;}
						else i--;
					    }
					    tmp++;
					}
				continue;
		    case '\\':
		    case 'E':
		    case ':':
		    case ')':
		    case '}':	point[1] = d;
				return d;
	    /*  set number to /n10n   or stack to /n15s*/
		    case 'n': c++; tmp = c;
				while(*tmp && *tmp != 'n' && *tmp != 's') tmp++;
				    digi = 0;
				    i = 0;
				    while(c[i] >= '0' && c[i] <= '9'){ digi = digi*10 + (c[i] - '0'); i++; if(i > 5) break;}
				    c = tmp;
				    if(*c == 'n') number = digi;	//set number to digi
				    else if(*c == 's') stack_ = digi;
				    if(*c) c++; //if not end - increase c.
				    continue;
	    /* by /sn   - set number = source */
	    /* by /sv   - set value = source */
	    /* by /ss   - set stack = source */
		    case 's': c++; ch = *c; i = 0;
				digi = 0;
				while(*d >= '0' && *d <= '9'){
				    if(i < 7){ digi = digi*10 + (*d - '0'); i++;}
				    d++;
				}
				if(i == 0) return NULL; //in *d no digits
				if(ch == 'n'){ c++; number = digi; continue;}
				else if(ch == 'v'){ c++; value_ = digi; continue;}
				else if(ch == 's'){ c++; stack_ = digi; continue;}
				else { c = c - 2; break;} //go back to c = /sv(n) and parse it
	    /* by /hn   - set number = source (hex) */
	    /* by /hv   - set value = source (hex) */
	    /* by /hs   - set stack = source (hex) */
		    case 'h': digi = 0; ch = 0;
				while(1){
				    i = *d;
				    if(i >= '0' && i <= '9') ch = '0';
				    else if(i >= 'A' && i <= 'F') ch = 'A' - 10;
				    else if(i >= 'a' && i <= 'f') ch = 'a' - 10;
				    else break;
				    digi = (digi<<4) + i - ch;
				    d++;
				}
				if(ch == 0) return NULL;
				c++;
				if(*c == 'n'){ c++; number = digi; continue;}
				else if(*c == 'v'){ c++; value_ = digi; continue;}
				else if(*c == 's'){ c++; stack_ = digi; continue;}
				else { c = c - 2; break;} //go back to c = /hv(n) and parse it
	    /* by /iv<10v or /iv>5v or /in<45n or /in>24n or /is>0s - if not matches return NULL */
	    /* by /iv+10v or /iv-5v or /in-45n or /in+24n or /is+1s */
		    case 'i': c++; ch = *c; 
				if(ch != 'v' && ch != 'n' && ch != 's'){ c = c - 2; break;}
				c++; i = *c;
				if(i != '<' && i != '>' && i != '+' && i != '-'){ c = c - 3; break;}
				c++; digi = 0;
				while(*c){
				    if(*c >= '0' && *c <= '9'){ digi = digi*10 + (*c - '0'); c++; continue;}
				    else if(*c == ch) break;
				    else{ break;}
				}
				if(*c == ch){
				    if(i == '+'){
					if(ch == 'v'){ value_ += digi;}
					else if(ch == 'n'){ number += digi;}
					else if(ch == 's'){ stack_ += digi;}
					c++; continue;
				    }else if(i == '-'){
					if(ch == 'v'){ value_ -= digi;}
					else if(ch == 'n'){ number -= digi;}
					else if(ch == 's'){ stack_ -= digi;}
					c++; continue;
				    }
				    unsigned long val = 0;
				    if(ch == 'v'){ val = value_;}
				    else if(ch == 'n'){ val = number;}
				    else if(ch == 's'){ val = stack_;}
				    if(i == '>'){ if(val <= digi) return NULL;}
				    else if(i == '<'){ if(val >= digi) return NULL;}
				    c++; continue;
				}else return NULL;
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
				    case '\\':
				    case '-':
				    case '\"':	ch = *c;break;
				    case 't':	ch = '\t';break;
				    case 'n':	ch = '\n';break;
				    default: c--; ch = *c; break;
				    }
				} else {
				    ch = *c;
				    if(*(c+1) == '-' && *(c+2)){	//if /<-a-z/> --> (ch=a <= *d <= *(c+2)=z)
					if((ch < *(c+2)) && (*d >= ch) && (*d <= *(c+2))){
					    do{if(*d == '\0') break; d++;}while((*d >= ch) && (*d <= *(c+2)));
					    i = 1;
					    if(*d){ c = tmp; continue;}
					}
					c = c + 3; continue;
				    }
				}
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
				    case '\\':
				    case '-':
				    case '\"':	ch = *c;break;
				    case 't':	ch = '\t';break;
				    case 'n':	ch = '\n';break;
				    case '0':	ch = '\0';break;//check if *d == '\0'.
							//In /* is set until zero, inclusive zero!
				    default: c--; ch = *c; break;
				    }
				} else {
				    ch = *c;
				    if(*(c+1) == '-' && *(c+2)){	//if /<1a-z/> --> (ch=a <= *d <= *(c+2)=z)
					if((ch < *(c+2)) && (*d >= ch) && (*d <= *(c+2))) i = 1;
					c = c + 3; continue;
				    }
				}
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
		    case '?':		// /?variable?/	- nicht eingebetet(simple)
		    case 'Q':		// /Qvar?/	- eingebetet
			ch = *c;	//in ch: Q or ?
			c++;
			char *m;
			tmp = c;
			tmp2 = NULL;
			while(*tmp){	//   /?var?/ - is old, /?var/? - is new
			    if((*tmp == '?' && *(tmp+1) == '/') || (*tmp == '/' && *(tmp+1) == '?')){
				i = tmp - c + 1;
				if((i > 1) && (i < 33)){
				m = malloc(i);
				if(m){
				    strmycpy(m, c, i);
				    tmp2 = get_var(NULL, m);		//get_var and get_variable
				    free(m);
				}
				}
				c=tmp+2;
				break;
			    }
			    tmp++;
			}

			if(tmp2 && *tmp2){
			    if(ch == '?'){
				i = strlen(tmp2);
				if(i && strncmp(d, tmp2, i)) return NULL;
				d = d + i;
			    }else{
				static int time = 0;
				time++;
				if(time < 10){
				    unsigned long long size = strncpy_(NULL, tmp2, 0)+1;	//this is max. size of tmp2-string
				    if(size > 1 && (m = malloc(size))){
				    strncpy_(m, tmp2, size);
				    tmp = parsestr1(d, m);
				    free(m);
				    if(!tmp){time--; return NULL;}
				    d = tmp;
				    //if(*d != '\0') d++;		//????????
				    }
				}else printf("parsestr: max. counter\n");
				time--;
			    }
			}
			else return NULL;

			continue;		//it's or c++ or c=tmp+2

		    case '-': if(*(c+1) == '-') { d=d-1; c=c+2;} //  /--   -means d-1
			continue;

		    case 'm': c++; ch = *c; c++; tmp2 = c;// /m-WARN: not matched!\0thisIsNotMatched!
			while(*tmp2){
			    if(*tmp2 == '\\' && *(tmp2+1) == '\\') tmp2++;	//tmp2 = tmp2 + 2
			    else if(*tmp2 == '\\' && *(tmp2+1) == '0'){ tmp2 = tmp2 + 2; break;}	//string must have \\0 at the end
			    tmp2++;						//and don't have /\\ or /}, /), /E, /:
			}
			tmp = parsestr1(d, tmp2);
			if(ch == '-' && tmp == NULL) print_pstr(stderr, c);
			return tmp;
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
	    /* \0 the same as "bla/"; if after is "bla\02" -> "bla\0" + number=2 */
		    case '0': if(*d != '\0') return NULL;
				    c++; int digi = 0; i = 0;
				    while(c[i] >= '0' && c[i] <= '9'){ digi = digi*10 + (c[i] - '0'); i++; if(i > 5) break;}
				    if(i) number = digi;	//set number to digi
				point[1] = d;//new 14.07.2019
				return d;
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

    number = 0;
    point[0] = NULL;
    if(tmp = parsestr1(d, c) /*&& (ptr != NULL)*/){
	ptr->ch = ch_zero;
	ptr->num = number;
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
	    if(*var_index == '_'){	//??_@_variable?? - show begin variable from table
		var_index++;
		unsigned int *i;
		i = get_tbl_begin(var_index);
		if(i){
		    snprintf(value, 15, "%d", *i);
		    ptr = value;
		}
	    } else {
		ptr = get_tbl(var_index);
		if(ptr != NULL) size = strlen(ptr) + 1;		//for limit by show str.
	    }
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
	    p = get_last_page(var_index+8, 1);
	    if(p){
		ptr = p->name;
		size = p->size;
	    }
	} else if(!strncmp(var_index,"back", 4)){	//last .term file
	    struct page_n *p;
	    p = get_last_page(var_index+4, 0);
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
	}else if(!strcmp(var_index,"auth")){
//	    snprintf(value, 15, "%d", auth);
//	    ptr = value;
	    ptr = auth;
	    size = 5;
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

