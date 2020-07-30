#include <stdio.h>
#include <string.h>
extern void diagMessage(const char *format, ...);
#include "makefid.h"

#include "src/caps.h"

#define CONTINUE 0
#define STOP 1
#define LOOP 2
#define END_LOOP 3

BOARD_INFO board[32];
int cur_board;


static double sw = 1.0;

int pb_count_boards()
{
   diagMessage("called pb_count_boards\n");
   return 1;
}
int pb_select_board(int board_num)
{
   diagMessage("called pb_select_board %d\n",board_num);
   return 0;
}
int pb_set_debug(int level)
{
   diagMessage("called pb_set_debug %d\n",level);
   return 0;
}
int pb_init()
{
   diagMessage("called pb_init\n");
   return 0;
}
char *pb_get_error()
{
   static char string[128];
   strcpy(string,"called pb_get_error\n");
   diagMessage("called pb_get_error\n");
   return string;
}
int pb_set_amp(float amp, int addr)
{
   diagMessage("called pb_set_amp\n");
   diagMessage("   amp= %g addr= %d\n",amp, addr);
   return 0;
}
int pb_inst_radio_shape(int freq, int cos_phase, int sin_phase, int tx_phase,
                     int tx_enable, int phase_reset, int trigger_scan,
                     int use_shape, int amp, int flags, int inst,
                     int inst_data, double length)
{
   char inststr[16];

   diagMessage("called pb_inst_radio_shape\n");
   diagMessage("   freq=%d cos_phase=%d sin_phase=%d tx_phase=%d\n",freq,cos_phase, sin_phase,tx_phase);
   diagMessage("   tx_enable=%d phase_reset=%d trigger_scan=%d\n",tx_enable,phase_reset, trigger_scan);
   if (inst == CONTINUE)
      strcpy(inststr,"CONTINUE");
   else if (inst == LOOP)
      strcpy(inststr,"LOOP");
   else if (inst == END_LOOP)
      strcpy(inststr,"ENDLOOP");
   else if (inst == STOP)
      strcpy(inststr,"STOP");
   else
      strcpy(inststr,"UNKNOWN");
   diagMessage("   use_shape=%d amp=%d flags=%d\n",use_shape,amp,flags);
   diagMessage("   inst=%s inst_data=%d length=%g\n",
                   inststr, inst_data, length);
   return( (inst == LOOP) ? 22 : 0);
}
int pb_inst_radio_shape_cyclops(int freq, int cos_phase, int sin_phase, int tx_phase,
                     int tx_enable, int phase_reset, int trigger_scan,
                     int use_shape, int amp,
                     int recRe, int recIm, int recSwap,
                     int flags, int inst,
                     int inst_data, double length)
{
   char inststr[16];

   diagMessage("called pb_inst_radio_shape_cyclops\n");
   diagMessage("   freq=%d cos_phase=%d sin_phase=%d tx_phase=%d\n",freq,cos_phase, sin_phase,tx_phase);
   diagMessage("   tx_enable=%d phase_reset=%d trigger_scan=%d\n",tx_enable,phase_reset, trigger_scan);
   if (inst == CONTINUE)
      strcpy(inststr,"CONTINUE");
   else if (inst == LOOP)
      strcpy(inststr,"LOOP");
   else if (inst == END_LOOP)
      strcpy(inststr,"ENDLOOP");
   else if (inst == STOP)
      strcpy(inststr,"STOP");
   else
      strcpy(inststr,"UNKNOWN");
   diagMessage("   use_shape=%d amp=%d flags=%d\n",use_shape,amp,flags);
   diagMessage("   recRe=%d recIm=%d recSwap=%d\n",recRe,recIm,recSwap);
   diagMessage("   inst=%s inst_data=%d length=%g\n",
                   inststr, inst_data, length);
   return 0;
}
int pb_set_phase(double phase)
{
   diagMessage("called pb_set_phase\n");
   diagMessage("   phase= %g\n",phase);
   return 0;
}
int pb_stop_programming()
{
   diagMessage("called pb_stop_programming\n");
   return 0;
}
int pb_start_programming(int device)
{
   diagMessage("called pb_start_programming device %d\n",device);
   return 0;
}
int pb_set_defaults()
{
   diagMessage("called pb_set_defaults\n");
   return 0;
}
void pb_core_clock(double clock_freq)
{
   diagMessage("called pb_core_clock %g\n",clock_freq);
}
int pb_overflow(int reset, void *of)
{
   diagMessage("called pb_overflow\n");
   return 0;
}
int pb_setup_filters(double spectral_width, int scan_repetitions, int cmd)
{
   diagMessage("called pb_setup_filters\n");
   diagMessage("   sw=%g scan=%d cmd=%d\n",spectral_width,scan_repetitions,cmd);
   sw = spectral_width * 1000.0;
   return 8;
}
int pb_set_freq(double freq)
{
   diagMessage("called pb_set_freq\n");
   diagMessage("   freq= %g\n",freq);
   return 0;
}
int pb_scan_count( int reset)
{
   diagMessage("called pb_scan_count\n");
   diagMessage("   reset= %d\n",reset);
   return 0;
}
int pb_dds_load( const float *data, int device)
{
   diagMessage("called pb_dds_load\n");
   diagMessage("   device= %d\n",device);
   return 0;
}
int pb_set_num_points(int num_points)
{
   diagMessage("called pb_set_num_points\n");
   diagMessage("   num_points= %d\n",num_points);
   return 0;
}
int pb_set_scan_segments(int num_segments)
{
   diagMessage("called pb_set_scan_segments\n");
   diagMessage("   segments= %d\n",num_segments);
   return 0;
}

