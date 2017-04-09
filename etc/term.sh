#!/bin/sh
DESC="term Daemon"
i=$2

start() {
	echo " *** Start $DESC ***>>"
	/bin/ln -sf /dev/usb/tts/$i /dev/ttyUSB$i ; /etc/init.d/S29httpd stop
	stty -F /dev/usb/tts/$i -echo cstopb -cread -clocal -ocrnl -opost -onlcr -igncr -icrnl -isig -icanon -ixon  cols 24 rows 16 9600
	touch /tmp/term
	/sbin/terminal /dev/ttyUSB$i &
}	
stop() {
	echo " *** Stop $DESC ***>>"
	killall terminal
	rm /tmp/term
	/bin/rm -f /dev/ttyUSB$i ; /etc/init.d/S29httpd start &
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

exit $?

