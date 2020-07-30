/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#ifndef DPS

#define PULSE_WIDTH		1
#define PULSE_PRE_ROFF		2
#define PULSE_POST_ROFF		3
#define PULSE_DEVICE		4
#define PULSE_PHASE		5
#define PULSE_PHASE_TABLE	6

#define DELAY_TIME		21

#define OFFSET_DEVICE		31
#define OFFSET_FREQ		32

#define RTDELAY_MODE		40
#define RTDELAY_INCR		41
#define RTDELAY_TBASE		42
#define RTDELAY_COUNT		43
#define RTDELAY_HSLINES		44

#define FREQSWEEP_CENTER	50
#define FREQSWEEP_WIDTH		51
#define FREQSWEEP_NP		52
#define FREQSWEEP_MODE		53
#define FREQSWEEP_INCR		54
#define FREQSWEEP_DEVICE	55

#define SLIDER_LABEL		101
#define SLIDER_SCALE		102
#define SLIDER_MAX		103
#define SLIDER_MIN		104
#define SLIDER_UNITS		105

#define TYPE_PULSE		111
#define TYPE_DELAY		111

#define TYPE_TOF		121
#define TYPE_DOF		122
#define TYPE_DOF2		123
#define	TYPE_DOF3		124

#define TYPE_FREQSWPWIDTH	131
#define TYPE_FREQSWPCENTER	132

#define TYPE_RTPARAM		141

#define  DELAY1 0
#define  DELAY2 1
#define  DELAY3 2
#define  DELAY4 3
#define  DELAY5 4

#define NSEC 1
#define USEC 2
#define MSEC 3
#define SEC  4
/* Note: defines duplicated in acodes.h */
#define         TCNT 1
#define         HSLINE 2
#define         TCNT_HSLINE 3
#define         TWRD 4
#define         TWRD_HSLINE 5
#define         TWRD_TCNT 6

/*------ SISCO defines	---------------------------------------*/
#define MAX_POWER		127.0

/* ADC Overflow Checking Keywords for ADC_overflow_check() */
#define ADC_CHECK_ENABLE	201	/* Enable/Disable checking */
#define ADC_CHECK_NPOINTS	202	/* Number of points to check */
#define ADC_CHECK_OFFSET	203	/* Number of points to skip at	*/
					/*  beginning of buffer		*/

/* Round to nearest integer--for internal use */
#define IRND(x)		((x) >= 0 ? ((int)((x)+0.5)) : (-(int)(-(x)+0.5)))
/* Round to nearest positive integer */
#define URND(x)		((x) > 0 ? ((unsigned)((x)+0.5)) : 0)

/*------  End SISCO defines	--------------------------------*/

/*--------------------------------------------------------------
|  To allow lint to check the for proper parameter type & number
|  passed to macros, each macro is define twice. 
|  For lint the macro is defined as its self but in all capital
|  letters. These are then defined in the lintfile.c
|  The incantation is then:
|   cc -DLINT -P -I. s2pul.c
|   lint -DLINT -a -c -h -u -z -v -n -I. s2pul.i llib-lpsg.ln
|
|					Greg Brissey.
+-------------------------------------------------------------*/

#ifndef LINT
/*---------------------------------------------------------------*/
#define initval(value,index)					\
			G_initval((double)(value),index)

#define F_initval(value,index)					\
			G_initval((double)(value),index)

#define	obsblank()						\
			blankon(OBSch)

#define	obsunblank()						\
			blankoff(OBSch)

#define	decblank()						\
			blankon(DECch)

#define	decunblank()						\
			blankoff(DECch)

#define	dec2blank()						\
			blankon(DEC2ch)

#define	dec2unblank()						\
			blankoff(DEC2ch)

#define	dec3blank()						\
			blankon(DEC3ch)

#define	dec3unblank()						\
			blankoff(DEC3ch)

#define obsstepsize(stepval)                                  \
                        stepsize((double)(stepval),OBSch)

#define decstepsize(stepval)                                 \
                        stepsize((double)(stepval),DECch)

#define dec2stepsize(stepval)                                 \
                        stepsize((double)(stepval),DEC2ch)

#define dec3stepsize(stepval)                                 \
                        stepsize((double)(stepval),DEC3ch)

