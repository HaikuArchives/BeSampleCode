/* whack.cpp */

/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <Application.h>
#include <Message.h>

#include <Window.h>
#include <View.h>
#include <Menu.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <TextControl.h>
#include <Button.h>
#include <Bitmap.h>
#include <Alert.h>
#include <Roster.h>

#include <DirectWindow.h>

#include <TranslationKit.h>

#include <Entry.h>
#include <Path.h>
#include <File.h>
#include <FilePanel.h>
#include <FindDirectory.h>

#include "whack.h"
#include "addon.h"
#include "settings.h"

#define DEFAULT_EXPRESSION			(((x+y)^0^x)+f)
#define DEFAULT_EXPRESSION_STR		"(((x+y)^0^x)+f)"

BMessage settings;

void
default_whacker(int32 f, int32 *pixel_cnt, uint8 *framebuffer, int32 rowbytes,
				clipping_rect *rects, int32 nrects, clipping_rect wbounds,
				color_space mode)
{
	int32 i, x, y, ix, iy, t;

	switch(mode) {
	case B_RGB32:
	case B_RGBA32:
	case B_RGB24:
	case B_RGB32_BIG:
	case B_RGBA32_BIG:
	case B_RGB24_BIG:
		for (i = 0; i < nrects; i++) {
			for (y = rects[i].top; y <= rects[i].bottom; y++) {
				for (x = rects[i].left; x <= rects[i].right; x++) {
					t = *pixel_cnt = *pixel_cnt + 1;
					ix = x - wbounds.left;
					iy = y - wbounds.top;
					*(uint32 *)((long(framebuffer)+(x*4))+(rowbytes*y)) =
								DEFAULT_EXPRESSION;
				}
			}
		}
		return;
	
	case B_RGB16:
	case B_RGB15:
	case B_RGBA15:
	case B_RGB16_BIG:
	case B_RGB15_BIG:
	case B_RGBA15_BIG:
		/* fill this in with the color-space conversion
		 * code of your choosing
		 */
		return;

	case B_CMAP8:
		/* same here */
		return;

	default:
		/* unsupported mode */
		return;
	}
}

#if 0
static void
default_whacker(int32 f, int32 *pixel_cnt, uchar *framebuffer, int32 rowbytes,
				clipping_rect *rects, int32 nrects, clipping_rect wbounds,
				color_space mode)
{
	int32 i, x, y, ix, iy, t;

	if (mode == B_RGB32 || mode == B_RGBA32 || mode == B_RGB24) {
		for (i = 0; i < nrects; i++) {
			for (y = rects[i].top; y <= rects[i].bottom; y++) {
				for (x = rects[i].left; x <= rects[i].right; x++) {
					ix = x - wbounds.left;
					iy = y - wbounds.top;
					*(uint32 *)((long(framebuffer)+(x*4))+(rowbytes*y)) =
													DEFAULT_EXPRESSION;
				}
			}
		}
		return;
	}
	
	/* here is where you can do colorspace conversion to support
	 * multiple pixel depths, etc.  i didn't have time to do this
	 */
	if (mode == B_RGB16 || mode == B_RGB15 || mode == B_RGBA15) {
		return;
	}

	if (mode == B_CMAP8) {
		return;
	}
	
	/* unsupported mode */
	return;
}
#endif

static whack_addon default_addon = {
	DEFAULT_NAME,
	"/no/where",
	DEFAULT_EXPRESSION_STR,
	-1,
	default_whacker
};

WhackView::WhackView(BRect r)
		: BView(r, "whack_view", B_FOLLOW_ALL, B_WILL_DRAW)
{
	/* so the app_server doesn't try to clear it */
	SetViewColor(B_TRANSPARENT_32_BIT);
	
	_last_hit = 0;
}

void 
WhackView::MessageReceived(BMessage *msg)
{
	switch(msg->what) {
	case B_SIMPLE_DATA:
		if (msg->HasRef("refs")) {
			msg->what = FILE_OPENED;
			Window()->PostMessage(msg);
		}
		break;
	default:
		BView::MessageReceived(msg);
	}
}

