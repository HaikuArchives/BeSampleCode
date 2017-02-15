/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include "nervous.h"
#include <InterfaceDefs.h>
#include <stdlib.h>


bool		NervousInputDevice::sActive = false;
int32		NervousInputDevice::sSpeed = 0;
thread_id	NervousInputDevice::sThread = B_ERROR;


BInputServerDevice*
instantiate_input_device()
{
	// this is where it all starts
	// make sure this function is exported!

	return (new NervousInputDevice());
}


NervousInputDevice::NervousInputDevice()
	: BInputServerDevice()
{
	// register your device(s) with the input_server
	// the last device in the list should be NULL

	input_device_ref	nervousDevice			= { "Nervous Device",
													B_POINTING_DEVICE,
													NULL };
	input_device_ref	*nervousDeviceList[2]	= {&nervousDevice, NULL};

	RegisterDevices(nervousDeviceList);	
}


NervousInputDevice::~NervousInputDevice()
{
	// cleanup
}


status_t
NervousInputDevice::InitCheck()
{
	// do any init code that could fail here
	// you will be unloaded if you return false

	return (BInputServerDevice::InitCheck());
}


status_t
NervousInputDevice::SystemShuttingDown()
{
	// do any cleanup (ie. saving a settings file) when the
	// system is about to shut down

	return (BInputServerDevice::SystemShuttingDown());
}


status_t
NervousInputDevice::Start(
	const char	*device,
	void		*cookie)
{
	// start generating events
	// this is a hook function, it is called for you
	// (you should not call it yourself)

	sActive = true;
	get_mouse_speed(&sSpeed);
	sThread = spawn_thread(the_nerves, device, B_LOW_PRIORITY, this);

	resume_thread(sThread);

	return (B_NO_ERROR);
}


status_t
NervousInputDevice::Stop(
	const char	*device,
	void		*cookie)
{
	// stop generating events
	// this is a hook function, it is called for you
	// (you should not call it yourself)

	status_t err = B_OK;

	sActive = false;
	wait_for_thread(sThread, &err);
	sThread = B_ERROR;

	return (B_NO_ERROR);
}


status_t
NervousInputDevice::Control(
	const char	*device,
	void		*cookie,
	uint32		code,
	BMessage	*message)
{
	// respond to changes in the system
	// this is a hook function, it is called for you
	// (you should not call it yourself)

	switch (code) {
		case B_MOUSE_SPEED_CHANGED:
			get_mouse_speed(&sSpeed);
			break;

		default:
			break;
	}

	return (B_NO_ERROR);
}


int32
NervousInputDevice::the_nerves(
	void	*arg)
{
	// randomly generates mouse moved events

	const int32 kMoveAmount[] = { -3, -2, -2, -1, -1, 
								  0, 0, 0, 0, 0, 0, 
								  1, 1, 2, 2, 3 };

	NervousInputDevice *nervousDevice = (NervousInputDevice *)arg;

	srandom(system_time());

	while (nervousDevice->sActive) {
		bigtime_t	now = system_time();
		BMessage	*event = new BMessage(B_MOUSE_MOVED);
		event->AddInt64("when", now);
		event->AddInt32("buttons", 0);
		event->AddInt32("x", kMoveAmount[(random() >> 16) & 15]);
		event->AddInt32("y", kMoveAmount[(random() >> 5) & 15]);

		nervousDevice->EnqueueMessage(event);

		snooze(150000 - (10000 * nervousDevice->sSpeed));
	}

	return (B_OK);
}

