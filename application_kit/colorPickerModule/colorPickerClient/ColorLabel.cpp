/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include <Bitmap.h>
#include <Roster.h>
#include <Window.h>

#include "ColorLabel.h"
#include "../simpleColorPicker/Protocol.h"

// add some handy calls first

static bool
CompareColors(const rgb_color a, const rgb_color b)
{
	return a.red == b.red
		&& a.green == b.green
		&& a.blue == b.blue
		&& a.alpha == b.alpha;
}

inline bool 
operator==(const rgb_color &a, const rgb_color &b)
{
	return CompareColors(a, b);
}

inline bool 
operator!=(const rgb_color &a, const rgb_color &b)
{
	return !CompareColors(a, b);
}

inline uchar
ShiftComponent(uchar component, float percent)
{
	// change the color by <percent>, make sure we aren't rounding
	// off significant bits
	if (percent >= 1)
		return (uchar)(component * (2 - percent));
	else
		return (uchar)(255 - percent * (255 - component));
}

rgb_color
ShiftColor(rgb_color color, float percent)
{
	rgb_color result = {
		ShiftComponent(color.red, percent),
		ShiftComponent(color.green, percent),
		ShiftComponent(color.blue, percent),
		255
	};
	
	return result;
}

ColorLabel::ColorLabel(BRect frame, const char *name, const char *label,
	rgb_color initialValue, BMessage *message, uint32 resizeMask, uint32 flags)
	:	BControl(frame, name, label, message, resizeMask, flags),
		colorValue(initialValue),
		divider(frame.Width() / 2),
		proxy(this, kColorPickerType /*, "application/x-vnd.Be-STEE"*/)
{
	SetFont(be_plain_font);
}

rgb_color 
ColorLabel::ValueAsColor() const
{
	return colorValue;
}

void 
ColorLabel::SetColor(rgb_color color)
{
	if (color == colorValue)
		return;

	colorValue = color;
	Invalidate(ColorRect()); 
}

BRect 
ColorLabel::ColorRect() const
{
	BRect result(Bounds());
	result.left += divider;
	result.InsetBy(1, 1);
		// room for bevel
	
	return result;
}

void
ColorLabel::SetDivider(float newDivider)
{
	if (divider == newDivider)
		return;
	
	divider = newDivider;
	if (Parent())
		Invalidate();
}

float 
ColorLabel::Divider() const
{
	return divider;
}

void 
ColorLabel::AttachedToWindow()
{
	_inherited::AttachedToWindow();
	proxy.SetTarget(Looper());

	BView *parent = Parent();
	if (!parent)
		return;
	
	// make ourselves transparent
	rgb_color color = parent->ViewColor();
	SetViewColor(color);
	SetLowColor(color);
}

struct general_ui_info {
	rgb_color	background_color;
	rgb_color	mark_color;
	rgb_color	highlight_color;
	bool		color_frame;
	rgb_color	window_frame_color;
};

extern _IMPORT general_ui_info general_info;

