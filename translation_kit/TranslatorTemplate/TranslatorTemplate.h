#ifndef TRANSLATOR_TEMPLATE_H
#define TRANSLATOR_TEMPLATE_H

#include <Window.h>
#include <View.h>

class TranslatorWindow : public BWindow {
	public:
		TranslatorWindow(BRect rect, const char *name);
		bool QuitRequested();
};

class TranslatorView : public BView {
	public:
		TranslatorView(BRect rect, const char *name);
};

#endif
