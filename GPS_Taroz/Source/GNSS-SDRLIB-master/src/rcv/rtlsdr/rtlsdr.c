/*------------------------------------------------------------------------------
* rtlsdr.c : Realtek RTL2832 based DVB dongle functions
*
* Copyright (C) 2014 Taro Suzuki <gnsssdrlib@gmail.com>
*-----------------------------------------------------------------------------*/
#include "../../../src/sdr.h"

static rtlsdr_dev_t *dev=NULL;

/* rtlsdr stream callback  -----------------------------------------------------
* callback for receiving RF data
*-----------------------------------------------------------------------------*/
void stream_callback_rtlsdr(unsigned char *buf, uint32_t len, void *ctx)
{
	/* Get the Thread Priority  and Boost it to Highest (Real time) priority */
	HANDLE hThread = GetCurrentThread();

	BOOL bBoostEnabled = FALSE;
	if(GetThreadPriorityBoost(hThread, &bBoostEnabled))
	{
		if(bBoostEnabled)
		{
			if(SetThreadPriorityBoost(hThread, FALSE))
			{
				SDRPRINTF("RTL-SDR: Thread %08X Priority BOOST is successfully DISABLED\n", (int) hThread);
			}
		}
	}

	int priority = GetThreadPriority(hThread);
	if(priority == THREAD_PRIORITY_TIME_CRITICAL || priority == THREAD_PRIORITY_HIGHEST)
	{
		// this is what i want
	}
	else
	{
		/* upgrade process priority class */
		HANDLE hCurProcess = GetCurrentProcess();

		if(GetPriorityClass(hCurProcess) != HIGH_PRIORITY_CLASS)
		{
			if(SetPriorityClass(hCurProcess, HIGH_PRIORITY_CLASS))
			{
				SDRPRINTF("RTL-SDR: Thread %08X upgraded process %08X to HIGH PRIORITY CLASS\n", (int) hThread, (int) hCurProcess);
			}
		}

		/* increase thread priority */
		if(SetThreadPriority(hThread, THREAD_PRIORITY_TIME_CRITICAL))
		{
			SDRPRINTF("RTL-SDR: Thread %08X Priority is successfully set to TIME CRITICAL\n", (int) hThread);
		}
		else
		{
			if(SetThreadPriority(hThread, THREAD_PRIORITY_HIGHEST))
			{
					SDRPRINTF("RTL-SDR: Thread %08X Priority is successfully set to HIGHEST\n", (int) hThread);

			}
		}
	}

    /* copy stream data to global buffer */
    mlock(hbuffmtx);
    memcpy(&sdrstat.buff[(sdrstat.buffcnt%MEMBUFFLEN)*2*RTLSDR_DATABUFF_SIZE],
        buf,2*RTLSDR_DATABUFF_SIZE);
    unmlock(hbuffmtx);

    mlock(hreadmtx);
    sdrstat.buffcnt++;
    unmlock(hreadmtx);

    if (sdrstat.stopflag) rtlsdr_cancel_async(dev);
}

/***** RTLSDR Gain Callback */
double rtlsdr_UpdateGain(double percent)
{
	double maxGain = 48.0;
	double minGain = 0.0;

	double actualGain = minGain + ((percent/100.0)*(maxGain-minGain));
	if(actualGain<minGain) actualGain = minGain;
	if(actualGain>maxGain) actualGain = maxGain;
	
	int ret;
	
	ret = rtlsdr_set_tuner_gain(dev, (int)(actualGain*10.0));
	if (ret != 0) 
	{
		SDRPRINTF("error: failed to set Gain of %g\n", actualGain);
        return -1;
    }


	double devGain = 0.1 * rtlsdr_get_tuner_gain(dev);

	percent = (devGain-minGain)*100.0/(maxGain-minGain);

	SDRPRINTF("RTLSDR: Gain updated to %g dB (%g %%)", actualGain, percent);
	return percent;
}

void rtlsdr_init_GainCallback(void)
{
	int r;

	r = rtlsdr_set_tuner_gain_mode(dev, 1);
	if (r < 0) {
		fprintf(stderr, "WARNING: Failed to enable manual gain.\n");
		return;
	}

	SDRPRINTF("RTLSDR: RTLSDR is successfully set to MANUAL Gain Mode\n");

	GainChanger_SetCallback(rtlsdr_UpdateGain);
}


/* rtlsdr initialization -------------------------------------------------------
* search front end and initialization
* args   : none
* return : int                  status 0:okay -1:failure
*-----------------------------------------------------------------------------*/
extern int rtlsdr_init(void) 
{
    int ret,dev_index=0;;

    /* open rtlsdr */
    dev_index=verbose_device_search("0");
    ret=rtlsdr_open(&dev, (uint32_t)dev_index);
    if (ret<0) {
        SDRPRINTF("error: failed to open rtlsdr device #%d.\n",dev_index);
        return -1;
    }

    /* set configuration */
    ret=rtlsdr_initconf();
    if (ret<0) {
        SDRPRINTF("error: failed to initialize rtlsdr\n");
        return -1;
    }

	/* register gain callback */
	rtlsdr_init_GainCallback();

	/* LOG the settings */
	SDRPRINTF("RTL-SDR Settings: Sample Rate = %g MHz, Frequency = %g MHz, Gain = %g dB\n",
		(rtlsdr_get_sample_rate(dev) * 1.0e-6),
		(rtlsdr_get_center_freq(dev) * 1.0e-6),
		(rtlsdr_get_tuner_gain(dev) * 0.1));

    return 0;
}



