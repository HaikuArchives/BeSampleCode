// Copyright 2000, Be Incorporated. All Rights Reserved.
// This file may be used under the terms of the Be Sample Code License.

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <Application.h>
#include <Debug.h>
#include <Directory.h>
#include <Entry.h>
#include <NodeMonitor.h>
#include <Path.h>
#include "drive.h"
#include "drivelistener.h"
#include "daemonapp.h"
#include "drivedaemon.h"

//////////////////////////////////////////////////////////////////////
// Constructor/Destructor

DaemonApp::DaemonApp()
	: BApplication("application/x-vnd.Be-DTS.DeviceDaemon")
{
}

DaemonApp::~DaemonApp()
{
	// Delete the listeners first, so that when we remove the drives,
	// they won't receive false notifications.
	int32 numListeners = m_listeners.CountItems();
	for (int32 i=numListeners-1; i>=0; i--) {
		DriveListener* l = static_cast<DriveListener*>(m_listeners.RemoveItem(i));
		delete l;
	}
	// Delete the drives
	int32 numDrives = m_drives.CountItems();
	for (int32 i=numDrives-1; i>=0; i--) {
		Drive* d = static_cast<Drive*>(m_drives.RemoveItem(i));
		PRINT(("entry removed 0x%016Lx\n", d->NodeRef().node));
		NotifyRemove(*d);
		delete d;
	}
}

//////////////////////////////////////////////////////////////////////
// Adding/Removing Drives

void DaemonApp::AddDrive(const node_ref& nref, const BPath& path)
{
	// First check to see whether the node we're given really
	// appears to be a raw drive device. In the devfs hierarchy,
	// these are represented by files named "raw".
	if (! strcmp(path.Leaf(), "raw")) {
		// It's a drive, all right! Make a note of it.
		Drive* d = new Drive(nref, path.Path());
		m_drives.AddItem(d);
		NotifyAdd(*d);
	}
}

void DaemonApp::RemoveDrive(const node_ref& nref)
{
	// Check to see whether we know about this device.
	// If we do, remove the device.
	int32 numDevices = m_drives.CountItems();
	for (int32 i=0; i<numDevices; i++) {
		Drive* d = static_cast<Drive*>(m_drives.ItemAt(i));
		if (nref == *d) {
			PRINT(("entry removed 0x%016Lx\n", nref.node));
			m_drives.RemoveItem(i);
			NotifyRemove(*d);
			delete d;
			break;
		}
	}
}

//////////////////////////////////////////////////////////////////////
// Adding/Removing Listeners

void DaemonApp::AddListener(const BMessenger& msgr, const BMessage& msg)
{
	m_listeners.AddItem(new DriveListener(msgr, msg));
}

void DaemonApp::RemoveListener(const BMessenger& msgr)
{
	int32 count = m_listeners.CountItems();
	for (int32 i=0; i<count; i++) {
		DriveListener* l = static_cast<DriveListener*>(m_listeners.ItemAt(i));
		if (l && *l == msgr) {
			m_listeners.RemoveItem(i);
			delete l;
			return;
		}
	}
}

//////////////////////////////////////////////////////////////////////
// Listener Notifications

void DaemonApp::NotifyAdd(const Drive& d)
{
	// Tell any interested parties that this drive has just been added.
	int32 count = m_listeners.CountItems();
	for (int32 i=0; i<count; i++) {
		DriveListener* l = static_cast<DriveListener*>(m_listeners.ItemAt(i));
		l->NotifyAdd(d);
	}
}

void DaemonApp::NotifyRemove(const Drive& d)
{
	// Tell any interested parties that this drive has just been removed.
	int32 count = m_listeners.CountItems();
	for (int32 i=0; i<count; i++) {
		DriveListener* l = static_cast<DriveListener*>(m_listeners.ItemAt(i));
		l->NotifyAdd(d);
	}		
}

void DaemonApp::NotifyStatusChange(const Drive& d, status_t status)
{
	// Tell any interested parties that this drive's media status just changed.
	int32 count = m_listeners.CountItems();
	for (int32 i=0; i<count; i++) {
		DriveListener* l = static_cast<DriveListener*>(m_listeners.ItemAt(i));
		l->NotifyStatusChange(d, status);
	}	
}

//////////////////////////////////////////////////////////////////////
// Node Monitor Message Utilities

status_t DaemonApp::GetNodeRef(const BMessage& msg, node_ref* ref) const
{
	// Construct a node_ref from a Node Monitor message.
	status_t err;
	err = msg.FindInt32("device", &(ref->device));
	if (err != B_OK)
		return err; 
	err = msg.FindInt64("node", &(ref->node));
	return err;
}

status_t DaemonApp::GetEntryRef(const BMessage& msg, entry_ref* ref) const
{
	// Construct an entry_ref from a Node Monitor message.
	// entry_refs are similar to node_refs except that they aren't
	// guaranteed to exist; only their parent directory is. This
	// explains the difference in structure.
	status_t err;
	const char *name;
	err = msg.FindString("name", &name);
	if (err != B_OK)
		return err;
	err = msg.FindInt32("device", &(ref->device)); 
	if (err != B_OK)
		return err;
	err = msg.FindInt64("directory", &(ref->directory));
	if (err != B_OK)
		return err;
	err = ref->set_name(name);
	return err; 
}

//////////////////////////////////////////////////////////////////////
// Handling NodeMonitor Messages

