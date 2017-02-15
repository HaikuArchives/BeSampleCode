//--------------------------------------------------------------------
//	
//	EZLauncher.cpp
//
//  Written by Robert Polic
//
//--------------------------------------------------------------------

/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <Alert.h>
#include <Application.h>
#include <Roster.h>
#include <Bitmap.h>
#include <Directory.h>
#include <Entry.h>
#include <FindDirectory.h>
#include <ListItem.h>
#include <ListView.h>
#include <MenuItem.h>
#include <Node.h>
#include <NodeInfo.h>
#include <Path.h>
#include <PopUpMenu.h>
#include <ScrollView.h>
#include <View.h>
#include <Window.h>

#define kDEFAULT_ITEM_HEIGHT			(B_LARGE_ICON + 5)
#define kDRAG_SLOP						  4
#define kITEM_MARGIN					  5

enum	MESSAGES						{eItemDblClicked = 1000,
										 eItemMenuSelected,
										 eItemDragged};

const rgb_color kLIST_COLOR				= {144, 176, 160, 255};
const rgb_color kSELECTED_ITEM_COLOR	= {220, 192, 192, 255};
const rgb_color	kTEXT_COLOR				= {0, 0, 0, 255};
const rgb_color kDISABLED_TEXT_COLOR	= {128, 128, 128, 255};


//====================================================================
//
//	Class definitions
//
//====================================================================

class TEZLauncherApplication : public BApplication 
{
public:
					TEZLauncherApplication();
};

//--------------------------------------------------------------------

class TEZLauncherView;
class TEZLauncherWindow : public BWindow 
{
public:
					TEZLauncherWindow(BRect frame); 
	virtual void	MessageReceived(BMessage*);
	virtual	bool	QuitRequested();
private:
	void			BuildList();
	TEZLauncherView	*fList;
};

//--------------------------------------------------------------------

struct list_tracking_data {
	BListView		*view;
	BPoint			start;
};

class TEZLauncherView : public BListView
{
public:
					TEZLauncherView(BRect);
					~TEZLauncherView();
	virtual void	AttachedToWindow();
	virtual void	Draw(BRect);
	virtual void	MouseDown(BPoint);
private:
	static status_t	TrackItem(list_tracking_data*);
	BPopUpMenu		*fMenu;
};

//--------------------------------------------------------------------

class TListItem : public BListItem
{
public:

					TListItem(BEntry*);
					~TListItem();
	virtual void	DrawItem(BView*, BRect, bool);
	virtual	void	Update(BView*, const BFont*);
	BBitmap*		Bitmap() {return fIcon;};
	entry_ref*		Ref() {return &fRef;};
private:
	char			fName[B_FILE_NAME_LENGTH];
	BBitmap			*fIcon;
	entry_ref		fRef;
};


//====================================================================

int main(int, char**)
{	
	TEZLauncherApplication	app;

	app.Run();

	return(B_NO_ERROR);
}


//====================================================================

TEZLauncherApplication::TEZLauncherApplication()
					   :BApplication("application/x-vnd.Be-EZLauncher")
{
	// set up a rectangle, instantiate and show a new window
	(new TEZLauncherWindow(BRect(100, 80, 260, 480)))->Show();
}


//====================================================================

TEZLauncherWindow::TEZLauncherWindow(BRect frame)
				  :BWindow(frame, "EZ Launcher", B_TITLED_WINDOW, B_NOT_ZOOMABLE |
				  												  B_WILL_ACCEPT_FIRST_CLICK)
{
	// set up a rectangle and instantiate a new view
	BRect		aRect(Bounds());
	BScrollView	*aScroller;

	aRect.right -= B_V_SCROLL_BAR_WIDTH;
	fList = new TEZLauncherView(aRect);
	AddChild(aScroller = new BScrollView("", fList, B_FOLLOW_ALL, B_WILL_DRAW, true,
									true, B_PLAIN_BORDER));
	BuildList();
}

