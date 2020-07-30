/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/file.h>
#include <time.h>
#include "group.h"
#include "symtab.h"
#include "variables.h"
#include "params.h"
#include "pvars.h"
#include "abort.h"
#include "vfilesys.h"
#include "CSfuncs.h"
#include "arrayfuncs.h"
#include "cps.h"


/*----------------------------------------------------------------------------
|
|	Pulse Sequence Generator
 *--------------------------------------------------------------------*/


#define CALLNAME 0
#define EXEC_GO 0
#define EXEC_SU 1
#define EXEC_SHIM 2
#define EXEC_LOCK 3
#define EXEC_SPIN 4
#define EXEC_CHANGE 5
#define EXEC_SAMPLE 6

#define OK 0
#define FALSE 0
#define TRUE 1
#define ERROR 1
#define NOTFOUND -1
#define NOTREE -1
#define MAXSTR 256
#define MAXARYS 256
#define STDVAR 70
#define MAXVALS 20
#define MAXPARM 60
#define MAXPATHL 128

#define MAX_RFCHAN_NUM 6

extern void createPS();		/* create Pulse Sequence routine */
extern double getval(const char *variable);
extern double sign_add();
extern int sendExpproc(char *acqaddrstr, char *filename, char *info, int nextflag);
extern void check_for_abort();
extern void write_shr_info(double exp_time);
extern void init_hash(int size);
extern void load_hash(char **name, int number);
extern void printlpel();
extern int parse(char *string, int *narrays);
extern int A_getstring(int tree, const char *name, char **straddr, int index);
extern int A_getnames(int tree, char **nameptr, int *numvar, int maxptr);
extern int A_getvalues(int tree, char **nameptr,  int *numvar, int maxptr);
extern void initparms();
extern void initglobalptrs();
extern void initlpelements();
extern void createDPS(char *cmd, char *expdir, double arraydim, int array_num,
                      char *array_str, int pipe2nmr);
extern int closeCmd();
extern void arrayPS(int index, int numarrays);
extern void setupsignalhandler();
extern int P_receive(int *fd);
extern int setup_parfile(int suflag);
extern int setup_comm();

void first_done();
void reset();
int setup4D(double ni3, double ni2, double ni, char *parsestr, char *arraystr);
int option_check(char *option);
int gradtype_check();
int setGflags();
int QueueExp(char *codefile, int nextflag);

int bgflag = 0;
int debug  = 0;
int bugmask = 0;
int lockfid_mode = 0;
int clr_at_blksize_mode = 0;
int initializeSeq = 1;

int debugFlag = 0;	/* do NOT set this flag based on a command line argument !! */

static int pipe1[2];
static int pipe2[2];
static int ready = 0;

/**************************************
*  Structure for real-time AP tables  *
*  and global variables declarations  *
**************************************/

int  	t1, t2, t3, t4, t5, t6, 
                t7, t8, t9, t10, t11, t12, 
                t13, t14, t15, t16, t17, t18, 
                t19, t20, t21, t22, t23, t24,
                t25, t26, t27, t28, t29, t30,
                t31, t32, t33, t34, t35, t36,
                t37, t38, t39, t40, t41, t42,
                t43, t44, t45, t46, t47, t48,
                t49, t50, t51, t52, t53, t54,
                t55, t56, t57, t58, t59, t60;

struct _Tableinfo
{
   int		reset;
   int		table_number;
   int		*table_pointer;
   int		*hold_pointer;
   int		table_size;
   int		divn_factor;
   int		auto_inc;
   int		wrflag;
   int	indexptr;
   int	destptr;
};

typedef	struct _Tableinfo	Tableinfo;



/**********************************************
*  End table structures and global variables  *
**********************************************/


char       **cnames;	/* pointer array to variable names */
int          nnames;	/* number of variable names */
int          ntotal;	/* total number of variable names */
int          ncvals;	/* NUMBER OF VARIABLE  values */
double     **cvals;		/* pointer array to variable values */

char rftype[MAXSTR];	/* type of rf system used for trans & decoupler */
char amptype[MAXSTR];	/* type of amplifiers used for trans & decoupler */
char rfwg[MAXSTR];	/* y/n for rf waveform generators */
char gradtype[MAXSTR];	/* char keys w - waveform generation s-sisco n-none */
char rfband[MAXSTR];	/* RF band of trans & dec  (high or low) */
double  cattn[MAX_RFCHAN_NUM+1]; /* indicates coarse attenuators for channels */
double  fattn[MAX_RFCHAN_NUM+1]; /* indicates fine attenuators for channels */

char dqd[MAXSTR];	/* Digital Quadrature Detection, y or n */

/* --- global flags --- */
int  newacq = 1;	/* temporary nessie flag */
int  acqiflag = 0;	/* if 'acqi' was an argument, then interactive output */
int  checkflag = 0;	/* if 'check' was an argument, then check sequnece but no acquisition */
int  nomessageflag = 0;	/* if 'nomessage' was an argument, then suppress text and warning messages */
int  waitflag = 0;	/* if 'acqi' was an argument, then interactive output */
int  prepScan = 0;	/* if 'prep' was an argument, then wait for sethw to start */
int  fidscanflag = 0;	/* if 'fidscan' was an argument, then use fidscan mode for vnmrj */
int  tuneflag = 0;	/* if 'tune' or 'qtune' was an argument, then use tune mode for vnmrj */
int  mpstuneflag = 0;	/* if 'mpstune' was an argument, then use tune mode for vnmrj */
int  ok;		/* global error flag */
int  automated;		/* True if system is an automated one */
int  H1freq;		/* Proton Freq. of instrument 200,300,400,500 */
int  ptsval[MAX_RFCHAN_NUM+1];	/* PTS type for trans & decoupler */
int  rcvroff_flag = 0;	/* receiver flag; initialized to ON */
int  rcvr_hs_bit;	/* HS line that switches rcvr on/off, 0x8000 or 0x1 */
int  rfp270_hs_bit;
int  dc270_hs_bit;
int  dc2_270_hs_bit;
int  homospoil_bit;
int  spare12_hs_bits=0;
int  ap_ovrride = 0;	/* ap delay override flag; initialized to FALSE */
int  phtable_flag = 0;	/* global flag for phasetables */
int  newdec;		/* True if system uses the direct synthesis for dec */
int  newtrans; 		/* True if system uses the direct synthesis for trans */
int  newtransamp;       /* True if system uses class A amplifiers for trans */
int  newdecamp;         /* True if system uses class A amplifiers for dec */
int  vttype;		/* VT type 0=none,1=varian,2=oxford */
int  SkipHSlineTest = 0;/* Skip or not skip the HSline test agianst presHSline */
			/* need to skip this test during pulse sequence since the */
			/* usage of 'ifzero(v1)' constructs would cause the test */
			/* to fail.	*/

