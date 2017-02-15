//****************************************
//effectpal.h
//****************************************
/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef EFFECTPAL_H
#define EFFECTPAL_H

#include <Window.h>

class EffectPal : public BWindow
{
	public:
		EffectPal( BRect, const char*, window_type, uint32, uint32 );
		~EffectPal();

			void 	Init( void );
	virtual	bool	QuitRequested();

};


//****************************************
#endif //EFFECTPAL_H
	
	
	
