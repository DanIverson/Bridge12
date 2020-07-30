/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
#ifndef B12FUNCS_H
#define B12FUNCS_H

#define BNC0 0
#define BNC1 1
#define MIN_DELAY 70e-9

extern void acquire(double datapnts, double dwell );
extern void delay(double time);
extern void microwavedelay(double duration);
extern void offset(double frequency );
extern void power(double amp );
extern void pulse(double time, int phase);
extern void rgpulse(double time, int phase, double rg1, double rg2);
extern void status(int state);
extern void mpsTune(double freq, double width, double power);
extern void mTune(double freq, double width);

extern int bnc;


/* --- sequence status constants --- */

#define A 0
#define B 1
#define C 2
#define D 3
#define E 4
#define F 5
#define G 6
#define H 7
#define I 8
#define J 9
#define K 10
#define L 11
#define M 12
#define N 13
#define O 14
#define P 15
#define Q 16
#define R 17
#define S 18
#define T 19
#define U 20
#define V 21
#define W 22
#define X 23
#define Y 24
#define Z 25


#endif
