/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef _BTSBUTTONAPP_H_
#define _BTSBUTTONAPP_H_


#include <Application.h>

#include "BTSButtonWindow.h"

//#define BUTTON_APP_SIG 'BTAP'
#define BUTTON_APP_SIG "application/x-vnd.Be-BasicButton"

class BTSButtonApp: public BApplication
{
	public:
						BTSButtonApp();
		virtual void	ReadyToRun();
	private:
		BTSButtonWindow*	fWindow;
};

#endif
