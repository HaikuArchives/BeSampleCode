/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include "potview.h"
#include "potwin.h"
#include <Locker.h>

BLocker PotWin::s_instanceLocker("PotWin instance lock");
PotWin* PotWin::s_instance = 0;

PotWin* PotWin::Instance()
{
	if (s_instanceLocker.Lock()) {
		if (! s_instance) {
			s_instance = new PotWin();
		}
		s_instanceLocker.Unlock();
	}
	return s_instance;
}

void PotWin::Create()
{
	Instance();
}

PotWin::PotWin()
		: BWindow(BRect(50,275,460,420), "Color Controls", B_FLOATING_WINDOW,
		B_NOT_CLOSABLE | B_NOT_ZOOMABLE | B_NOT_RESIZABLE)
{
	BRect r = Bounds();
	m_pv = new PotView(r);
	AddChild(m_pv);
	
	Show();
}

