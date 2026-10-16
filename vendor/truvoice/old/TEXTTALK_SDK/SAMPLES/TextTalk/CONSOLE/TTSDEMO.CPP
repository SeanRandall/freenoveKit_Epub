/*************************************************************************
 * DESCRIPTION : Multithreaded program to test TTS library.
 * All channels play text files in async & sync mode.
 *************************************************************************/
#include <windows.h>
#include <winnt.h>
#include <limits.h>
#include <stdio.h>
#include <io.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>

#include <srllib.h>
#include <dxxxlib.h>
#include "texttalk.h"

#define TELEPHONE				  1   // Telephone(1)/Keybd(0) Interface
#define TTS_INFINITE_LOOP         0   // 1=> Demo runs in an infinite loop

#define TTS_DEFAULT_JUMP_SIZE     60
#define TTS_SPEAKMODE             EV_ASYNC;   // (EV_SYNC / EV_ASYNC

#define MAXCHAN					  4  // Maximum number of channels allowed


static int chdevmain[MAXCHAN];
static int ttsHandle[MAXCHAN];
static int playDone[MAXCHAN];
static int countmain[MAXCHAN];

char *channels[] = {
   "dxxxB1C1",
   "dxxxB1C2",
   "dxxxB1C3",
   "dxxxB1C4",
   NULL,
};


void chkargs(int argc, char* argv[]);
DWORD WINAPI ThreadProc ( LPVOID );
int StartProc (int inpId ); 
int EndProc (int inpId ); 
int WaitProc (); 
void processDigit(int, char);
int getId(int);
void getCommand(int *rcu_dev);


/* Declarations for the multithreaded part */
int		NoOfThreads = 1;
HANDLE	hThread[MAXCHAN];
DWORD	dwThreadId;

// Globals
char	pStr[5*1024];
int		speechrate = TTS_DEFAULT_SPEECH_RATE;
int		pitch = TTS_DEFAULT_PITCH;
int		traceLevel = 0;
DX_UIO	ttsUIO;


/**********************************************************************
 *       NAME : exitChildProcess
 *DESCRIPTION : Closes all open devices, stops output and kills the
 *              child process specified
 *      INPUT : child Number
 *     RETURN : none
 *      CALLS :
 *   CAUTIONS : none.
 **********************************************************************/
void exitChildProcess( int child )
{

   int rc;

   printf("Child process for channel %d exit...\n", child);
   if (ttsHandle[child] > 0)
    {
        rc = tts_stop(ttsHandle[child]);
		if (rc < 0)
		{
            printf("exit :Error %d in tts_stop(%d) - %s\n", 
                    rc, ttsHandle[child], tts_geterror(rc));
		}

        rc = tts_close(ttsHandle[child]);
		if (rc < 0)
		{
			printf("exit :Error %d in tts_close(%d) - %s\n",
                    rc, ttsHandle[child], tts_geterror(rc));
		}
		ttsHandle[child] = 0;
    }

    if (chdevmain[child] > 0)
    {
		dx_stopch(chdevmain[child], EV_SYNC);
        dx_sethook(chdevmain[child], DX_ONHOOK, EV_SYNC);
        dx_close(chdevmain[child]);
		chdevmain[child] = 0;
    }

}


/**********************************************************************
 *       NAME : signal_action
 *DESCRIPTION : Action to do when process is killed unnormally.
 *      INPUT : sig = signal type.
 *     RETURN : none
 *      CALLS :
 *   CAUTIONS : none.
 **********************************************************************/
void signal_action(int sig)
{
    signal(sig,SIG_IGN);

    printf("EOD stop command is sent to process because CTRL-BRK\n");
	for (int i=0; i < MAXCHAN; i++) exitChildProcess(i);
	exit(0);
}
    

