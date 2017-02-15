/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include "sv_private.h"
#include <string.h>
#include <byteorder.h>
#include "sound.h"
#include "R3MediaDefs.h"

#if !defined(_KERNEL_EXPORT_H)
#include <KernelExport.h>
#endif /* _KERNEL_EXPORT_H */

extern int sprintf(char *, const char *, ...);


extern void dump_card(sonic_vibes_dev * card);

#if !defined(OLDAPI)
 #if DEBUG
  #define OLDAPI(x) dprintf x
 #else
  #define OLDAPI(x)
 #endif
#endif

#if DEBUG
int32 int_cnt;
int32 put_cnt;
bigtime_t the_time;
#endif

#if 0
/* early Intel kernels forgot to export these functions */
#undef B_HOST_TO_LENDIAN_FLOAT
#undef B_HOST_TO_BENDIAN_FLOAT
#undef B_LENDIAN_TO_HOST_FLOAT
#undef B_BENDIAN_TO_HOST_FLOAT

static float
_swap_float_(float x)
{
	uint32 temp1 = *(uint32*)&x;
	uint32 temp2 = ((temp1>>24)|((temp1>>8)&0xff00)|((temp1<<8)&0xff0000)|
					(temp1<<24));
	return *(float *)&temp2;
}

#if B_HOST_IS_BENDIAN
#define B_HOST_TO_LENDIAN_FLOAT(x) _swap_float_(x)
#define B_HOST_TO_BENDIAN_FLOAT(x) ((float)(x))
#define B_LENDIAN_TO_HOST_FLOAT(x) _swap_float_(x)
#define B_BENDIAN_TO_HOST_FLOAT(x) ((float)(x))
#else
#define B_HOST_TO_LENDIAN_FLOAT(x) ((float)(x))
#define B_HOST_TO_BENDIAN_FLOAT(x) _swap_float_(x)
#define B_LENDIAN_TO_HOST_FLOAT(x) ((float)(x))
#define B_BENDIAN_TO_HOST_FLOAT(x) _swap_float_(x)
#endif

#endif


static status_t pcm_open(const char *name, uint32 flags, void **cookie);
static status_t pcm_close(void *cookie);
static status_t pcm_free(void *cookie);
static status_t pcm_control(void *cookie, uint32 op, void *data, size_t len);
static status_t pcm_read(void *cookie, off_t pos, void *data, size_t *len);
static status_t pcm_write(void *cookie, off_t pos, const void *data, size_t *len);
//static status_t pcm_writev(void *cookie, off_t pos, const iovec *vec, size_t count, size_t *len); /* */

device_hooks pcm_hooks = {
    &pcm_open,
    &pcm_close,
    &pcm_free,
    &pcm_control,
    &pcm_read,
    &pcm_write,
    NULL,			/* select */
    NULL,			/* deselect */
    NULL,			/* readv */
	NULL	// &pcm_writev		/* writev */
};

static pcm_cfg default_pcm = {
	44100.0,	/* sample rate */
	2,			/* channels */
	0x2,		/* format */		
#if B_HOST_IS_BENDIAN
	1,			/* endian (big) */
#else
	0,			/* endian (little) */
#endif
	0,			/* header size */
	PLAYBACK_BUF_SIZE,	/* these are currently hard-coded */
	RECORD_BUF_SIZE		/* and cannot be changed without re-compile */
};


#if 0

typedef struct {
	uint8 control;
	uint8 imask;
	uint8 regs[0x2e];
} chip_state;

static void
save_state(
	pcm_dev * port,
	chip_state * state)
{
	int ix;
	state->control = get_direct(port->card, 0);
	state->imask = get_direct(port->card, 1);
	for (ix=0; ix<0x2e; ix++) {
		if (ix != 0x28 && ix != 0x29 && ix != 0x1a && ix != 0x1b) {
			state->regs[ix] = get_indirect(port->card, ix);
		}
	}
}


static void
restore_state(
	pcm_dev * port,
	const chip_state * state)
{
	int ix;
	set_direct(port->card, 0, state->control, 0xff);
	for (ix=0; ix<0x2e; ix++) {
		if (ix != 0x28 && ix != 0x29 && ix != 0x1a && ix != 0x1b) {
			set_indirect(port->card, ix, state->regs[ix], 0xff);
		}
	}
	set_direct(port->card, 1, state->imask, 0xff);
}


static void
reset_chip(
	pcm_dev * port)
{
	set_direct(port->card, 0, 0x80, 0x80);
	snooze(2);
	set_direct(port->card, 0, 0x00, 0x80);
}

#endif


static void
stop_dma(
	pcm_dev * port)
{
	set_indirect(port->card, 0x13, 0, 0xff);
	PCI_IO_WR(port->dma_a+0xd, 0);	/* master clear */
	PCI_IO_WR(port->dma_c+0xd, 0);	/* master clear */

	ddprintf(("sonic_vibes: DMA stopped\n"));
}


