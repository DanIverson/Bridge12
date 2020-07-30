#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#ifndef PSG_LC
#include "inc/spinapi.h"

extern int ovj_get_radioprocessor(void);
#else
#include "vnmrsys.h"

#endif

struct brd {
   int fw_id;
   int radio;
   };

struct brds {
   struct brd item[5];
   };


#ifndef PSG_LC
void writeConf(const char *confPath)
{
   int i;
   int fd;
   int res __attribute__((unused));
   struct brds conf;
   int numboard = pb_count_boards();

   for (i=0; i < 5; i++)
   {
      conf.item[i].fw_id = 0;
      conf.item[i].radio = 0;
   }
   for (i=0; i < numboard; i++)
   {
      pb_select_board(i);
      pb_init();
      conf.item[i].fw_id = pb_get_firmware_id();
      conf.item[i].radio = ovj_get_radioprocessor();
   }
   if ( (fd= open(confPath, O_WRONLY | O_CREAT | O_TRUNC, 0666)) > 0)
   {
      res = write(fd, (const void *) &conf, sizeof(struct brds) );
      close(fd);
   }
}

void showConf(const char *confPath)
{
   int i;
   int fd;
   int res __attribute__((unused));
   struct brds conf;

   if ( ! access(confPath, R_OK) )
   {
      if ( (fd = open(confPath,O_RDONLY)) > 0)
      {
         res = read(fd, (void *) &conf, sizeof(struct brds) );
         fprintf(stdout,"B12 configuration file %s\n",confPath);
         for (i=0; i < 5; i++)
         {
            fprintf(stdout,"board %d: FirmWare: 0x%x  RadioController: %d\n",
                            i, conf.item[i].fw_id, conf.item[i].radio);
         }
      }
   }
   else
   {
      fprintf(stdout,"B12 configuration file %s does not exist\n",confPath);
   }
}
#endif

#ifdef PSG_LC
void readConf(int index, int *radio)
{
   int i;
   int fd;
   int res __attribute__((unused));
   struct brds conf;
   char confPath[MAXPATH];
   sprintf(confPath,"%s/acqqueue/b12conf",systemdir);

   if ( access(confPath, R_OK) )
   {
      char execCmd[3*MAXPATH];
      sprintf(execCmd,"%s/acqbin/B12proc Conf %s",
              systemdir,confPath);
      res = system(execCmd);
      i=0;
      while ( access(confPath, R_OK) && (i < 15) )
      {
         sleep(1);
         i++;
      }
   }
   *radio = 1;
   if ( ! access(confPath, R_OK) )
   {
      if ( (fd = open(confPath,O_RDONLY)) > 0)
      {
         res = read(fd, (void *) &conf, sizeof(struct brds) );
         *radio = conf.item[index].radio;
         close(fd);
      }
   }
}
#endif
