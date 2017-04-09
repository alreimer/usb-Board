#!/bin/sh

/bin/mkdir /var/log
/bin/mkdir /var/lock
/bin/mkdir /var/run
/bin/mkdir /var/tmp
/bin/mkdir /var/sm
/bin/mkdir /var/state
/bin/mkdir -p /var/spool/cron/crontabs
/bin/touch /var/sm/sm.bak
/bin/touch /var/xtab
/bin/touch /var/etab
/bin/touch /var/rmtab
/bin/touch /var/log/wtmp
/bin/touch /var/log/lastlog

/bin/chmod 1777 /var/tmp
/bin/chmod 1777 /var/spool/cron/crontabs
#/bin/ln -s /etc/crontab /var/spool/cron/crontabs
