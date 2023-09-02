/*--------------------------------------------------------*/
/*
 File:	main.cpp

 main implementation file for the BeBox bouncing ball demo

	Copyright 1996-1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.


 Changes since the MacTEch article was published (january, 1996)

 1/2/96 - Add flow control for messaging. Could get into situation
 where the window was getting too many BB_DRAW or B_WINDOW_MOVED
 events. The messages were coming in faster then they could be
 handled. If this situation arises some messages are now dropped.
*/
/*--------------------------------------------------------*/

#include <Debug.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "main.h"
#include "Crunch.h"
#include <List.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <Roster.h>
#include <MessageQueue.h>


/*--------------------------------------------------------*/

int main(int argc, char* argv[])
{
	/*
	 To randomize the movement of the bouncing ball set
	 the random seed to the system time
	*/
	srand((long) system_time());

	// make the new application object and start it running
	my_app = new TBounceApp();
	my_app->Run();

	// application is finished so cleanup and return
	delete my_app;
	return 0;
}


/*--------------------------------------------------------*/

/*
 The Application object for the BeBounce application.
 Passing my signature to the system
*/

TBounceApp::TBounceApp()
	: BApplication(MY_SIG)
{
	BList list;
	BRect wpos(120, 40, 321, 241);

	fPartner = NULL;

	/*
	 This version of BeBounce only supports 2 instances of
	 the app running at the same time. So if there are
	 already 2 instances running force a QUIT.

	 In the BeOS the 'roster' object maintains system wide
	 information about all applications. We ask it
	 for a list of all the running applications with
	 the same signature as mine.
	*/
	be_roster->GetAppList(MY_SIG, &list);
	long app_count = list.CountItems();
	if (app_count > 2) {
		// only support 2 apps at a time
		PostMessage(B_QUIT_REQUESTED);
		return;
	}
	
	if (app_count == 1) {
		// The first instance of BeBounce will have a ball.
		fWindow = new TWindow(wpos, "BeBounce 1", TRUE);
	} else {
		fWindow = new TWindow(wpos, "BeBounce 2", FALSE);

		/*
		 There are 2 instances of BeBounce. One is me(!)
		 and one is the other guy. Determine which is
		 which by comparing team_id's in the list 
		 returned by GetAppList() against my thread.
		*/
		team_id team = (team_id) list.ItemAt(0);
		if (team == Team())
			team = (team_id) list.ItemAt(1);
		
		InitPartner(team);		 // tid is our partner!

		/*
		 Send the introductory message along with my thread
		 id and location in screen corrdinates.
		*/
		BMessage msg(BB_HELLO);
		msg.AddInt32("team", Team());
		msg.AddRect("rect", wpos);
		SendToPartner(&msg);
	}

	fWindow->AddShortcut('K', B_COMMAND_KEY, new BMessage(BB_STOP));

	fWindow->Show();
}

/*--------------------------------------------------------*/

TBounceApp::~TBounceApp()
{
	fWindow = NULL;
	RemovePartner();
}

/*--------------------------------------------------------*/

void TBounceApp::InitPartner(team_id team)
{
	// race condition is 2 apps both send BB_HELLO message
	if (fPartner)
		return;

	// Establish a 'messenger' as the link to our partner.
	fPartner = new BMessenger(MY_SIG, team);
	if (!fPartner->IsValid()) {
		delete fPartner;
		fPartner = NULL;
	} 
}

/*--------------------------------------------------------*/

void TBounceApp::RemovePartner()
{
	// Delete the messenger and tell the window.
	if (fPartner) {
		delete fPartner;
		fPartner = NULL;
	}
	if (fWindow && fWindow->Lock()) {
		fWindow->PartnerGone();
		fWindow->Unlock();
	}
}

/*--------------------------------------------------------*/

bool TBounceApp::SendToPartner(BMessage *msg)
{
	/*
	 Send the message to our partner (if we have one).
	 If the send fails then we must assume that the partner
	 is dead so it gets removed.
	*/
	if (!fPartner) {
		return FALSE;
	}

	long error = fPartner->SendMessage(msg);
	if (error != B_NO_ERROR) {
		RemovePartner();
		return FALSE;
	}
	return TRUE;
}

/*--------------------------------------------------------*/

