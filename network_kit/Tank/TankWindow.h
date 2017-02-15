/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef TankWindow_H
#define TankWindow_H

#include <Window.h>
#include <View.h>
#include <Bitmap.h>
#include <MessageRunner.h>
#include "dfp.h"

#include "FishPortal.h"

class Fish;

const uint32 F_PULSE = '0foo';
/* 0foo for historical reasons. */

class TankView : public BView {
public:
	//constructor / destructor
	TankView(BRect, char *);
		
	//overridden functions
	void AttachedToWindow(void);
	void Draw(BRect);

	//added functions
	void AddFish(Fish *newFish);
	BList		fishList;
	bool		tank_left, tank_right, tank_up, tank_down;

	uint8		tank_x;
	uint8		tank_y;

private:
	FishPortal		*FishNet;
	virtual void MessageReceived(BMessage *message);

	void Update(void);
	BBitmap		redDot;
	BBitmap		greenDot;
	BBitmap		fishMap;
};

/**************/

class TankWindow : public BWindow {
public:
	TankWindow(uint8 tank_x, uint8 tank_y);
	virtual bool QuitRequested();
	
private:

	BMessageRunner	*pulseRunner;

	
	TankView	*tankView;
};

#endif