/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include <stdio.h>
#include <string.h>

#include <Screen.h>
#include <MenuItem.h>
#include <NodeInfo.h>
#include <TextView.h>
#include <ScrollView.h>
#include <TranslationUtils.h>

#include "main.h"
#include "win.h"
#include "iview.h"

// This is used to ensure that when text wrapping at edge is used, the horizontal scroll bar will be disabled,
//   instead of the full width of the scroll bar.
#define TEXT_WRAP_FUDGE 3

TWin::TWin(BRect frame, const char *filename) : BWindow(frame, APP_NAME, B_DOCUMENT_WINDOW, B_CURRENT_WORKSPACE)
{
	BRect rect(0,0,0,0);
	BMenuItem *item;
	BMenu *sub;
	menu = new BMenuBar(rect, "MenuBar");
	sub = new BMenu("File");

	sub->AddItem(item = new BMenuItem("Open", new BMessage(T_OPEN), 'O'));
	item->SetTarget(be_app);
	sub->AddSeparatorItem();

	sub->AddItem(new BMenuItem("Close", new BMessage(B_CLOSE_REQUESTED),'W'));
	sub->AddItem(item = new BMenuItem("Quit", new BMessage(B_CLOSE_REQUESTED),'Q'));
	item->SetTarget(be_app);
	menu->AddItem(sub);
	WrapAtEdge = false;

	viewtype = VIEWTYPE_NONE;
	status_t err = B_OK;
	if(filename!=NULL){
		path.SetTo(filename, NULL, true);
		SetTitle(path.Leaf());
		BNode *node = new BNode(path.Path());
		if((err = node->InitCheck())!=B_OK){
			fprintf(stderr, "Error occured while opening node.\n");
			fprintf(stderr, "[%s:%d] Error (0x%lx) %s\n",__FILE__,__LINE__,err,strerror(err));
			viewtype = VIEWTYPE_ERROR;
		}
		BNodeInfo *ninfo = new BNodeInfo(node);
		if((err = ninfo->InitCheck())!=B_OK){
			fprintf(stderr, "Error occured while opening node info.\n");
			fprintf(stderr, "[%s:%d] Error (0x%lx) %s\n",__FILE__,__LINE__,err,strerror(err));
			viewtype = VIEWTYPE_ERROR;
		} else {
			char *tstr = new char[B_MIME_TYPE_LENGTH];
			if((err = ninfo->GetType(tstr))!=B_OK){
				fprintf(stderr, "Error occured while getting the file type.\n");
				fprintf(stderr, "[%s:%d] Error (0x%lx) %s\n",__FILE__,__LINE__,err,strerror(err));
				viewtype = VIEWTYPE_ERROR;
			}else if(!strncmp(tstr,"image/",6)){
				viewtype = VIEWTYPE_IMAGE;
			}else if(!strncmp(tstr,"text/",5)){
				viewtype = VIEWTYPE_TEXT;
			}else{
				viewtype = VIEWTYPE_UNKNOWN;
				fprintf(stderr, "Found unknown filetype '%s'\n",tstr);
			}
			delete tstr;
		}
		delete ninfo;
		delete node;
	}

	if(viewtype == VIEWTYPE_IMAGE){
		BMessage *msg;
		sub = new BMenu("Image");
		sub->AddItem(item = new BMenuItem(T_FITMENU, msg = new BMessage(T_TOFIT)));
		BMenu *ssub = new BMenu(T_SCALEMENU);
		ssub->SetRadioMode(true);
		ssub->AddItem(item = new BMenuItem("50%", msg = new BMessage(T_SCALE)));
		msg->AddFloat("scale",0.5);
		msg->AddInt32("scale_mode", SCALE_NORMAL);
		ssub->AddItem(item = new BMenuItem("100%", msg = new BMessage(T_SCALE)));
		msg->AddFloat("scale",1.0);
		msg->AddInt32("scale_mode", SCALE_NORMAL);
		ssub->AddItem(item = new BMenuItem("200%", msg = new BMessage(T_SCALE)));
		msg->AddFloat("scale",2.0);
		msg->AddInt32("scale_mode", SCALE_NORMAL);
		ssub->AddSeparatorItem();
		ssub->AddItem(item = new BMenuItem("Max", msg = new BMessage(T_SCALE)));
		msg->AddInt32("scale_mode", SCALE_MAX);
		item->SetMarked(true);
		sub->AddItem(ssub);
		menu->AddItem(sub);
	}else if(viewtype == VIEWTYPE_TEXT){
		sub = new BMenu("Text");
		sub->AddItem(item = new BMenuItem(T_WRAPMENU, new BMessage(T_WRAP)));
		sub->AddItem(item = new BMenuItem(T_EDGEMENU, new BMessage(T_EDGE)));
		item->SetMarked(true);
		item->SetEnabled(false);
		menu->AddItem(sub);
	}
	AddChild(menu);

	rect = menu->Bounds();
	float hi = rect.bottom;
	rect = frame;
	rect.OffsetTo(0,0);
	rect.top += hi+ 1;
	text = NULL;
	image = NULL;
	imageview = NULL;
	switch(viewtype){
	case VIEWTYPE_IMAGE:
		AddScrollingImage(rect);
		Zoom();
		break;
	case VIEWTYPE_TEXT:
		AddScrollingText(rect);
		break;
	case VIEWTYPE_NONE:
		printf("No file type, nothing to do.\n");
		break;
	case VIEWTYPE_ERROR:
		printf("An error occured, don't be surprised nothing happened.\n");
		break;
	case VIEWTYPE_UNKNOWN:
		break;
	}
}

