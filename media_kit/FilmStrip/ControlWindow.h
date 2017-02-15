#ifndef _CONTROLWINDOW_H_
#define _CONTROLWINDOW_H_

#include <app/Messenger.h>
#include <interface/CheckBox.h>
#include <interface/Slider.h>
#include <interface/Window.h>

class ControlWindow : public BWindow {
public:
	ControlWindow(const BPoint offset,const BWindow* const win);
	~ControlWindow(void);
	
	void MessageReceived(BMessage* msg);
	
private:
	BMessenger* mFSWin;
	BSlider* mGrabIntvlSlider;
	BCheckBox* mDropFramesChkBox;
	BSlider* mFrameIntvlSlider;
};

#endif // #ifndef _CONTROLWINDOW_H_