#define obspower(reqpower)                                  \
                        rlpower((double)(reqpower),OBSch)

#define decpower(reqpower)                                 \
                        rlpower((double)(reqpower),DECch)

#define dec2power(reqpower)                                 \
                        rlpower((double)(reqpower),DEC2ch)

#define dec3power(reqpower)                                 \
                        rlpower((double)(reqpower),DEC3ch)

#define dec4power(reqpower)                                 \
                        rlpower((double)(reqpower),DEC4ch)

#define obspwrf(reqpower)					\
                        rlpwrf((double)(reqpower),OBSch)

#define decpwrf(reqpower)					\
                        rlpwrf((double)(reqpower),DECch)

#define dec2pwrf(reqpower)					\
                        rlpwrf((double)(reqpower),DEC2ch)

#define dec3pwrf(reqpower)					\
                        rlpwrf((double)(reqpower),DEC3ch)

#define ipwrf(value,device,string)				\
			G_Power(POWER_VALUE,	value,		\
			      POWER_DEVICE,	device,		\
			      SLIDER_LABEL,	string,		\
			      0)

#define ipwrm(value,device,string)				\
			G_Power(POWER_VALUE,	value,		\
			      POWER_DEVICE,	device,		\
			      SLIDER_LABEL,	string,		\
			      0)


/*  offset is still a subroutine because defining it as a Macro
    wrecks havoc on any program that uses `offset' as a symbol.  */

#define obsoffset( value )			\
			G_Offset( OFFSET_DEVICE,OBSch,		\
				  OFFSET_FREQ,  value,		\
				  0 )

#define obspulse()						\
			G_Pulse(PULSE_DEVICE,	OBSch,		\
			      0)

#define pwrm(rtparam,device)					\
			pwrf(rtparam,device)

#define rlpwrm(value,device)					\
			rlpwrf(value,device)

#define setautoincrement(tname)					\
			Table[tname - BASEINDEX]->auto_inc =	\
			TRUE

#define setdivnfactor(tname, dfactor) 				\
			Table[tname - BASEINDEX]->divn_factor =	\
			dfactor

#define tsadd(t1name, sval, mval)                               \
                        tablesop(TADD, t1name, sval, mval)

#define tsdiv(tname, sval, mval)                                \
                        tablesop(TDIV, tname, sval, mval)
                               
#define tsmult(tname, sval, mval)                               \
                        tablesop(TMULT, tname, sval, mval)
 
#define tssub(tname, sval, mval)                                \
                        tablesop(TSUB, tname, sval, mval)
 
#define ttadd(t1name, t2name, mval)                             \
                        tabletop(TADD, t1name, t2name, mval)

#define ttdiv(t1name, t2name, mval)                             \
                        tabletop(TDIV, t1name, t2name, mval)
 
#define ttmult(t1name, t2name, mval)                            \
                        tabletop(TMULT, t1name, t2name, mval)
 
#define ttsub(t1name, t2name, mval)				\
			tabletop(TSUB, t1name, t2name, mval)

#define xgate(value)						\
			sync_on_event(Ext_Trig,			\
			      SET_DBVALUE, (double)(value),	\
			      0)

#define sp3on()		sp_on(3)
#define sp3off()	sp_off(3)
#define sp4on()		sp_on(4)
#define sp4off()	sp_off(4)
#define sp5on()		sp_on(5)
#define sp5off()	sp_off(5)
 
 
#define is_y(target)   ((target == 'y') || (target == 'Y'))
#define is_w(target)   ((target == 'w') || (target == 'W'))
#define is_r(target)   ((target == 'r') || (target == 'R'))
#define is_c(target)   ((target == 'c') || (target == 'C'))
#define is_d(target)   ((target == 'd') || (target == 'D'))
#define is_p(target)   ((target == 'p') || (target == 'P'))
#define is_q(target)   ((target == 'q') || (target == 'Q'))
#define is_t(target)   ((target == 't') || (target == 'T'))
#define is_u(target)   ((target == 'u') || (target == 'U'))
#define is_porq(target)((target == 'p') || (target == 'P') || (target == 'q') || (target == 'Q'))

