/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef _pendlg_h
#define _pendlg_h

#include <TextControl.h>
#include <Window.h>

/////////////////////////////////////////////////////////////////////////////
// Class: PenWidthsDlg
// -------------------
// Implements the Pen Widths dialog. To use it, create the object, run
// DoModal, get the data, and then post the message B_QUIT_REQUESTED to
// the dialog to destroy it.
//
// MFC NOTE: There's no CDialog equivalent in the BeOS API. This
// is an example of how one could be written, with trimmed data
// exchange and validation procedures. This class also demonstrates
// how to get a window to behave in a modal manner.  

class PenWidthsDlg : public BWindow
{
// construction, destruction, operators
public:
	PenWidthsDlg();
	
// overrides
public:
	void MessageReceived(BMessage* message);
	
// operations
public:
	uint32 DoModal();
	
// message handlers
private:
	void OnDefaultPenWidths();
	void EndDialog(uint32 result);

// implementation
private:
	void UpdateData(bool fromControls);
	bool ValidateData();

	status_t GetTCFloatValue(BTextControl* pCtrl, float* dest);
	status_t SetTCFloatValue(BTextControl* pCtrl, float src);
	bool	ValidateTCFloat(BTextControl* pCtrl, float low, float high);	
	
// data members
public:
	float	m_fThinSize;
	float	m_fThickSize;
private:
	BTextControl*	m_pThinCtrl;	// associated with m_fThinSize
	BTextControl*	m_pThickCtrl;	// associated with m_fThickSize
	uint32			m_nExitStatus;
	sem_id			m_modalSem;
};

#endif /* _pendlg_h */
