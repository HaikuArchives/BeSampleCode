#include "WhackLoader.h"
#include <StringView.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <PopUpMenu.h>
#include <Directory.h>
#include <Entry.h>
#include <FindDirectory.h>
#include <Path.h>
#include <stdlib.h>
#include <string.h>

#define STEPS 50

class SetupView : public BView
{
	BPath	*pref;
public:
	SetupView(BRect frame, const char *name, BPath *preferred)
	 : BView(frame, name, 0, B_FOLLOW_ALL), pref(preferred)
	{
	}

	void AttachedToWindow()
	{
		SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
		AddChild(new BStringView(BRect(10, 10, 230, 35), B_EMPTY_STRING, "WhackLoader — the screen saver factory"));
		AddChild(new BStringView(BRect(25, 80, 200, 95), B_EMPTY_STRING, "Pick your favorites in Whack"));
		AddChild(new BStringView(BRect(25, 96, 200, 111), B_EMPTY_STRING, "to populate this menu"));

		BPopUpMenu *pu = new BPopUpMenu("");

		BMenuItem	*it;
		BPath		path;
		BEntry		entry;
		BDirectory	dir;

		if (find_directory(B_USER_SETTINGS_DIRECTORY, &path, true) != B_OK)
			path.SetTo("/boot/home/config/settings");
		path.Append("whack/favorites");

		dir.SetTo(path.Path());
		while(dir.GetNextEntry(&entry) == B_OK)
		{
			BMessage *msg = new BMessage('pick');
			BPath path;
			entry.GetPath(&path);
			msg->AddString("path", path.Path());
			pu->AddItem(it = new BMenuItem(path.Leaf(), msg));
			if(path == *pref)
				it->SetMarked(true);
		}

		pu->SetTargetForItems(this);
		BMenuField *whack = new BMenuField(BRect(10, 50, 200, 70), "preferred", "Whack:", pu);
		whack->SetDivider(70);
		AddChild(whack);
	}

	void MessageReceived(BMessage *msg)
	{
		const char *path;

		switch(msg->what)
		{
			case 'pick' :
				if(msg->FindString("path", &path) == B_OK)
					pref->SetTo(path);
				break;

			default :
				BView::MessageReceived(msg);
				break;
		}
	}
};

// MAIN INSTANTIATION FUNCTION
extern "C" _EXPORT BScreenSaver *instantiate_screen_saver(BMessage *message, image_id image)
{
	return new WhackLoader(message, image);
}

WhackLoader::WhackLoader(BMessage *message, image_id id)
 : BScreenSaver(message, id)
{
	const char *path;
	message->FindString("whack", &path);
	whack.SetTo(path);
}

status_t WhackLoader::SaveState(BMessage *into) const
{
	if(whack.Path())
		into->AddString("whack", whack.Path());
	return B_OK;
}

void WhackLoader::StartConfig(BView *view)
{
	setup = new SetupView(view->Bounds(), "setup", &whack);
	view->AddChild(setup);
}

status_t WhackLoader::StartSaver(BView *, bool preview)
{
	SetTickSize(20000);

	whacker = 0;
	image = load_add_on(whack.Path());
	if(image > 0)
		get_image_symbol(image, "whack_frame", B_SYMBOL_TYPE_TEXT,
						(void **)&(whacker));

	return preview ? B_ERROR : B_OK;	// won't work in preview mode
}

void WhackLoader::StopSaver()
{
	unload_add_on(image);
}

void WhackLoader::DirectConnected(direct_buffer_info *info)
{
	_framebuffer = (uint8 *)info->bits;
	_bytes_per_row = info->bytes_per_row;
	_mode = info->pixel_format;
	_bounds = info->window_bounds;
}

void WhackLoader::DirectDraw(int32 frame)
{
	int32 _pixel_count_i_dont_care_about = 0;

	if(whacker)
		whacker(frame, &_pixel_count_i_dont_care_about, _framebuffer,
						_bytes_per_row,	&_bounds, 1, _bounds, _mode);
}
