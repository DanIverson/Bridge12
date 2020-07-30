/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#ifndef MPSSTAT_H
#define MPSSTAT_H

extern int getMpsRfstatus(void);
extern int setMpsRfstatus(int val);
extern int getMpsWgstatus(void);
extern int setMpsWgstatus(int val);
extern int getMpsAmpstatus(void);
extern int setMpsAmpstatus(int val);
extern int getMpsLockstatus(void);
extern int setMpsLockstatus(int val);
extern int getMpsRxpowermv(void);
extern int setMpsRxpowermv(int val);
extern int getMpsTxpowermv(void);
extern int setMpsTxpowermv(int val);
extern int getMpsAmptemp(void);
extern int setMpsAmptemp(int val);
extern int getMpsFreq(void);
extern int setMpsFreq(int val);
extern int getMpsPower(void);
extern int setMpsPower(int val);

#endif
