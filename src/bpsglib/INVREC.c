// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.

#include <standard.h>

static int phs1[4] = {0,2,1,3};

void pulsesequence()
{
   settable(oph,4,phs1);

   status(A);
   delay(d1);
   status(B);
   pulse(p1,zero); 
   delay(d2); 
   rgpulse(pw,oph,rof1,rof2);
   status(C);
}