static void
start_dma(
	pcm_dev * port)
{
	/* start out with a clean slate */

	KTRACE();
	ddprintf(("sonic_vibes: start_dma()\n"));
	if (port->config.format == 0x11) {
		memset((void*)port->card->low_mem, 0x80, port->config.play_buf_size + 
			port->config.rec_buf_size);
	}
	else {
		memset((void *)port->card->low_mem, 0, port->config.play_buf_size + 
			port->config.rec_buf_size);
	}

	port->wr_cur = port->wr_2;
	port->wr_silence = port->config.play_buf_size;
	port->was_written = 0;
	port->rd_cur = port->rd_2;
	port->was_read = port->config.rec_buf_size/2; /* how much has been read */
	port->wr_total = 0;
	port->rd_total = 0;

	/* we split the available low memory buffer in a small chunk */
	/* for playback, and a large chunk for recording. Then we split */
	/* each of those in half for double-buffering. While the DMA */
	/* just runs the entire range of the buffer, wrapping around when */
	/* done, the count register is set up to generate interrupt after */
	/* each half of the buffer. Because of latency requirements, we */
	/* will get 187 interrupts a second from playback, and 94 interrupts */
	/* a second from recording, at 48 kHz sampling rate, when buffers */
	/* are 2048 for playback and 4096 for record. */
	
	ddprintf(("play_buf_size %lx   rec_buf_size %lx\n", 
		port->config.play_buf_size/2, port->config.rec_buf_size/2));

	PCI_IO_WR(port->dma_c, ((uint32)port->card->low_phys+
		port->config.play_buf_size)&0xff);
	PCI_IO_WR(port->dma_c+1, (((uint32)port->card->low_phys+
		port->config.play_buf_size)>>8)&0xff);
	PCI_IO_WR(port->dma_c+2, (((uint32)port->card->low_phys+
		port->config.play_buf_size)>>16)&0xff);
	PCI_IO_WR(port->dma_a+3, 0);
	/* this is a 16 bit channel, so divide transfer count in 2 */
	PCI_IO_WR(port->dma_c+4, (port->config.rec_buf_size/2-1)&0xff);
	PCI_IO_WR(port->dma_c+5, ((port->config.rec_buf_size/2-1)>>8)&0xff);
	PCI_IO_WR(port->dma_a+6, 0);
	PCI_IO_WR(port->dma_c+11, 0x14);	/* auto-init IOR */
	/* the chip interrupt count is curiously big-endian (!) */
	/* we also divide the half-buffer count in 2 for words->bytes */
	set_indirect(port->card, 0x1c, ((port->config.rec_buf_size/4-1)>>8)&0xff, 0xff);
	set_indirect(port->card, 0x1d, (port->config.rec_buf_size/4-1)&0xff, 0xff);

	PCI_IO_WR(port->dma_a, ((uint32)port->card->low_phys)&0xff);
	PCI_IO_WR(port->dma_a+1, ((uint32)port->card->low_phys>>8)&0xff);
	PCI_IO_WR(port->dma_a+2, ((uint32)port->card->low_phys>>16)&0xff);
	PCI_IO_WR(port->dma_a+3, 0);
	PCI_IO_WR(port->dma_a+4, (port->config.play_buf_size-1)&0xff);
	PCI_IO_WR(port->dma_a+5, ((port->config.play_buf_size-1)>>8)&0xff);
	PCI_IO_WR(port->dma_a+6, 0);
	PCI_IO_WR(port->dma_a+11, 0x18);	/* auto-init IOW */
	/* the chip interrupt count is curiously big-endian (!) */
	set_indirect(port->card, 0x18, ((port->config.play_buf_size/2-1)>>8)&0xff, 0xff);
	set_indirect(port->card, 0x19, (port->config.play_buf_size/2-1)&0xff, 0xff);

/* here, we should mute the PCM output to avoid clicking */

	ddprintf(("sonic_vibes: DMA starts as %lx/%lx\n", port->config.format, port->open_mode));
	set_indirect(port->card, 0x13, port->open_mode, 0xff);

/* here, we should snooze for 16 samples' time, then un-mute the PCM output */
	KTRACE();
}


static status_t 
configure_pcm(
	pcm_dev * port,
	pcm_cfg * config,
	bool force)
{
	status_t err = B_OK;
	int m = 0, n = 0, r = 0;	/* parameters for the PLL sample rate synthesizer */
	int asr = -1;	/* alternate sample rate divisor */
	uint32 s;	/* size of buffer */

	ddprintf(("sonic_vibes: configure_pcm()\n"));

	/* check args */
	if (config->sample_rate < 4000.0) {
		config->sample_rate = default_pcm.sample_rate;
	}
	if (config->sample_rate > 48000.0) {
		config->sample_rate = 48000.0;
	}
	if (config->channels < 1) {
		config->channels = default_pcm.channels;
	}
	if (config->channels > 2) {
		config->channels = default_pcm.channels;
	}
	/* secret format of format: upper nybble = signed, unsigned, float */
	/* lower nybble = bytes per sample */
	if ((config->format != 0x11) && (config->format != 0x2) && 
		(config->format != 0x24) && (config->format != 0x4)) {
		config->format = default_pcm.format;
	}
	if (config->buf_header < 0) {
		config->buf_header = 0;
	}

	/* figure out buffer size that's a power of two and within size limits */
	if (!config->play_buf_size) {
		/* default is 256 samples for a comfy 6 ms latency */
		s = 256*config->channels*(config->format&0xf);
	}	/* minimum is 32 samples for a more extreme 0.75ms latency */
	else for (s = 32*config->channels*(config->format&0xf); s < MIN_MEMORY_SIZE/2; s = (s<<1)) {
		if (s >= config->play_buf_size) {
			break;
		}
	}
	config->play_buf_size = s;
	if (!config->rec_buf_size) {
		s = 256*config->channels*(config->format&0xf);
	}
	else for (s = 32*config->channels*(config->format&0xf); s < MIN_MEMORY_SIZE/2; s = (s<<1)) {
		if (s >= config->rec_buf_size) {
			break;
		}
	}
	config->rec_buf_size = s;

	/* calculate m, n and r (and asr) */

	if (!force && abs(config->sample_rate - port->config.sample_rate) < config->sample_rate/250) {
		n = -1;
	}
	else if (config->sample_rate == 48000.0) {
		asr = 0;
	}
	else if (config->sample_rate == 24000.0) {
		asr = 1;
	}
	else if (config->sample_rate == 16000.0) {
		asr = 2;
	}
	else if (config->sample_rate == 8000.0) {
		asr = 4;
	}
	else {
		float freq;
		float delta = 1000000.0;
		int sr = -1;
		int sn = -1;
		int sm = -1;
		float diff;

		for (r=0; r<8; r++) {
			if ((1<<r)*config->sample_rate*512 < MIN_FREQ) {
				continue;
			}
			break;
		}
		if (r == 8) {
			OLDAPI(("sonic_vibes: r value is 8!\n"));
			r = 7;
		}
		n = 0;
		do {
			n++;
			m = config->sample_rate*512/1000*(n+2)*(1<<r)/(F_REF/1000)-2;
			if (m < 1) {
				continue;
			}
			if (m > 255) {
				ddprintf(("sonic_vibes: m > 255; outahere\n"));
				break;
			}
			freq = (m+2)*(F_REF/1000)/(512*(n+2)*(1<<r)/1000);
			diff = freq-config->sample_rate;
			if (diff < 0) {
				diff = -diff;
			}
			if (diff < delta) {
				sr = r;
				sn = n;
				sm = m;
			}
		} while (n < 31);
		r = sr;
		n = sn;
		m = sm;
		ddprintf(("sonic_vibes: m = %d   r = %d   n = %d\n", m, r, n));
	}

	/* configure device */

	if (!force) {
		stop_dma(port);
		/* should mute PCM out, too */
	}
	if (asr > -1 || n > -1) { /* new sampling rate */
		if (asr > -1) {
			port->config.sample_rate = config->sample_rate;
			set_indirect(port->card, 0x23, (asr<<4), 0xff);
			set_indirect(port->card, 0x22, 0x10, 0xff);
		}
		else {
			port->config.sample_rate = ((float)m+2.0)*(F_REF/1000.0)/
				(0.512*(n+2.0)*(1<<r));
			config->sample_rate = port->config.sample_rate;
			set_indirect(port->card, 0x24, m, 0xff);
			set_indirect(port->card, 0x25, (r<<5)|n, 0xff);
			set_indirect(port->card, 0x22, 0x00, 0xff);
		}
		set_indirect(port->card, 0x1e, ((uint32)port->config.sample_rate*
			65535/48000), 0xff);
		set_indirect(port->card, 0x1f, ((uint32)port->config.sample_rate*
			65535/48000)>>8, 0xff);
	}
	if (force || config->channels != port->config.channels ||
		config->format != port->config.format) {
		uchar val = 0;
		if (config->channels == 2) {
			val |= 0x11;	/* stereo */
		}
		if (config->format != 0x11) {
			val |= 0x22;	/* 16 bits */
		}
		set_indirect(port->card, 0x52, val, 0xff); /* MCE -- may take time to take effect */
		spin(1);
		set_indirect(port->card, 0x52, val, 0xff); /* MCE -- so we re-try it */
		port->config.channels = config->channels;
		port->config.format = config->format;
		val = get_indirect(port->card, 0x12); /* clear MCE */
	}
	if (force || config->big_endian != port->config.big_endian) {
		port->config.big_endian = config->big_endian;
	}
	if (force || config->buf_header != port->config.buf_header) {
		port->config.buf_header = config->buf_header;
	}
	if (force || config->play_buf_size != port->config.play_buf_size*2) {
		port->config.play_buf_size = config->play_buf_size*2;	/* because we break it in two */
	}
	if (force || config->rec_buf_size != port->config.rec_buf_size*2) {
		port->config.rec_buf_size = config->rec_buf_size*2;	/* because we break it in two */
	}
ddprintf(("port->config.play_buf_size = %ld\n", port->config.play_buf_size));

/* here is where we should care about record and playback buffer sizes */

	ddprintf(("sonic_vibes: play %04lx rec %04lx\n", port->config.play_buf_size/2, 
		port->config.rec_buf_size/2));

	port->wr_1 = port->card->low_mem;
	port->wr_2 = port->wr_1+port->config.play_buf_size/2;
	port->wr_size = port->config.play_buf_size/2;

	port->rd_1 = port->card->low_mem+port->config.play_buf_size;
	port->rd_2 = port->rd_1+port->config.rec_buf_size/2;
	port->rd_size = port->config.rec_buf_size/2;

	if (!force) {
		/* should un-mute PCM out, if we muted it */
		start_dma(port);
	}
	return err;
}


