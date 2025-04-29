#!/bin/sh
### BEGIN INIT INFO
# Provides:          mydaemon
# Required-Start:    $remote_fs $syslog networking
# Required-Stop:     $remote_fs $syslog
# Default-Start:     2 3 4 5
# Default-Stop:      0 1 6
# Short-Description: Start/stop mydaemon
### END INIT INFO

PATH=/sbin:/bin:/usr/sbin:/usr/bin

NAME=main
DAEMON=/home/root/main
DESC="Server Proyecto Empotrados"

case "$1" in
  start)
    echo "Starting $DESC: $NAME"
    start-stop-daemon --start --quiet --background --exec $DAEMON
    ;;
  stop)
    echo "Stopping $DESC: $NAME"
    start-stop-daemon --stop --quiet --name $NAME
    ;;
  restart)
    echo "Restarting $DESC: $NAME"
    $0 stop
    sleep 1
    $0 start
    ;;
  *)
    echo "Usage: $0 {start|stop|restart}"
    exit 1
    ;;
esac

exit 0