/* acquisition */
char  il[MAXSTR];	/* interleaved acquisition parameter, 'y','n', or 'f#' */
double  sw; 		/* Sweep width */
double  nf;		/* For E.A.T., number of fids in Pulse sequence */
double  np; 		/* Number of data points to acquire */
double  nt; 		/* number of transients */
double  sfrq;   	/* Transmitter Frequency MHz */
double  sfrq_base;   	/* Transmitter Frequency MHz */
double  dfrq; 		/* Decoupler Frequency MHz */
double  dfrq2; 		/* 2nd Decoupler Frequency MHz */
double  dfrq3; 		/* 3rd Decoupler Frequency MHz */
double  dfrq4; 		/* 4th Decoupler Frequency MHz */
double  fb;		/* Filter Bandwidth */
double  bs;		/* Block Size */
double  tof;		/* Transmitter Offset */
double  dof;		/* Decoupler Offset */
double  dof2;		/* 2nd Decoupler Offset */
double  dof3;		/* 3rd Decoupler Offset */
double  dof4;		/* 4th Decoupler Offset */
double  gain; 		/* receiver gain value, or 'n' for autogain */
int     gainactive;	/* gain active parameter flag */
double  dlp; 		/* decoupler Low Power value */
double  dhp; 		/* decoupler High Power value */
double  tpwr; 		/* Transmitter pulse power */
double  tpwrf;		/* Transmitter fine linear attenuator for pulse power */
double  dpwr;		/* Decoupler pulse power */
double  dpwrf;		/* Decoupler fine linear atten for pulse power */
double  dpwrf2;		/* 2nd Decoupler fine linear atten for pulse power */
double  dpwrf3;		/* 3rd Decoupler fine linear atten for pulse power */
double  dpwrf4;		/* 4th Decoupler fine linear atten for pulse power */
double  dpwr2;		/* 2nd Decoupler pulse power */
double  dpwr3;		/* 3rd Decoupler pulse power */
double  dpwr4;		/* 4th Decoupler pulse power */
double  filter;		/* pulse Amp filter setting */
double	xmf;		/* transmitter decoupler modulation frequency */
double  dmf;		/* 1st decoupler modulation frequency */
double  dmf2;		/* 2nd decoupler modulation frequency */
double  dmf3;		/* 3rd decoupler modulation frequency */
double  dmf4;		/* 4th decoupler modulation frequency */
double  fb;		/* filter bandwidth */
double  rattn; 		/* Receive attenuation */
int     cpflag;  	/* phase cycling flag  1=none,  0=quad detection */
int     dhpflag;	/* decoupler High Power Flag */

    /* --- pulse widths --- */
double  pw; 		/* pulse width */
double  p1; 		/* A pulse width */
double  pw90;		/* 90 degree pulse width */
double  hst;    	/* time homospoil is active */

/* --- delays --- */
double  alfa; 		/* Time after rec is turned on that acqbegins */
double  beta; 		/* audio filter time constant */
double  d1; 		/* delay */
double  d2; 		/* A delay, used in 2D/3D/4D experiments */
double  d2_init = 0.0;  /* Initial value of d2 delay, used in 2D/3D/4D experiments */
double  inc2D;		/* t1 dwell time in a 2D/3D/4D experiment */
double  d3;		/* t2 delay in a 3D/4D experiment */
double  d3_init = 0.0;  /* Initial value of d3 delay, used in 2D/3D/4D experiments */
double  inc3D;		/* t2 dwell time in a 3D/4D experiment */
double  d4;		/* t3 delay in a 4D experiment */
double  d4_init = 0.0;  /* Initial value of d4 delay, used in 2D/3D/4D experiments */
double  inc4D;		/* t3 dwell time in a 4D experiment */
double  pad; 		/* Pre-acquisition delay */
int     padactive; 	/* Pre-acquisition delay active parameter flag */
double  rof1; 		/* Time receiver is turned off before pulse */
double  rof2;		/* Time after pulse before receiver turned on */

/* --- total time of experiment --- */
double  totaltime; 	/* total timer events for a fid */
double  exptime; 	/* total time for an exp duration estimate */

int   phase1;            /* 2D acquisition mode */
int   phase2;            /* 3D acquisition mode */
int   phase3;            /* 4D acquisition mode */

int   d2_index = 0;      /* d2 increment (from 0 to ni-1) */
int   d3_index = 0;      /* d3 increment (from 0 to ni2-1) */
int   d4_index = 0;      /* d4 increment (from 0 to ni3-1) */

/* --- programmable decoupling sequences -- */
char  xseq[MAXSTR];
char  dseq[MAXSTR];
char  dseq2[MAXSTR];
char  dseq3[MAXSTR];
char  dseq4[MAXSTR];
double xres;		/* digit resolutio prg dec */
double dres;		/* digit resolutio prg dec */
double dres2;		/* digit resolutio prg dec */ 
double dres3;		/* digit resolutio prg dec */ 
double dres4;		/* digit resolutio prg dec */ 


/* --- status control --- */
char  xm[MAXSTR];		/* transmitter status control */
char  xmm[MAXSTR]; 		/* transmitter modulation type control */
char  dm[MAXSTR];		/* decoupler status control */
char  dmm[MAXSTR]; 		/* decoupler modulation type control */
char  dm2[MAXSTR]; 		/* 2nd decoupler status control */
char  dm3[MAXSTR]; 		/* 3rd decoupler status control */
char  dm4[MAXSTR]; 		/* 4th decoupler status control */
char  dmm2[MAXSTR]; 		/* 2nd decoupler modulation type control */
char  dmm3[MAXSTR]; 		/* 3rd decoupler modulation type control */
char  dmm4[MAXSTR]; 		/* 4th decoupler modulation type control */
char  homo[MAXSTR]; 		/* first  decoupler homo mode control */
char  homo2[MAXSTR]; 		/* second decoupler homo mode control */
char  homo3[MAXSTR]; 		/* third  decoupler homo mode control */
char  homo4[MAXSTR]; 		/* fourth  decoupler homo mode control */
char  hs[MAXSTR]; 		/* homospoil status control */
int   xmsize;			/* number of characters in xm */
int   xmmsize;			/* number of characters in xmm */
int   dmsize;			/* number of characters in dm */
int   dmmsize;			/* number of characters in dmm */
int   dm2size;			/* number of characters in dm2 */
int   dm3size;			/* number of characters in dm3 */
int   dm4size;			/* number of characters in dm4 */
int   dmm2size;			/* number of characters in dmm2 */
int   dmm3size;			/* number of characters in dmm3 */
int   dmm4size;			/* number of characters in dmm4 */
int   homosize;			/* number of characters in homo */
int   homo2size;		/* number of characters in homo2 */
int   homo3size;		/* number of characters in homo3 */
int   homo4size;		/* number of characters in homo4 */
int   hssize; 			/* number of characters in hs */

int curfifocount;		/* current number of word in fifo */
int setupflag;			/* alias used to invoke PSG ,go=0,su=1,etc*/
int dps_flag;			/* dps flag */
int ra_flag;			/* ra flag */
unsigned long start_elem;       /* elem (FID) for acquisition to start on (RA)*/
unsigned long completed_elem;   /* total number of completed elements (FIDs)  (RA) */
int statusindx;			/* current status index */
int HSlines;			/* High Speed output board lines */
int presHSlines;		/* last output of High Speed output board lines */

/* --- Explicit acquisition parameters --- */
int hwlooping;			/* hardware looping inprogress flag */
int hwloopelements;		/* PSG elements in hardware loop */
int starthwfifocnt;		/* fifo count at start of hwloop */
int acqtriggers;			/*# of data acquires */
int noiseacquire;		/* noise acquiring flag */

/*- These values are used so that they are changed only when their values do */
int oldlkmode;			/* previous value of lockmode */
int oldspin;			/* previous value of spin */
int oldwhenshim;		/* previous value of shimming mask */
double oldvttemp;		/* previous value of vt tempature */
double oldpad;			/* previous value of pad */

