/* term_add.c:  A very simple connection programm for USB-Board
 *
 * Copyright (c) 2015 - 2021 Alexander Reimer <alex_raw@rambler.ru>
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
#include <sys/wait.h>
#include <sys/types.h>

#include "include/httpd.h"

#include "term_add.h"
#include "term_tbl.h"
#include "term_cgi.h"
#include "term_parser.h"
#include "terminal.h"

//to use httpd descriptor in terminal (declared in parsestr1(/mW) and so on)
#define fdcr fp

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

//in parameters
FILE *fdtr = NULL;
char *arg_ = NULL;
char parse_flags = 0b00000000;
exec_fnctn *fn_exec = NULL;	//if NULL -> no /e (or /c)->no_more because of /W
unsigned char *bucks = NULL;
//end of in

//internal pars
unsigned char *point[3] = {NULL, NULL, NULL};		//0 - place where is ch_zero by /]; 1 - end of string; 2 - technical var
unsigned char ch_zero = '\0';

unsigned char *begin_ = NULL;
//unsigned char *end_ = NULL;
unsigned long number = 0;
unsigned long value_ = 0;
unsigned long stack_ = 0;

unsigned long exec_ = 0;	//if == 0 -> no /c
unsigned long ca_se_ = 0;
//end of internals
//only internal pars
char stop_ = 0;			//if == 1 -> stop parsing, return NULL;
struct cascade *cascade = NULL;
struct strctexec *point_exec = NULL;//if NULL -> no /e
//end of only internals

void free_cascade_1(struct cascade **ptr){
	if(!ptr || !*ptr) return;
	free_cascade_1(&((*ptr)->next));
	free(*ptr);
	*ptr = NULL;
}
void free_cascade(void){
	free_cascade_1(&cascade);
}

unsigned char *parsestr1(unsigned char *d, unsigned char *c)	//try identic strings!, "xxx*NULL" combination
{
	unsigned char *tmp, *tmp2, *tmp3, *tmp4, *tmp5, ch;
	unsigned char *text = "parsestr(): too much /\\ or /E";
	unsigned int i;
	unsigned long digi, val, stc, exc, cse;



	while (*c)
	{
	    
	    if(*c == '/' ){
		c++;
		switch (*c){
    		    case '*':/* printf("d1:%s\n", d);*/
			      c++; while(1){tmp = parsestr1(d, c); if (tmp) return tmp; if(! *d) break; d++;} return NULL;//depend on continues parsing
							//if * in b string -> some symbols between
//	!a*(b+c) = !(a+!(b+c))->	/!/Ba/\/!/Bb/\c/\/E/E
		    case 'B': c++;

//check				tmp3 = NULL;

//printf("/B%.5s--/E%.5s--\n",c, tmp );
				while(1){	//not matched
				    tmp3 = point[1];//Use only in parsestr, parsestr1_ or parsest2. it must be NULL at the beginning
				    tmp = parsestr1(d, c);
//check				    if(tmp3 != NULL && tmp3 > point[1]) point[1] = tmp3;

/*				    if(tmp){
					if(tmp3 > point[1]) point[1] = tmp3;//Use only in parsestr, parsestr1_ or parsest2
				        break;
				    }
*/

				    if(tmp3 > point[1]) point[1] = tmp3;//Use only in parsestr, parsestr1_ or parsest2 or parsestr2_s
				    if(tmp) break;

				    i = 0;
				    while(1){
					if(*c == '/' && *(c+1) == '/') c++; //c+=2
					else if(*c == '/' && *(c+1) == 'B' && i <= 1024) {i++; c++;}
					else if(i == 0 && *c == '/' && *(c+1) == '\\'){c = c + 2; break;} // if(/\) 
					else if(*c == '/' && *(c+1) == 'E'){	//if(/E)
						if(i == 0){return NULL;}	//nothing to compare
						else{ i--; c++;}
					}

					else if(*c == '\0') return NULL;	//end of compare-strings - no matches - return NULL
/*					else if(*c == '/' && *(c+1) == 'm' && (tmp4 = parsestr2_s(NULL, NULL, c, message))){
						c = tmp4;
						continue;
					}
*/
					c++;
				    }

//check				    tmp3 = point[1];
				}
