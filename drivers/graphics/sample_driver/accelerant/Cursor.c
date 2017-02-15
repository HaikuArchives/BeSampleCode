/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include "GlobalData.h"
#include "generic.h"

void set_cursor_colors(void) {
	/* a place-holder for a routine to set the cursor colors */
	/* In our sample driver, it's only called by the INIT_ACCELERANT() */
	
}

status_t SET_CURSOR_SHAPE(uint16 width, uint16 height, uint16 hot_x, uint16 hot_y, uint8 *andMask, uint8 *xorMask) {

	/* NOTE: Currently, for BeOS, cursor width and height must be equal to 16. */

	if ((width != 16) || (height != 16))
	{
		return B_ERROR;
	}
	else if ((hot_x >= width) || (hot_y >= height))
	{
		return B_ERROR;
	}
	else
	{
		/*
		Different cards have different cursor configuration requirements, so you're
		on your own.
		*/

		/* Update cursor variables appropriately. */
		si->cursor.width = width;
		si->cursor.height = height;
		si->cursor.hot_x = hot_x;
		si->cursor.hot_y = hot_y;
	}

	return B_OK;
}

/*
Move the cursor to the specified position on the desktop.  If we're
using some kind of virtual desktop, adjust the display start position
accordingly and position the cursor in the proper "virtual" location.
*/
void MOVE_CURSOR(uint16 x, uint16 y) {

	bool move_screen = false;
	uint16 hds = si->dm.h_display_start;	/* the current horizontal starting pixel */
	uint16 vds = si->dm.v_display_start;	/* the current vertical starting line */

	/*
	Most cards can't set the starting horizontal pixel to anything but a multiple
	of eight.  If your card can, then you can change the adjustment factor here.
	Oftentimes the restriction is mode dependant, so you could be fancier and
	perhaps get smoother horizontal scrolling with more work.  It's a sample driver,
	so we're going to be lazy here.
	*/
	uint16 h_adjust = 7;	/* a mask to make horizontal values a multiple of 8 */

	/* clamp cursor to virtual display */
	if (x >= si->dm.virtual_width) x = si->dm.virtual_width - 1;
	if (y >= si->dm.virtual_height) y = si->dm.virtual_height - 1;

	/* adjust h/v_display_start to move cursor onto screen */
	if (x >= (si->dm.timing.h_display + hds)) {
		hds = ((x - si->dm.timing.h_display) + 1 + h_adjust) & ~h_adjust;
		move_screen = true;
	} else if (x < hds) {
		hds = x & ~h_adjust;
		move_screen = true;
	}
	if (y >= (si->dm.timing.v_display + vds)) {
		vds = y - si->dm.timing.v_display + 1;
		move_screen = true;
	} else if (y < vds) {
		vds = y;
		move_screen = true;
	}
	/* reposition the desktop on the display if required */
	if (move_screen) MOVE_DISPLAY(hds,vds);

	/* put cursor in correct physical position */
	x -= hds;
	y -= vds;

	/* position the cursor on the display */
	/* this will be card dependant */
}

void SHOW_CURSOR(bool is_visible) {

	if (is_visible)	{
		/* add cursor showing code here */
	} else {
		/* add cursor hiding code here */
	}
	/* record for our info */
	si->cursor.is_visible = is_visible;
}
