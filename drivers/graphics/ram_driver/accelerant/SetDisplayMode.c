/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include "GlobalData.h"
#include "generic.h"
#include <sys/ioctl.h>

/*
	Enable/Disable interrupts.  Just a wrapper around the
	ioctl() to the kernel driver.
*/
static void interrupt_enable(bool flag) {
	status_t result;
	ram_set_bool_state sbs;

	/* set the magic number so the driver knows we're for real */
	sbs.magic = RAM_PRIVATE_DATA_MAGIC;
	sbs.do_it = flag;
	/* contact driver and get a pointer to the registers and shared data */
	result = ioctl(fd, RAM_RUN_INTERRUPTS, &sbs, sizeof(sbs));
}

/*
	Calculates the number of bits for a given color_space.
	Usefull for mode setup routines, etc.
*/
static uint32 calcBitsPerPixel(uint32 cs) {
	uint32	bpp = 0;

	switch (cs) {
		case B_RGB32_BIG:
		case B_RGBA32_BIG:
		case B_RGB32_LITTLE:
		case B_RGBA32_LITTLE:
			bpp = 32; break;
		case B_RGB24_BIG:
		case B_RGB24_LITTLE:
			bpp = 24; break;
		case B_RGB16_BIG:
		case B_RGB16_LITTLE:
			bpp = 16; break;
		case B_RGB15_BIG:
		case B_RGBA15_BIG:
		case B_RGB15_LITTLE:
		case B_RGBA15_LITTLE:
			bpp = 15; break;
		case B_CMAP8:
			bpp = 8; break;
	}
	return bpp;
}

/*
	The code to actually configure the display.  Unfortunately, there's not much
	that can be provided in the way of sample code.  If you're lucky, you're writing
	a driver for a device that has all (or at least most) of the bits for a particular
	configuration value in the same register, rather than spread out over the standard
	VGA registers + a zillion expansion bits.  In any case, I've found that the way
	to simplify this routine is to do all of the error checking in PROPOSE_DISPLAY_MODE(),
	and just assume that the values I get here are acceptable.
*/
static void do_set_display_mode(display_mode *dm) {
	/* disable interrupts using the kernel driver */
	interrupt_enable(false);
	/* save the display mode */
	si->dm = *dm;
	si->fbc.frame_buffer = si->framebuffer;
	si->fbc.frame_buffer_dma = si->framebuffer_pci;
	si->fbc.bytes_per_row = dm->virtual_width * (calcBitsPerPixel(dm->space) >> 3);
	
	/* enable interrupts using the kernel driver */
	interrupt_enable(true);
}

/*
	The exported mode setting routine.  First validate the mode, then call our
	private routine to hammer the registers.
*/
status_t SET_DISPLAY_MODE(display_mode *mode_to_set) {
	display_mode bounds, target;

	/* ask for the specific mode */
	target = bounds = *mode_to_set;
	if (PROPOSE_DISPLAY_MODE(&target, &bounds, &bounds) == B_ERROR)
		return B_ERROR;
	do_set_display_mode(&target);
	return B_OK;
}

/*
	Set which pixel of the virtual frame buffer will show up in the
	top left corner of the display device.  Used for page-flipping
	games and virtual desktops.
*/
status_t MOVE_DISPLAY(uint16 h_display_start, uint16 v_display_start) {
	/*
	Many devices have limitations on the granularity of the horizontal offset.
	Make any checks for this here.  A future revision of the driver API will
	add a hook to return the granularity for a given display mode.
	*/
	/* most cards can handle multiples of 8 */
	if (h_display_start & 0x07)
		return B_ERROR;
	/* do not run past end of display */
	if ((si->dm.timing.h_display + h_display_start) > si->dm.virtual_width)
		return B_ERROR;
	if ((si->dm.timing.v_display + v_display_start) > si->dm.virtual_height)
		return B_ERROR;

	/* everybody remember where we parked... */
	si->dm.h_display_start = h_display_start;
	si->dm.v_display_start = v_display_start;

	/* actually set the registers */
	/* INSERT YOUR CODE HERE */
	return B_OK;
}

/*
	Set the indexed color palette.
*/
void SET_INDEXED_COLORS(uint count, uint8 first, uint8 *color_data, uint32 flags) {
}
