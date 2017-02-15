// NodeHarnessWin.cpp
/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include "NodeHarnessWin.h"
#include "LoggingConsumer.h"
#include <app/Application.h>
#include <interface/Button.h>
#include <storage/Entry.h>
#include <media/MediaRoster.h>
#include <media/MediaAddOn.h>
#include <media/TimeSource.h>
#include <media/MediaTheme.h>
#include <stdio.h>

const int32 BUTTON_CONNECT = 'Cnct';
const int32 BUTTON_START = 'Strt';
const int32 BUTTON_STOP = 'Stop';

#define TEST_WITH_AUDIO 1

// --------------------
// static utility functions
static void ErrorCheck(status_t err, const char* msg)
{
	if (err)
	{
		fprintf(stderr, "* FATAL ERROR (%s): %s\n", strerror(err), msg);
		exit(1);
	}
}

// --------------------
// NodeHarnessWin implementation
NodeHarnessWin::NodeHarnessWin(BRect frame, const char *title)
	:	BWindow(frame, title, B_TITLED_WINDOW, B_NOT_RESIZABLE | B_ASYNCHRONOUS_CONTROLS),
		mLogNode(NULL), mIsConnected(false), mIsRunning(false)
{
	// build the UI
	BRect r(10, 10, 100, 40);
	mConnectButton = new BButton(r, "Connect", "Connect", new BMessage(BUTTON_CONNECT));
	mConnectButton->SetEnabled(true);
	AddChild(mConnectButton);
	r.OffsetBy(0, 40);
	mStartButton = new BButton(r, "Start", "Start", new BMessage(BUTTON_START));
	mStartButton->SetEnabled(false);
	AddChild(mStartButton);
	r.OffsetBy(0, 40);
	mStopButton = new BButton(r, "Stop", "Stop", new BMessage(BUTTON_STOP));
	mStopButton->SetEnabled(false);
	AddChild(mStopButton);
}

NodeHarnessWin::~NodeHarnessWin()
{
	BMediaRoster* r = BMediaRoster::Roster();

	// tear down the node network
	if (mIsRunning) StopNodes();
	if (mIsConnected)
	{
		printf("Total late buffers: %ld\n", mLogNode->LateBuffers());
		r->StopNode(mConnection.consumer, 0, true);
		r->Disconnect(mConnection.producer.node, mConnection.source,
			mConnection.consumer.node, mConnection.destination);
		r->ReleaseNode(mConnection.producer);
		r->ReleaseNode(mConnection.consumer);
	}
}

void 
NodeHarnessWin::Quit()
{
	be_app->PostMessage(B_QUIT_REQUESTED);
	BWindow::Quit();
}

