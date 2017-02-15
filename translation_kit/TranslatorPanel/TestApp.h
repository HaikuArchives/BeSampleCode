#ifndef TESTAPP_H
#define TESTAPP_H

#include <app/Application.h>

class TestApp : public BApplication {
	public:
		TestApp();
		void RefsReceived(BMessage *message);
};

#endif
