// EasySampleWin.h
/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef EasySampleWin_H
#define EasySampleWin_H 1

#include <be/interface/Window.h>
#include <be/media/ParameterWeb.h>

class EasySampleWin : public BWindow
{
public:
	EasySampleWin(BRect frame);
	~EasySampleWin();

	bool QuitRequested();
	void MessageReceived(BMessage*);

private:
	BParameterWeb* mParamWeb;
};

#endif		// #ifndef ParamSampleWin_H