static status_t
pcm_open(
	const char * name,
	uint32 flags,
	void ** cookie)
{
	int ix;
	pcm_dev * port = NULL;
	char name_buf[256];
	int32 prev_mode;

	ddprintf(("sonic_vibes: pcm_open()\n"));

	*cookie = NULL;
	for (ix=0; ix<num_cards; ix++) {
		if (!strcmp(name, cards[ix].pcm.name)) {
			goto gotit;
		}
	}
	for (ix=0; ix<num_cards; ix++) {
		if (!strcmp(name, cards[ix].pcm.oldname)) {
			goto gotit;
		}
	}
	ddprintf(("sonic_vibes: %s not found\n", name));
	return ENODEV;

gotit:
	*cookie = port = &cards[ix].pcm;

	acquire_sem(port->init_sem);

	prev_mode = port->open_mode;
	if ((flags & 3) == O_RDONLY) {
		atomic_or(&port->open_mode, kRecord);
	}
	else if ((flags & 3) == O_WRONLY) {
		atomic_or(&port->open_mode, kPlayback);
	}
	else {
		atomic_or(&port->open_mode, kPlayback|kRecord);
	}

	if (atomic_add(&port->open_count, 1) == 0) {

		/* initialize device first time */

		port->card = &cards[ix];
		port->config = default_pcm;
		port->config.play_buf_size *= 2;
		port->config.rec_buf_size *= 2;

		/* playback */

		port->wr_lock = 0;
		port->dma_a = cards[ix].dma_base;
		port->wr_1 = cards[ix].low_mem;
		port->wr_2 = cards[ix].low_mem+port->config.play_buf_size/2;
		port->wr_size = port->config.play_buf_size/2;
		port->write_waiting = 0;
		sprintf(name_buf, "WS:%s", port->name);
		name_buf[B_OS_NAME_LENGTH-1] = 0;
		port->write_sem = create_sem(0, name_buf);
		if (port->write_sem < B_OK) {
			port->open_count = 0;
			return port->write_sem;
		}
		set_sem_owner(port->write_sem, B_SYSTEM_TEAM);
		name_buf[0] = 'W'; name_buf[1] = 'E';
		port->wr_entry = create_sem(1, name_buf);
		if (port->wr_entry < B_OK) {
			delete_sem(port->write_sem);
			port->open_count = 0;
			return port->wr_entry;
		}
		set_sem_owner(port->wr_entry, B_SYSTEM_TEAM);
		name_buf[1] = 'T';
		port->wr_time_wait = 0;
		port->wr_time_sem = create_sem(0, name_buf);
		if (port->wr_time_sem < B_OK) {
			delete_sem(port->write_sem);
			delete_sem(port->wr_entry);
			port->open_count = 0;
			return port->wr_time_sem;
		}
		set_sem_owner(port->wr_time_sem, B_SYSTEM_TEAM);
		
		/* recording */

		port->rd_lock = 0;
		port->dma_c = cards[ix].dma_base+0x10;
		port->rd_1 = cards[ix].low_mem+port->config.play_buf_size;
		port->rd_2 = cards[ix].low_mem+port->config.play_buf_size+port->config.rec_buf_size/2;
		port->rd_size = port->config.rec_buf_size/2;
		port->read_waiting = 0;
		name_buf[0] = 'R'; name_buf[1] = 'S';
		port->read_sem = create_sem(0, name_buf);
		if (port->read_sem < B_OK) {
			delete_sem(port->write_sem);
			delete_sem(port->wr_entry);
			delete_sem(port->wr_time_sem);
			port->open_count = 0;
			return port->read_sem;
		}
		set_sem_owner(port->read_sem, B_SYSTEM_TEAM);
		name_buf[0] = 'R'; name_buf[1] = 'E';
		port->rd_entry = create_sem(1, name_buf);
		if (port->rd_entry < B_OK) {
			delete_sem(port->write_sem);
			delete_sem(port->wr_entry);
			delete_sem(port->read_sem);
			delete_sem(port->wr_time_sem);
			port->open_count = 0;
			return port->rd_entry;
		}
		set_sem_owner(port->rd_entry, B_SYSTEM_TEAM);
		name_buf[1] = 'T';
		port->rd_time_wait = 0;
		port->rd_time_sem = create_sem(0, name_buf);
		if (port->rd_time_sem < B_OK) {
			delete_sem(port->write_sem);
			delete_sem(port->wr_entry);
			delete_sem(port->read_sem);
			delete_sem(port->wr_time_sem);
			delete_sem(port->rd_entry);
			port->open_count = 0;
			return port->rd_time_sem;
		}
		set_sem_owner(port->rd_time_sem, B_SYSTEM_TEAM);

		port->rd_time = 0;
		port->next_rd_time = 0;
		port->wr_time = 0;

		/* old API */

		port->old_cap_sem = -1;
		port->old_play_sem = -1;

		/* configuration */

		configure_pcm(port, &default_pcm, true);

		/* interrupts */
		KTRACE();
		increment_interrupt_handler(port->card);
		set_direct(port->card, 0x01, 0x00, 0x05);

		start_dma(port);
		
		/* initialization is done, let other clients of the driver go */
	} else {
		if (prev_mode != port->open_mode) {
			pcm_cfg temp = port->config;
			temp.play_buf_size /= 2;
			temp.rec_buf_size /= 2;
			configure_pcm(port, &temp, false);	/* change rec/play if needed */
		}
	}
	release_sem(port->init_sem);

#if DEBUG
	dump_card(&cards[ix]);
#endif

	return B_OK;
}