/*- These values are used so that they are included in the Acode only when */
/*  the next value out of the arrayed values has been obtained */
int oldpadindex;		/* previous value of pad value index */
int newpadindex;		/* new value of pad value index */

/* --- Pulse Seq. globals --- */
double xmtrstep;		/* phase step size trans */
double decstep;			/* phase step size dec */

unsigned int 	ix;			/* FID currently in Acode generation */
int 	nth2D;			/* 2D Element currently in Acode generation (VIS usage)*/
int     arrayelements;		/* number of array elements */
int     fifolpsize;		/* fifo loop size (63, 512, 1k, 2k, 4k) */
int     ap_interface;		/* ap bus interface type 1=500style, 2=amt style */ 
int   rotorSync;		/* rotor sync interface 1=present or 0=not present */ 

/* ---  interlock parameters, etc. --- */
char 	interLock[MAXSTR];
char 	alock[MAXSTR];
char 	wshim[MAXSTR];
int  	spin;
int  	traymax;
int  	loc;
int  	spinactive;
double  vttemp; 	/* VT temperature setting */
double  vtwait; 	/* VT temperature timeout setting */
double  vtc; 		/* VT temperature cooling gas setting */
int  	tempactive;
int  	lockmode;
int  	whenshim;
int  	shimatanyfid;


/*------------------------------------------------------------------
    RF PULSES
------------------------------------------------------------------*/
double  p2;                  /* pulse length */
double  p3;                  /* pulse length */
double  p4;                  /* pulse length */
double  p5;                  /* pulse length */
double  p6;                  /* pulse length */
double  pi;                  /* inversion pulse length */
double  psat;                /* saturation pulse length */
double  pmt;                 /* magnetization transfer pulse length */
double  pwx;                 /* X-nucleus pulse length */
double  pwx2;                /* X-nucleus pulse length */
double  psl;                 /* spin-lock pulse length */

char  pwpat[MAXSTR];         /* pattern for pw,tpwr */
char  p1pat[MAXSTR];         /* pattern for p1,tpwr1 */
char  p2pat[MAXSTR];         /* pattern for p2,tpwr2 */
char  p3pat[MAXSTR];         /* pattern for p3,tpwr3 */
char  p4pat[MAXSTR];         /* pattern for p4,tpwr4 */
char  p5pat[MAXSTR];         /* pattern for p5,tpwr5 */
char  p6pat[MAXSTR];         /* pattern for p5,tpwr5 */
char  pipat[MAXSTR];         /* pattern for pi,tpwri */
char  satpat[MAXSTR];        /* pattern for psat,satpat */
char  mtpat[MAXSTR];         /* magnetization transfer RF pattern */
char  pslpat[MAXSTR];        /* pattern for spin-lock */

double  tpwr1;               /* Transmitter pulse power */
double  tpwr2;               /* Transmitter pulse power */
double  tpwr3;               /* Transmitter pulse power */
double  tpwr4;               /* Transmitter pulse power */
double  tpwr5;               /* Transmitter pulse power */
double  tpwr6;               /* Transmitter pulse power */
double  tpwri;               /* inversion pulse power */
double  satpwr;              /* saturation pulse power */
double  mtpwr;               /* magnetization transfer pulse power */
double  pwxlvl;              /* pwx power level */
double  pwxlvl2;             /* pwx2 power level */
double  tpwrsl;              /* spin-lock power level */


double  dpwr1;               /* Decoupler pulse power */
double  dpwr4;               /* Decoupler pulse power */
double  dpwr5;               /* Decoupler pulse power */


/*------------------------------------------------------------------
    GRADIENTS
------------------------------------------------------------------*/
double  at;                  /* acquisition time */
double  satdly;              /* saturation time */
double  tau;                 /* general use delay */    
double  runtime;             /* user variable for total exp time */    


/*------------------------------------------------------------------
    FREQUENCIES
------------------------------------------------------------------*/
double  wsfrq;               /* water suppression offset */
double  chessfrq;            /* chemical shift selection offset */
double  satfrq;              /* saturation offset */


/*------------------------------------------------------------------
    BANDWIDTHS
------------------------------------------------------------------*/
double  sw1,sw2,sw3;         /* phase encode bandwidths */


/*------------------------------------------------------------------
    COUNTS AND FLAGS
------------------------------------------------------------------*/
double  nD;                  /* experiment dimensionality */
double  ns;                  /* number of slices */
double  ne;                  /* number of echoes */
double  ni;                  /* number of standard increments */
double  ni2;                 /* number of 3d increments */
double  nv,nv2,nv3;          /* number of phase encode views */
double  ssc;                 /* compressed ss transients */
double  ticks;               /* external trigger counter */

char  ir[MAXSTR];            /* inversion recovery flag */
char  ws[MAXSTR];            /* water suppression flag */
char  mt[MAXSTR];            /* magnetization transfer flag */
char  exptype[MAXSTR];       /* e.g. "se" or "fid" in CSI */
char  apptype[MAXSTR];       /* keyword for param init, e.g., "imaging"*/
char  seqfil[MAXSTR];        /* pulse sequence name */
char  satmode[MAXSTR];       /* presaturation mode */
char  verbose[MAXSTR];       /* verbose mode for sequences and psg */


/*------------------------------------------------------------------
    Miscellaneous
------------------------------------------------------------------*/
double  rfphase;             /* rf phase shift  */
double  B0;                  /* static magnetic field level */
double  aqtm;                /*  */


/* --- Pulse Seq. globals --- */
char vnHeader[50];		/* header sent to vnmr */

/*  RF Channels */
int OBSch=1;			/* The Acting Observe Channel */
int DECch=2;  			/* The Acting Decoupler Channel */
int DEC2ch=3;  			/* The Acting 2nd Decoupler Channel */
int DEC3ch=4;  			/* The Acting 3rd Decoupler Channel */
int DEC4ch=5;  			/* The Acting 4th Decoupler Channel */
int NUMch=2;			/* Number of channels configured */

/* --- Bridge 12 Pulse Seq. globals --- */
int B12_BoardNum = 0;
int B12_BlankBit = 2;
int B12_BypassFIR = 1;
int bnc = 0;
double B12_ADC = 75.0;

/*------------------------------------------------------------------------
|
|	This module is a child task.  It is called from vnmr and is passed
|	pipe file descriptors. Option are passed in the parameter go_Options.
|       This task first transfers variables passed through the pipe to this
|	task's own variable trees.  Then this task can do what it wants.
|
|	The go_Option parameter passed from parent can include
|         debug which turns on debugging.
|         next  which for automation, puts the acquisition message at the
|               top of the queue
|         acqi  which indicates that the interactive acquisition program
|               wants an acode file.  No message is sent to Acqproc.
|	  fidscan  similar to acqi, except for vnmrj
|	  ra    which means the VNMR command is RA, not GO
+-------------------------------------------------------------------------*/
char    filepath[MAXPATHL];		/* file path for Codes */
char    filexpath[MAXPATHL];		/* file path for exp# file */
char    fileRFpattern[MAXPATHL];	/* path for obs & dec RF pattern file */
char    filegrad[MAXPATHL];		/* path for Gradient file */
char    filexpan[MAXPATHL];		/* path for Future Expansion file */
char    abortfile[MAXPATHL];		/* path for abort signal file */

