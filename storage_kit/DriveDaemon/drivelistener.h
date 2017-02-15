// Copyright 2000, Be Incorporated. All Rights Reserved.
// This file may be used under the terms of the Be Sample Code License.

#ifndef _drivelistener_h
#define _drivelistener_h

#include <Messenger.h>
#include <Message.h>

class Drive;

class DriveListener
{
public:
	DriveListener(const BMessenger& msgr, const BMessage& msg);
	~DriveListener();
	
	const BMessenger& Messenger() const { return m_msgr; }

	void NotifyAdd(const Drive& d);
	void NotifyRemove(const Drive& d);
	void NotifyStatusChange(const Drive& d, status_t status);
		
private:
	bool ShouldListen(const Drive& drive) const;

	BMessenger m_msgr;
	BMessage m_msg;
};

inline bool operator==(const BMessenger& msgr, const DriveListener& listener)
{
	return (msgr == listener.Messenger());
}

inline bool operator==(const DriveListener& listener, const BMessenger& msgr)
{
	return (msgr == listener.Messenger());
}

#endif /* _drivelistener_h */
