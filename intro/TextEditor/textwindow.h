//
// TextWindow class
//
// This class defines the hello world window.
//

/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef __TWINDOW_H__
#define __TWINDOW_H__

class TextWindow : public BWindow {
	public:
						TextWindow(BRect frame);
						TextWindow(BRect frame, entry_ref *ref);
						~TextWindow();
		virtual bool	QuitRequested();
		virtual void	MessageReceived(BMessage *message);
		virtual void	FrameResized(float width, float height);
		
		status_t		Save(BMessage *message);
	
	private:
		void			_InitWindow(void);
		void			Register(bool need_id);
		void			Unregister(void);
		
		BMenuBar		*menubar;
		BTextView		*textview;
		BScrollView		*scrollview;
		BMenuItem		*saveitem;
		BMessage		*savemessage;
		int32			window_id;
		
		BFilePanel		*savePanel;
};

#endif