/* Used by locksys.c  routines from vnmr */
char systemdir[MAXPATHL];       /* vnmr system directory */
char userdir[MAXPATHL];         /* vnmr user system directory */
char curexp[MAXPATHL];       /* current experiment path */
char exppath[MAXPATHL];       /* current experiment path */
FILE *psgFile = NULL;

int main(int argc, char *argv[])
{   
    char    array[MAXSTR];
    char    parsestring[MAXSTR];	/* string that is parsed */
    char    arrayStr[MAXSTR];           /* string that is not parsed */
    char    filename[MAXPATHL];		/* file name for Codes */
    char   *gnames[50];
    char   *appdirs;

    double arraydim;	/* number of fids (ie. set of acodes) */
    double ni = 0.0;
    double ni2 = 0.0;
    double ni3 = 0.0;

    int     ngnames;
    int     i;
    int     narrays;
    int     nextflag = 0;
//    int     Rev_Num;
    int     P_rec_stat;
  
    acqiflag = ra_flag = dps_flag = waitflag =
	       fidscanflag = tuneflag = checkflag = mpstuneflag = 0;
    ok = TRUE;

    setupsignalhandler();  /* catch any exception that might core dump us. */

    if (argc < 7)  /* not enought args to start, exit */
    {	fprintf(stderr,
	"This is a background task! Only execute from within 'vnmr'!\n");
	exit(1);
    }
//    Rev_Num = atoi(argv[1]); /* GO -> PSG Revision Number */
    pipe1[0] = atoi(argv[2]); /* convert file descriptors for pipe 1*/
    pipe1[1] = atoi(argv[3]);
    pipe2[0] = atoi(argv[4]); /* convert file descriptors for pipe 2*/
    pipe2[1] = atoi(argv[5]);
    setupflag = atoi(argv[6]);	/* alias flag */
    if (bgflag)
    {
       fprintf(stderr,"\n BackGround PSG job starting\n");
       fprintf(stderr,"setupflag = %d \n", setupflag);
    }
 
    close(pipe1[1]); /* close write end of pipe 1*/
    close(pipe2[0]); /* close read end of pipe 2*/
    P_rec_stat = P_receive(pipe1);  /* Receive variables from pipe and load trees */    
    close(pipe1[0]);
    /* check options passed to psg */
    if (option_check("next"))
       waitflag = nextflag = 1;
    if (option_check("ra"))
       ra_flag = 1;
    if (option_check("acqi"))
       acqiflag = 1;
    if (option_check("fidscan"))
       fidscanflag = 1;
    if (option_check("tune"))
       tuneflag = 1;
    if (option_check("mpstune"))
       mpstuneflag = 1;
    if (option_check("debug"))
       bgflag++;
    if (option_check("sync"))
       waitflag = 1;
    if (option_check("prep"))
       prepScan = 1;
    if (option_check("check"))
    {
       checkflag = 1;
    }
    bugmask = option_check("bugmask");
    lockfid_mode = option_check("lockfid");
    clr_at_blksize_mode = option_check("bsclear");
    if (bgflag)
      fprintf(stderr,"PSG: Piping Complete\n");
 
    if ( (setupflag >= EXEC_SU) && (setupflag <= EXEC_SAMPLE) )
    {
       char cmd[64];

       switch (setupflag)
       {
          case EXEC_SU:
             strcpy(cmd,"su");
             break;
          case EXEC_SHIM:
             strcpy(cmd,"shim");
             break;
          case EXEC_LOCK:
             strcpy(cmd,"lock");
             break;
          case EXEC_SPIN:
             strcpy(cmd,"spin");
             break;
          case EXEC_CHANGE:
             strcpy(cmd,"change");
             break;
          case EXEC_SAMPLE:
             strcpy(cmd,"sample");
             break;
          default:
             strcpy(cmd,"Unknown");
             break;
       }
       text_message("%s setup command not available\n",cmd);
       closeCmd();
       close_error(0);
       exit(0);
    }
    debug = bgflag;
    gradtype_check();

/* determine whether this is INOVA or older now, before possibly calling (psg)abort */

    getparm("acqaddr","string",GLOBAL,filename,MAXPATHL);

    /* ------- Check For GO - PSG Revision Clash ------- */
    if (P_rec_stat == -1 )
    {	
	text_error("P_receive had a fatal error.\n");
	psg_abort(1);
    }

    /*-----------------------------------------------------------------
    |  begin of PSG task
    +-----------------------------------------------------------------*/

    if (strncmp("dps", argv[0], 3) == 0)
    {
        dps_flag = 1;
        if ((int)strlen(argv[0]) >= 5)
           if (argv[0][3] == '_')
                dps_flag = 3;
    }

    /* null out optional file */
    fileRFpattern[0] = 0;	/* path for Obs & Dec RF pattern file */
    filegrad[0] = 0;		/* path for Gradient file */
    filexpan[0] = 0;		/* path for Future Expansion file */

    A_getstring(CURRENT,"appdirs", &appdirs, 1);
    setAppdirValue(appdirs);
    setup_comm();   /* setup communications with acquisition processes */

    getparm("goid","string",CURRENT,filename,MAXPATHL);

    if (fidscanflag)
    {
        double fs_val;
        P_setstring(CURRENT,"alock","n",1);
        P_setstring(CURRENT,"load","n",1);
        P_setstring(CURRENT,"wshim","n",1);
	P_setstring(CURRENT,"ss","n",1);
	P_setstring(CURRENT,"dp","y",1);
	P_setstring(CURRENT,"cp","n",1);
	P_setreal(CURRENT,"bs",1.0,1);
	P_setstring(CURRENT,"wdone","",1);
	P_setreal(CURRENT,"nt",1e6,1);	/* on the new digital console */
	if (P_setreal(CURRENT,"lockacqtc",1.0,1))
        {
           P_creatvar(CURRENT, "lockacqtc", T_REAL);
	   P_setreal(CURRENT,"lockacqtc",1.0,1);
        }
        if ( (P_getreal(CURRENT,"fidshimnt",&fs_val,1) == 0) &&
             (P_getactive(CURRENT,"fidshimnt") == 1) &&
             ( (int) (fs_val+0.1) >= 1) &&
             ( (int) (fs_val+0.1) <= 16) )
        {
             double val = (double) ( (int) (fs_val+0.1) );
             P_setreal(CURRENT,"nt",val,1);
             P_setreal(CURRENT,"bs",val,1);
             P_setreal(CURRENT,"pad", 0.0, 1);
             putCmd("wnt='fid_display' wexp='fid_scan(1)'\n");
        }
    }

    if (!dps_flag)
       setup_parfile(setupflag);
    strcpy(filepath,filename);	/* /vnmrsystem/acqqueue/exp1.greg.012345 */
    strcpy(filexpath,filepath); /* save this path for psg_abort() */
    strcat(filepath,".Code");	/* /vnmrsystem/acqqueue/exp1.greg.012345.Code */


    A_getnames(GLOBAL,gnames,&ngnames,50);
    if (bgflag)
    {
        fprintf(stderr,"Number of names: %d \n",nnames);
        for (i=0; i< ngnames; i++)
        {
          fprintf(stderr,"global name[%d] = '%s' \n",i,gnames[i]);
        }
    }

    /* --- setup the arrays of variable names and values --- */

    /* --- get number of variables in CURRENT tree */
    A_getnames(CURRENT,0L,&ntotal,1024); /* get number of variables in tree */
    if (bgflag)
        fprintf(stderr,"Number of variables: %d \n",ntotal);
    cnames = (char **) malloc(ntotal * (sizeof(char *))); /* allocate memory */
    cvals = (double **) malloc(ntotal * (sizeof(double *)));
    if ( cvals == 0L  || cnames == 0L )
    {
	text_error("insuffient memory for variable pointer allocation!!");
	reset(); 
	psg_abort(0);
    }
    /* --- load pointer arrays with pointers to names and values --- */
    A_getnames(CURRENT,cnames,&nnames,ntotal);
    A_getvalues(CURRENT,(char **) cvals,&ncvals,ntotal);

    /* variable look up now done with hash tables */
    init_hash(ntotal);
    load_hash(cnames,ntotal);

    /* --- malloc space for Acodes --- */
    /* NB: parameter acqcycles now gives number of acode sets;
     * "arraydim" parameter is number of data blocks. */
    if (getparm("acqcycles","real",CURRENT,&arraydim,1))
	psg_abort(1);

    if (acqiflag || checkflag || (setupflag > 0))  /* if not a GO force to 1D */
        arraydim = 1.0;		/* any setup is 1D, nomatter what. */

    /* --- set parameter to unused value, so new & old will not be 
       --- the same 1st time --- */
    oldlkmode = -1;			/* previous value of lockmode */
    oldspin = -1;			/* previous value of spin */
    oldwhenshim = -1;			/* previous value of shimming mask */
    oldvttemp = 29999.0;		/* previous value of vt tempature */
    oldpad = 29999.0;			/* previous value of pad */
    oldpadindex = -1;			/* previous value index of PAD */
    newpadindex = 0;			/* new value index of PAD */
    exptime  = 0.0; 	   /* total timer events for a exp duration estimate */
    start_elem = 1;             /* elem (FID) for acquisition to start on (RA)*/
    completed_elem = 0;         /* total number of completed elements (FIDs)  (RA) */

    /* --- setup global flags and variables --- */
    if (setGflags()) psg_abort(0);

    /* --- set up global parameter --- */
    initparms();
    /* --- set up lc data sturcture  --- */
    /* --- set up automation data sturcture  --- */

    setupPsgFile();
    /* ---- A single or arrayed experiment ???? --- */
    ix = nth2D = 0;		/* initialize ix */

    if (checkflag || mpstuneflag || tuneflag || (dps_flag > 1))
        arraydim = 1.0;
    if (option_check("checkarray"))
    {
       checkflag = 1;
    }
    if (!dps_flag)
    {
       if ( (psgFile = fopen(filepath,"w")) == NULL )
       {
           text_error("go: could not create file for pulse sequence codes\n");
	   psg_abort(0);
       }
       fprintf(psgFile,"DEBUG          %d\n",debug);
       if ( ! mpstuneflag )
       {
       fprintf(psgFile,"BOARD_NUMBER   %d\n",B12_BoardNum);
       fprintf(psgFile,"BLANK_BIT      %d\n",B12_BlankBit);
       fprintf(psgFile,"BYPASS_FIR     %d\n",B12_BypassFIR);
       fprintf(psgFile,"ADC_FREQUENCY  %g\n",B12_ADC);
       }
       fprintf(psgFile,"FILE           %s\n",exppath);
       fprintf(psgFile,"ARRAYDIM       %d\n",(int) (arraydim+0.1));
    }
    sfrq_base =sfrq;

    if (arraydim <= 1.5)
    {
        totaltime  = 0.0; /* total timer events for a fid */
	ix = nth2D = 1;		/* generating Acodes for FID 1 */
	if (dps_flag)
        {
              createDPS(argv[0], curexp, arraydim, 0, NULL, pipe2[1]);
              close_error(0);
              exit(0);
        }
	if ( ! mpstuneflag)
           fprintf(psgFile,"PULSEPROG_START     %d\n",ix);

	createPS();
	if ( ! mpstuneflag && ! tuneflag )
           fprintf(psgFile,"PULSEPROG_DONE      %d\n",ix);
#ifdef XXX
        totaltime -= pad;
        if (var_active("ss",CURRENT))
        {
           int ss = (int) (sign_add(getval("ss"), 0.0005));
           if (ss < 0)
              ss = -ss;
           totaltime *= (getval("nt") + (double) ss);   /* mult by NT + SS */
        }
        else
           totaltime *= getval("nt");   /* mult by number of transients (NT) */
	exptime += (totaltime + pad);
        totaltime *= getval("nt");   /* mult by number of transients (NT) */
#endif
        exptime = totaltime;
        first_done();
    }
    else	/*  Arrayed Experiment .... */
    {
        /* --- initial global variables value pointers that can be arrayed */
        initglobalptrs();
        initlpelements();

    	if (getparm("array","string",CURRENT,array,MAXSTR))
	    psg_abort(1);
    	if (bgflag)
	    fprintf(stderr,"array: '%s' \n",array);
	strcpy(parsestring,array);
        /*----------------------------------------------------------------
        |	test for presence of ni, ni2, and ni3
	|	generate the appropriate 2D/3D/4D experiment
        +---------------------------------------------------------------*/
	P_getreal(CURRENT, "ni", &ni, 1);
	P_getreal(CURRENT, "ni2", &ni2, 1);
        P_getreal(CURRENT, "ni3", &ni3, 1);

	if (setup4D(ni3, ni2, ni, parsestring, array))
           psg_abort(0);

        if (dps_flag)
           strcpy(arrayStr, parsestring);

        /*----------------------------------------------------------------*/

	if (bgflag)
	  fprintf(stderr,"parsestring: '%s' \n",parsestring);
    	if (parse(parsestring,&narrays))  /* parse 'array' setup looping elements */
          psg_abort(1);
        arrayelements = narrays;


	if (bgflag)
        {
          printlpel();
        }
        if (dps_flag)
        {
              createDPS(argv[0], curexp, arraydim, narrays, arrayStr, pipe2[1]);
              close_error(0);
              exit(0);
        }

    	arrayPS(0,narrays);
    }
    if (psgFile != NULL)
       fclose(psgFile);
    if (checkflag)
    {
       if (!option_check("checksilent"))
          text_message("go check complete\n");
       closeCmd();
       close_error(0);
       exit(0);
    }

    if (bgflag)
    {
      fprintf(stderr,"total time of experiment = %lf (sec) \n",exptime);
      fprintf(stderr,"Submit experiment to acq. process \n");
    }
    write_shr_info(exptime);
    if (!acqiflag && QueueExp(filexpath,nextflag))
      psg_abort(1);
    if (bgflag)
      fprintf(stderr,"PSG terminating normally\n");
    close_error(0);
    exit(0);
}

