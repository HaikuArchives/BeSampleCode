/*******************************************************************************
/
/	File:			MediaControlPanelApplication.h
/
/   Description:	Displays a simple control panel for a media node.
/
*******************************************************************************/
/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef _MediaControlPanelApp_h
#define _MediaControlPanelApp_h

#include <map>
#include <Application.h>
#include <MediaDefs.h>

// Class: MediaControlPanelApp
// ---------------------------
// Displays a simple control panel for a node. Your add-on
// instantiates and runs this application in its main()
// function. This provides a response to the default
// implementation of BControllable::StartControlPanel().
class MediaControlPanelApp : public BApplication
{
public:
	// IMPORTANT: When you build your add-on, remember
	// to link in a resource file which specifies the
	// same signature as the one you specify here, or
	// this ain't gonna work!
	MediaControlPanelApp(const char* signature);
	
	virtual void ReadyToRun();
	virtual void ArgvReceived(int32 argc, char** argv);
	virtual void MessageReceived(BMessage* message);
	
	// Override this if you want to create your own
	// custom control panels to work with this application.
	virtual BWindow* CreateControlPanel(media_node_id id);
	
	// If you've implemented your own control panels,
	// call this function when one of the control panels
	// is going to quit. Just tell me which node this
	// involves. This is thread-safe -- really!
	void ControlPanelQuitting(media_node_id id);

private:
	void ShowControlPanel(media_node_id id);
	void RemoveControlPanel(media_node_id id);
	
	map<media_node_id, BWindow*> _mControlPanels;
};

#endif /* _MediaControlPanelApp_h */