bool TBounceApp::SendPosition(BRect rect)
{
	/*
	 Give our partner that the current window position
	 (in screen coordinates).
	*/
	if (fPartner) {
		BMessage	msg(BB_WINDOW_MOVED);
		msg.AddRect("rect", rect);
		return SendToPartner(&msg);
	}

	return FALSE;
}

/*--------------------------------------------------------*/

void TBounceApp::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case BB_HELLO:
			if (fWindow->Lock()) {
				/*
				 A new instance of BeBounce was just launched
				 and sent us the introductory message.
				*/
				InitPartner(msg->FindInt32("team"));

				// Tell our new partner our current location.
				BRect pos = fWindow->Frame();
				SendPosition(pos);

				// Initialize our partner's current location.
				pos = msg->FindRect("rect");
				fWindow->PartnerMoved(pos);
				fWindow->Unlock();
			}
			break;
		case BB_GOODBYE:
			if (fWindow->Lock()) {
				// Our partner is quitting.
				RemovePartner();
				if (msg->HasBool("ball")) {
					fWindow->AddBall();
				}
				fWindow->Unlock();
			}
			break;
		case BB_WINDOW_MOVED:
			// Our partner is informing us that it moved.
			if (fWindow->Lock()) {
				BRect pos = msg->FindRect("rect");
				fWindow->PartnerMoved(pos);
				fWindow->Unlock();
			}
			break;
		case BB_BALL:
			if (fWindow->Lock()) {
				// Our partner just passed us the ball.
				BPoint	speed = msg->FindPoint("speed");
				float	rel_loc = msg->FindFloat("rel_loc");
				fWindow->AddBall(rel_loc, speed);
				fWindow->Unlock();
			}
			break;
	}
}

/*--------------------------------------------------------*/
/*--------------------------------------------------------*/
/*--------------------------------------------------------*/

TWindow::TWindow(BRect frame, const char *title, bool ball)
	: BWindow(frame, title, B_TITLED_WINDOW, B_NOT_RESIZABLE | B_NOT_ZOOMABLE)
{
	Lock();

	BRect b = frame;

	// shift be to window coordinate system
	b.OffsetTo(B_ORIGIN);

	fMyFrame = frame;
	fGapStart = -1.0;
	fGapEnd = -1.0;
	fBall = NULL;

	if (ball)
		AddBall();

	/*
	 The drawing will take place in the view fOffView that
	 will be added to the offscreen bitmap fBitmap. In 
	 this way we'll do the drawing offscreen and then just
	 blit the result to the screen.
	*/
	fBitmap = new BBitmap(b, B_COLOR_8_BIT, TRUE);
	fOffView = new BView(b, "", 0, B_WILL_DRAW);

	fBitmap->Lock();
	fBitmap->AddChild(fOffView);
	DrawOffScreen(fOffView);	// draw the initial setup
	fBitmap->Unlock();

	/*
	 This view will be added to the visible window. It's
	 only role is to blit the offscreen bitmap to the
	 visible window.
	*/
	fMainView = new TBitmapView(b, fBitmap);
	AddChild(fMainView);

	Unlock();
}

/*--------------------------------------------------------*/

TWindow::~TWindow()
{
	delete fBitmap;
}

/*--------------------------------------------------------*/

void TWindow::FrameMoved(BPoint new_pos)
{
	/*
	 Override of BWindow::FrameMoved() so that as the
	 window is moved around the screen we can inform our
	 partner of our new location.
	*/

	/*
	 Cheezy flow control. Eat up all remaining WINDOW_MOVED messages.
	*/

	BMessage *msg;
	while ((msg = MessageQueue()->FindMessage(B_WINDOW_MOVED, 0)) != NULL) {
		MessageQueue()->RemoveMessage(msg);
		delete msg;
	}

	fMyFrame = Frame();
	if (my_app->SendPosition(fMyFrame))
		WindowsMoved(fMyFrame, fPartnerFrame);
}


/*--------------------------------------------------------*/

void TWindow::AddBall()
{
	/*
	 Simply add a ball in some default place with some
	 default speed.
	*/
	if (fBall)
		return;
	BRect boundary = fMyFrame;
	boundary.OffsetTo(B_ORIGIN);
	boundary.InsetBy(1,1);
	fBall = new TBall(this, boundary, BPoint(40,30),
		RADIUS, BPoint(8,19));
	fBall->SetGap(fGapStart, fGapEnd);
}

