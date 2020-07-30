// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.

/*  s2pul - standard two-pulse sequence */

#include "standard.h"

void pulsesequence()
{
   /* equilibrium period */
   status(A);
   delay(d1);

   status(B);
   pulse(p1, zero);
   delay(d2);

   /* --- observe pulse --- */
   status(C);
   rgpulse(pw,oph,rof1,rof2);
}
