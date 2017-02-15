// ParamSampleApp.h
/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef ParamSampleApp_H
#define ParamSampleApp_H 1

#include <be/app/Application.h>

class ParamSampleApp : public BApplication
{
public:
	ParamSampleApp(const char* signature);
	~ParamSampleApp();

	void ReadyToRun();

private:
};

#endif		// #ifndef ParamSampleApp_H