static status_t
pcm_close(
	void * cookie)
{
	pcm_dev * port = (pcm_dev *)cookie;
	cpu_status cp;
	int spin = 0;

	ddprintf(("sonic_vibes: pcm_close()\n"));

	acquire_sem(port->init_sem);

	if (atomic_add(&port->open_count, -1) == 1) {

		KTRACE();
		cp = disable_interrupts();
		acquire_spinlock(&port->card->hardware);

		/* turn off interrupts */
		stop_dma(port);
		set_direct(port->card, 0x01, 0x05, 0x05);	/* */

		if (port->config.format == 0x11) {
			memset((void *)port->wr_1, 0x80, port->config.play_buf_size);	/* play silence */
		}
		else {
			memset((void *)port->wr_1, 0, port->config.play_buf_size);	/* play silence */
		}
		spin = 1;

		release_spinlock(&port->card->hardware);
		restore_interrupts(cp);

		delete_sem(port->write_sem);
		delete_sem(port->read_sem);
		delete_sem(port->wr_entry);
		delete_sem(port->rd_entry);
		delete_sem(port->rd_time_sem);
		delete_sem(port->wr_time_sem);
		port->write_sem = -1;
		port->read_sem = -1;
		port->wr_entry = -1;
		port->rd_entry = -1;
		port->rd_time_sem = -1;
		port->wr_time_sem = -1;
	}
	release_sem(port->init_sem);

	if (spin) {
		/* wait so we know FIFO gets filled with silence */
		snooze(port->config.play_buf_size*1000/(port->config.sample_rate*
			(port->config.format&0xf)*port->config.channels/1000));
	}
	return B_OK;
}


static status_t
pcm_free(
	void * cookie)
{
	cpu_status cp;
	pcm_dev * port = (pcm_dev *)cookie;

	ddprintf(("sonic_vibes: pcm_free()\n"));

	acquire_sem(port->init_sem);

	if (((pcm_dev *)cookie)->open_count == 0) {

		/* the last free will actually stop everything  */

		KTRACE();
		cp = disable_interrupts();
		acquire_spinlock(&port->card->hardware);
	
		decrement_interrupt_handler(port->card);
	
		release_spinlock(&port->card->hardware);
		restore_interrupts(cp);
	}
	release_sem(port->init_sem);

	return B_OK;
}


