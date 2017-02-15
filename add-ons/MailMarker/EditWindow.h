/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef EDIT_WINDOW_H
#define EDIT_WINDOW_H

#include <Window.h>

class BListView;
class BTextControl;
class BFile;
class BButton;

class EditWindow : public BWindow {
	public:
						EditWindow(BFile *file, char **buffer);
		virtual			~EditWindow();
		
		void			MessageReceived(BMessage *msg);			
	private:
		BTextView *		fText;
		BButton *		fRevert;
		BButton *		fSave;
		char *			fBuffer;
		char **			fOrigin;
		BFile *			fFile;
};

#endif
