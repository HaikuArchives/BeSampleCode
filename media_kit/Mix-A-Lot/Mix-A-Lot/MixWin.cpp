/*
	
	MixWin.cpp
	
	Implements the mixer window, with draggable sound file docks.
	
*/
/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include <stdio.h>
#include <string.h>

#include <BeBuild.h>
#include <Application.h>
#include <MediaRoster.h>
#include <MediaTheme.h>
#include <Message.h>
#include <Debug.h>

#include "MixWin.h"
#include "MixerManager.h"
#include "SoundDock.h"

static const uint32 SOUND_FILE_CHANGED = 'cSND';
static const float DOCK_VIEW_HEIGHT = 60;

MixWin::MixWin()
	: BWindow(BRect(50, 50, 100, 100), "Mix-A-Lot", B_TITLED_WINDOW, 0 )
{
	if (Init() == B_OK) {
		Show();
	} else {
		PostMessage(B_QUIT_REQUESTED);
	}
}

MixWin::~MixWin()
{
	delete m_mixerManager;
}

bool MixWin::QuitRequested()
{
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}

void MixWin::MessageReceived(BMessage* message)
{
	switch (message->what) {
	case SOUND_FILE_CHANGED:
		SoundFileChanged(message);
		break;
	case B_MEDIA_WEB_CHANGED:
		MediaWebChanged(message);
		break;
	default:
		BWindow::MessageReceived(message);
		break;
	}
}

status_t MixWin::Init()
{
	m_mixerManager = new MixerManager;
	m_mixerView = 0;
	m_dockBkg = 0;
	
	// Tell the media roster that we want to be informed of
	// all events that happen in the media system from now
	// on. (This allows us to catch events that affect our
	// mixer and its interface.)
	BMediaRoster::Roster()->StartWatching(BMessenger(0, this));
	status_t err = UpdateMixerView();
	if (err != B_OK) {
		delete m_mixerManager;
		m_mixerManager = 0;
		return err;
	}
	
	CreateSoundDocks();
	return B_OK;
}

status_t MixWin::UpdateMixerView()
{
	// This gets called whenever the mixer's view becomes invalid
	// (e.g. when the parameter web changes). In these cases, we
	// have to create a brand-new mixer view and replace the
	// existing view (if we had already created one).
	
	status_t err;
	
	media_node* mixerNode = m_mixerManager->Mixer();
	if (! mixerNode) {
		return B_ERROR;
	}
	
	BParameterWeb* mixerWeb;
	err = BMediaRoster::Roster()->GetParameterWebFor(*mixerNode, &mixerWeb);

	if (err == B_OK) {
		// Create the new mixer view, position it below the dock,
		// and resize the window to fully encompass the view.
		//
		// This code depends on the media theme to create a
		// correctly sized view.
		BMediaTheme* theme = BMediaTheme::PreferredTheme();
		BView* newMixerView = theme->ViewFor(mixerWeb);
		if (m_mixerView) {
			RemoveChild(m_mixerView);
			delete m_mixerView;
		}
		m_mixerView = newMixerView;

		m_mixerView->MoveBy(0, DOCK_VIEW_HEIGHT);
		ResizeTo(m_mixerView->Frame().right, m_mixerView->Frame().bottom);		
		AddChild(m_mixerView);
		
		UpdateLayout();
	} else {
		fprintf(stderr, "Couldn't get the mixer's parameter web: %s\n", strerror(err));
	}
	return err;
}

void MixWin::CreateSoundDocks()
{
	BRect r = m_mixerView->Frame();
	r.bottom = r.top-1; r.top = 0;
	m_dockBkg = new BView(r, "Docks", B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW);
	m_dockBkg->SetViewColor(m_mixerView->ViewColor());
	AddChild(m_dockBkg);

	for (int32 i=0; i<MIXER_MAX_CHANNELS; i++) {
		m_docks.AddItem(CreateSoundDock(m_dockBkg));
	}
	
	UpdateLayout();
}

SoundDock* MixWin::CreateSoundDock(BView* parent)
{
	SoundDock* dock = new SoundDock("Dock");
	
	// Tell the dock that we want to be notified when
	// the dock receives a sound file.
	dock->SetTarget(this);
	BMessage* msg = new BMessage(SOUND_FILE_CHANGED);
	dock->SetMessage(msg);

	dock->ResizeToPreferred();
	parent->AddChild(dock);
	return dock;
}

