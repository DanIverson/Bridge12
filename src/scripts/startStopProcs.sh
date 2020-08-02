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
#  startStopProcs - This script is run as root
#                    On newer systems with systemd in use, unless the
#                    procs are started by the systemd system, they will
#                    be terminated when the user that starts them logs out.
#

# set -x

vnmrsystem=/vnmr
npids=$(pgrep Expproc)
if [[ -z $npids ]]; then
   echo "Starting Acquisition communications."
   if [[ -f /lib/systemd/system/vnmr.service ]]; then
      systemctl start vnmr.service
   else
      ${vnmrsystem}/acqbin/ovjProcs &
   fi
else
   echo "Stopping Acquisition communications."
   ${vnmrsystem}/acqbin/ovjProcs
fi
