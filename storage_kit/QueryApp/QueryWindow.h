/* QueryWindow.h */
/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

/* Object that both builds a simple query structure and displays */
/* the results in a view below */

#include <Window.h>
#include <Messenger.h>
#include <TextControl.h>

class BList;
class BListView;
class BMenuControl;
class BQuery;
class BStringView;
class BTextControl;


class QueryWindow : public BWindow {
	public:
								QueryWindow();
		virtual					~QueryWindow();		
		virtual	void			MessageReceived(BMessage *message);	
		virtual	bool			QuitRequested();		
	private:
		void					ClearResults();
		void					GetResults();
		void					FetchC();
		void					FetchQuery();
		void					FetchLive();
		
		BStringView *			fQueryString;
		BListView *				fItemList;
		BTextControl *			fNameString;
		int32					fFileType;
		int32					fMethod;
};