void
WhackView::MouseDown(BPoint p)
{
	bigtime_t t = system_time();
	bigtime_t interval;
	
	get_click_speed(&interval);

	if ((t - _last_hit) < interval)
		Window()->PostMessage(MENU_FULL_SCREEN);

	_last_hit = t;
}

/* ------------------------------------------------------------ */

WhackWindow::WhackWindow(BRect r)
			: BDirectWindow(r, "Whack", B_TITLED_WINDOW_LOOK,
							B_NORMAL_WINDOW_FEEL, 0)
{
	float t, b;


	_addon = &default_addon;
	
	/* directwindow stuff */
	_connected = false;
	_disabled = false;
	_rects = NULL;
	_nrects = 0;
	
	_frame_count = 0;
	_pixel_count = 0;
	
	LoadFavorites();

	if (!SupportsWindowMode()) {
		ErrorAlert("Your display doesn't support direct drawing.");
		be_app->PostMessage(B_QUIT_REQUESTED);
		return;
	}

	r.OffsetTo(B_ORIGIN);
	t = InitMenuBar(r);
	b = InitExpression(r);

	SetSizeLimits(100, 32767,
			_menubar_height + _expression_height + 100, 32767);
	
#if QUICKPAINT_SUITE
	_quickpaint_present = false;
	CheckForQuickPaint();
	be_roster->StartWatching(BMessenger(this));
#endif

	r.top = t;
	r.bottom = b;
	_whack_view = new WhackView(r);
	AddChild(_whack_view);
	
	_dirty = true;
	_draw_thread = spawn_thread(_draw_loop_, "drawing thread",
								B_NORMAL_PRIORITY, this);
	resume_thread(_draw_thread);
}

WhackWindow::~WhackWindow()
{
	int32 junk;
	
	_disabled = true;
	Hide();
	Sync();
	wait_for_thread(_draw_thread, &junk);
	free(_rects);
	
	UnloadFavorites();
	clean_temporary_files();
	
	if (settings.HasRect(WINDOW_RECT))
		settings.ReplaceRect(WINDOW_RECT, Frame());
	else
		settings.AddRect(WINDOW_RECT, Frame());
}

#if QUICKPAINT_SUITE
void
WhackWindow::CheckForQuickPaint(void)
{
	bool present;
	
	present = be_roster->IsRunning(QUICKPAINT_SIGNATURE);
	if (present == _quickpaint_present)
		return; /* no change */

	_quickpaint_present = present;
	
	if (!_quickpaint_present) {
		_menubar->RemoveItem(_quickpaint_menu);
		return;
	}

	_quickpaint_menu = new BMenu("QuickPaint");
	BMenuItem *i = new BMenuItem("Send Frame to QuickPaint",
							new BMessage(MENU_QUICKPAINT_SEND_FRAME));
	_quickpaint_menu->AddItem(i);
	_menubar->AddItem(_quickpaint_menu);
}
#endif /* QUICKPAINT_SUITE */

bool 
WhackWindow::QuitRequested()
{
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}