static status_t
pcm_control(
	void * cookie,
	uint32 iop,
	void * data,
	size_t len)
{
	pcm_dev * port = (pcm_dev *)cookie;
	status_t err = B_BAD_VALUE;
	pcm_cfg config = port->config;
	static float rates[4] = { 48000.0, 24000.0, 16000.0, 8000.0 };
	bool configure = false;
	config.play_buf_size /= 2;
	config.rec_buf_size /= 2;

	ddprintf(("sonic_vibes: pcm_control()\n"));

	switch (iop) {
	case B_AUDIO_GET_AUDIO_FORMAT:
		memcpy(data, &config, sizeof(port->config));
		err = B_OK;
		break;
	case B_AUDIO_GET_PREFERRED_SAMPLE_RATES:
		memcpy(data, rates, sizeof(rates));
		err = B_OK;
		break;
	case B_AUDIO_SET_AUDIO_FORMAT:
		memcpy(&config, data, sizeof(config));
		configure = true;
		err = B_OK;
		break;
	case SV_RD_TIME_WAIT:
		atomic_add(&port->rd_time_wait, 1);
		err = acquire_sem(port->rd_time_sem);
		if (err >= B_OK) {
			cpu_status cp;
			KTRACE();
			cp = disable_interrupts();
			acquire_spinlock(&port->rd_lock);
			((sv_timing *)data)->time = port->rd_time;
			((sv_timing *)data)->bytes = port->rd_total;
			((sv_timing *)data)->skipped = port->rd_skipped;
			((sv_timing *)data)->_reserved_[0] = 0xffffffffUL;
			release_spinlock(&port->rd_lock);
			restore_interrupts(cp);
		}
		break;
	case SV_WR_TIME_WAIT:
		atomic_add(&port->wr_time_wait, 1);
		err = acquire_sem(port->wr_time_sem);
		if (err >= B_OK) {
			cpu_status cp;
			KTRACE();
			cp = disable_interrupts();
			acquire_spinlock(&port->wr_lock);
			((sv_timing *)data)->time = port->wr_time;
			((sv_timing *)data)->bytes = port->wr_total;
			((sv_timing *)data)->skipped = port->wr_skipped;
			((sv_timing *)data)->_reserved_[0] = 0xffffffffUL;
			release_spinlock(&port->wr_lock);
			restore_interrupts(cp);
		}
		break;
	case SV_SECRET_HANDSHAKE: {
		cpu_status cp;
		KTRACE();
		cp = disable_interrupts();
		acquire_spinlock(&port->wr_lock);
		acquire_spinlock(&port->rd_lock);
		((sv_handshake *)data)->wr_time = port->wr_time;
		((sv_handshake *)data)->wr_skipped = port->wr_skipped;
		((sv_handshake *)data)->rd_time = port->rd_time;
		((sv_handshake *)data)->rd_skipped = port->rd_skipped;
		((sv_handshake *)data)->wr_total = port->wr_total;
		((sv_handshake *)data)->rd_total = port->rd_total;
		((sv_handshake *)data)->_reserved_[0] = 0xffffffffUL;
		err = B_OK;
		release_spinlock(&port->rd_lock);
		release_spinlock(&port->wr_lock);
		restore_interrupts(cp);
		} break;
	case SOUND_GET_PARAMS: {
		cpu_status cp;
		uchar u;
		sound_setup * sound = (sound_setup *)data;
		err = B_OK;
		cp = disable_interrupts();
		acquire_spinlock(&port->card->hardware);
		/* Here we get to hard-code the mix/mux values. */
		/* Huh-huh; he said "hard-code"! */
		sound->sample_rate = kHz_44_1;
		if (!port->config.big_endian == !B_HOST_IS_BENDIAN) {
			sound->playback_format = linear_16bit_big_endian_stereo;
			sound->capture_format = linear_16bit_big_endian_stereo;
		}
		else {
			sound->playback_format = linear_16bit_little_endian_stereo;
			sound->capture_format = linear_16bit_little_endian_stereo;
		}
		sound->dither_enable = false;
		sound->loop_attn = 0;
		sound->loop_enable = 0;
		sound->output_boost = 0;
		sound->highpass_enable = 0;
		/* this is a stereo control on SonicVibes... */
		u = get_indirect(port->card, 0xE);
		sound->mono_gain = 63-(((u&0x1f)<<1)|((u>>4)&1));
		sound->mono_mute = u&0x80;
		/* left channel */
		u = get_indirect(port->card, 0x0);
		switch ((u>>5)&7) {
		case 4:
			sound->left.adc_source = line;
			break;
		case 1:
			sound->left.adc_source = aux1;
			break;
		case 6:
			sound->left.adc_source = mic;
			break;
		default:
			sound->left.adc_source = loopback;
			break;
		}
		sound->left.adc_gain = u&15;
		sound->left.mic_gain_enable = u&16;
		u = get_indirect(port->card, 0x4);
		sound->left.aux1_mix_gain = u&31;
		sound->left.aux1_mix_mute = u&128;
		u = get_indirect(port->card, 0xC);
		sound->left.aux2_mix_gain = u&31;
		sound->left.aux2_mix_mute = u&128;
		u = get_indirect(port->card, 0x6);
		sound->left.line_mix_gain = u&31;
		sound->left.line_mix_mute = u&128;
		u = get_indirect(port->card, 0x10);
		sound->left.dac_attn = u&63;
		sound->left.dac_mute = u&128;
		/* right channel */
		u = get_indirect(port->card, 0x1);
		switch ((u>>5)&7) {
		case 4:
			sound->right.adc_source = line;
			break;
		case 1:
			sound->right.adc_source = aux1;
			break;
		case 6:
			sound->right.adc_source = mic;
			break;
		default:
			sound->right.adc_source = loopback;
			break;
		}
		sound->right.adc_gain = u&15;
		sound->right.mic_gain_enable = sound->left.mic_gain_enable;
		u = get_indirect(port->card, 0x5);
		sound->right.aux1_mix_gain = u&31;
		sound->right.aux1_mix_mute = u&128;
		u = get_indirect(port->card, 0xD);
		sound->right.aux2_mix_gain = u&31;
		sound->right.aux2_mix_mute = u&128;
		u = get_indirect(port->card, 0x7);
		sound->right.line_mix_gain = u&31;
		sound->right.line_mix_mute = u&128;
		u = get_indirect(port->card, 0x11);
		sound->right.dac_attn = u&63;
		sound->right.dac_mute = u&128;
		/* done */
		release_spinlock(&port->card->hardware);
		restore_interrupts(cp);
		} break;
	case SOUND_SET_PARAMS: {
		cpu_status cp;
		uchar u;
		sound_setup * sound = (sound_setup *)data;
		err = B_OK;
		cp = disable_interrupts();
		acquire_spinlock(&port->card->hardware);
		/* Here we get to hard-code the mix/mux values. */
		/* Huh-huh; he said "hard-code"! */
		/* ignore sample rate */
		sound->sample_rate = kHz_44_1;
		if (config.sample_rate < 43999 || config.sample_rate > 44201) {
			config.sample_rate = 44100.0;
			configure = true;
		}
		/* we only support 16-bit formats */
		if (sound->playback_format == linear_16bit_big_endian_stereo &&
			sound->capture_format == linear_16bit_big_endian_stereo) {
			if (!config.big_endian != !B_HOST_IS_BENDIAN || config.format != 0x2) {
				config.big_endian = B_HOST_IS_BENDIAN;
				config.format = 0x2;
				configure = true;
			}
			OLDAPI(("same_endian\n"));
		}
		else if (sound->playback_format == linear_16bit_little_endian_stereo &&
			sound->capture_format == linear_16bit_little_endian_stereo) {
			if (!config.big_endian != !!B_HOST_IS_BENDIAN || config.format != 0x2) {
				config.big_endian = !B_HOST_IS_BENDIAN;
				config.format = 0x2;
				configure = true;
			}
			OLDAPI(("other_endian\n"));
		}
		else {
			config.big_endian = !!B_HOST_IS_BENDIAN;
			configure = true;
			OLDAPI(("other format!!!\n"));
		}
		/* ignore these values */
		sound->dither_enable = false;
		sound->loop_attn = 0;
		sound->loop_enable = 0;
		sound->output_boost = 0;
		sound->highpass_enable = 0;
		/* this is a stereo control on SonicVibes... */
		u = 0; // (31-(sound->mono_gain>>1))&0x1f;
		if (sound->mono_mute) u |= 0x80;
		OLDAPI(("output: %x\n", u));
		set_indirect(port->card, 0xE, u, 0xff);
		set_indirect(port->card, 0xF, u, 0xff);
		/* left channel */
		switch (sound->left.adc_source) {
		case line:
			u = 4<<5;
			break;
		case aux1:
			u = 1<<5;
			break;
		case mic:
			u = 6<<5;
			break;
		default:
			u = 7<<5;
			break;
		}
		u |= (sound->left.adc_gain&15);
		if (sound->left.mic_gain_enable) u |= 0x20;
		OLDAPI(("input: %x\n", u));
		set_indirect(port->card, 0x0, u, 0xff);
		u = sound->left.aux1_mix_gain&31;
		if (sound->left.aux1_mix_mute) u |= 128;
		OLDAPI(("cd: %x\n", u));
		set_indirect(port->card, 0x4, u, 0xff);
		u = sound->left.aux2_mix_gain&31;
		if (sound->left.aux2_mix_mute) u |= 128;
		OLDAPI(("aux2: %x\n", u));
		set_indirect(port->card, 0xC, u, 0xff);
		u = sound->left.line_mix_gain&31;
		if (sound->left.line_mix_mute) u |= 128;
		OLDAPI(("line: %x\n", u));
		set_indirect(port->card, 0x6, u, 0xff);
		u = sound->left.dac_attn & 63;
		if (sound->left.dac_mute) u |= 128;
		OLDAPI(("PCM: %x\n", u));
		set_indirect(port->card, 0x10, u, 0xff);
		/* right channel */
		switch (sound->right.adc_source) {
		case line:
			u = 4<<5;
			break;
		case aux1:
			u = 1<<5;
			break;
		case mic:
			u = 6<<5;
			break;
		default:
			u = 7<<5;
			break;
		}
		u |= (sound->right.adc_gain&15);
		/* no right mic gain */
		sound->right.mic_gain_enable = sound->left.mic_gain_enable;
		set_indirect(port->card, 0x1, u, 0xff);
		u = sound->right.aux1_mix_gain&31;
		if (sound->right.aux1_mix_mute) u |= 128;
		set_indirect(port->card, 0x5, u, 0xff);
		u = sound->right.aux2_mix_gain&31;
		if (sound->right.aux2_mix_mute) u |= 128;
		set_indirect(port->card, 0xD, u, 0xff);
		u = sound->right.line_mix_gain&31;
		if (sound->right.line_mix_mute) u |= 128;
		set_indirect(port->card, 0x7, u, 0xff);
		u = sound->right.dac_attn & 63;
		if (sound->right.dac_mute) u |= 128;
		set_indirect(port->card, 0x11, u, 0xff);
		/* done */
		release_spinlock(&port->card->hardware);
		restore_interrupts(cp);
		} break;
	case SOUND_SET_PLAYBACK_COMPLETION_SEM:
		port->old_play_sem = *(sem_id *)data;
		err = B_OK;
		break;
	case SOUND_SET_CAPTURE_COMPLETION_SEM:
		port->old_cap_sem = *(sem_id *)data;
		err = B_OK;
		break;
//	case SOUND_GET_PLAYBACK_TIMESTAMP:
//		break;
//	case SOUND_GET_CAPTURE_TIMESTAMP:
//		break;
//	case SOUND_DEBUG_ON:
//		break;
//	case SOUND_DEBUG_OFF:
//		break;
	case SOUND_UNSAFE_WRITE: {
		audio_buffer_header * buf = (audio_buffer_header *)data;
		size_t n = buf->reserved_1-sizeof(*buf);
		pcm_write(cookie, 0, buf+1, &n);
		buf->time = port->wr_time;
		buf->sample_clock = port->wr_total/4 * 10000 / 441;
		err = release_sem(port->old_play_sem);
		} break;
	case SOUND_UNSAFE_READ: {
		audio_buffer_header * buf = (audio_buffer_header *)data;
		size_t n = buf->reserved_1-sizeof(*buf);
		pcm_read(cookie, 0, buf+1, &n);
		buf->time = port->rd_time;
		buf->sample_clock = port->rd_total/4 * 10000 / 441;
		err = release_sem(port->old_cap_sem);
		} break;
	case SOUND_LOCK_FOR_DMA:
		err = B_OK;
		break;
	case SOUND_SET_PLAYBACK_PREFERRED_BUF_SIZE:
		config.play_buf_size = (int32)data;
		configure = true;
		err = B_OK;
		break;
	case SOUND_SET_CAPTURE_PREFERRED_BUF_SIZE:
		config.rec_buf_size = (int32)data;
		configure = true;
		err = B_OK;
		break;
	case SOUND_GET_PLAYBACK_PREFERRED_BUF_SIZE:
		*(int32*)data = config.play_buf_size;
		err = B_OK;
		break;
	case SOUND_GET_CAPTURE_PREFERRED_BUF_SIZE:
		*(int32*)data = config.rec_buf_size;
		err = B_OK;
		break;
	default:
		OLDAPI(("sonic_vibes: unknown code %ld\n", iop));
		err = B_BAD_VALUE;
		break;
	}
	if ((err == B_OK) && configure) {
		cpu_status cp;
		KTRACE();
		cp = disable_interrupts();
		acquire_spinlock(&port->card->hardware);
		err = configure_pcm(port, &config, false);
		release_spinlock(&port->card->hardware);
		restore_interrupts(cp);
	}
	return err;
}


