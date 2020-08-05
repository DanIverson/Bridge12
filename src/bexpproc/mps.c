/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/*---------------------------------------------------------------------------
|
|
|    fts.h include file is not compatable with -D_FILE_OFFSET_BITS==64
|
+----------------------------------------------------------------------------*/

#ifdef _FILE_OFFSET_BITS
#undef _FILE_OFFSET_BITS
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fts.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <termios.h>
#include <regex.h>
#include <time.h>
#include <signal.h>
#include <ctype.h>

#include "errLogLib.h"
#include "mpsstat.h"

extern int setRtimer(double timsec, double interval);
extern void sigInfoproc();

void statusMPS(void);
void mpsTuneData(int init, char *outfile0, int msec,
		 int np0);

static int mpsFD = -1;
static int statusInterval;
static int statTuneFlag;

int  getStatrateMPS()
{
   return(statusInterval);
}

void statusCheckMPS(int sig)
{
   if (statTuneFlag == 0)
   {
      statusMPS();
      setRtimer( (double) statusInterval * 1e-3, 0.0 );
   }
   else
   {
   DPRINT1(1,"called statusCheckMPS statTune= %d\n",statTuneFlag);
      mpsTuneData(1, NULL, 0, 0);
   }
   sigInfoproc();
}

static void statrateInit()
{
   sigset_t            qmask;
   struct sigaction intserv;
   static int setup = -1;

   /* --- set up signal handler --- */

   if (setup != -1)
      return;
   sigemptyset( &qmask );
   sigaddset( &qmask, SIGALRM );
   intserv.sa_handler = statusCheckMPS;
   intserv.sa_mask = qmask;
   intserv.sa_flags = 0;
   sigaction( SIGALRM, &intserv, NULL );
   setup = 0;
}

void statrateMPS(int msec)
{
   DPRINT1(1, "statrate %d msec\n", msec);
   statusInterval = msec;
   statTuneFlag = 0;
   statrateInit();
   setRtimer( (double) msec * 1e-3, 0.0 );
}

void defaultStatrateMPS()
{
   if (mpsFD < 0)
      return;
   statrateMPS(2000);
}

static void sleepMilliSeconds(int msec)
{
   struct timespec req;

   req.tv_sec = msec / 1000;
   req.tv_nsec = (msec % 1000) * 1000000;
   nanosleep( &req, NULL);
}

static int checkName(const char *name)
{
   char *mpsId = "2341";
   FILE *fd;
   int ret = -1;
   char val[256];
   int ret2 __attribute__((unused));

// fprintf(stderr,"checkName %s\n",name);
   if ( ! access(name, R_OK) )
   {
      fd = fopen(name,"r");
      ret2 =fscanf(fd,"%s", val);
      fclose(fd);
// fprintf(stderr,"check id %s against %s\n",val,mpsId);
      ret = strcmp(mpsId,val);
      if (ret)
         ret = 1;
   }
// fprintf(stderr,"checkName returns %d\n",ret);
   return(ret);
}