//--------------------------------------------------------------------

void TEZLauncherWindow::MessageReceived(BMessage *msg)
{
	char		string[512];
	int32		index;
	entry_ref	entry;
	entry_ref	*ref = NULL;
	status_t	result;
	TListItem	*item;

	switch (msg->what) {
		case eItemDblClicked:
			// item was dbl-clicked.  from the message we can find the item
			msg->FindInt32("index", &index);
			item = dynamic_cast<TListItem *>(fList->ItemAt(index));
			if (item)
				ref = item->Ref();
			break;

		case eItemMenuSelected:
			// item was selected with menu.  find item using CurrentSelection
			index = fList->CurrentSelection();
			item = dynamic_cast<TListItem *>(fList->ItemAt(index));
			if (item)
				ref = item->Ref();
			break;

		case eItemDragged:
			// item was dropped on us.  get ref from message
			if (msg->HasRef("entry_ref")) {
				msg->FindRef("entry_ref", &entry);
				ref = &entry;
			}
			break;

		default:
			BWindow::MessageReceived(msg);
	}
	if (ref) {
		// if we got a ref, try launching it
		result = be_roster->Launch(ref);
		if (result != B_NO_ERROR) {
			sprintf(string, "Error launching: %s", strerror(result));
			(new BAlert("", string, "OK"))->Go();
		}
	}
}

//--------------------------------------------------------------------

bool TEZLauncherWindow::QuitRequested()
{
	be_app->PostMessage(B_QUIT_REQUESTED);
	return(true);
}

//--------------------------------------------------------------------

void TEZLauncherWindow::BuildList()
{
	BDirectory	dir;
	BEntry		entry;
	BPath		path;

	// walk through the apps directory adding all apps to the list
	find_directory(B_APPS_DIRECTORY, &path, true);
	dir.SetTo(path.Path());
	// loop until we get them all
	while (dir.GetNextEntry(&entry, true) == B_NO_ERROR) {
		if (entry.IsFile())
			fList->AddItem(new TListItem(&entry));
	}
}


//====================================================================

TEZLauncherView::TEZLauncherView(BRect rect)
				:BListView(rect, "list_view", B_SINGLE_SELECTION_LIST, B_FOLLOW_ALL)
{
}

//--------------------------------------------------------------------

TEZLauncherView::~TEZLauncherView()
{
	int32		index = 0;
	TListItem	*item;

	// delete all items in list...
	while ((item = dynamic_cast<TListItem *>(ItemAt(index++))) != NULL) {
		delete item;
	}
}

//--------------------------------------------------------------------

void TEZLauncherView::AttachedToWindow()
{
	// build menu
	fMenu = new BPopUpMenu("context menu");
	fMenu->SetRadioMode(false);
	fMenu->AddItem(new BMenuItem("Launch item", new BMessage(eItemMenuSelected)));
	// set window as target so we get the message
	fMenu->SetTargetForItems(Window());

	// set up message we'll get when item is invoked
	SetInvocationMessage(new BMessage(eItemDblClicked));

	// let app_server know we'll be doing all the drawing
	SetViewColor(B_TRANSPARENT_32_BIT);

	// let list view do whatever it needs to do
	BListView::AttachedToWindow();
}

//--------------------------------------------------------------------

void TEZLauncherView::Draw(BRect where)
{
	// intersection of the view and update rect
	BRect	r = Bounds() & where;

	// if rect is valid, fill rect with our background color
	if (r.IsValid()) {
		SetHighColor(kLIST_COLOR);
		FillRect(r);
	}

	// draw all items
	BListView::Draw(where);
}

//--------------------------------------------------------------------

