#include "TestApp.h"
#include "TestWindow.h"
#include "Common.h"

int main() {
	TestApp *myapp = new TestApp();
	myapp->Run();
	delete myapp;
	return 0;
}

TestApp::TestApp() : BApplication("application/x-vnd.BeDTS-TranslatorPanel") {
	BRect rect(100, 100, 500, 400);
	TestWindow *testwindow = new TestWindow(rect, "Translator Panel");
	testwindow->Show();
}

void TestApp::RefsReceived(BMessage *message) {
	message->what = OPEN_FILE_PANEL;
	BWindow *window = WindowAt(0);
	if (window != NULL) window->PostMessage(message);
}