/*--------------------------------------------------------*/

void TWindow::AddBall(float rel_loc, BPoint speed)
{
	/*
	 We're adding a ball that just 'jumped' over from
	 our partner. We're given the info needed to
	 determine where to place the ball.
	*/
	if (fBall)
		return;

	float	radius = RADIUS;
	BRect	b = fMyFrame;
	b.OffsetTo(B_ORIGIN);
	b.InsetBy(radius, radius);
	BPoint	position = CalcInitialPosition(rel_loc, fGapStart, fGapEnd, b);

	b = fMyFrame;
	b.OffsetTo(B_ORIGIN);

	fBall = new TBall(this, b, position, radius, speed);
	fBall->SetGap(fGapStart, fGapEnd);
}

/*--------------------------------------------------------*/

void TWindow::PartnerMoved(BRect partner_frame)
{
	// our partner moved so we must update the 'gap'
	fPartnerFrame = partner_frame;
	WindowsMoved(fMyFrame, partner_frame);
}

/*--------------------------------------------------------*/

void TWindow::PartnerGone()
{
	// partner is gone so remove the gap!
	fGapStart = fGapEnd = -1.0;
	if (fBall)
		fBall->SetGap(fGapStart, fGapEnd);
	Update();
}

/*--------------------------------------------------------*/

void TWindow::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case BB_DRAW:
			{
			/*
			 Flow control.
			 Look for other pending BB_DRAW events. "eat" those events
			 and force a redraw emcompassing both the first updateRect
			 (this will have the old ball position) and the last updateRect
			 (this will have the new ball position).
			 Not the most optimal solution, but it will do.
			*/
			if (fBall && !fBall->Lock()) {
				break;
			}
			BRect		updateRect = msg->FindRect("update");
			BRect		updateRect2;
			bool		multi = FALSE;
			BMessage	*next;
			while ((next = MessageQueue()->FindMessage(BB_DRAW, 0)) != NULL) {
				multi = TRUE;
				updateRect2 = next->FindRect("update");
				MessageQueue()->RemoveMessage(next);
				delete next;
			}

			// The ball is telling the window to redraw
			if (multi)
				Update(updateRect | updateRect2);
			else
				Update(updateRect);
			if (fBall)
				fBall->Unlock();
			break;
			}
		case BB_HIT_HOLE:
			{
			/*
			 The ball is telling us that it just hit the
			 hole. So it should get sent to the partner.
			*/
			BMessage	m(BB_BALL);
			m.AddPoint("speed", msg->FindPoint("speed"));
			m.AddFloat("rel_loc", msg->FindFloat("rel_loc"));

			// send the 'ball' to our partner
			my_app->SendToPartner(&m);

			// delete the ball from this window
			if (fBall->Lock())
				fBall->Quit();
			fBall = NULL;

			Update();
			break;
			}
		case BB_STOP:
			{
			// Stop the ball in mid-flight
			if (fBall) {
				fBall->SetEnabled(!fBall->IsEnabled());
				if (fBall->IsEnabled())
					fBall->PostMessage(BB_TICK);
			}
			break;
			}
		default:
			// MessageReceived(msg);
			break;
	}
}

/*--------------------------------------------------------*/

bool TWindow::QuitRequested()
{
	/*
	 The window was asked to quit/close. Send a message
	 to our partner, giving him the ball if we've
	 currently got it.
	*/
	BMessage	m(BB_GOODBYE);
	if (fBall) {
		if (fBall->Lock())
			fBall->Quit();
		fBall = NULL;
		m.AddBool("ball", TRUE);
	}
	my_app->SendToPartner(&m);

	// Tell the app to go ahead and quit.
	my_app->PostMessage(B_QUIT_REQUESTED);
	return TRUE;
}

/*--------------------------------------------------------*/

void TWindow::WindowsMoved(BRect window_frame, BRect partner_frame)
{
	// Either we moved or the partner. So recalculate the gap.
	CalcGapAngles(window_frame, partner_frame, &fGapStart, &fGapEnd);
	if (fBall)
		fBall->SetGap(fGapStart, fGapEnd);
	Update();
}

/*--------------------------------------------------------*/

void TWindow::Update(BRect rect)
{
	// redraw window, only blitting the given rect.
	DrawOffScreen(fOffView);
	fMainView->Draw(rect);
}

