/* lInfoView */
/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef L_INFO_VIEW_H
#define L_INFO_VIEW_H

#include "Defs.h"
#include <View.h>
#include <Entry.h>

struct stat;

class lInfoView : public BView {
	public:
							lInfoView(entry_ref &ref, struct stat &st);
							~lInfoView();
	
		status_t			InitCheck();
		
		void				MessageReceived(BMessage *msg);
		void				Draw(BRect updateRect);
		
		char *				GetName();
		
	private:
		status_t			fStatus;
		entry_ref			fRef;
		char *				fNameStr;
		char *				fPathStr;
		char *				fCreateStr;
		char *				fModStr;
		char *				fLinkStr;
		bool				fAbsolute;

};

#endif