#define anyrfwg   ( (is_y(rfwg[0])) || (is_y(rfwg[1])) || (is_y(rfwg[2])) || (is_y(rfwg[3])) )
#define anygradwg ((is_w(gradtype[0])) || (is_w(gradtype[1])) || (is_w(gradtype[2])))
#define anygradcrwg ((is_r(gradtype[0])) || (is_r(gradtype[1])) || (is_r(gradtype[2])))
#define anypfga   ((is_p(gradtype[0])) || (is_p(gradtype[1])) || (is_p(gradtype[2])))
#define anypfgw   ((is_q(gradtype[0])) || (is_q(gradtype[1])) || (is_q(gradtype[2])))
#define anypfg3 ((is_c(gradtype[0])) || (is_c(gradtype[1])) || (is_c(gradtype[2])) || (is_t(gradtype[0])) || (is_t(gradtype[1])) || (is_t(gradtype[2]))) 
#define anypfg3w ((is_d(gradtype[0])) || (is_d(gradtype[1])) || (is_d(gradtype[2])) || (is_u(gradtype[0])) || (is_u(gradtype[1])) || (is_u(gradtype[2])))  
#define anypfg	(anypfga || anypfgw || anypfg3 || anypfg3w)
#define anywg  (anyrfwg || anygradwg || anygradcrwg || anypfgw || anypfg3w)

#define iscp(a)  ((cpflag) ? zero : (a))


#define decshaped_pulse(name, width, phase, rx1, rx2)		\
	newdec ? genshaped_pulse(name, width, phase, rx1, rx2,	\
			  0.0, 0.0, DECch)			\
		: S_decshapedpulse(name,(double)(width),	\
				phase,(double)(rx1),(double)(rx2))

#define dec2shaped_pulse(name, width, phase, rx1, rx2)		\
	genshaped_pulse(name, width, phase, rx1, rx2,		\
			  0.0, 0.0, DEC2ch)

#define dec3shaped_pulse(name, width, phase, rx1, rx2)		\
	genshaped_pulse(name, width, phase, rx1, rx2,		\
			  0.0, 0.0, DEC3ch)

#define simshaped_pulse(n1, n2, w1, w2, ph1, ph2, r1, r2)	\
	newtrans ? gensim3shaped_pulse(n1, n2, "", w1, w2, 0.0, ph1, ph2, \
			zero, r1, r2,0.0, 0.0, OBSch, DECch, NULL)	\
			 : S_simshapedpulse(n1, n2,(double)(w1),	\
				(double)(w2), ph1, ph2,	\
				(double)(r1),(double)(r2))

#define sim3shaped_pulse(n1, n2, n3, w1, w2, w3, ph1, ph2, ph3, r1, r2)	\
	gensim3shaped_pulse(n1, n2, n3, w1, w2, w3, ph1, ph2,	\
			  ph3, r1, r2, 0.0, 0.0, OBSch, DECch,	\
			  DEC2ch)

#define shapedvpulse(name, width, rtamp, phase, rx1, rx2)		\
	genshaped_rtamppulse(name, (double)(width), rtamp, phase, \
		(double)(rx1),(double)(rx2), 0.0, 0.0, OBSch)			\

#define apshaped_pulse(name, width, phase, tbl1, tbl2, rx1, rx2) \
	gen_apshaped_pulse(name, width, phase, tbl1, tbl2, rx1, rx2, OBSch)

#define apshaped_decpulse(name, width, phase, tbl1, tbl2, rx1, rx2) \
	gen_apshaped_pulse(name, width, phase, tbl1, tbl2, rx1, rx2, DECch)

#define apshaped_dec2pulse(name, width, phase, tbl1, tbl2, rx1, rx2) \
	gen_apshaped_pulse(name, width, phase, tbl1, tbl2, rx1, rx2, DEC2ch)

#define spinlock(   name, pp_90, pp_res, phase, nloops)		\
	genspinlock(name, pp_90, pp_res, phase, nloops, OBSch)

#define decspinlock(name, pp_90, pp_res, phase, nloops)		\
	genspinlock(name, pp_90, pp_res, phase, nloops, DECch)

#define dec2spinlock(name, pp_90, pp_res, phase, nloops)	\
	genspinlock(name, pp_90, pp_res, phase, nloops, DEC2ch)

#define dec3spinlock(name, pp_90, pp_res, phase, nloops)	\
	genspinlock(name, pp_90, pp_res, phase, nloops, DEC3ch)

