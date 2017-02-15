/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include "TankWindow.h"

#include <Application.h>
#include <Messenger.h>
#include <MessageRunner.h>
#include <socket.h>
#include <stdlib.h>
#include <stdio.h>

#include "FishBits.h"
#include "FishPortal.h"
#include "dfp.h"

TankView::TankView(BRect frame, char *title)
	: BView(frame, title, B_FOLLOW_ALL, B_WILL_DRAW | B_FRAME_EVENTS ),
	redDot(BRect(0,0,31,31), kRedDotColorSpace), greenDot(BRect(0,0,31,31), kGreenDotColorSpace),
	fishMap(BRect(0,0,31,31), kFishDataColorSpace)
{
	tank_left	=	false;
	tank_right	=	false;
	tank_up		=	false;
	tank_down	=	false;

}

/****************/

void 
TankView::AttachedToWindow(void)
{
	SetViewColor(B_TRANSPARENT_32_BIT);
	SetDrawingMode(B_OP_OVER);

	//Set up our bitmaps to indicate network connections
	redDot.SetBits(kRedDotBits, sizeof(kRedDotBits), 0, kRedDotColorSpace);
	greenDot.SetBits(kGreenDotBits, sizeof(kGreenDotBits), 0, kGreenDotColorSpace);
	fishMap.SetBits(kFishBits, sizeof(kFishBits), 0, kFishDataColorSpace);

	FishNet = new FishPortal(BMessenger(this));
	Window()->AddHandler(FishNet);
}

/****************/

void 
TankView::Draw(BRect)
{
	int index;
	
	SetHighColor(50,50,200,0);
	FillRect(Bounds());

	Fish *thisFish;
	index = fishList.CountItems();
	while (index > 0) {
		thisFish = (Fish *) fishList.ItemAt(index-1); /* anyone counting parens? */
		DrawBitmapAsync( &fishMap, BPoint(  ( Bounds().Width()-fishMap.Bounds().Width()  )  * ( (double) thisFish->x / 255.0 ),
		                                    ( Bounds().Height()-fishMap.Bounds().Height())  *( (double) thisFish->y/ 255.0  ) ) );		
		index--;
	}

	/* draw our network indicator dots */
	if (tank_left) {
		DrawBitmapAsync( &greenDot, BPoint(Bounds().left + 10, (Bounds().Height() / 2) - greenDot.Bounds().Height() / 2 ) );
	} else {
		DrawBitmapAsync( &redDot, BPoint(Bounds().left + 10, (Bounds().Height() / 2) - redDot.Bounds().Height() / 2 ) );
	}
	
	if (tank_right) {
		DrawBitmapAsync( &greenDot, BPoint(Bounds().right - greenDot.Bounds().Width()- 10, ( Bounds().Height() / 2) - greenDot.Bounds().Height() / 2 ) );
	} else {
		DrawBitmapAsync( &redDot, BPoint(Bounds().right - redDot.Bounds().Width()- 10, ( Bounds().Height() / 2) - redDot.Bounds().Height() / 2 ) );
	}
	
	if (tank_up) {
		DrawBitmapAsync( &greenDot, BPoint((Bounds().Width() / 2) - ( greenDot.Bounds().Width() / 2 ), Bounds().top + 10) );
	} else {
		DrawBitmapAsync( &redDot, BPoint((Bounds().Width() / 2) - ( redDot.Bounds().Width() / 2 ), Bounds().top + 10) );
	}
	if (tank_down) {
		DrawBitmapAsync( &greenDot, BPoint((Bounds().Width() / 2) - ( greenDot.Bounds().Width() / 2 ), Bounds().bottom - greenDot.Bounds().Height() - 10) );
	} else {
		DrawBitmapAsync( &redDot, BPoint((Bounds().Width() / 2) - ( redDot.Bounds().Width() / 2 ), Bounds().bottom - redDot.Bounds().Height() - 10) );
	}
	
	Sync();
}

/****************/


