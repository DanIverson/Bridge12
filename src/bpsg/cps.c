/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <math.h>
#include "group.h"
#include "symtab.h"
#include "variables.h"
#include "acqparms.h"
#include "macros.h"
#include "pvars.h"
#include "abort.h"
#include "cps.h"
#include "b12funcs.h"


#define CALLNAME 0
#define OK 0
#define ERROR 1
#define NOTFOUND -1
#define NOTREE -1
#define MAXSTR 256
#define MAXARYS 256
#define STDVAR 70
#define MAXVALS 20
#define MAXPARM 60
#define EXEC_SU 1
#define EXEC_SHIM 2

extern double sign_add();
extern void pulsesequence();
extern void pre_fidsequence();
extern int getExpNum();
extern int deliverMessageSuid(char *interface, char *message );
extern double calc_sw(double sweepWidth, int scans);
extern int maxPhaseCycle();
extern void initPhaseTables();
extern void chkLoop(int maxPh, int totalScans, int *loops,  int *loopcnt, int *rem);
extern void initElems();
extern void endElems();

extern int bgflag;
extern int ap_interface;	/* ap bus interface type 1=500style, 2=amt style */ 
extern int SkipHSlineTest;
extern int presHSlines;
extern int acqiflag;
extern int checkflag;
extern int dps_flag;
extern int lockfid_mode;
extern int initializeSeq;
extern int tuneflag;
extern int mpstuneflag;

extern char   **cnames;	/* pointer array to variable names */
extern int	newtransamp;	/* True if transmitter uses Class A amps */
extern int      nnames;		/* number of variable names */
extern int      ntotal;		/* total number of variable names */
extern int      ncvals;		/* NUMBER OF VARIABLE  values */
extern double **cvals;	/* pointer array to variable values */
extern double relaxdelay;
extern int scanflag,initflagrt,ilssval,ilctss; /* inova realtime vars */
extern int tmprt; /* inova realtime vars */
extern FILE *psgFile;


extern int traymax;

int lkflt_fast;	/* value to set lock loop filter to when fast */
int lkflt_slow;	/* value to set lock loop filter to when slow */
int HSrotor;
int HS_Dtm_Adc;		/* Flag to convert.c to select INOVA 5MHz High Speed DTM/ADC */
int activeRcvrMask;	/* Which receivers to use */

double d0; 	      /* defined array element overhead time */
double interfidovhd;  /* calculated array element overhead time */

int ntrt, ct, ctss, fidctr, HSlines_ptr, oph, bsval, bsctr;
int ssval, ssctr, nsp_ptr, sratert, rttmp, spare1rt, id2, id3, id4;
int zero, one, two, three;
int v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14;
int tablert, tpwrrt, dhprt, tphsrt, dphsrt, dlvlrt;
int lIndex;

