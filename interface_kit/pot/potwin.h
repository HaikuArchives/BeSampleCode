/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef _potwin_h
#define _potwin_h

#include <Window.h>

class BPot;
class PotView;

class PotWin : public BWindow
{
public:
	static void Create();
	static PotWin* Instance();

protected:
	PotWin();
	
public:
	PotView* GetPotView() const { return m_pv; }
		
private:
	PotView* m_pv;
	
	static BLocker s_instanceLocker;
	static PotWin* s_instance;
};

#endif /* _potwin_h */