/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <sys/stat.h>
#include <string.h>
// #include <errno.h>
#include "variables.h"
#include "data.h"
#include "group.h"
#include "abort.h"
#include "acqparms.h"
#include "pvars.h"
#include "cps.h"

#include "shrexpinfo.h"

#define PRTLEVEL 1
#ifdef  DEBUG
#define DPRINT(level, str) \
        if (bgflag >= level) fprintf(stderr,str)
#define DPRINT1(level, str, arg1) \
        if (bgflag >= level) fprintf(stderr,str,arg1)
#define DPRINT2(level, str, arg1, arg2) \
        if (bgflag >= level) fprintf(stderr,str,arg1,arg2)
#define DPRINT3(level, str, arg1, arg2, arg3) \
        if (bgflag >= level) fprintf(stderr,str,arg1,arg2,arg3)
#define DPRINT4(level, str, arg1, arg2, arg3, arg4) \
        if (bgflag >= level) fprintf(stderr,str,arg1,arg2,arg3,arg4)
#else
#define DPRINT(level, str)
#define DPRINT1(level, str, arg2)
#define DPRINT2(level, str, arg1, arg2)
#define DPRINT3(level, str, arg1, arg2, arg3)
#define DPRINT4(level, str, arg1, arg2, arg3, arg4)
#endif

extern int option_check(char *option);
extern int clr_at_blksize_mode;
extern int bgflag;
extern int      acqiflag;	/* for interactive acq. or not? */

static char infopath[256];


SHR_EXP_STRUCT ExpInfo;

void printShrExpInfo(SHR_EXP_INFO expInfo)
{
   fprintf(stderr,"ExpDuration: %lf\n", expInfo->ExpDur);
   fprintf(stderr,"DataSize: %lld\n", expInfo->DataSize);
   fprintf(stderr,"ArrayDim: %d\n", expInfo->ArrayDim);
   fprintf(stderr,"FidSize: %d\n", expInfo->FidSize);
   fprintf(stderr,"DataPtSize: %d\n", expInfo->DataPtSize);
   fprintf(stderr,"NumAcodes: %d\n", expInfo->NumAcodes);
   fprintf(stderr,"NumTables: %d\n", expInfo->NumTables);
   fprintf(stderr,"NumFids: %d\n", expInfo->NumFids);
   fprintf(stderr,"NumTrans: %d\n", expInfo->NumTrans);
   fprintf(stderr,"NumInBS: %d\n", expInfo->NumInBS);
   fprintf(stderr,"NumDataPts: %d\n", expInfo->NumDataPts);
   fprintf(stderr,"ExpNum: %d\n", expInfo->ExpNum);
   fprintf(stderr,"ExpFlags: %d\n", expInfo->ExpFlags);
   fprintf(stderr,"IlFlag: %d\n", expInfo->IlFlag);
   fprintf(stderr,"GoFlag: %d\n", expInfo->GoFlag);
   fprintf(stderr,"MachineID: '%s'\n", expInfo->MachineID);
//   fprintf(stderr,"InitCodefile: '%s'\n", expInfo->InitCodefile);
//   fprintf(stderr,"PreCodefile: '%s'\n", expInfo->PreCodefile);
//   fprintf(stderr,"PSCodefile: '%s'\n", expInfo->PSCodefile);
//   fprintf(stderr,"PostCodefile: '%s'\n", expInfo->PostCodefile);
//   fprintf(stderr,"AcqRTTablefile: '%s'\n", expInfo->AcqRTTablefile);
//   fprintf(stderr,"RTParmFile: '%s'\n", expInfo->RTParmFile);
//   fprintf(stderr,"WaveFormFile: '%s'\n", expInfo->WaveFormFile);
   fprintf(stderr,"Codefile: '%s'\n", expInfo->Codefile);
   fprintf(stderr,"DataFile: '%s'\n", expInfo->DataFile);
   fprintf(stderr,"CurExpFile: '%s'\n", expInfo->CurExpFile);
   fprintf(stderr,"UsrDirFile: '%s'\n", expInfo->UsrDirFile);
   fprintf(stderr,"SysDirFile: '%s'\n", expInfo->SysDirFile);
   fprintf(stderr,"UserName: '%s'\n", expInfo->UserName);
   fprintf(stderr,"AcqBaseBufName: '%s'\n", expInfo->AcqBaseBufName);
   fprintf(stderr,"VpMsgID: '%s'\n", expInfo->VpMsgID);
}

