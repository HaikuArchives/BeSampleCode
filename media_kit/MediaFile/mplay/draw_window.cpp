/*
	
  The core code of Draw.

  Dominic Giampaolo
  dbg@be.com
	
*/
#include "draw_window.h"
#include "draw.h"
#include "MediaView.h"
#include <Path.h>


DrawWindow::DrawWindow(
	BRect		frame, 
	const char	*path)
		: BWindow(frame, "Draw", B_TITLED_WINDOW, 0)
{
	SetTitle(BPath(path).Leaf());

	MediaView *drawview = new MediaView(Bounds(), "drawview", B_FOLLOW_ALL);
	AddChild(drawview);

	//drawview->SetColorSpace(B_CMAP8);
	drawview->SetMediaSource(path);
	
	float w = 0.0;
	float h = 0.0;
	drawview->GetPreferredSize(&w, &h);	

	SetSizeLimits(80.0, 30000.0, (drawview->HasVideoTrack()) ? 80.0 : h, (drawview->HasVideoTrack()) ? 30000.0 : h);
	SetZoomLimits(w, h);
	ResizeTo(w, h);

	drawview->MakeFocus(true);
	drawview->Control(MEDIA_PLAY);
}


DrawWindow::~DrawWindow()
{
	be_app->PostMessage(msg_WindowClosed);
}