/****************************************************************
*        NAME : getTTSId()
* DESCRIPTION : returns channel # given a tts handle
*       INPUT : tts device handle
*      OUTPUT :
*     RETURNS : channel #, -1 if not found
*    CAUTIONS :
****************************************************************/
int getTTSId(int tts_handle)
{
    int i;
	
    if (traceLevel > 70)
    {
		printf("getTTSId: inp=%d, ", tts_handle);
	}

    for (i = 0; i < MAXCHAN; i++)
    {
	    if (traceLevel > 80)
		{
			printf("(%d): dev=%d, ", i, ttsHandle[i]);
		}
        if(ttsHandle[i] == tts_handle)
        {
            return (i);
        }
    }
    return (-1);
}


/****************************************************************
*        NAME : tts_read, tts_write and tts_seek
* DESCRIPTION : Simulate read and seek functions  
*       INPUT : error number
*      OUTPUT :
*     RETURNS : Error code
*    CAUTIONS :
****************************************************************/
int tts_read(int fd, char *bufPtr, unsigned int bytes)
{

	int bytesRead = read(fd, bufPtr, bytes);
	printf("tts_read: fd=%d, bytes=%d\n", fd, bytesRead);
	return bytesRead;
}


int tts_write(int fd, char *bufPtr, unsigned int bytes)
{
	printf("using tts_write\n");
	return(write(fd, bufPtr, bytes));
}


long tts_seek(int fd, long offset, int whence)
{
	printf("using tts_seek, fd=%d, off=%d, wh=%d\n",
		    fd, offset, whence);
	return(lseek(fd, offset, whence));
}

/****************************************************************
*        NAME : playPCMfile 
* DESCRIPTION : play a PCM data file (for testing only)  
*       INPUT : error number
*      OUTPUT :
*     RETURNS : Error code
*    CAUTIONS :
****************************************************************/
int playPCMfile(int handle, char* fileName)
{
	DX_IOTT iott;

	iott.io_type = IO_DEV|IO_UIO|IO_EOT;
	iott.io_bufp = 0;
	iott.io_offset = 0;
	iott.io_length = -1;
	iott.io_nextp = NULL;
	iott.io_fhandle = open(fileName, O_RDWR|O_BINARY);

	dx_play(handle, &iott, NULL, EV_SYNC | PM_SR8 | MD_PCM);
	printf("dx_play done on dev=%d\n", handle);
	close(iott.io_fhandle);
	return 0;
}


/*************************************************************************
 *        NAME : play_handler
 * DESCRIPTION : called whenever an event is detected by WaitProc()
 *       INPUT : 
 *      OUTPUT : none
 *     RETURNS : none
 *    CAUTIONS : none
 *************************************************************************/
long _cdecl play_handler(void)
{
	int channel, chId = 0;
    long event;
	DX_CST *callStruct;

    event = sr_getevttype(0);
	channel = sr_getevtdev(0);

    if (traceLevel > 50)
    {
         printf("play_handler: Got event 0x%x on ch=%d\n", event, channel);
    }

    /* Check for dialogic generic TTS events */
	switch(event)
	{
		/* Detected call state transition (Digits, Hangup etc) */
        case TDX_CST :
             if (traceLevel > 40)
             {
                 printf("Got TDX_CST command.. ");
             }
             callStruct = (DX_CST *)sr_getevtdatap(0);
			 if (callStruct->cst_event != DE_DIGITS)
			 {
				 // HangUp detected
					 if (callStruct->cst_event == DE_LCOFF)
					 {
						printf("Hangup detected. Stop TTS\n");
						int chId = getId(channel);

						 int rc = tts_stop(ttsHandle[chId]);
						 if (rc < 0)
						 {
							 printf("tts_stop ERROR: %s\n", 
									tts_geterror(rc));
						 }
					 }

				 return(0);
			 }
			 processDigit(channel, (char)callStruct->cst_data);
			 break;

		case TDX_PLAY :
			 printf("Got TDX_PLAY event on dx_dev=%d\n", channel);
			 break;

		
		case TTS_EV_FILE_END :
			chId = getId(channel);
            printf("Got TTS_EV_FILEEND command on CHANNEL %d.\n", chId);
		
			EndProc(chId);

			// End program
				if (!TTS_INFINITE_LOOP)
				{
					playDone[chId] = TRUE;
					NoOfThreads--;
				}

			// Spawn the thread
			if (!playDone[chId])
			{
				// reopen devices
					StartProc(chId);

				CloseHandle (hThread[chId]);
   				hThread[chId] = CreateThread(NULL, 0, 
									  (LPTHREAD_START_ROUTINE)ThreadProc, 
									  (LPVOID) &(countmain[chId]),
      								  CREATE_SUSPENDED, &dwThreadId);
				SetThreadPriority ( hThread[chId], THREAD_PRIORITY_NORMAL );
				ResumeThread ( hThread[chId] );
				if (traceLevel > 40)
				{
					printf("Spawned new thread on CHANNEL %d.\n", chId);
				}
			}
			break;

        default :
             printf("dx_handler: Got Invalid event of type 0x%x\n", 
				     event);
    }

    if (traceLevel > 90)
    {
		printf("returning 1 for event 0x%x on dx_dev=%d\n", event, channel);
	}
	return(1);
}


