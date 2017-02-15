/* StringAppkication.h */

/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef STRING_APPLICATION_H
#define STRING_APPLICATION_H

#include <Application.h>

class StringLooper;

class StringApplication : public BApplication {
	public:
								StringApplication();
		virtual					~StringApplication();
		virtual	bool			QuitRequested();
		virtual void			ReadyToRun();
		
	private:
		void					TestPhrase(const char *name, const char *phrase);
		StringLooper *			fStrings;
};

#endif

