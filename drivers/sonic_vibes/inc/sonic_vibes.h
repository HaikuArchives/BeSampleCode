/* sonic_vibes.h -- specifics for S3-based PCI audio cards */
/* $Id$ */
/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#if !defined(_SONIC_VIBES_H)
#define _SONIC_VIBES_H

#include <Drivers.h>
#include <SupportDefs.h>
#include <OS.h>
#include "audio_driver.h"
#include "midi_driver.h"
#include "joystick_driver.h"


#define SONIC_VIBES_VENDOR_ID	0x5333	/* S3 Inc */
#define SONIC_VIBES_DEVICE_ID	0xCA00	/* Sonic Vibes */


#define SONIC_VIBES_JOYSTICK_MIN_LATENCY 5000		/* 200 times a second! */
#define SONIC_VIBES_JOYSTICK_MAX_LATENCY 100000		/* 10 times a second */

typedef struct joystick sonic_vibes_joystick;


typedef struct audio_format sonic_vibes_audio_format;
typedef struct audio_buf_header sonic_vibes_audio_buf_header;


/* the mux devices use these records */
typedef audio_routing sonic_vibes_routing;

/* this is the argument for ioctl() */
typedef audio_routing_cmd sonic_vibes_routing_cmd;


/* selectors for routing */
#define SONIC_VIBES_INPUT_MUX				B_AUDIO_INPUT_SELECT
#define SONIC_VIBES_MIC_BOOST				B_AUDIO_MIC_BOOST
#define SONIC_VIBES_MIDI_OUTPUT_TO_SYNTH	B_AUDIO_MIDI_OUTPUT_TO_SYNTH
#define SONIC_VIBES_MIDI_INPUT_TO_SYNTH		B_AUDIO_MIDI_INPUT_TO_SYNTH
#define SONIC_VIBES_MIDI_OUTPUT_TO_PORT		B_AUDIO_MIDI_OUTPUT_TO_PORT

/* input MUX source values */
#define SONIC_VIBES_INPUT_CD				B_AUDIO_INPUT_CD
#define SONIC_VIBES_INPUT_DAC				B_AUDIO_INPUT_DAC
#define SONIC_VIBES_INPUT_AUX2				B_AUDIO_INPUT_AUX2
#define SONIC_VIBES_INPUT_LINE				B_AUDIO_INPUT_LINE_IN
#define SONIC_VIBES_INPUT_AUX1				B_AUDIO_INPUT_AUX1
#define SONIC_VIBES_INPUT_MIC				B_AUDIO_INPUT_MIC
#define SONIC_VIBES_INPUT_MIX_OUT			B_AUDIO_INPUT_MIX_OUT


/* the mixer devices use these records */
typedef audio_level sonic_vibes_level;

/* this is the arg to ioctl() */
typedef audio_level_cmd sonic_vibes_level_cmd;

/* bitmask for the flags */
#define SONIC_VIBES_LEVEL_MUTED				B_AUDIO_LEVEL_MUTED

/* selectors for levels */
#define SONIC_VIBES_LEFT_ADC_INPUT_G		B_AUDIO_MIX_ADC_LEFT
#define SONIC_VIBES_RIGHT_ADC_INPUT_G 		B_AUDIO_MIX_ADC_RIGHT
#define SONIC_VIBES_LEFT_AUX1_LOOPBACK_GAM	B_AUDIO_MIX_VIDEO_LEFT
#define SONIC_VIBES_RIGHT_AUX1_LOOPBACK_GAM	B_AUDIO_MIX_VIDEO_RIGHT
#define SONIC_VIBES_LEFT_CD_LOOPBACK_GAM	B_AUDIO_MIX_CD_LEFT
#define SONIC_VIBES_RIGHT_CD_LOOPBACK_GAM	B_AUDIO_MIX_CD_RIGHT
#define SONIC_VIBES_LEFT_LINE_LOOPBACK_GAM	B_AUDIO_MIX_LINE_IN_LEFT
#define SONIC_VIBES_RIGHT_LINE_LOOPBACK_GAM	B_AUDIO_MIX_LINE_IN_RIGHT
#define SONIC_VIBES_MIC_LOOPBACK_GAM		B_AUDIO_MIX_MIC
#define SONIC_VIBES_LEFT_SYNTH_OUTPUT_GAM	B_AUDIO_MIX_SYNTH_LEFT
#define SONIC_VIBES_RIGHT_SYNTH_OUTPUT_GAM	B_AUDIO_MIX_SYNTH_RIGHT
#define SONIC_VIBES_LEFT_AUX2_LOOPBACK_GAM	B_AUDIO_MIX_AUX_LEFT
#define SONIC_VIBES_RIGHT_AUX2_LOOPBACK_GAM	B_AUDIO_MIX_AUX_RIGHT
#define SONIC_VIBES_LEFT_MASTER_VOLUME_AM	B_AUDIO_MIX_LINE_OUT_LEFT
#define SONIC_VIBES_RIGHT_MASTER_VOLUME_AM	B_AUDIO_MIX_LINE_OUT_RIGHT
#define SONIC_VIBES_LEFT_PCM_OUTPUT_GAM		B_AUDIO_MIX_DAC_LEFT
#define SONIC_VIBES_RIGHT_PCM_OUTPUT_GAM	B_AUDIO_MIX_DAC_RIGHT
#define SONIC_VIBES_DIGITAL_LOOPBACK_AM		B_AUDIO_MIX_LOOPBACK_LEVEL


/* secret handshake ioctl()s */
#define SV_SECRET_HANDSHAKE 10100
typedef struct {
	bigtime_t	wr_time;
	bigtime_t	rd_time;
	uint32		wr_skipped;
	uint32		rd_skipped;
	uint64		wr_total;
	uint64		rd_total;
	uint32		_reserved_[6];
} sv_handshake;
#define SV_RD_TIME_WAIT 10101
#define SV_WR_TIME_WAIT 10102
typedef struct {
	bigtime_t	time;
	bigtime_t	bytes;
	uint32		skipped;
	uint32		_reserved_[3];
} sv_timing;



#endif	/* _SONIC_VIBES_H */