/*************************************************************************
 *        NAME : main(argc, argv[])
 * DESCRIPTION : entry point to application.
 *       INPUT : argc = argument count.
 *             : argv = array of pointers to arguments.
 *      OUTPUT : none.
 *     RETURNS : none.
 *    CAUTIONS : none.
 *************************************************************************/
void _cdecl main( int argc, char *argv[] )
{
	int i, sr_mode = SR_POLLMODE;

	if (argc > 1) 
	{
		chkargs(argc, argv);
	}

	if (sr_setparm(SRL_DEVICE, SR_MODEID, (void *)&sr_mode) == -1)
	{
		printf("Failed to set SRL mode\n");
		exit (1);
	}

	printf("Using %d channels with %d thread(s)\n", NoOfThreads, NoOfThreads);

	signal(SIGINT,signal_action);
    signal(SIGTERM,signal_action);

	// Initialize the TTS engine
		tts_init();

	// Start each channel
		for ( i = 0; i < NoOfThreads; i++ ) 
		{
			StartProc(i);
		}

	// Spawn off thread for each channel
		for ( i = 0; i < NoOfThreads; i++ ) 
		{
			countmain[i] = i;
   			hThread[i] = CreateThread(NULL, 0, 
									  (LPTHREAD_START_ROUTINE)ThreadProc, 
									  (LPVOID) &(countmain[i]),
      								  0, &dwThreadId);
			SetThreadPriority ( hThread[i], THREAD_PRIORITY_NORMAL );

		}

	WaitProc();

	// Spawn off thread for each channel
		for ( i = 0; i < NoOfThreads; i++ ) 
		{
			EndProc(i);
		}

	WaitForMultipleObjects ( min(NoOfThreads,64), hThread, TRUE, INFINITE );

	printf ( "Exiting the program\n" );
	return;
}


/****************************************************************
*        NAME : WaitProc
* DESCRIPTION : Waits for DTMF events and handles them
*       INPUT : 
*      OUTPUT :
*     RETURNS : void
*    CAUTIONS :
****************************************************************/
int WaitProc (void) 
{

	while (NoOfThreads > 0)
	{
#if TELEPHONE

			if (sr_waitevt(-1) == -1)
			{	
				printf("ERROR:sr_waitevt: %s", ATDV_ERRMSGP(SRL_DEVICE));
				return(0);	  
			}

			if (traceLevel > 30)
			{
				printf ( "Channel : Got sr_evt %x on  %s\n",
						sr_getevttype(), ATDV_NAMEP ( sr_getevtdev() ));
			}

			// Switch on the event type
					play_handler();
#else
					int count = NoOfThreads-1;
					HANDLE tempThr = CreateThread(NULL, 0, 
								(LPTHREAD_START_ROUTINE)getCommand, 
								(LPVOID) &(ttsHandle[count]),
      							0, &dwThreadId);
					WaitForSingleObject(tempThr, INFINITE);
					//getCommand(ttsHandle[count]);
#endif
	}// while loop end
	
	return ( 0 );
}