int setup_parfile(int suflag)
{
    double tmp;
    char tmpstr[256];
    int t;
    char *ptr;

    ExpInfo.PSGident = 1;	/* identify as 'C' varient PSG, Java == 100 */

    if ((P_getreal(CURRENT,"priority",&tmp,1)) >= 0)
       ExpInfo.Priority = (int) (tmp + 0.0005);
    else
       ExpInfo.Priority = 0;

    if ((P_getreal(CURRENT,"nt",&tmp,1)) >= 0)
       ExpInfo.NumTrans = (int) (tmp + 0.0005);
    else
    {   text_error("initacqqueue(): cannot set nt.");
	psg_abort(1);
    }

    if ((P_getreal(CURRENT,"bs",&tmp,1)) >= 0)
       ExpInfo.NumInBS = (int) (tmp + 0.0005);
    else
    {   text_error("initacqqueue(): cannot set bs.");
	psg_abort(1);
    }
    if (!(var_active("bs",CURRENT)))
       ExpInfo.NumInBS = 0;

    if ((P_getreal(CURRENT,"np",&tmp,1)) >= 0)
       ExpInfo.NumDataPts = (int) (tmp + 0.0005);
    else
    {   text_error("initacqqueue(): cannot set np.");
	psg_abort(1);
    }

    /* --- Number of FIDs per CT --- */

    if ((P_getreal(CURRENT,"nf",&tmp,1)) >= 0)
    { 
	DPRINT2(1,"initacqqueue(): nf = %5.0lf, active = %d \n",
				tmp,var_active("nf",CURRENT));
	if ( (tmp < 2.0 ) || (!(var_active("nf",CURRENT))) )
	{
	    tmp = 1.0;
	}
    }
    else  /* no nf, so set it to one.  */
    {
        tmp = 1.0;
    }
    ExpInfo.NumFids = (int) (tmp + 0.0005);

    ExpInfo.DataPtSize = 4;

    /* --- receiver gain --- */

    if ((P_getreal(CURRENT,"gain",&tmp,1)) >= 0)
       ExpInfo.Gain = (int) (tmp + 0.0005);
    else
    {   text_error("initacqqueue(): cannot set gain.");
	psg_abort(1);
    }

    /* --- sample spin rate --- */

    ExpInfo.Spin = 0;

    /* --- completed transients (ct) --- */

    if ((P_getreal(CURRENT,"ct",&tmp,1)) >= 0)
       ExpInfo.CurrentTran = (int) (tmp + 0.0005);
    else
    {   text_error("initacqqueue(): cannot set ct.");
	psg_abort(1);
    }

    /* --- number of fids --- */

    if ((P_getreal(CURRENT,"arraydim",&tmp,1)) >= 0)
       ExpInfo.ArrayDim = (int) (tmp + 0.0005);
    else
    {   text_error("initacqqueue(): cannot read arraydim.");
	psg_abort(1);
    }

    if ((P_getreal(CURRENT,"acqcycles",&tmp,1)) >= 0)
	ExpInfo.NumAcodes = (int) (tmp + 0.0005);
    else
    {   text_error("initacqqueue(): cannot read acqcycles.");
	psg_abort(1);
    }

    ExpInfo.FidSize = ExpInfo.DataPtSize * ExpInfo.NumDataPts * ExpInfo.NumFids;
    ExpInfo.DataSize = sizeof(struct datafilehead);
    ExpInfo.DataSize +=  (unsigned long long) (sizeof(struct datablockhead) + ExpInfo.FidSize) *
                         (unsigned long long) ExpInfo.ArrayDim;

    /* --- path to the user's experiment work directory  --- */

    if (P_getstring(GLOBAL,"userdir",tmpstr,1,255) < 0) 
    {   text_error("initacqqueue(): cannot get userdir");
	psg_abort(1);
    }
    strcpy(ExpInfo.UsrDirFile,tmpstr);

    if (P_getstring(GLOBAL,"systemdir",tmpstr,1,255) < 0) 
    {   text_error("initacqqueue(): cannot get systemdir");
	psg_abort(1);
    }
    strcpy(ExpInfo.SysDirFile,tmpstr);

    if (P_getstring(GLOBAL,"curexp",tmpstr,1,255) < 0) 
    {   text_error("initacqqueue(): cannot get curexp");
	psg_abort(1);
    }
    strcpy(ExpInfo.CurExpFile,tmpstr);

    /* --- suflag					*/
    ExpInfo.GoFlag = suflag;

    /* -------------------------------------------------------------- 
    |      Unique name to this GO,
    |      vnmrsystem/acqqueue/id is path to acq proccess files
    |
    |      Notice that goid is an array of strings, with each
    |      element having a carefully defined meaning.
    +-----------------------------------------------------------------*/

    if (P_getstring(CURRENT,"goid",infopath,1,255) < 0) 
    {   text_error("initacqqueue(): cannot get goid");
	psg_abort(1);
    }
    strcpy(ExpInfo.Codefile,infopath);
    strcat(ExpInfo.Codefile,".Code");
    ExpInfo.RTParmFile[0] = '\0';
    ExpInfo.TableFile[0] = '\0';
    ExpInfo.WaveFormFile[0] = '\0';
    ExpInfo.GradFile[0] = '\0';

    /* Beware that infopath gets accessed again
       if acqiflag is set, for the data file path */

    /* --- file path to named acqfile or exp# acqfile  'file' --- */

    if (!acqiflag)
    {
      int autoflag;
      char autopar[12];

      if (P_getstring(CURRENT,"exppath",tmpstr,1,255) < 0) 
      {   text_error("initacqqueue(): cannot get exppath");
	  psg_abort(1);
      }
      strcpy(ExpInfo.DataFile,tmpstr);
      ExpInfo.InteractiveFlag = 0;
      if (getparm("auto","string",GLOBAL,autopar,12))
          autoflag = 0;
      else
          autoflag = ((autopar[0] == 'y') || (autopar[0] == 'Y'));
      ExpInfo.ExpFlags = 0;
      if (autoflag)
      {
         strcat(ExpInfo.DataFile,".fid");
	 ExpInfo.ExpFlags |= AUTOMODE_BIT;  /* set automode bit */
      }
      if (ra_flag)
         ExpInfo.ExpFlags |=  RESUME_ACQ_BIT;  /* set RA bit */
      if (clr_at_blksize_mode)
	  ExpInfo.ExpFlags |=  CLR_AT_BS_BIT; /* For "Repeat Scan" */
    }
    else
    {
      strcpy(ExpInfo.DataFile,infopath);
      strcat(ExpInfo.DataFile,".Data");
      ExpInfo.InteractiveFlag = 1;
      ExpInfo.ExpFlags = 0;
    }

    if (P_getstring(CURRENT,"goid",tmpstr,2,255) < 0) 
    {   text_error("initacqqueue(): cannot get goid: user");
	psg_abort(1);
    }
    strcpy(ExpInfo.UserName,tmpstr);

    if (P_getstring(CURRENT,"goid",tmpstr,3,255) < 0) 
    {   text_error("initacqqueue(): cannot get goid: exp number");
	psg_abort(1);
    }
    ExpInfo.ExpNum = atoi(tmpstr);

    if (P_getstring(CURRENT,"goid",tmpstr,4,255) < 0) 
    {   text_error("initacqqueue(): cannot get goid: exp");
	psg_abort(1);
    }
    strcpy(ExpInfo.AcqBaseBufName,tmpstr);

    if (P_getstring(GLOBAL,"vnmraddr",tmpstr,1,255) < 0) 
    {   text_error("initacqqueue(): cannot get vnmraddr");
	psg_abort(1);
    }
    strcpy(ExpInfo.MachineID,tmpstr);

    /* --- interleave parameter 'il' --- */

    if (P_getstring(CURRENT,"il",tmpstr,1,4) < 0) 
    {   text_error("initacqqueue(): cannot get il");
	psg_abort(1);
    }
    ExpInfo.IlFlag = ((tmpstr[0] != 'n') && (tmpstr[0] != 'N')) ? 1 : 0;
    if (ExpInfo.IlFlag)
    {
	if (ExpInfo.NumAcodes <= 1) ExpInfo.IlFlag = 0;
	if (ExpInfo.NumInBS == 0) ExpInfo.IlFlag = 0;
	if (ExpInfo.NumTrans <= ExpInfo.NumInBS) ExpInfo.IlFlag = 0;
    }

    /* --- current element 'celem' --- */

    if ((P_getreal(CURRENT,"celem",&tmp,1)) >= 0)
       ExpInfo.Celem = (int) (tmp + 0.0005);
    else
    {   text_error("initacqqueue(): cannot set celem.");
	psg_abort(1);
    }

    /* --- Check for valid RA --- */
    ExpInfo.RAFlag = 0;		/* RaFlag */
    if (ra_flag)
    {
    	/* --- Do RA stuff --- */
	ExpInfo.RAFlag = 1;		/* RaFlag */
	if (ExpInfo.IlFlag)
	{
	   if ((ExpInfo.CurrentTran % ExpInfo.NumInBS) != 0)
	   	ExpInfo.Celem = ExpInfo.Celem - 1;
	   else
	   {
		if ((ExpInfo.Celem < ExpInfo.NumAcodes) && 
			(ExpInfo.CurrentTran >= ExpInfo.NumInBS))
		   ExpInfo.CurrentTran = ExpInfo.CurrentTran - ExpInfo.NumInBS;
	   }
	}
	else
	{
    	   if ((ExpInfo.CurrentTran > 0) && (ExpInfo.CurrentTran < 
						ExpInfo.NumTrans))
	   	ExpInfo.Celem = ExpInfo.Celem - 1;
    	   if ((ExpInfo.CurrentTran == ExpInfo.NumTrans) &&
				(ExpInfo.Celem < ExpInfo.NumAcodes))
		   ExpInfo.CurrentTran = 0;
	}
    	if ((ExpInfo.Celem < 0) || (ExpInfo.Celem >= ExpInfo.NumAcodes))
	   ExpInfo.Celem = 0;
    	ExpInfo.CurrentElem = ExpInfo.Celem;
    	/* fprintf(stdout,"initacqparms: Celem = %d\n",ExpInfo.Celem); */
    }

    /* --- when_mask parameter  --- */

    if ((P_getreal(CURRENT,"when_mask",&tmp,1)) >= 0)
       ExpInfo.ProcMask = (int) (tmp + 0.0005);
    else
    {   text_error("initacqqueue(): cannot set when_mask.");
	psg_abort(1);
    }

    ExpInfo.ProcWait = (option_check("wait")) ? 1 : 0;
    ExpInfo.DspGainBits = 0;
    ExpInfo.DspOversamp = 0;
    ExpInfo.DspOsCoef = 0;
    ExpInfo.DspSw = 0.0;
    ExpInfo.DspFb = 0.0;
    ExpInfo.DspOsskippts = 0;
    ExpInfo.DspOslsfrq = 0.0;
    ExpInfo.DspFiltFile[0] = '\0';

    ExpInfo.UserUmask = umask(0);
    umask( ExpInfo.UserUmask );		/* make sure the process umask does not change */

    /* fill in the account info */
    strcpy(tmpstr,ExpInfo.SysDirFile);
    strcat(tmpstr,"/adm/accounting/acctLog.xml");
    if ( access(tmpstr,F_OK) != 0)
    {
       ExpInfo.Billing.enabled = 0;
    }
    else
    {
        ExpInfo.Billing.enabled = 1;
    }
        t = time(0);
        ExpInfo.Billing.submitTime = t;
        ExpInfo.Billing.startTime  = t;
        ExpInfo.Billing.doneTime   = t;
        if (P_getstring(GLOBAL, "operator", tmpstr, 1, 255) < 0)
           ExpInfo.Billing.Operator[0]='\000';
        else
           strncpy(ExpInfo.Billing.Operator,tmpstr,200);
        if (P_getstring(CURRENT, "account", tmpstr, 1, 255) < 0)
           ExpInfo.Billing.account[0]='\000';
        else
           strncpy(ExpInfo.Billing.account,tmpstr,200);
        if (P_getstring(CURRENT, "pslabel", tmpstr, 1, 255) < 0)
           ExpInfo.Billing.seqfil[0]='\000';
        else
           strncpy(ExpInfo.Billing.seqfil,tmpstr,200);
        ptr = strrchr(infopath,'/');
        if ( ptr )
        {
           ptr++;
           strncpy(ExpInfo.Billing.goID, ptr ,200);
        }
        else
        {
           ExpInfo.Billing.goID[0]='\000';
        }
    return(0);
}

