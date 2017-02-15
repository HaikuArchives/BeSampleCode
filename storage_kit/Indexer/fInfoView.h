/* fInfoView */
/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef F_INFO_VIEW_H
#define F_INFO_VIEW_H

#include "Defs.h"
#include <View.h>
#include <Entry.h>

struct stat;
class BListView;
class BList;

class fInfoView : public BView {
	public:
							fInfoView(entry_ref &ref, struct stat &st);
							~fInfoView();
	
		status_t			InitCheck();

		void				MessageReceived(BMessage *msg);
		void				Draw(BRect updateRect);
		
		char *				GetName();
		
	private:
		void				GetAttributes(BNode &node, BList &list);

		entry_ref			fRef;
		BListView *			fAttrList;
		BList *				fIndexList;
		status_t			fStatus;

		char *				fNameStr;
		char *				fCreateStr;
		char *				fModStr;
		char *				fPathStr;
		char *				fSizeStr;
		char *				fMimeStr;
		status_t			fIndexed;

};

#endif

