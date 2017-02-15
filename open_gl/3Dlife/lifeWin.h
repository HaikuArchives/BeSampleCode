/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include <Window.h>
#include "lifeView.h"

class lifeWin : public BWindow
{
 public:
 	lifeWin();
 	~lifeWin();
 	bool QuitRequested();
 private:
 	lifeView *mv;
};
