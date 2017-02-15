/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#if !defined(DragWindow_h)
#define DragWindow_h

#include <Window.h>


class DragView;


class DragWindow : public BWindow {
public:
		DragWindow(
				const BRect & frame,
				const char * name);
virtual	~DragWindow();

virtual	void MessageReceived(
				BMessage * message);
virtual	bool QuitRequested();

		enum {
			QUIT_ALERT_REPLY = 'dw00'
		};

private:

		DragView * m_view;

		void ActionCopy(
				BMessage * request);
		void ActionTrash(
				BMessage * request);
		void QuitAlertReply(
				BMessage * reply);
};


#endif	//	DragWindow_h

