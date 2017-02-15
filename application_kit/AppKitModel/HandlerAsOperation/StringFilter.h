/* StringFilter.h */

/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef STRING_FILTER_H
#define STRING_FILTER_H

#include <Handler.h>

class BString;

class StringFilter : public BHandler {
	public:
								StringFilter(const char *name);
		virtual					~StringFilter();
		
		virtual void			MessageReceived(BMessage *msg);

	private:
		// change the string directly
		virtual	void			Filter(BString &string) = 0;
		//add an operation name, opcode and description for each opcode handled
		virtual void			Opcodes(BMessage *info) = 0;
		/* handle the replies for the filtering */
		void					FilterStrings(BMessage *msg);
};

#endif
