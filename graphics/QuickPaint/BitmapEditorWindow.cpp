/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include <stdio.h>

#include <Path.h>
#include <FilePanel.h>
#include <TranslationKit.h>
#include "BitmapDocument.h"
#include "PaintApp.h"
#include "BitmapEditor.h"
#include "BitmapEditorWindow.h"
#include "Matrix.h"
#include "Layers.h"
#include "Scroller.h"

BBitmapEditorWindow::BBitmapEditorWindow(BRect r, BBitmapDocument *bitmap)
	: BWindow(r,"BitmapEditorWindow",B_DOCUMENT_WINDOW,0)
{
	if (bitmap==NULL) {
		bitmap = new BBitmapDocument(64,64);
	};
		
	r = Bounds();
	r.bottom = r.top+15;
	BMenuItem *addImage;
	m_menu = new BMenuBar(r,"MenuBar");

	BMenu *submenu = new BMenu("Bitmap");
	BMenuItem *item;
	submenu->AddItem(item = new BMenuItem(	"New",new BMessage(bmsgNewBitmap),
											'N',B_COMMAND_KEY));
	submenu->AddSeparatorItem();
	submenu->AddItem(item = new BMenuItem(	"Open",new BMessage(bmsgOpenBitmap),
											'O',B_COMMAND_KEY));
	submenu->AddSeparatorItem();
	submenu->AddItem(item = new BMenuItem(	"Save",new BMessage(bmsgSave),
											'S',B_COMMAND_KEY));
	submenu->AddItem(item = new BMenuItem(	"Save as...",new BMessage(bmsgSaveAs),
											'S',B_COMMAND_KEY|B_SHIFT_KEY));
	submenu->AddSeparatorItem();
	submenu->AddItem(addImage=item = new BMenuItem(	"Add image",new BMessage(bmsgLoadImage),
											'I',B_COMMAND_KEY));

	m_menu->AddItem(submenu);

	submenu = new BMenu("View");
	submenu->AddItem(item = new BMenuItem(	"Open new view",new BMessage(bmsgOpenView),
											'V',B_COMMAND_KEY));
	submenu->AddSeparatorItem();
	submenu->AddItem(item = new BMenuItem(	"Zoom In",new BMessage(bmsgZoomIn),
											'+',B_COMMAND_KEY));
	submenu->AddItem(item = new BMenuItem(	"Zoom Out",new BMessage(bmsgZoomOut),
											'-',B_COMMAND_KEY));
	m_menu->AddItem(submenu);

	submenu = new BMenu("Layers");
	submenu->AddItem(item = new BMenuItem(	"Add new layer",new BMessage(bmsgAddLayer),
											'L',B_COMMAND_KEY));
	submenu->AddItem(item = new BMenuItem(	"Remove current layer",new BMessage(bmsgRemoveLayer),
											'R',B_COMMAND_KEY));
	submenu->AddSeparatorItem();
	submenu->AddItem(item = new BMenuItem(	"Show current layer",new BMessage(bmsgShowLayer),
											0,B_COMMAND_KEY));
	submenu->AddItem(item = new BMenuItem(	"Hide current layer",new BMessage(bmsgHideLayer),
											0,B_COMMAND_KEY));
	m_menu->AddItem(submenu);

	m_menu->ResizeToPreferred();
	AddChild(m_menu);

	r.Set(0,0,48-1,48*2-1);
	m_layers = new BLayerView(r,48,48,bitmap,NULL);
	BView *v = new BScroller("layerContainer",m_layers);
	v->SetViewColor(190,190,190);
	AddChild(m_layerContainer=v);

	r = Bounds();
	r.top = m_menu->Frame().bottom+1;
	m_layerContainer->MoveTo(0,m_menu->Frame().bottom+1);
	m_layerContainer->ResizeTo(m_layerContainer->Bounds().Width(),r.Height());

	r = Bounds();
	r.OffsetTo(B_ORIGIN);
	r.top = m_menu->Frame().bottom+1;
	r.left = m_layerContainer->Frame().right+1;
	r.bottom -= 14;
	r.right -= 14;
	m_editor = new BBitmapEditor(r,B_FOLLOW_ALL,B_WILL_DRAW|B_FRAME_EVENTS,bitmap);
		
	AddChild(m_scrollView = new BScrollView(	"BitmapScroller",m_editor,B_FOLLOW_ALL,
												0,TRUE,TRUE,B_NO_BORDER));

	m_layers->SetHandler(m_editor);
	addImage->SetTarget(m_editor);

	RedoSizes();
	ResizeToMax();
};

void BBitmapEditorWindow::RedoSizes()
{
	float sizeX,sizeY;
	
	m_editor->GetMaxSize(&sizeX,&sizeY);

	float minX,maxX,minY,maxY;
	minX = 14+m_layerContainer->Frame().right+1+64;
	minY = 14+82;
	maxX = sizeX+14+m_layerContainer->Frame().right+1;
	maxY = sizeY+14+m_menu->Frame().bottom+1;
	maxX = min(maxX,1000);
	maxY = min(maxY,1000);
	SetSizeLimits(minX,maxX,minY,maxY);
};

void BBitmapEditorWindow::ResizeToMax()
{
	float sizeX,sizeY;
	m_editor->GetMaxSize(&sizeX,&sizeY);
	sizeX += 14+m_layerContainer->Frame().right+1;
	sizeY += 14+m_menu->Frame().bottom+1;
	ResizeTo(sizeX,sizeY);
};

void BBitmapEditorWindow::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case bmsgZoomIn:
		case bmsgZoomOut:
			{
				float ratioX,ratioY,sizeX,sizeY,newScale;
				BRect r = m_editor->Bounds();
				m_editor->GetMaxSize(&sizeX,&sizeY);
				ratioX = r.Width() / sizeX;
				ratioY = r.Height() / sizeY;
				newScale = m_editor->Scale();
				if (msg->what == bmsgZoomIn)
					newScale *= 1.05;
				else
					newScale /= 1.05;
				m_editor->SetScale(newScale);
				RedoSizes();
				m_editor->GetMaxSize(&sizeX,&sizeY);
				sizeX = floor(ratioX*sizeX + 0.5);
				sizeY = floor(ratioY*sizeY + 0.5);
				ResizeBy(sizeX-r.Width(),sizeY-r.Height());
			}
			break;
		case bmsgOpenView:
			{
				BRect r = Frame();
				r.OffsetBy(15,15);
				BWindow *w = new BBitmapEditorWindow(r,m_editor->Document());
				w->Show();
			}
			break;
		case bmsgOpenBitmap:
		case bmsgSaveAs:
		case bmsgSave:
			{
				// An exercise for the reader...
			}
			break;
		case bmsgAddLayer:
			m_layers->AddLayer();
			break;
		case bmsgRemoveLayer:
			m_layers->DeleteLayer();
			break;
		case bmsgShowLayer:
			m_layers->ShowLayer(m_layers->Selection());
			break;
		case bmsgHideLayer:
			m_layers->HideLayer(m_layers->Selection());
			break;
		default:
			BWindow::MessageReceived(msg);
	};
};

bool BBitmapEditorWindow::QuitRequested()
{
	be_app->PostMessage(B_QUIT_REQUESTED);
	return TRUE;
};

