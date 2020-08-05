 
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <stdarg.h>
#include "inc/spinapi.h"
#include "shrstatinfo.h"

extern void initError( char *infoPath );
extern void dataError( char *infoPath, int ct, int ix );
extern void saveData( int ct, int ix, int arraydim,
                      int *real, int *imag, char *info);
extern void writeConf(const char *confPath);
extern void showConf(const char *confPath);
extern void abortExp( char *infoPath, char *code, int ct, int ix );
extern void sleepMilliSeconds(int msecs);
extern void endComm();
extern void requestMpsData(double freq, double width, double power,
	        double del, char *dataPath, char *infoPath );
extern void saveBsData( int *real, int *imag, char *infoPath, int ct);
extern void endMpsData();
extern void mtune(char *dataPath, double freq, double width,
	  int pts, char *codePath,
	  int *real, int *imag, char *infoPath);

#define DEBUG

// User friendly names for the phase registers of the cos and sin channels
#define PHASE000    0
#define PHASE090    1
#define PHASE180    2
#define PHASE270    3

// User friendly names for the control bits
#define TX_ENABLE       1       
#define TX_DISABLE      0
#define PHASE_RESET     1
#define NO_PHASE_RESET  0
#define DO_TRIGGER      1
#define NO_TRIGGER      0
#define NO_DATA         0
#define NO_SHAPE        0
#define AMP0            0
#define BLANK_PA        0x00
#define MIN_DELAY       70.0


struct _PSelem {
                 char inst[64];
                 char vals[256];
                 struct _PSelem *next;
              };
typedef struct _PSelem PSelem;

PSelem *PSstart = NULL;

struct _Globals {
                  int debug;
                  int board_number;
                  int board_open;
                  int board_start;
                  int blank_bit;
                  int blank_bit_mask;
                  int bypass_fir;
                  int arraydim;
                  int complex_points;   // number of complex pairs
                  int *real;
                  int *imag;
                  double adc_frequency;
                  double totalTime;
                  char DataDir[512];
                  char InfoFile[512];
                  char CodePath[512];
                };
typedef struct _Globals Globals;

struct _Exps {
                  int nt;
                  int segments;
                  int dec_amount;
                  int loop;
                  int loopCnt;
                  int elem;
                  int phaseReset;
                  int opCode;
                  int opCodeData;
                  int status;
                  double sw_kHz;
                  double actual_sw;
                  double at;
                  double sfrq;
                  double tpwrf;
                  double exp_time;
             };
typedef struct _Exps Exps;


static FILE *debugFile = NULL;
// Constants for receiver phase cycling
static int recReal[4] = {1,1,0,0};
static int recImag[4] = {1,0,0,1};
static int recSwap[4] = {0,1,0,1};

void diagMessage(const char *format, ...)
{
   va_list vargs;
   char    str[2048];

   va_start(vargs, format);
   vsnprintf(str, 2040, format,vargs);
   va_end(vargs);
   if (debugFile == NULL)
   {
      debugFile = fopen("/vnmr/tmp/b12out","w");
   }
   fprintf(debugFile,"%s",str);
   fflush(debugFile);
}

PSelem *newPsElem()
{
   static PSelem *r;

   if ( (r = (PSelem *) malloc(sizeof(PSelem))) == NULL )
   {
      diagMessage("B12proc out of memory\n");
      exit(EXIT_FAILURE);
   }
   r->inst[0] = '\0';
   r->next = NULL;
   return(r);
}

static int readPS(Globals *globals)
{
   PSelem *r;
   FILE *inputFile;
   char inst[64];
   char vals[256];

    /* Open the input file */
   inputFile = fopen(globals->CodePath, "r");
    /* does file exist? */
   if (inputFile == NULL)
   {
      debugFile = fopen("/vnmr/tmp/b12noFile","w");
      diagMessage("B12proc input file %s does not exist\n",globals->CodePath);
      abortExp( & (globals->InfoFile[0]), globals->CodePath, 1, 1);
      PSstart = NULL;
      return(0);
   }
   r =  newPsElem();
   PSstart = r;
   while (fscanf(inputFile,"%s %[^\n]\n",inst,vals) != EOF)
   {
      if (r->inst[0] != '\0')
      {
         r->next =  newPsElem();
         r = r->next;
      }
      strcpy(r->inst,inst);
      strcpy(r->vals,vals);
   }
   fclose(inputFile);

   r = PSstart;
   while ( (r != NULL) && r->inst[0] != '\0')
   {
      diagMessage("%s %s\n",r->inst,r->vals);
      r = r->next;
   }
   return(0);
}

void setGlobalsDefault(Globals *globals, const char *path)
{
   globals->debug = 0;
   globals->board_number = 0;
   globals->board_open = 0;
   globals->board_start = 0;
   globals->blank_bit = 2;
   globals->blank_bit_mask = (1 << globals->blank_bit);
   globals->bypass_fir = 1;
   globals->adc_frequency = 75.0;
   globals->DataDir[0] = '\0';;
   globals->arraydim = 1;
   globals->complex_points = 0;
   globals->totalTime = 0.0;
   globals->real = NULL;
   globals->imag = NULL;
   strcpy(globals->InfoFile, path);
   strcpy(globals->CodePath, path);
   strcat(globals->CodePath,".Code");

}

void printGlobals(Globals *globals)
{
   diagMessage("Globals\n\n");
   diagMessage("  debug:          %d\n",globals->debug);
   diagMessage("  board_number:   %d\n",globals->board_number);
   diagMessage("  board_open:     %d\n",globals->board_open);
   diagMessage("  board_start:    %d\n",globals->board_start);
   diagMessage("  blank_bit:      %d\n",globals->blank_bit);
   diagMessage("  blank_bit_mask: %d\n",globals->blank_bit_mask);
   diagMessage("  bypass_fir:     %d\n",globals->bypass_fir);
   diagMessage("  complex_points: %d\n",globals->complex_points);
   diagMessage("  arraydim:       %d\n",globals->arraydim);
   diagMessage("  adc_frequency:  %g\n",globals->adc_frequency);
   diagMessage("  DataDir:        %s\n",globals->DataDir);
   diagMessage("  InfoFile:       %s\n",globals->InfoFile);
}

