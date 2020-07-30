/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#ifndef STANDARD_H
#define STANDARD_H
/*-------------------------------------------------------------------------
|  Basic Include Files all pulse sequences need.
+-------------------------------------------------------------------------*/
#ifndef DPS

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "abort.h"
#include "acqparms.h"
#include "macros.h"
#include "group.h"
#include "pvars.h"
#include "cps.h"
#include "b12funcs.h"
#include "phase.h"

#define ALL	0

extern int t1, t2, t3, t4, t5, t6, t7, t8, t9, t10;
extern int	rcvroff_flag,
		ap_ovrride;
extern int      initializeSeq;
extern double	parmult(int tree, const char *name);
extern int      isarry(char *var);
extern void     dps_show(char *nm, ...);
extern void     dps_on(void);
extern void     dps_off(void);
extern void     dps_skip(void);

extern void     dps_loadtable(char *filename);
extern void     dps_simgradpulse(char *name, int code, int dcode, double level, double width,
                     char *s1, char *s2);
extern void     dps_simshapedgradient(char *name,int code,int dcode,char *pat,
                   double  level,double width,int loop,int wait,
                   char *s1,char *s2,char *s3,char *s4,char *s5);
extern void     dps_simshapedgradpulse(char *name,int code,int dcode,char *pat,
                   int h, int w, char *s1,char *s2,char *s3);

extern int      dps_create_offset_list(char *name, int code, int dcode, double parray[],
                     int v1, int v2, int v3,
                     char *s1, char *s2, char *s3, char *s4);
extern int      dps_create_rotation_list(char *name, int code, double *angle_set,
                    int num_sets);
extern int      dps_create_angle_list(char *name, int code, double *angle_set,
                    int num_sets);

extern void     dps_magradpulse(char *name, int code, int dcode,
                    double level, double width, double theta, double phi,
                    char *s1, char *s2, char *s3, char *s4);
extern int      dps_poffset_list(char *name, int code, int chan, double parray[],
                    double f1, double f2, double f3, int v1, int v2, char *s1,
                    char *s2, char *s3, char *s4, char *s5, char *s6);

extern int      dps_create_delay_list(char *name, int code, int dcode, 
                    double parray[], int v1, int v2, char *s1, char *s2, char *s3); 

extern int      dps_create_freq_list(char *name, int code, int dcode, double parray[],
                     int v1, int v2, int v3, char *s1, char *s2, char *s3,
                     char *s4);
extern int      dps_create_offset_list(char *name, int code, int dcode, double parray[],
                     int v1, int v2, int v3, char *s1, char *s2, char *s3, 
                     char *s4);

extern void     dps_magradient(char *name, int code, int dcode, double level,
                     double theta, double phi, char *s1, char *s2, char *s3);

extern void     dps_simgradient(char *name, int code, int dcode, double level,
                     char *s1);
extern void     dps_mashapedgradient(char *name, int code,int dcode,char *pat,
                     double level,double width,double theta,double phi,int loop,
                     int wait,char *s1,char *s2,char *s3,char *s4,char *s5,char *s6,char *s7);

extern void     dps_mashapedgradpulse(char *name,int code,int dcode,char * pat,
                     double level,double width,double theta,double phi,char *s1,
                     char *s2,char *s3,char *s4,char *s5);
extern int     dps_initRFGroupPulse(char *name, int code, int dcode, double pw,
                    char *pat, char mode, double cpwr, double fpwr,double sphase,
                    double *fa, int num,
                    char *s1,char *s2,char *s3,char *s4,char *s5,char *s6,
                    char *s7, char *s8);
extern void     dps_GroupPulse(char *name, int code,int dcode, int grpId,
                    double rof1, double rof2, int vphase, int vselect,
                    char *s1,char *s2,char *s3,char *s4,char *s5);
extern void     dps_TGroupPulse(char *name, int code,int dcode, int grpId,
                    double rof1, double rof2, int vphase, int vselect,
                    char *s1,char *s2,char *s3,char *s4,char *s5);
extern void     dps_modifyRFGroupFreqList(char *name, int code,int dcode, 
                    int grpId, int num, double *fa,
                    char *s1, char *s2, char *s3);
extern int    dps_shapelist(char *name, int code, int dcode, char *pattern,
                    double width, double *listarray, double nsval, double frac,
                    char mode,
                    char *s1, char *s2, char *s3, char *s4, char *s5, char *s6);

extern void    dps_shapedpulselist(char *name, int code, int dcode, int listId,
                    double width, int rtvar, double rof1, double rof2, char mode,
                    int vvar,
                    char *s1, char *s2, char *s3, char *s4, char *s5, char *s6,
                    char *s7);

extern void     initparms_sis();
extern int      S_getarray (const char *parname, double array[], int arraysize);
extern void     g_setExpTime(double val);
extern void     setRcvrPhaseStep(double step);

#else

#include "macros.h"

#endif

extern int DPSprint(double, const char *, ...);
extern int DPStimer(int code, int subcode, int vnum, int fnum, ...);
extern double nucleus_gamma(int device);


#undef TODEV
#undef DODEV
#undef DO2DEV
#undef DO3DEV
#undef DO4DEV
#define TODEV OBSch
#define DODEV DECch
#define DO2DEV DEC2ch
#define DO3DEV DEC3ch
#define DO4DEV DEC4ch

#endif
