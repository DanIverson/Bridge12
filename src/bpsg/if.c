#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <stdint.h>
#include "vnmrsys.h"
#include "acqparms.h"

extern void readConf(int index, int *radio);
extern int bgflag;

extern int B12_BoardNum;
extern int B12_BypassFIR;
extern double B12_ADC;

#define NARROW_BW 0x0008
#define BYPASS_FIR 0x0100

static int 
round_int(double value)
{
    return (int)floor(value + 0.5);
}

int ovj_setup_filters (double spectral_width, int scan_repetitions, int cmd,
                       int radio, double clock)
{
  if (bgflag) fprintf(stderr,"spectral_width %f, repetitions %d\n",	 spectral_width, scan_repetitions);
  if (bgflag) fprintf(stderr,"radio %d, clock %g\n", radio, clock);


  int bypass_fir = cmd;
  int use_narrow_bw = 0;
  
  // RPG has limited options
  if (radio == 2)
    {
      bypass_fir = 1;
      use_narrow_bw = 0;
    }

  int fir_dec_amount;
  if (bypass_fir)
    {
      fir_dec_amount = 1;
      if (bgflag) fprintf(stderr,"bypassing FIR filter\n");
    }
  else
    {
      fir_dec_amount = 8;
    }

  int cic_stages;		// Number of stages in cic filter
  if (use_narrow_bw)
    {
      cic_stages = 3;
      if (bgflag) fprintf(stderr,"Using narrow CIC bandwidth option\n");
    }
  else
    {
      cic_stages = 1;
    } 
  // Set CIC decimation amount based on the FIR dec. amount to achieve desired SW
  int cic_dec_amount = round_int(clock * 1000.0 / (spectral_width * (double) fir_dec_amount));	// x 1000 because clock is internally stored as GHz
  if (bgflag) fprintf(stderr,"cic_dec_amount %d\n", cic_dec_amount);

  // filter parameters        
  int fir_shift_amount;		// Will hold amount to shift right the output of the fir filter
  int cic_shift_amount;		// Will hold amount to shift right the output of the cic filter

  int cic_m = 1;		// M value for cic filter

  // Calculate the maximum bit growth due to averaging 
  int average_bit_growth =
    (int) (ceil (log ((double) scan_repetitions) / log (2.0)));
  if (bgflag) fprintf(stderr,"average_bit_growth %d\n", average_bit_growth);

  // Figure out what is the worst case bit growth inside the CIC filter
  int cic_bit_growth =
    (int) (ceil
	   ((double) cic_stages * log ((double) (cic_m * cic_dec_amount)) /
	    log (2.0)));
  if (bgflag) fprintf(stderr,"cic bit growth is %d\n", cic_bit_growth);
  //CIC has 28 bit input, 35 bit output
  cic_shift_amount = cic_bit_growth + 28 - 35;
  // If bypassing the FIR filter, need to account for bit growth of averaging here
  if (bypass_fir)
    {
      if (bgflag) fprintf(stderr,"taking care of average bit growth after CIC filter\n");
      cic_shift_amount += average_bit_growth;
    }
  if (cic_shift_amount < 0)
    {
      cic_shift_amount = 0;
    }
  if (bgflag) fprintf(stderr,"cic_shift_amount %d\n", cic_shift_amount);

  int num_taps = 131;
  int fir_bit_growth = 21;

  // FIR has 35 bit input, 32 bit output
  fir_shift_amount = 35 + fir_bit_growth - 32;
  if (!bypass_fir)
    {
      if (bgflag) fprintf(stderr,"taking care of average bit growth after FIR filter\n");
      fir_shift_amount += average_bit_growth;
    }
  if (fir_shift_amount < 0)
    {
      fir_shift_amount = 0;
    }
  if (bgflag) fprintf(stderr,"fir_shift_amount %d\n", fir_shift_amount);

  // The FIR filter requires num_taps/2 + 5 clock cycles to process each
  // point of data. Because of this, the CIC filter must decimate by at least
  // the much or the FIR filter will not be able to keep up.
  int min_dec_amount = num_taps / 2 + 5;
  
  if (cic_dec_amount < min_dec_amount && !bypass_fir)
    {
      if (bgflag) fprintf(stderr,"limiting cic_dec_amount to a minimum of %d (was %d)\n",
	 min_dec_amount, cic_dec_amount);
      cic_dec_amount = min_dec_amount;
    }
  // 8 is the minimum decimation no matter what
  if (cic_dec_amount < 8)
    {
      if (bgflag) fprintf(stderr,"limiting cic_dec_amount to 8 (was %d)\n",
	     cic_dec_amount);
      cic_dec_amount = 8;
    }

  return fir_dec_amount * cic_dec_amount;
}

double calc_sw(double sweepWidth, int scans)
{
   int radio;

   readConf(B12_BoardNum, &radio);
   if (bgflag)
      fprintf(stderr,"RadioProcess type %d\n", radio);
   int dec_amount = ovj_setup_filters (sweepWidth*1e-6, scans,
                      B12_BypassFIR, radio, B12_ADC);
   if (bgflag)
      fprintf(stderr,"dec_amount= %d\n", dec_amount);
   if (dec_amount <= 0)
      fprintf(stderr,"\n\nError: Invalid data returned from ovj_setup_filters(). Please check your board.\n\n");
   double ADC_frequency_kHz = B12_ADC * 1000;
   double actual_sw = ADC_frequency_kHz *1e6 / (double) dec_amount;
   if (bgflag)
      fprintf(stderr,"Orig sw= %10.4f  actual sw = %10.4f\n", sw, actual_sw);
   return(actual_sw);
}

