// Copyright 2000, Be Incorporated.   All Rights Reserved.
// This file may be used under the terms of the Be Sample Code License.

#include <Window.h>
#include <ScrollBar.h>
#include <CheckBox.h>
#include <Button.h>
#include <stdio.h>

class Ruler: public BView
{
	public:
		Ruler();
		virtual void AttachedToWindow();
		virtual void Draw(BRect);
};

class CPUView: public BView
{
	public:
		CPUView();
		virtual void AttachedToWindow();
		virtual void Draw(BRect);
		virtual void FrameResized(float x, float y);
};

class Scroller: public BScrollBar
{
	public:
		Scroller();
		virtual void AttachedToWindow();
		virtual void ValueChanged(float newvalue);
};

class ControlView: public BView
{
	public:
		ControlView();
		virtual void AttachedToWindow();
		virtual void MessageReceived(BMessage *mes);
};

class DisplayView: public BView
{
	public:
		DisplayView();
		virtual void AttachedToWindow();
		virtual void Draw(BRect);
};