static int getTTY(char *ttyPath)
{
   char parent[256];
   char name[256];
   char *argv2[2];
   int found = 0;

   argv2[0] = "/sys/bus/usb/devices";
   argv2[1] = NULL;

   strcpy(parent,argv2[0]);
   strcat(parent,"/usb");
   strcpy(ttyPath,"");
   

   FTS *tree = fts_open(argv2, FTS_NOCHDIR|FTS_LOGICAL, NULL);
   if (!tree)
      return(-1);
   FTSENT *node;
// fprintf(stderr,"argv2[0] is %s errno = %d\n",argv2[0], errno);
   while ((node = fts_read(tree)))
   {
// fprintf(stderr,"path: %s errno = %d\n",node->fts_path, errno);
      if ( ! strcmp(argv2[0],node->fts_path) )
         continue;
//         fprintf(stderr,"test fts_path: %s\n",node->fts_path);
      if ( ((node->fts_info == FTS_D) || (node->fts_info == FTS_SL)) &&
           ! strncmp(node->fts_path,parent,strlen(parent)) )
      {
         int ret;
//          fprintf(stderr,"pre fts_path: %s found= %d\n",node->fts_path,found);
         if ( ! found )
         {
         strcpy(name,node->fts_path);
         strcat(name,"/idVendor");
         if ( (ret = checkName(name)) == 0 )
         {

            strcpy(name,node->fts_path);
            strcat(name,"/tty");
            if ( ! access(name, X_OK) )
            {
               strcpy(ttyPath,name);
               break;
            }
            strcpy(parent,node->fts_path);
//            strcat(parent,"/");
//            strcat(parent,node->fts_name);
            found = 1;
         }
         else if (ret < 0)
         {
         fts_set(tree, node, FTS_SKIP);
         }
         }
         else if (found == 1)
         {
            strcpy(name,node->fts_path);
            strcat(name,"/tty");
            if ( ! access(name, X_OK) )
            {
               found = 2;
            strcpy(parent,node->fts_path);
               strcat(parent,"/tty");
            }
            else
            {
         fts_set(tree, node, FTS_SKIP);
            }
         }
         else if ( (found == 2) &&
            strcmp(node->fts_path,parent) )
         {
               strcpy(ttyPath,"/dev/");
               strcat(ttyPath,node->fts_name);
            break;
         }
      }
/*
      else if ((found == 1) && !strcmp(node->fts_name,"tty") )
      {
         fprintf(stderr,"skip fts_path: %s\n",node->fts_path);
         getTTY(node->fts_path,"",ttyPath,2);
         if ( strlen(ttyPath) )
            break;
      }
      else if ((found == 2) && !strncmp(node->fts_name,"tty",3) )
      {
         fprintf(stderr,"found tty: %s\n",node->fts_name);
         strcpy(ttyPath,node->fts_name);
         break;
      }
  */
      else if ( (node->fts_info == FTS_D) || (node->fts_info == FTS_SL) )
      {
         fts_set(tree, node, FTS_SKIP);
      }
   }
   if (fts_close(tree))
      return(-3);
   return(0);
}

static int openSerialPort(char *dev)
{
   struct termios options;
   char line[256];
   int bytes;

   if ( (mpsFD = open(dev, O_RDWR|O_NOCTTY|O_NONBLOCK|O_CLOEXEC )) >= 0 )
   {
      int reading = 1;
      int charCnt = 0;
      tcgetattr(mpsFD, &options);
      cfsetispeed( &options, B115200);
      cfsetospeed( &options, B115200);
      options.c_cflag &= ~(PARENB | CSTOPB | CSIZE | CRTSCTS);
      options.c_cflag |= (CS8 | CREAD | CLOCAL);
      options.c_lflag &= ~(ICANON | IEXTEN | ECHO | ECHOE | ECHOK | ECHOCTL | ECHOKE | ISIG);
      options.c_oflag &= ~(OPOST | ONLCR);
      options.c_iflag &= ~(IGNBRK | ICRNL | IXON | IXOFF | IXANY);
      tcsetattr(mpsFD, TCSANOW, &options);
      sleepMilliSeconds(100);
      while (reading)
      {
         sleepMilliSeconds(100);
         bytes = 0;
         ioctl(mpsFD, FIONREAD, &bytes);
         if (bytes)
         {
            int i = 0;
            int ch;

            while (i < bytes)
            {
               i++;
               if ( read(mpsFD, &ch, 1) )
               {

               if ( ch != '\r')
                 line[charCnt++] = ch;
               if (ch == '\n')
               {
                  line[charCnt] = '\0';
                  DPRINT1(1,"%s",line);
                  charCnt = 0;
               }
               }
            }
            if (strstr(line,"System") != NULL )
               reading = 0;
         }
      }
      
   }
   return(mpsFD);
}

int openMPS(void)
{
   char ttyPath[256];

   mpsFD = -1;
   if ( getTTY(ttyPath) )
      return(-1);
   if ( ! strlen(ttyPath))
      return(-1);
   if ( openSerialPort(ttyPath) < 0 )
      return(-1);
   return(0);
}

