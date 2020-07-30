/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/*
 *  Map MPS status to existing csb entries
 *  rfstatus -> AcqSpinSpan
 *  wgstatus -> AcqSpinAdj
 *  ampstatus -> AcqSpinMax
 *  lockstatus -> AcqSpinActSp
 *  rxpowermv -> AcqSpinAct
 *  txpowermv -> AcqSpinSpeedLimit
 *  amptemp -> AcqVTAct
 *  freq -> AcqSpinSet
 *  power -> AcqSpinProfile
 */

int getMpsRfstatus()
{
   return((int)ExpStatus->csb.AcqSpinSpan);
}

int setMpsRfstatus(int val)
{
   ExpStatus->csb.AcqSpinSpan = (short) val;
   shrmTake(ExpStatusObj);
   gettimeofday( &(ExpStatus->TimeStamp), NULL);
   shrmGive(ExpStatusObj);
   return(0);
}

int getMpsWgstatus()
{
   return((int)ExpStatus->csb.AcqSpinAdj);
}

int setMpsWgstatus(int val)
{
   ExpStatus->csb.AcqSpinAdj = (short) val;
   shrmTake(ExpStatusObj);
   gettimeofday( &(ExpStatus->TimeStamp), NULL);
   shrmGive(ExpStatusObj);
   return(0);
}

int getMpsAmpstatus()
{
   return((int)ExpStatus->csb.AcqSpinMax);
}

int setMpsAmpstatus(int val)
{
   ExpStatus->csb.AcqSpinMax = (short) val;
   shrmTake(ExpStatusObj);
   gettimeofday( &(ExpStatus->TimeStamp), NULL);
   shrmGive(ExpStatusObj);
   return(0);
}

int getMpsLockstatus()
{
   return((int)ExpStatus->csb.AcqSpinActSp);
}

int setMpsLockstatus(int val)
{
   ExpStatus->csb.AcqSpinActSp = (short) val;
   shrmTake(ExpStatusObj);
   gettimeofday( &(ExpStatus->TimeStamp), NULL);
   shrmGive(ExpStatusObj);
   return(0);
}

int getMpsRxpowermv()
{
   return(ExpStatus->csb.AcqSpinAct);
}

int setMpsRxpowermv(int val)
{
   ExpStatus->csb.AcqSpinAct = val;
   shrmTake(ExpStatusObj);
   gettimeofday( &(ExpStatus->TimeStamp), NULL);
   shrmGive(ExpStatusObj);
   return(0);
}

int getMpsTxpowermv()
{
   return(ExpStatus->csb.AcqSpinSpeedLimit);
}

int setMpsTxpowermv(int val)
{
   ExpStatus->csb.AcqSpinSpeedLimit = val;
   shrmTake(ExpStatusObj);
   gettimeofday( &(ExpStatus->TimeStamp), NULL);
   shrmGive(ExpStatusObj);
   return(0);
}

int getMpsAmptemp()
{
   return(ExpStatus->csb.AcqVTAct);
}

int setMpsAmptemp(int val)
{
   ExpStatus->csb.AcqVTAct = val;
   shrmTake(ExpStatusObj);
   gettimeofday( &(ExpStatus->TimeStamp), NULL);
   shrmGive(ExpStatusObj);
   return(0);
}

int getMpsFreq()
{
   return(ExpStatus->csb.AcqSpinSet);
}

int setMpsFreq(int val)
{
   ExpStatus->csb.AcqSpinSet = val;
   shrmTake(ExpStatusObj);
   gettimeofday( &(ExpStatus->TimeStamp), NULL);
   shrmGive(ExpStatusObj);
   return(0);
}

int getMpsPower()
{
   return(ExpStatus->csb.AcqSpinProfile);
}

int setMpsPower(int val)
{
   ExpStatus->csb.AcqSpinProfile = val;
   shrmTake(ExpStatusObj);
   gettimeofday( &(ExpStatus->TimeStamp), NULL);
   shrmGive(ExpStatusObj);
   return(0);
}