void printExps(Exps *exps)
{
   diagMessage("Exps\n\n");
   diagMessage("  nt:            %d\n",exps->nt);
   diagMessage("  segments:      %d\n",exps->segments);
   diagMessage("  dec_amount:    %d\n",exps->dec_amount);
   diagMessage("  sw_kHz:        %g\n",exps->sw_kHz);
   diagMessage("  actual_sw:     %g\n",exps->actual_sw);
   diagMessage("  at:            %g\n",exps->at);
   diagMessage("  sfrq:          %g\n",exps->sfrq);
   diagMessage("  tpwrf:         %g\n",exps->tpwrf);
}

int initSystem(Globals *globals)
{
   int numBoards = pb_count_boards();

   if (numBoards <= 0)
   {
      diagMessage("No RadioProcessor boards were detected in your system.\n");
      return -1;
   }
   if (globals->board_number > numBoards )
   {
      diagMessage("Invalid board number. Use (0-%d).\n", numBoards - 1);
      return -1;
   }
   pb_select_board(globals->board_number);
   if (pb_init())
   {
      diagMessage("Error initializing board: %s\n", pb_get_error ());
      return -1;
   }
   globals->board_open = 1;
   return(0); 
}

void make_shape_data (float *shape_array, void *arg, void (*shapefnc) (float *, void *)){
	shapefnc (shape_array, arg);
}

void shape_sinc (float *shape_data, void *nlobe){
	static double pi = 3.1415926;
	int i;
	int lobes = *((int *) nlobe);
	double x;
	double scale = (double) lobes * (2.0 * pi);
  	
	for (i = 0; i < 1024; i++){
		x = (double) (i - 512) * scale / 1024.0;
		if(x == 0.0){
			shape_data[i] = 1.0;
		}
		else{
			shape_data[i] = sin (x) / x;
		}
	}
}

int configSystem(Globals *globals)
{
   pb_set_defaults ();
   pb_core_clock (globals->adc_frequency);
	
   pb_overflow (1, 0);		//Reset the overflow counters
   pb_scan_count (1);		//Reset scan counter

   // Load the shape parameters
   float shape_data[1024];
   int num_lobes = 3;

   make_shape_data (shape_data, (void *) &num_lobes, shape_sinc);
   pb_dds_load (shape_data, DEVICE_SHAPE);
   return(0);
}
	
int configExp(Globals *globals, Exps *exps)
{
   int cmd = 0;

   pb_set_amp(exps->tpwrf/100.0, 0);
   if (globals->bypass_fir)
      cmd = BYPASS_FIR;
   exps->dec_amount = pb_setup_filters (exps->sw_kHz/1000.0, exps->nt, cmd);
   if (exps->dec_amount <= 0)
      exps->dec_amount = 1;
   exps->actual_sw = (globals->adc_frequency*1000.0) /(double) exps->dec_amount;
   exps->at = (double) globals->complex_points / (exps->actual_sw);
   diagMessage("actual_sw %g adc=%g decimate=%d at=%g\n",
                  exps->actual_sw, globals->adc_frequency,
                  exps->dec_amount, exps->at);

   pb_set_num_points(globals->complex_points);
   pb_set_scan_segments(1);

   pb_start_programming(FREQ_REGS);
   pb_set_freq(exps->sfrq);
   pb_stop_programming();

   //Real Phase
   pb_start_programming (COS_PHASE_REGS);
   pb_set_phase (0.0);
   pb_set_phase (90.0);
   pb_set_phase (180.0);
   pb_set_phase (270.0);
   pb_stop_programming ();
 
   //Imaginary Phase
   pb_start_programming (SIN_PHASE_REGS);
   pb_set_phase (0.0);
   pb_set_phase (90.0);
   pb_set_phase (180.0);
   pb_set_phase (270.0);
   pb_stop_programming ();
 
   //Transmitted Phase
   pb_start_programming (TX_PHASE_REGS);
   pb_set_phase (0.0);
   pb_set_phase (90.0);
   pb_set_phase (180.0);
   pb_set_phase (270.0);
   pb_stop_programming ();

   exps->exp_time = 0.0;
   return(0);
}

int getData(Globals *globals, Exps *exps)
{
   int ct = 0;
   double msecPerCT;

   msecPerCT = 1000.0 * exps->exp_time / exps->nt;
   diagMessage("wait %d ms for exp to finish\n",(int) (exps->exp_time*1000.0));
   globals->totalTime += exps->exp_time;
   while (ct < exps->nt)
   {
       ct++;
       setStatCT(ct);
       if ( access(globals->CodePath, F_OK) )
       {
          abortExp( & (globals->InfoFile[0]), globals->CodePath, ct, exps->elem);
          return(-1);
       }
       sleepMilliSeconds((int) (msecPerCT) );
   }
   while (!(pb_read_status() & STATUS_STOPPED))
   {
      pb_sleep_ms (1);
   }

   if( pb_get_data (globals->complex_points, globals->real, globals->imag) < 0 )
   {
      diagMessage("Failed to retrieve data\n");
      dataError( & (globals->InfoFile[0]), ct, exps->elem);
      return -1;
   }
   diagMessage("save data to %s\n",globals->DataDir);
   return(0);
}

void resetExp(int pbRes, Exps *exps)
{
   if (exps->opCode == LOOP)
      exps->loop = pbRes;
   if (exps->opCode == END_LOOP)
   {
// diagMessage("end_loop exptime= %g loop count= %d\n",exps->exp_time, exps->loopCnt);
      exps->exp_time *= exps->loopCnt;
// diagMessage("end_loop new exptime= %g\n",exps->exp_time);
   }

   exps->phaseReset = NO_PHASE_RESET;
   exps->opCode = CONTINUE;
   exps->opCodeData = NO_DATA;
}