void TEZLauncherView::MouseDown(BPoint where)
{
	uint32		buttons;

	// retrieve the button state from the MouseDown message
	if (Window()->CurrentMessage()->FindInt32("buttons", (int32 *)&buttons) == B_NO_ERROR) {
		// find item at the mouse location
		int32 item = IndexOf(where);
		// make sure item is valid
		if ((item >= 0) && (item < CountItems())) {
			// if clicked with second mouse button, let's do a context-sensitive menu
			if (buttons & B_SECONDARY_MOUSE_BUTTON) {
				BPoint	point = where;
				ConvertToScreen(&point);
				// select this item
				Select(item);
				fMenu->Go(point, true, false, true);
				return;
			}
			// clicked with primary button
			else {
				int32 clicks;
				// see how many times we've been clicked
				Window()->CurrentMessage()->FindInt32("clicks", &clicks);
				// if we've only been clicked once on this item, see if user wants to drag
				if ((clicks == 1) || (item != CurrentSelection())) {
					// select this item
					Select(item);

					// create a structure of useful data
					list_tracking_data	*data = new list_tracking_data();
					data->start = where;
					data->view = this;

					// spawn a thread that watches the mouse to see if a drag
					// should occur.  this will free up the window for more
					// important tasks
					resume_thread(spawn_thread((status_t (*)(void *)) TrackItem,
						"list_tracking", B_DISPLAY_PRIORITY, data));
					return;
				}
			}
		}
	}
	// either the user dbl-clicked an item or clicked in an area with no
	// items.  either way, let BListView take care of it
	BListView::MouseDown(where);
}

//--------------------------------------------------------------------

status_t TEZLauncherView::TrackItem(list_tracking_data *data)
{
	uint32	buttons;
	BPoint	point;

	// we're going to loop as long as the mouse is down and hasn't moved
	// more than kDRAG_SLOP pixels
	while (1) {
		// make sure window is still valid
		if (data->view->Window()->Lock()) {
			data->view->GetMouse(&point, &buttons);
			data->view->Window()->Unlock();
		}
		// not?  then why bother tracking
		else
			break;
		// button up?  then don't do anything
		if (!buttons)
			break;
		// check to see if mouse has moved more the kDRAG_SLOP pixels
		// in any direction
		if ((abs((int)(data->start.x - point.x)) > kDRAG_SLOP) ||
			(abs((int)(data->start.y - point.y)) > kDRAG_SLOP)) {
			// make sure window is still valid
			if (data->view->Window()->Lock()) {
				BBitmap				*drag_bits;
				BBitmap				*src_bits;
				BMessage			drag_msg(eItemDragged);
				BView				*offscreen_view;
				int32				index = data->view->CurrentSelection();
				TListItem			*item;

				// get the selected item
				item = dynamic_cast<TListItem *>(data->view->ItemAt(index));
				if (item) {
					// init drag message with some useful information
					drag_msg.AddInt32("index", index);
					// we can even include the item
					drag_msg.AddRef("entry_ref", item->Ref());

					// get bitmap from current item
					src_bits = item->Bitmap();
					// make sure bitmap is valid
					if (src_bits) {
						// create a new bitmap based on the one in the list (we
						// can't just use the bitmap we get passed because the
						// app_server owns it after we call DragMessage, besides
						// we wan't to create that cool semi-transparent look)
						drag_bits = new BBitmap(src_bits->Bounds(), B_RGBA32, true);
						// we need a view so we can draw
						offscreen_view = new BView(drag_bits->Bounds(), "", B_FOLLOW_NONE, 0);
						drag_bits->AddChild(offscreen_view);

						// lock it so we can draw
						drag_bits->Lock();
						// fill bitmap with black
						offscreen_view->SetHighColor(0, 0, 0, 0);
						offscreen_view->FillRect(offscreen_view->Bounds());
						// set the alpha level
						offscreen_view->SetDrawingMode(B_OP_ALPHA);
						offscreen_view->SetHighColor(0, 0, 0, 128);
						offscreen_view->SetBlendingMode(B_CONSTANT_ALPHA, B_ALPHA_COMPOSITE);
						// blend in bitmap
						offscreen_view->DrawBitmap(src_bits);
						drag_bits->Unlock();

						// initiate drag from center of bitmap
						data->view->DragMessage(&drag_msg, drag_bits, B_OP_ALPHA,
									BPoint(drag_bits->Bounds().Height() / 2,
										   drag_bits->Bounds().Width() / 2));
					} // endif src_bits
					else {
						// no src bitmap? then just drag a rect
						data->view->DragMessage(&drag_msg, BRect(0, 0, B_LARGE_ICON - 1, B_LARGE_ICON - 1));
					}
				} // endif item
				data->view->Window()->Unlock();
			} // endif window lock
			break;
		} // endif drag start
		// take a breather
		snooze(10000);
	} // while button
	// free resource
	free(data);
	return B_NO_ERROR;
}


