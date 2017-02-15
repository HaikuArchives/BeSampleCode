// HarderSampleWin.cpp
/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include "HarderSampleWin.h"
#include <be/media/ParameterWeb.h>
#include <be/media/TimeSource.h>
#include <be/media/MediaRoster.h>
#include <be/media/MediaTheme.h>
#include <be/app/Application.h>
#include <be/interface/MultiChannelControl.h>
#include <math.h>
#include <stdio.h>

// a unique message tag for our use
static const uint32 gControlMessage = 'myCC';

// Parameter window implementation
HarderSampleWin::HarderSampleWin(BRect frame)
	: BWindow(frame, "HarderSampleWin", B_TITLED_WINDOW, B_ASYNCHRONOUS_CONTROLS)
{
	media_node mixerNode;

	BMediaRoster* roster = BMediaRoster::Roster();
	roster->GetAudioMixer(&mixerNode);
	roster->GetParameterWebFor(mixerNode, &mParamWeb);

	// find the master gain parameter
	int32 numParams = mParamWeb->CountParameters();
	BParameter* param = NULL;
	for (int32 i = 0; i < numParams; i++)
	{
		BParameter* p = mParamWeb->ParameterAt(i);
		if (!strcmp(p->Kind(), B_MASTER_GAIN))
		{
			param = p;
			break;
		}
	}

	// we need it to be the right type; this will terminate the app
	// if we didn't find it
	if (param->Type() == BParameter::B_CONTINUOUS_PARAMETER)
	{
		mGainParam = dynamic_cast<BContinuousParameter*>(param);
	}
	else
	{
		printf("ERROR: expected the system mixer to be a BContinuousParameter!\n");
		exit(1);
	}

	// get the single control for this parameter
	BMediaTheme* theme = BMediaTheme::PreferredTheme();
	BControl* control = theme->MakeControlFor(mGainParam);
	mGainControl = dynamic_cast<BMultiChannelControl*>(control);

	// configure the control's "change" message so we can detect it
	mGainControl->SetMessage(new BMessage(gControlMessage));

	// calculate the scaling from the control's range to the parameter's
	float paramMin = mGainParam->MinValue();
	float paramMax = mGainParam->MaxValue();
	int32 controlMin, controlMax;
	mGainControl->GetLimitsFor(0, &controlMin, &controlMax);
	mScale = (paramMax - paramMin) / (controlMax - controlMin);
	mOffset = paramMax - mScale * controlMax;
	mControlMin = controlMin;

	// put it in the window and resize to fit the control
	AddChild(mGainControl);
	ResizeTo(mGainControl->Bounds().Width(), mGainControl->Bounds().Height());
}

HarderSampleWin::~HarderSampleWin()
{
	// must delete the BParameterWeb
	delete mParamWeb;
}

bool 
HarderSampleWin::QuitRequested()
{
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}

void 
HarderSampleWin::MessageReceived(BMessage *msg)
{
	// is it one of our control messages?
	if (msg->what == gControlMessage)
	{
		// support stereo (2-channel) controls
		int32 chan1, chan2;

		// first check for multichannel data
		status_t err = msg->FindInt32("be:channel_value", 0, &chan1);
		if (!err) err = msg->FindInt32("be:channel_value", 1, &chan2);
		else		// mono data - get the value, assign it to both channels
		{
			err = msg->FindInt32("be:value", &chan1);
			chan2 = chan1;
		}
		if (err)
		{
			printf("ERROR: control message without a value member!\n");
			msg->PrintToStream();
			chan1 = chan2 = mControlMin;
		}

		// transform the control values into their proper ranges - this is always
		// a linear mapping from the control's range to the parameter's
		float chanData[2];
		chanData[0] = mScale * chan1 + mOffset;
		chanData[1] = mScale * chan2 + mOffset;

		// figure out what "now" is in the performance time base associated
		// with the node that the BParameterWeb controls
		BMediaRoster* roster = BMediaRoster::Roster();
		BTimeSource* timeSource = roster->MakeTimeSourceFor(mParamWeb->Node());
		bigtime_t now = timeSource->Now();

		// now set the BParameter!
		mGainParam->SetValue(&chanData, sizeof(chanData), now);

		// must delete the time source from BMediaRoster::MakeTimeSourceFor(...)
		timeSource->Release();
	}
	else BWindow::MessageReceived(msg);		// not a control message; pass it on
}

