#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include "inc/spinapi.h"

extern void diagMessage(const char *format, ...);
extern void sendExpMsg(char *cmd);
extern void sleepMilliSeconds(int msecs);
extern void saveData( int ct, int ix, int arraydim,
                      int *real, int *imag, char *info);
extern void saveBsData( int *real, int *imag, char *infoPath, int ct);

static int mtunePts = 0;
static double mtuneStart;
static double mtuneIncr;
static int mtuneLooping;

void mtuneHandler(int sig)
{
   static int index = 0;
   char *expCmpCmd = "tuneData 1";
   int stat __attribute__((unused));

    index++;
    if (index == mtunePts)
    {
       index = 0;
       mtuneLooping = 0;
    }
    pb_start_programming(FREQ_REGS);
    pb_set_freq(mtuneStart + mtuneIncr*index);
    pb_stop_programming();
    pb_start();
    if ( index )
    {
       sleepMilliSeconds(10);
       sendExpMsg(expCmpCmd);
    }
}

static void setupMtuneHandler()
{
    sigset_t            qmask;
    struct sigaction intserv;
    static int setup = -1;

    /* --- set up signal handler --- */

    if (setup != -1)
       return;
    sigemptyset( &qmask );
    sigaddset( &qmask, SIGUSR2 );
    intserv.sa_handler = mtuneHandler;
    intserv.sa_mask = qmask;
    intserv.sa_flags = 0;

    sigaction( SIGUSR2, &intserv, NULL );

    sigemptyset( &qmask );
    sigaddset( &qmask, SIGUSR2 );
    sigprocmask( SIG_UNBLOCK, &qmask, NULL );
    setup = 0;
}

static void getBsTune(char *path, int pts, int *buf)
{
   FILE *fd;
   int *ptr;
   int index;
   char val[128];

   diagMessage("getBSDate from %s\n",path);
   if ( (fd = fopen(path,"r")) )
   {
      ptr = buf;
      diagMessage("getBSDate read %d pts\n",pts);
      for (index=0; index<pts; index++)
      {
            if (fscanf(fd,"%s\n", val))
            {
      diagMessage("getBSDate %d: %s\n",index, val);
               *ptr++ = atoi(val);
	    }
      }
      fclose(fd);
   }
//   unlink(path);
}

void mtune(char *dataPath, double freq, double width,
	  int pts, char *codePath,
	  int *real, int *imag, char *infoPath)
{
   char expCmpCmd[512];
   int ct = 1;

   mtuneStart = freq - (width/2.0);
   mtuneIncr = width / (double) (pts - 1);
   mtunePts = pts;
   diagMessage("mtune start= %g incr= %g pts= %d\n", mtuneStart, mtuneIncr, pts);
   sprintf(expCmpCmd,"tuneData 0 %s", dataPath);
   setupMtuneHandler(pts);
   while ( 1 == 1)
   {
      mtuneLooping = 1;
      sendExpMsg(expCmpCmd);
      diagMessage("mtune start loop\n");
      while (mtuneLooping)
      {
         if ( access(codePath, F_OK) )
         {
            diagMessage("mtune aborted\n");
            return;
         }
         sleepMilliSeconds(50);
      }
      diagMessage("mtune get BS data\n");
      getBsTune(dataPath, pts, real);
      diagMessage("mtune save BS data\n");
      saveBsData( real, imag, infoPath, ct);
      ct = 2;
//            return;
   }
}