int getMpsData(double del, char *path,  Globals *globals, Exps *exps)
{
   int msec;
   int count;

   // del is in sec
   msec = (int) (del * (double) globals->complex_points * 1000.0) + 100;
   count = msec / 100;
   diagMessage("start getMPS wait %d ms\n", count * 100);
   // wait for tuning to start
   while (count > 0)
   {
      count--;
      sleepMilliSeconds(100);
      if ( access(globals->CodePath, F_OK) )
      {
	 unlink(path);
         diagMessage("getMpsData Exp aborted while waiting\n");
         abortExp( & (globals->InfoFile[0]), globals->CodePath, 1, exps->elem);
//	 if (count)
//            sleepMilliSeconds(100*count);
         return(-1);
      }
   }
   // now wait for data
   count = 100;
   while (count > 0)
   {
      count--;
      if ( ! access(path, F_OK) )
      {
         FILE *fd;
	 int *ptr;
	 int index;
	 int val;

         diagMessage("Tune data present count= %d\n", 100 - count);
         fd = fopen(path,"r");
	 ptr = globals->real;
	 for (index=0; index<globals->complex_points; index++)
         {
            if (fscanf(fd,"%d\n", &val))
               *ptr++ = val;
	 }
	 fclose(fd);
	 unlink(path);
         return(0);
      }
      sleepMilliSeconds(100);
      if ( access(globals->CodePath, F_OK) )
      {
	 unlink(path);
         abortExp( & (globals->InfoFile[0]), globals->CodePath, 1, exps->elem);
         diagMessage("getMpsData Exp aborted after waiting\n");
         return(-1);
      }
   }
   abortExp( & (globals->InfoFile[0]), globals->CodePath, 1, exps->elem);
   diagMessage("getMpsData failed to appear\n");
   return(-1);
}