#define obsprgon(name, pp_90, pp_res)				\
	prg_dec_on(name, pp_90, pp_res, OBSch)

#define decprgon(name, pp_90, pp_res)				\
	prg_dec_on(name, pp_90, pp_res, DECch)

#define dec2prgon(name, pp_90, pp_res)				\
	prg_dec_on(name, pp_90, pp_res, DEC2ch)

#define dec3prgon(name, pp_90, pp_res)				\
	prg_dec_on(name, pp_90, pp_res, DEC3ch)

#define obsprgoff()						\
	prg_dec_off(2, OBSch)

#define decprgoff()						\
	prg_dec_off(2, DECch)

#define dec2prgoff()						\
	prg_dec_off(2, DEC2ch)

#define dec3prgoff()						\
	prg_dec_off(2, DEC3ch)

#define initdelay(incrtime,index) 				\
	G_RTDelay(RTDELAY_MODE,SET_INITINCR,RTDELAY_TBASE,index,\
		  RTDELAY_INCR,incrtime,0)

#define incdelay(multparam,index) \
	G_RTDelay(RTDELAY_MODE,SET_RTINCR,RTDELAY_TBASE,index,	\
		  RTDELAY_COUNT,multparam,0)

#define vdelay(base,rtcnt) \
	G_RTDelay(RTDELAY_MODE,TCNT,RTDELAY_TBASE,(int)(base),	\
		  RTDELAY_COUNT,rtcnt,0)

#define statusdelay(index,delaytime) \
	S_statusdelay((int)(index), (double)(delaytime)) 

/*---------------------------------------------------------------*/
/*- INOVA defines 						-*/
/*---------------------------------------------------------------*/

#define i_power(rtparam,device,string)				\
			G_Power(POWER_RTVALUE,	rtparam,	\
			      POWER_DEVICE,	device,		\
			      POWER_TYPE,	COARSE,		\
			      SLIDER_LABEL,	string,		\
			      0)

#define i_pwrf(value,device,string)				\
			G_Power(POWER_RTVALUE,	rtparam,	\
			      POWER_DEVICE,	device,		\
			      SLIDER_LABEL,	string,		\
			      0)

#define setuserap(value,reg)						\
	setBOB((int)(value),(int)(reg))

#define vsetuserap(rtparam,reg)					\
	vsetBOB(rtparam,(int)(reg))

#define readuserap(rtparam)					\
	vreadBOB(rtparam,3)

#define setExpTime(duration)	g_setExpTime( (double) duration)
/*---------------------------------------------------------------*/
/*- SISCO defines (gradients)					-*/
/*---------------------------------------------------------------*/


#define decshapedpulse(pulsefile,pulsewidth,phaseptr,rx1,rx2)		\
		newdec ? genshaped_pulse(pulsefile,(double)(pulsewidth), \
				phaseptr,(double)(rx1),(double)(rx2),  \
				0.0,0.0,DECch)	\
		       : S_decshapedpulse(pulsefile,(double)(pulsewidth), \
				phaseptr,(double)(rx1),(double)(rx2))

#define simshapedpulse(fno,fnd,transpw,decpw,transphase,decphase,rx1,rx2) \
		newtrans ? gensim2shaped_pulse(fno,fnd,(double)(transpw),	\
				(double)(decpw),transphase,decphase,	\
				(double)(rx1),(double)(rx2),		\
				0.0,0.0,OBSch,DECch) \
			 : S_simshapedpulse(fno,fnd,(double)(transpw),	\
				(double)(decpw),transphase,decphase,	\
				(double)(rx1),(double)(rx2))

#define shapedincgradient(axis,pattern,width,a0,a1,a2,a3,x1,x2,x3,loops,wait) \
		shaped_INC_gradient(axis, pattern, (double)(width), \
			(double)(a0), (double)(a1), (double)(a2), (double)(a3), \
			x1, x2, x3, IRND(loops), IRND(wait))

#define shapedgradient(pulsefile,pulsewidth,gamp0,which,loops,wait_4_me) \
		S_shapedgradient(pulsefile,(double)(pulsewidth),	\
				(double)(gamp0),which,(int)(loops+0.5),	\
				(int)(wait_4_me+0.5))