int sendMPS(const char *msg)
{
   ssize_t bytes;
   char err[512];

   if (statTuneFlag)
      DPRINT1(1,"sendMPS %s",msg);
   if (mpsFD < 0)
      return(-1);
   // check if any characters in read buffer
   bytes = 0;
   ioctl(mpsFD, FIONREAD, &bytes);
   if (bytes)
   {
      bytes = read(mpsFD,err, sizeof(err)-1);
   }
   bytes = write(mpsFD, msg, strlen(msg) );
   return(0);
}

void recvMPS(char *msg, size_t len)
{
   ssize_t bytes;
   int loops = 0;

   msg[0] = '\0';
   sleepMilliSeconds(2);
   while (loops < 40)  // maximum I have seen is 11 loops
   {
      bytes = 0;
      ioctl(mpsFD, FIONREAD, &bytes);
      if (bytes)
      {
         sleepMilliSeconds(4);  // wait a little extra so that all bytes are present
         bytes = read(mpsFD,msg,len-1);
         msg[bytes] = '\0';
	 break;
      }
      else
      {
         sleepMilliSeconds(5);
      }
      loops++;
   }
   if (statTuneFlag)
      DPRINT2(1,"recvMPS %s loops= %d\n",msg,loops);
}

static void recvTuneMPS(FILE *fd)
{
   char line[512];
   ssize_t bytes;
   int reading;
   int charCnt = 0;
   int vals = 0;

   DPRINT(1,"recvTuneMPS\n");
   strcpy(line,"");
#ifdef XXX
      sleepMilliSeconds(4); 
      reading = 40;
      // First get the status result
      while (reading > 0)
      {
	 reading--;
         bytes = 0;
         ioctl(mpsFD, FIONREAD, &bytes);
         if (bytes)
         {
            int i = 0;
            int ch;

//   DPRINT1(1,"bytes= %d\n",bytes);
            while (i < bytes)
            {
               i++;
               if ( read(mpsFD, &ch, 1) )
               {

//   DPRINT1(1,"ch= %c\n",ch);
               line[charCnt++] = ch;
               if (ch == '\n')
               {
                  line[charCnt] = '\0';
                  DPRINT1(1,"%s \n",line);
		  reading = 0;
		  break;
               }
               }
            }
         }
	 else
	 {
   DPRINT(1,"wait for doloop response\n");
         sleepMilliSeconds(100); 
	 }
      }
#endif
//      if ( ! strncmp(line,"ERROR",strlen("ERROR")) )
//      {
//         DPRINT(1,"Got an error ***************************************\n");
//	 return;
//      }
//      recvMPS(line, sizeof(line));
//      sleepMilliSeconds(5000); 
//      recvMPS(line, sizeof(line));
      sendMPS("rfsweepdata?\n");
      reading = 1;
      charCnt = 0;
      // Now read the values
      while (reading)
      {
         bytes = 0;
         ioctl(mpsFD, FIONREAD, &bytes);
         if (bytes)
         {
            int i = 0;
            int ch;

//   DPRINT1(1,"bytes= %d\n",bytes);
            while (i < bytes)
            {
               i++;
               if ( read(mpsFD, &ch, 1) )
               {

if ( (ch != ',') && !isdigit(ch) && (ch != '\n') && (ch != '\r') )
   DPRINT1(1,"ch= %c\n",ch);
	       if (ch == ',')
	       {
		   line[charCnt]='\0';
	           fprintf(fd,"%s\n",line);
		   charCnt = 0;
		   vals++;
	       }
               if ( isdigit(ch) )
                 line[charCnt++] = ch;
               if ( (ch == '\n') && (vals != 0) )
               {
                  line[charCnt] = '\0';
//                  DPRINT1(1,"%s\n",line);
	          fprintf(fd,"%s\n",line);
                  charCnt = 0;
		  reading = 0;
               }
	       else if ( (ch == '\n') && (i == bytes) )
	       {
                  DPRINT(1,"Error recvtune\n");
		  reading = 0;
	       }
               }
            }
         }
	 else
	 {
   DPRINT(1,"no bytes sleeping\n");
         sleepMilliSeconds(100); 
	 }
      }
   DPRINT1(1,"values read= %d\n",vals);
#ifdef XXX
   msg[0] = '\0';
   sleepMilliSeconds(2);
   while (loops < 40)  // maximum I have seen is 11 loops
   {
      bytes = 0;
      ioctl(mpsFD, FIONREAD, &bytes);
      if (bytes)
      {
         sleepMilliSeconds(4);  // wait a little extra so that all bytes are present
         bytes = read(mpsFD,msg,len-1);
         msg[bytes] = '\0';
	 break;
      }
      else
      {
         sleepMilliSeconds(5);
      }
      loops++;
   }
#endif
//   DPRINT2(1,"recvMPS %s loops= %d\n",msg,loops);
}