/****************************************************************
*        NAME : EndProc
* DESCRIPTION : Ends the TTS process on a channel
*               
*       INPUT : Channel Number
*      OUTPUT :
*     RETURNS : void
*    CAUTIONS :
****************************************************************/
int EndProc (int inpId ) 
{
	int count, error;

	count = inpId;

	/* close the engine */
	
		if (ttsHandle[count] > 0)
		{
			if ((error = tts_close(ttsHandle[count])) < 0)
			{
				printf("tts_close failed:%d : %s\n", error, 
						tts_geterror(error));
			}
			ttsHandle[count] = 0;
		}
	
			printf ( "Iteration complete on Channel %d: %s\n", 
					 count, ATDV_NAMEP ( chdevmain[count] ));

	// Set the channel offhook
		if (chdevmain[count] > 0)
		{
			if (dx_sethook(chdevmain[count], DL_ONHOOK, EV_SYNC) < 0) 
			{
				printf("EndProc: dx_sethook failed: %d, %s\n", 
					   chdevmain[count], ATDV_ERRMSGP (chdevmain[count]) );
			} 
			else if (traceLevel > 20)
			{
				printf("ONHOOK on Channel %d (dev=%d): %s.\n", count,
						chdevmain[count], ATDV_NAMEP (chdevmain[count]));
			}
			dx_close(chdevmain[count]);
			chdevmain[count] = 0;
		}

	return ( 0 );
}


/****************************************************************
*        NAME : StartProc
* DESCRIPTION : Set up the TTS channel for tts_speak()
*               
*       INPUT : Channel Number
*      OUTPUT :
*     RETURNS : void
*    CAUTIONS :
****************************************************************/
int StartProc (int inpId ) 
{
	int count;

	count = inpId;

	// open a channel
		if ((chdevmain[count] = dx_open(channels[count], 0)) == -1) 
		{
			printf("Error opening channel %s\n", channels[count]);
			return(0);
		}
		if (traceLevel > 20)
		{
			printf("Channel %s opened..", channels[count] );
		}

	// clear the digit buffer
		dx_clrdigbuf(chdevmain[count]);

    // Enable receiving digits & loop current off
		if (dx_setevtmsk(chdevmain[count], DM_DIGITS | DM_LCOFF| DM_LCON) == -1)
        {
			 printf("Error in dx_setevtmsk()\n"); 
        }

	// set uio mode & test it with a play
		ttsUIO.u_read = tts_read;
		ttsUIO.u_write = tts_write;
		ttsUIO.u_seek = tts_seek;

		// Set the channel off-hook
			if (dx_sethook(chdevmain[count], DL_OFFHOOK, EV_SYNC) < 0) 
			//if (dx_wtring(chdevmain[count], 1, DL_OFFHOOK, -1) < 0) 
			{
				printf("ERROR: main: dx_sethook failed: %d, %s\n", 
					    chdevmain[count], ATDV_ERRMSGP (chdevmain[count]));
			} 
			else if (traceLevel > 30)
			{
				printf("OFFHOOK on Channel: %s\n", 
						ATDV_NAMEP ( chdevmain[count] ));
			}

		// play a PCM stream (to test UIO/bufSize/sampleRate restore)
			//dx_setdevuio(chdevmain[count], &ttsUIO, NULL);
			//dx_play(chdevmain[count], "sample.vox", NULL, EV_SYNC);
			//playPCMfile(chdevmain[count], "PLAY01.RAW");

		/* open a TTS engine */
			if ((ttsHandle[count] = tts_open(chdevmain[count])) < 0)
			{
				printf("tts_open(%d) failed:%d : %s\n", count, 
					    ttsHandle[count], tts_geterror(ttsHandle[count]));
				return(0);
			}

			tts_tracelevel(ttsHandle[count], traceLevel);

	return 0;

}


