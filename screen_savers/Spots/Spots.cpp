#include "Spots.h"
#include <StringView.h>
#include <stdlib.h>

// MAIN INSTANTIATION FUNCTION
extern "C" _EXPORT BScreenSaver *instantiate_screen_saver(BMessage *message, image_id image)
{
	return new Spots(message, image);
}

Spots::Spots(BMessage *message, image_id image)
 : BScreenSaver(message, image)
{
}

void Spots::StartConfig(BView *view)
{
	view->AddChild(new BStringView(BRect(10, 10, 200, 35), B_EMPTY_STRING, "Spots, holes in the desktop"));
}

status_t Spots::StartSaver(BView *, bool /* preview */)
{
	SetTickSize(50000);
	return B_OK;
}

void Spots::Draw(BView *view, int32)
{
	// display spot
	BRect r;
	r.top = (rand() % view->Bounds().IntegerHeight()) - 15;
	r.left = (rand() % view->Bounds().IntegerWidth()) - 15;
	r.bottom = r.top + 30;
	r.right = r.left + 30;
	view->FillRoundRect(r, 15, 15, B_SOLID_LOW);
}