/*--------------------------------------------------------------------------
|   This writes to the pipe that go is waiting on to decide when the first
|   element is done.
+--------------------------------------------------------------------------*/
void first_done()
{
   int ret __attribute__((unused));
   closeCmd();
   ret = write(pipe2[1],&ready,sizeof(int)); /* tell go, PSG is done with first element */
}
/*--------------------------------------------------------------------------
|   This closes the pipe that go is waiting on when it is in automation mode.
|   This procedure is called from close_error.  Close_error is called either
|   when PSG completes successfully or when PSG calls psg_abort.
+--------------------------------------------------------------------------*/
void closepipe2(int success)
{
    char autopar[12];
    int ret __attribute__((unused));


    if (acqiflag || waitflag)
       ret = write(pipe2[1],&ready,sizeof(int)); /* tell go, PSG is done */
    else if (!dps_flag && (getparm("auto","string",GLOBAL,autopar,12) == 0))
        if ((autopar[0] == 'y') || (autopar[0] == 'Y'))
            ret = write(pipe2[1],&ready,sizeof(int)); /* tell go, PSG is done */
    close(pipe2[1]); /* close write end of pipe 2*/
}
/*-----------------------------------------------------------------------
+------------------------------------------------------------------------*/
void reset()
{
    if (cnames) free(cnames);
    if (cvals) free(cvals);
}
/*------------------------------------------------------------------------------
|
|	setGflag()
|
|	This function sets the global flags and variables that do not change
|	during the pulse sequence 
|       the 2D experiment 
|   Modified   Author     Purpose
|   --------   ------     -------
|   12/13/88   Greg B.    1. Added Code for new Objects, 
|			     Ext_Trig, ToLnAttn, DoLnAttn, HS_Rotor.
|			  2. Attn Object has changed to use the OFFSET rather
|			     than the MAX to determine the directional offset 
|			     (i.e. works forward or backwards)
+----------------------------------------------------------------------------*/
int setGflags()
{
    double tmpval;

    if ( P_getreal(GLOBAL,"B12_BoardNum",&tmpval,1) >= 0 )
    {
       B12_BoardNum = (int) (tmpval + 0.0005);
    }
    if ( P_getreal(GLOBAL,"BB12_BlankBit",&tmpval,1) >= 0 )
    {
       B12_BlankBit = (int) (tmpval + 0.0005);
    }
    if ( P_getreal(GLOBAL,"B12_BypassFIR",&tmpval,1) >= 0 )
    {
       B12_BypassFIR = (int) (tmpval + 0.0005);
    }
    if ( P_getreal(GLOBAL,"B12_ADC",&tmpval,1) >= 0 )
    {
       B12_ADC = tmpval;
    }
    if ( P_getreal(GLOBAL,"bnc",&tmpval,1) >= 0 )
    {
       bnc = (int) (tmpval + 0.0005);
    }
    if ( P_getreal(GLOBAL,"numrfch",&tmpval,1) >= 0 )
    {
        NUMch = (int) (tmpval + 0.0005);
	if (( NUMch < 1) || (NUMch > MAX_RFCHAN_NUM))
	{
            abort_message(
              "Number of RF Channels specified '%d' is too large.. PSG Aborted.\n",
	      NUMch);
	}
        /* Fool PSG if numrfch is 1 */
	if ( NUMch < 2)
          NUMch = 2;
    }
    else
       NUMch = 2;

    /*--- obtain coarse & fine attenuator RF channel configuration ---*/
    cattn[0] = fattn[0] = 0.0;  /* dont use 0 index */

    if (rftype[1] == '\0')
    {
       rftype[1] = rftype[0];
       rftype[2] = '\0';
    }
    if (rftype[1] == 'c'  || rftype[1] == 'C' ||
        rftype[1] == 'd'  || rftype[1] == 'D')
    {
	newdec = TRUE;
    }
    else
    {
	newdec = FALSE;
    }

    if (amptype[0] == 'c'  || amptype[0] == 'C')
    {
	newtransamp = FALSE;
    }
    else
    {
	newtransamp = TRUE;
    }

    if (amptype[1] == '\0')
    {
       amptype[1] = amptype[0];
       amptype[2] = '\0';
    }
    if (amptype[1] == 'c'  || amptype[1] == 'C')
    {
	newdecamp = FALSE;
    }
    else
    {
	newdecamp = TRUE;
    }
    automated = TRUE;

    /* ---- set basic instrument proton frequency --- */
    if (getparm("h1freq","real",GLOBAL,&tmpval,1))
    	return(ERROR);
    H1freq = (int) tmpval;

/*****************************************
*  Load userdir, systemdir, and curexp.  *
*****************************************/

    if (P_getstring(GLOBAL, "userdir", userdir, 1, MAXPATHL-1) < 0)
       text_error("PSG unable to locate current user directory\n");
    if (P_getstring(GLOBAL, "systemdir", systemdir, 1, MAXPATHL-1) < 0)
       text_error("PSG unable to locate current system directory\n");
    if (P_getstring(GLOBAL, "curexp", curexp, 1, MAXPATHL-1) < 0)
       text_error("PSG unable to locate current experiment directory\n");
    if (P_getstring(CURRENT, "exppath", exppath, 1, MAXPATHL-1) < 0)
       if (!dps_flag)
          text_error("PSG unable to locate current experiment path\n");
    sprintf(abortfile,"%s/acqqueue/psg_abort", systemdir);
    if (access( abortfile, W_OK ) == 0)
       unlink(abortfile);


    /* --- set VT type on instrument --- */
    if (getparm("vttype","real",GLOBAL,&tmpval,1))
    	return(ERROR);
    vttype = (int) tmpval;

    /* --- set lock correction factor --- */
    /* now calc in freq object */

    /* --- set lockfreq flag, active or not active --- */
    /*   not used any more  */

    return(OK);
}

