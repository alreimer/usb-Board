#!/bin/sh

if [ $2 == 1 ]; then
  j=0;
  for i in /dev/$1*
    do
      S=`cat /etc/known_usb | grep -v "#" | cut -d ":" -f 1 | grep $4 `
      if [ $4 = $S ]
	then
          P=`cat /etc/known_usb | grep -v "#" | grep $4 | cut -d ":" -f 2`
	  if [ $j = $P ]
            then 
              M=`cat /etc/known_usb | grep -v "#" | grep $4 | cut -d ":" -f 3`
              C=`cat /etc/known_usb | grep -v "#" | grep $4 | cut -d ":" -f 4`
              mount $i $M
              $C
            else
              mkdir -p /mnt/home/usb/$4/part$j
              mount $i /mnt/home/usb/$4/part$j
              if [ $? != 0 ]; then
                rm -rf /mnt/home/usb/$4/part$j
              fi
				
          fi
        else
          mkdir -p /mnt/home/usb/$4/part$j
          mount $i /mnt/home/usb/$4/part$j
          if [ $? != 0 ]; then
            rm -rf /mnt/home/usb/$4/part$j
          fi
      fi
      j=$(($j+1))
   done
elif [ $2 == 0 ]; then
	S=`cat /etc/known_usb | grep -v "#" | cut -d ":" -f 1 | grep $4 `
	M=`cat /etc/known_usb | grep -v "#" | grep $4 | cut -d ":" -f 3`
	umount $M
	for i in /mnt/home/usb/$4/part*
          do
            umount -f $i 
        done
	rm -rf /mnt/home/usb/$4
fi
exit 0;
