/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
#ifndef PSG_H
#define PSG_H

extern void check_for_abort();
extern void pulse(double time, int phase);
extern void offset(double frequency );
extern void acquire(double pts, double dwell );
extern void power(double amp );

#endif