void MixWin::UpdateLayout()
{
	// This gets called when the mixer view changes. It
	// recalculates the positions of the docks so that
	// they align nicely with the mixer's channels.
	
	if (! (m_dockBkg && m_mixerView)) {
		return;
	}
	
	m_dockBkg->ResizeBy(m_mixerView->Frame().Width() - m_dockBkg->Frame().Width(), 0);
	BRect r = m_dockBkg->Frame();
	BPoint dockPt;
	dockPt.y = (r.top+r.bottom) / 2;
	
	float basex = 0.0f; // keeps track of the x-coord
						// so that we remain in window coords.
						
	// The mixer view has a child called "Channels" which
	// contains all of the channel views.
	BView* channelP = m_mixerView->FindView("Channels");
	basex += channelP->Frame().left;
	// We'll step through this view's children. Each child
	// encompasses the slider, Mute box, etc. for one channel.
	BView* channel = channelP->ChildAt(0);
	basex += channel->Frame().left;
	// BeginViewTransaction() disables clipping calculations and
	// invalid updates while we juggle the views around, where
	// the calculations are redundant. The bracketing EndView
	// Transaction() re-enables the calculations. For large #s
	// of views, the performance savings can be substantial.
	BeginViewTransaction();
	for (int32 channelNum = 0; channelNum < MIXER_MAX_CHANNELS; channelNum++) {
		// Repssition the corresponding dock to be centered above
		// our current channel view.
		ASSERT(channel);
		BRect fr = channel->Frame();
		dockPt.x = basex + (fr.left+fr.right) / 2;
		BView* dock = static_cast<BView*>(m_docks.ItemAt(channelNum));
		ASSERT(dock);
		CenterView(dock, dockPt); 
		channel = channel->NextSibling();
	}
	EndViewTransaction();
	m_mixerView->Invalidate();
	m_dockBkg->Invalidate();
}

void MixWin::CenterView(BView* v, BPoint where)
{
	float width = v->Bounds().Width();
	float height = v->Bounds().Height();	
	v->MoveTo(where.x - width/2, where.y - height/2);
}

void MixWin::SoundFileChanged(BMessage* message)
{
	// This gets called in reponse to a SOUND_FILE_CHANGED
	// message that's posted to our window from one of the
	// SoundDocks. We need to tell our mixer manager to
	// create a new connection for that channel.
	void* src;
	if (message->FindPointer("source", &src) == B_OK) {
		int32 index = m_docks.IndexOf(src);
		ASSERT(index >= 0);
		
		bool hasRef = message->HasRef("refs");
		if (hasRef) {
			entry_ref ref;
			message->FindRef("refs", &ref);
			m_mixerManager->SoundFileChanged(index, &ref);
		} else {
			// This means the file was detached from the
			// dock. Passing 0 disconnects the sound file
			// reader from the channel.
			m_mixerManager->SoundFileChanged(index, 0);
		}
	}			
}

void MixWin::MediaWebChanged(BMessage* message)
{
	// This gets called whenever any media node in the
	// BeOS changes its parameter web. If the node
	// happens to be ours, we'll update our mixer view.
	media_node* mixerNode = m_mixerManager->Mixer();
	if (mixerNode) {
		media_node_id mixerID = mixerNode->node;
		const media_node* changeNode;
		ssize_t changeNodeSize;
		int32 index=0;
		// The B_MEDIA_WEB_CHANGED notification tells us
		// which media_node was affected.
		while (message->FindData("node", B_RAW_TYPE, index++, (const void**) &changeNode,
			&changeNodeSize) == B_OK)
		{
			media_node_id changeID = changeNode->node;
			if (changeID == mixerID) {
				m_mixerManager->SetMixerControls();
				UpdateMixerView();
			}
		}
	}
}

bool MixWin::ChannelAvailable(SoundDock* dock)
{
	int32 index = m_docks.IndexOf(dock);
	if (index >= 0) {
		return m_mixerManager->ChannelAvailable(index);
	} else {
		return false;
	}
}