//printf("str:%.5s d=%.5s c=%.5s\n", tmp,d,c);

				i = 0;
				while(1){
				    if(*c == '\0') return tmp;
				    else if(*c == '/' && *(c+1) == '/') c++; //c+=2
				    else if(*c == '/' && *(c+1) == 'B' && i <= 1024) {i++; c++;}
				    else if(*c == '/' && *(c+1) == 'E'){	//if(/E)
					if(i == 0){ c += 2; break;}
					else{ i--; c++;}
				    }
				    /*c == /m --needed here because of recursivity*/
/*				    else if(*c == '/' && *(c+1) == 'm' && (tmp3 = parsestr2_s(NULL, NULL, c, message))){
					c = tmp3;
					continue;
				    }
*/
				    c++;
				}

				//in c last after /E
				d = point[1];
				continue;
		    case '{': c++; ch = *c; c++; /* tmp = NULL; */ tmp2 = c; i = 0;
					while(1){
					    if(*tmp2 == '\0') return NULL;//new here 23.11.2021
					    if(*tmp2 == '/' && *(tmp2+1) == '/') tmp2++; //tmp2+=2
					    else if(*tmp2 == '/' && *(tmp2+1) == '{' && i <= 1024){ i++; tmp2++;} //if(/{)
					    else if(*tmp2 == '/' && *(tmp2+1) == '}'){
						if(i == 0){tmp = tmp2 + 2; break;}
						else{ i--; tmp2++;}
					    }

/*					    else if(*tmp2 == '/' && *(tmp2+1) == 'm' && (tmp3 = parsestr2_s(NULL, NULL, tmp2, message))){
						tmp2 = tmp3;
						continue;
					    }
*/
					    tmp2++;
					}
				//tmp - end of compare-string
				//tmp2 - can be used,
				
				//  /t/{*/*="/*/N\\N"/t/}-->
				//  /t/{-/*="/*/N\\N"/t/}-->
				//	 |		 |
				//	 c		tmp

				if(ch == '*'){//  /{*..../}  - 0 Times or more
//printf("/{%.5s--/}%.5s--\n",c,tmp );
					while(1){
					    if(tmp){
						tmp2 = parsestr1(d, tmp);
//20220813						if(stop_){ stop_ = 0; return NULL;}
						if(tmp2 /*&& tmp2 != d*/) return tmp2;
					    }
					    tmp3 = point[1];
				point[1] = NULL;
					    tmp2 = parsestr1(d, c);
				if(point[1] == NULL) point[1] = tmp3;
					    if(tmp2 == NULL) return NULL;
					    if(stop_){ stop_ = 0; return NULL;}
					     //returned by /\ or /E and not from /}
					    if(point[1] != point[2]){printf("%s #*\n", text); return NULL;}
//					    if(tmp3 != point[1]) tmp2 = point[1];	//new here
//----					    if(tmp2 == d) return NULL;		//no progress
//----					    d = tmp2;
				if(d != point[2]) d = point[2];	//progress!!
				else return NULL;
					}
				}
				if(ch == '-'){//  /{-..../}  - 1 Time or more
//printf("chars: %.3s\n", tmp);
					while(1){
					    tmp3 = point[1];
				point[1] = NULL;
					    tmp2 = parsestr1(d, c);
				if(point[1] == NULL) point[1] = tmp3;
					    if(tmp2 == NULL) return NULL;
					    if(stop_){ stop_ = 0; return NULL;}
					     //returned by /\ or /E and not from /}
					    if(point[1] != point[2]){printf("%s #-\n", text); return NULL;}
//					    if(tmp3 != point[1]) tmp2 = point[1];	//new here
//-----					    if(tmp2 == d) return NULL;		//no progress
//-----					    d = tmp2;
				if(d != point[2]) d = point[2];	//progress!!
				else return NULL;
					    if(tmp){
//printf("charsd: %.3s\n", d);
						tmp2 = parsestr1(d, tmp);
//20220813						if(stop_){ stop_ = 0; return NULL;}
						if(tmp2 /*&& tmp2 != d*/) return tmp2;
					    }
					}
				}
				if(ch == 'R' || ch == 'r'){//  /{R..../}  - Repead 1 Time or more
					i = 0;			// /{r..../}  - Repead 0 Time or more
					while(1){
//21.11.2021					    tmp3 = point[1];
					    tmp2 = parsestr1(d, c);
					    if(tmp2 == NULL) break;
					    if(stop_){ stop_ = 0; break;}
					     //returned by /\ or /E and not from /}
					    if(point[1] != point[2]){printf("%s #R\n", text); return NULL;}
//21.11.2021					    if(tmp3 != point[2]) tmp2 = point[2];	//new here
					    if(d != point[2]) d = point[2];	//progress!!
					    else break;
/*23.11.2021					    if(d != point[2]) tmp2 = point[2];	//new here
					    if(tmp2 == d) break;	//no progress
					    d = tmp2;*/ i = 1;
					}
					if(ch == 'R' && i == 0) return NULL;
					c = tmp; continue;
				}
				if(ch == 'C'){//  /{C..../}  - Compare witch pointer is bigger
					tmp2 = parsestr1(d, c);
					if(tmp2 == NULL) return NULL;
					 //returned by /\ or /E and not from /}
					if(point[1] != point[2]){printf("%s #C\n", text); return NULL;}
					tmp2 = point[2];
					tmp3 = parsestr1(d, tmp);
					if(tmp3 == NULL) return NULL;
					tmp3 = point[1];
					if(tmp3 > tmp2) return tmp3;
					return NULL;
				}
				return NULL;	//default, if not '*' and not '-'
		    case '(': c++; ch = *c; c++; tmp = c; i = 0; tmp4 = NULL;
					while(1){
					    if(*c == '\0') return NULL;
					    else if(*c == '/' && *(c+1) == '/') c++; //c+=2
					    else if(*c == '/' && *(c+1) == '(' && i <= 1024) i++; //if( /( )
					    else if(*c == '/' && *(c+1) == ')'){	//if( /) )
						if(i == 0){c = c + 2; break;}
						else i--;
					    }
					    else if(i == 0 && *c == '/' && *(c+1) == ':'){
						if(tmp4 == NULL) tmp4 = c + 2;
						else return NULL;//double /(.../:.../:.../) in parse string
					    }
					    c++;
					}
					if(tmp4 == NULL) return NULL;//no /: in /(.../)

				// in tmp - plus, and in tmp4 - minus, in c - end of ( /)... )
					if(ch == 'S'){		///(S.../:.../) - use Stack with !manual! inc- decrimintation
					i = 0; tmp2 = d; tmp3 = d;
					while(1){
						tmp2 = parsestr1(d, tmp);
						if(tmp2){
//						    if(tmp2 != d) d = tmp2;	//progress!!
						    if(d != point[1]) d = point[1];	//progress!!
						    else tmp2 = NULL;
						}
						if(i && stack_ == 0) break;
						if(tmp2 == NULL && tmp3 == NULL) return NULL;

						tmp3 = parsestr1(d, tmp4);
						if(tmp3){
//						    if(tmp3 != d) d = tmp3;	//progress!!
						    if(d != point[1]) d = point[1];	//progress!!
						    else tmp3 = NULL;
						}
						if(i && stack_ == 0) break;
						if(tmp2 == NULL && tmp3 == NULL) return NULL;

						i = 1;
					}
					} else return NULL;
				// in d - end of parsed string in c - str after /)...
				continue;
		    case ':':
		    case ')':
		    case '}':	point[2] = d;
		    case '\\':
		    case 'E':	point[1] = d;//importend !!!
				return d;
		    case 'P':	c++;ch = *c;		//make var in stack
				if(ch == 'S') digi = stack_;	//if /PS -> stack_ in stack
				tmp = parsestr1(d, c+1);
				if(ch == 'S') stack_ = digi;
				return tmp;
		    case 'S':	c++;ch = *c;		//make stop
				if(ch == 't') stop_ = 1;	//if /St -> stop
				c++;
				continue;
	    /*  set number to /n10N   or stack to /n15S or exec to /n15E or value to /n20V*/
		    case 'n': c++; tmp = c;
				while(*tmp && *tmp != 'N' && *tmp != 'S' && *tmp != 'E' && *tmp != 'V' &&
					*tmp != 'n' && *tmp != 's' && *tmp != 'e' && *tmp != 'v') tmp++;
				    digi = 0;
				    i = 0;
				    while(c[i] >= '0' && c[i] <= '9'){ digi = digi*10 + (c[i] - '0'); i++; if(i > 5) break;}
