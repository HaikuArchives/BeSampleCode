#ifndef TESTWINDOW_H
#define TESTWINDOW_H

#include <interface/Window.h>
#include "TestView.h"
#include "TranslatorSavePanel.h"

class TestWindow : public BWindow {
	public:
		TestWindow(BRect rect, const char *name);
		~TestWindow();
		void MessageReceived(BMessage *message);
		bool QuitRequested();

	private:
		TranslatorSavePanel *savepanel;
		BFilePanel *openpanel;
		TestView *testview;
};

#endif
