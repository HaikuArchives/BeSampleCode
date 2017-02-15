//*******************************
//	loadaddonapp.cpp
//*******************************
/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include "loadaddonapp.h"
#include "effectpal.h"


int main(int, char**)
{
	LoadAddOnApp  app;
	
	app.Run();
	
	return (0);
}


LoadAddOnApp::LoadAddOnApp()
				: BApplication("application/x-vnd.Be-LoadAddOnSample")
{
	BRect rect( 100, 100, 200, 200 );

	EffectPal *palette = new EffectPal( rect, "Effects", B_TITLED_WINDOW, 0, 0 );
	
	palette->Init();

	palette->Show();
}

