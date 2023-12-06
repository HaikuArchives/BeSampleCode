/* QueryWindow.cpp */
/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#define DEUBUG 1
#include <Debug.h>

#include "QueryWindow.h"
#include <View.h>
#include <Query.h>
#include <ListView.h>
#include <ListItem.h>
#include <Application.h>
#include <StringView.h>
#include <stdio.h>
#include <Volume.h>
#include <Path.h>
#include <stdlib.h>
#include <Box.h>
#include <MenuField.h>
#include <PopUpMenu.h>
#include <MenuItem.h>
#include <TextControl.h>
#include <Button.h>
#include <ScrollView.h>
#include <Roster.h>
#include <fs_query.h>


const int32	FILES_TYPE =	'?fil';
const int32	FOLDERS_TYPE =	'?fol';
const int32	SOURCE_TYPE	=	'?src';
const int32	CPLUSPLUS =		'?c++';
const int32	STRAIGHTC =		'?ccc';
const int32	LIVE =			'?liv';
const int32	FETCH =			'?fch';

int main(int, char**) {
	BApplication app("application/x-vnd.BeDTS-QueryApp");
	QueryWindow *window = new QueryWindow();
	window->Show();
	app.Run();
	return 0;
}


QueryWindow::QueryWindow()
	: BWindow(BRect(50,50,394,399), "QueryWindow", B_TITLED_WINDOW, B_NOT_RESIZABLE),
		fFileType(FILES_TYPE), fMethod(CPLUSPLUS)
{
	//create a top view so things look nice
	BBox *top = new BBox(Bounds(), "top", B_FOLLOW_ALL, B_WILL_DRAW|B_NAVIGABLE, B_PLAIN_BORDER);
	AddChild(top); 

	//build the items for choosing the query
	BPopUpMenu * pop_up = new BPopUpMenu("FileType");
	BMenuItem *item = new BMenuItem("Files", new BMessage(FILES_TYPE));
	item->SetMarked(true);
	pop_up->AddItem(item);
	item = new BMenuItem("Folders", new BMessage(FOLDERS_TYPE));
	pop_up->AddItem(item);
	item = new BMenuItem("Source", new BMessage(SOURCE_TYPE));
	pop_up->AddItem(item);
	BMenuField * menu_field = new BMenuField(BRect(25, 5, 175, 17), "FileType", "File Type:", pop_up);
	menu_field->SetDivider(be_bold_font->StringWidth("File Type:"));
	top->AddChild(menu_field);

	pop_up = new BPopUpMenu("Method");
	item = new BMenuItem("BQuery", new BMessage(CPLUSPLUS));
	item->SetMarked(true);
	pop_up->AddItem(item);
	item = new BMenuItem("C Functions", new BMessage(STRAIGHTC));
	pop_up->AddItem(item);
	item = new BMenuItem("Live", new BMessage(LIVE));
	item->SetEnabled(false);
	pop_up->AddItem(item);
	menu_field = new BMenuField(BRect(185, 5, 314, 17), "Method Menu", "Method:", pop_up);
	menu_field->SetDivider(be_bold_font->StringWidth("Method:"));
	top->AddChild(menu_field); 

	fNameString = new BTextControl(BRect(25,27, 314, 32), "Name String", "Names Contains:", "query", NULL);
	
	fNameString->SetDivider(be_bold_font->StringWidth("Name Contains:"));
	top->AddChild(fNameString);

	BButton *button = new BButton(BRect(25,47, 314, 57), "Fetch", "Fetch Query Results", new BMessage(FETCH));
	top->AddChild(button);

	//our results go here
	BStringView *label = new BStringView(BRect(25,80,314, 90), "Results", "Query String:");
	label->SetAlignment(B_ALIGN_CENTER);
	top->AddChild(label);
	
	fQueryString = new BStringView(BRect(25,90,314, 102), "QueryString", "");
	fQueryString->SetViewColor(215,215,215);
	fQueryString->SetAlignment(B_ALIGN_CENTER);
	BFont font;
	fQueryString->GetFont(&font);
	font.SetSize(font.Size() -2);
	fQueryString->SetFont(&font);
	top->AddChild(fQueryString);

	//build our list
	BRect rect(15,105,327,344);
	BBox *item_box = new BBox(rect, "Matching Paths");
	item_box->SetLabel("Matching Paths");
	rect.OffsetTo(5,15);
	rect.right = 293;
	rect.bottom = 221;
	
	fItemList = new BListView(rect, "ItemList");
	fItemList->GetFont(&font);
	size_t size = (size_t)font.Size();
	if (size <=10) size -= 1;
	else size -= 2;
	font.SetSize(size);
	fItemList->SetFont(&font);

	BScrollView *scroller = new BScrollView("Scroller", fItemList, B_FOLLOW_LEFT|B_FOLLOW_TOP,0, true, true);
	BScrollBar *scroll = scroller->ScrollBar(B_HORIZONTAL);
	scroll->SetRange(0, 200);
	item_box->AddChild(scroller);
	top->AddChild(item_box);
}


