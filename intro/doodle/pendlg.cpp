/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include <stdio.h>
#include <string.h>
#include <Alert.h>
#include <Box.h>
#include <Button.h>
#include <Screen.h>
#include <TextControl.h>
#include <View.h>

#include "constants.h"
#include "pendlg.h"
#include "syncutil.h"

/////////////////////////////////////////////////////////////////////////////
// PenWidthsDlg construction, destruction, operators

// PenWidthsDlg::PenWidthsDlg
// --------------------------
// Constructs the pen widths dialog with the associated parent.
//
// MFC NOTE: There's no standard way to edit dialogs visually in
// the BeOS. For the purposes of this example, I hard-code the
// dialog's construction instead of loading it from a resource.
PenWidthsDlg::PenWidthsDlg()
	: BWindow(BRect(0,0,250,115), "Pen Widths", B_MODAL_WINDOW,
	B_NOT_RESIZABLE),
	m_fThinSize(0), m_fThickSize(0),
	m_pThinCtrl(0), m_pThickCtrl(0), m_nExitStatus(0), m_modalSem(-1) 
{
	// set background color
	BView* pBkg = new BView(Bounds(), "Background", B_FOLLOW_ALL,
		B_WILL_DRAW);
	pBkg->SetViewColor(BKG_GREY);
	
	// create buttons
	BButton* pButton = new BButton(BRect(170, 10, 240, 10),
		"OK button", "OK", new BMessage(DLG_OK));
	pBkg->AddChild(pButton);
	pButton->MakeDefault(true);
	pButton = new BButton(BRect(170, 45, 240, 45),
		"Cancel button", "Cancel", new BMessage(DLG_CANCEL));
	pBkg->AddChild(pButton);
	pButton = new BButton(BRect(170, 80, 240, 80),
		"Default button", "Default", new BMessage(DLG_PWD_DEFAULT));
	pBkg->AddChild(pButton);
	
	// add box and text controls
	BBox* pBox = new BBox(BRect(10, 10, 160, 105), "Text Width");
	pBox->SetLabel("Pen Widths");
	pBkg->AddChild(pBox);
	m_pThinCtrl = new BTextControl(BRect(10, 20, 140, 20),
		"ThinTC", "Thin", 0, 0);
	m_pThinCtrl->SetDivider(45);
	
	pBox->AddChild(m_pThinCtrl);
	m_pThickCtrl = new BTextControl(BRect(10, 55, 140, 55),
		"ThickTC", "Thick", 0, 0);
	m_pThickCtrl->SetDivider(45);
	pBox->AddChild(m_pThickCtrl);
	AddChild(pBkg);
	
	// center window on screen
	BScreen screen(this);
	BRect screenFrame = screen.Frame();
	BRect windowFrame = Frame();
	BPoint movePt(screenFrame.left, screenFrame.top);
	movePt.x += floor((screenFrame.Width() - windowFrame.Width()) / 2);
	movePt.y += floor((screenFrame.Height() - windowFrame.Height()) / 2);
	MoveTo(movePt);	
}



/////////////////////////////////////////////////////////////////////////////
// PenWidthsDlg overrides

// PenWidthsDlg::MessageReceived
// -----------------------------
// The message dispatch routine for the dialog.
//
// MFC NOTE: Analagous to MFC's message map (but without the macros to
// hide the details, or VC++ to automatically generate the maps).
void PenWidthsDlg::MessageReceived(BMessage* message)
{
	switch (message->what) {
	case DLG_OK:
		if (ValidateData()) {
			EndDialog(DLG_OK);
		}
		break;
	case DLG_CANCEL:
		EndDialog(DLG_CANCEL);
		break;
	case DLG_PWD_DEFAULT:
		OnDefaultPenWidths();
		break;
	}
}

/////////////////////////////////////////////////////////////////////////////
// PenWidthsDlg operations

// PenWidthsDlg::DoModal
// ---------------------
// Runs the dialog modally. Returns the value of the button which closed
// the dialog (DLG_OK or DLG_CANCEL).
//
// MFC NOTE: Designed to be analagous to CDialog::DoModal.
uint32 PenWidthsDlg::DoModal()
{
	m_modalSem = create_sem(0, "ModalSem");

	// MFC NOTE: for further compatibility, we could put an
	// InitDialog() hook here
		
	UpdateData(false);
	Show();
	
	// MFC NOTE: WaitForDelete is different from CWnd::RunModalLoop
	// because it doesn't process messages.
	m_nExitStatus = WaitForDelete(m_modalSem);
	return m_nExitStatus;		
}



