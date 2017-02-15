/* DDApp.cpp */
/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/
#include <Alert.h>
#include "DDApp.h"
#include "MsgVals.h"

/*-------------------------------------------------------------------------*/

DDApp::DDApp()
	: BApplication("application/x-vnd.Be-dynadraw")
{
	ddWin = new DDWindow();
	colorWin = NULL;
	tweakWin = NULL;
}

/*-------------------------------------------------------------------------*/

void
DDApp::MessageReceived(BMessage* msg)
{
	switch(msg->what)
	{
	 case TWEAK_REQ:
	 	if(tweakWin != NULL) break;
		tweakWin = new TweakWin(ddWin, ddWin->Mass(), ddWin->Drag(), 
								ddWin->Width(),	ddWin->Sleep());
	 	break;
	 	
	 case COLOR_REQ:
		if(colorWin != NULL) break;
	 	colorWin = new ColorWin(ddWin, ddWin->Color());
	 	break;
	 	
	 case TWEAK_QUIT:
	 	tweakWin = NULL;
	 	break;
	 
	 case COLOR_QUIT:
	 	colorWin = NULL;
	 	break;
	 	
	 default:
	 	BApplication::MessageReceived(msg);
	 	break;
	}
}

/*-------------------------------------------------------------------------*/

void
DDApp::AboutRequested()
{
	(new BAlert("About...",
		"DynaDraw\n\nOriginal SGI Version, Paul Haeberli 1989\n"
		"Be Version, Michael Morrissey",
		"Cool!"))->Go();
}
/*-------------------------------------------------------------------------*/
