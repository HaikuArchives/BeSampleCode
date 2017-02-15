/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include <string.h>
#include <TranslationKit.h>
#include "PaintApp.h"
#include "BitmapDocument.h"
#include "BitmapEditor.h"
#include "BitmapEditorWindow.h"
#include "Toolbox.h"
#include "ToolLib.h"

BToolbox *		gToolbox;
BColorControl *	gColorSelector;

PaintApp::PaintApp() : BApplication("application/x-vnd.Be-QuickPaint")
{
	BBitmapDocument *bm = new BBitmapDocument(320,240);

	BRect r(80,25,263,263);
	BWindow *w = new BBitmapEditorWindow(r,bm);
	w->Show();

	r.Set(200,40,400,540);
	BToolboxWindow *tb = new BToolboxWindow(r);
	gToolbox = tb->Toolbox();
	gColorSelector = tb->ColorSelector();
	tb->Show();

	gToolbox->AddTool(new BPenTool());
	gToolbox->AddTool(new BRectTool(true));
	gToolbox->AddTool(new BRectTool(false));
	gToolbox->AddTool(new BEllipseTool(true));
	gToolbox->AddTool(new BEllipseTool(false));
	gToolbox->AddTool(new BHandTool());
	gToolbox->AddTool(new BAlphaTool());
};

PaintApp::~PaintApp()
{
};

bool PaintApp::CanDoFormat(const char *mimeType)
{
	if (strcmp(mimeType,"image/x-be-bitmap") == 0) return true;

	BTranslatorRoster *trans = BTranslatorRoster::Default();
	translator_id * allTranslators = NULL;
	bool r = false;
	int32 fmtCount,count = 0;
	status_t err = trans->GetAllTranslators(&allTranslators, &count);
	if (err >= B_OK) {
		err = B_ERROR;
		for (int32 ix=0;ix<count;ix++) {
			const translation_format * outFormats;
			if (trans->GetOutputFormats(allTranslators[ix],
					&outFormats, &fmtCount) == B_OK) {
				for (int i=0; i<fmtCount; i++) {
					if (strcmp(mimeType,outFormats[i].MIME) == 0) {
						r = true;
						goto out;
					};
				}
			}
		}
	}
	out:
	delete[] allTranslators;
	return r;
};

void PaintApp::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case B_SIMPLE_DATA:
		case B_MIME_DATA:
			for (int32 i=0;i<CountWindows();i++) {
				BWindow *w = WindowAt(i);
				BBitmapEditorWindow *bew =
					dynamic_cast<BBitmapEditorWindow*>(w);
				if (bew) {
					bew->PostMessage(msg,bew->Editor());
					return;
				};
			};
			break;
		default:
			BApplication::MessageReceived(msg);
	};
}

int main()
{
	PaintApp *p = new PaintApp();
	p->Run();
	delete p;
};