#define shaped2Dgradient(pulsefile,pulsewidth,gamp0,which,loops,wait_4_me,tag) \
		shaped_2D_gradient(pulsefile,(double)(pulsewidth),	\
				(double)(gamp0),which,(int)(loops+0.5),	\
				(int)(wait_4_me+0.5),(int)(tag+0.5))

#define shapedvgradient(pfile,pwidth,gamp0,gampi,vmult,which,vloops,wait,tag) \
		shaped_V_gradient(pfile,(double)(pwidth),(double)(gamp0), \
				(double)(gampi),vmult,which,vloops,	\
				(int)(wait+0.5),(int)(tag+0.5))

#define phase_encode_shapedgradient(pat,width,stat1,stat2,stat3,step2,vmult2,lim2,ang1,ang2,ang3,vloops,wait,tag)   \
                S_phase_encode_shapedgradient(pat,(double)(width),        \
				(double)(stat1),(double)(stat2),          \
				(double)(stat3),(double)(step2),          \
				vmult2,(double)(lim2),(double)(ang1),     \
				(double)(ang2),(double)(ang3),vloops,     \
				(int)(wait+0.5),(int)(tag+0.5))

#define phase_encode3_gradient(stat1,stat2,stat3,step1,step2,step3,vmult1,vmult2,vmult3,lim1,lim2,lim3,ang1,ang2,ang3) \
                S_phase_encode3_gradient((double)(stat1),(double)(stat2), \
				(double)(stat3),(double)(step1),          \
				(double)(step2),(double)(step3),vmult1,   \
				vmult2,vmult3,(double)(lim1),(double)(lim2), \
				(double)(lim3),(double)(ang1),(double)(ang2), \
				(double)(ang3))

#define phase_encode3_shapedgradient(pat,width,stat1,stat2,stat3,step1,step2,step3,vmult1,vmult2,vmult3,lim1,lim2,lim3,ang1,ang2,ang3,loops,wait) \
                S_phase_encode3_shapedgradient(pat,(double)(width), \
				(double)(stat1),(double)(stat2), \
				(double)(stat3),(double)(step1),          \
				(double)(step2),(double)(step3),vmult1,   \
				vmult2,vmult3,(double)(lim1),(double)(lim2), \
				(double)(lim3),(double)(ang1),(double)(ang2), \
				(double)(ang3), IRND(loops), IRND(wait))

#define position_offset(pos,grad,resfrq,device)                 \
                S_position_offset((double)(pos),(double)(grad), \
				(double)(resfrq),(int)(device))

#define position_offset_list(posarray,grad,nslices,resfrq,device,listno,apv1) \
                S_position_offset_list(posarray,(double)(grad),      \
				(double)(nslices),(double)(resfrq),  \
				(int)(device),(int)(listno),apv1)

/*---------------------------------------------------------------*/
/*- SISCO defines (Misc)					-*/
/*---------------------------------------------------------------*/
#define observepower(reqpower)                                  \
                        rlpower((double)(reqpower),OBSch)

#define decouplepower(reqpower)                                 \
                        rlpower((double)(reqpower),DECch)

#define getarray(paramname,arrayname)                           \
			S_getarray(paramname,arrayname,sizeof(arrayname))


/*---------------------------------------------------------------*/
/*- SISCO defines (Imaging Sequence Developement)		-*/
/*---------------------------------------------------------------*/


#define  rotate() \
    rotate_angle(psi,phi,theta,offsetx,offsety,offsetz,gxdelay,gydelay,gzdelay)

#define  rot_angle(psi,phi,theta) \
    rotate_angle(psi,phi,theta,offsetx,offsety,offsetz,gxdelay,gydelay,gzdelay)

#define  poffset(POS,GRAD)  position_offset(POS,GRAD,resto,OBSch)

#define  poffset_list(POSARRAY,GRAD,NS,APV1) \
    position_offset_list(POSARRAY,GRAD,NS,resto,OBSch,0,APV1)

#define pe_gradient(STAT1,STAT2,STAT3,STEP2,VMULT2) \
    phase_encode_gradient(STAT1,STAT2,STAT3,STEP2,VMULT2,nv/2,psi,phi,theta)

