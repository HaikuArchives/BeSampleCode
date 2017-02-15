//****************************************
// effectaddon.c
//****************************************
/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/
#include "effect.h"

#include <image.h>

//****************************************

extern "C" _EXPORT Effect* NewEffect( image_id );

//****************************************

Effect* NewEffect( image_id id )
{
	Effect* effect;
  	effect = new Effect( id );
  	return effect;
}
