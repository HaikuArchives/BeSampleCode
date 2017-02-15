/* StringLooper.h */

/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef STRING_LOOPER_H
#define STRING_LOOPER_H

#include <Looper.h>

class StringLooper : public BLooper {
	public:
							StringLooper();
		virtual				~StringLooper();
		
		virtual void		MessageReceived(BMessage *msg);
		virtual bool		QuitRequested();
	private:
		void				NewString(BMessage *msg);
		void				GetStrings(BMessage *msg);
};

#endif