static void
copy_short_to_float(
	float * f,
	const short * s,
	int c,
	int endian)	/*	endian means float data in big-endian	*/
{
	if (endian) {
		while (c > 1) {
			short sh = B_LENDIAN_TO_HOST_FLOAT(*s);
			*(f++) = B_HOST_TO_BENDIAN_FLOAT((float)B_LENDIAN_TO_HOST_INT16(sh));
			s++;
			c -= 2;
		}
	}
	else {
		while (c > 1) {
			short sh = B_LENDIAN_TO_HOST_FLOAT(*s);
			*(f++) = B_HOST_TO_LENDIAN_FLOAT((float)B_LENDIAN_TO_HOST_INT16(sh));
			s++;
			c -= 2;
		}
	}
}


static void
copy_float_to_short(
	short * s,
	const float * f,
	int c,
	int endian)	/*	endian means float data in big-endian	*/
{
	if (endian) {
		while (c > 1) {
			float fl = *(f++);
			*(s++) = B_HOST_TO_LENDIAN_INT16((float)B_BENDIAN_TO_HOST_FLOAT(fl));
			c -= 2;
		}
	}
	else {
		while (c > 1) {
			float fl = *(f++);
			*(s++) = B_HOST_TO_LENDIAN_INT16((float)B_LENDIAN_TO_HOST_FLOAT(fl));
			c -= 2;
		}
	}
}