void 
ColorLabel::Draw(BRect)
{
	// currently does not handle drawing in a disabled state

	rgb_color high = HighColor();

	// draw the label first
	MovePenTo(Bounds().LeftBottom() + BPoint(2, - 2));
	DrawString(Label());

	PushState();
	BRect colorPadRect = ColorRect();
	SetHighColor(ValueAsColor());
	
	// draw the color label inside, using the selected color
	FillRect(colorPadRect);
	
	// draw a bevel around the color label inside, lightening and darkening the
	// selected color
	rgb_color light = ShiftColor(ValueAsColor(), 0.4);
	rgb_color dark = ShiftColor(ValueAsColor(), 1.2);

	BeginLineArray(8);

	AddLine(colorPadRect.LeftTop(), colorPadRect.RightTop() - BPoint(1, 0), light);
	AddLine(colorPadRect.LeftTop(), colorPadRect.LeftBottom() - BPoint(0, 1), light);
	AddLine(colorPadRect.RightTop() + BPoint(0, 1), colorPadRect.RightBottom(), dark);
	AddLine(colorPadRect.RightBottom(), colorPadRect.LeftBottom() + BPoint(1, 0), dark);
	
	// draw a bevel lightening and darkening the view color - usually gray
	light = ShiftColor(ViewColor(), 0.1);
	dark = ShiftColor(ViewColor(), 1.4);
	colorPadRect.InsetBy(-1, -1);

	BPoint hless(-1, 0);
	BPoint hmore(1, 0);
	BPoint vless(0, -1);
	BPoint vmore(0, 1);

	if (IsFocus() && Window()->IsActive()) {
		// if view focused, draw a focus outline instead
		light = general_info.mark_color;
		dark = general_info.mark_color;
		hless = BPoint(0, 0);
		hmore = BPoint(0, 0);
		vless = BPoint(0, 0);
		vmore = BPoint(0, 0);
	}

	AddLine(colorPadRect.LeftTop(), colorPadRect.RightTop() + hless, dark);
	AddLine(colorPadRect.LeftTop(), colorPadRect.LeftBottom() + vless, dark);
	AddLine(colorPadRect.RightTop() + vmore, colorPadRect.RightBottom(), light);
	AddLine(colorPadRect.RightBottom(), colorPadRect.LeftBottom() + hmore, light);

	EndLineArray();
	PopState();
}

void 
ColorLabel::MessageReceived(BMessage *message)
{
	if (message->WasDropped()) {
		// handle roColour drag&drop
		const rgb_color *color;
		ssize_t size;
		if (message->FindData("RGBColor", 'RGBC',
			reinterpret_cast<const void **>(&color), &size) == B_OK) {
			SetColor(*color);
			// notify the color picker of a color change
			// for simplicity do it here; in a real world version
			// of ColorLabel should do this in SetValue and distinguish
			// between color change from color picker and other changes
			proxy.UpdateValue(*color);
			return;
		}
	}
	
	_inherited::MessageReceived(message);
}


void 
ColorLabel::MouseDown(BPoint oldWhere)
{
	// do some fun drag&drop support
	
	bigtime_t doubleClickSpeed;
	get_click_speed(&doubleClickSpeed);
	bigtime_t startDragTime = system_time() + doubleClickSpeed;
	
	BPoint newWhere;
	uint32 buttons;

	GetMouse(&newWhere, &buttons);
	if ((buttons & B_SECONDARY_MOUSE_BUTTON) || (modifiers() & B_CONTROL_KEY)) {
		proxy.RunPreferredPickerSelector(ConvertToScreen(newWhere));
		return;
	} 
	
	for (;;) {
		GetMouse(&newWhere, &buttons);
		if (!buttons)
			break;
		
		BRect moveMargin(oldWhere, oldWhere);
		moveMargin.InsetBy(-1, -1);
		
		if (system_time() > startDragTime || !moveMargin.Contains(newWhere)) {
			
			// initiate drag&drop - drag a color swatch around
			const int32 kSwatchSize = 10;
			BRect bitmapBounds(0, 0, kSwatchSize, kSwatchSize);
			BBitmap *bitmap = new BBitmap(bitmapBounds, B_COLOR_8_BIT, true);
			BView *view = new BView(bitmapBounds, "", B_FOLLOW_NONE, 0);
			bitmap->AddChild(view);
			bitmap->Lock();
			rgb_color value = ValueAsColor();
			view->SetHighColor(ValueAsColor());
			view->FillRect(view->Bounds());
			view->SetHighColor(100, 100, 100);
			view->StrokeRect(view->Bounds());
			view->Sync();
			bitmap->Unlock();
			BMessage *dragMessage = new BMessage(B_SIMPLE_DATA);
			dragMessage->AddData("RGBColor", 'RGBC', &value, sizeof(value));
			DragMessage(dragMessage, bitmap, BPoint(kSwatchSize - 2, kSwatchSize - 2));
			return;
		} 
	}
	
	Invoke();
}


status_t 
ColorLabel::Invoke(BMessage *)
{
	proxy.Invoke();				
	return B_NO_ERROR;
}
