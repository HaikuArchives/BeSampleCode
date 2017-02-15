/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef _BTSBUTTONWINDOW_H_
#define _BTSBUTTONWINDOW_H_


#include <Window.h>
#include <Button.h>

#define BUTTON_MSG 'PRES'

class BTSButtonWindow : public BWindow
{
	public:
						BTSButtonWindow();
		virtual bool	QuitRequested();
		virtual void	MessageReceived(BMessage* message);
	private:
		BButton*		fButton;
};

#endif
