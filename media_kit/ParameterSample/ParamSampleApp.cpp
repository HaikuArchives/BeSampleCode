// ParamSampleApp.cpp
/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include "ParamSampleApp.h"
#include "EasySampleWin.h"
#include "HarderSampleWin.h"
#include <stdio.h>

ParamSampleApp::ParamSampleApp(const char *signature)
	: BApplication(signature)
{
}


ParamSampleApp::~ParamSampleApp()
{
}

void 
ParamSampleApp::ReadyToRun()
{
	(new EasySampleWin(BRect(100, 50, 200, 150)))->Show();
	(new HarderSampleWin(BRect(50, 50, 150, 150)))->Show();
}