/////////////////////////////////////////////////////////////////////////////
// PenWidthsDlg message handlers

void PenWidthsDlg::OnDefaultPenWidths() 
{
	m_fThinSize = DEFAULT_THIN_SIZE;
	m_fThickSize = DEFAULT_THICK_SIZE;
	UpdateData(false);
}

// PenWidthsDlg::EndDialog
// -----------------------
// Finish the dialog and signal DoModal to continue.
void PenWidthsDlg::EndDialog(uint32 result)
{
	UpdateData(true);
	m_nExitStatus = result;
	Hide();
	delete_sem(m_modalSem); // causes DoModal to unblock
}



/////////////////////////////////////////////////////////////////////////////
// PenWidthsDlg implementation

// PenWidthsDlg::UpdateData
// ------------------------
// if fromControls is true, load the data members from the dialog controls.
// if fromControls is false, load the dialog controls from the data members.
//
// MFC NOTE: look familiar?
void PenWidthsDlg::UpdateData(bool fromControls)
{
	if (fromControls) {
		GetTCFloatValue(m_pThinCtrl, &m_fThinSize);
		GetTCFloatValue(m_pThickCtrl, &m_fThickSize);
	} else {
		SetTCFloatValue(m_pThinCtrl, m_fThinSize);
		SetTCFloatValue(m_pThickCtrl, m_fThickSize);
	}
}

// PenWidthsDlg::ValidateData
// --------------------------
// Makes sure the values entered into the dialog controls are correct.
//
// MFC NOTE: in MFC, you'd specify these ranges using DoDataExchange,
// and the actual validation would happen behind the scene. This is
// a less generic way of doing this.
bool PenWidthsDlg::ValidateData()
{
	// the text controls should contain floats between 1 and 20
	if (! ValidateTCFloat(m_pThinCtrl, 1, 20)) {
		return false;
	} else if (! ValidateTCFloat(m_pThickCtrl, 1, 20)) {
		return false;
	}
	
	return true;
}

// PenWidthsDlg::GetTCFloatValue
// -----------------------------
// Converts the control's text to a float and stuffs the float
// into dest. Returns B_OK if successful, or B_BAD_VALUE if the
// control's text couldn't be translated.
status_t PenWidthsDlg::GetTCFloatValue(BTextControl* pCtrl, float* dest)
{
	const char* buffer = pCtrl->Text();
	char guard[2];
	float bufFloat;
	int res = sscanf(buffer, "%g%1s", &bufFloat, guard);
	if (res != 1) {
		return B_BAD_VALUE;
	} else {
		*dest = bufFloat;
		return B_OK;
	}
}

// PenWidthsDlg::SetTCFloatValue
// -----------------------------
// Sets the control's text to represent src. Returns B_OK if successful.
status_t PenWidthsDlg::SetTCFloatValue(BTextControl* pCtrl, float src)
{
	char buffer[50];
	sprintf(buffer, "%g", src);
	pCtrl->SetText(buffer);
	return B_OK;
}

// PenWidthsDlg::ValidateTCFloat
// -----------------------------
// Checks to see if the control's value is a float between the
// specified minimum and maximum numbers (inclusive). If not, it
// notifies the user. Returns true iff the control's value is
// valid.
bool PenWidthsDlg::ValidateTCFloat(BTextControl* pCtrl, float low, float high)
{
	bool res;
	float theFloat;
	if (GetTCFloatValue(pCtrl, &theFloat) != B_OK) {
		res = false;
	} else if (theFloat < low || theFloat > high) {
		res = false;
	} else {
		res = true;
	}
	
	if (! res) {
		const char* ctrlLabel = pCtrl->Label();
		char* msg = new char[strlen(ctrlLabel) + 100];
		sprintf(msg, "For \"%s\", please enter a floating-point number "
			"between %g and %g.",
			ctrlLabel, low, high);
		BAlert* pAlert = new BAlert("Validation error",
			msg, "OK");
		pAlert->Go();
		delete [] msg;
	}

	return res;
}