int main (int argc, char *argv[])
{
   PSelem *r;
   Globals globals;
   Exps exps;
   int pbRes;
	
   // Special cases to handle configuration file
   if (argc == 3)
   {
      if ( ! strcmp(argv[1],"Conf") )
         writeConf(argv[2]);
      if ( ! strcmp(argv[1],"show") )
         showConf(argv[2]);
      if (debugFile != NULL)
         fclose(debugFile);
      exit(EXIT_SUCCESS);
   }

   if (argc == 1)
   {
      diagMessage("%s must be called with Codepath\n", argv[0]);
      if (debugFile != NULL)
         fclose(debugFile);
      exit(EXIT_SUCCESS);
   }

   //
   //Process Arguments
   //
		
   setGlobalsDefault( &globals, argv[1] );
   initExpStatus(0);
   readPS(&globals);
   r = PSstart;
   while ( (r != NULL) && r->inst[0] != '\0')
   {
//      diagMessage("%s %s\n",r->inst,r->vals);
      if ( ! strcmp(r->inst,"DEBUG") )
      {
         globals.debug = atoi(r->vals);
      }
      else if ( ! strcmp(r->inst,"BOARD_NUMBER") )
      {
         globals.board_number = atoi(r->vals);
      }
      else if ( ! strcmp(r->inst,"BLANK_BIT") )
      {
         globals.blank_bit = atoi(r->vals);
         globals.blank_bit_mask = (1 << globals.blank_bit);
      }
      else if ( ! strcmp(r->inst,"BYPASS_FIR") )
      {
         globals.bypass_fir = atoi(r->vals);
      }
      else if ( ! strcmp(r->inst,"NUMBER_POINTS") )
      {
         globals.complex_points = atoi(r->vals) / 2;
         if (globals.real == NULL)
         {
            diagMessage("malloc %d ints\n",globals.complex_points );
            globals.real = (int *)malloc(sizeof(int) * globals.complex_points);
            globals.imag = (int *)malloc(sizeof(int) * globals.complex_points);
         }
      }
      else if ( ! strcmp(r->inst,"ADC_FREQUENCY") )
      {
         globals.adc_frequency = atof(r->vals);
      }
      else if ( ! strcmp(r->inst,"FILE") )
      {
         strcpy(globals.DataDir,r->vals);
      }
      else if ( ! strcmp(r->inst,"ARRAYDIM") )
      {
         globals.arraydim = atoi(r->vals);
      }
      else if ( ! strcmp(r->inst,"PULSEPROG_START") )
      {
         exps.elem= atof(r->vals);
         exps.opCode = CONTINUE;
         exps.status = BLANK_PA;
         resetExp(0, &exps);
         if (exps.elem == 1)
         {
            if (globals.debug)
               printGlobals( &globals);
            if (initSystem( &globals))
            {
               initError( & (globals.InfoFile[0]) );
               break;
            }
            if (globals.debug)
               pb_set_debug(1);
            if (configSystem( &globals))
               break;
         }
         else
         {
            setStatElem(exps.elem);
            setStatCT(1);
         }
         if ( access(globals.CodePath, F_OK) )
         {
               abortExp( & (globals.InfoFile[0]), globals.CodePath, 1, exps.elem);
               break;
         }
      }
      else if ( ! strcmp(r->inst,"RATTN") )
      {
	 char cmd[512];
	 int ret __attribute__((unused));

	 sprintf(cmd,"/vnmr/bin/mcl_RUDAT %s\n",r->vals);
         ret = system(cmd);
      }
      else if ( ! strcmp(r->inst,"POWER") )
      {
         exps.tpwrf= atof(r->vals);
      }
      else if ( ! strcmp(r->inst,"SPECTROMETER_FREQUENCY") )
      {
         exps.sfrq= atof(r->vals);
      }
      else if ( ! strcmp(r->inst,"NUMBER_OF_SCANS") )
      {
         exps.nt= atoi(r->vals);
         exps.segments = 1;
      }
      else if ( ! strcmp(r->inst,"SPECTRAL_WIDTH") )
      {
         exps.sw_kHz= atof(r->vals)/1000.0;
      }
      else if ( ! strcmp(r->inst,"PULSE_ELEMENTS") )
      {
         configExp( &globals, &exps);
         if (globals.debug)
            printExps( &exps);
         pb_start_programming (PULSE_PROGRAM);
      }
      else if ( ! strcmp(r->inst,"PHASE_RESET") )
      {
         exps.phaseReset = PHASE_RESET;
      }
      else if ( ! strcmp(r->inst,"NSC_LOOP") )
      {
         exps.opCode = LOOP;
         exps.loopCnt = exps.opCodeData = atoi(r->vals);
/*
         pbRes = pb_inst_radio_shape (0, PHASE090, PHASE000, 0,
               TX_DISABLE, exps.phaseReset,
               NO_TRIGGER, NO_SHAPE, AMP0, exps.status,
               exps.opCode, exps.opCodeData, MIN_DELAY);
         resetExp(pbRes, &exps);
 */
      }
      else if ( ! strcmp(r->inst,"DELAY") )
      {
         double duration = atof(r->vals);
         if (duration > 0.0)
         {
            pbRes = pb_inst_radio_shape (0, PHASE090, PHASE000, 0,
               TX_DISABLE, exps.phaseReset,
               NO_TRIGGER, NO_SHAPE, AMP0, exps.status,
               exps.opCode, exps.opCodeData, duration * 1e9);
            exps.exp_time += duration;
            resetExp(pbRes, &exps);
         }
            
      }
      else if ( ! strcmp(r->inst,"PULSE") )
      {
         int iphase;
         double duration;
         double phase;
         double blank_delay;
         double ringdown_delay;
         sscanf(r->vals,"%lg %lg %lg %lg",
                &duration, &phase, &blank_delay, &ringdown_delay);
         iphase = (int) (phase+0.001);
         if (duration > 0.0)
         {
            if (blank_delay > 0.0)
            {
               pbRes = pb_inst_radio_shape (0, PHASE090, PHASE000, 0,
                  TX_DISABLE, exps.phaseReset,
                  NO_TRIGGER, NO_SHAPE, AMP0, globals.blank_bit_mask + exps.status,
                  CONTINUE, NO_DATA, blank_delay * 1e9);
               exps.exp_time += blank_delay;
            }
            if (ringdown_delay > 0.0)
            {
               pbRes = pb_inst_radio_shape (0, PHASE090, PHASE000, iphase,
                  TX_ENABLE, exps.phaseReset,
                  NO_TRIGGER, NO_SHAPE, AMP0, globals.blank_bit_mask + exps.status,
                  CONTINUE, NO_DATA, duration * 1e9);
               pbRes = pb_inst_radio_shape (0, PHASE090, PHASE000, 0,
                  TX_DISABLE, exps.phaseReset,
                  NO_TRIGGER, NO_SHAPE, AMP0, globals.blank_bit_mask + exps.status,
                  exps.opCode, exps.opCodeData, ringdown_delay * 1e9);
            }
            else
            {
               pbRes = pb_inst_radio_shape (0, PHASE090, PHASE000, iphase,
                  TX_ENABLE, exps.phaseReset,
                  NO_TRIGGER, NO_SHAPE, AMP0, globals.blank_bit_mask + exps.status,
                  exps.opCode, exps.opCodeData, duration * 1e9);
            }
            exps.exp_time += duration + ringdown_delay;
            resetExp(pbRes, &exps);
         }
      }
      else if ( ! strcmp(r->inst,"ACQUIRE") )
      {
         int recIndex = atoi(r->vals);
         pbRes = pb_inst_radio_shape_cyclops (0, PHASE090, PHASE000, 0,
               TX_DISABLE, exps.phaseReset,
               DO_TRIGGER, NO_SHAPE, AMP0,
               recReal[recIndex], recImag[recIndex], recSwap[recIndex],
               exps.status,
               exps.opCode, exps.opCodeData, exps.at * 1e6);
         exps.exp_time += exps.at * 1e-3;
         resetExp(pbRes, &exps);
      }
      else if ( ! strcmp(r->inst,"STATUS") )
      {
         int device;
         int state;

         sscanf(r->vals,"%d %d", &device, &state);
         exps.status = state << device;
      }     
      else if ( ! strcmp(r->inst,"NSC_ENDLOOP") )
      {
         exps.opCode = END_LOOP;
         exps.opCodeData = exps.loop;
      }
      else if ( ! strcmp(r->inst,"PULSEPROG_DONE") )
      {
/*
         pb_inst_radio_shape (0, PHASE090, PHASE000, 0,
               TX_DISABLE, exps.phaseReset,
               NO_TRIGGER, NO_SHAPE, AMP0, exps.status,
               END_LOOP, exps.loop, MIN_DELAY);
         exps.exp_time += MIN_DELAY * 1e-9;
 */
         pb_inst_radio_shape (0, PHASE090, PHASE000, 0,
               TX_DISABLE, NO_PHASE_RESET,
               NO_TRIGGER, NO_SHAPE, AMP0, exps.status,
               STOP, NO_DATA, MIN_DELAY);
         exps.exp_time += MIN_DELAY * 1e-9;

         pb_stop_programming();
//         exps.exp_time *= exps.nt;
         pb_reset();
         pb_start();
         globals.board_start = 1;
         if (getData( &globals, &exps))
            break;
         saveData( exps.nt, atoi(r->vals), globals.arraydim,
                   globals.real, globals.imag,
                   &(globals.InfoFile[0]));
      }
      else if ( ! strcmp(r->inst,"MTUNE") )
      {
         double freq;
         double width;
         char dataPath[512];
	 int loops;
	 int pbRes __attribute__((unused));

         sscanf(r->vals,"%s %lg %lg", dataPath, &freq, &width);
         diagMessage("start MTUNE\n");
	 width *= 1e-6; // convert to MHz
	 memset ((void *) globals.imag, 0,
			  sizeof(int) * globals.complex_points );
	 unlink(dataPath);
	 pbRes = pb_inst_radio_shape (0, PHASE090, PHASE000, 0,
                  TX_ENABLE, exps.phaseReset,
                  NO_TRIGGER, NO_SHAPE, AMP0, globals.blank_bit_mask + exps.status,
                  CONTINUE, NO_DATA, 1e3);
         loops = pb_inst_radio_shape (0, PHASE090, PHASE000, 0,
                  TX_ENABLE, exps.phaseReset,
                  NO_TRIGGER, NO_SHAPE, AMP0, globals.blank_bit_mask + exps.status,
                  LOOP, 1000000000, 1e3);
         loops = pb_inst_radio_shape (0, PHASE090, PHASE000, 0,
                  TX_ENABLE, exps.phaseReset,
                  NO_TRIGGER, NO_SHAPE, AMP0, globals.blank_bit_mask + exps.status,
                  WAIT, 0, 1e6);
         pbRes = pb_inst_radio_shape (0, PHASE090, PHASE000, 0,
                  TX_ENABLE, exps.phaseReset,
                  NO_TRIGGER, NO_SHAPE, AMP0, globals.blank_bit_mask + exps.status,
                  END_LOOP, loops, 1e3);
         pb_inst_radio_shape (0, PHASE090, PHASE000, 0,
               TX_DISABLE, NO_PHASE_RESET,
               NO_TRIGGER, NO_SHAPE, AMP0, exps.status,
               STOP, NO_DATA, MIN_DELAY);
         pb_stop_programming();
         pb_reset();
         pb_start();
         globals.board_start = 1;
	 mtune(dataPath, freq, width, globals.complex_points,
		   globals.CodePath,
                   globals.real, globals.imag,
                   &(globals.InfoFile[0]));
         abortExp( & (globals.InfoFile[0]), globals.CodePath, 1, 1);
	 
/*
 * setup interupt handler
 * add pb_ commands to loop with a wait.
 * While (1 == 1)
 * start pb_
 * while (index1 < np) select with timeout
 * send message to expproc to initialize collection of reflected power
 * If flag is there
 *   send BS to OVJ
 *   send pb_start
 *   send message to Expproc
 *
 * Interrupt hander
 *   send pb_start
 *   send message to Expproc
 *   if index = np, set flag to send BS and reset for new scan
 */
         diagMessage("end MTUNE\n");
      }
      else if ( ! strcmp(r->inst,"MPSTUNE") )
      {
         double freq;
         double width;
         double power;
         double del;
         char dataPath[512];
	 int doEndCall = 1;
	 int ct = 1;

         sscanf(r->vals,"%s %lg %lg %lg", dataPath,
			 &freq, &width, &power);
	 del = 0.01; // dwell time between tune points in sec (10 msec)
         diagMessage("start MPSTUNE\n");
	 memset ((void *) globals.imag, 0,
			  sizeof(int) * globals.complex_points );
	 unlink(dataPath);
	 while ( 1 == 1)
         {
            requestMpsData( freq, width, power, del, dataPath,
                            &(globals.InfoFile[0]));
	    diagMessage("call getMPS\n");
            if (getMpsData( del, dataPath, &globals, &exps ) )
	    {
	       doEndCall = 0;
               break;
	    }
            diagMessage("save MPSTUNE data\n");
	    saveBsData( globals.real, globals.imag,
                      &(globals.InfoFile[0]), ct);
	    ct = 2;
	 }
	 if (doEndCall)
	    endMpsData();
         diagMessage("end MPSTUNE\n");
      }     
      r = r->next;
   }
   if (globals.board_start)
      pb_stop();
   if (globals.board_open)
      pb_close();
   diagMessage("call endComm\n");
   endComm();
   if (globals.real)
      free(globals.real);
   if (globals.imag)
      free(globals.imag);
   r = PSstart;
   while ( (r != NULL))
   {
      PSelem *n;
      
      n = r->next;
      free(r);
      r = n;
   }
   if (globals.totalTime > 60.0)
      diagMessage("Total experiment time %d:%d sec\n",
         (int) (globals.totalTime/60.0),
         (int) globals.totalTime - (int) (globals.totalTime/60.0) * 60 );
   else
      diagMessage("Total experiment time %g sec\n",globals.totalTime );
   if (debugFile != NULL)
      fclose(debugFile);
exit(EXIT_SUCCESS);
#ifdef XXX
		
	int processStatus = processArguments (argc, argv, myScan);
	if (processStatus != 0){
		pause();
		return processStatus;
	}	
	
	if (myScan->debug){
		pb_set_debug(1);
	}
	
		
	//
	//Configure Board
	//	
	
	//Set board defaults.
	if (configureBoard (myScan) < 0){
		printf ("Error: Failed to configure board.\n");
		pause();
		return CONFIGURATION_FAILED;
	}
	
	allocInitMem((void *) &real, myScan->num_points * sizeof(int));
	allocInitMem((void *) &imag, myScan->num_points * sizeof(int));
	
		
	//
	//Print Parameters
	//
		
	printScanParams (myScan);

	//Calculate and print total time
	double repetition_delay_ms = myScan->repetition_delay * 1000;
	double total_time = (myScan->scan_time + repetition_delay_ms)*myScan->num_scans;
	if(total_time < 1000){
		printf ("Total Experiment Time   : %lf ms\n\n", total_time);
	}
	else if(total_time < 60000){
		printf ("Total Experiment Time   : %lf s\n\n", total_time/1000);	
	}
	else if(total_time < 3600000){
		printf ("Total Experiment Time   : %lf minutes\n\n", total_time/60000);	
	}
	else{
		printf ("Total Experiment Time   : %lf hours\n\n\n", total_time/3600000);	
	}
	printf ("\nWaiting for the data acquisition to complete... \n\n");
	
		
	//
	//Program Board
	//
	
	if (programBoard (myScan) != 0){
		printf ("Error: Failed to program board.\n");
		pause();
		return PROGRAMMING_FAILED;
	}
		
		
	///
    /// Trigger Pulse Program
    ///
	
	pb_reset ();	
	pb_start ();
	
			
	//
    // Count Scans
    //
	
	int scan_count = 0; // Scan count is not deterministic. Reported scans may be skipped or repeated (actual scan count is correct)
	do{      
		pb_sleep_ms (total_time/myScan->num_scans);
		
		if (scan_count != pb_scan_count(0)){
			scan_count = pb_scan_count(0);
			printf("Current Scan: %d\n", scan_count);
		}
	} while (!(pb_read_status() & STATUS_STOPPED));	//Wait for the board to complete execution.
	
	printf("\nExecution complete \n\n\n");
		
		
	///
    /// Read Data From RAM
    ///
	
	if (myScan->enable_rx){
		if( pb_get_data (myScan->num_points, real, imag) < 0 ){
			printf("Failed to get data from board.\n");
			pause();
			return DATA_RETRIEVAL_FAILED;
		}
		
		//Estimate resonance frequency
		double SF_Hz = (myScan->SF)*1e6;
		double actual_SW_Hz = (myScan->SW)*1e3;
		myScan->res_frequency = pb_fft_find_resonance(myScan->num_points, SF_Hz, actual_SW_Hz, real, imag)/1e6;
		
		//Print resonance estimate	   
		printf("Highest Peak Frequency: %lf MHz\n\n\n", myScan->res_frequency);
	  	
	  	  	
	  	//
	  	// Write Output Files
	  	//
	  	
	  	writeDataToFiles(myScan, real, imag);
	}
	
		
	
	//
	// End Program
	//
	pb_stop ();
		
	pb_close ();

	free (myScan);
	free (real);
	free (imag);
	
	return 0;
#endif
}

