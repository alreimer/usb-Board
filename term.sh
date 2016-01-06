#/bin/sh

stty -F /dev/ttyUSB0 -echo cstopb -cread -clocal -ocrnl -opost -onlcr -igncr -icrnl -isig -icanon -ixon  cols 24 rows 16 9600
