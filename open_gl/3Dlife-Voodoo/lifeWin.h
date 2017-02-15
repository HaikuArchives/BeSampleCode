/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include <Window.h>
#include "lifeView.h"

//vd class lifeWin : public BWindow
class lifeWin : public BDirectWindow
{
 public:
 	lifeWin();
 	~lifeWin();
 	bool QuitRequested();
	virtual void DirectConnected(direct_buffer_info *info); //voodoo
 private:
 	lifeView *mv;
};
