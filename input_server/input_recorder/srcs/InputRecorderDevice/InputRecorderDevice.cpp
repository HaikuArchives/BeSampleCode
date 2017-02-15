/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <Debug.h>
#include <FindDirectory.h>
#include <InterfaceDefs.h>
#include <Message.h>
#include <OS.h>
#include <add-ons/input_server/InputServerDevice.h>
#include <SupportDefs.h>

extern "C" _EXPORT BInputServerDevice* instantiate_input_device();

class InputRecorderDevice;

class InputRecorderDevice : public BInputServerDevice {
public:
	InputRecorderDevice();
	virtual ~InputRecorderDevice();

	virtual status_t Start(const char *device, void *cookie);
	virtual	status_t Stop(const char *device, void *cookie);

private:
	thread_id _Thread;
	static int32 _StartWatchPort(void *arg);
	void _WatchPort();
	port_id _Port;
};


BInputServerDevice* instantiate_input_device()
{
	return (new InputRecorderDevice());
}


InputRecorderDevice::InputRecorderDevice()
	: BInputServerDevice()
{
	input_device_ref *device[2];
	input_device_ref mover = {"InputRecorderDevice", B_POINTING_DEVICE, NULL};
	device[0] = &mover;
	device[1] = NULL;
	_Port = -1;

	RegisterDevices(device);
}


InputRecorderDevice::~InputRecorderDevice()
{
}


status_t InputRecorderDevice::Start(const char *device, void *cookie)
{
	_Port = create_port(100,"InputDevicePort");
	if(_Port < 0) {
		_sPrintf("InputRecorderDevice: create_port error: (0x%x) %s\n",_Port,strerror(_Port));
		return(_Port);
	}
	_Thread = spawn_thread(_StartWatchPort, device, B_REAL_TIME_DISPLAY_PRIORITY+4, this);
	resume_thread(_Thread);
	return (B_NO_ERROR);
}


status_t InputRecorderDevice::Stop(const char *device, void *cookie)
{
	kill_thread(_Thread);
	delete_port(_Port);
	_Port = -1;
	return (B_NO_ERROR);
}

int32 InputRecorderDevice::_StartWatchPort(void *arg)
{
	InputRecorderDevice *self = (InputRecorderDevice*)arg;
	self->_WatchPort();
	return (B_NO_ERROR);
}

void InputRecorderDevice::_WatchPort()
{
	int32 code;
	ssize_t length;
	char *buffer;
	BMessage *event;
	status_t err;

	while (true) {
		// Block until we find the size of the next message
		length = port_buffer_size(_Port);
		buffer = (char*)malloc(length);
		event = NULL;
		event = new BMessage();
		err = read_port(_Port, &code, buffer, length);
		if(err != length) {
			if(err >= 0) {
				_sPrintf("InputRecorderDevice: failed to read full packet (read %u of %u)\n",err,length);
			} else {
				_sPrintf("InputRecorderDevice: read_port error: (0x%x) %s\n",err,strerror(err));
			}
		} else if ((err = event->Unflatten(buffer)) < 0) {
			_sPrintf("InputRecorderDevice: (0x%x) %s\n",err,strerror(err));
		} else {
			EnqueueMessage(event);	
			event = NULL;
		}
		free( buffer);
		if(event!=NULL) {
			delete(event);
			event = NULL;
		}
	}
}