void 
WhackWindow::MessageReceived(BMessage *msg)
{
	entry_ref ref;
	BEntry entry;
	BPath path, path2;
	const char *name;
	BMessenger self(this);
	
	switch(msg->what) {
#if QUICKPAINT_SUITE
	/* the roster has notified us that there has been a change in
	 * the list of running apps.  update the status of quickpaint.
	 */
	case B_SOME_APP_LAUNCHED:
	case B_SOME_APP_QUIT:
		CheckForQuickPaint();
		break;
	
	/* the user wants to send the current frame to quickpaint.
	 * initiate the B_SIMPLE_DATA drag-and-drop protocol.
	 */
	case MENU_QUICKPAINT_SEND_FRAME: {
		BMessenger m(QUICKPAINT_SIGNATURE);
		BMessage simple(B_SIMPLE_DATA);
		simple.AddInt32("be:actions", B_COPY_TARGET);
		simple.AddString("be:types", "image/x-be-bitmap");
		m.SendMessage(&simple, this);
		break;
	}

	/* quickpaint has responded to our request.  hack, hack, hack
	 * our way into sending it an image.
	 */
	case B_COPY_TARGET: {
		_lock.Lock();
		/* create a bitmap as big as the whack_view currently is,
		 * and build a fake clipping_rect list to pass to the
		 * current frame whacker to fake out the framebuffer.
		 */
		BBitmap *bm = new BBitmap(_whack_view->Bounds(), B_RGBA32);
		BRect r = bm->Bounds();
		clipping_rect cr;
		cr.left = (int)r.left;
		cr.right = (int)r.right;
		cr.top = (int)r.top;
		cr.bottom = (int)r.bottom;
		_addon->whacker(_frame_count, &_pixel_count, (uint8 *)bm->Bits(),
						bm->BytesPerRow(), &cr, 1, _bounds, B_RGBA32);
		_lock.Unlock();
		/* send the final response.  since we are sending in the
		 * image/x-be-bitmap format, we need to allocate yet more
		 * memory and feed it through BitmapStream.  fortunately,
		 * this only happens once in a while.
		 */
		BMessage mime(B_MIME_DATA);
		BBitmapStream inStream(bm);
		BMallocIO outStream;
		uint8 buffer[1024];
		int32 size;
		while ((size = inStream.Read(buffer, 1024)) > 0) {
			outStream.Write(buffer, size);
		}
		mime.AddData("image/x-be-bitmap", B_MIME_TYPE, outStream.Buffer(),
						outStream.BufferLength());
		msg->SendReply(&mime, this);
		delete bm;
		break;
	}
#endif /* QUICKPAINT_SUITE */

#if DEBUG
	case MENU_DEBUG_DUMP_ADDON:
		printf("---- addon dump ----\n");
		printf("name: %s\n", _addon->name);
		printf("path: %s\n", _addon->path);
		printf("expr: %s\n", _addon->expression);
		printf("image: %d\n", _addon->image);
		printf("whacker: 0x%x\n", _addon->whacker);
		break;
	
	case MENU_DEBUG_DUMP_FAVORITES:
		printf("---- favorites dump ----\n");
		for (int32 i = 0; i < _favorites->CountItems(); i++) {
			printf("%s\n", (const char *)_favorites->ItemAt(i));
		}
		break;
		
#endif /* DEBUG */

	case MENU_OPEN:
		if (_open_panel == NULL) {
			_open_panel = new BFilePanel(B_OPEN_PANEL, &self,
										NULL, B_FILE_NODE, false,
										new BMessage(FILE_OPENED),
										NULL);
		}
		_open_panel->Show();
		break;
		
	case MENU_SAVE_AS:
		if (_addon == &default_addon) {
			ErrorAlert("You can't save the default add-on.");
			break;
		}
			
		if (_save_panel == NULL) {
			_save_panel = new BFilePanel(B_SAVE_PANEL, &self,
										NULL, B_FILE_NODE, false,
										new BMessage(FILE_SAVED),
										NULL);
		}
		_save_panel->Show();
		break;
		
	case MENU_ABOUT: {
		BAlert *a = new BAlert("About Whack",
			"Whack 2.0\n"
			"by Ficus Kirkpatrick\n"
			"\n"
			"Thanks to Benoit for inspiration and George for\n"
			"the cross-licensing agreement with QuickPaint, Inc.\n",
			"Unh!", "Hit me", "Oh yeah");
		a->Go();
		break;
	}
				
	case MENU_ADD_FAVORITE:
		AddFavorite();
		break;
	
	case MENU_REMOVE_FAVORITE:
		RemoveFavorite(_addon->name);
		break;
		
#if 0
	case MENU_FULL_SCREEN:
		SetFullScreen(!IsFullScreen());
		break;
#endif

	case MENU_RESET:
		_frame_count = 0;
		_pixel_count = 0;
		break;
				
	case MENU_SETTINGS:
		/* ha ha.  what was i thinking. */
		break;
		
	case BUTTON_COMPILE:
		_compile_thread = spawn_thread(_compile_, "compiler thread",
										B_NORMAL_PRIORITY, this);
		resume_thread(_compile_thread);
		break;

	case FAVORITE_SELECTED:
		name = msg->FindString("name");
		_favorites_dir.GetEntry(&entry);
		entry.GetPath(&path);
		path.Append(name);
		ChangeAddon(path.Path());
		break;
		
	case FILE_OPENED:
		msg->FindRef("refs", &ref);
		entry.SetTo(&ref);
		entry.GetPath(&path);
		ChangeAddon(path.Path());
		break;

	case FILE_SAVED:
		name = msg->FindString("name");
		strcpy(_addon->name, name);
		_save_panel->GetPanelDirectory(&ref);		
		entry.SetTo(&ref);
		entry.GetPath(&path);
		if (save_addon(_addon, path.Path()) != B_OK) {
			ErrorAlert("The add-on could not be saved.");
			break;
		}
		ChangeAddon(_addon);
		break;

	default:
		BDirectWindow::MessageReceived(msg);
	}
}