//				    c = tmp;
				    /* forwart wave */
				    ch = *tmp;
				    if(ch == 'N') number = digi;	//set number to digi
				    else if(ch == 'S') stack_ = digi;
				    else if(ch == 'E') exec_ = digi;
				    else if(ch == 'V') value_ = digi;
//				    if(*c) c++; //if not end - increase c.
//				    continue;
				    tmp2 = parsestr1(d, tmp+1);
				    /* back wave */
				    if(tmp2){		//only if ok
				    if(ch == 'n') number = digi;	//set number to digi
				    else if(ch == 's') stack_ = digi;
				    else if(ch == 'e') exec_ = digi;
				    else if(ch == 'v') value_ = digi;
				    }
				    return tmp2;
	    /* by /sN   - set number = source */
	    /* by /sV   - set value = source */
	    /* by /sS   - set stack = source */
		    case 's': c++; ch = *c; i = 0;
				digi = 0;
				while(*d >= '0' && *d <= '9'){
				    if(i < 7){ digi = digi*10 + (*d - '0'); i++;}
				    d++;
				}
				if(i == 0) return NULL; //in *d no digits
				if(ch == 'N'){ c++; number = digi; continue;}
				else if(ch == 'V'){ c++; value_ = digi; continue;}
				else if(ch == 'S'){ c++; stack_ = digi; continue;}
				else { /*remove printf*/ printf("var: /s%c\n", ch); c = c - 2; break;} //go back to c = /sv(n) and parse it
	    /* by /hN   - set number = source (hex) */
	    /* by /hV   - set value = source (hex) */
	    /* by /hS   - set stack = source (hex) */
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
				ch = *c;
				if(ch == 'N'){ c++; number = digi; continue;}
				else if(ch == 'V'){ c++; value_ = digi; continue;}
				else if(ch == 'S'){ c++; stack_ = digi; continue;}
				else { c = c - 2; break;} //go back to c = /hv(n) and parse it
	    /* by /iv<10v or /iv>5v or /in<45n or /in>24n or /is>0s or /ie>10e - if not matches return NULL */
	    /* by /iv+10v or /iv-5v or /in-45n or /in+24n or /is+1s or /ie+2e */
		    case 'i': c++; ch = *c; 
				if(ch != 'v' && ch != 'n' && ch != 's' && ch != 'e'){ c = c - 2; break;}
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
					else if(ch == 'e'){ exec_ += digi;}
					c++; continue;
				    }else if(i == '-'){
					if(ch == 'v'){ value_ -= digi;}
					else if(ch == 'n'){ number -= digi;}
					else if(ch == 's'){ stack_ -= digi;}
					else if(ch == 'e'){ exec_ -= digi;}
					c++; continue;
				    }
				    unsigned long val = 0;
				    if(ch == 'v'){ val = value_;}
				    else if(ch == 'n'){ val = number;}
				    else if(ch == 's'){ val = stack_;}
				    else if(ch == 'e'){ val = exec_;}
				    if(i == '>'){ if(val <= digi) return NULL;}
				    else if(i == '<'){ if(val >= digi) return NULL;}
				    c++; continue;
				}else return NULL;
	    /* skip zero or one character*/
		    case '0': tmp = d; while(tmp <= d+1){tmp = parsestr1(tmp, c+1); if (tmp) return tmp; tmp++;} return NULL;
	    /* skip one symbol in d, exept \0 */
		    case '|': c++; if(*d == '\0') return NULL; d++; continue;
	    /* by x/.x. skip x-symbol 1 or more times */
	    /* by /.x. skip x-symbol all at once (0 or more times)*/
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
				    case ':':	//used in change_line
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
				    }//if /<--a/> --> (ch=- && *(c+1)=a) --> so match only d with '-' and wenn 'a'
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
				    case ':':	//used in change_line
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
		    case '[': if(parsestr1(d, c+1)){ /*point[0] = d;*/ begin_ = d; return d;} return NULL;		//if [ in b so this pointer will be returned, and c is move forward. BEGINNofSTR

		    case ']':/* printf("d:%s\n", d);*/
