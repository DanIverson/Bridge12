/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#ifdef XXX
#include <pwd.h>
#include <netinet/in.h>

#include <errno.h>

#include "errLogLib.h"
#include "mfileObj.h"
#include "shrMLib.h"
#include "chanLib.h"
#include "hostAcqStructs.h"
#include "hostMsgChannels.h"
#include "expQfuncs.h"
#include "dspfuncs.h"
#endif  // XXX

#include "data.h"
#include "shrexpinfo.h"
#include "shrstatinfo.h"
#include "shrMLib.h"
#include "msgQLib.h"
#include "procQfuncs.h"
#include "expDoneCodes.h"
#include "errorcodes.h"
#include "expQfuncs.h"

extern void diagMessage(const char *format, ...);
extern void expStatusRelease();

#define IBUF_SIZE       (8*XFR_SIZE)

#define S_OLD_COMPLEX   0x40
#define S_OLD_ACQPAR    0x80

#define SAVE_DATA	0
#define DELETE_DATA	1
#define DATA_ERROR	-1

SHR_EXP_INFO expInfo = NULL;   /* start address of shared Exp. Info Structure */

static SHR_MEM_ID  ShrExpInfo = NULL;  /* Shared Memory Object */

/* Vnmr Data File Header & Block Header */
static struct datafilehead fidfileheader;
static struct datablockhead fidblockheader;

/* dummy struct for now */
#ifdef XXX
typedef struct  {
			long np;
			long ct;
			long bs;
			long elemid;
			long v1;
			long v2;
			long v4;
			long v5[10];
		  } lc;
#endif

#ifdef LINUX
struct recvProcSwapbyte
{
   short s1;
   short s2;
   short s3;
   short s4;
   int  l1;
   int  l2;
   int  l3;
   int  l4;
   int  l5;
};

typedef union
{
   struct datablockhead *in1;
   struct recvProcSwapbyte *out;
} recvProcHeaderUnion;

typedef union
{
   float *fval;
   int   *ival;
} floatInt;                                                                               
#endif

extern void diagMessage(const char *format, ...);
/* char tmp[IBUF_SIZE+1]; */

static MSG_Q_ID pProcMsgQ = NULL;
static MSG_Q_ID pExpMsgQ = NULL;
static MFILE_ID ifile = NULL;
static char expInfoFile[512] = { '\0' };

static int bbytes = 0;

// void UpdateStatus(FID_STAT_BLOCK *);
// void rmAcqiFiles(SHR_EXP_INFO);
int mapIn(char *filename);
int mapOut(char *str);

void sleepMilliSeconds(int msecs)
{
   struct timespec req;
   req.tv_sec = msecs / 1000;
   req.tv_nsec = (msecs % 1000) * 1000000;
   nanosleep( &req, NULL);
}

void resetState()
{
  /* close msgQs if open */
  if (pProcMsgQ != NULL)
  {
	closeMsgQ(pProcMsgQ);
	pProcMsgQ = NULL;
  }
  if (pExpMsgQ != NULL)
  {
	closeMsgQ(pExpMsgQ);
	pExpMsgQ = NULL;
  }
  /* close mmap data file if opened. */
  if (ifile != NULL)
  {
    mClose(ifile); 
    ifile = NULL;
  }

  initActiveExpQ(0);
//  diagMessage("resetState: activeExpQdelete= %s\n",expInfoFile);
  activeExpQdelete(expInfoFile);
  activeExpQRelease();
  /* map out shared expingo info file  */
  if ( (int) strlen(expInfoFile) > 1)
  {
     mapOut(expInfoFile);
     expInfoFile[0] = '\0';
  }
  expStatusRelease();
}

