/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <stdarg.h>

#define  DELIMITER_2      '\n'

#include "sockets.h"
#include "hostMsgChannels.h"
#include "hostAcqStructs.h"
#include "msgQLib.h"
#include "eventHandler.h"
#include "commfuncs.h"
#include "errLogLib.h"

#ifndef DEBUG_HEARTBEAT
#define DEBUG_HEARTBEAT	(9)
#endif

#define TRUE 1
#define FALSE 0
#define FOR_EVER 1
#define HEARTBEAT_TIMEOUT_INTERVAL	(2.8)

extern MSG_Q_ID pRecvMsgQ;

int chanId;
int chanIdSync;		/* Expproc's updating socket to console */

static int Chan_Num = - 1;
static int Heart1stTime = 1;
static int HeartBeatReply = -1;
static int doHeartBeat = 0;
void setupHeartBeat();

#define WALLNUM 2
static char *wallpaths[] = { "/usr/sbin/wall", "/usr/bin/wall" };
static char wallpath[25] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };
static int shpid;

/**************************************************************
*
*  shutdownComm - Close the Message Q , DataBase & Channel 
*
* This routine closes the message Q , DataBase and Channel
*  
* RETURNS:
* MSG_Q_ID , or NULL on error. 
*
*       Author Greg Brissey 8/4/94
*/
void shutdownComm(void)
{
     deleteMsgQ(pRecvMsgQ);
}

/* define this dummy for all other processes except Expproc */
#ifndef NODUMMY
void processChanMsge()
{
}
void resetExpproc()
{
}
int isExpActive()
{
   return(1);
}
#endif

static void ignoreSigalrm(int signal)
{
}

int consoleConn()
{
   return(1);
}

/*-------------------------------------------------------------------------
|
|   Setup the timer interval alarm
|
+--------------------------------------------------------------------------*/
int setRtimer(double timsec, double interval)
{
    long sec,frac;
    struct itimerval timeval,oldtime;

    sec = (long) timsec;
    frac = (long) ( (timsec - (double)sec) * 1.0e6 ); /* usecs */
    DPRINT2(3,"setRtimer(): sec = %ld, frac = %ld\n",sec,frac);
    timeval.it_value.tv_sec = sec;
    timeval.it_value.tv_usec = frac;
    sec = (long) interval;
    frac = (long) ( (interval - (double)sec) * 1.0e6 ); /* usecs */
    DPRINT2(3,"setRtimer(): sec = %ld, frac = %ld\n",sec,frac);
    timeval.it_interval.tv_sec = sec;
    timeval.it_interval.tv_usec = frac;
    if (setitimer(ITIMER_REAL,&timeval,&oldtime) == -1)
    {
         perror("setitimer error");
         return(-1);
    }
    return(0);
}

/**************************************************************
*
*   deliverMessage - send message to a named message queue or socket
*
*   Use this program to send a message to a message queue
*   using the name of the message queue.  It opens the
*   message queue, sends the message and then closes the
*   message queue.  The return value is the result from
*   sending the message, unless it cannot access the message
*   queue, in which case it returns -1.  Its two arguments
*   are the name of the message queue and the message, both
*   the address of character strings.
*   
*   
*   March 1998:  deliverMessage was extended to send the message
*   to either a message queue or a socket.  It selects the system
*   interface based on the format of the interface argument.
*
*   A message queue will have an interface either like "Expproc"
*   (single word) or "Vnmr 1234" (word, space, number - note
*   this latter form is now obsolete).
*
*   A socket will have an interface like "inova400 34567 1234"
*   (word, space, number, space, number - the word is the hostname,
*   the first number is the port address of the socket to connect
*   to, the second is the process ID).
*
*   Using a format specifier of "%s %d %d" (string of non-space
*   characters, space, number, space, number) it scans this
*   interface argument.  If it can convert three fields, then
*   the interface is a socket; otherwise it is a message queue.
*/

int
deliverMessage( char *interface, char *message )
{
	char		tmpstring[ 256 ];
	int		ival1, ival2;
	int		ival, mlen;
	MSG_Q_ID	tmpMsgQ;

	if (interface == NULL)
	  return( -1 );
	if (message == NULL)
	  return( -1 );
	mlen = strlen( message );
	if (mlen < 1)
	  return( -1 );

    	ival = sscanf( interface, "%s %d %d\n", &tmpstring[ 0 ], &ival1, &ival2 );

/*
*        diagPrint(debugInfo,"Procproc deliverMessage  ----> host: '%s', port: %d,(%d,%d)  pid: %d\n",
*           tmpstring,ival1,0xffff & ntohs(ival1), 0xffff & htons(ival1), ival2);
*/  

	if (ival >= 3) {
		int	 replyPortAddr;
		char	*hostname;
		Socket	*pReplySocket;

		replyPortAddr = ival1;
		hostname = &tmpstring[ 0 ];

		pReplySocket = createSocket( SOCK_STREAM );
		if (pReplySocket == NULL)
		  return( -1 );
		ival = openSocket( pReplySocket );
		if (ival != 0)
                {
		  free( pReplySocket );
		  return( -1 );
                }
                /* vnm port is already in network order, so switch to host order since
                 * sockets.c will switch to network order
                 */
		ival = connectSocket( pReplySocket, hostname, 0xFFFF & htons(replyPortAddr) );
		if (ival == 0)
                {
		   writeSocket( pReplySocket, message, mlen );
                }
		closeSocket( pReplySocket );
		free( pReplySocket );
		return( ival );
	}

	tmpMsgQ = openMsgQ( interface );
	if (tmpMsgQ == NULL)
	  return( -1 );

        ival = sendMsgQ( tmpMsgQ, message, mlen, MSGQ_NORMAL, NO_WAIT );

        closeMsgQ( tmpMsgQ );
	return( ival );
}

/*  See deliverMessage for an explanation of how this works  */

int
verifyInterface( char *interface )
{
	char		tmpstring[ 256 ];
	int		ival1, ival2;
	int		ival;
	MSG_Q_ID	tmpMsgQ;

	if (interface == NULL)
	  return( 0 );

    	ival = sscanf( interface, "%s %d %d\n", &tmpstring[ 0 ], &ival1, &ival2 );

	if (ival >= 3) {
		int	 peerProcId;

		peerProcId = ival2;
		ival = kill( peerProcId, 0 );
		if (ival != 0) {
			if (errno != ESRCH)
			  DPRINT1( -1, "Error accessing PID %d\n", peerProcId );
			return( 0 );
		}
		else
		  return( 1 );
	}
	else {
		tmpMsgQ = openMsgQ( interface );
		if (tmpMsgQ == NULL)
		  return( 0 );

	        closeMsgQ( tmpMsgQ );
		return( 1 );
	}
}

