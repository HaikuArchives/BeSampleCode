/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef DRIVERINTERFACE_H
#define DRIVERINTERFACE_H

#include <Accelerant.h>
#include <Drivers.h>
#include <PCI.h>
#include <OS.h>

/*
	This is the info that needs to be shared between the kernel driver and
	the accelerant for the ram driver.
*/
#if defined(__cplusplus)
extern "C" {
#endif

typedef struct {
	sem_id	sem;
	int32	ben;
} benaphore;

#define INIT_BEN(x)		x.sem = create_sem(0, "RAM "#x" benaphore");  x.ben = 0;
#define AQUIRE_BEN(x)	if((atomic_add(&(x.ben), 1)) >= 1) acquire_sem(x.sem);
#define RELEASE_BEN(x)	if((atomic_add(&(x.ben), -1)) > 1) release_sem(x.sem);
#define	DELETE_BEN(x)	delete_sem(x.sem);


#define RAM_PRIVATE_DATA_MAGIC	'RAMB' /* a private driver rev, of sorts */

#define MAX_RAM_DEVICE_NAME_LENGTH 32

#define RKD_MOVE_CURSOR    0x00000001
#define RKD_PROGRAM_CLUT   0x00000002
#define RKD_SET_START_ADDR 0x00000004
#define RKD_SET_CURSOR     0x00000008
#define RKD_HANDLER_INSTALLED 0x80000000

enum {
	RAM_GET_PRIVATE_DATA = B_DEVICE_OP_CODES_END + 1,
	RAM_DEVICE_NAME,
	RAM_RUN_INTERRUPTS
};

typedef struct {
	area_id	fb_area;	/* Frame buffer's area_id.  The addresses are shared
							with all teams. */
	void	*framebuffer;	/* As viewed from virtual memory */
	void	*framebuffer_pci;	/* As viewed from the PCI bus (for DMA) */
	area_id	mode_area;	/* Contains the list of display modes the driver supports */
	uint32	mode_count;	/* Number of display modes in the list */
	sem_id	vblank;	/* The vertical blank semaphore.  Ownership will be
						transfered to the team opening the device first */

	int32	flags;

	struct {
		uint8*	data;		/*  Pointer into the frame buffer to where the
								cursor data starts */
		uint16	hot_x;		/* Cursor hot spot. The top left corner of the cursor */
		uint16	hot_y;		/* is 0,0 */
		uint16	x;			/* The location of the cursor hot spot on the */
		uint16	y;			/* display (or desktop?) */
		uint16	width;		/* Width and height of the cursor shape */
		uint16	height;
		bool	is_visible;	/* Is the cursor currently displayed? */
	}		cursor;
	uint16	first_color;
	uint16	color_count;
	bigtime_t	refresh_period;	/* Duration of one frame (ie 1/refresh rate) */
	bigtime_t	blank_period;	/* Duration of the blanking period.   These are
									usefull when faking vertical blanking
									interrupts. */
	uint8	color_data[3 * 256];	/* */
	uint8	cursor0[64*64/8];	/* AND mask for a 64x64 cursor */
	uint8	cursor1[512];		/* XOR mask for a 64x64 cursor */
	display_mode
			dm;		/* current display mode configuration */
	frame_buffer_config
			fbc;	/* bytes_per_row and start of frame buffer */
	struct {
		uint64		count;		/* last fifo slot used */
		uint64		last_idle;	/* last fifo slot we *know* the engine was idle after */ 
		benaphore	lock;		/* for serializing access to the acceleration engine */
	}		engine;

	uint32	pix_clk_max8;		/* The maximum speed the pixel clock should run */
	uint32	pix_clk_max16;		/*  at for a given pixel width.  Usually a function */
	uint32	pix_clk_max32;		/*  of memory and DAC bandwidths. */
	uint32	mem_size;			/* Frame buffer memory, in bytes. */
} shared_info;

/* Set some boolean condition (like enabling or disabling interrupts) */
typedef struct {
	uint32	magic;		/* magic number to make sure the caller groks us */
	bool	do_it;		/* state to set */
} ram_set_bool_state;

/* Retrieve the area_id of the kernel/accelerant shared info */
typedef struct {
	uint32	magic;		/* magic number to make sure the caller groks us */
	area_id	shared_info_area;	/* area_id containing the shared information */
} ram_get_private_data;

/* Retrieve the device name.  Usefull for when we have a file handle, but want
to know the device name (like when we are cloning the accelerant) */
typedef struct {
	uint32	magic;		/* magic number to make sure the caller groks us */
	char	*name;		/* The name of the device, less the /dev root */
} ram_device_name;

enum {
	RAM_WAIT_FOR_VBLANK = (1 << 0)
};

#if defined(__cplusplus)
}
#endif


#endif