QueryWindow::~QueryWindow()
{
	ClearResults();
}

void 
QueryWindow::MessageReceived(BMessage *msg)
{
	switch(msg->what) {

		case FILES_TYPE:
		case FOLDERS_TYPE:
		case SOURCE_TYPE:
			fFileType = msg->what;
			break;
		case CPLUSPLUS:
		case STRAIGHTC:
			fMethod = msg->what;
			break;
		case LIVE:
			printf("live queries not yet supported\n");
			break;
		case FETCH:
			ClearResults();
			GetResults();
			break;

		default:
			BWindow::MessageReceived(msg);
			break;
	}
}

bool 
QueryWindow::QuitRequested()
{
	be_app->PostMessage(B_QUIT_REQUESTED);
	TRACE();
	return true;
}

void 
QueryWindow::GetResults()
{
	//disable window updates
	DisableUpdates();

	if (fMethod == CPLUSPLUS) FetchQuery();
	else if (fMethod == STRAIGHTC) FetchC();
	else if (fMethod == LIVE) FetchLive();

	//if we have changed the items we need to
	//manually invalidate the list view
	fItemList->Invalidate();
	
	//enable the window updates
	EnableUpdates();
}

void 
QueryWindow::FetchC()
{
	//gather the information from the views
	char query_string[1024];

	if (fFileType == FILES_TYPE) {
		sprintf(query_string, "((BEOS:TYPE!=\"application/x-vnd.Be-directory\")"
			"&&(name==\"*%s*\"))", fNameString->Text());
	} else if (fFileType == FOLDERS_TYPE) {
		sprintf(query_string, "((BEOS:TYPE==\"application/x-vnd.Be-directory\")"
			"&&(name==\"*%s*\"))", fNameString->Text());
	} else if (fFileType == SOURCE_TYPE) {
		sprintf(query_string, "((BEOS:TYPE==\"text/x-source-code\")"
			"&&(name==\"*%s*\"))", fNameString->Text());
	}
	
	fQueryString->SetText(query_string);
//	printf("%s\n", query_string);	

	//find the device id of our application
	app_info info;
	be_app->GetAppInfo(&info);
	//info.ref.device is our device id		
	
	DIR *query;
	dirent *match;
	
	query = fs_open_query(info.ref.device, query_string, 0);
	if (query) {
		while((match = fs_read_query(query))) {
			//create an entry ref from the dirent
			entry_ref ref;
			ref.device = match->d_pdev;
			ref.directory = match->d_pino;
			ref.set_name(match->d_name);
			
			BEntry entry(&ref);
			if (entry.InitCheck() == B_OK) {
				BPath path;
				entry.GetPath(&path);
				if (path.InitCheck() == B_OK) {
				//	printf("item: %s\n", path.Path());
					BStringItem *item = new BStringItem(path.Path());
					fItemList->AddItem(item);
				}
			}
		}
		fs_close_query(query);
	}
}

void 
QueryWindow::FetchQuery()
{
	BQuery fQuery;
	
	//gather all of the information from the views
	char *type = NULL;
	fQuery.PushAttr("BEOS:TYPE");
	if (fFileType == SOURCE_TYPE) type = "text/x-source-code";
	else type = "application/x-vnd.Be-directory";
	fQuery.PushString(type);
	if (fFileType == FILES_TYPE) fQuery.PushOp(B_NE);
	else fQuery.PushOp(B_EQ);
	
	fQuery.PushAttr("name");
	fQuery.PushString(fNameString->Text());
	fQuery.PushOp(B_CONTAINS);
	fQuery.PushOp(B_AND);	
	
	size_t len = fQuery.PredicateLength();
	char *string = (char *) malloc(len + 1);
	fQuery.GetPredicate(string, len + 1);
	fQueryString->SetText(string);
	free(string);

	/* use this application's volume */
	app_info info;
	be_app->GetAppInfo(&info);
	//use the ref of the application binary
	//use the device from this ref to create a volume
	BVolume vol(info.ref.device);
	fQuery.SetVolume(&vol);
	
	//fetch the query
	if (fQuery.Fetch() != B_OK)
		return;
	
	//Iterate through the entries
	BEntry entry;
	BPath path;
	while(fQuery.GetNextEntry(&entry) != B_ENTRY_NOT_FOUND) {
		if (entry.InitCheck() == B_OK) {
			entry.GetPath(&path);
			if (path.InitCheck() == B_OK) {
			//	printf("item: %s\n", path.Path());
				BStringItem *item = new BStringItem(path.Path());
				fItemList->AddItem(item);
			}
		}
	}
}

void 
QueryWindow::FetchLive()
{
}


void 
QueryWindow::ClearResults()
{
	BListItem *item = NULL;
	while ((item = fItemList->RemoveItem((int32)0))) {
		delete item;
	}
	fQueryString->SetText("");
	fItemList->Invalidate();
}

