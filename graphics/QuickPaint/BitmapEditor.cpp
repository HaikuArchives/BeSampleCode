/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include <string.h>
#include <StorageKit.h>
#include <TranslationKit.h>
#include "PaintApp.h"
#include "BitmapDocument.h"
#include "BitmapEditor.h"
#include "Toolbox.h"

BBitmapEditor::BBitmapEditor(BRect rect, ulong resizeMode,
	ulong flags, BBitmapDocument *bitmap)
	: BBitmapView(rect,resizeMode,flags,bitmap)
{
	m_currentLayer = 0;
	m_filePanel = NULL;
};

void BBitmapEditor::SetCurrentLayer(int32 layer)
{
	m_currentLayer = layer;
};

void BBitmapEditor::RefReceived(entry_ref *ref)
{
	BEntry entry(ref,true);
	BPath path;
	entry.GetPath(&path);
	BBitmap *bmp = BTranslationUtils::GetBitmapFile(path.Path());
	if (bmp) {
		Document()->AddLayer(-1,bmp);
		delete bmp;
	} else {
		BAlert *a = new BAlert("Unknown file type","QuickPaint, with all its "
			"amazing capabilities, does not know how to handle this file.","Oh.");
		a->Go();
	};
}

void BBitmapEditor::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case bmsgLoadImage:
			m_filePanel = new BFilePanel(B_OPEN_PANEL,new BMessenger(this),NULL,0,true,
				new BMessage(bmsgLoadImageCallback));
			m_filePanel->Show();
			break;
		case bmsgLoadImageCallback:
		{
			entry_ref ref;
			delete m_filePanel;
			msg->FindRef("refs",&ref);
			RefReceived(&ref);
			break;
		}
		case B_SIMPLE_DATA:
		{
			entry_ref ref;
			const char *s;
			int32 i=0,action;
			if (msg->FindRef("refs",&ref) == B_OK) {
				RefReceived(&ref);
				return;
			};
			while (	(msg->FindInt32("be:actions",i,&action)==B_OK) &&
					(action!=B_COPY_TARGET)) i++;
			if (action != B_COPY_TARGET) return;
			i = 0;
			while (	(msg->FindString("be:types",i,&s)==B_OK) &&
					!PaintApp::CanDoFormat(s)) i++;
			if (!s) return;
			msg->PrintToStream();
			
			BMessage reply(B_COPY_TARGET);
			reply.AddString("be:types",s);
			msg->SendReply(&reply,this);
			break;
		}
		case B_MIME_DATA:
		{
			msg->PrintToStream();
			int32 numBytes = 0;
			type_code type;
			const void *data;
			const char *s;
			if (msg->GetInfo(B_MIME_TYPE,0,(char**)&s,&type,&numBytes) != B_OK) return;
			msg->FindData(s,B_MIME_TYPE,&data,&numBytes);
			BMemoryIO inStream(data,numBytes);
			BBitmapStream outStream;
			if (strcmp(s,"image/x-be-bitmap") == 0) {
				uint8 buffer[1024];
				int32 size;
				while ((size=inStream.Read(buffer,1024)) > 0)
					outStream.Write(buffer,size);
				goto success;
			} else if (BTranslatorRoster::Default()->Translate(
				&inStream,NULL,NULL,&outStream,B_TRANSLATOR_BITMAP,
				0,s) == B_OK) {
				success:
				BBitmap *bmp;
				outStream.DetachBitmap(&bmp);
				Document()->AddLayer(-1,bmp);
				delete bmp;
			};
		}
		case bmsgCurrentLayer:
			SetCurrentLayer(msg->FindInt32("layer"));
			break;
		default:
			BBitmapView::MessageReceived(msg);
	};
};

BBitmapEditor::~BBitmapEditor()
{
};

extern BLooper *currentTool;

void BBitmapEditor::MouseDown(BPoint point)
{
	BMessage msg(bmsgToolInvocation);
	rgb_color white = {255,255,255,255};
	rgb_color rgb = gColorSelector->ValueAsColor();
	rgb.alpha = 255;
	BRect r(0,0,0,0);
	GetMaxSize(&r.right,&r.bottom);
	msg.AddRect("region",r);
	msg.AddPoint("point",point);
	msg.AddPointer("client",this);
	msg.AddData("fore",B_RGB_COLOR_TYPE,&rgb,sizeof(rgb_color));
	msg.AddData("back",B_RGB_COLOR_TYPE,&white,sizeof(rgb_color));
	msg.AddPoint("ulhc",B_ORIGIN);
	msg.AddFloat("scale",m_scale);
	msg.AddPointer("bitmap",m_bitmap);
	msg.AddInt32("button",Window()->CurrentMessage()->FindInt32("buttons"));
	msg.AddInt32("layer",m_currentLayer);
	gToolbox->InvokeTool(&msg);
	m_trackingState = 1;
};

void BBitmapEditor::MouseMoved(BPoint /* point */, uint32 /* transit */, const BMessage * /* message */)
{
	if (m_trackingState)
		gToolbox->InvokeTool(Window()->CurrentMessage());
};

void BBitmapEditor::MouseUp(BPoint /* point */)
{
	if (m_trackingState)
		gToolbox->InvokeTool(Window()->CurrentMessage());
	m_trackingState = 0;
};
