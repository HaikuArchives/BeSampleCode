// HarderSampleWin.h
/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef HarderSampleWin_H
#define HarderSampleWin_H 1

#include <be/interface/Window.h>

class BParameterWeb;
class BContinuousParameter;
class BMultiChannelControl;

class HarderSampleWin : public BWindow
{
public:
	HarderSampleWin(BRect frame);
	~HarderSampleWin();

	bool QuitRequested();
	void MessageReceived(BMessage*);

private:
	BParameterWeb* mParamWeb;
	BMultiChannelControl* mGainControl;
	BContinuousParameter* mGainParam;
	float mScale, mOffset;
	int32 mControlMin;
};

#endif		// #ifndef ParamSampleWin_H
