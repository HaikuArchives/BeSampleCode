/* vInfoView */
/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef V_INFO_VIEW_H
#define V_INFO_VIEW_H

#include <View.h>

#include "Defs.h"

/* flags for the volume flags bitfield */
enum {
	IS_PERSIST	= 1,	/* Persistant (non-virtual volume) */
	IS_RDONLY	= 2,	/* Read-Only */
	IS_REMOVE	= 4,	/* Removable */
	IS_SHARED	= 8,	/* Shared */
	IS_ATTR		= 16,	/* Knows Attributes */
	IS_MIME		= 32,	/* Knows Mime */
	IS_QUERY	= 64	/* Knows Query */
};

class BVolume;
class BListView;


class vInfoView : public BView {
	public:
						vInfoView(BVolume &vol, struct stat &st);
						~vInfoView();

		status_t		InitCheck() const;
		
		void			MessageReceived(BMessage *msg);
		void			Draw(BRect updateRect);
		
		/* return the name of the volume so that the window */
		/* title can be set appropriately */
		char *			GetName() const;
	
	private:
		dev_t			fDevice;
		status_t		fStatus;
		/* the names of the indexed attributes (if any) */
		BListView *		fIndexList;
		/* volume name */
		char *			fNameStr;
		/* volume mount point */
		char *			fMountPoint;
		/* volume creation time */
		char *			fCreateStr;
		/* volume modification time */
		char *			fModStr;
		/* total bytes and free bytes */
		char *			fSizeStr;
		/* bitfield describing various aspects of the volume */
		uint32			fVolumeFlags;
};

#endif

