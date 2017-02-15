// Copyright 2000, Be Incorporated. All Rights Reserved.
// This file may be used under the terms of the Be Sample Code License.

#include <Application.h>
#include <Debug.h>
#include <Drivers.h>
#include <Locker.h>
#include <Message.h>
#include <string.h>
#include <errno.h>

#include "drive.h"

static const bigtime_t kFastSnooze = 100000LL;
static const bigtime_t kSlowSnooze = 1000000LL;

Drive::Drive(node_ref nref, const char *path)
	: m_ref(nref), m_path(path)
{
	m_threadID = -1;
	m_lock = NULL;
	m_pollSnooze = kSlowSnooze;
		
	PRINT(("new entry %s\n", path));

	m_fd = open(path, O_RDONLY | O_CLOEXEC);
	if(m_fd < 0) {
		PRINT(("could not open %s, %s\n", path, strerror(errno)));
		return;
	}

	// What kind of a drive is this?
	if(ioctl(m_fd, B_GET_GEOMETRY, &m_geom, sizeof(device_geometry)) < B_OK) {
		PRINT(("could not get geometry for %s, %s\n", path, strerror(errno)));
		return;
	}
	
	if(!m_geom.removable) {
		// This drive doesn't have removable media.
		PRINT(("new fixed Drive %s\n", path));
	} else {
		// This drive has removable media.
		// Check to see what the initial media status is, and
		// spawn a thread to track the changing media status.
		m_status = B_ERROR;
		if(ioctl(m_fd, B_GET_MEDIA_STATUS, &m_status, sizeof(status_t)) == B_OK) {
		
			// Repeatedly polling the floppy drive is noisy, so we don't spawn
			// a polling thread for it. You could add an option to poll the floppy
			// if you wish (an exercise for the reader).
			if(! strcmp(path, "/dev/disk/floppy/raw")) {
				PRINT(("new floppy %s, media status: %s\n", path, strerror(m_status)));
				return;
			}

			PRINT(("new Drive %s, media status: %s\n", path, strerror(m_status)));

			char name[256];
			sprintf(name, "%s drive lock", path);
			m_lock = new BLocker(name);
			sprintf(name, "%s status tracker", path);
			m_threadID = spawn_thread(track_status_entry, name, B_NORMAL_PRIORITY, this);
			if (m_threadID > 0) {
				resume_thread(m_threadID);
			} else {
				PRINT(("%s: couldn't spawn status tracking thread\n", path));
			}
		}
		else {
			PRINT(("%s: no media status\n", path));
		}
	}
}

Drive::~Drive()
{
	if (m_lock && m_lock->Lock()) {
		PRINT(("%s gone\n", m_path.String()));
		if(m_fd >= 0) {
			close(m_fd);
		}
		thread_id tid = m_threadID;

		delete m_lock;
		m_lock = NULL;
		
		if(tid >= 0) {
			status_t exitStatus;
			wait_for_thread(tid, &exitStatus);
		}
	}
}

status_t Drive::track_status_entry(void *obj)
{
	Drive *d = static_cast<Drive*>(obj);
	d->TrackStatus();
	return B_OK;
}

void Drive::SetStatus(status_t status)
{
	if(status != m_status) {
		PRINT(("%s media status: %s\n", m_path.String(), strerror(status)));
		BMessage msg(_DRIVE_STATUS_CHANGED);
		msg.AddPointer("_drive", this);
		msg.AddInt32("_status", status);
		
		// be careful in case the app's port is full, so we don't deadlock.
		// only update status if notification to app was successful,
		// otherwise we'll retry it again in a bit.
		if (be_app_messenger.SendMessage(&msg, (BHandler*)NULL, 100000) == B_OK) {
			m_status = status;
			// if device is not ready, something's about to happen
			// (e.g. CD being inserted), so poll faster
			m_pollSnooze = (status == B_DEV_NOT_READY) ? kFastSnooze : kSlowSnooze;
		} else {
			// need to retry the status change, so poll faster
			m_pollSnooze = kFastSnooze;
		}
	}
}

void Drive::TrackStatus()
{
	while(m_lock && m_lock->Lock()) {
		if (m_fd >= 0) {
			// Poll the drive for its current media status.
			status_t curStatus = B_ERROR;
			if (ioctl(m_fd, B_GET_MEDIA_STATUS, &curStatus, sizeof(status_t)) == B_OK) {
				SetStatus(curStatus);
			} else {
				PRINT(("%s: no media status\n", m_path.String()));
			}
			
			m_lock->Unlock();
			
			// Sleep for a bit between polls.
			snooze(m_pollSnooze);
		} else {
			m_threadID = -1;
			m_lock->Unlock();
			return;
		}
	}
	m_threadID = -1;
}