#ifdef XXX

//
//
// FUNCTIONS
//
//

int allocInitMem (void **array, int size){
	//Allocate memory
	*array = (void *) malloc (size);
	
	//Verify allocation
	if( *array == NULL ){
		printf("Memory allocation failed.\n");
		pause();
		return ALLOCATION_FAILED;
	}
	
	//Initialize allocated memory to zero
	memset ((void *) *array, 0, size);
	
	return 0;
}


//
// Parameter Reading
//

int processArguments (int argc, char *argv[], SCANPARAMS * scanParams){
	//Check for valid argument count
	if (argc != NUM_ARGUMENTS + 1){
    	printProperUse ();		
		return INVALID_NUM_ARGUMENTS;
    }
	
	//
	//Process arguments
	//
	
	scanParams->file_name           = argv[1];
	
	//Board Parameters
	scanParams->board_num           = atoi (argv[2]);
	scanParams->deblank_bit         = (unsigned short) atoi (argv[3]);
	scanParams->deblank_bit_mask    = (1 << scanParams->deblank_bit);
	scanParams->debug               = (unsigned short) atoi(argv[4]);
		
	//Frequency Parameters
	scanParams->ADC_frequency       = atof (argv[5]);
	scanParams->SF                  = atof (argv[6]);
	scanParams->SW                  = atof (argv[7]);
	
	//Pulse Parameters
	scanParams->enable_tx           = (unsigned short) atof (argv[8]);
	scanParams->use_shape           = (unsigned short) atoi (argv[9]);
	scanParams->amplitude           = atof (argv[10]);
	scanParams->p90_time            = atof (argv[11]);
	scanParams->p90_phase           = atof (argv[12]);
	
	//Acquisition Parameters
	scanParams->enable_rx           = (unsigned short) atof (argv[13]);
	scanParams->bypass_fir          = (unsigned short) atoi (argv[14]);
	scanParams->num_points          = (unsigned int) roundUpPower2(atoi (argv[15])); //Only use powers of 2
	scanParams->num_scans           = (unsigned int) atoi (argv[16]);
	
	//Delay Parameters
	scanParams->deblank_delay       = atof (argv[17]);
	scanParams->transient_delay     = atof (argv[18]);
	scanParams->repetition_delay    = atof (argv[19]);
	
	//Ensure number of points is a power of 2
	if(scanParams->num_points != atoi (argv[15])){
		printf("| Notice:                                                                      |\n"); 
		printf("|     The desired number of points is not a power of 2.                        |\n");
		printf("|     The number of points have been rounded up to the nearest power of 2.     |\n");
		printf("|                                                                              |\n");
	}
	
	//Check if transmit and recieve are both disabled and warn user
	if((scanParams->enable_tx == 0) && (scanParams->enable_rx == 0)){
			printf("| Notice:                                                                      |\n"); 
			printf("|     Transmit and recieve have both been disabled.                            |\n");
			printf("|     The board will not be programmed.                                        |\n");
			printf("|                                                                              |\n");
	}
	
	
	//Check parameters
	if (verifyArguments (scanParams) != 0){
		return INVALID_ARGUMENTS;
	}

	return 0;
}