/****************************************************************
*        NAME : ThreadProc
* DESCRIPTION : Calls tts_speak()
*               
*       INPUT : Channel Number.
*      OUTPUT :
*     RETURNS : void
*    CAUTIONS :
****************************************************************/
DWORD WINAPI ThreadProc (LPVOID inpId ) 
{
	int count, error;
	int mode = TTS_SPEAKMODE;
	count = *((int *)inpId);

	// open and read the input text file "state01.inp" into a buffer
		int fd = open("state01.inp", O_RDONLY);
		int bytesRead = read(fd, pStr, sizeof(pStr));
		pStr[bytesRead] = 0;

	do
	{
		// verify that a channel was available
			if (ttsHandle[count] < 0)
			{
				return 0;
			}

	// Set up the context for email pre-processing
		tts_setcontext(ttsHandle[count], TTS_EMAIL_CTX);

	// start the speak operation
		//if ((error = tts_speakemail(ttsHandle[count], pStr, 
		//	                        mode, NULL)) != 0)
		//if ((error = tts_speakbuffer(ttsHandle[count], pStr, 
		//	                        mode, NULL)) != 0)
		if ((error = tts_speakfile(ttsHandle[count], "state01.inp", 
									mode, NULL)) != 0)
		{
			printf("tts_speak(%d) on %s failed:%d : %s\n", 
					ttsHandle[count], ATDV_NAMEP(chdevmain[count]),
					error, tts_geterror(error));
			return(0);
		}

		if (traceLevel > 50)
		{
			printf("Called tts_speakemail(%d) %s\n", ttsHandle[count],
					ATDV_NAMEP(chdevmain[count]));
		}

	} while ((mode == EV_SYNC) && (!playDone[count]));

	return (0);
}


/*************************************************************************
 *        NAME : chkargs(argc, argv)
 * DESCRIPTION : check command-line arguments
 *       INPUT : argc = argument count
 *             : *argv[] = array of pointers to command-line arguments
 *      OUTPUT : none
 *     RETURNS : none
 *    CAUTIONS : none
 *************************************************************************/
void chkargs(int argc, char* argv[])
{
   int argno;

   /* Check each argument. */
   for (argno = 1; argno < argc; argno++) 
   {

      /* Check for dash. */
      if (*argv[argno] != '-') 
	  {
         printf("Must begin option with '-'\n");
         exit(-1);
      }

      switch(tolower(*(argv[argno]+1))) 
	  {
      case 'c':
         /* Set maximum number of channels to use. */
         sscanf(argv[argno]+2, "%d", &NoOfThreads);
         if ( NoOfThreads < 1 || NoOfThreads > MAXCHAN ) 
		 {
            printf("Use from 1 to %d Threads\n", MAXCHAN );
            exit(2);
         }
         break;

      case 't':
         /* Set the traceLevel to use. */
         sscanf(argv[argno]+2, "%d", &traceLevel);
         if ( traceLevel < 0 ) 
		 {
            printf("Use traceLevel greater than zero\n" );
            exit(2);
         }
         break;

      default:
         printf("%s: unknown option -- %c\n", *argv, 
			    tolower(*(argv[argno]+1)));

      case '?':
         /* display help message if '-?' is given */
         printf("usage: %s -c<NoOfChannels> -t<traceLevel>\n", *argv);
         printf("-c Number of channels to create, default: %d\n",
				 NoOfThreads);
         printf("-t the trace level for the demo, default=%d\n",
				traceLevel );
         exit(0);
         break;

      }
   };
}


/****************************************************************
*        NAME : getId()
* DESCRIPTION : returns the channel # given a dialogic handle
*       INPUT : dialogic device handle
*      OUTPUT :
*     RETURNS : channel #, -1 if not found
*    CAUTIONS :
****************************************************************/
int getId(int dx_handle)
{
    int i;
	
    if (traceLevel > 70)
    {
		printf("getId: inp=%d, ", dx_handle);
	}

    for (i = 0; i < MAXCHAN; i++)
    {
	    if (traceLevel > 80)
		{
			printf("(%d): dev=%d, ", i, chdevmain[i]);
		}
        if(chdevmain[i] == dx_handle)
        {
            return (i);
        }
    }
    return (-1);
}


