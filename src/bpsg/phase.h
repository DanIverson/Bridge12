/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
#ifndef PHASE_H
#define PHASE_H

#define PH0   0
#define PH90  1
#define PH180 2
#define PH270 3

#define ZERO  0
#define ONE   1
#define TWO   2
#define THREE 3

#define V0    THREE
#define V1    V0+1
#define V2    V0+2
#define V3    V0+3
#define V4    V0+4
#define V5    V0+5
#define V6    V0+6
#define V7    V0+7
#define V8    V0+8
#define V9    V0+9
#define V10   V0+10

#define T0    V10
#define T1    T0+1
#define T2    T0+2
#define T3    T0+3
#define T4    T0+4
#define T5    T0+5
#define T6    T0+6
#define T7    T0+7
#define T8    T0+8
#define T9    T0+9
#define T10   T0+10

#define OPH   T10+1

extern int getRecPhase(int index);

#endif
