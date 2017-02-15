/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

/* WatchItem.h */

#ifndef WATCH_ITEM_H
#define WATCH_ITEM_H

#include <ListItem.h>
#include <OS.h>
#include <String.h>
#include <interface/GraphicsDefs.h>


class BString;
struct rgb_color;
class BView;

const rgb_color black  = {0,0,0,255};
const rgb_color red = {255,0,0,255};
const rgb_color blue = {0,0,255,255};


class WatchItem : public BListItem {
	public:
							WatchItem(uint32 outlineLevel = 2, bool expanded = false);
		virtual 			~WatchItem();
		virtual void		DrawItem(BView *owner, BRect frame, bool complete = false);
	protected:
		virtual void		SetColor(rgb_color color);
		virtual void		SetString() = 0;
		BString 			fDisplayString;
		rgb_color			fColor;
};

class AppItem : public WatchItem {
	public:
							AppItem(team_id team);
		virtual				~AppItem();
		void				Activate();
		void				Deactivate();
		const team_id		Team() const;
	private:
		virtual void		SetString();
		team_id				fTeam;
};

class BVolume;

class VolItem : public WatchItem {
	public:
							VolItem(BVolume *vol);
		virtual				~VolItem();
		const dev_t			Device() const;
		const uint64		ID() const;
		void				Mount(dev_t device);
		void				Unmount();
	private:
		virtual void		SetString();
		uint64				fID;
		dev_t				fDevice;
		bool				fMounted;
};

uint64 get_volume_id(dev_t device);

#endif
