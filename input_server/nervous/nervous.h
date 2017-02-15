/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef _NERVOUS_H
#define _NERVOUS_H

#include <add-ons/input_server/InputServerDevice.h>
#include <List.h>
#include <OS.h>
#include <SupportDefs.h>


// export this for the input_server
extern "C" _EXPORT BInputServerDevice* instantiate_input_device();
 

class NervousInputDevice : public BInputServerDevice {
public:
							NervousInputDevice();
	virtual					~NervousInputDevice();

	virtual status_t		InitCheck();
	virtual status_t		SystemShuttingDown();

	virtual status_t		Start(const char *device, void *cookie);
	virtual	status_t		Stop(const char *device, void *cookie);
	virtual status_t		Control(const char	*device,
									void		*cookie,
									uint32		code, 
									BMessage	*message);

private:
	static int32			the_nerves(void *arg);
	
	static bool				sActive;
	static int32			sSpeed;
	static thread_id		sThread;
};


#endif