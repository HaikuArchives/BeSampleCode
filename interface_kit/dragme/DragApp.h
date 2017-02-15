/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#if !defined(DragApp_h)
#define DragApp_h

#include <Application.h>
#include <set>

class DragWindow;

class DragApp : public BApplication {
public:
		DragApp(
				const char * signature);
virtual	~DragApp();

virtual	bool QuitRequested();
virtual	void MessageReceived(
				BMessage * message);
virtual	void ReadyToRun();

		enum {
			ADD_WINDOW = 'da00',
			REMOVE_WINDOW,
			NEW_WINDOW
		};

private:

		BRect m_windowRect;
		int m_windowCount;
		set<DragWindow *> m_windows;

		void AddWindow(
				DragWindow * window);
		void RemoveWindow(
				DragWindow * window);
		void NewWindow();

};


#endif // DragApp_h
