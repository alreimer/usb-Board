#!/bin/sh
#############################################
# made by alex_raw
# usage: $1 sd.. or hd..
# $2 = 1 -mount, 0 -umount
# $3 = index of port
# $4 = serialnumber

S=`cat /etc/usb/known_usb | grep -v "#" | grep $4`

if [ -z "$S" ]; then
    if [ -e /dev/$12  ]; then
    S="$4:*:/mnt/usb/$4:"
    else
    S="$4:1:/mnt/usb/$4:"
    fi
    echo $S >> /etc/usb/known_usb
fi

for i in $S
    do
	if [ -n "$4" ]; then
	S=`echo $i | cut -d ":" -f 1`
	    if [ "$S" == "$4" ]; then
		P=`echo $i | cut -d ":" -f 2 `
		M=`echo $i | cut -d ":" -f 3 `
		if [ -z "$M" ]; then
		    M=/mnt/usb/$4
		fi
		if [ $2 == 0 ]; then
		    if [ "$P" == "*" ]; then
			for k in "$M/part"*
			do
			    umount -f "$k"
			    [ $? == 0 ] && rm -rf "$k"
			done
			[ ! -e "$M/"* ] && rm -rf "$M"
		    else
#			if [ -e /dev/$1$P ]; then
			umount -f "$M"
			[ $? == 0 ] && rm -rf "$M"
#			else
#			echo "device /dev/$1$P not found"
#			fi
		    fi
		elif [ $2 == 1 ]; then
		    C=`echo $i | cut -d ":" -f 4 `
		    sleep 1
		    if [ "$P" == "*" ]; then
			j=0
			for k in /dev/$1*
			do
			    #not use /dev/sdb, use only /dev/sdb1...
			    if [ $j != 0 ]; then
				mkdir -p "$M/part$j"
				mount $k "$M/part$j"
				[ $? != 0 ] && rm -rf "$M/part$j"
			    fi
			j=$(($j+1))
			done
			$C
		    else
			if [ -e /dev/$1$P ]; then
			mkdir -p "$M"
			mount /dev/$1$P "$M"
			[ $? != 0 ] && rm -rf "$M"
			$C
			else
			echo "device /dev/$1$P not found"
			fi
		    fi
		fi
	    fi
	fi
done