TWin :: ~TWin(void)
{
	if(image){
		delete image;
	}
}

status_t TWin :: AddScrollingImage(BRect frame)
{
	BRect fr = frame; 
	fr.right -= B_V_SCROLL_BAR_WIDTH;
	fr.bottom -= B_H_SCROLL_BAR_HEIGHT;

	printf("Loading Image '%s'\n",path.Path());
	imageview = new TImageView(fr, path.Path(), 0.0, SCALE_MAX);
	AddChild(new BScrollView("ImageScroll", imageview, B_FOLLOW_ALL_SIDES, B_WILL_DRAW, true, true));
	RedoSizes();
	return(B_OK);
}

void TWin :: FrameResized(float w, float h)
{
	if(w!=-1 && h!=-1) BWindow::FrameResized(w,h);
	if(text!=NULL && WrapAtEdge){
		BRect rect;
		if(w==-1 && h==-1){
			rect = Bounds();
			h = rect.bottom;
			w = rect.right;
		}
		rect = text->Frame();
		rect.bottom = rect.top + h;
		rect.right = rect.left + w - B_V_SCROLL_BAR_WIDTH - TEXT_WRAP_FUDGE;
		text->SetTextRect(rect);
	}
}

status_t TWin :: AddScrollingText(BRect frame)
{
	BRect trect(0,0,0,0); 

	BRect fr = frame; 
	fr.right -= B_V_SCROLL_BAR_WIDTH ;
	fr.bottom -= B_H_SCROLL_BAR_HEIGHT ;

	text = new BTextView(fr, "textView", trect, B_FOLLOW_ALL_SIDES,
		B_WILL_DRAW | B_NAVIGABLE);
	text->SetStylable(false);

	bool wrappy = false;
	BMenuItem *item;
	if((item = menu->FindItem(T_WRAPMENU)) != NULL){
		wrappy = item->IsMarked();
	}
	text->MakeEditable(false);
	text->SetWordWrap(wrappy);

	AddChild(new BScrollView("TextScroll", text, B_FOLLOW_ALL_SIDES, 0, true, true));
	BFile file(path.Path(), B_READ_ONLY);
	off_t fsize;
	if(!file.GetSize(&fsize)){
		printf("Loading Text '%s'\n",path.Path());
		text->SetText(&file, 0, fsize);
	}

	int n = text->CountLines();
	float t;
	filewidth = 1;
	for(int i = 0; i < n; i++){
		t = text->LineWidth(i);
		if(t > filewidth) filewidth = t;
	}
	trect.right = filewidth;
	text->SetTextRect(trect);

	return(B_OK);
}