/*--------------------------------------------------------*/

void TWindow::Update()
{
	// redraw window, blitting the entire window
	DrawOffScreen(fOffView);
	fMainView->Draw(fMainView->Bounds());
}

/*--------------------------------------------------------*/

void TWindow::DrawOffScreen(BView *v)
{
	/*
	 Do the actual drawing of the 'room'. The walls will get
	 drawn (with a gap if there is one) and the ball will
	 be placed in the proper position.
	*/
	fBitmap->Lock();

	BRect b = v->Bounds();
	
	v->SetPenSize(2.0);
	v->SetHighColor(100,100,0);
	v->FillRect(b, B_SOLID_LOW);

//+	b.bottom -= 1.0;
//+	b.right -= 1.0;
//+	b.InsetBy(1,1);
	b.top += 1;
	b.left += 1;

	// draw the appropriate border - with or without a 'gap'
	if (fGapStart == -1.0)
		v->StrokeRect(b);
	else
		DrawFrame(v, b, fGapStart, fGapEnd);

	if (fBall)
		fBall->Draw(v);

	v->Sync();			// flush all drawing to the bitmap

	fBitmap->Unlock();
}

/*--------------------------------------------------------*/
/*--------------------------------------------------------*/
/*--------------------------------------------------------*/

TBitmapView::TBitmapView(BRect frame, BBitmap *bitmap)
	: BView(frame, "", B_FOLLOW_NONE, B_WILL_DRAW)
{
	/*
	 The only job of this view is to blit the offscreen
	 bits into the onscreen view.
	*/
	fBitmap = bitmap;
}

/*--------------------------------------------------------*/

void TBitmapView::Draw(BRect update)
{
	DrawBitmap(fBitmap, update, update);
}

/*--------------------------------------------------------*/
/*--------------------------------------------------------*/
/*--------------------------------------------------------*/

TBall::TBall(TWindow *window, BRect bounds, BPoint center,
	float radius, BPoint speed)
{
	/*
	 The ball object actually runs in its own thread so
	 that it is independent of any window. It sends the
	 window messages as needed.
	*/
	fWindow = window;
	fCenter = center;
	fBoundary = bounds;
	fRadius = radius;
	fSpeed = speed;
	fLength = sqrt((fSpeed.x*fSpeed.x) + (fSpeed.y*fSpeed.y));
	fSleep = 40;
	fPercentRemaining = 1.0;
	fGapStart = fGapEnd = -1.0;
	fEnabled = TRUE;

	Run();
	// post initial message to get things rolling
	PostMessage(BB_TICK);
}

/*--------------------------------------------------------*/

void TBall::Draw(BView *view)
{
	// The balls draws itself in the given view
	Lock();
	rgb_color	c = view->HighColor();

	view->SetPenSize(1.0);
	view->SetHighColor(150, 30, 30);
	view->FillArc(fCenter, fRadius, fRadius, 0, 360);
	view->SetHighColor(c);
	Unlock();
}

/*--------------------------------------------------------*/

void TBall::Tick()
{
	/*
	 This method is repeatedly call to simulate the ball's
	 motion. The ball will wake up every fSnooze millisecs
	 and update its position.

	 So basically this is where the hit-testing and logic
	 lives for bouncing the ball around the window and
	 deciding when the ball hits the 'gap' and gets
	 teleported over to the partner application.
	*/
	float	angle;
	bool	hit_hole = FALSE;
	BRect	updateRect;

	if ((rand() % 3000) == 0) {
		// add a little random bahavior to the ball.
		long r = (int32)((rand() % 3) - 1.0);
		fSpeed.x += r;
		fSpeed.y -= r;
		fLength = sqrt((fSpeed.x*fSpeed.x) +
						(fSpeed.y*fSpeed.y));
	}

	/*
	 To minimize blitting we'll calculate the rect that
	 encloses the before & after locations of the ball
	 and only redraw that portion of the screen. That's
	 what 'updateRect' will do.
	*/
	updateRect = Bounds();

	// This will move the ball to its new location.
	NextPosition(&hit_hole, &angle);

	updateRect = updateRect | Bounds();
	updateRect.InsetBy(-1, -1);

	// inform the window to redraw the window.
	BMessage	msg(BB_DRAW);
	msg.AddRect("update", updateRect);
	fWindow->PostMessage(&msg);


	if (hit_hole) {
		/*
		 The 'gap' was hit. So we package up the info
		 like speed and relative location, which gives
		 our partner enough information to have the
		 ball appear in the correct place.
		*/
		float rel_loc = CalcGapPosition(angle, fGapStart,
			fGapEnd);

		BMessage msg(BB_HIT_HOLE);
		msg.AddPoint("speed", fSpeed);
		msg.AddFloat("rel_loc", rel_loc);
		fWindow->PostMessage(&msg);
		fEnabled = FALSE;
	}
}

