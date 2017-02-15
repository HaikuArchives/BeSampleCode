/* dInfoView */
/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef D_INFO_VIEW_H
#define D_INFO_VIEW_H

#include "Defs.h"
#include <View.h>
#include <Entry.h>

struct stat;
class BDirectory;
class BList;

class dInfoView : public BView {
	public:
							dInfoView(entry_ref &ref, struct stat &st, BDirectory &dir);
							~dInfoView();
	
		status_t			InitCheck();
	
		void				MessageReceived(BMessage *msg);
		void				Draw(BRect updateRect);
		
		char *				GetName();
		
	private:
	
		void				TraverseDirectory(BDirectory &dir);
		void				EvaluateRef(entry_ref &ref);
		
		status_t			fStatus;
		entry_ref			fRef;
		char *				fNameStr;
		char *				fPathStr;
		char *				fCreateStr;
		char *				fModStr;

		BList *				fIndexList;

		int32				fEntryCount;
		int32				fSubDirCount;
		int32				fLinkCount;
		int32				fFileCount;
		int32				fIndexed;
		int32				fPartialIndexed;
		int32				fNotIndexed;
		int32				fInvalidCount;
};

#endif
