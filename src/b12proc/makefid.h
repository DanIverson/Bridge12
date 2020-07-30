#include <math.h>

void calc_FID(int *rdata, int *idata, double freq, double amp, double decay, double phase, double dwell, int npnts)
{
   int i;
   double arg;
   double time;
   int *rptr;
   int *iptr;
   double ph;
   double eval;

   rptr = rdata;
   iptr = idata;
//   diagMessage("calc_FID\n");

// fprintf(stderr,"calc_FID2: freq= %g, amp= %g, decay= %g, dwell= %g, npnts= %d\n", freq, amp, decay, dwell, npnts);
   ph = (phase/360.0) * 2.0 * M_PI;
   for (i=0; i < npnts; i++)
   {
      time = i * dwell;
      arg = time*freq*2.0*M_PI + ph;
      eval = amp * exp(-time/decay);
//      diagMessage("%f   %f\n", eval * cos(arg), -eval * sin(arg) );
      *rptr++ = (int) (eval * cos(arg));   // calc_FID uses += here
      *iptr++ = (int) (-eval * sin(arg));  // calc_FID uses += here
   }
#ifdef XXX
   rptr = rdata;
   iptr = idata;
   for (i=0; i < npnts/2; i++)
   {
      fprintf(stderr,"%d   %d\n", *rptr, *iptr);
      rptr++;
      iptr++;
   }
#endif
}