int verifyArguments (SCANPARAMS * scanParams){
	int fw_id;
	
	if (pb_count_boards () <= 0){
		printf("No RadioProcessor boards were detected in your system.\n");
		return BOARD_NOT_DETECTED;
	}
	
	if (pb_count_boards () > 0 && scanParams->board_num > pb_count_boards () - 1){
		printf ("Error: Invalid board number. Use (0-%d).\n", pb_count_boards () - 1);
		return -1;
    }
    
	pb_select_board(scanParams->board_num);
	if (pb_init()){
		printf ("Error initializing board: %s\n", pb_get_error ());
		return -1;
    }
  
	fw_id = pb_get_firmware_id();
	if ((fw_id > 0xA00 && fw_id < 0xAFF) || (fw_id > 0xF00 && fw_id < 0xFFF)){
		if (scanParams->num_points > 16*1024 || scanParams->num_points < 1){
			printf ("Error: Maximum number of points for RadioProcessorPCI is 16384.\n");
			return -1;
		}
	}
	
	else if(fw_id > 0xC00 && fw_id < 0xCFF){
		if (scanParams->num_points > 256*1024 || scanParams->num_points < 1){
			printf ("Error: Maximum number of points for RadioProcessorUSB is 256k.\n");
			return -1;
		}
	}
  	
  	if (scanParams->num_scans < 1){
		printf ("Error: There must be at least one scan.\n");
		return -1;
    }

	if (scanParams->p90_time < 0.065){
		printf ("Error: Pulse time is too small to work with board.\n");
		return -1;
    }

	if (scanParams->transient_delay < 0.065){
		printf ("Error: Transient delay is too small to work with board.\n");
		return -1;
    }

	if (scanParams->repetition_delay <= 0){
		printf ("Error: Repetition delay is too small.\n");
		return -1;
    }

	if (scanParams->amplitude < 0.0 || scanParams->amplitude > 1.0){
		printf ("Error: Amplitude value out of range.\n");
		return -1;
    }

	// The RadioProcessor has 4 TTL outputs, check that the blanking bit
	// specified is possible if blanking is enabled.
	if (scanParams->deblank_bit < 0 || scanParams->deblank_bit > 3){
		printf("Error: Invalid de-blanking bit specified.\n");
		return -1;
	}

	return 0;
}


//
// Terminal Output
//

void printProgramTitle(char* title){
	//Create a title block of 80 characters in width
	printf ("|------------------------------------------------------------------------------|\n");
	printf ("|                                                                              |\n");
	printf ("|                               Single Pulse NMR                               |\n");
	printf ("|                                                                              |\n");
	printf ("|                       Using SpinAPI Version:  %.8s                       |\n", pb_get_version());
	printf ("|                                                                              |\n");
	printf ("|------------------------------------------------------------------------------|\n");
}