/*-----------------------------------------------------------------------
|  createPS()
|	create the acode for the Pulse Sequence.
|				Author Greg Brissey  6/30/86
+-----------------------------------------------------------------------*/
void createPS()
{
   FILE *tmpFD;
   int maxPh;
   int totalScans;
   int nscLoop;
   int loopCnt;
   int remCnt;


//    setRF();		/* set RF to initial states */

    fprintf(psgFile,"NUMBER_POINTS %d\n",(int) (np+0.1));

    if ( mpstuneflag || tuneflag )
    {
       pulsesequence();
       return;
    }
    fprintf(psgFile,"SPECTROMETER_FREQUENCY %g\n",sfrq);
    totalScans = (int) (nt+0.1);
    fprintf(psgFile,"NUMBER_OF_SCANS %d\n",totalScans);
    fprintf(psgFile,"SPECTRAL_WIDTH %g\n",sw);
    fprintf(psgFile,"POWER %g\n",tpwrf);
    double actual_sw = calc_sw(sw, (int) (nt+0.1) );
    if ( fabs(sw-actual_sw) > 0.1 )
    {
       double actual_at= (np / 2) / actual_sw;
       if (ix == 1)
       {
          putCmd("setvalue('sw',%10.4f) setvalue('sw',%10.4f,'processed')",
                  actual_sw, actual_sw);
          putCmd("setvalue('at',%10.4f) setvalue('at',%10.4f,'processed')",
                  actual_at, actual_at);
       }
       sw = actual_sw;
       at = actual_at;
    }
    

//    loadshims();


    curfifocount = 0;		/* reinitialize number of fifo words */



    
    /* --- initialize expilict acquisition parameters --- */
    hwlooping = 0;
    hwloopelements = 0;
    starthwfifocnt = 0;

/******************************************
*  Start pulsesequence and table section  *
******************************************/

    SkipHSlineTest = 1; /* incase of ifzero(v1) type of constructs don't */
			/* test HSlines against presHSlines */

    initPhaseTables();
    fprintf(psgFile,"PULSE_ELEMENTS START\n");
    fprintf(psgFile,"RATTN %g\n",rattn);
    tmpFD = psgFile;
    psgFile = NULL;
    pulsesequence();
    maxPh = maxPhaseCycle();
    psgFile = tmpFD;
    chkLoop(maxPh, totalScans, &nscLoop, &loopCnt, &remCnt);
    if (nscLoop)
    {
       fprintf(psgFile,"NSC_LOOP %d\n", nscLoop);
       for (lIndex = 0; lIndex < loopCnt; lIndex++)
       {
    acqtriggers = 0;
       if (setupflag == 0)
       pre_fidsequence();		/* users pre fid functions */
    fprintf(psgFile,"PHASE_RESET 1\n");
    initElems();
    pulsesequence();	/* generate Acodes from USER Pulse Sequence */
   if (acqtriggers == 0)
   {
      acquire(np, 1.0/sw);
   }
   if (lIndex+1 == loopCnt)
      fprintf(psgFile,"NSC_ENDLOOP %d\n", (int) (nt+0.1));
   endElems();
      }
         totaltime *= nscLoop;
   }
   if (remCnt)
    {
       for (lIndex = loopCnt; lIndex < loopCnt+remCnt; lIndex++)
       {
    acqtriggers = 0;
       if (setupflag == 0)
       pre_fidsequence();		/* users pre fid functions */
    fprintf(psgFile,"PHASE_RESET 1\n");
    initElems();
    pulsesequence();	/* generate Acodes from USER Pulse Sequence */
   if (acqtriggers == 0)
   {
      acquire(np, 1.0/sw);
   }
   endElems();
      }
   }



    /* compress by default!! and don't if commanded */

    /* write out generated lc,auto & acodes */


    return;
}