/*--------------------------------------------------------*/

void TBall::MessageReceived(BMessage *msg)
{
	if (!fEnabled)
		return;

	switch (msg->what) {
		case BB_TICK:
			{
			Tick();

			/*
			 Adjust the 'snooze' duration to keep the
			 animation flowing smoothly.
			*/
			float sleep_for = fPercentRemaining * fSleep;

			// ??? yuck! Need a better mechanism for waking up self
			Unlock();
			snooze((bigtime_t) (sleep_for * 1000));
			Lock();
			PostMessage(BB_TICK);

			break;
			}
	}
}

/*--------------------------------------------------------*/

BRect TBall::Bounds()
{
	return BRect(fCenter.x - fRadius, fCenter.y - fRadius,
				fCenter.x + fRadius, fCenter.y + fRadius);
}

/*--------------------------------------------------------*/

void TBall::SetGap(float start, float end)
{
	Lock();
	fGapStart = start;
	fGapEnd = end;
	Unlock();
}

/*--------------------------------------------------------*/

void TBall::NextPosition(bool *hit_hole, float *angle)
{
	/*
	 Move the ball to it's next position.
	*/
	BPoint	move_by(fSpeed.x * fPercentRemaining,
					fSpeed.y * fPercentRemaining);
	BPoint	p1 = fCenter;
	BPoint	p2 = fCenter + move_by;
	BRect	b = fBoundary;

	b.InsetBy(BPoint(fRadius, fRadius));

	if (!b.Contains(p2)) {
		/*
		 The ball is now outside the boundary - so it either
		 hit a wall of it is going through the 'gap'.
		 Calculate the intersection point and the ball's
		 new speed (i.e. direction after the bounce).
		*/

//		PRINT(("\nCalcIntersection\n"));
//		PRINT_OBJECT(b); PRINT_OBJECT(p1); PRINT_OBJECT(p2); PRINT_OBJECT(fSpeed);

		BPoint new_speed = fSpeed;
		BPoint hit = CalcIntersection(p1, p2, b, &new_speed);

		// check to see if the ball is in the 'gap'
		if (fGapStart != -1.0) {
			BPoint	center = rect_center(fBoundary);
			long	unused;

			*angle = CalcAngle(center, hit, &unused);
			if (fGapEnd > fGapStart)
				*hit_hole = ((fGapStart < *angle) &&
							(*angle < fGapEnd));
			else
				*hit_hole = (*angle > fGapStart) ||
							(*angle < fGapEnd);
		}

		p2 = hit;

		if (!*hit_hole) {
			/*
			 Didn't hit the gap. The ball's new position
			 will be at the point of impact with the wall.
			 We do this so that the animation will show
			 the ball actually hitting the wall. Because
			 of this will need to adjust the 'snooze'
			 period until the next animation to keep to
			 speed of the ball relatively constant.
			*/
			fSpeed = new_speed;
			float partial = segment_length(p1, hit);
			fPercentRemaining -= (partial / fLength);
		}
	} else {
		// Didn't hit the wall.
		fPercentRemaining = 1.0;

		/*
		 However, if the ball stopped exactly on an edge,
		 reverse it's direction for next time.
		*/
		if ((p2.y == b.top) || (p2.y == b.bottom))
			fSpeed.y *= -1.0;
		if ((p2.x == b.left) || (p2.x == b.right))
			fSpeed.x *= -1.0;
	}

	fCenter = p2;	// finally update the ball's position

#if DEBUG
	if (!b.Contains(fCenter)) {
		PRINT(("fCenter is outside of boundary!\n"));
		PRINT_OBJECT(b);
		PRINT_OBJECT(fCenter);
		ASSERT(0);
	}
#endif
}
