// Copyright 2000, Be Incorporated. All Rights Reserved.
// This file may be used under the terms of the Be Sample Code License.

#ifndef _drive_h
#define _drive_h

#include <Drivers.h>
#include <Node.h>
#include <String.h>

enum {
	_DRIVE_STATUS_CHANGED = '_dST'
};

class Drive {
	public:
		Drive(node_ref nref, const char *path);
		~Drive();
		
		const node_ref& NodeRef() const { return m_ref; }
		const device_geometry& Geometry() const { return m_geom; }
		const char* Name() const { return m_path.String(); }
		
	protected:
		void SetStatus(status_t status);
		void TrackStatus();
		static status_t track_status_entry(void *obj);
		
		node_ref m_ref;
		BString m_path;
		int m_fd;
		device_geometry m_geom;
		status_t m_status;
		thread_id m_threadID;
		BLocker* m_lock;
		bigtime_t m_pollSnooze;
};

inline bool operator==(const node_ref& nref, const Drive& drive)
{
	return (drive.NodeRef() == nref);
}

inline bool operator==(const Drive& drive, const node_ref& nref)
{
	return (drive.NodeRef() == nref);
}

#endif /* _drive_h */
