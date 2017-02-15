/*
	ScriptView.h
	
	Copyright 1997 Be Incorporated, All Rights Reserved.
*/

/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef SCRIPT_VIEW_H
#define SCRIPT_VIEW_H

#ifndef _OUTLINE_LIST_VIEW_H
#include <OutlineListView.h>
#endif

class ScriptView : public BOutlineListView
{
public:
	ScriptView(BRect frame, const char *name);
	
virtual status_t FillList(); /* creates the list */
virtual status_t Invoke(BMessage *msg = NULL);
};

#endif //SCRIPT_VIEW_H