void DaemonApp::DoEntryCreated(const BMessage& msg)
{
	// The Node Monitor just notified us that something just got
	// created in one of the directories we're watching. Handle
	// the new entry in the appropriate way.
	entry_ref ref;
	if (GetEntryRef(msg, &ref) == B_OK) {
		BEntry entry(&ref);
		if (entry.InitCheck() == B_OK) {
			BPath path;
			node_ref nref;
			entry.GetPath(&path);
			entry.GetNodeRef(&nref);
				
			BDirectory newDir(&entry);
			if (newDir.InitCheck() == B_OK) {
				// Found a directory, so we need to monitor it.
				PRINT(("new dir 0x%016Lx: %s\n", nref.node, path.Path()));
				WatchDir(&newDir);
			} else {
				// This is a regular file; treat it as a device.
				AddDrive(nref, path);
			}
		} else {
			PRINT(("B_ENTRY_CREATED: not a valid entry\n"));
		}
	} else {
		PRINT(("B_ENTRY_CREATED: bad entry ref\n"));
	}	
}

void DaemonApp::DoEntryRemoved(const BMessage& msg)
{
	// The Node Monitor just notified us that something just got
	// removed in one of the directories we're watching. Update
	// our drive information accordingly.
	node_ref nref;
	if (GetNodeRef(msg, &nref) == B_OK) {
		RemoveDrive(nref);
	} else {
		PRINT(("B_ENTRY_REMOVED: bad node ref\n"));
	}
}

void DaemonApp::DoEntryMoved(const BMessage& msg)
{
	// The Node Monitor just notified us that one of the entries
	// we're watching just moved. We should never get this
	// notification, because entries in devfs shouldn't move 
	// around! Even if they do, however, the node_ref won't
	// change, so we don't need to do anything unusual here.
	node_ref nref;
	if (GetNodeRef(msg, &nref) == B_OK) {
		PRINT(("Error: entry 0x%016Lx moved!\n", nref.node));
	}
}

//////////////////////////////////////////////////////////////////////
// Starting the Node Monitor

// Sets up a node monitor to watch a directory, and recursively watches
// all subdirectories and drives within this directory.
void DaemonApp::WatchDir(BDirectory *dir)
{
	node_ref nref;
	dir->GetNodeRef(&nref);
	status_t err = watch_node(&nref, B_WATCH_ALL, be_app_messenger);
	if(err != B_NO_ERROR) {
		printf("could not watch directory\n");
		return;
	}
	BEntry entry;
	while(dir->GetNextEntry(&entry) == B_NO_ERROR) {
		BPath path(&entry);
		BDirectory subdir(&entry);
		if(subdir.InitCheck() == B_NO_ERROR) {
			// Recursively call WatchDir to pick up the contents
			// of the subdirectory.
			//PRINT(("watch subdir %s\n", path.Path()));
			WatchDir(&subdir);
		} else {
			// Try treating the entry as a drive.
			entry.GetNodeRef(&nref);
			AddDrive(nref, path);
		}
	}
}

//////////////////////////////////////////////////////////////////////
// Handling NodeMonitor Messages

void DaemonApp::MessageReceived(BMessage* msg)
{
	switch(msg->what) {
	case DRIVE_DAEMON_START_NOTIFY:
		{
			// Someb0ody wants to start receiving notifications.
			// Add him to our list of listeners.
			BMessenger msgr;
			if (msg->FindMessenger("messenger", &msgr) == B_OK) {
				// remove the messenger field, but preserve the
				// rest, since it's a set of criteria that the
				// listener is interested in.
				msg->RemoveName("messenger");
				AddListener(msgr, *msg);
			}
		}
		break;
	case DRIVE_DAEMON_STOP_NOTIFY:
		{
			// Somebody wants to stop receiving notifications.
			// Remove him from our list of listeners.
			BMessenger msgr;
			if (msg->FindMessenger("messenger", &msgr) == B_OK) {
				RemoveListener(msgr);
			}
		}
		break;
	case B_NODE_MONITOR:
		{
			// Dispatch the node monitor message.
			int32 opcode;
			if(msg->FindInt32("opcode", &opcode) == B_OK) {
				switch(opcode) {
				case B_ENTRY_CREATED:
					DoEntryCreated(*msg);
					break;
				case B_ENTRY_REMOVED:
					DoEntryRemoved(*msg);
					break;
				case B_ENTRY_MOVED:
					DoEntryMoved(*msg);
					break;
				default:
					break;
				}
			} else {
				PRINT(("B_NODE_MONITOR: couldn't find opcode\n"));
			}
		}
		break;
	case _DRIVE_STATUS_CHANGED:
		{
			// A drive just notified us that its status changed.
			// Dispatch this notification.
			Drive* d;
			status_t status;
			if (msg->FindPointer("_drive", (void**) &d) == B_OK
				&& msg->FindInt32("_status", &status) == B_OK)
			{
				NotifyStatusChange(*d, status);
			}
		}
		break;
	default:
		BApplication::MessageReceived(msg);
		break;
	}
}

void DaemonApp::ReadyToRun()
{
	BDirectory diskdir("/dev/disk");
	if(diskdir.InitCheck() != B_NO_ERROR) {
		PRINT(("could not open /dev/disk\n"));
		Quit();
		return;
	}
	WatchDir(&diskdir);
}

int main()
{
	DaemonApp app;
	app.Run();
	return 0;
}