int mapIn(char *filename)
{
//    DPRINT1(1,"mapIn: mapin  '%s'\n",filename);

    ShrExpInfo = shrmCreate(filename,SHR_EXP_INFO_RW_KEY,(unsigned long)sizeof(SHR_EXP_STRUCT)); 
    if (ShrExpInfo == NULL)
    {
//       errLogSysRet(ErrLogOp,debugInfo,"mapIn: shrmCreate() failed:");
       return(-1);
    }

    if (ShrExpInfo->shrmem->byteLen < sizeof(SHR_EXP_STRUCT))
    {
	/* hey, this file is not a shared Exp Info file */
       shrmRelease(ShrExpInfo);		/* release shared Memory */
       unlink(filename);	/* remove filename that shared Mem created */
       ShrExpInfo = NULL;
       expInfo = NULL;
       return(-1);
    }

#ifdef DEBUG
    if (DebugLevel >= 2)
      shrmShow(ShrExpInfo);
#endif

    expInfo = (SHR_EXP_INFO) shrmAddr(ShrExpInfo);

    return(0);
}

int mapOut(char *str)
{
    if (ShrExpInfo != NULL)
    {
       shrmRelease(ShrExpInfo);
       ShrExpInfo = NULL;
       expInfo = NULL;
    }
    return(0);
}

int InitialFileHeaders()
{

    /* datasize = expptr->DataPtSiz;  data pt bytes
     * fidsize  = expptr->FidSiz;     fid in bytes
     * np = expptr->N_Pts;
     * dpflag = datasize;
     */

    /* decfactor = *( (int *)getDSPinfo(DSP_DECFACTOR) ); */


   fidfileheader.nblocks   = expInfo->ArrayDim; /* n fids*/
   fidfileheader.ntraces   = expInfo->NumFids; /* NF */;
   fidfileheader.np  	= expInfo->NumDataPts;
   fidfileheader.ebytes    = expInfo->DataPtSize;     /* data pt bytes */
   fidfileheader.tbytes    = expInfo->DataPtSize * expInfo->NumDataPts; /* trace in bytes */
                                /*blk in byte*/
   bbytes = (fidfileheader.tbytes * fidfileheader.ntraces)
                                        + sizeof(fidblockheader);
   fidfileheader.bbytes = bbytes;
 
   fidfileheader.nbheaders = 1;
   fidfileheader.status    = S_DATA | S_OLD_COMPLEX;/* init status FID */
   fidfileheader.vers_id   = 0;
#ifdef LINUX
   fidfileheader.nblocks   = htonl(fidfileheader.nblocks);
   fidfileheader.ntraces   = htonl(fidfileheader.ntraces);
   fidfileheader.np        = htonl(fidfileheader.np);
   fidfileheader.ebytes    = htonl(fidfileheader.ebytes);
   fidfileheader.tbytes    = htonl(fidfileheader.tbytes);
   fidfileheader.bbytes    = htonl(fidfileheader.bbytes);
   fidfileheader.nbheaders = htonl(fidfileheader.nbheaders);
   fidfileheader.vers_id   = htons(fidfileheader.vers_id);
   fidfileheader.status    = htons(fidfileheader.status);
#endif

        /* --------------------  FID Header  ---------------------------- */
   fidblockheader.scale = (short) 0;
   fidblockheader.status = S_DATA | S_OLD_COMPLEX;/* init status to fid*/
   fidblockheader.index = (short) 0;
   fidblockheader.mode = (short) 0;
   fidblockheader.ctcount = (long) 0;
   fidblockheader.lpval = (float) 0.0;
   fidblockheader.rpval = (float) 0.0;
   fidblockheader.lvl = (float) 0.0;
   fidblockheader.tlt = (float) 0.0;
   if ( expInfo->DataPtSize == 4)  /* dp='y' (4) */
   {                                
      fidfileheader.status |= S_32;
      fidblockheader.status |= S_32;
   }
#ifdef LINUX
   {
      recvProcHeaderUnion hU;
                                                                                
      fidfileheader.status    = htons(fidfileheader.status);
      hU.in1 = &fidblockheader;
      hU.out->s1 = htons(hU.out->s1);
      hU.out->s2 = htons(hU.out->s2);
      hU.out->s3 = htons(hU.out->s3);
      hU.out->s4 = htons(hU.out->s4);
      hU.out->l1 = htonl(hU.out->l1);
      hU.out->l2 = htonl(hU.out->l2);
      hU.out->l3 = htonl(hU.out->l3);
      hU.out->l4 = htonl(hU.out->l4);
      hU.out->l5 = htonl(hU.out->l5);
   }
#endif
   return(0);
}

