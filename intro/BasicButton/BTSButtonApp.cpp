/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include "BTSButtonApp.h"
#include "BTSButtonWindow.h"

int main(int, char**)
{
	BTSButtonApp* app = new BTSButtonApp();
	app->Run();
	
	return(0);
}

BTSButtonApp::BTSButtonApp()
	: BApplication(BUTTON_APP_SIG)
{
}

void
BTSButtonApp::ReadyToRun()
{
	fWindow = new BTSButtonWindow();
	fWindow->Show();
}