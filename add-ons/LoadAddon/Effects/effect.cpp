//****************************************
// effect.cpp
//****************************************

/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/
#include <Window.h>

#include "effect.h"

#define EFFECT_BUTTON_MSG	'Ebut'

//****************************************
// Effect

//construcor
Effect::Effect( image_id id )
{
	mId	= id;
	mpButton = NULL;
}

//GetButtonSize
void Effect::GetButtonSize( BPoint* point )
{
	float	width, height;
	if(mpButton){
		mpButton->GetPreferredSize( &width, &height );
		point->x = width;
		point->y = height;
	};
}

//Init	
void Effect::Init( BWindow* win, BPoint topleft )		//set up
{
	Effect*		peffect = this;
	
	//do initialization specific to effect here
	BMessage* msg = new BMessage( EFFECT_BUTTON_MSG );
	msg->AddPointer( "effectptr", peffect );
	BRect rect( 0, 0, 102, 24 );
	rect.OffsetTo( topleft );
	mpButton = new BButton( rect,								
							"Effect",
							"Effect",
							 msg );	
	win->Lock();
	win->AddChild( mpButton );
	win->Unlock();
}

//****************************************