//			      end_ = d;
			      tmp = parsestr1(d, c+1);
			      if(!tmp) return NULL;
			      //case tmp=d (*(c+1) = '/0')[ end braces in end of string]
			      if(tmp == d && *tmp){ tmp++; point[1] = tmp;}
			      /*if(*d) point[0] = d+1; else */
			      point[0] = d;	// point[0] points on character to be zeroed
			      ch_zero = *d;
			      *d = '\0'; return tmp;//depend on continues(if /[ goes after /] - begin)
							// make end of string hier!, check rest as (]...) and return next to end char.
		    case 'r'://recover the end
				if(point[0]){
				    *(point[0]) = ch_zero;//be carefull with it!!. Use only in parsestr, parsestr1_ or parsest2
				    point[0] = NULL;
				}
				c++;
			    continue;
	    /* second '/' charakter is to match */
		    case '/': break;
//		    case '\0': continue;//need to check it!!!!!!

	    /* end of string is to match */
		    case '\0': if(*d != '\0') return NULL; point[1] = d; return d;
		
		    case '!':		//Negotiation
					//  /!/*ab	-no more "ab" in rest of string
			    tmp2 = point[1];
			    tmp = parsestr1(d, c+1);
			    if(tmp)	return NULL;
			    else { if(tmp2 == point[1]) point[1] = d; return d;}
		    case '$'://		/$	-get variable from outside
			if(bucks){
				i = strlen(bucks);
				if(i && strncmp(d, bucks, i)) return NULL;
				d = d + i;
			}
				c++;
			continue;
		    case 'Q':		// /Q var | <?if? compare /?str/? or str ?fi? | par /q - nicht eingebetet
		    case '?':		// /?variable?/	- nicht eingebetet(simple)	// /?$var?/	- eingebetet
			ch = *c;	//in ch: Q or ?
			c++;
			char *m, *t, ch1, ch2 = '\0';
			if(*c == '$') {ch2 = *c; c++;}//in ch2: $
			while(*c == ' ') c++;
			tmp = c;
			tmp2 = NULL;

			tmp5 = bucks;
			tmp3 = point[0];
			tmp4 = point[1];
			ch1 = ch_zero;
			digi = number;
			val = value_;
			stc = stack_;
			exc = exec_;

			if(ch == 'Q'){
			i = 0;

			while(*tmp){	//   /Q var q/
				if(*tmp == '/' && *(tmp+1) == '/') tmp++; //tmp+=2
				else if(*tmp == '/' && *(tmp+1) == 'Q' && i <= 1024) i++; //if(/Q)
				else if(*tmp == '/' && *(tmp+1) == 'q'){	//if(/q)
				    if(i == 0){
					t = tmp;
					while(*(tmp-1) == ' ' && tmp != c) tmp--;
					i = tmp - c + 1;
					if(i > 1){
					m = malloc(i);
					if(m){
					    strmycpy(m, c, i);
					    tmp2 = get_var(NULL, m);		//get_var and get_variable
					    free(m);
					}
					}
					c=t+2;
					break;
				    }
				    else i--;
				}
			    tmp++;
			}

			} else {//ch is not 'Q', is ?
			while(*tmp){	//   /?var?/ - is old, /? var /? - is new
			    if((*tmp == '?' && *(tmp+1) == '/') || (*tmp == '/' && *(tmp+1) == '?')){
				t = tmp;
				while(*(tmp-1) == ' ' && tmp != c) tmp--;
				i = tmp - c + 1;
				if((i > 1) && (i < 33)){
				m = malloc(i);
				if(m){
				    strmycpy(m, c, i);
				    tmp2 = get_var(NULL, m);		//get_var and get_variable
				    free(m);
				}
				}
				c=t+2;
				break;
			    }
			    tmp++;
			}
			}

			exec_ = exc;
			stack_ = stc;
			value_ = val;
			number = digi;
			ch_zero = ch1;
			point[1] = tmp4;
			point[0] = tmp3;
			bucks = tmp5;

			if(tmp2 /*&& *tmp2*/){//this is comment for access_folders.htm
			    if(ch2 != '$'){
				i = strlen(tmp2);
				if(i && strncmp(d, tmp2, i)) return NULL;
				d = d + i;
			    }else{
				static int time = 0;
				time++;
				if(time < 10){
/*				    tmp3 = point[0];
				    tmp4 = point[1];
				    ch1 = ch_zero;
				    digi = number;
				    val = value_;
				    stc = stack_;
				    exc = exec_;
*/
				    unsigned long long size = strncpy_(NULL, tmp2, 0);	//this is max. size of tmp2-string
				    if(size && (m = malloc(size+1))){
				    strncpy_(m, tmp2, size);

				    exec_ = exc;
				    stack_ = stc;
				    value_ = val;
				    number = digi;
				    ch_zero = ch1;
				    point[1] = tmp4;
				    point[0] = tmp3;
				    bucks = tmp5;

				    tmp = parsestr1(d, m);
/* maybe is needed?
				    exec_ = exc;
				    stack_ = stc;
				    value_ = val;
				    number = digi;
				    ch_zero = ch1;
				    point[1] = tmp4;
				    point[0] = tmp3;
				    bucks = tmp5;
*/
				    free(m);
				    if(!tmp){time--; return NULL;}
				    d = tmp;//maybe d = point[1]
				    //c -next current part of string
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

		    case 'm': c++; ch = *c; c++; tmp2 = c;// /m-WARN: not matched!\0 -thisIsNotMatched!
			while(*tmp2){
			    if(*tmp2 == '\\' && *(tmp2+1) == '\\') tmp2++;	//tmp2 = tmp2 + 2
			    else if(*tmp2 == '\\' && *(tmp2+1) == '0'){ tmp2 = tmp2 + 2; break;}	//string must have \\0 at the end
			    tmp2++;						//and don't have /\\ or /}, /), /E, /:
			}
			if(ch == 'w') print_pstr(fdcr, c); //putout to WEB: /mw....\0  without matching!
			if(ch == '*') print_pstr(stderr, c); //putout to console: /m*....\0 without matching. used for testing of strings
			tmp = parsestr1(d, tmp2);
			if(ch == '-' && tmp == NULL) print_pstr(stderr, c);
			if(ch == '+' && tmp != NULL) print_pstr(stderr, c); //if matched the rest - putout to stderr: /m+...\0
			if(ch == 'N' && tmp == NULL) print_pstr(fdcr, c); //if not mantched the rest - putout to WEB: /mN...\0
			if(ch == 'W' && tmp != NULL) print_pstr(fdcr, c); //if matched the rest - putout to WEB: /mW....\0
			return tmp;
			// /mw..\0/mW..\0/?par/?	==	/mw..\0/?par/?/mW..\0
		    case 'c':	//collect strctexec struct
			c++; i = 0; ch = 0xff;//set to default(EV)
			while(*c){
			    if(*c == 'c') break;
			    if(i > 1) return NULL;//limit parameters to 2chars
			    if(*c == 'E') ch = ch | 1;
			    if(*c == 'V') ch = ch | 2;
			    if(*c == 'e') ch = ch & 0xfe;//11111110
			    if(*c == 'v') ch = ch & 0xfd;//11111101
			    c++; i++;
			}
			if(*c == '\0') c--;//shorter parameter
			if(/*exec_ == 0 ||*/ fn_exec == NULL){c++; continue;}
//			exec_ = 0;	//switch off  /c/c combination
			struct strctexec **p_exec, *pexc;
			p_exec = &(point_exec);
			while(*p_exec){p_exec = &((*p_exec)->next);}
			*p_exec = (struct strctexec *)malloc(sizeof(struct strctexec));
			pexc = *p_exec;
			if(pexc == NULL){printf("parserstr: Unable alloc memory\n"); return NULL;}
			point[0] = NULL;

					//those parameters must be befor /c
			if(ch & 1) pexc->exec = exec_;	//   /cEc
			    else pexc->exec = 0;//new here, for definition
			pexc->begin = d;//begin_;
			if(ch & 2) pexc->value = value_;	//   /cVc
			    else pexc->value = 0;//new here, for definition
			pexc->next = NULL;

			if(tmp = parsestr1(d, c+1)){//      /e .... /n10E .. /sV.... /cEVc/*/]....
						    //or    /e ..../sV. /{-.. /ceVc/*/].. /n1e.../}..
						    //or    /e .../n0E .. /cc.....   - means /cEVc
			pexc->end = point[0];		//those parameters must be after /c
			pexc->ch = ch_zero;
			if(!(ch & 1)) pexc->exec = exec_;	//   /cec
			if(!(ch & 2)) pexc->value = value_;	//   /cvc
//			begin_ = NULL;
//			point[0] = NULL;
//			end_ = NULL;
////////printf("exec: %ld\n", exec_);
			}else{
			*p_exec = (*p_exec)->next;	//cross connection
			if(pexc) free(pexc);		//free mem for this entry
			}
			return tmp;
		    case 'e':	//execute fn_exec() function
			if(tmp = parsestr1(d, c+1)){
			    if(point_exec){
				if(fn_exec){

				tmp5 = bucks;
				tmp2 = point[0];
				tmp3 = point[1];
				tmp4 = point[2];
				ch = ch_zero;
				digi = number;
				val = value_;
				stc = stack_;
				exc = exec_;
//				unsigned char *beg = begin_;
//				unsigned char *en = end_;
				struct strctexec *pexec = point_exec;
				exec_fnctn *fnexec = fn_exec;

				point_exec = NULL;//for new beginning
				fn_exec(pexec);
				free_strctexec();//free this new beginning

				fn_exec = fnexec;
				point_exec = pexec;
//				end_ = en;
//				begin_ = beg;
				exec_ = exc;
				stack_ = stc;
				value_ = val;
				number = digi;
				ch_zero = ch;
				point[2] = tmp4;
				point[1] = tmp3;
				point[0] = tmp2;
				bucks = tmp5;
				}
//				free_strctexec();	//and destroy it
			    }
			}
			free_strctexec();	//and destroy it
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
		    case ':':	//used in change_line
	    /* \" string is to match */
		    case '\"': if(*d != *c) return NULL; c++; d++; continue;
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

void free_strctexec_1(struct strctexec **ptr){
    if(!ptr || !*ptr) return;
    free_strctexec_1(&((*ptr)->next));
#ifdef DEBUG
    printf("free exec:'%ld' %s  ---%c---\n", (*ptr)->exec, (*ptr)->begin, (*ptr)->ch);
#endif
	//at hier action
    if((*ptr)->end && *((*ptr)->end) == '\0') *((*ptr)->end) = (*ptr)->ch;
    free(*ptr);
    *ptr = NULL;
}

void free_strctexec(void){
    free_strctexec_1(&point_exec);
}

unsigned char *parsestr(unsigned char *d, unsigned char *c){		//push and pop the pointers
    char *tmp, flags;

    point[0] = NULL;
    point[1] = NULL;
    point[2] = NULL;
    begin_ = NULL;
    struct cascade *cascade_ = cascade;
    cascade = NULL;

    flags = parse_flags;
    parse_flags = 0b00000000;//default setting

    tmp = parsestr1(d, c);

    parse_flags = flags;

    free_cascade();
    cascade = cascade_;
    if(begin_ == NULL) begin_ = tmp;

    return tmp;
}

char *parsestr1_(char *d, char *c){		//push and pop the pointers
    char *tmp, *a, *b, *e, flags;
    exec_fnctn *fn;
    struct cascade *cascade_;

    a = point[0];
    b = point[1];
    e = point[2];
    fn = fn_exec;
    cascade_ = cascade;
    cascade = NULL;

    flags = parse_flags;
    parse_flags = 0b00000000;//default setting

    fn_exec = NULL;	//switch off the /e and /c functions
    point[0] = NULL;
    point[1] = NULL;
    point[2] = NULL;
    begin_ = NULL;

    tmp = parsestr1(d, c);

    parse_flags = flags;

    free_cascade();
    cascade = cascade_;

//    free_strctexec();
    fn_exec = fn;
    point[0] = a;
    point[1] = b;
    point[2] = e;
    if(begin_ == NULL) begin_ = tmp;

    return tmp;
}

char *parsestr2( struct parsestr *ptr, exec_fnctn *fn, char *d, char *c){		//use the pointers in struct
    FILE *fd;
    char *tmp, *arg, flags;
    exec_fnctn *fnct;
    struct cascade *cascade_;

    number = 0;
    point[0] = NULL;
    point[1] = NULL;
    point[2] = NULL;
    begin_ = NULL;

    fd = fdtr;//write to fdtr
    fdtr = ptr->fd;
    arg = arg_;//write to arg_
    arg_ = ptr->arg;
    flags = parse_flags;
    parse_flags = ptr->flags;

    fnct = fn_exec;
    fn_exec = fn;	//the /e and /c functions
    cascade_ = cascade;
    cascade = NULL;

    tmp = parsestr1(d, c);
    if(begin_ == NULL) begin_ = tmp;
    if(tmp /*&& (ptr != NULL)*/){
	ptr->begin = begin_;
	ptr->flags = parse_flags;//new here from 30.10.2023
	ptr->ch = ch_zero;
	ptr->num = number;
	ptr->val = value_;
	ptr->zero = point[0];	//if NULL -> restore_str is not made
//printf("str2:%c:%s\n", ch_zero, point[0]);
//	ptr->end = point[1];
    } else {
	if(point[0]){	//new here
	    *point[0] = ch_zero;
	}
	ptr->zero = NULL;
    }
    ptr->end = point[1];

    free_cascade();
    cascade = cascade_;
    free_strctexec();
    fn_exec = fnct;
    parse_flags = flags;
    arg_ = arg;
    fdtr = fd;

    return tmp;
}

//if ptr == NULL -> just parse without writting to ptr
char *parsestr2_s( struct parsestr *ptr, exec_fnctn *fn, char *d, char *c){		//use the pointers in struct
	char *tmp;
	unsigned char *tmp2, *tmp3, *tmp4,/* *tmp5,*/ ch, ch1;
	unsigned long digi, val, stc, exc, cse;


//	tmp5 = bucks;
	tmp2 = point[0];
	tmp3 = point[1];
	tmp4 = point[2];
	ch = ch_zero;
	digi = number;
	val = value_;
	stc = stack_;
	exc = exec_;
	cse = ca_se_;

	ch1 = stop_;
	stop_ = 0;
	unsigned char *beg = begin_;
//	unsigned char *en = end_;

	struct strctexec *pexec = point_exec;
	point_exec = NULL;//for new beginning

	if(ptr){
		tmp = parsestr2(ptr, fn, d, c);
	} else {
		exec_fnctn *fnexec = fn_exec;
		fn_exec = fn;	//the /e and /c functions
//moved to parsestr()
//		struct cascade *cascade_ = cascade;
//		cascade = NULL;

		tmp = parsestr(d, c);

//		free_cascade();
//		cascade = cascade_;

		free_strctexec();//free this new beginning
		fn_exec = fnexec;
	}

	point_exec = pexec;


//	end_ = en;
	begin_ = beg;
	stop_ = ch1;
	ca_se_ = cse;
	exec_ = exc;
	stack_ = stc;
	value_ = val;
	number = digi;
	ch_zero = ch;
	point[2] = tmp4;
	point[1] = tmp3;
	point[0] = tmp2;
//	bucks = tmp5;

    return tmp;
}

char *restore_str( struct parsestr *ptr){	//note: in parsestring MUST BE-> "/]"
    if(ptr->zero){
	*(ptr->zero) = ptr->ch;
	ptr->zero = NULL;
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
	} else if(tmp2 = parsestr2(&strct, NULL, tmp1, "??/[/N?N/*/]??")){		//??variable??
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
	unsigned long long size = strncpy_(NULL, cmd, 0);	//this is max. size of cmd-string
	char *arg;

	if(arg = malloc(size+1)){
	    strncpy_(arg, cmd, size);
	    write_system(buf_in, size_in, buf, size_1, mode, arg);
	    free(arg);
	}
}

void my_shell(FILE *out, char *tmp){		//tmp- is max. 2048byte  if runned from CGI-script
	unsigned long long size = strncpy_(NULL, tmp, 0);	//this is max. size of arg-string
	char *arg;

	if(arg = malloc(size+1)){
	    strncpy_(arg, tmp, size);
	    my_system(out, arg);
	    free(arg);
	}
}

void shell(char *tmp){		//tmp- is max. 2048byte  if runned from CGI-script
	unsigned long long size = strncpy_(NULL, tmp, 0);	//this is max. size of arg-string
	char *arg;

	if(arg = malloc(size+1)){
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
		    static char bgn[128];
		    snprintf(bgn, 15, "%d", *i);
		    ptr = bgn;
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
	}else if(!strcmp(var_index,"number")){
	    static char num[130];
	    snprintf(num, 128, "%ld", number);//unsigned long
	    ptr = num;
	}else if(!strcmp(var_index,"value")){
	    static char val[130];
	    snprintf(val, 128, "%ld", value_);//unsigned long
	    ptr = val;
	}else if(!strcmp(var_index,"nvalue")){
	    static char nval[128];
	    snprintf(nval, 15, "%d", A1_BTN);
	    ptr = nval;
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