inline void printProperUse (){
	printf ("Incorrect number of arguments, there should be %d. Syntax is:\n", NUM_ARGUMENTS);
	printf ("--------------------------------------------\n");
	printf ("Variable                       Units\n");
	printf ("--------------------------------------------\n");
	printf ("Filename.......................Filename to store output\n");
	printf ("Board Number...................(0-%d)\n", pb_count_boards () - 1);
  	printf ("Blanking TTL Flag Bit..........TTL Flag Bit used for amplifier blanking signal\n");
  	printf ("Debugging Output...............(1 enables debugging output logfile, 0 disables)\n");
	printf ("ADC Frequency..................ADC sample frequency\n");
	printf ("Spectrometer Frequency.........MHz\n");
	printf ("Spectral Width.................kHz\n");
	printf ("Enable Transmitter Stage.......(1 turns transmitter on, 0 turns transmitter off)\n");
	printf ("Shaped Pulse...................(1 to output shaped pulse, 0 otherwise)\n");
	printf ("Amplitude......................Amplitude of excitation pulse (0.0 to 1.0)\n");
	printf ("90 Degree Pulse Time...........us\n");
	printf ("90 Degree Pulse Phase..........degrees\n");
	printf ("Enable Receiver Stage..........(1 turns receiver on, 0 turns receiver off)\n");
	printf ("Bypass FIR.....................(1 to bypass, or 0 to use)\n");
	printf ("Number of Points...............(1-16384)\n");
	printf ("Number of Scans................(1 or greater)\n");
	printf ("De-blanking Delay..............Delay between de-blanking and the TX Pulse (ms)\n");
	printf ("Transient Delay................us\n");
	printf ("Repetition Delay...............s\n");
}

void printScanParams (SCANPARAMS * myScan){
	//Create a table of 80 characters in width
	char buffer[80] = {0};
	printf ("|-----------------------------  Scan  Parameters  -----------------------------|\n");
	printf ("|------------------------------------------------------------------------------|\n");
	printf ("| Filename: %-66s |\n",											myScan->file_name);
	printf ("|                                                                              |\n");
	printf ("| Board Parameters:                                                            |\n");
	sprintf(buffer, "%d", myScan->board_num);
	printf ("|      Board Number                   : %-38s |\n", buffer);
	sprintf(buffer, "%d", myScan->deblank_bit);
	printf ("|      De-blanking TTL Flag Bit       : %-38s |\n", buffer);
	printf ("|      Debugging                      : %-38s |\n", (myScan->debug != 0) ? "Enabled":"Disabled");
	printf ("|                                                                              |\n");
	printf ("| Frequency Parameters:                                                        |\n");
	sprintf(buffer, "%.4f MHz", myScan->ADC_frequency);
	printf ("|      ADC Frequency                  : %-38s |\n", buffer);
	sprintf(buffer, "%.4f MHz", myScan->SF);
	printf ("|      Spectrometer Frequency         : %-38s |\n", buffer);
	sprintf(buffer, "%.4f kHz", myScan->SW);
	printf ("|      Spectral Width                 : %-38s |\n", buffer);
	printf ("|                                                                              |\n");
	printf ("| Pulse Parameters:                                                            |\n");
	printf ("|      Enable Transmitter             : %-38s |\n", (myScan->enable_tx != 0) ? "Enabled":"Disabled");
	printf ("|      Use Shape                      : %-38s |\n", (myScan->use_shape != 0) ? "Enabled":"Disabled");
	sprintf(buffer, "%.4f", myScan->amplitude);
	printf ("|      Amplitude                      : %-38s |\n", buffer);
	sprintf(buffer, "%.4f us", myScan->p90_time);
	printf ("|      90 Degree Pulse Time           : %-38s |\n", buffer);
	sprintf(buffer, "%.4f degrees", myScan->p90_phase);
	printf ("|      90 Degree Pulse Phase          : %-38s |\n", buffer);
	printf ("|                                                                              |\n");
	printf ("| Acquistion Parameters:                                                       |\n");
	printf ("|      Enable Reciever                : %-38s |\n", (myScan->enable_rx != 0) ? "Enabled":"Disabled");
	printf ("|      Bypass FIR                     : %-38s |\n", (myScan->bypass_fir != 0) ? "Enabled":"Disabled");
	sprintf(buffer, "%d", myScan->num_points);
	printf ("|      Number of Points               : %-38s |\n", buffer);
	sprintf(buffer, "%d", myScan->num_scans);
	printf ("|      Number of Scans                : %-38s |\n", buffer);
	sprintf(buffer, "%.4f ms", myScan->scan_time);
	printf ("|      Total Acquisition Time         : %-38s |\n", buffer);
	printf ("|                                                                              |\n");
	printf ("| Delay Parameters:                                                            |\n");
	sprintf(buffer, "%.4f ms", myScan->deblank_delay);
	printf ("|      De-blanking Delay              : %-38s |\n", buffer);
	sprintf(buffer, "%.4f us", myScan->transient_delay);
	printf ("|      Transient Delay                : %-38s |\n", buffer);
	sprintf(buffer, "%.4f s", myScan->repetition_delay);
	printf ("|      Repetition Delay               : %-38s |\n", buffer);
	printf ("|                                                                              |\n");
	printf ("|------------------------------------------------------------------------------|\n");
}


//
// Board Interfacing
//

