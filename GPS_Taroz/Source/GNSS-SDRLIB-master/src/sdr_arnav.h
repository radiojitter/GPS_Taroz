#ifndef SDR_ARNAV_H
#define SDR_ARNAV_H
#pragma once

/* Extensions by Arnav */

#ifdef __cpluplus
#define EXTERNCPP		extern "C"
#else
#define EXTERNCPP
#endif



/* Gain Changing helper (Gain_Change_Helper.cpp) */
typedef double (*GainChanger_UpdateGainCallback)(double percent);

EXTERNCPP void GainChanger_SetCallback(GainChanger_UpdateGainCallback _func);
EXTERNCPP void GainChanger_SetGain(int percent);
EXTERNCPP int	GainChanger_GetGain(void);
EXTERNCPP void GainChanger_Reset(void);


/* Frequency Changer helper (Frequency_Change_Helper.cpp) */
EXTERNCPP void FrequencyChanger_SetFrequency(double freq);
EXTERNCPP double FrequencyChanger_GetFrequency(void);


/* IRNSS API */
#define SYS_IRNSS			0x80				/* Navigation System: L5 IRNSS */
#define CTYPE_IRNSS			30					/* IRNSS L5 */
#define MAXIRNSSSATNO		7					/* Maximum number of allowed Sat ID for IRNSS */
#define LEN_IRNSS			1023				/* IRNSS SPS PRN Chip Size */
#define CRATE_IRNSS			1.023e6				/* IRNSS Code Chip Rate (chip/sec) */
#define LOOP_IRNSS		    10					/* IRNSS Track Settings : loop interval */
#define ACQINTG_IRNSS	    10                  /* Acquisition Settings for IRNSS : number of non-coherent integration */

#define NAVRATE_IRNSS    20						/* length (multiples of ranging code) */
#define NAVFLEN_IRNSS    300					/* navigation frame data (bits) */
#define NAVADDFLEN_IRNSS  2					/* additional bits of frame (bits) */
#define NAVPRELEN_IRNSS  16						/* preamble bits length (bits) */
#define NAVEPHCNT_IRNSS  3						/* number of eph. contained frame */
#define NSATIRNSS		 8						/* Number of IRNSS sats in constellation */

#define MINPRN_IRNSS		1					/* Minimum PRN for IRNSS */
#define MAXPRN_IRNSS		7					/* Maximum PRN for IRNSS */


//<forward PRN>#define DEFINE_IRNSS_PREAMBLE_VAR(_varname)		int _varname [16] = { 1,1,1,0, 1,0,1,1, 1,0,0,1, 0,0,0,0 };  /* Define a variable for IRNSS Preamble (in navigation struct) */

#define DEFINE_IRNSS_PREAMBLE_VAR(_varname)		int _varname [16] = { 0,0,0,0, 1,0,0,1, 1,1,0,1, 0,1,1,1 };  /* Define a variable for IRNSS Preamble (in navigation struct) */
#define DEFINE_IRNSS_CODE_initnavstruct()\
    if (ctype==CTYPE_IRNSS) {\
        nav->rate=NAVRATE_IRNSS;\
        nav->flen=NAVFLEN_IRNSS;\
        nav->addflen=NAVADDFLEN_IRNSS;\
        nav->prelen=NAVPRELEN_IRNSS;\
        nav->sdreph.cntth=NAVEPHCNT_IRNSS;\
        nav->update=(int)(nav->flen*nav->rate);\
        memcpy(nav->prebits,pre_l1ca,sizeof(int)*nav->prelen);\
		/* overlay code (all 1) */\
        nav->ocode=(short *)calloc(nav->rate,sizeof(short));\
        for (i=0;i<nav->rate;i++) nav->ocode[i]=1; /* not avaialbale for irnss, so make them all 1 */\
		SDRPRINTF("IRNSS IR%d: initnavstruct() setup complete\n", nav->sdreph.prn);\
    }


/* IRNSS data code generator */
EXTERNCPP short* gencode_IRNSS(int prn, int* len, double *crate);


#endif
