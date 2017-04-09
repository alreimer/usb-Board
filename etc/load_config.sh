#!/bin/sh

echo " *** Load Config ***>> "

/bin/cp -r /etc /var/
/bin/mount -t tmpfs tmpfs /etc
/bin/cp -arf /var/etc/* /etc/

/bin/dd if=/dev/mtdblock4 of=/var/etc.tgz >/dev/null 2>&1
#cd /var;/bin/gunzip etc.tgz;tar -xf etc.tar -C / > /dev/null 2>&1 || echo "   *** Restore Config failed ***>>"
cd /var;tar -xzf etc.tgz -C / > /dev/null 2>&1 || echo "   *** Restore Config failed ***>>"
/bin/mv -f /var/etc/version /etc/
/bin/rm -r /var/etc*
