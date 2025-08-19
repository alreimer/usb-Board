#!/bin/sh
#############################################
# made by alex_raw

DESC="term Daemon"
i=$2
start() {
#	if [ ! -e /run/term.pid ]; then
	echo " *** Start $DESC ***>>"
#	/bin/ln -sf /dev/usb/tts/$i /dev/ttyUSB$i ;
	/etc/init.d/S29httpd stop
#	stty -F /dev/usb/tts/$i -echo cstopb -cread -clocal -ocrnl -opost -onlcr -igncr -icrnl -isig -icanon -ixon  cols 24 rows 16 9600
	stty -F /dev/ttyUSB$i -echo cstopb -cread clocal crtscts -ocrnl -opost -onlcr -igncr -icrnl -isig -icanon -ixon  cols 24 rows 16 9600
	chmod 600 /dev/ttyUSB$i
	echo "$i" > /run/term.pid
	/sbin/terminal /dev/ttyUSB$i > /dev/tty8 &
#	fi
}	
stop() {
#	if [ -e /run/term.pid ]; then
	echo " *** Stop $DESC ***>>"
	/usr/bin/killall terminal
#	rm /run/term.pid
#	/bin/rm -f /dev/ttyUSB$i
#	fi
	/etc/init.d/S29httpd start &
}
restart() {
	killall -HUP terminal
}

case "$1" in
  start)
	start
	;;
  stop)
	stop
	;;
  restart)
	restart
	;;
  *)
	echo "Usage: $0 {start|stop|restart} /dev/ttyUSB*"
	exit 1
esac

exit 1

