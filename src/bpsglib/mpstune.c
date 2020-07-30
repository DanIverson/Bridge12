// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.

/*  mpstune - tune for MPS */

#include "standard.h"

void pulsesequence()
{
   double freq;
   double tunesw;
   double tupwr;

   freq = getval( "mpsfreq" );
   tunesw = getval( "tunesw" );
   tupwr = getval( "tupwr" );

   mpsTune(freq, tunesw, tupwr);
}
