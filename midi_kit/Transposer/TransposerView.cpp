#include <Slider.h>
#include "TransposerApp.h"
#include "TransposerView.h"

TransposerView::TransposerView(BRect rect, const char* name, uint32 resizingMode, uint32 flags)
	: BView(rect, name, resizingMode, flags)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	
	m_slider = new BSlider(BRect(10, 10, 190, 30), "Transpose",
		"Transpose", new BMessage(MSG_CHANGE_TRANSPOSE), -36, 36);
	AddChild(m_slider); 
}

void TransposerView::AttachedToWindow()
{
	m_slider->SetTarget(be_app); 
	m_slider->SetLimitLabels("-36", "+36");
	m_slider->SetHashMarks(B_HASH_MARKS_BOTH);
	m_slider->SetHashMarkCount(7);
	m_slider->MakeFocus();
}