/* stop front-end --------------------------------------------------------------
* stop grabber of front end
* args   : none
* return : none
*-----------------------------------------------------------------------------*/
extern void rtlsdr_quit(void) 
{
	GainChanger_Reset();
    rtlsdr_cancel_async(dev);
    rtlsdr_close(dev);

	/* Since we are done ... reduce the priority class */
	HANDLE hCurProcess = GetCurrentProcess();

	if(GetPriorityClass(hCurProcess) != NORMAL_PRIORITY_CLASS)
	{
		if(SetPriorityClass(hCurProcess, NORMAL_PRIORITY_CLASS))
		{
			SDRPRINTF("RTL-SDR: Process %08X downgraded to NORMAL PRIORITY CLASS\n", (int) hCurProcess);
		}
	}
}
/* rtlsdr configuration function -----------------------------------------------
* load configuration file and setting
* args   : none
* return : int                  status 0:okay -1:failure
*-----------------------------------------------------------------------------*/
extern int rtlsdr_initconf(void) 
{
    int ret;
    
    /* Set the sample rate */
    ret=verbose_set_sample_rate(dev,RTLSDR_SAMPLE_RATE);
    if (ret<0) {
        SDRPRINTF("error: failed to set samplerate\n");
        return -1;
    }

    /* Set the frequency */
    ret=verbose_set_frequency(dev,RTLSDR_FREQUENCY);
    if (ret<0) {
        SDRPRINTF("error: failed to set frequency\n");
        return -1;
    }

    /* Enable automatic gain */
//    ret=verbose_auto_gain(dev);
//	double manual_gain_db = 42.0;
//	ret = verbose_gain_set(dev, int(manual_gain_db*10.0));
 //   if (ret<0) {
 //       SDRPRINTF("error: failed to set automatic gain\n");
 //       return -1;
 //   }


    /* set ppm offset */
    ret=verbose_ppm_set(dev,sdrini.rtlsdrppmerr);
    if (ret<0) {
        SDRPRINTF("error: failed to set ppm\n");
        return -1;
    }

    return 0;
}
/* start grabber ---------------------------------------------------------------
* start grabber of front end
* args   : none
* return : int                  status 0:okay -1:failure
*-----------------------------------------------------------------------------*/
extern int rtlsdr_start(void) 
{
    int ret;
    
    /* reset endpoint before we start reading from it (mandatory) */
    ret=verbose_reset_buffer(dev);
    if (ret<0) {
        SDRPRINTF("error: failed to reset buffers\n");
        return -1;
    }

    /* start stream and stay there until we kill the stream */
    ret=rtlsdr_read_async(dev,stream_callback_rtlsdr,
        NULL,RTLSDR_ASYNC_BUF_NUMBER,2*RTLSDR_DATABUFF_SIZE);

    if (ret<0&&!sdrstat.stopflag) {
        SDRPRINTF("error: failed to read in async mode\n");
        return -1;
    }

    return 0;
}
/* data expansion --------------------------------------------------------------
* get current data buffer from memory buffer
* args   : int16_t *buf     I   bladeRF raw buffer
*          int    n         I   number of grab data
*          char   *expbuf   O   extracted data buffer
* return : none
*-----------------------------------------------------------------------------*/
extern void rtlsdr_exp(uint8_t *buf, int n, char *expbuf)
{
    int i;

    for (i=0;i<n;i++) {
        expbuf[i]=(char)((buf[i]-127.5)); /* unsigned char to char */
    }
}
/* get current data buffer -----------------------------------------------------
* get current data buffer from memory buffer
* args   : uint64_t buffloc I   buffer location
*          int    n         I   number of grab data
*          char   *expbuf   O   extracted data buffer
* return : none
*-----------------------------------------------------------------------------*/
extern void rtlsdr_getbuff(uint64_t buffloc, int n, char *expbuf)
{
    uint64_t membuffloc=2*buffloc%(MEMBUFFLEN*2*RTLSDR_DATABUFF_SIZE);
    int nout;
    n=2*n;
    nout=(int)((membuffloc+n)-(MEMBUFFLEN*2*RTLSDR_DATABUFF_SIZE));

    mlock(hbuffmtx);
    if (nout>0) {
        rtlsdr_exp(&sdrstat.buff[membuffloc],n-nout,expbuf);
        rtlsdr_exp(&sdrstat.buff[0],nout,&expbuf[n-nout]);
    } else {
        rtlsdr_exp(&sdrstat.buff[membuffloc],n,expbuf);
    }
    unmlock(hbuffmtx);
}
/* push data to memory buffer --------------------------------------------------
* push data to memory buffer from STEREO binary IF file
* args   : none
* return : none
*-----------------------------------------------------------------------------*/
extern void frtlsdr_pushtomembuf(void) 
{
    size_t nread;

    mlock(hbuffmtx);

    nread=fread(
        &sdrstat.buff[(sdrstat.buffcnt%MEMBUFFLEN)*2*RTLSDR_DATABUFF_SIZE],
        1,2*RTLSDR_DATABUFF_SIZE,sdrini.fp1);

    unmlock(hbuffmtx);

    if (nread<2*RTLSDR_DATABUFF_SIZE) {
        sdrstat.stopflag=ON;
        SDRPRINTF("end of file!\n");
    }

    mlock(hreadmtx);
    sdrstat.buffcnt++;
    unmlock(hreadmtx);
}
