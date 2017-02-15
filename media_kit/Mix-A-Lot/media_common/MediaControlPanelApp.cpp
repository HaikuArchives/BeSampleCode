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

#include <Debug.h>
#include <MediaAddOn.h>
#include <MediaRoster.h>
#include <MediaTheme.h>
#include <Window.h>
#include "ArgvHandler.h"
#include "MediaControlPanelApp.h"

static const uint32 _MEDIA_CONTROL_PANEL_GONE = 'mCPG';

// Class: _MediaControlPanel
// -------------------------
// Displays the control panel UI for a single Media Node.
class _MediaControlPanel : public BWindow
{
public:
	_MediaControlPanel(media_node_id id);
	void Quit();

private:
	status_t Init();
	status_t FindLiveNodeForID(media_node_id id, live_node_info* info);
	
	media_node_id _mID;
};

_MediaControlPanel::_MediaControlPanel(media_node_id id)
	: BWindow(BRect(100,100,110,110), "Media Control Panel", B_TITLED_WINDOW,
		B_NOT_RESIZABLE | B_NOT_ZOOMABLE), _mID(id)
{
	status_t err = Init();
	Show();
	if (err != B_OK) {
		Hide();
		PostMessage(B_QUIT_REQUESTED);
	}
}

void
_MediaControlPanel::Quit()
{
	// Let the app know that we're going away.
	MediaControlPanelApp* app;
	app = dynamic_cast<MediaControlPanelApp*>(be_app);
	if (app)
		app->ControlPanelQuitting(_mID);
	BWindow::Quit();
}

status_t
_MediaControlPanel::Init()
{
	live_node_info info;
	status_t err;
	
	// The live node info gives us both the media_node we want
	// and the name of the node.
	err = FindLiveNodeForID(_mID, &info);
	if (err != B_OK)
		return err;
	
	// We use the media_node to obtain the parameter web.	
	BParameterWeb* web;
	err = BMediaRoster::Roster()->GetParameterWebFor(info.node, &web);
	if (err != B_OK)
		return err;
	
	// We use the parameter web and the default media theme
	// to get a complete view representing the node.
	BView* view = BMediaTheme::ViewFor(web);
	
	// Now shrink-wrap the window around this view.
	view->MoveTo(0,0);
	ResizeTo(view->Bounds().Width(), view->Bounds().Height());
	AddChild(view);
	
	// Have the window use the node's name as its title.
	SetTitle(info.name);
	
	return B_OK;
}

status_t _MediaControlPanel::FindLiveNodeForID(media_node_id id,
	live_node_info* info)
{
	media_node node;
	status_t err = BMediaRoster::Roster()->GetNodeFor(id, &node);
	if (err == B_OK)
		err = BMediaRoster::Roster()->GetLiveNodeInfo(node, info);

	return err;
}



MediaControlPanelApp::MediaControlPanelApp(const char* signature)
	: BApplication(signature)
{
}

void MediaControlPanelApp::ReadyToRun()
{
	if (! _mControlPanels.size()) {
		// No control panels were sucessfully created
		// during launch, so we bail out.
		PostMessage(B_QUIT_REQUESTED);
	}
}

void MediaControlPanelApp::ArgvReceived(int32 argc, char** argv)
{
	// The node ID is passed to us via argv!
	ArgvHandler argvHandler;
	argvHandler.Init(argc-1, argv+1);
	char* arg;
	const char* nodeKey = "node=";
	bool printHelp = false;
	bool validArg = false;
	while (argvHandler.NextArg(&arg) == B_OK) {
		if (! strncmp(arg, nodeKey, strlen(nodeKey))) {
			arg += strlen(nodeKey);
			int32 id;
			char* trailing;
			id = strtol(arg, &trailing, 0);
			if (trailing != arg && *trailing == '\0') {
				ShowControlPanel(id);
				validArg = true;
			}
		} else {
			fprintf(stderr, "Argument not understood: %s\n", arg);
			printHelp = true;
		}
	}
	if ((! validArg) || printHelp) {
		printf("Usage: <media_addon> {node=<node_id>}+\n");
		printf("%-15s%-30s\n", "<media_addon>", "The name of this add-on");
		printf("%-15s%-30s\n", "<node_id>", "The ID of the media node whose UI is to be displayed");
	}	
}

void MediaControlPanelApp::MessageReceived(BMessage* message)
{
	switch (message->what) {
	case _MEDIA_CONTROL_PANEL_GONE:
		{
			media_node_id id;
			if (message->FindInt32("_node_id", &id) == B_OK) {
				RemoveControlPanel(id);
			}
			break;
		}
	default:
		BApplication::MessageReceived(message);
		break;
	}
}

BWindow* MediaControlPanelApp::CreateControlPanel(media_node_id id)
{
	return new _MediaControlPanel(id);
}

void MediaControlPanelApp::ControlPanelQuitting(media_node_id id)
{
	BMessage msg(_MEDIA_CONTROL_PANEL_GONE);
	msg.AddInt32("_node_id", id);
	PostMessage(&msg);
}

void MediaControlPanelApp::ShowControlPanel(media_node_id id)
{
	map<media_node_id, BWindow*>::iterator i;
	i = _mControlPanels.find(id);
	if (i != _mControlPanels.end()) {
		// We've already created a control panel for this
		// node. Bring this panel to the front.
		BWindow* panel = i->second;
		panel->Show();
		panel->Activate(true);
	} else {
		// We haven't already created a control panel for
		// this node, so create the control panel and add
		// it to our registry.
		_mControlPanels[id] = CreateControlPanel(id);
	}
}

void MediaControlPanelApp::RemoveControlPanel(media_node_id id)
{
	map<media_node_id, BWindow*>::iterator i;
	i = _mControlPanels.find(id);
	if (i != _mControlPanels.end()) {
		// We found the control panel, so remove it
		// from our registry.
		_mControlPanels.erase(i);
		if (! _mControlPanels.size()) {
			// There are no control panels left.
			// Time to go away.
			PostMessage(B_QUIT_REQUESTED);
		}
	}
}
