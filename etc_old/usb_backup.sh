#!/bin/sh
. /etc/config

FROM=/mnt/home/Share

if [ -n "$BACKUP_PATH" ]; then
    TO=$BACKUP_PATH
else
    TO=/mnt/home/usb_backup
fi

NAME="."

# Modes
# 1 = from usb storage to hdd
# 2 = from hdd to usb storage
MODE=1

echo "starting usb backup"
#2-toggle HD_FULL flash
#3-toggle BACKUP flash
/sbin/nh23X_ioctl 3 > /dev/null

if [ $MODE == 1 ]; then
	for i in $NAME; do
		for j in `cat /proc/mounts | grep /mnt/home/usb/$i | cut -d " " -f 2`; do
			sub=`echo $j | sed s,/mnt/home/usb/,,g`
			[ -d $TO/$sub ] && sub=$sub"_`date +%Y%m%d%H%M%S`"
			mkdir -p $TO/$sub
			cp -ar $j/* $TO/$sub/
			sync; sync
		done
	done
elif [ $MODE == 2 ]; then
	if [ -z "`cat /proc/mounts | grep /mnt/home/usb/`" ]; then
		echo "no usb storage device found"
		exit 0;
	fi
	dir="`cat /proc/mounts | grep /mnt/home/usb/ | tail -n1 | cut -d " " -f 2`"
	cp -ar $FROM $dir
fi

echo "usb backup done"
/sbin/nh23X_ioctl 3 > /dev/null