static void initComm(char *info)
{
   int stat;

   initProcQs(0);
   pProcMsgQ = openMsgQ("Procproc");
   strcpy(expInfoFile,info);
   stat = mapIn(info);
   diagMessage("saveData: mapIn stat= %d\n",stat);
}

static void initSave(char *info)
{
   char fidpath[1024];

   initComm(info);
   sprintf(fidpath,"%s/fid",expInfo->DataFile);
   diagMessage("saveData: fidpath= %s size= %d\n",fidpath,expInfo->DataSize);
   ifile = mOpen(fidpath,expInfo->DataSize,O_RDWR | O_CREAT | O_TRUNC);
   InitialFileHeaders(); /* set the default values for the file & block headers */

       /* write fileheader to datafile (via mmap) */
   diagMessage("saveData: memcpy file header bytes= %d\n",sizeof(fidfileheader));
   memcpy((void*) ifile->offsetAddr,
		(void*) &fidfileheader,sizeof(fidfileheader));
   ifile->offsetAddr += sizeof(fidfileheader);  /* move my file pointers */
   ifile->newByteLen += sizeof(fidfileheader);
}

void endComm()
{
   int stat;
   char expCmpCmd[512];

   stat = sendMsgQ(pProcMsgQ,"chkQ",strlen("chkQ"),
			     MSGQ_NORMAL,WAIT_FOREVER);
   diagMessage("endComm: chkQ stat= %d\n",stat);
   if (pExpMsgQ == NULL)
      pExpMsgQ = openMsgQ("Expproc");
   sprintf(expCmpCmd,"expdone %s",ShrExpInfo->MemPath);
   stat = sendMsgQ(pExpMsgQ,expCmpCmd,strlen(expCmpCmd),MSGQ_NORMAL,
                                WAIT_FOREVER);
   diagMessage("endComm: expdone stat= %d\n",stat);
   setStatExpName("");
   resetState();
}

static void saveFid(MFILE_ID datafile, int ct, int elem, int *real, int *imag)
{
   char *fidblkhdrSpot;
   int *data;
   struct datablockhead *tmp;
   int i;
   int itmp;

   diagMessage("saveData: dataheadsize= %d bbytes= %d elem= %d\n",
                  sizeof(struct datafilehead), bbytes, elem);
   mFidSeek(datafile, elem, sizeof(struct datafilehead), bbytes );
   fidblkhdrSpot = datafile->offsetAddr;	/* save block header position */
   memcpy((void*)fidblkhdrSpot,(void*)&fidblockheader,
						sizeof(fidblockheader));

   data = (int *) (datafile->offsetAddr + sizeof(fidblockheader));
   diagMessage("saveData: save numPoints= %d\n", expInfo->NumDataPts);
   for (i=0; i< expInfo->NumDataPts/2; i++)
   {
      itmp = *real++;
      *data++ = htonl(itmp);
      itmp = *imag++;
      *data++ = htonl(itmp);
   }
   tmp = (struct datablockhead *) fidblkhdrSpot;
   tmp->ctcount=htonl(ct);
   tmp->index=htons(elem);
}

void saveData( int ct, int ix, int arraydim,
               int *real, int *imag, char *infoPath)
{
   int stat;
   diagMessage("saveData: ct= %d ix= %d arraydim= %d\n",
                         ct,ix,arraydim);
   if (pProcMsgQ == NULL)
      initSave(infoPath);
   saveFid(ifile, ct, ix, real, imag);
   setStatElem(ix);
   if (ix == arraydim)
   {
      procQadd(WEXP_WAIT, ShrExpInfo->MemPath, ix,
             ct, EXP_COMPLETE, 0);
      setStatGoFlag(-1);     /* No Go,Su,etc acquiring */
   }
   else
   {
      procQadd(WFID, ShrExpInfo->MemPath, ix,
             ct, EXP_FID_CMPLT, 0);

      stat = sendMsgQ(pProcMsgQ,"chkQ",strlen("chkQ"),
			     MSGQ_NORMAL,WAIT_FOREVER);
      diagMessage("saveData: chkQ stat= %d\n",stat);
   }
}

