/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include "imaging.h"

class BBitmap;

// Scale an image.  It will work in either direction.
// Scaling up or down.  Scaling by integer values will be
// most optimal.

status_t	scale(const BBitmap *source, BBitmap *dst, 
				const float xFactor = -1, const float yFactor = -1,
				scale_method = IMG_SCALE_BILINEAR);

// These would make the transformation manipulations complete
status_t	rotate(const BBitmap *source, BBitmap *dest, uint32 degrees);

status_t	shear();

status_t	translate();

status_t	transform();

