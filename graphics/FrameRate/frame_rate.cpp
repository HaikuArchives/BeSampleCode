/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include <Application.h>
#include <Window.h>
#include <View.h>
#include <Screen.h>
#include <stdio.h>
#include <OS.h>

int32 rate_calc_thread(void * _rw);

class RateView : public BView {
public:
	RateView(BRect frame);
virtual void Draw(BRect r);
private:
	float rate, width, y;
	friend int32 rate_calc_thread(void * _rw);
};

RateView::RateView(BRect frame)
	:	BView(frame, "rate view", B_FOLLOW_ALL, B_WILL_DRAW|B_FULL_UPDATE_ON_RESIZE)
{
	float height = frame.Height();
	y = height * 0.2;
	width = frame.Width();
	SetFont(be_fixed_font);
	SetFontSize(height * 0.8);
	SetDrawingMode(B_OP_COPY);
	SetLowColor(255,255,255);
	SetViewColor(255,255,255);
	SetHighColor(0,0,0);
	rate = 0.0;
}

void 
RateView::Draw(BRect /* r */)
{
	float x;
	BRect bounds = Bounds();
	char buffer[16];
	sprintf(buffer, "%.1f", rate);
	x = (width - StringWidth(buffer)) / 2;
	DrawString(buffer, bounds.LeftBottom() + BPoint(x,-y));
}


class RateWindow : public BWindow {
public:
	RateWindow(BRect r, uint32 prior);
virtual bool QuitRequested(void);
private:
	static int32 window_count;
	RateView *rv;
	thread_id tid;
	int run_thread;
	friend int32 rate_calc_thread(void * _rw);
};
int32 RateWindow::window_count = 0;


RateWindow::RateWindow(BRect r, uint32 prior)
	:	BWindow(r, "Frame Rate", B_TITLED_WINDOW, B_NOT_RESIZABLE|B_NOT_ZOOMABLE)
{
	r.OffsetTo(0,0);
	rv = new RateView(r);
	AddChild(rv);
	tid = spawn_thread(rate_calc_thread, "rate calc", prior, this);
	if (tid > 0) {
		run_thread = 1;
		resume_thread(tid);
	}
	atomic_add(&window_count, 1);
	Show();
}

bool 
RateWindow::QuitRequested(void)
{
	run_thread = 0;
	status_t dont_care;
	wait_for_thread(tid, &dont_care);
	if (atomic_add(&window_count, -1) == 1)
		be_app->PostMessage(B_QUIT_REQUESTED, be_app);
	return BWindow::QuitRequested();
}


class RateApp : public BApplication {
public:
	RateApp(void);
};

RateApp::RateApp(void)
	:	BApplication("application/x-vnd.Be.FrameRate")
{
	BRect r(0,0, 123,40);
	r.OffsetBy(100,100);
	new RateWindow(r, B_REAL_TIME_DISPLAY_PRIORITY);
}


// One second worth of microseconds
#define ONE_SECOND 1000000
// SLOTS needs to be a power of 2.  256 is just over 2
// seconds worth of measurements at 120Hz and just over 4
// seconds worht of measuerments at 60Hz (a common range of
// vertical refresh rates for displays, coincidently :-)
#define SLOTS 256

int32 rate_calc_thread(void * _rw) {
	RateWindow *rw = (RateWindow *)_rw;
	BScreen bs(rw);
	RateView *rv = rw->rv;
	bigtime_t last_time, tmp_diff;
	bigtime_t this_time, time_diff = 0;
	bigtime_t diffs[SLOTS];
	status_t result;
	int i;

	// if the driver doesn't support retrace events
	// give up now
	result = bs.WaitForRetrace();
	if (result != B_OK) return result;
	last_time = system_time();
	bs.WaitForRetrace();
	this_time = system_time();
	tmp_diff = this_time - last_time;
	// Initialize diffs array so it takes less time to
	// converge.
	for (i = 0; i < SLOTS; i++) {
		diffs[i] = tmp_diff;
		time_diff += tmp_diff;
	}
	i = 1;
	// sync up again before entering the timing loop
	bs.WaitForRetrace();
	last_time = system_time();
	while (rw->run_thread) {
		result = bs.WaitForRetrace();
		this_time = system_time();
		if (result == B_OK) {
			// sutract out the oldest measurement and
			// replace it with the one we just took
			time_diff -= diffs[i];
			diffs[i] = this_time - last_time;
			time_diff += diffs[i];
			// wrap around at the end
			i++; i &= (SLOTS-1);
			last_time = this_time;
			// to reduce jitter, only update the display
			// every 8th frame
			if ((i & 0x7)== 0) {
				rv->rate = ((double)ONE_SECOND * SLOTS)
				         / (double)time_diff;
				// and only if it's not alread busy
				if (rw->LockWithTimeout(0) == B_OK) {
					rv->Invalidate();
					rw->Unlock();
				}
			}
		} else return B_ERROR;
	}
	return B_OK;
}

int main(int /* argc */, char ** /* argv */) {
	RateApp r;
	r.Run();
	return 0;
}