void 
TankView::AddFish(Fish *newFish)
{
	fishList.AddItem( (void *) newFish );
}
/*********************/

void 
TankView::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case F_PULSE : {
			Update();
			Draw(Bounds());
		break;		
		}
		case TANK_STATUS : {
			msg->FindBool("tank_left", &tank_left);
			msg->FindBool("tank_right", &tank_right);
			msg->FindBool("tank_up", &tank_up);
			msg->FindBool("tank_down", &tank_down);
			break;
		}
		case SEND_FISH : {
			ssize_t	size;
			dfp_pkt_transfer *dfp;
			msg->FindData("fish", DFP_TRANSFER_TYPE, (const void **) &dfp, &size);
			/* need to give it a start away from the wall, or it will bounce */
			if(dfp->fish.x == 0)   dfp->fish.x = 1   + dfp->fish.dx;
			if(dfp->fish.y == 0)   dfp->fish.y = 1   + dfp->fish.dy;
			if(dfp->fish.x == 255) dfp->fish.x = 254 + dfp->fish.dx;
			if(dfp->fish.y == 255) dfp->fish.y = 254 + dfp->fish.dy;
			fishList.AddItem(new Fish(dfp));
			break;
		}
		case PORTAL_INIT : {
			Window()->PostMessage(msg, FishNet);
			break;
			}
		default: { //Always handle the defaults
			 BView::MessageReceived(msg);
		}
	} // end of switch							
}

/*********************/
void 
TankView::Update(void)
{
	int 	i;
	Fish 	*thisFish;
	BRect	border;

/* handle the fish movement */
	border = Bounds();

	i = fishList.CountItems();
	while ( (thisFish = (Fish *) fishList.ItemAt(i-1)) != NULL) {
		bool send = false;
		if ( thisFish->x  + thisFish->dx < 0    ) {thisFish->x = 0; send = true;}
		if ( thisFish->y  + thisFish->dy < 0    ) {thisFish->y = 0; send = true;}
		if ( thisFish->x  + thisFish->dx > 255  ) {thisFish->x = 255; send = true;}
		if ( thisFish->y  + thisFish->dy > 255  ) {thisFish->y = 255; send = true;}
		if(send) { 
			dfp_pkt_transfer	dfp;
			BMessage	msg(NET_FISH); //destined for the net
			thisFish->Wrap(&dfp);
			msg.AddData("fish", DFP_TRANSFER_TYPE, &dfp, sizeof(dfp));
			Window()->PostMessage(&msg, FishNet);
			fishList.RemoveItem(thisFish);
		}
		/* moving fish */
		thisFish->x = thisFish->x + (thisFish->dx );
		thisFish->y = thisFish->y + (thisFish->dy );
		i--;
	}
}

/*********************/
/*********************/

TankWindow::TankWindow(uint8 tank_x, uint8 tank_y):BWindow(BRect(100, 100, 740, 580), "Tank", B_TITLED_WINDOW, 0)
{

	// Add the tank View to the window
	tankView = new TankView(Bounds(), "Main Tank View");
	AddChild(tankView);
	pulseRunner = new BMessageRunner(BMessenger(tankView), new BMessage( F_PULSE ), 100000);

	/* testing stuff */
	tankView->AddFish( new Fish(10, 10,  -2,  1, "Fish #1",3,4, 1600));
	tankView->AddFish( new Fish(10, 20, -3,  2, "Fish #2",3,4, 1600));
	tankView->AddFish( new Fish(30, 40,  3, -3, "Fish #3",3,4, 1600));
	tankView->AddFish( new Fish(50, 60, -2, -4, "Fish #4",3,4, 1600));

	BMessage InitMessage(PORTAL_INIT);
	InitMessage.AddInt8("tank_x", tank_x);
	InitMessage.AddInt8("tank_y", tank_y);
	PostMessage(&InitMessage, tankView);
}

/*********************/

bool
TankWindow::QuitRequested()
{
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}


