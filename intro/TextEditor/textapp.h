//
// TextApp class
//
// This class, derived from BApplication, defines the
// Hello World application itself.
//
/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef __TEXTAPP_H__
#define __TEXTAPP_H__

extern const char *APP_SIGNATURE;

class TextApp : public BApplication {
	public:
						TextApp();
		virtual void	MessageReceived(BMessage *message);
		virtual void	RefsReceived(BMessage *message);
	
	private:
		int32			window_count;
		int32			next_untitled_number;
		
		BFilePanel		*openPanel;
};
#endif