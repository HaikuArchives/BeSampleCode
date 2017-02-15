// Copyright 2000, Be Incorporated. All Rights Reserved.
// This file may be used under the terms of the Be Sample Code License.

#include <Application.h>
#include <Debug.h>
#include <Messenger.h>
#include "drive.h"
#include "drivedaemon.h"
#include "drivelistener.h"

DriveListener::DriveListener(const BMessenger& msgr, const BMessage& msg)
	: m_msgr(msgr), m_msg(msg)
{}

DriveListener::~DriveListener()
{
	m_msgr.SendMessage(DRIVE_DAEMON_GONE, (BHandler*)NULL);
}

void DriveListener::NotifyAdd(const Drive& drive)
{
	if (ShouldListen(drive)) {
		BMessage msg(DRIVE_ADDED);
		msg.AddString("name", drive.Name());
		status_t err = m_msgr.SendMessage(&msg, be_app_messenger, 1000000);
		if (err != B_OK)
			PRINT(("error sending DRIVE_ADDED notification: %s\n", strerror(err)));
	}
}

void DriveListener::NotifyRemove(const Drive& drive)
{
	if (ShouldListen(drive)) {
		BMessage msg(DRIVE_REMOVED);
		msg.AddString("name", drive.Name());
		status_t err = m_msgr.SendMessage(&msg, be_app_messenger, 1000000);
		if (err != B_OK)
			PRINT(("error sending DRIVE_REMOVED notification: %s\n", strerror(err)));
	}
}

void DriveListener::NotifyStatusChange(const Drive& drive, status_t status)
{
	if (ShouldListen(drive)) {
		BMessage msg(DRIVE_STATUS_CHANGED);
		msg.AddString("name", drive.Name());
		msg.AddInt32("status", status);
		status_t err = m_msgr.SendMessage(&msg, be_app_messenger, 1000000);
		if (err != B_OK)
			PRINT(("error sending DRIVE_STATUS_CHANGED notification: %s\n", strerror(err)));
	}
}

bool DriveListener::ShouldListen(const Drive& drive) const
{
	const device_geometry& g = drive.Geometry();
	type_code msgType;
	int32 typeCount;
	int32 i;
	if (m_msg.GetInfo("type", &msgType, &typeCount) == B_OK
		&& typeCount > 0 && msgType == B_INT32_TYPE)
	{
		// check to see if the drive's geometry matches one of
		// the blessed types for this listener
		int32 curGeomType = g.device_type;
		for (i=0; i<typeCount; i++) {
			int32 desiredGeomType;
			if (m_msg.FindInt32("type", i, &desiredGeomType) == B_OK
				&& desiredGeomType == curGeomType)
			{
				break;
			}
		}
		if (i == typeCount) {
			// didn't find the type we were looking for
			return false;
		}
	}
	
	bool desiredRemovable;
	if (m_msg.FindBool("removable", &desiredRemovable) == B_OK
		&& desiredRemovable != g.removable)
	{
		// removable flag doesn't match
		return false;
	}
	
	if (m_msg.GetInfo("name", &msgType, &typeCount) == B_OK
		&& typeCount > 0 && msgType == B_STRING_TYPE)
	{
		// check to see if the drive's name matches one of
		// the blessed names for this listener
		const char* curName = drive.Name();
		for (i=0; i<typeCount; i++) {
			const char* desiredName;
			if (m_msg.FindString("name", i, &desiredName) == B_OK
				&& (! strcmp(desiredName, curName)))
			{
				break;
			}
		}
		if (i == typeCount) {
			// didn't find the right name
			return false;
		}
	}
	
	return true;	
}