static int findWord(const char *key, char *instr)
{
    char *ptr;
    char *nextch;

    ptr = instr;
    while ( (ptr = strstr(ptr, key)) )
    {
       nextch = ptr + strlen(key);
       if ( ( *nextch == ',' ) || (*nextch == ')' ) || (*nextch == '\0' ) )
       {
          return(1);
       }
       ptr += strlen(key);
    }
    return(0);
}

/*--------------------------------------------------------------------------
|
|       setup4D(ninc_t3, ninc_t2, ninc_t1, parsestring, arraystring)/5
|
|	This function arrays the d4, d3 and d2 variables with the correct
|	values for the 2D/3D/4D experiment.  d2 increments first, then d3,
|	and then d4.
|
+-------------------------------------------------------------------------*/
int setup4D(double ni3, double ni2, double ni, char *parsestr, char *arraystr)
{
   int		index = 0,
		i,
		num = 0;
   int          elCorr = 0;
   double	sw1,
		sw2,
		sw3,
		d2,
		d3,
		d4;
   int          sparse = 0;
   int          CStype = 0;


   if ( ! CSinit("sampling", curexp) )
   {
      int res;
      res =  CSgetSched(curexp);
      if (res)
      {
         if (res == -1)
            abort_message("Sparse sampling schedule not found");
         else if (res == -2)
            abort_message("Sparse sampling schedule is empty");
         else if (res == -3)
            abort_message("Sparse sampling schedule columns does not match sparse dimensions");
      }
      else
         sparse =  1;
   }

   strcpy(parsestr, "");
   if (sparse)
   {
      num = getCSdimension();
      if (num == 0)
         sparse = 0;
   }
   if (sparse)
   {
      int narray;
      int lps;
      int i;
      int j,k;
      int found = -1;
      int chk[CSMAXDIM];

      strcpy(parsestr, arraystr);
      if (parse(parsestr, &narray))
         abort_message("array parameter is wrong");
      strcpy(parsestr, "");
      lps = numLoops();
      CStype = CStypeIndexed();
      for (i=0; i < num; i++)
         chk[i] = 0;
      for (i=0; i < lps; i++)
      {
         for (j=0; j < varsInLoop(i); j++)
         {
            for (k=0; k < num; k++)
            {
               if ( ! strcmp(varNameInLoop(i,j), getCSpar(k) ) )
               {
                  chk[k] = 1;
                  found = i;
               }
            }
         }
      }
      k = 0;
      for (i=0; i < num; i++)
         if (chk[i])
            k++;
      if (k > 1)
      {
         abort_message("array parameter is inconsistent with sparse data acquisition");
      }
      if (found != -1)
      {
         i = 0;
            for (j=0; j < varsInLoop(found); j++)
            {
               for (k=0; k < num; k++)
               {
                  if ( ! strcmp(varNameInLoop(found,j), getCSpar(k) ) )
                     i++;
               }
            }
         if (i != num)
            abort_message("array parameter is inconsistent with sparse data acquisition");
      }

      if (num == 1)
         strcpy(parsestr, getCSpar(0));
      else
      {
         strcpy(parsestr, "(");
         for (k=0; k < num-1; k++)
         {
           strcat(parsestr, getCSpar(k));
           strcat(parsestr, ",");
         }
         strcat(parsestr, getCSpar(num-1));
         strcat(parsestr, ")");
      }
   }

   if (ni3 > 1.5)
   {
      int sparseDim;
      if (getparm("sw3", "real", CURRENT, &sw3, 1))
         return(ERROR);
      if (getparm("d4","real",CURRENT,&d4_init,1))
         return(ERROR);

      sparseDim = (sparse && ( (index = getCSparIndex("d4")) != -1) );
      if ( ! findWord("d4",arraystr))
      {
         if ( ! sparseDim)
            strcpy(parsestr, "d4");
      }
      else
         elCorr++;

      inc4D = 1.0/sw3;
      if (sparseDim)
      {
         num  = getCSnum();
         for (i = 0;  i < num; i++)
         {
            if (CStype)
            {
               int csIndex;

               csIndex = (int) getCSval(index, i);
               d4 = d4_init + csIndex * inc4D;
            }
            else
            {
               d4 = d4_init + getCSval(index, i);
            }
            if ((P_setreal(CURRENT, "d4", d4, i+1)) == ERROR)
            {
               text_error("Could not set d4 values");
               return(ERROR);
            }
         }
      }
      else
      {
         num = (int) (ni3 - 1.0 + 0.5);
         d4 = d4_init;
         for (i = 0, index = 2; i < num; i++, index++)
         {
            d4 = d4 + inc4D;
            if ((P_setreal(CURRENT, "d4", d4, index)) == ERROR)
            {
               text_error("Could not set d4 values");
               return(ERROR);
            }
         }
      }
   }


   if (ni2 > 1.5)
   {
      int sparseDim;
      if (getparm("sw2", "real", CURRENT, &sw2, 1))
           return(ERROR);
      if (getparm("d3","real",CURRENT,&d3_init,1))
           return(ERROR);

      sparseDim = (sparse && ( (index = getCSparIndex("d3")) != -1) );
      if ( ! findWord("d3",arraystr))
      {
         if ( ! sparseDim )
         {
         if (strcmp(parsestr, "") == 0)
         {
            strcpy(parsestr, "d3");
         }
         else
         {
            strcat(parsestr, ",d3");
         }
         }
      }
      else
      {
         elCorr++;
      }

      inc3D = 1.0/sw2;
      if (sparseDim)
      {
         num  = getCSnum();
         for (i = 0;  i < num; i++)
         {
            if (CStype)
            {
               int csIndex;

               csIndex = (int) getCSval(index, i);
               d3 = d3_init + csIndex * inc3D;
            }
            else
            {
               d3 = d3_init + getCSval(index, i);
            }
            if ((P_setreal(CURRENT, "d3", d3, i+1)) == ERROR)
            {
               text_error("Could not set d3 values");
               return(ERROR);
            }
         }
      }
      else
      {
         num = (int) (ni2 - 1.0 + 0.5);
         d3 = d3_init;
         for (i = 0, index = 2; i < num; i++, index++)
         {
            d3 = d3 + inc3D;
            if ((P_setreal(CURRENT, "d3", d3, index)) == ERROR)
            {
               text_error("Could not set d3 values");
               return(ERROR);
            }
         }
      }
   }

   if (ni > 1.5)
   {
      int sparseDim;
      if (getparm("sw1", "real", CURRENT, &sw1, 1))
           return(ERROR);
      if (getparm("d2","real",CURRENT,&d2_init,1))
           return(ERROR);

      sparseDim = (sparse && ( (index = getCSparIndex("d2")) != -1) );
      if ( ! findWord("d2",arraystr))
      {
         if ( ! sparseDim )
         {
         if (strcmp(parsestr, "") == 0)
         {
            strcpy(parsestr, "d2");
         }
         else
         {
            strcat(parsestr, ",d2");
         }
         }
      }
      else
      {
         elCorr++;
      }

      inc2D = 1.0/sw1;
      if (sparseDim)
      {
         num  = getCSnum();
         for (i = 0;  i < num; i++)
         {
            if (CStype)
            {
               int csIndex;

               csIndex = (int) getCSval(index, i);
               d2 = d2_init + csIndex * inc2D;
            }
            else
            {
               d2 = d2_init + getCSval(index, i);
            }
            if ((P_setreal(CURRENT, "d2", d2, i+1)) == ERROR)
            {
               text_error("Could not set d2 values");
               return(ERROR);
            }
         }
      }
      else
      {
         num = (int) (ni - 1.0 + 0.5);
         d2 = d2_init;
         for (i = 0, index = 2; i < num; i++, index++)
         {
            d2 = d2 + inc2D;
            if ((P_setreal(CURRENT, "d2", d2, index)) == ERROR)
            {
               text_error("Could not set d2 values");
               return(ERROR);
            }
         }
      }
   }

   if (elCorr)
   {
      double arrayelemts;
      P_getreal(CURRENT, "arrayelemts", &arrayelemts, 1);
      arrayelemts -= (double) elCorr;
      P_setreal(CURRENT, "arrayelemts", arrayelemts, 1);
   }

   if (arraystr[0] != '\0')
   {
      if (strcmp(parsestr, "") != 0)
         strcat(parsestr, ",");
      strcat(parsestr, arraystr);
   }

   return(OK);
}

/*------------------------------------------------------------------------------
|
|	Write the acquisition message to psgQ
|
+----------------------------------------------------------------------------*/
static void queue_psg(char *auto_dir, char *fid_name, char *msg)
{
}
/*------------------------------------------------------------------------------
|
|	Queue the Experiment to the acq. process 
|
|   Modified   Author     Purpose
|   --------   ------     -------
|   2/10/89   Greg B.     1. Changed QueueExp() to pass the additional information:
|			     start_elem and complete_elem to Acqproc 
|
|   5/1998    Robert L.   include the "setup flag" in the information string
|                         sent to Expproc, so the nature of the experiment
|			  may be derived from the experiment queue.  See
|			  acqhwcmd.c, SCCS category vnmr.
+----------------------------------------------------------------------------*/
#define QUEUE 1
int QueueExp(char *codefile, int nextflag)
{
    char fidpath[MAXSTR];
    char addr[MAXSTR];
    char message[MAXSTR];
    char commnd[MAXSTR*2];
    char autopar[12];
    int autoflag;
    int expflags;
    int priority;
    int ret __attribute__((unused));

/*	No Longer use Vnmr priority parameter, now used for U+ non-automation go and go('next') */
/*
    if (getparm("priority","real",CURRENT,&priority,1))
    {
        text_error("cannot get priority variable\n");
        return(ERROR);
    }
*/

    if (!nextflag)
      priority = 5;
    else
      priority = 6;  /* increase priority to this Exp run next */


    if (getparm("auto","string",GLOBAL,autopar,12))
        autoflag = 0;
    else
        autoflag = ((autopar[0] == 'y') || (autopar[0] == 'Y'));

    expflags = 0;
#ifdef XXX
    if (autoflag)
	expflags = ((long)expflags | AUTOMODE_BIT);  /* set automode bit */

    if (ra_flag)
        expflags |=  RESUME_ACQ_BIT;  /* set RA bit */
#endif

    if (getparm("file","string",CURRENT,fidpath,MAXSTR))
	return(ERROR);
    if (getparm("vnmraddr","string",GLOBAL,addr,MAXSTR))
	return(ERROR);
    sprintf(message,"%d,%s,%d,%lf,%s,%s,%s,%s,%s,%d,%d,%lu,%lu,",
		QUEUE,addr,
		(int)priority,exptime,fidpath,codefile,
		fileRFpattern, filegrad, filexpan,
		(int)setupflag,(int)expflags,
                start_elem, completed_elem);
    if (bgflag)
    {
      fprintf(stderr,"fidpath: '%s'\n",fidpath);
      fprintf(stderr,"codefile: '%s'\n",codefile);
      fprintf(stderr,"priority: %d'\n",(int) priority);
      fprintf(stderr,"time: %lf\n",totaltime);
      fprintf(stderr,"suflag: %d\n",setupflag);
      fprintf(stderr,"expflags: %d\n",expflags);
      fprintf(stderr,"start_elem: %lu\n",start_elem);
      fprintf(stderr,"completed_elem: %lu\n",completed_elem);
      fprintf(stderr,"msge: '%s'\n",message);
      fprintf(stderr,"auto: %d\n",autoflag);
    }
    check_for_abort();
    if (autoflag)
    {   char autodir[MAXSTR];
        char tmpstr[MAXSTR];
        int ex;

        if (getparm("autodir","string",GLOBAL,autodir,MAXSTR))
	    return(ERROR);

        P_getstring(CURRENT,"goid",tmpstr,3,255);
        sprintf(message,"%s %s auto",codefile,tmpstr);

        /* There is a race condition between writing out the
           psgQ files and go.c preparing the .fid directory where
           the data will be stored.  This section of code makes
           sure go.c finishes creating the .fid directory before
           psgQ is written.  There is a 5 second timeout. Generally,
           it waits between 0 and 20 msec.
         */
        if (fidpath[0] == '/')
           sprintf(commnd,"%s.fid/text",fidpath);
        else
           sprintf(commnd,"%s/%s.fid/text",autodir,fidpath);
        ex = 0;
        while ( access(commnd,R_OK) && (ex < 50) )
        {
           struct timespec timer;

           timer.tv_sec=0;
           timer.tv_nsec = 10000000;   /* 10 msec */
           nanosleep(&timer, NULL);
           ex++;
        }

        if ( getCSnum() )
        {
           char *csname = getCSname();
           if (fidpath[0] == '/')
              sprintf(commnd,"cp %s/%s %s.fid/%s",curexp,csname,fidpath,csname);
           else
              sprintf(commnd,"cp %s/%s %s/%s.fid/%s",curexp,csname,autodir,fidpath,csname);
           ret = system(commnd);
        }

        if (nextflag)
        {
          /* Temporarily use commnd to hold the filename */
          sprintf(commnd,"%s/psgQ",autodir);
          ex = access(commnd,W_OK);
          if (ex == 0)
          {
             sprintf(commnd,"mv %s/psgQ %s/psgQ.locked",autodir,autodir);
             ret = system(commnd);
          }
          queue_psg(autodir,fidpath,message);
          if (ex == 0)
          {
             sprintf(commnd,"cat %s/psgQ.locked >> %s/psgQ; rm %s/psgQ.locked",
                           autodir,autodir,autodir);
             ret = system(commnd);
          }
        }
        else
        {
          queue_psg(autodir,fidpath,message);
        }
        sync();
    }
    else
    {
       char infostr[MAXSTR];
       char tmpstr[MAXSTR];
       char tmpstr2[MAXSTR];

       if (getparm("acqaddr","string",GLOBAL,addr,MAXSTR))
	 return(ERROR);

       if ( getCSnum() )
       {
          char *csname = getCSname();

          sprintf(commnd,"cp %s/%s %s/acqfil/%s",curexp,csname,curexp,csname);
          ret = system(commnd);
       }

       P_getstring(CURRENT,"goid",tmpstr,3,255);
       P_getstring(CURRENT,"goid",tmpstr2,2,255);
       sprintf(infostr,"%s %s %d",tmpstr,tmpstr2,setupflag);
       if (sendExpproc(addr,codefile,infostr,nextflag))
       {
        text_error("Experiment unable to be sent\n");
        return(ERROR);
       }
    }
    return(OK);
}

void check_for_abort()
{
   if (access( abortfile, W_OK ) == 0)
   {
      unlink(abortfile);
      psg_abort(1);
   }
}

/*
 * Check if the passed argument is gradtype
 * reset gradtype is needed
 */
int gradtype_check()
{
   vInfo  varinfo;
   int num;
   size_t oplen;
   char tmpstring[MAXSTR];
   char option[16];
   char gradtype[16];

   if ( P_getVarInfo(CURRENT,"go_Options",&varinfo) )
      return(0);
   if (P_getstring(GLOBAL, "gradtype", gradtype, 1, 15) < 0)
      return(0);
   num = 1;
   strcpy(option,"gradtype");
   oplen = strlen(option);
   while (num <= varinfo.size)
   {
      if ( P_getstring(CURRENT,"go_Options",tmpstring,num,MAXPATHL-1) >= 0 )
      {
         if ( strncmp(tmpstring, option, oplen) == 0 )
         {
             size_t gradlen = strlen(tmpstring);
             if (gradlen == oplen)
             {
                 gradtype[0]='a';
                 gradtype[1]='a';
                 break;
             }
             else if (gradlen <= oplen+4)
             {
                 int count = 0;
                 oplen++;
                 while ( (tmpstring[oplen] != '\0') && (count < 3) )
                 {
                    gradtype[count] = tmpstring[oplen];
                    count++;
                    oplen++;
                 }
             }
         }
      }
      else
      {
         return(0);
      }
      num++;
   }
   P_setstring(GLOBAL, "gradtype", gradtype, 1);
   return(0);
}

/*
 * Check if the passed argument is present in the list of
 * options passed to PSG from go.
 */
int option_check(char *option)
{
   vInfo  varinfo;
   int num;
   int oplen;
   char tmpstring[MAXSTR];

   if ( P_getVarInfo(CURRENT,"go_Options",&varinfo) )
      return(0);
   num = 1;
   oplen = strlen(option);
   while (num <= varinfo.size)
   {
      if ( P_getstring(CURRENT,"go_Options",tmpstring,num,MAXPATHL-1) >= 0 )
      {
         if ( strncmp(tmpstring, option, oplen) == 0 )
	 {
	     if (strlen(tmpstring) == oplen)
	     {
		 return(1);
	     }
	     else
	     {
		 if (tmpstring[oplen] == '=')
		 {
		     return (int)strtol(tmpstring+oplen+1, NULL, 0);
		 }
	     }
	 }
      }
      else
      {
         return(0);
      }
      num++;
   }
   return(0);
}

int
is_psg4acqi()
{
    return( acqiflag );
}
