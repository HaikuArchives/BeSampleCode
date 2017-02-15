/*
 File:	main.h

 Class declarations for the BeBox bouncing ball demo.

	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef MAIN_H
#define MAIN_H

#include <Application.h>
#include <Locker.h>
#include <Messenger.h>
#include <Window.h>
#include <View.h>
#include <Bitmap.h>

enum {
	/*
	 enum's for the various messages sent within and
	 between instances of the BeBounce application.
	*/

	BB_HELLO		= 'bhel',
	BB_GOODBYE		= 'bgby',
	BB_WINDOW_MOVED	= 'bwmv',
	BB_BALL			= 'ball',
	BB_DRAW			= 'draw',
	BB_HIT_HOLE		= 'hole',
	BB_TICK			= 'tick',
	BB_STOP			= 'stop'
};

const char *MY_SIG = "application/x-vnd.Be-bbll";
const float	RADIUS = 10.0;

class TWindow;
class TBitmapView;
class TBall;

/*--------------------------------------------------------*/

class TBounceApp : public BApplication {
public:
					TBounceApp();
virtual				~TBounceApp();

virtual	void		MessageReceived(BMessage *msg);
		void		InitPartner(team_id team);
		void		RemovePartner();
		bool		SendToPartner(BMessage *msg);
		bool		SendPosition(BRect rect);

private:
		TWindow		*fWindow;
		BMessenger	*fPartner;
};

TBounceApp	*my_app;

/*--------------------------------------------------------*/

class TWindow : public BWindow {
public:
					TWindow(BRect frame, const char *title,
							bool with_ball);
virtual				~TWindow();

virtual	void		MessageReceived(BMessage *msg);
virtual bool		QuitRequested();
virtual void		FrameMoved(BPoint new_position);
		void		PartnerMoved(BRect pos);
		void		Update(BRect rect);
		void		Update();
		void		DrawOffScreen(BView *v);
		void		WindowsMoved(BRect window_frame,
								BRect partner_frame);
		void		PartnerGone();
		void		AddBall();
		void		AddBall(float rel_location,
							BPoint speed);

private:

		TBitmapView	*fMainView;
		BRect		fMyFrame;
		BRect		fPartnerFrame;
		float		fGapStart;
		float		fGapEnd;
		BBitmap		*fBitmap;
		BView		*fOffView;
		TBall		*fBall;
};

/*--------------------------------------------------------*/

class TBitmapView : public BView {
public:
					TBitmapView(BRect frame, BBitmap *bitmap);

virtual	void		Draw(BRect updateRect);

private:
		BBitmap		*fBitmap;
};

/*--------------------------------------------------------*/

class TBall : public BLooper {
public:
					TBall(TWindow *window, BRect bounds,
						BPoint center, float radius,
						BPoint speed);

		void		Draw(BView *view);
virtual	void		MessageReceived(BMessage *msg);

		void		SetGap(float start, float end);
		void		SetEnabled(bool state);
		bool		IsEnabled();
		BRect		Bounds();

private:

		void		NextPosition(bool *hit, float *angle);
		void		Tick();

		TWindow		*fWindow;
		BPoint		fCenter;
		float		fRadius;
		BPoint		fSpeed;
		float		fSleep;
		BRect		fBoundary;
		float		fPercentRemaining;
		float		fLength;
		float		fGapStart;
		float		fGapEnd;
		bool		fEnabled;
};

inline void TBall::SetEnabled(bool state)
	{ fEnabled = state; }

inline bool TBall::IsEnabled()
	{ return fEnabled; }

#endif