void 
WhackWindow::DirectConnected(direct_buffer_info *info)
{
	uint32 i;
	
	if (!_connected && _disabled)
		return;
		
	_lock.Lock();
	
	switch(info->buffer_state & B_DIRECT_MODE_MASK) {
	case B_DIRECT_START:
		_connected = true;
		/* fall through */
	case B_DIRECT_MODIFY:
		_framebuffer = (uint8 *)info->bits;
		_bytes_per_row = info->bytes_per_row;
		_mode = info->pixel_format;
		_dirty = true;
		_bounds = info->window_bounds;
		
		free(_rects);
		_nrects = info->clip_list_count;
		_rects = (clipping_rect *)malloc(_nrects * sizeof(clipping_rect));
		memcpy(_rects, info->clip_list, _nrects * sizeof(clipping_rect));

		/* clip the rectangles to account for the menu bar and
		 * expression entry control.
		 */
		for (i = 0; i < _nrects; i++) {
			if (_rects[i].top <= (_bounds.top+_menubar_height))
				_rects[i].top = _bounds.top+_menubar_height+1;
			if (_rects[i].bottom >= (_bounds.bottom-_expression_height))
				_rects[i].bottom = _bounds.bottom-_expression_height-1;
		}
		break;
	
	case B_DIRECT_STOP:
		_connected = false;
		break;
	}
	
	_lock.Unlock();
}

int32 
WhackWindow::_draw_loop_(void *data)
{
	WhackWindow *w = (WhackWindow *)data;
	return w->DrawLoop();
}

int32 
WhackWindow::DrawLoop(void)
{
	while (!_disabled) {
		_lock.Lock();
		if (_connected) {
			_frame_count++;
			_addon->whacker(_frame_count, &_pixel_count, _framebuffer,
							_bytes_per_row,	_rects, _nrects, _bounds, _mode);
		}
		_lock.Unlock();
		snooze(20000);
	}
	
	return 0;
}

int32
WhackWindow::_compile_(void *data)
{
	WhackWindow *w = (WhackWindow *)data;
	return w->Compile();
}

int32
WhackWindow::Compile(void)
{
	/* disable the expression text control and "go" button until
	 * the compiler has exited.  this is sort of cheesy, and it
	 * would be nice to be able to interrupt the compiler, but
	 * this will have to do for now.
	 */
	if (Lock()) {
		_compile->SetEnabled(false);
		_expression->SetEnabled(false);
		Unlock();
	}
	
	whack_addon *a = build_addon(_expression->Text());
	
	if (a)
		ChangeAddon(a);
	else
		ErrorAlert("The expression could not be compiled. Sorry.");
	
	if (Lock()) {
		_compile->SetEnabled(true);
		_expression->SetEnabled(true);
		Unlock();
	}
		
	return 0;
}

void
WhackWindow::ChangeAddon(const char *path)
{
	whack_addon *new_addon;
	
	new_addon = load_addon(path);
	if (new_addon != NULL)
		ChangeAddon(new_addon);
}

void
WhackWindow::ChangeAddon(whack_addon *addon)
{		
	_lock.Lock();
	if (_addon != &default_addon && addon != _addon)
		destroy_addon(_addon);

	_addon = addon;
	if (_addon == &default_addon || !strcmp(_addon->name, DEFAULT_NAME)) {
		_favorites_add->SetEnabled(false);
		_favorites_remove->SetEnabled(false);
	} else if (IsFavorite(_addon->name)) {
		_favorites_add->SetEnabled(false);
		_favorites_remove->SetEnabled(true);
	} else {
		_favorites_add->SetEnabled(true);
		_favorites_remove->SetEnabled(false);
	}
//	_expression->SetText(_addon->expression);
	_lock.Unlock();
}