void 
NodeHarnessWin::MessageReceived(BMessage *msg)
{
	status_t err;

	switch (msg->what)
	{
	case BUTTON_CONNECT:
		mIsConnected = true;

		// set the button states appropriately
		mConnectButton->SetEnabled(false);
		mStartButton->SetEnabled(true);

		// set up the node network
		{
			BMediaRoster* r = BMediaRoster::Roster();

			// find a node that can handle an audio file
#if TEST_WITH_AUDIO
			entry_ref inRef;
			dormant_node_info info;

			::get_ref_for_path("/boot/optional/sound/virtual (void)", &inRef);
			err = r->SniffRef(inRef, B_BUFFER_PRODUCER | B_FILE_INTERFACE, &info);
			ErrorCheck(err, "couldn't find file reader node\n");

			err = r->InstantiateDormantNode(info, &mConnection.producer, B_FLAVOR_IS_LOCAL);
			ErrorCheck(err, "couldn't instantiate file reader node\n");

			bigtime_t dummy_length;			// output = media length; we don't use it
			err = r->SetRefFor(mConnection.producer, inRef, false, &dummy_length);
			ErrorCheck(err, "unable to SetRefFor() to read that sound file!\n");
#else
			r->GetVideoInput(&mConnection.producer);
#endif

			entry_ref logRef;
			::get_ref_for_path("/tmp/node_log", &logRef);

			mLogNode = new LoggingConsumer(logRef);
			err = r->RegisterNode(mLogNode);
			ErrorCheck(err, "unable to register LoggingConsumer node!\n");
			// make sure the Media Roster knows that we're using the node
			r->GetNodeFor(mLogNode->Node().node, &mConnection.consumer);

			// trim down the log's verbosity a touch
			mLogNode->SetEnabled(LOG_HANDLE_EVENT, false);

			// fire off a window with the LoggingConsumer's controls in it
			BParameterWeb* web;
			r->GetParameterWebFor(mConnection.consumer, &web);
			BView* view = BMediaTheme::ViewFor(web);
			BWindow* win = new BWindow(BRect(250, 200, 300, 300), "Controls",
				B_TITLED_WINDOW, B_ASYNCHRONOUS_CONTROLS);
			win->AddChild(view);
			win->ResizeTo(view->Bounds().Width(), view->Bounds().Height());
			win->Show();

			// set the nodes' time sources
			r->GetTimeSource(&mTimeSource);
			r->SetTimeSourceFor(mConnection.consumer.node, mTimeSource.node);
			r->SetTimeSourceFor(mConnection.producer.node, mTimeSource.node);

			// got the nodes; now we find the endpoints of the connection
			media_input logInput;
			media_output soundOutput;
			int32 count;
			err = r->GetFreeOutputsFor(mConnection.producer, &soundOutput, 1, &count);
			ErrorCheck(err, "unable to get a free output from the producer node");
			err = r->GetFreeInputsFor(mConnection.consumer, &logInput, 1, &count);
			ErrorCheck(err, "unable to get a free input to the LoggingConsumer");

			// fill in the rest of the Connection object
			mConnection.source = soundOutput.source;
			mConnection.destination = logInput.destination;

			// got the endpoints; now we connect it!
			media_format format;
#if TEST_WITH_AUDIO
			format.type = B_MEDIA_RAW_AUDIO;			// !!! hmmm.. how to fully wildcard this?
			format.u.raw_audio = media_raw_audio_format::wildcard;
#else
			format.type = B_MEDIA_RAW_VIDEO;			// !!! hmmm.. how to fully wildcard this?
			format.u.raw_video = media_raw_video_format::wildcard;
#endif
			err = r->Connect(mConnection.source, mConnection.destination, &format, &soundOutput, &logInput);
			ErrorCheck(err, "unable to connect nodes");
			mConnection.format = format;

#if !TEST_FOR_AUDIO
			// for video capture, we need to set the downstream latency for record -> playback
			bigtime_t latency;
			r->GetLatencyFor(mConnection.producer, &latency);
			r->SetRunModeNode(mConnection.producer, BMediaNode::B_RECORDING);
			r->SetProducerRunModeDelay(mConnection.producer, latency + 6000);			
#else
			// playing audio is playback, not capture, so we use a realtime playback mode
			r->SetRunModeNode(mConnection.producer, BMediaNode::B_DROP_DATA);
#endif

			// the logging consumer is always running in real time in this example
			r->SetRunModeNode(mConnection.consumer, BMediaNode::B_DROP_DATA);

			// preroll first, to be a good citizen
			r->PrerollNode(mConnection.consumer);
			r->PrerollNode(mConnection.producer);

			// start the LoggingConsumer and leave it running
			BTimeSource* ts = r->MakeTimeSourceFor(mTimeSource);
			r->StartNode(mConnection.consumer, ts->Now());
			ts->Release();
		}
		break;

	case BUTTON_START:
		mStartButton->SetEnabled(false);
		mStopButton->SetEnabled(true);

		// start the producer running
		{
			bigtime_t latency;
			BMediaRoster* r = BMediaRoster::Roster();
			BTimeSource* ts = r->MakeTimeSourceFor(mConnection.producer);
			r->GetLatencyFor(mConnection.producer, &latency);
			r->StartNode(mConnection.producer, ts->Now() + latency);
			ts->Release();
			mIsRunning = true;
		}
		break;

	case BUTTON_STOP:
		StopNodes();
		break;

	default:
		BWindow::MessageReceived(msg);
		break;
	}
}

// Private routines
void 
NodeHarnessWin::StopNodes()
{
	mStartButton->SetEnabled(true);
	mStopButton->SetEnabled(false);

	// stop the producer
	{
		BMediaRoster* r = BMediaRoster::Roster();
		r->StopNode(mConnection.producer, 0, true);		// synchronous stop
	}
}