void TWin :: MessageReceived(BMessage *message)
{
	uint32 type; 
	int32 count;

	switch(message->what){
	case B_SIMPLE_DATA:
		message->GetInfo("refs", &type, &count); 
		if ( count>0 && type == B_REF_TYPE ) { 
			message->what = B_REFS_RECEIVED;
			be_app->PostMessage(message);
		}
		break;

	case T_WRAP: {
		BMenuItem *item = NULL;
		bool wrappy = false;
		if((item = menu->FindItem(T_WRAPMENU)) != NULL){
			wrappy = !item->IsMarked();
		}
		item->SetMarked(wrappy);
		text->SetWordWrap(wrappy);
		if(!wrappy){
			BRect trect(0, 0, filewidth, 0);
			text->SetTextRect(trect);
		}
		WrapAtEdge = false;
		item = menu->FindItem(T_EDGEMENU);
		if(!wrappy){
			item->SetEnabled(false);
		}else{
			item->SetEnabled(true);
			if((item = menu->FindItem(T_EDGEMENU)) != NULL){
				wrappy = item->IsMarked();
			}
			if(wrappy){
				WrapAtEdge = true;
			}
		}
		FrameResized(-1, -1);
		break;
	}

	case T_EDGE: {
		BMenuItem *item = NULL;
		WrapAtEdge = false;
		item = menu->FindItem(T_EDGEMENU);
		if(item!=NULL && item->IsEnabled()){
			if((item = menu->FindItem(T_EDGEMENU)) != NULL){
				WrapAtEdge = !item->IsMarked();
			}
			item->SetMarked(WrapAtEdge);
		}
		FrameResized(-1, -1);
		break;
	}

	case T_TOFIT: {
		BMenuItem *item = menu->FindItem(T_FITMENU);
		bool tofit = false;
		if(item!=NULL){
			tofit = !item->IsMarked();
			item->SetMarked(tofit);
			item = menu->FindItem(T_SCALEMENU);
			if(item!=NULL){
				item->SetEnabled(!tofit);
			}
		}
//		BView *v = FindView("imageView");
		float t = 0.0;
		int32 m = tofit?SCALE_TOFIT:SCALE_NORMAL;
		imageview->SetImageScale(t, m);
		RedoSizes();
		break;
	}

	case T_SCALE: {
//		BView *v = FindView("imageView");
		float t = 0.0;
		message->FindFloat("scale",&t);
		int32 m = SCALE_LAST;
		message->FindInt32("scale_mode",&m);
		imageview->SetImageScale(t, m);
		RedoSizes();
		break;
	}

	default:
		BWindow::MessageReceived(message);
	}
}

void TWin::RedoSizes(void)
{
	if(imageview!=NULL){
		BScreen *scr = new BScreen(this);
		BRect screenbounds = scr->Frame();
		delete scr;

		float sizeX,sizeY;
		imageview->GetMaxSize(&sizeX,&sizeY);
	
		float minX,maxX,minY,maxY;
		
		if(sizeX < 0) sizeX = screenbounds.Width();
		if(sizeY < 0) sizeY = screenbounds.Height();
		maxX = sizeX+B_V_SCROLL_BAR_WIDTH;
		maxY = sizeY+B_H_SCROLL_BAR_HEIGHT;
		maxX = min_c(maxX,floor(screenbounds.Width()+0.5));
		maxY = min_c(maxY,floor(screenbounds.Height()+0.5)) + menu->Bounds().bottom+1.;

		minX = min_c(maxX, B_V_SCROLL_BAR_WIDTH + min_length);
		minY = min_c(maxY, B_H_SCROLL_BAR_HEIGHT + menu->Bounds().bottom+1 + min_length);
		SetSizeLimits(minX,maxX,minY,maxY);
		SetZoomLimits(maxX, maxY);

		BRect cur = Bounds();
		if(cur.Width()+1. > maxX || cur.Height()+1. > maxY){
			ResizeTo(maxX,maxY);
		}
	}
}

void TWin::ScreenChanged(BRect frame, color_space mode)
{
	RedoSizes();
}

