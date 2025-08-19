#!/bin/sh

echo " *** Save Config ***>> "

cd /tmp;	\
/bin/tar -cvf etc.tar /etc/* > /dev/null 2>&1;	\
/bin/gzip -9 etc.tar > /dev/null 2>&1;	\
/bin/dd if=etc.tar.gz of=/dev/mtdblock4 > /dev/null 2>&1;	\
/bin/rm -f etc.tar.gz  > /dev/null 2>&1
 
