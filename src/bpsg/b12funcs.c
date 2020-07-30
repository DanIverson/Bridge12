/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
#include <stdio.h>
#include "acqparms.h"
#include "b12funcs.h"
#include "vnmrsys.h"

extern void dps_off(void);
extern void dps_on(void);
extern int getPhase(int tab, int index);
extern int getRecPhase(int index);
extern void setPhaseInUse(int index);
extern int lIndex;

extern FILE *psgFile;

static char lastElem[512];

/*----------------------------------------------------------------
|   delay(time)/1
+----------------------------------------------------------------*/
void delay(double time)
{
   if (time <= 0.0)
      return;
   if (psgFile)
   {
      if (lastElem[0] != '\0')
         fprintf(psgFile,"%s\n",lastElem);
      sprintf(lastElem,"DELAY %g",time);
      totaltime += time;
   }
}

void rgpulse(double time, int phase, double rg1, double rg2)
{
   if (time <= 0.0)
      return;
   if (psgFile)
   {
      if (lastElem[0] != '\0')
         fprintf(psgFile,"%s\n",lastElem);
      sprintf(lastElem,"PULSE %g %d %g %g",
              time,getPhase(phase,lIndex),rg1,rg2);
      totaltime += time + rg1 + rg2;
   }
   else
   {
      setPhaseInUse(phase);
   }
}

void pulse(double time, int phase)
{
   if (time <= 0.0)
      return;
   rgpulse(time, phase, rof1, rof2 );
}

void offset(double frequency )
{
   if (psgFile)
   {
      if (lastElem[0] != '\0')
         fprintf(psgFile,"%s\n",lastElem);
      sprintf(lastElem,"OFFSET %g",frequency);
   }
}

void acquire(double datapnts, double dwell )
{
   acqtriggers = 1;
   if (psgFile)
   {
      delay(alfa);
      if (lastElem[0] != '\0')
         fprintf(psgFile,"%s\n",lastElem);
      sprintf(lastElem,"ACQUIRE %d", getRecPhase(lIndex) );
   }
}

void power(double amp )
{
   if (psgFile)
   {
      if (lastElem[0] != '\0')
         fprintf(psgFile,"%s\n",lastElem);
      sprintf(lastElem,"POWER %g",amp);
   }
}
void status(int state)
{
   if (psgFile)
   {
      if (lastElem[0] != '\0')
         fprintf(psgFile,"%s\n",lastElem);
      if (state <= xmsize)
         sprintf(lastElem,"STATUS %d %d",bnc,(xm[state]=='y') ? 1 : 0);
      else
         sprintf(lastElem,"STATUS %d %d",bnc,(xm[xmsize-1]=='y') ? 1 : 0);
   }
}

void microwavedelay(double duration)
{
   if (duration <= 0.0)
      return;
   dps_off();
   if (psgFile)
   {
      if (lastElem[0] != '\0')
         fprintf(psgFile,"%s\n",lastElem);
      fprintf(psgFile,"STATUS %d %d",bnc, 1);
      fprintf(psgFile,"DELAY %g", duration);
      sprintf(lastElem,"STATUS %d %d",bnc, 0);
      totaltime += duration;
   }
   dps_on();
}

void endElems()
{
   if (psgFile)
   {
      if (lastElem[0] != '\0')
         fprintf(psgFile,"%s\n",lastElem);
   }
   lastElem[0] = '\0';
}

void initElems()
{
   lastElem[0] = '\0';
}

void chkLoop(int maxPh, int totalScans, int *loops,  int *loopcnt, int *rem)
{
   int lps;

   lps = totalScans / maxPh;
   if (lps > 1)
   {
      *loops = lps;
      *loopcnt = maxPh;
      *rem = totalScans - (lps * maxPh);
   }
   else
   {
      *loops = 0;
      *loopcnt = 0;
      *rem = totalScans;
   }
}

void mpsTune(double freq, double width, double power)
{
   dps_off();
   if (psgFile)
   {
      fprintf(psgFile,"MPSTUNE %s/mpsData %g %g %g\n",
		      curexp, freq, width, power);
      totaltime = (double) ((int) (np+0.5)) * 0.057;
   }
   dps_on();
}

void mTune(double freq, double width)
{
   dps_off();
   if (psgFile)
   {
      fprintf(psgFile,"STATUS %d 1\n",bnc);
      fprintf(psgFile,"MTUNE %s/mData %g %g\n", curexp, freq, width);
      totaltime = (double) ((int) (np+0.5)) * 0.050;
   }
   dps_on();
}
