/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include "lifeWin.h"
#include <Menu.h>
#include <MenuItem.h>
#include <MenuBar.h>

lifeWin::lifeWin()
	: BWindow(BRect(100,100,510,510), "3D Life", B_TITLED_WINDOW, 0)
{
	mv = new lifeView(BRect(0,20,410,410));
	AddChild(mv);

	BMenuItem *item;
	BMenuBar *mb = new BMenuBar(BRect(0,0,410,20), "menubar");
	
	BMenu *menu = new BMenu("File");
	menu->AddItem(new BMenuItem("About", new BMessage(B_ABOUT_REQUESTED)));
	menu->AddItem(new BMenuItem("Quit", new BMessage(B_QUIT_REQUESTED)));
	menu->SetTargetForItems(be_app);
	mb->AddItem(menu);
		
	menu = new BMenu("Rules");
	menu->AddItem(item = new BMenuItem("Life 4555", new BMessage(LIFE_4555)));
		menu->AddItem(new BMenuItem("Life 5766", new BMessage(LIFE_5766)));
	item->SetMarked(true);
	menu->SetTargetForItems(mv);
	menu->SetRadioMode(true);
	mb->AddItem(menu);
	
	menu = new BMenu("Control");
	menu->AddItem(new BMenuItem("Single Step Once", new BMessage(SINGLE_STEP), 
					's', B_COMMAND_KEY));
	menu->AddItem(new BMenuItem("Continuous Run", new BMessage(CONTINUOUS), 
					'r', B_COMMAND_KEY));
	menu->SetTargetForItems(mv);
	mb->AddItem(menu);
	
	
	menu = new BMenu("Rotation");	
	BMenu *xmenu;
	BMenu *ymenu;
	BMenu *zmenu;
	menu->AddItem(new BMenuItem(xmenu = new BMenu("X Rotation"), NULL));
	menu->AddItem(new BMenuItem(ymenu = new BMenu("Y Rotation"), NULL));
	menu->AddItem(new BMenuItem(zmenu = new BMenu("Z Rotation"), NULL));
	
	xmenu->AddItem(item = new BMenuItem("No Spin", new BMessage(X_SPIN_STOP)));
	item->SetMarked(true);
	xmenu->AddItem(new BMenuItem("Positive Spin", new BMessage(X_SPIN_POS),
									'x', B_COMMAND_KEY));
	xmenu->AddItem(new BMenuItem("Negative Spin", new BMessage(X_SPIN_NEG),
									'x', B_CONTROL_KEY));
	xmenu->SetRadioMode(true);
	
	ymenu->AddItem(item = new BMenuItem("No Spin", new BMessage(Y_SPIN_STOP)));
	item->SetMarked(true);
	ymenu->AddItem(new BMenuItem("Positive Spin", new BMessage(Y_SPIN_POS),
									'y', B_COMMAND_KEY));
	ymenu->AddItem(new BMenuItem("Negative Spin", new BMessage(Y_SPIN_NEG),
									'y', B_CONTROL_KEY));
	ymenu->SetRadioMode(true);
		
	zmenu->AddItem(item = new BMenuItem("No Spin", new BMessage(Z_SPIN_STOP)));
	item->SetMarked(true);
	zmenu->AddItem(new BMenuItem("Positive Spin", new BMessage(Z_SPIN_POS),
									'z', B_COMMAND_KEY));
	zmenu->AddItem(new BMenuItem("Negative Spin", new BMessage(Z_SPIN_NEG),
									'z', B_CONTROL_KEY));
	zmenu->SetRadioMode(true);
	
	xmenu->SetTargetForItems(mv);
	ymenu->SetTargetForItems(mv);
	zmenu->SetTargetForItems(mv);
	mb->AddItem(menu);
	AddChild(mb);
	
}

lifeWin::~lifeWin()
{
	// nothing yet
}

bool
lifeWin::QuitRequested()
{
	mv->ExitThreads();
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}