int configureBoard (SCANPARAMS * myScan){
	
	pb_set_defaults ();
	pb_core_clock (myScan->ADC_frequency);
	
	pb_overflow (1, 0);		//Reset the overflow counters
	pb_scan_count (1);		//Reset scan counter
	
	
	// Load the shape parameters
	float shape_data[1024];
	int num_lobes = 3;
	
	make_shape_data (shape_data, (void *) &num_lobes, shape_sinc);
	pb_dds_load (shape_data, DEVICE_SHAPE);
	pb_set_amp (((myScan->enable_tx) ? myScan->amplitude : 0), 0);


	///
	/// Set acquisition parameters
	///

	//Determine actual spectral width
	int cmd = 0;
	if (myScan->bypass_fir){
		cmd = BYPASS_FIR;
    }

	double SW_MHz = myScan->SW / 1000.0;
	int dec_amount = pb_setup_filters (SW_MHz, myScan->num_scans, cmd);
	if (dec_amount <= 0){
		printf("\n\nError: Invalid data returned from pb_setup_filters(). Please check your board.\n\n");
		return INVALID_DATA_FROM_BOARD;
    }
	
	double ADC_frequency_kHz = myScan->ADC_frequency * 1000;
	myScan->actual_SW = ADC_frequency_kHz / (double) dec_amount;
	
	
	//Determine scan time, the total amount of time that data is collected in one scan cycle
	myScan->scan_time = (((double) myScan->num_points) / myScan->actual_SW);
	
	
	pb_set_num_points (myScan->num_points);
	pb_set_scan_segments(1);
	
	return 0;
}

int programBoard (SCANPARAMS * myScan){  
	if(!myScan->enable_rx && !myScan->enable_tx){
		return RX_AND_TX_DISABLED;
	}
	
	///
	/// Program frequency, phase and amplitude registers
	///
	
	int scan_loop_label;
	
	//Frequency	
  	pb_start_programming (FREQ_REGS);
  	pb_set_freq (myScan->SF * MHz);
  	pb_set_freq (checkUndersampling (myScan));
  	pb_stop_programming ();
	
	//Real Phase
  	pb_start_programming (COS_PHASE_REGS);
  	pb_set_phase (0.0);
  	pb_set_phase (90.0);
  	pb_set_phase (180.0);
	pb_set_phase (270.0);
	pb_stop_programming ();
	
	//Imaginary Phase
  	pb_start_programming (SIN_PHASE_REGS);
	pb_set_phase (0.0);
	pb_set_phase (90.0);
	pb_set_phase (180.0);
  	pb_set_phase (270.0);
	pb_stop_programming ();
	
	//Transmitted Phase
	pb_start_programming (TX_PHASE_REGS);
	pb_set_phase (0.0);
	pb_set_phase (90.0);
	pb_set_phase (180.0);
  	pb_set_phase (270.0);
	pb_stop_programming ();
	
	//Amplitude
	pb_set_amp(myScan->amplitude , 0);
	
	
	///
	/// Specify pulse program
	///
	
	pb_start_programming (PULSE_PROGRAM);
	
		scan_loop_label =
			//De-blank amplifier for the blanking delay so that it can fully amplify a pulse
		 	//Initialize scan loop to loop for the specified number of scans
			//Reset phase so that the excitation pulse phase will be the same for every scan		
			pb_inst_radio_shape (0, PHASE090, PHASE000, 0, TX_DISABLE, PHASE_RESET, 
				NO_TRIGGER, 0, 0, myScan->deblank_bit_mask, LOOP, myScan->num_scans, myScan->deblank_delay * ms); 
			
			//Transmit 90 degree pulse
		   	pb_inst_radio_shape (0, PHASE090, PHASE000, 0, myScan->enable_tx, NO_PHASE_RESET, 
				NO_TRIGGER, myScan->use_shape, 0, myScan->deblank_bit_mask, CONTINUE, 0, myScan->p90_time * us);
			
			//Wait for the transient to subside
		    pb_inst_radio_shape (0, PHASE090, PHASE000, 0, TX_DISABLE, NO_PHASE_RESET,
				NO_TRIGGER, 0, 0, BLANK_PA, CONTINUE, 0, myScan->transient_delay * us);
			
			//Trigger acquisition
			pb_inst_radio_shape (1, PHASE090, PHASE000, 0, TX_DISABLE, NO_PHASE_RESET,
				myScan->enable_rx, 0, 0, BLANK_PA, CONTINUE, 0, myScan->scan_time * ms);
			
			//Allow sample to relax before performing another scan cycle
			pb_inst_radio_shape (0, PHASE090, PHASE000, 0, TX_DISABLE, NO_PHASE_RESET, 
				NO_TRIGGER, 0, 0, BLANK_PA, END_LOOP, scan_loop_label, myScan->repetition_delay * 1000.0 * ms);
		
		//After all scans complete, stop the pulse program
		pb_inst_radio_shape (0, PHASE090, PHASE000, 0, TX_DISABLE, NO_PHASE_RESET,
			NO_TRIGGER, 0, 0, BLANK_PA, STOP, 0, 1.0 * us);

	pb_stop_programming ();

  return 0;
}


//
// Calculations
//

double checkUndersampling (SCANPARAMS * myScan){
	int folding_constant;
	double folded_frequency;
	double adc_frequency = myScan->ADC_frequency;
	double spectrometer_frequency = myScan->SF;
	double nyquist_frequency = adc_frequency / 2.0;
	
	if (spectrometer_frequency > nyquist_frequency){
		if (((spectrometer_frequency / adc_frequency) -	(int) (spectrometer_frequency / adc_frequency)) >= 0.5){
	 		folding_constant = (int) ceil (spectrometer_frequency / adc_frequency);
	 	}
    	else{
    		folding_constant = (int) floor (spectrometer_frequency / adc_frequency);
    	}
		
		folded_frequency = fabs (spectrometer_frequency - ((double) folding_constant) * adc_frequency);
		
		printf("Undersampling Detected: Spectrometer Frequency (%.4lf MHz) is greater than Nyquist (%.4lf MHz).\n", spectrometer_frequency, nyquist_frequency);
		
		spectrometer_frequency = folded_frequency;
    	
    	printf ("Using Spectrometer Frequency: %lf MHz.\n\n", spectrometer_frequency);
	}
    
	return spectrometer_frequency;
}

//Round a number up to the nearest power of 2 
 int roundUpPower2(int number){
 	int remainder_total = 0;
 	int rounded_number = 1;
 	
 	//Determine next highest power of 2
	 while(number != 0){
 		remainder_total += number % 2;
 		number /= 2;
 		rounded_number *= 2;
	}
	
	//If the number was originally a power of 2, it will only have a remainder for 1/2, which is 1
	//Then lower it a power of 2 to recieve the original value
	if(remainder_total == 1){
		rounded_number /= 2;	
	}
	
 	return rounded_number;
 }

#endif