/****************************************************************
*        NAME : processDigit
* DESCRIPTION : Processes a DTMF digit and performs the required action
*       INPUT : device handle and DTMF digit.
*      OUTPUT :
*     RETURNS : void
*    CAUTIONS :
****************************************************************/
void processDigit(int dx_dev, char digit )
{
    int rc, offset;

    if (traceLevel > 50)
    {
		printf("processDigit: dx_dev %d, digit %c\n", dx_dev, digit);
	}


	if ((offset = getId(dx_dev)) < 0)
	{
		printf("Could not get the tts device handle\n");
		return;
	}

    switch( digit )
    {
        case '1' :
             pitch += 15;
             if (pitch > TTS_MAX_PITCH)
             {
                 pitch = TTS_DEFAULT_PITCH;
             }
             rc = tts_pitch(ttsHandle[offset], pitch);
             if (rc < 0)
             {
                 printf("ERROR(DIGIT 1): %s\n", tts_geterror(rc));
             }
             break;

        case '2' :
             pitch -= 15;
             if (pitch < TTS_MIN_PITCH)
             {
                 pitch = TTS_DEFAULT_PITCH;
             }
             rc = tts_pitch(ttsHandle[offset], pitch);
             if (rc < 0)
             {
                 printf("ERROR(DIGIT 2): %s\n", tts_geterror(rc));
             }
             break;

        case '3' :
             rc = tts_pause(ttsHandle[offset]);
             if (rc < 0)
             {
                 printf("ERROR: %s\n", tts_geterror(rc));
             }
             break;

        case '4' :
             rc = tts_resume(ttsHandle[offset]);
             if (rc < 0)
             {
                 printf("ERROR: %s\n", tts_geterror(rc));
             }
             break;

        case '5' :
             speechrate += 20;
             if (speechrate > TTS_MAX_SPEECH_RATE)
             {
                 speechrate = TTS_DEFAULT_SPEECH_RATE;
             }

             rc = tts_speechrate(ttsHandle[offset], speechrate);
             if (rc < 0)
             {
                 printf("ERROR: %s\n", tts_geterror(rc));
             }
             break;

        case '6' :
             speechrate -= 20;
             if (speechrate < TTS_MIN_SPEECH_RATE)
             {
                 speechrate = TTS_DEFAULT_SPEECH_RATE;
             }

             rc = tts_speechrate(ttsHandle[offset], speechrate);
             if (rc < 0)
             {
                 printf("ERROR: %s\n", tts_geterror(rc));
             }
             break;

        case '7' :
             rc = tts_jump(ttsHandle[offset], TTS_JUMP_CHARACTER, TTS_DEFAULT_JUMP_SIZE);
             if (rc < 0)
             {
                 printf("ERROR: %s\n", tts_geterror(rc));
             }
             break;

        case '8' :
             rc = tts_jump(ttsHandle[offset], TTS_JUMP_CHARACTER, -1*TTS_DEFAULT_JUMP_SIZE);
             if (rc < 0)
             {
                 printf("ERROR: %s\n", tts_geterror(rc));
             }
             break;

        case '9' :
             rc = tts_stop(ttsHandle[offset]);
             if (rc < 0)
             {
                 printf("tts_stop ERROR: %s\n", tts_geterror(rc));
             }
             break;

        case '0' :
			 tts_reset(ttsHandle[offset]);
             break;

        case '*' :
			 playDone[offset] = TRUE;
			 // reduce count only if the channel is open
				if (ttsHandle[offset]) NoOfThreads--;
             rc = tts_stop(ttsHandle[offset]);
             if (rc < 0)
             {
                 printf("tts_stop ERROR: %s\n", tts_geterror(rc));
             }
             break;

        case '#' :
             break;

        default :
             printf("ERROR: Got %c in dig buffer\n", digit);
             break;
    }

    return;
}