void
WhackWindow::ErrorAlert(const char *error_str)
{
	BAlert *a = new BAlert("Error",  error_str, "Nuts", NULL, NULL,
							B_WIDTH_AS_USUAL, B_STOP_ALERT);
	a->Go();
}

void
WhackWindow::LoadFavorites(void)
{
	BPath path;
	BEntry entry;
	char name[B_FILE_NAME_LENGTH];
	char *favorite;
	
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path, true) != B_OK)
		path.SetTo("/boot/home/config/settings");
	path.Append(FAVORITES_DIR);
	mkdir(path.Path(), 0755);
	
	_favorites = new BList();
	
	_favorites_dir.SetTo(path.Path());
	while (_favorites_dir.GetNextEntry(&entry) == B_OK) {
		entry.GetName(name);
		favorite = (char *)malloc(strlen(name)+1);
		strcpy(favorite, name);
		if (favorite)
			_favorites->AddItem((void *)favorite);
	}
}

void
WhackWindow::UnloadFavorites(void)
{
	for (int32 i = 0; i < _favorites->CountItems(); i++) {
		free((char *)_favorites->ItemAt(i));
	}
}

void
WhackWindow::AddFavorite(void)
{
	BEntry entry;
	BPath path;
	
	_favorites_dir.GetEntry(&entry);
	entry.GetPath(&path);
	if (save_addon(_addon, path.Path()) != B_OK) {
		ErrorAlert("There was an error when attempting to save this favorite.");
		return;
	}

	_favorites_add->SetEnabled(false);
	_favorites_remove->SetEnabled(true);
		
	_favorites->AddItem(_addon->name);
	BMessage m(FAVORITE_SELECTED);
	m.AddString("name", _addon->name);

	/* separate in the menu bar if this is the first */
	if (_favorites->CountItems() == 1)
		_menubar->SubmenuAt(1)->AddSeparatorItem();
		
	_menubar->SubmenuAt(1)->AddItem(new BMenuItem(_addon->name, new BMessage(m)));
}

void
WhackWindow::RemoveFavorite(const char *name)
{
	BEntry entry;
	int32 i;

	_favorites_dir.FindEntry(name, &entry);
	entry.Remove();
	
	/* remove it from the favorites list */
	for (i = 0; i < _favorites->CountItems(); i++) {
		if (!strcmp(name, (const char *)_favorites->ItemAt(i))) {
			_favorites->RemoveItem(i);
			break;
		}
	}

	/* remove it from the menu bar */
	for (i = 0; i < _menubar->SubmenuAt(1)->CountItems(); i++) {
		if (!strcmp(name, ((BMenuItem *)_menubar->SubmenuAt(1)->ItemAt(i))->Label())) {
			_menubar->SubmenuAt(1)->RemoveItem(i);
			if (_favorites->IsEmpty())
				_menubar->SubmenuAt(1)->RemoveItem(i);
			break;
		}
	}
	
	/* go back to the default */
	ChangeAddon(&default_addon);
}

bool
WhackWindow::IsFavorite(const char *name)
{
	for (int32 i = 0; i < _favorites->CountItems(); i++) {
		if (!strcmp((const char *)_favorites->ItemAt(i), name))
			return true;
	}
	
	return false;
}