/* Allow user over ride of exptime calculation */
static double usertime = -1.0;
void g_setExpTime(double val)
{
  usertime = val;
}

double getExpTime()
{
  return(usertime);
}

void write_shr_info(double exp_time)
{
    int Infofd;	/* file discriptor Code disk file */
    int bytes;

    /* --- write parameter out to acqqueue file --- */

    if (usertime < 0.0)
       ExpInfo.ExpDur = exp_time;
    else
       ExpInfo.ExpDur = usertime;

    ExpInfo.NumTables = 0;
    strcpy(ExpInfo.WaveFormFile,"");
    if (ExpInfo.InteractiveFlag)
    {
       char tmppath[256*2];
       sprintf(tmppath,"%s.new",infopath);
       unlink(tmppath);
       Infofd = open(tmppath,O_EXCL | O_WRONLY | O_CREAT,0666);
    }
    else
    {
       Infofd = open(infopath,O_EXCL | O_WRONLY | O_CREAT,0666);
    }
    if (Infofd < 0)
    {	text_error("Exp info file already exists. PSG Aborted..\n");
	psg_abort(1);
    }
    bytes = write(Infofd, (const void *) &ExpInfo, sizeof( SHR_EXP_STRUCT ) );
    if (bgflag)
      fprintf(stderr,"Bytes written to info file: %d (bytes).\n",bytes);
    close(Infofd);
    if (bgflag)
       printShrExpInfo(&ExpInfo);
}

int getExpNum()
{
	return(ExpInfo.ExpNum);
}

int getIlFlag()
{
	return(ExpInfo.IlFlag);
}

/*--------------------------------------------------------------*/
/* getStartFidNum						*/
/*	Return starting fid number for ra.  ExpInfo.Celem goes	*/
/*	from 0 to n-1. getStartFidNum goes from 1 to n		*/
/*--------------------------------------------------------------*/
int getStartFidNum()
{
	return(ExpInfo.Celem + 1);
}
