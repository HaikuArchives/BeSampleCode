/* DDWindow.cpp */
/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/
#include <Application.h>
#include <Menu.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <Alert.h>
#include <PrintJob.h>
#include "DDWindow.h"
#include "MsgVals.h"

const BRect rect(50,50,600,600);

/*-------------------------------------------------------------------------*/

DDWindow::DDWindow()
	: BWindow(rect, "DynaDraw!", B_TITLED_WINDOW, 0)
{
	printSettings = 0;
	
	/* Create a menubar, and the two submenus... */
	BMenuBar* mb = new BMenuBar(BRect(0,0,rect.right,15), "menubar");

	/* Menu one */
	BMenu* menu = new BMenu("File");
	menu->AddItem(new BMenuItem("About", new BMessage(B_ABOUT_REQUESTED)));
	menu->AddItem(new BMenuItem("Quit", new BMessage(B_QUIT_REQUESTED)));

	/* Both the About and Quit messages should be directed to be_app */
	menu->SetTargetForItems(be_app);
	mb->AddItem(menu);
	
	/* Menu two */
	menu = new BMenu("Controls");
	BMenuItem* tmpItem = new BMenuItem("Tweakables", new BMessage(TWEAK_REQ));
	tmpItem->SetTarget(be_app);
	menu->AddItem(tmpItem);
	
	tmpItem = new BMenuItem("Color", new BMessage(COLOR_REQ));
	tmpItem->SetTarget(be_app);
	menu->AddItem(tmpItem);
	
	menu->AddSeparatorItem();
	menu->AddItem(new BMenuItem("Clear Screen", new BMessage(CLEAR_SCREEN)));
	/* Target not specified for Clear, so the target will be DDWindow */
	mb->AddItem(menu);
	
	/* Menu three */
	menu = new BMenu("Printing");
	menu->AddItem(scaleItem = new BMenuItem("Scale To Page", new BMessage(SCALE_PAGE)));
	menu->AddItem(new BMenuItem("Print", new BMessage(PRINT_REQ)));
	scaleItem->SetMarked(true);
	/* Target for these items is DDWindow */
	mb->AddItem(menu);
	
	AddChild(mb);
		
	/* create the filter view and attach it to the window */
	float mb_height = mb->Bounds().Height();
	fv = new FilterView(BRect(0,mb_height,rect.right, 
							rect.bottom - mb_height));
	AddChild(fv);
	
	/* show the window */
	Show();
	
}
/*-------------------------------------------------------------------------*/

void
DDWindow::MessageReceived(BMessage* msg)
{
	switch(msg->what)
	{
	 case COLOR_CHG:
	 case MASS_CHG:
	 case DRAG_CHG:
	 case WIDTH_CHG:
	 case FILL_CHG:
	 case ANGLE_CHG:
	 case SLEEPAGE_CHG:
	 case CLEAR_SCREEN:
	 	PostMessage(msg, fv);
		break;	
		
	 /* printing messages */	
	 case PRINT_REQ:
	 {
		DoPrint();
		break;
	 }
	 case SCALE_PAGE:
	 {
	 	bool tmp = !scaleItem->IsMarked();
	 	scaleItem->SetMarked(tmp);
	 	fv->SetScaleToPage(tmp);
	 	break;
	 }
	 default:
	 {
		/* always pass on messages we don't understand */
	 	BWindow::MessageReceived(msg);
	 	break;
	 }
	}
}
/*-------------------------------------------------------------------------*/

void
DDWindow::DoPrint()
{
	BPrintJob job("dynadraw");

	/* configure the page */
	if(SetUpPage() != B_NO_ERROR) 
	{
		(new BAlert("Cancel", "Print cancelled.", "OK"))->Go();
		return;
	}
	job.SetSettings(new BMessage(*printSettings));

	/* uncomment the following lines to see the contents */
	/* of the printSettings message. */
	// printf("printSettings message:\n"
	// printSettings->PrintToStream();

	/* get the printable area we have to work with */
	BRect pageRect = job.PrintableRect();
	BRect curPageRect = pageRect;
	
	/* get the width & height of the drawn view */
	int vWidth, vHeight;
	fv->GetExtremities(vWidth, vHeight);

	/* width & height of our printable page */	
	int pHeight = (int)pageRect.Height();
	int pWidth = (int)pageRect.Width();

	/* determine the number of horizontal and vertical pages */
	/* the total number of pages is xPages * yPages          */
	int xPages = 1, yPages = 1;
	if(!scaleItem->IsMarked())
	{
		xPages = (int)ceil((float)vWidth/(float)pWidth);
		yPages = (int)ceil((float)vHeight/(float)pHeight);
	}
	
	/* engage the print server */
	job.BeginJob();

	/* loop through and draw each page, and write to spool */
	for(int x=0; x < xPages; x++)
		for(int y=0; y < yPages; y++)
		{
			curPageRect.OffsetTo(x*pWidth, y*pHeight);
			job.DrawView(fv, curPageRect, BPoint(0.0,0.0));
			job.SpoolPage();
			
			if(!job.CanContinue())
			{
				(new BAlert("Cancel", "Print job cancelled", "OK"))->Go();
				return;
			}
		}
	
	/* commit the job, send the spool file */
	job.CommitJob();
	
}
/*-------------------------------------------------------------------------*/

status_t
DDWindow::SetUpPage()
{
	status_t rv;
	BPrintJob job("dynadraw");

	/* display the page configure panel */
	rv = job.ConfigPage();

	/* save a pointer to the settings */
	printSettings = job.Settings();

	return rv;
}
/*-------------------------------------------------------------------------*/

bool 
DDWindow::QuitRequested()
{
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}
/*-------------------------------------------------------------------------*/