void sendError( int ct, int ix, int doneCode, int errorCode,
                char *infoPath)
{
   int stat;
   diagMessage("sendError: ct= %d ix= %d doneCode= %d errorCode= %d infoPath= %s\n",
                         ct,ix,doneCode,errorCode,infoPath);
   if (pProcMsgQ == NULL)
      initComm(infoPath);
   procQadd(WERR, ShrExpInfo->MemPath, ix,
             ct, doneCode, errorCode);
   setStatGoFlag(-1);     /* No Go,Su,etc acquiring */

   if ((doneCode != HARD_ERROR) && (doneCode != EXP_ABORTED) )
   {
      stat = sendMsgQ(pProcMsgQ,"chkQ",strlen("chkQ"),
			     MSGQ_NORMAL,WAIT_FOREVER);
      diagMessage("saveData: chkQ stat= %d\n",stat);
   }
}

void initError( char *infoPath )
{
   sendError( 0, 0, HARD_ERROR, HDWAREERROR+OB_IDERROR, infoPath);
}
void abortExp( char *infoPath, char *code, int ct, int ix )
{
   char path[256];
   strcpy(path,code);
   strcat(path,"_halt");
   if ( ! access(path, F_OK) )
   {
      diagMessage("Exp halted\n");
      if (pProcMsgQ == NULL)
         initSave(infoPath);
      procQadd(WEXP_WAIT, ShrExpInfo->MemPath, ix,
             ct, EXP_COMPLETE, 0);
      setStatGoFlag(-1);     /* No Go,Su,etc acquiring */
      unlink(path);
   }
   else
   {
      sendError( ct, ix, EXP_ABORTED, ABORTERROR, infoPath);
      diagMessage("Exp aborted\n");
   }
}
void dataError( char *infoPath, int ct, int ix )
{
   sendError( ct, ix, HARD_ERROR, HDWAREERROR+DDR_CMPLTFAIL_ERR, infoPath);
}

void requestMpsData(double freq, double width, double power, double del,
		    char *dataPath, char *infoPath )
{
   int stat __attribute__((unused));
   char expCmpCmd[512];

   if (pProcMsgQ == NULL)
      initSave(infoPath);
   if (pExpMsgQ == NULL)
      pExpMsgQ = openMsgQ("Expproc");
   sprintf(expCmpCmd,"mpsData %g %g %g %g %s",
		    freq, width, power, del, dataPath);
   stat = sendMsgQ(pExpMsgQ,expCmpCmd,strlen(expCmpCmd),MSGQ_NORMAL,
                                WAIT_FOREVER);
}

void saveBsData( int *real, int *imag, char *infoPath, int ct)
{
   int stat;

   diagMessage("saveBsData\n");
   if (pProcMsgQ == NULL)
      initSave(infoPath);
   saveFid(ifile, 1, 1, real, imag);
   stat = procQadd(WBS, ShrExpInfo->MemPath, 1,
             ct, BS_CMPLT, ct);
   if (stat != SKIPPED)
   {
   stat = sendMsgQ(pProcMsgQ,"chkQ",strlen("chkQ"),
			     MSGQ_NORMAL,WAIT_FOREVER);
   diagMessage("saveBsData: chkQ stat= %d\n",stat);
   }
}

void endMpsData()
{
   procQadd(WEXP_WAIT, ShrExpInfo->MemPath, 1,
             1, EXP_COMPLETE, 0);
   setStatGoFlag(-1);     /* No Go,Su,etc acquiring */
}

void sendExpMsg(char *cmd)
{
   int stat __attribute__((unused));
   if (pExpMsgQ == NULL)
      pExpMsgQ = openMsgQ("Expproc");
   stat = sendMsgQ(pExpMsgQ,cmd,strlen(cmd),MSGQ_NORMAL, WAIT_FOREVER);
}
