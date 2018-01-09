#! /bin/sh
### BEGIN INIT INFO
# Provides:          opl3d
# Required-Start:    $local_fs midi
# Required-Stop:     $local_fs midi
# Default-Start:
# Default-Stop:
# Short-Description: OPL3_FPGA Midi USB->OPL3 relay daemon
### END INIT INFO

PATH=/sbin:/usr/sbin:/bin:/usr/bin
DESC="OPL3_FPGA Midi USB->OPL3 relay daemon"
NAME=opl3d
DAEMON=/usr/bin/$NAME
DAEMON_ARGS=""
PIDFILE=/var/run/$NAME.pid
SCRIPTNAME=/etc/init.d/$NAME

[ -x "$DAEMON" ] || exit 0

do_start()
{
	# Return
	#   0 if daemon has been started
	#   1 if daemon could not be started
	start-stop-daemon --start --quiet --background --make-pidfile --pidfile $PIDFILE \
		--exec $DAEMON --nicelevel -20 -- $DAEMON_ARGS \
		|| return 1
}

do_stop()
{
	# Return
	#   0 if daemon has been stopped
	#   1 if daemon was already stopped
	#   2 if daemon could not be stopped
	#   other if a failure occurred
	start-stop-daemon --stop --quiet --retry=TERM/30/KILL/5 --pidfile $PIDFILE --name $NAME
	RETVAL="$?"
	[ "$RETVAL" = 2 ] && return 2

	rm -f $PIDFILE
	return "$RETVAL"
}

case "$1" in
  start)
	echo -n "Starting $DESC" "$NAME... "
	do_start
	case "$?" in
		0) echo "done." ;;
		1) echo "failed." ;;
	esac
	;;
  stop)
	echo -n "Stopping $DESC" "$NAME... "
	do_stop
	case "$?" in
		0) echo "done." ;;
		1) echo "not running." ;;
		2) echo "failed." ;;
	esac
	;;
  restart)
	echo -n "Restarting $DESC" "$NAME... "
	do_stop
	case "$?" in
	  0|1)
		do_start
		case "$?" in
			0) echo "done." ;;
			1) echo "failed." ;;
		esac
		;;
	  *)
		# Failed to stop
		echo "failed."
		;;
	esac
	;;
  *)
	echo "Usage: $SCRIPTNAME {start|stop|restart}" >&2
	exit 3
	;;
esac

:
