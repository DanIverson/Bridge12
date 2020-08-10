/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include "group.h"
#include "variables.h"
#include "acqparms.h"
#include "pvars.h"
#include "CSfuncs.h"
#include "abort.h"
#include "cps.h"

#define DPRTLEVEL 1
#define MAXGLOBALS 135
#define MAXGVARLEN 40
#define MAXVAR 40
#define MAXLOOPS 80
#define MAXSTR 256

#define FALSE 0
#define TRUE 1
#define ERROR 1
#define NOTFOUND -1
#define NOTREE -1

#ifdef DEBUG
#define DPRINT(level, str) \
        if (bgflag >= level) fprintf(stdout,str)
#define DPRINT1(level, str, arg1) \
        if (bgflag >= level) fprintf(stdout,str,arg1)
#define DPRINT2(level, str, arg1, arg2) \
        if (bgflag >= level) fprintf(stdout,str,arg1,arg2)
#define DPRINT3(level, str, arg1, arg2, arg3) \
        if (bgflag >= level) fprintf(stdout,str,arg1,arg2,arg3)
#define DPRINT4(level, str, arg1, arg2, arg3, arg4) \
        if (bgflag >= level) fprintf(stdout,str,arg1,arg2,arg3,arg4)
#else
#define DPRINT(level, str)
#define DPRINT1(level, str, arg2)
#define DPRINT2(level, str, arg1, arg2)
#define DPRINT3(level, str, arg1, arg2, arg3)
#define DPRINT4(level, str, arg1, arg2, arg3, arg4)
#endif

extern int      bgflag;
extern char   **cnames;		/* pointer array to variable names */
extern double **cvals;		/* pointer array to variable values */
extern int      nnames;		/* number of variable names */
extern double   exptime;

extern double  d2_init;         /* Initial value of d2 delay, used in 2D/3D/4D experiments */
extern double  d3_init;         /* Initial value of d3 delay, used in 2D/3D/4D experiments */
extern double  d4_init;         /* Initial value of d4 delay, used in 2D/3D/4D experiments */

extern FILE *psgFile;
extern void check_for_abort();
extern void first_done();
extern int A_getstring(int tree, const char *name, char **straddr, int index);
extern int A_getreal(int tree, const char *name, double **valaddr, int index);
extern void createPS();
extern void reset();

/*-----------------------------------------------------------------------
|  structure loopelemt:
|	Looping element for an arrayed experiment.
|	Information needed for an looping element:
|	1. The number of variables in the looping element.
|	2. The number of values, this is constant  for all
|	   variables in a loop element  (check by go)
|	3. The array of pointers to the variable names in the
|	   in the looping element.
|	4. The array of pointers to the addresses of the variable values
|	   in the looping element.
|	5. The array of indecies  into the global structure for the updateing
|	    of the global values
+------------------------------------------------------------------------*/
struct _loopelemt
{
   char           *lpvar[MAXVAR];	/* pointers to variable name */
   char           *varaddr[MAXVAR];	/* pointers to where the variable's
					 * value ptr goes */
   int             numvar;	/* number of variables in loop element */
   int             numvals;	/* number of values, vars must have same #,
				 * chk in go */
   int             vartype[MAXVAR];	/* Basic Type of variable, string,
					 * real */
   int             glblindex[MAXVAR];	/* indexs of variable in to global
					 * structure */
};
typedef struct _loopelemt loopelemt;

static loopelemt *lpel[MAXLOOPS];	/* An array of pointers to looping
					 * elements */

static int numberLoops = 0;
/*-----------------------------------------------------------------------
|  structure glblvar:
|	global variable information to update the variable if it has been
|	arrayed.
|	Information needed for an looping element:
|	1. The variable name.
|	2. The the address of the global variable, so that it can be updated
|	3. Function that is called, to handle lc variable or others that require
|	   some type of translation, or addition information that depends on this
|	   variable.
+------------------------------------------------------------------------*/
struct _glblvar
{
   double         *glblvalue;
   int             (*funcptr) ();
   char            gnam[MAXGVARLEN];
};
typedef struct _glblvar glblvar;

static glblvar  glblvars[MAXGLOBALS];
static void setup(char *varptr, int lpindex, int varindex);
static int srchglobal(char *name);

int numLoops()
{
   return(numberLoops);
}

int varsInLoop(int index)
{
   return( (lpel[index]) ? lpel[index]->numvar : 0);
}

int valuesInLoop(int index)
{
   return( (lpel[index]) ? lpel[index]->numvals : 0);
}

char *varNameInLoop(int index, int index2)
{
   return( (lpel[index] && lpel[index]->lpvar[index2]) ? lpel[index]->lpvar[index2] : NULL);
}


/*------------------------------------------------------------------
|  initlpelements()  -  malloc space for the loop elements required
|			Author: Greg Brissey 6/2/89
+------------------------------------------------------------------*/
void initlpelements()
{
   int             i;

   for (i = 0; i < MAXLOOPS; i++)
      lpel[i] = (loopelemt *) 0;
   return;
}

void resetlpelements()
{
   int             i;

   for (i = 0; i < MAXLOOPS; i++)
   {
      if (lpel[i])
         free(lpel[i]);
      lpel[i] = (loopelemt *) 0;
   }
   numberLoops = 0;
   return;
}

/*------------------------------------------------------------------
|  printlpel()  -  print lpel strcuture contents
+------------------------------------------------------------------*/
void printlpel()
{
   int             i;
   int             j;

   for (i = 0; i < MAXLOOPS; i++)
   {
      if (lpel[i])
      {
         int num;

         num = lpel[i]->numvar;
         fprintf(stderr, "lpel[%d] contains: numvar= %d with numvalues= %d\n",
              i, lpel[i]->numvar, lpel[i]->numvals);
         for (j=0; j<num; j++)
         {
            fprintf(stderr, "Loop element %d: Variable[%d] '%s'\n",
              i, j, lpel[i]->lpvar[j]);
         }
      }
   }
}

/*--------------------------------------------------------------------------
|   parse()/2
|	parse the variable 'array' and setup the loop element structures
+--------------------------------------------------------------------------*/
#define letter(c) ((('a'<=(c))&&((c)<='z'))||(('A'<=(c))&&((c)<='Z'))||((c)=='_')||((c)=='$')||((c)=='#'))
#define digit(c) (('0'<=(c))&&((c)<='9'))
#define COMMA 0x2C
#define RPRIN 0x29
#define LPRIN 0x28
/*--------------------------------------------------------------------------
+--------------------------------------------------------------------------*/
int parse(char *string, int *narrays)
{
   int             state;
   int             varindex;
   char           *ptr;
   char           *varptr;
   int             lpindex = 0;

   state = 0;
   *narrays = 0;
   ptr = string;
   varindex = 0;
   varptr = NULL;
   if (bgflag)
      fprintf(stderr, "parse(): string: '%s' -----\n", string);
   /* ---  test the variables as we parse them --- */
   /*
    * ---  This is a 4 state parser, 0-1: separate variables
    * 2-4: diagonal set variables
    * --
    */
   while (1)
   {
      switch (state)
      {
	    /* ---  start of variable name --- */
	 case 0:
	    if (bgflag)
	    {
	       fprintf(stderr, "Case 0: ");
	       fprintf(stderr, "letter: '%c', ", *ptr);
	    }
	    if (letter(*ptr))	/* 1st letter go to state 1 */
	    {
	       varptr = ptr;
	       state = 1;
	       ptr++;
               *narrays += 1;
	    }
	    else
	    {
	       if (*ptr == LPRIN)	/* start of diagnal arrays */
	       {
		  state = 2;
		  ptr++;
                  *narrays += 1;
	       }
	       else
	       {
		  if (*ptr == '\0')	/* done ? */
		     return (0);
		  else		/* error */
		  {
		     text_error("Syntax error in variable array");
		     return (ERROR);
		  }
	       }
	    }
	    if (bgflag)
	       fprintf(stderr, " state = %d \n", state);
	    break;
	    /* --- complete a single array variable till ',' --- */
	 case 1:
	    if (bgflag)
	    {
	       fprintf(stderr, "Case 1: ");
	       fprintf(stderr, "letter: '%c', ", *ptr);
	    }
	    if (letter(*ptr) || digit(*ptr))
	    {
	       ptr++;
	    }
	    else
	    {
	       if (*ptr == COMMA)
	       {
		  *ptr = '\0';
		  setup(varptr, lpindex, varindex);
		  ptr++;
		  lpindex++;
		  state = 0;
	       }
	       else
	       {
		  if (*ptr == '\0')
		  {
		     setup(varptr, lpindex, varindex);
		     return (0);
		  }
		  else
		  {
		     text_error("Syntax Error in variable 'array'");
		     return (ERROR);
		  }
	       }
	    }
	    if (bgflag)
	       fprintf(stderr, " state = %d \n", state);
	    break;
	    /* --- start of diagnal arrayed variables  'eg. (pw,d1)' --- */
	 case 2:
	    if (bgflag)
	    {
	       fprintf(stderr, "Case 2: ");
	       fprintf(stderr, "letter: '%c', ", *ptr);
	    }
	    /* if (letter1(*ptr)) */
	    if (letter(*ptr))
	    {
	       varptr = ptr;
	       state = 3;
	       ptr++;
	    }
	    else
	    {
	       text_error("Syntax Error in variable 'array'");
	       return (ERROR);
	    }
	    if (bgflag)
	       fprintf(stderr, " state = %d \n", state);
	    break;
	    /* --- finish a diagonal arrayed variable  name --- */
	 case 3:
	    if (bgflag)
	    {
	       fprintf(stderr, "Case 3: ");
	       fprintf(stderr, "letter: '%c', ", *ptr);
	    }
	    if (letter(*ptr) || digit(*ptr))
	    {
	       ptr++;
	    }
	    else
	    {
	       if (*ptr == COMMA)
	       {
		  *ptr = '\0';
		  setup(varptr, lpindex, varindex);
		  ptr++;
		  varindex++;
		  state = 2;
	       }
	       else if (*ptr == RPRIN)
	       {
		  *ptr = '\0';
		  setup(varptr, lpindex, varindex);
		  ptr++;
		  varindex = 0; /* was == 0 8-9-90*/
		  state = 4;
	       }
	       else
	       {
		  text_error("Syntax Error in variable 'array'");
		  return (ERROR);
	       }
	    }
	    if (bgflag)
	       fprintf(stderr, " state = %d \n", state);
	    break;
	    /* --- finish a diagonal arrayed variable  set --- */
	 case 4:
	    if (bgflag)
	    {
	       fprintf(stderr, "Case 4: ");
	       fprintf(stderr, "letter: '%c', ", *ptr);
	    }
	    if (*ptr == COMMA)
	    {
	       *ptr = '\0';
	       ptr++;
	       lpindex++;
	       varindex = 0;
	       state = 0;
	    }
	    else
	    {
	       if (*ptr == '\0')
	       {
		  return (0);
	       }
	       else
	       {
		  text_error("Syntax Error in variable 'array'");
		  return (ERROR);
	       }
	    }
	    if (bgflag)
	       fprintf(stderr, " state = %d \n", state);
	    break;
      }
   }
}

/*-------------------------------------------------------------------
|  setup - setup lpel sturcture..
|
|  Initialize looping elements for the arrayed experiament
|	IF the variable array = "sw,(pw,d1),dm" then
|        sw variable element is lpelement 0, one variable in loop element.
|        pw variable element is lpelement 1, two variables in loop element.
|        d1 variable element is lpelement 1, two variables in loop element.
|        dm variable element is lpelement 2, one variable in loop element.
|   There are 3 looping elements in this arrayed experiment.
+--------------------------------------------------------------------*/
/*  *varptr	 pointer to variable name */
/*  lpindex	 loop element index 0-# */
/*  varindex	 variable index 0-# (# of variable per loop element */
static void setup(char *varptr, int lpindex, int varindex)
{
   int             index;
   int             type;
   int             ret;
   vInfo           varinfo;	/* variable information structure */

   /* --- variable info  --- */
   if ( (ret = P_getVarInfo(CURRENT, varptr, &varinfo)) )
   {
      text_error("Cannot find the variable: '%s'", varptr);
      if (bgflag)
	 P_err(ret, varptr, ": ");
      psg_abort(1);
   }

   type = varinfo.basicType;

   index = find(varptr);
   if (index == NOTFOUND)
   {
      text_error("variable '%s' does not exist.", varptr);
      psg_abort(1);
   }

   if ( ! lpel[lpindex] )
   {
      if ( (lpel[lpindex] = (loopelemt *) malloc(MAXLOOPS * sizeof(loopelemt))) == 0L)
      {
         text_error("insuffient memory for loop elements.");
         reset();
         psg_abort(1);
      }
   }

   /* --- number of variables --- */

   lpel[lpindex]->numvar = varindex + 1;


   /* --- number values for loopelement --- */
   lpel[lpindex]->numvals = varinfo.size;

   /* --- Basic Type of the variable, string or real --- */
   lpel[lpindex]->vartype[varindex] = type;	/* varinfo.basicType */

   /* --- pointer to a variable name --- */

   lpel[lpindex]->lpvar[varindex] = cnames[index];

   /* --- pointer to the array of param ptrs --- */

   lpel[lpindex]->varaddr[varindex] = (char *) &cvals[index];

   /* --- index of variable in global structure --- */

   lpel[lpindex]->glblindex[varindex] = srchglobal(varptr);

   numberLoops = lpindex+1;
}

/*-----------------------------------------------------------------------
|    arrayPS:
|	index = looping element index
|	numarrays = number of looping elements
|
|	This routine for each value of an arrayed variable for each
|	variable in the looping element is set in the PS param array.
|	Then this routine recusively calls itself until it reaches the
|	last looping element.
|	The net result are nested FOR loops for performing the arrayed
|	experiment.
|	For 2D experments an array of the d2 values is created and is the
|	 frist looping element(or only element).
|
|   Modified   Author     Purpose
|   --------   ------     -------
|   6/6/89     Greg B.    1. Added Code to use new lpel structure members
|			  2. Basic restructuring for speed improvements
|			  3. Use global strcuture for updateing global parameters
|
+-----------------------------------------------------------------------*/
static int curarrayindex = 0;

int get_curarrayindex()
{
   return(curarrayindex);
}

void arrayPS(int index, int numarrays)
{
   char           *name;
   char          **parmptr;
   int             valindx;
   int             varindx;
   int             nvals,
                   nvars;
   int             ret;
   int             gindx;

   name = lpel[index]->lpvar[0];
   nvals = lpel[index]->numvals;

   /* --- For each value of the arrayed variable(s) --- */
   for (valindx = 1; valindx <= nvals; valindx++)
   {
      nvars = lpel[index]->numvar;

      if (index == 0)
         nth2D++;		/* if first arrayed variable (i.e. 2D
				   variable) increment */

      /* --- for each of the arrayed variable(s) get its value --- */
      for (varindx = 0; varindx < nvars; varindx++)
      {
	 name = lpel[index]->lpvar[varindx];

	 /* -- if pad 'arrayed' set its global padindex to value index -- */
	 /* -- preacqdelay can tell a new value has been entered and that */
	 /* the pad should be included in the Acodes, otherwise not */
	 /* do the same for temp being arrayed, this way a preacqdelay is */
	 /* done after each change in temperature			 */

/*
	    if ( (strcmp(name,"pad") == NULL) || (strcmp(name,"temp") == NULL) )
	    {
		newpadindex = valindx;
	    }
*/

	 parmptr = (char **) lpel[index]->varaddr[varindx];

         curarrayindex = valindx - 1;
	 if (lpel[index]->vartype[varindx] != T_STRING)
	 {
	    if ((ret = A_getreal(CURRENT, name, (double **) parmptr, valindx)) < 0)
	    {
	       text_error("Cannot find the variable: '%s'", name);
	       if (bgflag)
		  P_err(ret, name, ": ");
	       psg_abort(1);
	    }
	    gindx = lpel[index]->glblindex[varindx];
	    if (gindx >= 0) {
	       if (glblvars[gindx].glblvalue)
		  *(glblvars[gindx].glblvalue) = *((double *) *parmptr);
	       if (glblvars[gindx].funcptr)
		  (*glblvars[gindx].funcptr) (*((double *) *parmptr));
	    }
	 }
	 else
	 {
	    if ((ret = A_getstring(CURRENT, name, parmptr, valindx)) < 0)
	    {
	       text_error("Cannot find the variable: '%s'", name);
	       if (bgflag)
		  P_err(ret, name, ": ");
	       psg_abort(1);
	    }
	    gindx = lpel[index]->glblindex[varindx];
	    if (gindx >= 0) {
	       if (glblvars[gindx].funcptr)
		  (*glblvars[gindx].funcptr) (((char *) *parmptr));
	    }
	 }

      }
      if (numarrays > 1)
	 arrayPS(index + 1, numarrays - 1);
      else
      {
	 totaltime = 0.0;	/* total timer events for a fid */
	 ix++;			/* generating Acode for FID ix */
         if ((ix % 100) == 0)
            check_for_abort();
	 /* update lc elemid = ix */
        fprintf(psgFile,"PULSEPROG_START     %d\n",ix);
	 createPS();
        fprintf(psgFile,"PULSEPROG_DONE      %d\n",ix);

	 /* code offset calcs moved to write_acodes */

#ifdef XXX
         if (var_active("ss",CURRENT))
         {
            int ss = (int) (sign_add(getval("ss"), 0.0005));
            if (ss < 0)
               ss = -ss;
            else if (ix != 1)
               ss = 0;
            totaltime *= (nt + (double) ss);   /* mult by NT + SS */
         }
         else
	    totaltime *= nt;	/* mult by number of transients (NT) */
#endif
	 exptime += totaltime; /* added back pad, add up times */
         if (ix == 1)
            first_done();
      }
   }
}

/*------------------------------------------------------------
|  srchglobal()/1  sreaches global structure array for named
|		   variable, returns index into variable
+-----------------------------------------------------------*/
static int srchglobal(char *name)
{
   int             index;

   index = 0;
   while (index < MAXGLOBALS)
   {
      if (strcmp(name, glblvars[index].gnam) == 0)
	 return (index);
      index++;
   }
   return (-1);
}

void setGlobalDouble(const char *name, double val)
{
   int             index;

   if ( (index = find(name)) != -1 )
   {
      *(cvals[index]) = val;
   }
}

/*---------------------------------------------------
|  functions call for parameters effecting
|  frequencies .
+--------------------------------------------------*/
static
int func4sfrq(double value)
{
   return (0);
}
static
int func4tof(double value)
{
   sfrq=sfrq_base+ (value*1e-6);
   return (0);
}
/*---------------------------------------------------
|  functions call for global double parameters effecting
|  low core or automation structure.
+--------------------------------------------------*/
static
int func4nt(double value)
{
   DPRINT1(DPRTLEVEL,"func for nt called, new value: %ld\n", (int) (value + 0.005));
   return (0);
}
static
int func4pad_temp(value)
double          value;
{
   oldpad = -1.0;  /* forces pad for either pad or temp change */
   DPRINT(DPRTLEVEL,"func for pad or temp called");
   return (0);
}

/*---------------------------------------------------
|  string functions call for global string parameters
+--------------------------------------------------*/
static
int func4hs(value)
char           *value;
{
   strcpy(hs, value);
   hssize = strlen(hs);
   DPRINT1(DPRTLEVEL,"func for hs called value='%s'\n", value);
   return (0);
}
static
int func4homo(value)
char           *value;
{
   strcpy(homo, value);
   homosize = strlen(homo);
   DPRINT1(DPRTLEVEL,"func for homo called value='%s'\n", value);
   return (0);
}
static
int func4wshim(value)
char           *value;
{
   return (0);
}
static int func4d2(double value)
{
   if (getCSparIndex("d2") == -1)
      d2_index = get_curarrayindex();
   else
      d2_index = (int) ( (d2 - d2_init) / inc2D + 0.5);
   DPRINT(DPRTLEVEL,"func for d2 called\n");
   return (0);
}
static int func4d3(double value)
{
   if (getCSparIndex("d3") == -1)
      d3_index = get_curarrayindex();
   else
      d3_index = (int) ( (d3 - d3_init) / inc3D + 0.5);

   DPRINT(DPRTLEVEL,"func for d3 called\n");
   return (0);
}
static int func4d4(double value)
{
   if (getCSparIndex("d4") == -1)
      d4_index = get_curarrayindex();
   else
      d4_index = (int) ( (d4 - d4_init) / inc4D + 0.5);
   DPRINT(DPRTLEVEL,"func for d4 called\n");
   return (0);
}
static
int func4phase1(value)
double          value;
{
  phase1 = (int) sign_add(value,0.005);
   DPRINT(DPRTLEVEL,"func for phase1 called\n");
   return (0);
}
static
int func4phase2(value)
double          value;
{
  phase2 = (int) sign_add(value,0.005);
   DPRINT(DPRTLEVEL,"func for phase2 called\n");
   return (0);
}
static
int func4phase3(value)
double          value;
{
  phase3 = (int) sign_add(value,0.005);
   DPRINT(DPRTLEVEL,"func for phase3 called\n");
   return (0);
}
static 
int func4satmode(value) 
char           *value;
{ 
   strcpy(satmode, value); 
   DPRINT1(DPRTLEVEL,"func for satmode called value='%s'\n", value); 
   return (0);
}

/*---------------------------------------------------
|  Auto structure parameter  functions
+--------------------------------------------------*/
static
int func4rcgain(value)
double          value;
{
   gainactive = TRUE; /* non arrayable */
   return (0);
}

/*----------------------------------------------------
|  initglblstruc - load up a global structure element
+----------------------------------------------------*/
static
void initglblstruc(index, name, glbladdr, function)
int             index;
char           *name;
double         *glbladdr;
int             (*function) ();

{
   if (index >= MAXGLOBALS)
   {
      fprintf(stdout, "initglblstruc: index: %d beyond limit %d\n", index, MAXGLOBALS);
      psg_abort(1);
   }
   strcpy(glblvars[index].gnam, name);
   glblvars[index].glblvalue = glbladdr;
   glblvars[index].funcptr = function;
}

/*-----------------------------------------------------------------------
|  elemvalues/1
|       returns the number of values a particlur looping element has.
|	If the element is out of bounds then a negative one is return.
|
|                       Author Greg Brissey   6/5/87
+------------------------------------------------------------------------*/
int elemvalues(int elem)
{
   char           *name;
   int             ret;
   vInfo           varinfo;	/* variable information structure */

   if ((elem < 1) || (elem > arrayelements))
   {
      fprintf(stdout,
	      "Warning, elemvalues() called with an invalid element: %d\n",
	      elem);
      return (-1);
   }

   name = lpel[elem - 1]->lpvar[0];
   if ( (ret = P_getVarInfo(CURRENT, name, &varinfo)) )
   {
      text_error("Cannot find the variable: '%s'", name);
      if (bgflag)
	 P_err(ret, name, ": ");
      psg_abort(1);
   }
   return ((int) varinfo.size);	/* # of values variable has */
}

