/*
	dugWindow.cpp
*/

/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef _APPLICATION_H
#include <Application.h>
#endif
#ifndef _SLIDER_H
#include <Slider.h>
#endif
#ifndef DUG_WINDOW_H
#include "dugWindow.h"
#endif



dugWindow::dugWindow(BRect frame)
		: BWindow(frame, "dugzWindow", B_TITLED_WINDOW, B_NOT_ZOOMABLE )
{
	BRect r(10,10,250,50);
	BSlider *slider = new BSlider(r, "slider1", "Construct Speed (%)", NULL, 0, 140);
	AddChild(slider);
}

bool
dugWindow::QuitRequested()
{
	be_app->PostMessage(B_QUIT_REQUESTED);
	return(true);
}
