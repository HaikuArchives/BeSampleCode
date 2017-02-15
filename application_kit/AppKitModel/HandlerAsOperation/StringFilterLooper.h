/* StringFilterLooper.h */

/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef STRING_FILTER_LOOPER_H
#define STRING_FILTER_LOOPER_H

#include <Looper.h>

class BList;

class StringFilterLooper : public BLooper {
	public:
								StringFilterLooper();
		virtual 				~StringFilterLooper();
		
		virtual void			MessageReceived(BMessage *msg);
		virtual bool			QuitRequested();
		virtual void			LoadFilters();
	private:
		virtual void			GetOpcodes(BMessage *msg);
		virtual void			Filter(BMessage *msg);
};

#endif
