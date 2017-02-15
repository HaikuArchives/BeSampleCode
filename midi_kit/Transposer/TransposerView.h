#ifndef _TransposerView_h
#define _TransposerView_h

#include <View.h>

class BSlider;

class TransposerView : public BView {
public:
	TransposerView(BRect rect, const char* name, uint32 resizingMode, uint32 flags);
		
	void AttachedToWindow();

private:
	BSlider* m_slider;
};


#endif /* _TransposerView_h */
