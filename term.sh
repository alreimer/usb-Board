#/bin/sh

stty -F /dev/ttyUSB0 -echo cstopb -cread clocal crtscts -ocrnl -opost -onlcr -igncr -icrnl -isig -icanon -ixon  cols 24 rows 16 9600
#stty -F /dev/ttyUSB0 -echo cstopb -cread -clocal -ocrnl -opost -onlcr -igncr -icrnl -isig -icanon -ixon  cols 24 rows 16 9600
#cs8 -means 8bit charsize