/*-----------------------------------------------------------------------
|  elemindex/2
|       returns which value the element is on for given FID number.
|	If the element is out of bounds then a negative one is return.
|
|                       Author Greg Brissey   6/5/87
+------------------------------------------------------------------------*/
int elemindex(int fid, int elem)
{
   int             i,
                   nvalues,
                   iteration,
                   prior_iter,
                   index;

   if ((elem < 1) || (elem > arrayelements))
   {
      fprintf(stdout,
	      "Warning, elemindex() called with an invalid element: %d\n",
	      elem);
      return (-1);
   }
   fid -= 1;			/* calc requires fid number go from 0 up, not
				 * 1 up */
   prior_iter = fid;

   /*
    * The value an element is on is the mod of the number of cycles the prior
    * element has been through
    */
   for (i = arrayelements; elem <= i; i--)
   {
      nvalues = elemvalues(i);
      iteration = prior_iter / nvalues;
      index = (prior_iter % nvalues) + 1;
      if (bgflag > 1)
	 fprintf(stderr, "elemindex(): elem; %d, on value: %d\n", i, index);
      prior_iter = iteration;
   }
   return (index);
}
/*-------------------------------------------------------
| initglobalptrs() - initialize the global parameter
|		     structure, this allows arrayPS()
|		     to update the global parameter
|		     directly, thereby not having to call
|		     initparms() for each fid
|			Author: Greg Brissey 6/2/89
+-------------------------------------------------------*/
void initglobalptrs()
{
   int index;

   index = 0;
   initglblstruc(index++, "d1", &d1, 0L);
   initglblstruc(index++, "d2", &d2, func4d2);
   initglblstruc(index++, "d3", &d3, func4d3);
   initglblstruc(index++, "d4", &d4, func4d4);
   initglblstruc(index++, "pw", &pw, 0L);
   initglblstruc(index++, "p1", &p1, 0L);
   initglblstruc(index++, "pw90", &pw90, 0L);
   initglblstruc(index++, "pad", &pad, func4pad_temp);
   initglblstruc(index++, "rof1", &rof1, 0L);
   initglblstruc(index++, "rof2", &rof2, 0L);
   initglblstruc(index++, "hst", &hst, 0L);
   initglblstruc(index++, "alfa", &alfa, 0L);
   initglblstruc(index++, "sw", &sw, 0L);
   initglblstruc(index++, "nf", &nf, 0L);
   initglblstruc(index++, "np", &np, 0L);
   initglblstruc(index++, "nt", &nt, func4nt);
   initglblstruc(index++, "sfrq", &sfrq, func4sfrq);
   initglblstruc(index++, "fb", &fb, 0L);
   initglblstruc(index++, "bs", &bs, 0L);
   initglblstruc(index++, "tof", &tof, func4tof);
   initglblstruc(index++, "gain", &gain, func4rcgain);
   initglblstruc(index++, "tpwr", &tpwr, 0L);
      initglblstruc(index++, "tpwrf", &tpwrf, 0L);
   initglblstruc(index++, "phase", 0L, func4phase1);
   initglblstruc(index++, "phase2", 0L, func4phase2);
   initglblstruc(index++, "phase3", 0L, func4phase3);

/* status control parameters */
   initglblstruc(index++, "hs", 0L, func4hs);
   initglblstruc(index++, "homo", 0L, func4homo);


   initglblstruc(index++, "wshim", 0L, func4wshim);

   initglblstruc(index++, "pwx", &pwx, 0L);
   initglblstruc(index++, "pwxlvl", &pwxlvl, 0L);
   initglblstruc(index++, "tau", &tau, 0L);
   initglblstruc(index++, "satdly", &satdly, 0L);
   initglblstruc(index++, "satfrq", &satfrq, 0L);
   initglblstruc(index++, "satpwr", &satpwr, 0L);
   initglblstruc(index++, "satmode", 0L, func4satmode);

   /* MAX is 135 at present, change MAXGLOBALS if needed */

}
