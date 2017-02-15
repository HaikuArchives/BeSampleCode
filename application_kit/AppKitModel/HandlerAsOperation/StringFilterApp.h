/* StringFilterApp.h */

/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef STRING_FILTER_APP_H
#define STRING_FILTER_APP_H

#include <Application.h>

class StringFilterLooper;

class StringFilterApp : public BApplication {
	public:
									StringFilterApp();
		virtual						~StringFilterApp();
		
		virtual void				ReadyToRun();
		virtual bool				QuitRequested();
	private:
		StringFilterLooper *		fFilters;
};

#endif