float
WhackWindow::InitMenuBar(BRect bounds)
{
	BRect r = bounds;
	int32 x;
		
	r.bottom = r.top + 20;
	_menubar = new BMenuBar(r, "menu_bar");
	r.bottom = r.top + _menubar->Frame().bottom+1;

	BMenu *m;
	BMenuItem *i;
	
	m = new BMenu("File");
	i = new BMenuItem("Open", new BMessage(MENU_OPEN), 'O');
	m->AddItem(i);
	i = new BMenuItem("Save As" B_UTF8_ELLIPSIS, new BMessage(MENU_SAVE_AS), 'S');
	m->AddItem(i);
	m->AddSeparatorItem();
	i = new BMenuItem("About Whack", new BMessage(MENU_ABOUT));
	m->AddItem(i);
	m->AddSeparatorItem();
	i = new BMenuItem("Quit", new BMessage(B_QUIT_REQUESTED), 'Q');
	m->AddItem(i);
	_menubar->AddItem(m);

	m = new BMenu("Favorites");
	_favorites_add = new BMenuItem("Add to Favorites",
									new BMessage(MENU_ADD_FAVORITE), 'A');
	_favorites_add->SetEnabled(false);
	m->AddItem(_favorites_add);
	_favorites_remove = new BMenuItem("Remove from Favorites",
										new BMessage(MENU_REMOVE_FAVORITE));
	m->AddItem(_favorites_remove);
	_favorites_remove->SetEnabled(false);
	if ((x = _favorites->CountItems()) > 0) {
		m->AddSeparatorItem();
		for (int32 j = 0; j < x; j++) {
			const char *name = (const char *)_favorites->ItemAt(j);
			BMessage *msg = new BMessage(FAVORITE_SELECTED);
			msg->AddString("name", name);
			m->AddItem(new BMenuItem(name, msg));
		}
	}
	_menubar->AddItem(m);

#if 0
	m = new BMenu("View");
	i = new BMenuItem("Reset", new BMessage(MENU_RESET));
	m->AddItem(i);
	_menubar->AddItem(m);
#endif
	
#if DEBUG
	m = new BMenu("Debug");
	i = new BMenuItem("Dump Addon", new BMessage(MENU_DEBUG_DUMP_ADDON));
	m->AddItem(i);
	i = new BMenuItem("Dump Favorites", new BMessage(MENU_DEBUG_DUMP_FAVORITES));
	m->AddItem(i);
	_menubar->AddItem(m);
#endif
		
	AddChild(_menubar);
	
	bounds = _menubar->Bounds();
	_menubar_height = (int32)(bounds.bottom - bounds.top);
	
	return r.bottom - 1;
}

float
WhackWindow::InitExpression(BRect bounds)
{
	BRect r = bounds;
	r.top = r.bottom - 23;
	_bottom_view = new BView(r, "some_view", B_FOLLOW_LEFT_RIGHT|B_FOLLOW_BOTTOM, 0);
	_bottom_view->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	AddChild(_bottom_view);

	r.OffsetTo(B_ORIGIN);
	float w = be_plain_font->StringWidth("Go");
	r.right -= 3;
	r.left = r.right - w - 14;
	r.bottom -= 3;
	_compile = new BButton(r, "compile_button", "Go", new BMessage(BUTTON_COMPILE),
							B_FOLLOW_RIGHT|B_FOLLOW_BOTTOM);
	_bottom_view->AddChild(_compile);
	r.bottom += 3;
	
	r.right = r.left - 5;
	r.left = 0;
	r.top += 2;
	_expression = new BTextControl(r, "expression_view",
							NULL, DEFAULT_EXPRESSION_STR,
							new BMessage(BUTTON_COMPILE),
							B_FOLLOW_LEFT_RIGHT|B_FOLLOW_BOTTOM);
	_bottom_view->AddChild(_expression);
	r.top -= 2;
	
	_expression_height = (int32)(r.bottom - r.top);
	return (bounds.bottom - (r.bottom - r.top));
}

int
main(void)
{
	BApplication app(APP_SIGNATURE);
	BPath path, dir;
	
	/* the metrowerks compiler will leave turds all over the current
	 * directory if we do not do this
	 */
	chdir("/tmp");

	/* find the settings file and load it into a BMessage */
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path, true) != B_OK)
		path.SetTo("/boot/home/config/settings");
	path.Append(SETTINGS_FILE);
	path.GetParent(&dir);
	mkdir(dir.Path(), 0755);
	
	BFile settings_file(path.Path(), B_READ_WRITE|B_CREATE_FILE);
	settings.Unflatten(&settings_file);
	
	BRect window_rect(100, 100, 500, 500);
	if (settings.HasRect(WINDOW_RECT))
		window_rect = settings.FindRect(WINDOW_RECT);

	WhackWindow *w = new WhackWindow(window_rect);
	w->Show();
			
	app.Run();
	
	/* write the settings back into a BMessage */
	settings_file.SetSize(0);
	settings_file.Seek(0, SEEK_SET);
	settings.Flatten(&settings_file);
	
	return 0;
}
