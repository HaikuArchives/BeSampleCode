/*
	ScriptWindow.cpp
	
	Copyright 1995 Be Incorporated, All Rights Reserved.
*/

/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef _APPLICATION_H
#include <Application.h>
#endif
#ifndef SCRIPT_WINDOW_H
#include "ScriptWindow.h"
#endif
#ifndef SCRIPT_VIEW_H
#include "ScriptView.h"
#endif

ScriptWindow::ScriptWindow(BRect frame)
				: BWindow(frame, "ScriptWorld", B_TITLED_WINDOW, B_NOT_ZOOMABLE )
{
	ScriptView	*aView;
	
	/* set up a rectangle and instantiate a new view */
	BRect aRect( Bounds() );
	aView = new ScriptView(aRect, "ScriptView");
	
	/* add view to window */
	AddChild(aView);
	aView->FillList();
}

bool ScriptWindow::QuitRequested()
{
	be_app->PostMessage(B_QUIT_REQUESTED);
	return(true);
}
