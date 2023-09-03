/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

/* WatchItem.cpp */

#include "WatchItem.h"

#include <String.h>
#include <View.h>
#include <Volume.h>
#include <GraphicsDefs.h>
#include <Roster.h>
#include <Entry.h>
#include <stdio.h>
#include <Directory.h>

WatchItem::WatchItem(uint32 outlineLevel, bool expanded)
	: BListItem(outlineLevel, expanded), fDisplayString("UnInit"),
		fColor(black)
{
}


WatchItem::~WatchItem()
{
}

void 
WatchItem::DrawItem(BView *owner, BRect frame, bool complete)
{
	rgb_color current = owner->HighColor();
	owner->SetHighColor(fColor);
	owner->MovePenTo(frame.LeftBottom() - BPoint(0,2));
	owner->DrawString(fDisplayString.String());

	owner->SetHighColor(current);
}


void 
WatchItem::SetColor(rgb_color color)
{
	fColor = color;
}


AppItem::AppItem(team_id team)
	: fTeam(team)
{
	SetString();
}


AppItem::~AppItem()
{
}

void 
AppItem::Activate()
{
	SetColor(red);
}

void 
AppItem::Deactivate()
{
	SetColor(black);
}

const team_id 
AppItem::Team() const
{
	return fTeam;
}

void 
AppItem::SetString()
{
	//find the name of the application
	app_info info;
	be_roster->GetRunningAppInfo(fTeam, &info);
	BEntry entry(&info.ref);
	
	fDisplayString.SetTo("");
	char name[B_FILE_NAME_LENGTH];
	entry.GetName(name);
	fDisplayString += name;
	fDisplayString += " (";
	fDisplayString << fTeam;
	fDisplayString  += ")";	
	
}



VolItem::VolItem(BVolume *vol)
{
	if (!vol) return;
	fDevice = vol->Device();
	fID = get_volume_id(fDevice);
	SetString();
}


VolItem::~VolItem()
{
}

const dev_t 
VolItem::Device() const
{
	return fDevice;
}

const uint64 
VolItem::ID() const
{
	return fID;
}

void 
VolItem::Mount(dev_t device)
{
	fDevice = device;
	fMounted = true;
	SetString();
	SetColor(black);
}

void 
VolItem::Unmount()
{
	SetColor(blue);
	fMounted = false;
}

void 
VolItem::SetString()
{
	char string[B_FILE_NAME_LENGTH];
	sprintf(string, "( %15Ld | %3ld )   ", fID, fDevice);

	fDisplayString = string;

	char name[B_FILE_NAME_LENGTH];
	BVolume vol(fDevice);
	vol.GetName(name);
	fDisplayString += name;
	
}

/* the only truly interesting function here */
/* finds the root directory of the volume */
/* and looks for the be:volume_id attribute that */
/* should be scribed there.  If found it returns the */
/* id.  if not it returns B_ERROR */
extern uint64 
get_volume_id(dev_t device)
{
	BVolume vol(device);
	if(vol.InitCheck() != B_OK) return B_ERROR;

	BDirectory dir;
	vol.GetRootDirectory(&dir);

	if (dir.InitCheck() != B_OK) return B_ERROR;
	
	uint64 id = 0;
	if (dir.ReadAttr("be:volume_id", B_INT64_TYPE, 0, (void *)&id, sizeof(uint64)) < B_OK)
		return B_ERROR;
	else return id;
}


