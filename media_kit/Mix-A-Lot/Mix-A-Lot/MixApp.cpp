/*
	
	MixApp.cpp
	
*/

/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include "MixWin.h"
#include "MixApp.h"

int main()
{	
	MixApp	myApplication;
	myApplication.Run();
	return 0;
}

MixApp::MixApp()
	: BApplication("application/x-vnd.Be-DTS.Mix-A-Lot")
{
}

void MixApp::ReadyToRun()
{
	new MixWin;
}
