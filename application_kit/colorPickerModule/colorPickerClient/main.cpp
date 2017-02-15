/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include <Application.h>

#include "SimpleWindow.h"

class ColorPickingApp : public BApplication {
public:
	ColorPickingApp()
		:	BApplication("application/x-vnd.Be.sampleColorPickerUser")
		{}

protected:
	virtual void ReadyToRun()
		{
			(new SimpleWindow(BPoint(200, 200)))->Show();
		}

};

int
main()
{
	ColorPickingApp app;
	app.Run();

	return 0;
}
