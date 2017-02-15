//****************************************
// effect.h
//****************************************

/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef EFFECT_H
#define EFFECT_H

#include <Button.h>
#include <image.h>

//****************************************
//Effect class
class Effect
{
public:
	//constructor and destructor
			Effect( image_id );
	
	//operations
	virtual void		GetButtonSize( BPoint* );
	virtual void		Init( BWindow*, BPoint );

protected:
	//members
	image_id		mId;
	BButton			*mpButton;
};

//****************************************

#endif //EFFECT_H
	