#define pe_shapedgradient(PAT,WIDTH,STAT1,STAT2,STAT3,STEP2,VMULT2,WAIT,TAG) \
    phase_encode_shapedgradient(PAT,WIDTH,STAT1,STAT2,STAT3,STEP2,VMULT2, \
        nv/2,psi,phi,theta,one,WAIT,TAG)

#define pe2_gradient(STAT1,STAT2,STAT3,STEP2,STEP3,VMULT2,VMULT3)   \
    phase_encode3_gradient(STAT1, STAT2, STAT3, 0.0, STEP2, STEP3,  \
        zero, VMULT2, VMULT3, 0.0, nv/2, nv2/2, psi, phi, theta)

#define pe3_gradient(STAT1,STAT2,STAT3,STEP1,STEP2,STEP3,VMULT1,VMULT2,VMULT3) \
    phase_encode3_gradient(STAT1, STAT2, STAT3, STEP1, STEP2, STEP3, \
        VMULT1, VMULT2, VMULT3, nv/2, nv2/2, nv3/2, psi, phi, theta)

#define pe2_shapedgradient(PAT,WIDTH,STAT1,STAT2,STAT3,STEP2,STEP3,VMULT2,VMULT3)   \
    phase_encode3_shapedgradient(PAT, WIDTH, STAT1, STAT2, STAT3, 0.0, \
			STEP2, STEP3, zero, VMULT2, VMULT3,  \
        		0.0, nv/2, nv2/2, psi, phi, theta, 1.0, WAIT)

#define pe3_shapedgradient(PAT,WIDTH,STAT1,STAT2,STAT3,STEP1,STEP2,STEP3,VMULT1,VMULT2,VMULT3) \
    phase_encode3_shapedgradient(PAT, WIDTH ,STAT1, STAT2, STAT3, \
			STEP1, STEP2, STEP3, VMULT1, VMULT2, VMULT3, \
        		nv/2, nv2/2, nv3/2, psi, phi, theta, 1.0, WAIT)

#define init_rfpattern(pattern_name,pulse_struct,steps)  \
        init_RFpattern(pattern_name, pulse_struct, IRND(steps))

#define init_decpattern(pattern_name,pulse_struct,steps)  \
        init_DECpattern(pattern_name, pulse_struct, IRND(steps))

#define init_gradpattern(pattern_name,pulse_struct,steps)  \
        init_Gpattern(pattern_name, pulse_struct, IRND(steps))

/*---------------------------------------------------------------*/
/*- End SISCO defines 						-*/
/*---------------------------------------------------------------*/

#ifndef DPS
extern void incr(int a);
extern void decr(int a);
extern void assign(int a, int b);
extern void dbl(int a, int b);
extern void hlv(int a, int b);
extern void modn(int a, int b, int c);
extern void mod4(int a, int b);
extern void mod2(int a, int b);
extern void add(int a, int b, int c);
extern void sub(int a, int b, int c);
extern void mult(int a, int b, int c);
extern void divn(int a, int b, int c);
extern void orr(int a, int b, int c);
extern void G_initval(double value, int index);
extern void settable(int tablename, int numelements, int tablearray[]);
extern void getelem(int tablename, int indxptr, int dstptr);
extern void getTabSkip(int tablename, int indxptr, int dstptr);

extern void starthardloop(int count);
extern void endhardloop();
extern void loop(int count, int counter);
extern void endloop(int counter);
extern void ifzero(int rtvar);
extern void ifmod2zero(int rtvar);
extern void elsenz(int rtvar);
extern void endif(int rtvar);
extern int  loadtable(char *infilename);
extern void tablesop(int operationtype, int tablename, int scalarval, int modval);
extern void tabletop(int operationtype, int table1name, int table2name, int modval);

extern void	setprgmode(),
		prg_dec_off();
extern void     zgradpulse(double gval, double gdelay);
extern double gen_shapelistpw(char *, double, int);
extern double gen_poffset(double, double, int);
extern double getTimeMarker(void);
extern double nuc_gamma(void);
extern void parallelstart(const char *chanType);
extern double parallelend();
#endif

/*---------------------------------------------------------------*/
/*---------------------------------------------------------------*/
#else
/*     LINT defines, So that Lint can find miss match parameters */
/*---------------------------------------------------------------*/

#endif

#endif