void closeMPS(void)
{
   if (mpsFD >=0)
      close(mpsFD);
   mpsFD = -1;
}

void statusMPS(void)
{
   char msg[512];

   if (sendMPS("rfstatus?\n"))
   {
      statrateMPS(0);
      return;
   }
   recvMPS(msg, sizeof(msg));
   setMpsRfstatus( atoi(msg) );

   if (sendMPS("wgstatus?\n"))
      return;
   recvMPS(msg, sizeof(msg));
   setMpsWgstatus( atoi(msg) );

   if (sendMPS("lockstatus?\n"))
      return;
   recvMPS(msg, sizeof(msg));
   setMpsLockstatus( atoi(msg) );

   if (sendMPS("rxpowermv?\n"))
      return;
   recvMPS(msg, sizeof(msg));
   setMpsRxpowermv( atoi(msg) );

   if (sendMPS("freq?\n"))
      return;
   recvMPS(msg, sizeof(msg));
   setMpsFreq( atoi(msg) );

   if (sendMPS("power?\n"))
      return;
   recvMPS(msg, sizeof(msg));
   setMpsPower( atoi(msg) );

#ifdef XXX
   if (sendMPS("ampstatus?\n"))
      return;
   recvMPS(msg, sizeof(msg));
   setMpsAmpstatus( atoi(msg) );

   if (sendMPS("txpowermv?\n"))
      return;
   recvMPS(msg, sizeof(msg));
   setMpsTxpowermv( atoi(msg) );
#endif
}

void acqMPS(int stage)
{
   static int wgstate;
   static int rfstate;
   char msg[512];

      DPRINT1(1,"acqMPS stage= %d\n", stage);
   if (stage == 0)  // initialization prior to acquisition
   {
      wgstate = rfstate = 0;
      if (sendMPS("rfstatus?\n"))
         return;
      recvMPS(msg, sizeof(msg));
      rfstate = atoi(msg);

      if (sendMPS("wgstatus?\n"))
         return;
      recvMPS(msg, sizeof(msg));
      wgstate = atoi(msg);

      if (sendMPS("wgstatus 1\n"))
         return;
      setMpsWgstatus( 1 );
      sleepMilliSeconds(10);
      if (sendMPS("rfstatus 2\n"))
         return;
      setMpsRfstatus( 2 );
   }
   else if (stage == 1)  // after acquisition reset to previous values
   {
      defaultStatrateMPS();
      sprintf(msg,"wgstatus %d\n",wgstate);
      if (sendMPS(msg))
         return;
      setMpsWgstatus( wgstate );
      if (wgstate == 0)
         rfstate = 0;
      sprintf(msg,"rfstatus %d\n",rfstate);
      sleepMilliSeconds(10);
      if (sendMPS(msg))
         return;
      setMpsRfstatus( rfstate );
   }
   else if (stage == 2)  // start of MPS or RF tuning
   {
      if (sendMPS("rfstatus?\n"))
         return;
      recvMPS(msg, sizeof(msg));
      rfstate = atoi(msg);
      if (rfstate != 1)
      {
      if (sendMPS("rfstatus 1\n"))
         return;
      setMpsRfstatus( 1 );
      }
   }
}

int mpsDataPt(int freq, int ms)
{
   char msg[128];
   int mv;

   sprintf(msg,"freq %d\n",freq);
   if (sendMPS(msg))
      return(-1);
   setMpsFreq( freq );
   sleepMilliSeconds(ms);
   sendMPS("rxpowermv?\n");
   recvMPS(msg, sizeof(msg));
   mv = atoi(msg);
   setMpsRxpowermv( mv );
   sigInfoproc();
   return( mv );
}

