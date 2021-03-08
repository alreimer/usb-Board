1. Commets:
/*comments*/
//or comments

1.2 put out to display information:
> "AL" 0 1
> "AT" 9 9 114 58 1 0 "CNetwork" 0
or:
> "AT" 9 9 114 58 1 0 "CNet" $supername:19 0	//putout 19 chars
or
> "AT" 9 9 114 58 1 0 "CNet" $supername:19:"/L/L" 0	//parse to pattern and output 19 chars


1.3 registring CGI:
< "A" 1
    boot_page "hd_mounts.term";	//this is body of CGI
end

//multiple difinition from 1 till 3
< "A" 1 3
    write_par "file_n:??_@list#0??";
    print [ 12   > "DL" > "AL" 0 1 "IMG_name: " $_@list#0 ];
//    boot_cgi [ "getCard" ];
end

< "show" 0
	print [ 12   > "DL" > "AL" 0 1
    > "AT" 2 1 201 29 1 0 "CLogin:|" $supername:19 0
    > "AT" 2 33 201 61 2 0 "CPassword:|" $sup_passwd:19 0
if "_etc_save:1" > "AT" 205 65 237 93 11 0 "CS" 0 fi
    > "AT" 205 1 237 29 13 0 "C<" 0 ];
end

1.4 run CGI:
cgi: "show" 0


1.5 Registration of parameters:
area:40000:buffer
par:2:dhcp_client:DHCP_ENABLE:[01]

1.5.1 read "par:" regs from Config file: /etc/config if (for example) DHCP_ENABLE is given.
If it's not given then it looks like this:
par:2:dhcp_client::[01]

readcfg

1.5.2 link in "par:" registration:
par:4:sleep_mode:?dhcp_client:0|120|180|240|241|242|244|246|248|250



2.1 write to parameter like _buf:
wr_par: _buf:/dev/sda SLEEP_MODE\n

2.2 write to parameter, like buffer, using shell:
wr_shell: :buffer:df -h

2.3 include file into this file:
include: net-keys.incterm

2.4 using if or ifnot in file:
ifnot "_buf://dev//sd/<1abcdefg/>/tSLEEP_MODE"
wr_par: _buf:/dev/sda SLEEP_MODE\n
fi

if "dhcp_client:0/"
    > "ZC" 51 13 "Enable" 0
< "A" 1
    write_par "_%dhcp_client:1";
    fill_cfg "dhcp_client";
    savecfg;
	system "/etc/init.d/rc 3\ 2 stop; /etc/init.d/rc 2\ 3 start &";

    boot_page "net-dhcp.term";
end
fi


3.Tables entries:
(if:'s can be TAB_LEN times)
first "if:" have double function: 1- parse the pattern, 2- skip to next while-area if not matches
each if begins at the end of if-bevor.

table: list
#-> parse_area: buffer
while:/[/*\n/]\n
if:/[/*/] 
if:/*/[HWaddr/*/]\n
if:/*/[inet/*/]  
if:/*/[Bcast/*/]  
if:/*/[Mask/*/]\n
if:/t/[/*/]  
if:/[MTU/*/]  
if:/*/[RX bytes/*/]  
if:/*/[TX bytes/*/]\n
#end_table


in Table Body can be:
#-> get_folder: /path/
#-> get_files: /path/to/files

or: <- !!!!not in tab!!!!

#-> parse_file: /etc/fstab
while:/[/*/]\n
if:

or:
#-> parse_file: /etc/??file_par??
while:/[/*/]\n
if:

or:
#-> parse_area: _#AREA-1
while:/[/*/]\n
if:

or:
#-> parse_area: _#AREA-1
while:/[/*/]\n
check: 			<- check in while-area(here it is string) and if not matched - skip to next while-area
if:			<- first "if:" have double function: 1- parse the pattern, 2- skip to next while-area if not matches
if:
if:

or:
#-> parse_area: _#AREA-1
while:/[/*/]\n
mixed			<- this means that all if's started at the beginning of while-area (string) and not from end if-bevor
if:
if:
if:


or:
#-> begin: _par

or:
#-> check_folders
/mnt/hd
/mnt/hd1
#-> end

or for NOT-existing:
#-> check_Nfolders
/mnt/hd
/mnt/hd1
#-> end

or for existing files:
#-> check_files
/mnt/README
/mnt/hd1
#-> end

4. CGI bodies:


5. Parameters:

??__variable?? - show environment variable
??_%variable?? - show new_variable						!=(":%@)
??_&variable?? - show fresh_variable
??_@variable?? - show variable from rnd table (size = strlen(ofVariable) + 1)	!=(":+#%@)
??_@_variable?? - show begin-variable from table
??_?file|expression?? -> in file this expression

??_referrer?? == ??_referrer1?? //last-relative to actual page ".term file"
??_back?? == ??_back1??		to last page (non relative)
??_etc_save?? (size=2)
??_value?? (size=0)
??_version?? (size = 256)
??_buf?? (size = 65537) //whole buffer
??_buffer?? if(buf_size >= 65537 /*|| buf_size == 0*/) size = 65537; else size = buf_size; //i think, it is right here!!
