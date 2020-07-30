// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.

/*  mtune - tune for RF */

#include "standard.h"

void pulsesequence()
{
   double tunesw;

   tunesw = getval( "tunesw" );

   mTune(sfrq, tunesw);
}
