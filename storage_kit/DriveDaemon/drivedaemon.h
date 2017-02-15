// Copyright 2000, Be Incorporated. All Rights Reserved.
// This file may be used under the terms of the Be Sample Code License.

#ifndef _drivedaemon_h
#define _drivedaemon_h

#define DRIVE_DAEMON_SIG "application/x-vnd.Be-DTS.DriveDaemon"

// Message protocols
enum {
	// --------
	// Client -> Server Protocols
	// --------
	
	// DRIVE_DAEMON_START_NOTIFY -- notify your app when stuff happens to a
	// device or group of devices.
	//
	// Required field:
	//    "messenger" (B_MESSENGER_TYPE) -- the intended recipient of notifications
	// Optional fields:
	//    "type" (B_INT32_TYPE) -- array of the device types you're interested in
	//       (one of the device types in Drivers.h)
	//    "removable" (B_BOOL_TYPE) -- whether you're only interested in removable or
	//       non-removable drives (not present = both)
	//    "name" (B_STRING_TYPE) -- array of specific names you're interested in
	// If no optional fields are specified, information about all drives will be
	// sent to you. 
	DRIVE_DAEMON_START_NOTIFY = 'ddSN',
	
	// DRIVE_DAEMON_STOP_NOTIFY -- stop notifying your app about all device
	// changes
	// "messenger" (B_MESSENGER_TYPE) -- the messenger you originally specified
	DRIVE_DAEMON_STOP_NOTIFY = 'ddQN',


	// --------
	// Server -> Client Protocols
	// --------	
	
	// DRIVE_DAEMON_GONE -- the drive daemon has stopped running.
	DRIVE_DAEMON_GONE = 'ddGN',
	
	// DRIVE_ADDED -- notification that a drive was added.
	// "name" (B_STRING_TYPE) -- the name of the device in the devfs hierarchy
	DRIVE_ADDED = 'dADD',

	// DRIVE_REMOVED -- notification that a drive was removed.
	// "name" (B_STRING_TYPE) -- the name of the device in the devfs hierarchy
	DRIVE_REMOVED = 'dREM',
	
	// DRIVE_STATUS_CHANGED -- notification that a drive's media status has changed.
	// "name" (B_STRING_TYPE) -- the name of the device in the devfs hierarchy
	// "status" (B_INT32_TYPE) -- new status (see Drivers.h : B_GET_MEDIA_STATUS)
	DRIVE_STATUS_CHANGED = 'dSTA'
};

#endif /* _drivedaemon_h */
