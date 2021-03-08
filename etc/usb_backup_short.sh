#!/bin/sh
echo "starting usb sync"
#2-toggle HD_FULL flash
#3-toggle BACKUP flash
/sbin/nh23X_ioctl 2 > /dev/null
sync;sync;sync
echo "usb sync done"
/sbin/nh23X_ioctl 2 > /dev/null

