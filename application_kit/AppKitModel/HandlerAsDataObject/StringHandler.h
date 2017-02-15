/* StringHandler.h */

/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef STRING_HANDLER_H
#define STRING_HANDLER_H

#include <Handler.h>
#include <String.h>

class StringHandler : public BHandler {
	public:
							StringHandler(const char *value, const char *name = NULL);
		virtual				~StringHandler();
		
		virtual void		MessageReceived(BMessage *msg);
	
	private:
		void				UpperCase();
		void				LowerCase();
		void				MixedCase();
		void				GetValue(BMessage *msg) const;
		void				SetValue(BMessage *msg);
		void				GetInfo(BMessage *msg) const;
		
		BString				fValue;
};

#endif