static void
swap_copy(
	short * dest,
	const short * src,
	int c)
{
	while (c > 1) {
		unsigned short sh = *(src++);
		*(dest++) = ((sh << 8) | (sh >> 8));
		c -= 2;
	}
}


static status_t
pcm_read(
	void * cookie,
	off_t pos,
	void * data,
	size_t * nread)
{
	pcm_dev * port = (pcm_dev *)cookie;
	size_t to_read = *nread;
	status_t err;
	int block;
	cpu_status cp;
	int bytes_xferred;
	void * hdrptr = data;
	int hdrsize = port->config.buf_header;
	sonic_vibes_audio_buf_header hdr;

//	ddprintf(("sonic_vibes: pcm_read()\n")); /* we're here */

	*nread = 0;
	data = ((char *)data)+hdrsize;
	to_read -= hdrsize;

	err = acquire_sem_etc(port->rd_entry, 1, B_CAN_INTERRUPT, 0);
	if (err < B_OK) {
		return err;
	}

	hdr.capture_time = port->rd_time;

	goto first_time;

	while (to_read > 0) {
		/* wait for more data */
		atomic_add(&port->read_waiting, 1);
		err = acquire_sem_etc(port->read_sem, 1, B_CAN_INTERRUPT, 0);
		if (err < B_OK) {
			release_sem(port->rd_entry);
			return err;
		}

first_time:	/* we need to check whether anything's available first */
		KTRACE();
		cp = disable_interrupts();
		acquire_spinlock(&port->rd_lock);

		block = port->rd_size-port->was_read;

		if (port->config.format == 0x24) {
			if (block > (to_read>>1)) {	/*	floats expand by factor 2	*/
				block = to_read>>1;
			}
		}
		else if (block > to_read) {
			block = to_read;
		}
		switch (port->config.format) {
		case 0x24:	/*	floats	*/
			copy_short_to_float((float *)data, (const short *)(port->rd_cur+port->was_read), 
				block, !B_HOST_IS_LENDIAN == !port->config.big_endian);
			bytes_xferred = block * 2;
			break;
		case 0x02:	/*	shorts	*/
			if (!B_HOST_IS_LENDIAN == !port->config.big_endian) {
				/*	we need to swap	*/
				swap_copy((short *)data, (const short *)(port->rd_cur+port->was_read), block);
				bytes_xferred = block;
				break;
			}
			/*	else fall through to default case	*/
		case 0x11:	/*	bytes	*/
		default:
			memcpy(data, (void *)(port->rd_cur+port->was_read), block);
			bytes_xferred = block;
			break;
		}
		port->was_read += block;

		release_spinlock(&port->rd_lock);
		restore_interrupts(cp);

		to_read -= bytes_xferred;
		data = ((char *)data)+bytes_xferred;
		*nread += bytes_xferred;
	}

	/*	provide header if requested	*/
	if (hdrsize > 0) {
		ddprintf(("header %d\n", hdrsize));
		*nread += hdrsize;
		hdr.capture_size = *nread;
		hdr.sample_rate = port->config.sample_rate;
		if (hdrsize > sizeof(hdr)) {
			hdrsize = sizeof(hdr);
		}
		memcpy(hdrptr, &hdr, hdrsize);
	}

	release_sem(port->rd_entry);

	return B_OK;
}


static status_t
pcm_write(
	void * cookie,
	off_t pos,
	const void * data,
	size_t * nwritten)
{
	pcm_dev * port = (pcm_dev *)cookie;
	status_t err;
	cpu_status cp;
	int written = 0;
	int to_write = *nwritten;	/*	 in play bytes, not input bytes!	*/
	int block;
	int bytes_xferred;

//	ddprintf(("sonic_vibes: pcm_write()\n")); /* we're here */

	*nwritten = 0;

	err = acquire_sem_etc(port->wr_entry, 1, B_CAN_INTERRUPT, 0);
	if (err < B_OK) {
		return err;
	}

	atomic_add(&port->write_waiting, 1);
	if (port->config.format == 0x24) {
		to_write >>= 1;	/*	floats collapse by 2	*/
	}
	while (to_write > 0) {

		/* wait to write */

		err = acquire_sem_etc(port->write_sem, 1, B_CAN_INTERRUPT, 0);
		if (err < B_OK) {
			release_sem(port->wr_entry);
			return err;
		}

#if DEBUG
		put_cnt++;
		{
			bigtime_t delta = system_time() - the_time;
			if (delta < 1) {
				ddprintf(("sonic_vibes: delta %Ld (low!) #%ld\n", delta, put_cnt));
			}
			else if (delta > 2000) {
				ddprintf(("sonic_vibes: delta %Ld (high!) #%ld\n", delta, put_cnt));
			}
		}
		if (put_cnt != int_cnt) {
	static int last;
			if (last != int_cnt-put_cnt)
				OLDAPI(("sonic_vibes: %ld mismatch\n", int_cnt-put_cnt));
			last = int_cnt-put_cnt;
		}
#endif /* DEBUG */

		KTRACE();
		cp = disable_interrupts();
		acquire_spinlock(&port->wr_lock);

		block = port->wr_size-port->was_written;
		if (block > to_write) {
			/* must let next guy in */
			if (atomic_add(&port->write_waiting, -1) > 0) {
				release_sem_etc(port->write_sem, 1, B_DO_NOT_RESCHEDULE);
			}
			else {
				atomic_add(&port->write_waiting, 1); /* undo damage */
			}
			block = to_write;
		}
		else if (block < to_write) {
			atomic_add(&port->write_waiting, 1); /* we will loop back */
		}
		switch (port->config.format) {
		case 0x24:	/*	floats	*/
			copy_float_to_short((short *)(port->wr_cur+port->was_written), (const float *)data, 
				block, !B_HOST_IS_LENDIAN == !port->config.big_endian);
			bytes_xferred = block * 2;
			break;
		case 0x02:	/*	shorts	*/
			if (!B_HOST_IS_LENDIAN == !port->config.big_endian) {
				/*	we need to swap	*/
				swap_copy((short *)(port->wr_cur+port->was_written), (const short *)data, block);
				bytes_xferred = block;
				break;
			}
			/*	else fall through to default case	*/
		case 0x11:	/*	bytes	*/
		default:
			memcpy((void *)(port->wr_cur+port->was_written), data, block);
			bytes_xferred = block;
			break;
		}
		port->was_written += block;
		port->wr_silence = 0;

		release_spinlock(&port->wr_lock);
		restore_interrupts(cp);

		data = ((char *)data)+bytes_xferred;
		written += bytes_xferred;
		to_write -= block;
	}

	*nwritten = written;
	release_sem(port->wr_entry);

	return B_OK;
}


