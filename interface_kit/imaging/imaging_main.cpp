/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include "imaging_app.h"

int main(int, char**)
{
	BTSImagingApp *myApp = new BTSImagingApp();
	myApp->Run();
	delete myApp;

	return 0;
}