// FOR TESTING ONLY
/****************************************************************
*        NAME : getCommand
* DESCRIPTION : Called to process a message from the keyboard or telephone
*               and interpret the command out of it.
*       INPUT : Process number.
*      OUTPUT :
*     RETURNS : void
*    CAUTIONS : Needed only for Testing
****************************************************************/
void getCommand(int* ttsHdle)
{

int rcu_dev = *ttsHdle;

#if TELEPHONE

	printf("Please enter a DIGIT on the HANDSET\n");
  
#else
    char input[80], speakBuffer[5*1024];
    int numDigits, rc, fd, bytes, mode = EV_ASYNC;

    printf("Please enter a command for ch%d..", rcu_dev);
    gets( input );

    switch( input[0] )
    {

        case '1' :
			 int speechRate;
             rc = tts_getspeechrate(rcu_dev, &speechRate);
             if (rc < 0)
             {
                 printf("ERROR: %s\n", tts_geterror(rc));
             }
			 printf("ENGINE SPEECH RATE is %d\n", speechRate);
			 break;

        case '2' :
			 int speechPitch;
             rc = tts_getpitch(rcu_dev, &speechPitch);
            if (rc < 0)
             {
                 printf("ERROR: %s\n", tts_geterror(rc));
             }
 			 printf("ENGINE PITCH is %d\n", speechPitch);
			 break;

        case '3' :
			 int speechVolume;
             rc = tts_getvolume(rcu_dev, &speechVolume);
             if (rc < 0)
             {
                 printf("ERROR: %s\n", tts_geterror(rc));
             }
			 printf("ENGINE VOLUME is %d\n", speechVolume);
			 break;

        case '4' :
			 int speechVoice;
             rc = tts_getvoice(rcu_dev, &speechVoice);
             if (rc < 0)
             {
                 printf("ERROR: %s\n", tts_geterror(rc));
             }
			 printf("ENGINE VOICE is %d\n", speechVoice);
			 break;

        case 'b' :
        case 'B' :
             printf("Please enter a fileName ..");
             gets( input );
             fd = open(input, O_RDONLY);
             bytes = read(fd, speakBuffer, sizeof(speakBuffer));
             speakBuffer[bytes] = 0;
			 rc = tts_speakbuffer(rcu_dev, speakBuffer, mode, NULL);
			 if (rc < 0) 
			 {
				 printf("tts_speakbuffer: Error %d (%s)\n",
							rc, tts_geterror(rc));
			 }
             break;

        case 'c' :
             rc = tts_close(rcu_dev);
             if (rc < 0)
             {
                 printf("ERROR: %s\n", tts_geterror(rc));
             }
             break;

        case 'D' :
             printf("Please enter a delay ..");
             gets( input );
             Sleep(atoi(input)*1000);
	     printf("DELAY: %d added\n", atoi(input));
             break;

        case 'd' :
             speechrate = TTS_DEFAULT_SPEECH_RATE;
             rc = tts_speechrate(rcu_dev, speechrate);
             if (rc < 0)
             {
                 printf("ERROR: %s\n", tts_geterror(rc));
             }

             pitch = TTS_DEFAULT_PITCH;
             rc = tts_pitch(rcu_dev, pitch);
             if (rc < 0)
             {
                 printf("ERROR: %s\n", tts_geterror(rc));
             }
             break;

#if EMAIL
		size_t   buffer_size = 0;
		char *emailOut;

        case 'e' :
             printf("Please enter a fileName ..");
             gets( input );
             fd = open(input, O_RDONLY);
             bytes = read(fd, speakBuffer, sizeof(speakBuffer));
             speakBuffer[bytes] = 0;
			 rc = tts_speakemail(rcu_dev, speakBuffer);
			 if (rc < 0) 
			 {
	 			 printf("tts_speakemail: Error %d (%s)\n",
                        rc, tts_geterror(rc));
			 }
			 break;


        case 'E' :
             printf("Please enter a fileName ..");
             gets( input );
             fd = open(input, O_RDONLY);
             bytes = read(fd, speakBuffer, sizeof(speakBuffer));
             speakBuffer[bytes] = 0;
             buffer_size = (size_t)strlen(speakBuffer);
             buffer_size =(size_t)(buffer_size*2 < 4096) ? 4096 : buffer_size*2;

             if ((emailOut = (char *) malloc(buffer_size)) == NULL)
             {
	 			printf("tts_processemail: Malloc Error\n");
             }

			rc = tts_processemail(speakBuffer, emailOut, buffer_size, 1);
			if (rc < 0) 
 			{
	 			printf("tts_processemail: Error %d (%s)\n",
                        rc, tts_geterror(rc));
			}

             printf("\nEMAIL OUTPUT\n");
             printf("%s\n", emailOut);
             free(emailOut);
             break;
#endif

        case 'f' :
             printf("Please enter a fileName ..");
             gets( input );
             rc = tts_speakbuffer(rcu_dev, input, mode, NULL);
             if (rc < 0)
             {
                 printf("ERROR %d: %s\n", rc, tts_geterror(rc));
             }
             break;

        case 'g' :
             printf("Please enter an Error Code ..");
             gets( input );
             numDigits = -1*atoi(input);
             printf("tts_geterror(%d): %s\n", 
					numDigits, tts_geterror(numDigits));
             break;

        case 'j' :
             printf("Please enter a JumpSize ..");
             gets( input );
			 if (input[0] == 'B')
			 {
				printf("Jump to BOF");
				rc = tts_jump(rcu_dev, TTS_JUMP_BEGINNING, 0);
			 }
			 else if (input[0] == 'E')
			 {
				printf("Jump to EOF");
				rc = tts_jump(rcu_dev, TTS_JUMP_END, 0);
			 }
			 else
			 {
				numDigits = atoi(input);
				printf("Jump by %d", numDigits);
				rc = tts_jump(rcu_dev, TTS_JUMP_CHARACTER, numDigits);
			 }

             if (rc < 0)
             {
                 printf("ERROR %d: %s\n", rc, tts_geterror(rc));
             }
             break;

        case 'n' :
        case 'N' :
             break;

        case 'p' :
             printf("Please enter a pitch frequency ..");
             gets( input );
             pitch = atoi(input);
             rc = tts_pitch(rcu_dev, pitch);
             if (rc < 0)
             {
                 printf("ERROR %d: %s\n", rc, tts_geterror(rc));
             }
             break;

        case 'P' :
             rc = tts_pause(rcu_dev);
             if (rc < 0)
             {
                 printf("ERROR %d: %s\n", rc, tts_geterror(rc));
             }
             break;

        case 'q' :
        case 'Q' :
             break;

        case 'R' :
             rc = tts_resume(rcu_dev);
             if (rc < 0)
             {
                 printf("ERROR %d: %s\n", rc, tts_geterror(rc));
             }
             break;

        case 'r' :
             rc = tts_reset(rcu_dev);
             if (rc < 0)
             {
                 printf("ERROR %d: %s\n", rc, tts_geterror(rc));
             }
             break;

        case 's' :
             speechrate = TTS_DEFAULT_SPEECH_RATE;
             printf("Please enter a Speech rate ..");
             gets( input );
             speechrate = atoi(input);
             rc = tts_speechrate(rcu_dev, speechrate);
             if (rc < 0)
             {
                 printf("ERROR %d: %s\n", rc, tts_geterror(rc));
             }
             break;

        case 'S' :
             rc = tts_stop(rcu_dev);
             if (rc < 0)
             {
                 printf("ERROR: %s\n", tts_geterror(rc));
             }
             break;

        case 'v' :
             printf("Please enter an output voulme ..");
             gets( input );
             pitch = atoi(input);
             rc = tts_volume(rcu_dev, pitch);
             if (rc < 0)
             {
                 printf("ERROR %d: %s\n", rc, tts_geterror(rc));
             }
             break;

        case 'V' :
             printf("Please enter an output voice ..");
             gets( input );
             pitch = atoi(input);
             rc = tts_voice(rcu_dev, pitch);
             if (rc < 0)
             {
                 printf("ERROR %d: %s\n", rc, tts_geterror(rc));
             }
             break;


        case 'x' :
        case 'X' :
			playDone[rcu_dev] = TRUE;
            ExitProcess( 0 );
			break;

		// Ignore comments in the input file
        case '#' :
			break;

    }
#endif

    return;
}