void mpsTuneData(int init, char *outfile0, int usec,
		 int np0)
{
   static int np;
   static int fstart;
   static double fincr;
   static FILE *outFD = NULL;
   static char outfile[256];
   static int index = 0;
   char outfile2[256];
   char msg[128];
   int mv;

   if (init == 0)  // initialization
   {
      DPRINT(1,"mpsTuneData init\n");
      np = np0;
//      fstart = fstart0;
//      fincr = fincr0;
      strcpy(outfile,outfile0);
      index = 0;
//      strcpy(outfile2,outfile);
//      strcat(outfile2,".tmp");
      acqMPS(2);
      sleepMilliSeconds(10);
#ifdef XXX
      if (outFD != NULL)
         fclose(outFD);
      if ( (outFD = fopen(outfile2,"w")) == NULL)
      {
	 DPRINT2(1,"Failed to open %s err= %s\n",outfile2, strerror(errno));
         return;
      }
#endif
      statusInterval = ((usec * np)/1000) + 100;  // msec
	 DPRINT3(1,"msec=%d np= %d statusInterval=%d (ms)\n",
			 usec/1000, np, statusInterval);
      statTuneFlag = 1;
      statrateInit();
#ifdef XXX
      sprintf(msg,"freq %d\n",fstart);
      if (sendMPS(msg))
      {
	 DPRINT1(1,"sendMPS %s failed\n",msg);
         return;
      }
      setMpsFreq( fstart );
#endif
      sendMPS("rfsweepdosweep?\n");
      recvMPS(msg, sizeof(msg));
      setRtimer( (double) statusInterval * 1e-3, 0.0 );
   DPRINT(1,"mpsTuneData init done\n");
   }
   else if (init == 1)  // collect tune data
   {
      int freq;

      DPRINT(1,"mpsTuneData data phase\n");
      strcpy(outfile2,outfile);
      strcat(outfile2,".tmp");
      if (outFD != NULL)
         fclose(outFD);
      if ( (outFD = fopen(outfile2,"w")) == NULL)
      {
	 DPRINT2(1,"Failed to open %s err= %s\n",outfile2, strerror(errno));
         return;
      }
      recvTuneMPS(outFD);
         strcpy(outfile2,outfile);
         strcat(outfile2,".tmp");
         fclose(outFD);
	 outFD = NULL;
         chmod(outfile2, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH );
         rename(outfile2,outfile);
#ifdef XXX
      DPRINT1(1,"mpsTuneData index= %d\n",index);
      sendMPS("rxpowermv?\n");
      recvMPS(msg, sizeof(msg));
      mv = atoi(msg);
      setMpsRxpowermv( mv );
      index++;
      freq = fstart + (int) ((double) index * fincr);
      sprintf(msg,"freq %d\n",freq);
      if (sendMPS(msg))
      {
         DPRINT1(1,"sendMPS %s failed\n",msg);
         return;
      }
      setMpsFreq( freq );
      fprintf(outFD,"%d\n", mv);
      if (index == np)
      {
         strcpy(outfile2,outfile);
         strcat(outfile2,".tmp");
         fclose(outFD);
	 outFD = NULL;
         chmod(outfile2, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH );
         rename(outfile2,outfile);
//         if ( ! link(outfile2,outfile))
//            unlink(outfile2);
      }
      else
      {
         setRtimer( (double) statusInterval * 1e-3, 0.0 );
      }
#endif
   }
   else if ((init == 2) && (statTuneFlag == 1) )  // abort case
   {
      DPRINT(1,"mpsTuneData halt case\n");
      // Sending any character aborts the tune sweep
      sendMPS("rxpowermv?\n");
      recvMPS(msg, sizeof(msg));
      setRtimer( 0.0, 0.0 );
      if (outFD != NULL)
      {
         fclose(outFD);
         outFD = NULL;
         strcpy(outfile2,outfile);
         strcat(outfile2,".tmp");
         unlink(outfile2);
      }
   }
}
