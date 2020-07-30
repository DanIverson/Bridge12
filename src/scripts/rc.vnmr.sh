#!/bin/bash
# 
#
# Copyright (C) 2015  University of Oregon
# 
# You may distribute under the terms of either the GNU General Public
# License or the Apache License, as specified in the LICENSE file.
# 
# For more information, see the LICENSE file.
# 
#
#
# Note that all "echo" commands are in parentheses.  This is done
# because all commands that redirect the output to "/dev/console"
# must be done in a child of the main shell, so that the main shell
# does not open a terminal and get its process group set.  Since
# "echo" is a builtin command, redirection for it will be done
# in the main shell unless the command is run in a subshell.
#
# usage:
#    $0 start          - start procs
#    $0 start procs    - start just the procs
#    $0 stop           - stop procs
#    $0 stop procs     - stop procs

# set -x 

if [[ -d /etc/rc.d ]]; then
   . /etc/rc.d/init.d/functions
fi

start_proc () {
(echo  'starting OpenVnmr services: ')			>/dev/console
# check for acqpresent in /vnmr/acqbin, if there, clear acqqueue
(echo  "     clearing $vnmrsystem/tmp ") >/dev/console
(cd $vnmrsystem/tmp; rm -f exp*)
if [ -f $vnmrsystem/acqbin/acqpresent ]; then
   (echo "     clearing $vnmrsystem/acqqueue ") >/dev/console
   (cd $vnmrsystem/acqqueue; rm -f exp*)
   ($vnmrsystem/acqbin/startStopProcs >/dev/console &)
fi
(echo '.')							>/dev/console
}

stop_proc () {
   ($vnmrsystem/acqbin/startStopProcs >/dev/console &)
}


vnmrsystem=/vnmr
case $# in
    0)
	start_proc
	;;

    *)
	case $1 in
	    'start')
		if [[ $# == 1 || $# > 1 && "$2" == "procs" ]]; then
		    start_proc
		fi
		;;

	    'stop')
		if [[ $# == 1 || $# > 1 && "$2" == "procs" ]]; then
		    stop_proc
		fi
		;;

	esac
	;;
esac
