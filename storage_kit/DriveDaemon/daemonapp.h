// Copyright 2000, Be Incorporated. All Rights Reserved.
// This file may be used under the terms of the Be Sample Code License.

#ifndef _daemonapp_h
#define _daemonapp_h

#include <Application.h>
#include <List.h>

class Drive;
struct node_ref;

class DaemonApp : public BApplication {
public:
	DaemonApp();
	~DaemonApp();
	
	virtual	void ReadyToRun();
	virtual	void MessageReceived(BMessage *message);

protected:
	void WatchDir(BDirectory *dir);
	void AddDrive(const node_ref& nref, const BPath& path);
	void RemoveDrive(const node_ref& nref);
	void AddListener(const BMessenger& msgr, const BMessage& msg);
	void RemoveListener(const BMessenger& msgr);
	status_t GetNodeRef(const BMessage& msg, node_ref* ref) const;
	status_t GetEntryRef(const BMessage& msg, entry_ref* ref) const;
	void DoEntryCreated(const BMessage& msg);
	void DoEntryRemoved(const BMessage& msg);
	void DoEntryMoved(const BMessage& msg);
	void NotifyAdd(const Drive& drive);
	void NotifyRemove(const Drive& drive);
	void NotifyStatusChange(const Drive& drive, status_t status);
	
	BList m_drives;
	BList m_listeners;
};

#endif /* _daemonapp_h */