bool
dma_a_interrupt(
	sonic_vibes_dev * dev)
{
	bool ret = false;
	pcm_dev * port = &dev->pcm;
	volatile uchar * ptr;
	uint32 addr;
	uint32 offs;
	bigtime_t st = system_time();
	int32 ww;

#if 0
ddprintf(("sonic_vibes: dma_a 0x%x+0x%x\n", PCI_IO_RD_32((int)port->dma_c), PCI_IO_RD_32((int)port->dma_c+4)));
#endif
//	KTRACE(); /* */
	acquire_spinlock(&port->wr_lock);

	if (port->write_sem < 0) {
		kprintf("sonic_vibes: spurious DMA A interrupt!\n");
		release_spinlock(&port->wr_lock);
		return false;
	}
	/* copy possible silence into playback buffer */

	if (port->was_written > 0 && port->was_written < port->wr_size) {
		if (port->config.format == 0x11) {
			memset((void *)(port->wr_cur+port->was_written), 0x80, port->wr_size-port->was_written);
		}
		else {
			memset((void *)(port->wr_cur+port->was_written), 0, port->wr_size-port->was_written);
		}
	}

	/* because the system may be lacking and not hand us the */
	/* interrupt in time, we check which half is currently being */
	/* played, and set the pointer to the other half */

	addr = PCI_IO_RD_32((uint32)port->dma_a);
	if ((offs = addr-(uint32)port->card->low_phys) < port->wr_size) {
		ptr = port->wr_2;
	}
	else {
		ptr = port->wr_1;
	}
	port->wr_total += port->config.play_buf_size/2;
	/* compensate for interrupt latency */
	/* assuming 4 byte frames */
	port->wr_time = st-(offs&(port->config.play_buf_size/2-1))*250000LL/(int64)port->config.sample_rate;
	if ((ww = atomic_add(&port->wr_time_wait, -1)) > 0) {
		release_sem_etc(port->wr_time_sem, 1, B_DO_NOT_RESCHEDULE);
		ret = true;
	}
	else {
		atomic_add(&port->wr_time_wait, 1); /* re-set to 0 */
	}

	if (port->wr_cur == ptr) {
		port->wr_skipped++;
		OLDAPI(("sonic_vibes: write skipped %ld\n", port->wr_skipped));
	}
	port->wr_cur = ptr;
	port->was_written = 0;

	/* check for client there to write into buffer */

	if (atomic_add(&port->write_waiting, -1) > 0) {
#if DEBUG
		int_cnt++;
		the_time = st;
#endif
		release_sem_etc(port->write_sem, 1, B_DO_NOT_RESCHEDULE);
		ret = true;
	}
	else {
		atomic_add(&port->write_waiting, 1);
		/* if none there, fill with silence */
		if (port->wr_silence < port->config.play_buf_size*2) {
			if (port->config.format == 0x11) {
				memset((void *)ptr, 0x80, port->wr_size);
			}
			else {
				memset((void *)ptr, 0, port->wr_size);
			}
			port->wr_silence += port->wr_size;
		}
	}
	/* copying will be done in user thread */

	release_spinlock(&port->wr_lock);
	return ret;
}


bool
dma_c_interrupt(
	sonic_vibes_dev * dev)
{
	bool ret = false;
	pcm_dev * port = &dev->pcm;
	volatile uchar * ptr;
	uint32 addr;
	uint32 offs;
	int32 rr;
	bigtime_t st = system_time();

	/* mark data as readable in record buffer */
#if 0
ddprintf(("sonic_vibes: dma_c 0x%x+0x%x\n", PCI_IO_RD_32((int)port->dma_c), PCI_IO_RD_32((int)port->dma_c+4)));
#endif
//	KTRACE(); /* */
	acquire_spinlock(&port->rd_lock);

	if (port->read_sem < 0) {
		kprintf("sonic_vibes: spurious DMA C interrupt!\n");
		release_spinlock(&port->rd_lock);
		return false;
	}
	/* if we lose an interrupt, we automatically avoid constant glitching by setting */
	/* the write pointer based on where the DMA counter is everytime */
	addr = PCI_IO_RD_32((uint32)port->dma_c);
	if ((offs = addr-port->config.play_buf_size-(uint32)port->card->low_phys) < port->rd_size) {
		ptr = port->rd_2;
	}
	else {
		ptr = port->rd_1;
	}
	if (port->rd_cur == ptr) {
		port->rd_skipped++;
		OLDAPI(("sonic_vibes: read skipped %ld\n", port->rd_skipped));
	}
	port->rd_total += port->rd_size;

	port->rd_cur = ptr;
	port->was_read = 0;
	port->rd_time = port->next_rd_time;
	/*	time stamp when this buffer became available -- compensate for interrupt latency	*/
	port->next_rd_time = st-(offs&(port->config.rec_buf_size/2-1))*1000000LL/(int64)port->config.sample_rate;
	if ((rr = atomic_add(&port->rd_time_wait, -1)) > 0) {
		release_sem_etc(port->rd_time_sem, 1, B_DO_NOT_RESCHEDULE);
		ret = true;
	}
	else {
		atomic_add(&port->rd_time_wait, 1); /* re-set to 0 */
	}

	if (atomic_add(&port->read_waiting, -1) > 0) {
		release_sem_etc(port->read_sem, 1, B_DO_NOT_RESCHEDULE);
		ret = true;
	}
	else {
		atomic_add(&port->read_waiting, 1);
	}
	/* copying will be done in the user thread */
	release_spinlock(&port->rd_lock);
	return ret;
}

