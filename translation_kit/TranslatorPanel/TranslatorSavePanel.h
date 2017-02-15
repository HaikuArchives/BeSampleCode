#ifndef TRANSLATORSAVEPANEL_H
#define TRANSLATORSAVEPANEL_H

#include <storage/FilePanel.h>
#include <interface/PopUpMenu.h>
#include <interface/MenuField.h>
#include <interface/MenuItem.h>
#include <interface/Window.h>
#include <app/Handler.h>
#include <translation/TranslationDefs.h>

class TranslatorMenuItem : public BMenuItem {
	public:
		TranslatorMenuItem(const char *name, BMessage *message, translator_id id, uint32 format);
		
		translator_id id;
		uint32 format;
};

class TranslatorSavePanel : public BFilePanel, public BHandler {
	public:
		TranslatorSavePanel(const char *name, BMessenger *target, entry_ref *start_directory,
			uint32 node_flavors, bool allow_multiple_selection, BMessage *message,
			BRefFilter *filter = NULL, bool modal = false, bool hide_when_done = true);
		void MessageReceived(BMessage *message);
		TranslatorMenuItem *GetCurrentMenuItem();
		~TranslatorSavePanel();

	private:
		void BuildMenu();
		void TranslatorSettings();

		BWindow *configwindow;
		BPopUpMenu *formatpopupmenu;
		BMenuField *formatmenufield;
		uint32 what;
};

#endif