/*-----------------------------------------------------------------
|	initparms()/
|	initializes the main variables used 
+------------------------------------------------------------------*/
void initparms()
{
    double getval();
    double getvalnwarn();
    char   tmpstr[20];

    sw = getval("sw");
    np = getval("np");

    if ( P_getreal(CURRENT,"nf",&nf,1) < 0 )
    {
        nf = 0.0;                /* if not found assume 0 */
    }
    if (nf < 2.0) 
	nf = 1.0;
    if ( P_getreal(CURRENT,"tpwrf",&tpwrf,1) < 0 )
    {
        tpwrf = 100.0;                /* if not found assume 0 */
    }
    if ( P_getreal(CURRENT,"rattn",&rattn,1) < 0 )
    {
        rattn = 0.0;                /* if not found assume 0 */
    }
    else if (!var_active("rattn",CURRENT))
    {
	rattn = 0.0;
    }

/*****************************
*  Set the observe channel.  *
*****************************/

    if (ap_interface == 4)
    {
       char nucleiNameStr[MAXSTR], nucleus[MAXSTR], strbuf[MAXSTR*2];  
       nucleiNameStr[0]='\0'; strbuf[0]='\0'; nucleus[0]='\0';
       strcpy(nucleiNameStr,"'");

       int i;
       for(i=1; i<=NUMch; i++)
       {
            if (i == OBSch)
            {
               getstrnwarn("tn",nucleus);
               if (strcmp(nucleus,"")==0)
                  strcpy(nucleus,"-");
               sprintf(strbuf,"%s ",nucleus);
               strcat(nucleiNameStr,strbuf);
            } 
            else if (i == DECch)
            {
               getstrnwarn("dn",nucleus);
               if (strcmp(nucleus,"")==0)
                  strcpy(nucleus,"-");
               sprintf(strbuf,"%s ",nucleus);
               strcat(nucleiNameStr,strbuf);
            } 
            else if (i == DEC2ch) 
            {
               getstrnwarn("dn2",nucleus);
               if (strcmp(nucleus,"")==0)
                  strcpy(nucleus,"-");
               sprintf(strbuf,"%s ",nucleus);
               strcat(nucleiNameStr,strbuf);
            } 
            else if (i == DEC3ch) 
            {
               getstrnwarn("dn3",nucleus);
               if (strcmp(nucleus,"")==0)
                  strcpy(nucleus,"-");
               sprintf(strbuf,"%s ",nucleus);
               strcat(nucleiNameStr,strbuf);
            }
       }
       nucleiNameStr[strlen(nucleiNameStr)-1] = '\0';
       strcat(nucleiNameStr,"'");
       if (P_getstring(CURRENT, "rfchnuclei", strbuf, 1, 255) >= 0)
       {
          putCmd("rfchnuclei = %s",nucleiNameStr);
       }

       if (bgflag)
         fprintf(stderr,"Obs= %d Dec= %d Dec2= %d Dec3= %d\n",
                         OBSch,DECch,DEC2ch,DEC3ch);
       xmf = getvalnwarn("xmf");	/* observe modulation freq */
       if (xmf == 0.0) xmf=1000.0;
/*       getstrnwarn("xmm",xmm); */
/*       if (xmm[0]==0) strcpy(xmm,"c"); */
       strcpy(xmm,"c");
       xmmsize = strlen(xmm);
       getstrnwarn("xseq",xseq);
       xres = getvalnwarn("xres");	/* prg decoupler digital resolution */
       if (xres < 1.0)
          xres = 1.0;
    }
    getstrnwarn("xm",xm);
    if (xm[0]==0) strcpy(xm,"n");
    xmsize = strlen(xm);
    nt = getval("nt");
    sfrq = getval("sfrq");

/* Digital Quadrature Detection */
    if (P_getstring(CURRENT,"cp",tmpstr,1,12) < 0)
       cpflag = 0;
    else
       cpflag = (tmpstr[0] == 'y') ? 0 : 1;

    getstr("il",il);		/* interleave parameter */
    if (P_getstring(GLOBAL,"dsp",tmpstr,1,12) < 0)
       tmpstr[0] = 'n';
    if (dps_flag)            /* no DSP if dps */
       tmpstr[0] = 'n';

    fb = getval("fb");
    filter = getval("filter");		/* pulse Amp filter setting */
    tof = getval("tof");
    bs = getval("bs");
    if (!var_active("bs",CURRENT))
	bs = 0.0;
    pw = getval("pw");
    pw90 = getval("pw90");
    p1 = getval("p1");

    pwx = getvalnwarn("pwx");
    pwxlvl = getvalnwarn("pwxlvl");
    tau = getvalnwarn("tau");
    satdly = getvalnwarn("satdly");
    satfrq = getvalnwarn("satfrq");
    satpwr = getvalnwarn("satpwr");
    getstrnwarn("satmode",satmode);

    /* --- delays --- */
    d1 = getval("d1"); 		/* delay */
    d2 = getval("d2"); 		/* a delay: used in 2D experiments */
    d3 = getvalnwarn("d3");	/* a delay: used in 3D experiments */
    d4 = getvalnwarn("d4");	/* a delay: used in 4D experiments */
    phase1 = (int) sign_add(getvalnwarn("phase"),0.005);
    phase2 = (int) sign_add(getvalnwarn("phase2"),0.005);
    phase3 = (int) sign_add(getvalnwarn("phase3"),0.005);
    rof1 = getval("rof1"); 	/* Time receiver is turned off before pulse */
    rof2 = getval("rof2");	/* Time after pulse before receiver turned on */
    alfa = getval("alfa"); 	/* Time after rec is turned on that acqbegins */
    pad = getval("pad"); 	/* Pre-acquisition delay */
    padactive = var_active("pad",CURRENT);
    hst = getval("hst"); 	/* HomoSpoil delay */

    getstr("rfband",rfband);	/* RF band, high or low */

    getstr("hs",hs);
    hssize = strlen(hs);
    /* setlockmode(); */		/* set up lockmode variable,homo bits */
    if (bgflag)
    {
      fprintf(stderr,"sw = %lf, sfrq = %10.8lf\n",sw,sfrq);
      fprintf(stderr,"hs='%s',%d\n",hs,hssize);
    }
    gain = getval("gain");
    /* if the preamp mixer is pre 20 Mhz IF for unity+ and 	*/
    /* inova systems 500 Mhz and above, the broadband (lowband) */
    /* mixer does not have 18 dB attenuators.  This means the	*/
    /* lowest receiver gain possible is 18.			*/ 
    gainactive = var_active("gain",CURRENT); /* non arrayable */
    /* InterLocks is set by go.  It will have three chars.
     * char 0 is for lock
     * char 1 is for spin
     * char 2 is for temp
     */

}
/*-----------------------------------------------------------------
|	getval()/1
|	returns value of variable 
+------------------------------------------------------------------*/
double getval(const char *variable)
{
    int index;

    /* index = findsname(variable,cnames,nnames); */
    index = find(variable);   /* hash table find */
    if (index == NOTFOUND)
    {
        fprintf(stdout,"'%s': not found, value assigned to zero.\n",variable);
	return(0.0);
    }
    if (bgflag)
        fprintf(stderr,"GETVAL(): Variable: %s, value: %lf \n",
     	    variable,*( (double *) cvals[index]) );
    return( *( (double *) cvals[index]) );
}
/*-----------------------------------------------------------------
|	getstr()/1
|	returns string value of variable 
+------------------------------------------------------------------*/
void getstr(const char *variable, char buf[])
{
    int index;

    /* index = findsname(variable,cnames,nnames); */
    index = find(variable);   /* hash table find */
    if (index != NOTFOUND)
    {   
	char *content;

	content = ((char *) cvals[index]);
    	if (bgflag)
            fprintf(stderr,"GETSTR(): Variable: %s, value: '%s' \n",
     	    	variable,content);
    	strncpy(buf,content,MAXSTR-1);
	buf[MAXSTR-1] = 0;
    }
    else
    {
        fprintf(stdout,"'%s': not found, value assigned to null.\n",variable);
	buf[0] = 0;
    }
}
/*-----------------------------------------------------------------
|	getvalnwarn()/1
|	returns value of variable 
+------------------------------------------------------------------*/
double getvalnwarn(const char *variable)
{
    int index;

    /* index = findsname(variable,cnames,nnames); */
    index = find(variable);   /* hash table find */
    if (index == NOTFOUND)
    {
	return(0.0);
    }
    if (bgflag)
        fprintf(stderr,"GETVAL(): Variable: %s, value: %lf \n",
     	    variable,*( (double *) cvals[index]) );
    return( *( (double *) cvals[index]) );
}
/*-----------------------------------------------------------------
|	getstrnwarn()/1
|	returns string value of variable 
+------------------------------------------------------------------*/
void getstrnwarn(const char *variable, char buf[])
{
    int index;

    /* index = findsname(variable,cnames,nnames); */
    index = find(variable);   /* hash table find */
    if (index != NOTFOUND)
    {   
	char *content;

	content = ((char *) cvals[index]);
    	if (bgflag)
            fprintf(stderr,"GETSTR(): Variable: %s, value: '%s' \n",
     	    	variable,content);
    	strncpy(buf,content,MAXSTR-1);
	buf[MAXSTR-1] = 0;
    }
    else
    {
	buf[0] = 0;
    }
}
/*-----------------------------------------------------------------
|	sign_add()/2
|  	 uses sign of first argument to decide to add or subtract 
|			second argument	to first
|	returns new value (double)
+------------------------------------------------------------------*/
double sign_add(arg1,arg2)
double arg1;
double arg2;
{
    if (arg1 >= 0.0)
	return(arg1 + arg2);
    else
	return(arg1 - arg2);
}
/*-----------------------------------------------------------------
|	putcode()/1
|	puts integer word into Codes array, increments pointer
|
+------------------------------------------------------------------*/
void putcode(int word)
{
}
/*-----------------------------------------------------------------
|       putLongCode()/1
|       puts long integer word into short interger Codes array
|
+------------------------------------------------------------------*/
void putLongCode(unsigned int longWord)
{
}

