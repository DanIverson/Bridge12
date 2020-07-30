#include <stdio.h>
#include "acqparms.h"
#include "phase.h"

extern int bgflag;
extern int t1, t2, t3, t4, t5, t6, t7, t8, t9, t10;


struct phaseTable {
   int inUse;
   int elems;
   int phase[32];
};

static struct phaseTable table[OPH+1];

void printPhaseTable()
{
   int i,j;
   for (i=ZERO; i <= OPH; i++)
   {
      fprintf(stderr,"table[%d] elems= %d inUse= %d\n",
                      i, table[i].elems, table[i].inUse);
      for (j=0; j< table[i].elems; j++)
         fprintf(stderr,"   %d", table[i].phase[j]);
      fprintf(stderr,"\n");
   }
}

void initPhaseTables()
{
   int i;

   for (i=ZERO; i <= OPH; i++)
   {
      table[i].inUse = 0;
      table[i].elems = 1;
      table[i].phase[0] = 0;
   }
   table[ONE].phase[0] = 1;
   table[TWO].phase[0] = 2;
   table[THREE].phase[0] = 3;
   if (cpflag == 0)
   {
      table[OPH].elems = 4;
      table[OPH].inUse = 1;
      table[OPH].phase[0] = ZERO;
      table[OPH].phase[1] = ONE;
      table[OPH].phase[2] = TWO;
      table[OPH].phase[3] = THREE;
   }
   else
   {
      table[OPH].elems = 1;
      table[OPH].inUse = 1;
      table[OPH].phase[0] = ZERO;
   }
   if (bgflag)
      printPhaseTable();
   zero = ZERO;
   one = ONE;
   two = TWO;
   three = THREE;
   oph = OPH;
   t1 = T1;
   t2 = T2;
   t3 = T3;
   t4 = T4;
   t5 = T5;
   t6 = T6;
   t7 = T7;
   t8 = T8;
   t9 = T9;
   t10 = T10;
}

int getPhase(int tab, int index)
{
   int tIndex;

   if ( (tab < ZERO) || (tab > OPH) )
      return(0);
   tIndex = index % table[tab].elems;
   return( table[tab].phase[tIndex] );
}

void settable(int tab, int num, int vals[])
{
   int i;

   table[tab].elems = num;
   for (i=0; i < num; i++)
      table[tab].phase[i] = vals[i];
   if (bgflag)
   {
      fprintf(stderr,"setTable %d with %d elems\n",tab,num);
      printPhaseTable();
   }
}

void setPhaseInUse(int index)
{
   table[index].inUse = 1;
}

int maxPhaseCycle()
{
   int i;
   int max;

   max = table[OPH].elems;

   for (i=T1; i < OPH; i++)
   {
      if ( table[i].inUse && ( table[i].elems > max ) )
         max = table[i].elems;
   }
   return(max);
}

// int getRecPhase(int index, int *re, int *im, int *swap)
int getRecPhase(int index)
{
   int tIndex;

   tIndex = index % table[OPH].elems;
//   *re = recReal[tIndex];
//   *im = recImag[tIndex];
//   *swap = recSwap[tIndex];
   return( table[OPH].phase[tIndex] );
}