int pb_write_felix()
{
   diagMessage("called pb_write_felix\n");
   return 0;
}
int pb_write_ascii_verbose()
{
   diagMessage("called pb_write_ascii_verbose\n");
   return 0;
}
int pb_write_jcamp()
{
   diagMessage("called pb_write_jcamp\n");
   return 0;
}
int pb_reset()
{
   diagMessage("called pb_reset\n");
   return 0;
}
int pb_start()
{
   diagMessage("called pb_start\n");
   return 0;
}
void pb_sleep_ms(int millis)
{
   diagMessage("called pb_sleep_ms %d\n", millis);
}
int pb_read_status()
{
   int stat = 0x03;
   diagMessage("called pb_read_status\n");
   return stat;
}
int pb_get_data(int num_points, int *real_data, int *imag_data)
{
   diagMessage("called pb_get_data\n");
   diagMessage("   num_points= %d\n",num_points);
   diagMessage("   call calc_FID: dwell= %g\n",1.0/(sw*1000.0));
   calc_FID(real_data,imag_data,20.0,100000.0, 0.5, 0.0, 1.0/(sw*1000.0), num_points);
   return 0;
}
int pb_fft_find_resonance()
{
   diagMessage("called pb_fft_find_resonance\n");
   return 0;
}
int pb_stop()
{
   diagMessage("called pb_stop\n");
   return 0;
}
int pb_close()
{
   diagMessage("called pb_close\n");
   return 0;
}
int pb_get_firmware_id()
{
   diagMessage("called pb_get_firmware_id\n");
   return 0;
}
const char *pb_get_version()
{
   static char vers[128];
   strcpy(vers,"aake");
   diagMessage("called pb_get_version\n");
   return vers;
}
int pb_set_clock()
{
   diagMessage("called pb_set_clock\n");
   return 0;
}
int pb_unset_radio_control()
{
   diagMessage("called pb_unset_radio_control\n");
   return 0;
}
int pb_zero_ram()
{
   diagMessage("called pb_zero_ram\n");
   return 0;
}
int pb_inst_radio(int freq, int cos_phase, int sin_phase,
                                  int tx_phase, int tx_enable,
                                  int phase_reset, int trigger_scan,
                                  int flags, int inst, int inst_data,
                                  double length)
{
   diagMessage("called pb_inst_radio\n");
   diagMessage("   freq=%d cos_phase=%d sin_phase=%d tx_phase=%d\n",freq,cos_phase, sin_phase,tx_phase);
   diagMessage("   tx_enable=%d phase_reset=%d trigger_scan=%d\n",tx_enable,phase_reset, trigger_scan);
   diagMessage("   flags=%d inst=%d inst_data=%d length=%g\n",flags,inst,inst_data, length);
   return 0;
}
int pb_set_radio_control()
{
   diagMessage("called pb_set_radio_control\n");
   return 0;
}
int pb_get_data_direct()
{
   diagMessage("called pb_get_data_direct\n");
   return 0;
}