void init_codefile(char *codepath)
{
}

/*-----------------------------------------------------------------
|       getmaxval()/1
|       Gets the maximum value of an arrayed or list real parameter. 
+------------------------------------------------------------------*/
int getmaxval(const char *parname )
{
    int      size,r,i,tmpval,maxval;
    double   dval;
    vInfo    varinfo;

    if ( (r = P_getVarInfo(CURRENT, parname, &varinfo)) ) {
        printf("getmaxval: could not find the parameter \"%s\"\n",parname);
	psg_abort(1);
    }
    if ((int)varinfo.basicType != 1) {
        printf("getmaxval: \"%s\" is not an array of reals.\n",parname);
	psg_abort(1);
    }

    size = (int)varinfo.size;
    maxval = 0;
    for (i=0; i<size; i++) {
        if ( P_getreal(CURRENT,parname,&dval,i+1) ) {
	    printf("getmaxval: problem getting array element %d.\n",i+1);
	    psg_abort(1);
	}
	tmpval = (int)(dval+0.5);
	if (tmpval > maxval)	maxval = tmpval;
    }
    return(maxval);

}


/*-----------------------------------------------------------------
|	putval()/2
|	Sets a vnmr parmeter to a given value.
+------------------------------------------------------------------*/
void putval(paramname,paramvalue)
char *paramname;
double paramvalue;
{
   int stat,expnum;
   char	message[STDVAR];
   char addr[MAXSTR];

   expnum = getExpNum();
   stat = -1;
   if (getparm("vnmraddr","string",GLOBAL,addr,MAXSTR))
   {
	text_error("putval: cannot get Vnmr address for %s.\n",paramname);
   }
   else
   {
   	sprintf(message,"sysputval(%d,'%s',%g)\n",
			expnum,paramname,paramvalue);
   	stat = deliverMessageSuid(addr,message);
	if (stat < 0)
	{
	   text_error("putval: Error in parameter: %s.\n",paramname);
	}
   }
   
}

/*-----------------------------------------------------------------
|	putstr()/2
|	Sets a vnmr parmeter to a given string.
+------------------------------------------------------------------*/
void putstr(paramname,paramstring)
char *paramname;
char *paramstring;
{
   int stat,expnum;
   char	message[STDVAR];
   char addr[MAXSTR];

   expnum = getExpNum();
   stat = -1;
   if (getparm("vnmraddr","string",GLOBAL,addr,MAXSTR))
   {
	text_error("putval: cannot get Vnmr address for %s.\n",paramname);
   }
   else
   {
   	sprintf(message,"sysputval(%d,'%s','%s')\n",
					expnum,paramname,paramstring);
   	stat = deliverMessageSuid(addr,message);
	if (stat < 0)
	{
	   text_error("putval: Error in parameter: %s.\n",paramname);
	}
   }
   
}

