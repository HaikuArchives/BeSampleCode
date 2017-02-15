//******************************************************************************
//
//	File:			SendPanel.h
//
//	Written by:		Douglas Wright
//
//******************************************************************************

/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef	_SEND_PANEL_H
#define _SEND_PANEL_H

#include <List.h>
#include <String.h>
#include <Window.h>
#include <View.h>

class SendView;

class SendPanel : public BWindow {

public:
	SendPanel(BMessenger *msgr);
	virtual ~SendPanel();
	
	virtual	void MessageReceived(BMessage *msg);
};


class SpecView;
class BBox;
class BTextControl;
class BCheckBox;
class BMenuField;

class SendView : public BView
{
public:
	SendView(BRect rect);
	virtual ~SendView();

	virtual	void AttachedToWindow();
	virtual	void MessageReceived(BMessage *msg);
	
	void BuildMessage(BMessage *msg) const;
	void SetMessenger(BMessenger *msgr);

private:
	
	void AddMessageTypesToMenu();
	
	void AddSpecView();
	void RemoveSpecView();
	void DoSendMessage();

	status_t GetTCInt32(const BTextControl* pCtrl, int32* dest) const;
	status_t GetTCUint32(const BTextControl* pCtrl, uint32* dest) const;
	status_t GetTCInt32Range(const BTextControl* pCtrl, int32* begin, int32* end) const;

	void SetUpButtons(BBox *box);

	BList		specViewList;
	BBox 		*fSpecBox;
	BMenu 		*fMessageTypeMenu;
	BMessenger 	*fMessenger;
	BTextView	*fText;

};

class SpecView : public BView
{
public:
	SpecView(BRect);
	virtual ~SpecView();
	
	virtual void AttachedToWindow();
	virtual	void MessageReceived(BMessage *msg);
	BMenuField	*fMenu;
	BTextControl *fText1;
	BTextControl *fText2;
	
};


#endif