//====================================================================

TListItem::TListItem(BEntry *entry)
		  :BListItem()
{
	BNode		node;
	BNodeInfo	node_info;

	// try to get node info for this entry
	if ((node.SetTo(entry) == B_NO_ERROR) &&
		(node_info.SetTo(&node) == B_NO_ERROR)) {
		// cache name
		entry->GetName(fName);
		// create bitmap large enough for icon
		fIcon = new BBitmap(BRect(0, 0, B_LARGE_ICON - 1, B_LARGE_ICON - 1), B_COLOR_8_BIT);
		// cache the icon
		node_info.GetIcon(fIcon);
		// adjust size of item to fit icon
		SetHeight(fIcon->Bounds().Height() + kITEM_MARGIN);
		// cache ref
		entry->GetRef(&fRef);
	}
	else {
		fIcon = NULL;
		strcpy(fName, "<Lost File>");
		SetHeight(kDEFAULT_ITEM_HEIGHT);
	}
}

//--------------------------------------------------------------------

TListItem::~TListItem()
{
	// free resources
	delete fIcon;
}

//--------------------------------------------------------------------

void TListItem::DrawItem(BView *view, BRect rect, bool /* complete */)
{
	float		offset = 10;
	BFont		font = be_plain_font;
	font_height	finfo;

	// set background color
	if (IsSelected()) {
		// fill color
		view->SetHighColor(kSELECTED_ITEM_COLOR);
		// anti-alias color
		view->SetLowColor(kSELECTED_ITEM_COLOR);
	}
	else {
		view->SetHighColor(kLIST_COLOR);
		view->SetLowColor(kLIST_COLOR);
	}
	// fill item's rect
	view->FillRect(rect);

	// if we have an icon, draw it
	if (fIcon) {
		view->SetDrawingMode(B_OP_OVER);
		view->DrawBitmap(fIcon, BPoint(rect.left + 2, rect.top + 3));
		view->SetDrawingMode(B_OP_COPY);
		offset = fIcon->Bounds().Width() + 10;
	}

	// set text color
	(IsEnabled()) ?  view->SetHighColor(kTEXT_COLOR) :
					 view->SetHighColor(kDISABLED_TEXT_COLOR);
	
	// set up font
	font.SetSize(12);
	font.GetHeight(&finfo);
	view->SetFont(&font);

	// position pen
	view->MovePenTo(offset,
		rect.top + ((rect.Height() - (finfo.ascent + finfo.descent + finfo.leading)) / 2) +
					(finfo.ascent + finfo.descent) - 2);
	// and draw label
	view->DrawString(fName);
}

//--------------------------------------------------------------------

void TListItem::Update(BView *owner, const BFont *finfo)
{
	// we need to override the update method so we can make sure are
	// list item size doesn't change
	BListItem::Update(owner, finfo);
	if ((fIcon) && (Height() < fIcon->Bounds().Height() + kITEM_MARGIN)) {
		SetHeight(fIcon->Bounds().Height() + kITEM_MARGIN);
	}
	else
		SetHeight(kDEFAULT_ITEM_HEIGHT);
}
