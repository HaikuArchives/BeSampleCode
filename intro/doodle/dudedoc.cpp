/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <Alert.h>
#include <Application.h>
#include <Autolock.h>
#include <File.h>
#include <FilePanel.h>
#include <Menu.h>
#include <MenuItem.h>
#include <Message.h>
#include <Node.h>
#include <NodeInfo.h>
#include "constants.h"
#include "pendlg.h"
#include "dudedoc.h"
#include "dudeview.h"
#include "dudewin.h"
#include "stroke.h"
#include "syncutil.h"

/////////////////////////////////////////////////////////////////////////////
// static variables

static const uint32 MSG_QUIT_NOW = 'mQNW';



/////////////////////////////////////////////////////////////////////////////
// DudeDoc static member definitions

BList DudeDoc::s_docList;
BLocker DudeDoc::s_listLock("DudeDoc static lock");



/////////////////////////////////////////////////////////////////////////////
// DudeDoc static methods

// DudeDoc::CreateDocument()
// -----------------------------
// Creates a brand new document and a new window for display.
//
// MFC NOTE: This would be handled by the document template in an MFC
// application.
status_t DudeDoc::CreateDocument()
{
	if (! s_listLock.Lock())
		return B_ERROR;
			
	status_t res;
	DudeDoc* pDoc = new DudeDoc;	
	AddDocument(pDoc);
	s_listLock.Unlock();
	
	if (! pDoc->WriteLock())
		return B_ERROR;
		
	pDoc->NewName();
	
	DudeWin* pWin = new DudeWin;
	if ((res = pWin->Init(pDoc)) != B_OK) {
		delete pDoc;
		delete pWin;
		return res;
	}
	pDoc->WriteUnlock();
	pDoc->Run();
	
	pWin->Show();
	return B_OK;
}

// DudeDoc::CreateDocument(const entry_ref*)
// ---------------------------------------------
// Opens a document and its associated window(s) from a file.
//
// MFC NOTE: This would be handled by the document template in an MFC
// application.
status_t DudeDoc::CreateDocument(const entry_ref* ref)
{
	BAutolock listLock(s_listLock);
	if (! listLock.IsLocked())
		return B_ERROR;
		
	if (ActivateExistingDocument(ref)) {
		return B_OK;
	}

	status_t res;
	BFile file(ref, B_READ_WRITE);
	if ((res = file.InitCheck()) != B_OK) {
		return res;
	}
	
	// MFC NOTE: We assume the contents of the file are
	// a flattened message which contains our document, and
	// use the archiving mechanism instantiate_object to
	// try to recreate the document.
	BMessage archive;
	if ((res = archive.Unflatten(&file)) != B_OK) {
		return res;
	}
	
	DudeDoc* pDoc = dynamic_cast<DudeDoc*>
		(instantiate_object(&archive));
	
	if (! pDoc) {
		// instantiate_object does not always set errno
		// correctly in R3
		return (errno != B_OK) ? errno : B_ERROR;
	}
	
	AddDocument(pDoc);

	if (! pDoc->WriteLock())
		return B_ERROR;
			
	pDoc->SetRef(ref);
	
	DudeWin* pWin = new DudeWin;
	if ((res = pWin->Init(pDoc)) != B_OK) {
		delete pDoc;
		delete pWin;
		return res;
	}

	pDoc->WriteUnlock();
	pDoc->Run();	
	pWin->Show();

	return B_OK;
}

int32 DudeDoc::CountDocuments()
{
	return s_docList.CountItems();
}

DudeDoc* DudeDoc::DocumentAt(int32 index)
{
	return static_cast<DudeDoc*>(s_docList.ItemAt(index));
}

// DudeDoc::Instantiate
// --------------------
// Creates a new document from an archive.
BArchivable* DudeDoc::Instantiate(BMessage* archive)
{
	if (validate_instantiation(archive, "DudeDoc")) {
		return new DudeDoc(archive);
	} else {
		return 0;
	}	
}



/////////////////////////////////////////////////////////////////////////////
// DudeDoc static implementation

void DudeDoc::AddDocument(DudeDoc* document)
{
	s_docList.AddItem(document);
}

void DudeDoc::RemoveDocument(DudeDoc* document)
{	
	s_docList.RemoveItem(document);
	if (s_docList.CountItems() == 0) {
		// no open documents, quit
		be_app->PostMessage(B_QUIT_REQUESTED);
	}
}

// DudeDoc::ActivateExistingDocument
// ---------------------------------
// If a document could be found which matches the entry_ref,
// it will be activated. Returns true iff a document was
// activated.
bool DudeDoc::ActivateExistingDocument(const entry_ref* ref)
{
	bool ok = false;
	if (! s_listLock.Lock())
		return false;
		
	int32 numDocs = CountDocuments();
	for (int32 i=0; i<numDocs; i++) {
		DudeDoc* pDoc = DocumentAt(i);
		if (*(pDoc->m_pEntryRef) == *ref) {
			pDoc->Activate();
			ok = true;
			break;
		}
	}
	s_listLock.Unlock();
	return ok;
}



/////////////////////////////////////////////////////////////////////////////
// DudeDoc construction, destruction, operators

DudeDoc::DudeDoc()
	: BLooper(),
	m_fThinSize(DEFAULT_THIN_SIZE), m_fThickSize(DEFAULT_THICK_SIZE), 
	m_docBounds(0,0,800,900),
	m_bVirgin(true), m_bModified(false), m_pEntryRef(0), 
	m_pPrintSettings(0), m_pSavePanel(0), m_saveSem(-1),
	m_bWaitForSave(false), m_bCancelSave(false) 
{
	SetPenSize(false);
}

// DudeDoc::DudeDoc(BMessage*)
// ---------------------------
// Constructs a document from a message archive.
//
// MFC NOTE: This is how BArchivable objects are 'unserialized.' This is
// similar to Serialize where CArchive::IsStoring() == false. Compare this
// function to its counterpart, DudeDoc::Archive.
DudeDoc::DudeDoc(BMessage* archive)
	: BLooper(archive),
	m_fThinSize(DEFAULT_THIN_SIZE), m_fThickSize(DEFAULT_THICK_SIZE),
	m_bVirgin(false), m_bModified(false), m_pEntryRef(0),
	m_pPrintSettings(0), m_pSavePanel(0), m_saveSem(-1),
	m_bWaitForSave(false), m_bCancelSave(false)
{
	SetPenSize(false);
	archive->FindRect("bounds", &m_docBounds);
	BMessage* printSettings = new BMessage();
	if (archive->FindMessage("page setup", printSettings) == B_OK) {
		SetPrintSettings(printSettings);	
	} else {
		delete printSettings;
	}
	int32 i=0;
	BMessage strokeArchive;
	while (archive->FindMessage("strokes", i++, &strokeArchive) == B_OK) {
		// MFC NOTE: we use the archiving function instantiate_object
		// to 'unserialize' the stroke.
		PenStroke* pStroke = dynamic_cast<PenStroke*>
			(instantiate_object(&strokeArchive));
		if (pStroke) {
			AddStroke(pStroke);
		}
	}
}

DudeDoc::~DudeDoc()
{
	if (s_listLock.Lock()) {
		RemoveDocument(this);
		s_listLock.Unlock();
	}
	DeleteContents();
}



/////////////////////////////////////////////////////////////////////////////
// DudeDoc overrides

// DudeDoc::Archive
// ----------------
// Saves the state of the document into the archive.
//
// MFC NOTE: This is how BArchivable objects are 'serialized.' This is
// similar to Serialize where CArchive::IsStoring() == true. Compare this
// function to its counterpart, DudeDoc::DudeDoc(BMessage*).
status_t DudeDoc::Archive(BMessage* archive, bool deep) const
{
	status_t res = BLooper::Archive(archive, deep);
	if (res != B_OK) {
		return res;
	}
	archive->AddRect("bounds", m_docBounds);
	
	if (m_pPrintSettings)
		archive->AddMessage("page setup", m_pPrintSettings);
		
	if (deep) {
		int32 len = CountStrokes();
		for (int32 i=0; i<len; i++) {
			PenStroke* pStroke = StrokeAt(i);
			// archive this stroke into a message
			BMessage strokeArchive;
			pStroke->Archive(&strokeArchive);
			archive->AddMessage("strokes", &strokeArchive);
		}
	}
	return B_OK;
}

// DudeDoc::MessageReceived
// ------------------------
// The message dispatch routine for the document.
//
// MFC NOTE: Analagous to MFC's message map (but without the macros to
// hide the details, or VC++ to automatically generate the maps).
void DudeDoc::MessageReceived(BMessage* message)
{
	switch (message->what) {
	case MSG_EDIT_CLEAR_ALL:
		OnEditClearAll();
		break;
	case MSG_PEN_THICK_OR_THIN:
		OnPenThickOrThin();
		break;
	case MSG_PEN_WIDTHS:
		OnPenWidths();
		break;
	case MSG_FILE_SAVE:
		OnSave();
		break;
	case MSG_FILE_SAVE_AS:
		OnSaveAs();
		break;
	case MSG_QUIT_NOW:
		Quit();
		break;
	case B_SAVE_REQUESTED:
		SaveRequested(message);
		break;
	case B_CANCEL:
	{
		// figure out what was cancelled
		int32 val;
		if (message->FindInt32("old_what", &val) == B_OK) {
			uint32 what = static_cast<uint32>(val);
			if (what == B_SAVE_REQUESTED) {
				CancelSave();
			}
		}
		break;
	}
	default:
		BHandler::MessageReceived(message);
		break;
	}
}

// DudeDoc::QuitRequested
// ----------------------
// Returns true iff it's safe to close the document. This function
// will wait for the user to save the document, if necessary, before
// returning.
bool DudeDoc::QuitRequested()
{
	if (! IsModified()) {
		return true;
	} else {
		bool result;
		char msg[B_FILE_NAME_LENGTH + 50];
		sprintf(msg, "Save changes to the document \"%s\"?", m_pEntryRef->name);
		BAlert* pAlert = new BAlert("Save before close", msg,
			"Don't Save", "Cancel", "Save");
		switch (pAlert->Go()) {
		case 0:
			// don't save
			result = true;
			break;
		case 1:
			// cancel operation
			result = false;
			break;
		case 2:
			// save document before closing
			result = SaveBeforeClose();
			break;
		default:
			result = true;
			break;
		}
		return result;
	}
}



/////////////////////////////////////////////////////////////////////////////
// DudeDoc accessors

bool DudeDoc::ReadLock()
{
	return m_dataLock.ReadLock();
}

void DudeDoc::ReadUnlock()
{
	m_dataLock.ReadUnlock();
}

bool DudeDoc::WriteLock()
{
	return m_dataLock.WriteLock();
}

void DudeDoc::WriteUnlock()
{
	m_dataLock.WriteUnlock();
}

bool DudeDoc::IsReadLocked()
{
	return m_dataLock.IsReadLocked();
}

bool DudeDoc::IsWriteLocked()
{
	return m_dataLock.IsWriteLocked();
}

BMessage* DudeDoc::PrintSettings() const
{
	return m_pPrintSettings;
}

void DudeDoc::SetPrintSettings(BMessage* settings)
{
	if (m_pPrintSettings)
		delete m_pPrintSettings;
	
	m_pPrintSettings = settings;
}

void DudeDoc::AddStroke(PenStroke* stroke)
{
	m_strokeList.AddItem(stroke);
}

int32 DudeDoc::CountStrokes() const
{
	return m_strokeList.CountItems();
}

PenStroke* DudeDoc::StrokeAt(int32 index) const
{
	return static_cast<PenStroke*>(m_strokeList.ItemAt(index));
}

PenStroke* DudeDoc::RemoveStroke(int32 index)
{
	return static_cast<PenStroke*>(m_strokeList.RemoveItem(index));
}

void DudeDoc::AddWindow(DudeWin* win)
{
	m_winList.AddItem(win);
}

int32 DudeDoc::CountWindows() const
{
	return m_winList.CountItems();
}

int32 DudeDoc::IndexOfWindow(DudeWin* pWin) const
{
	return m_winList.IndexOf(pWin);
}

DudeWin* DudeDoc::WindowAt(int32 index) const
{
	return static_cast<DudeWin*>(m_winList.ItemAt(index));
}

void DudeDoc::RemoveWindow(DudeWin* win)
{
	m_winList.RemoveItem(win);
	int32 nItems = m_winList.CountItems();
	if (nItems == 0) {
		// no windows left for this document; destroy it!
		PostMessage(MSG_QUIT_NOW);
		return;
	} else if (nItems >= 1) {
		// this function may affect the order of windows;
		// we may need to retitle them
		for (int32 i=0; i<nItems; i++) {
			WindowAt(i)->PostMessage(MSG_DOC_TITLE_CHANGED);
		}
	}
}

void DudeDoc::AddView(DudeView* view)
{
	m_viewList.AddItem(view);
}

int32 DudeDoc::CountViews() const
{
	return m_viewList.CountItems();
}

DudeView* DudeDoc::ViewAt(int32 index) const
{
	return static_cast<DudeView*>(m_viewList.ItemAt(index));
}

void DudeDoc::RemoveView(DudeView* view)
{
	m_viewList.RemoveItem(view);
}



/////////////////////////////////////////////////////////////////////////////
// DudeDoc operations

// DudeDoc::DeleteContents
// -----------------------
// Clear all of the contents of the document.
//
// MFC NOTE: MFC provides this as a CDocument override.
void DudeDoc::DeleteContents()
{
	if (! WriteLock())
		return;
		
	int32 i = CountStrokes();
	while (--i >= 0) {
		delete RemoveStroke(i);
	} 
	SetModifiedFlag();
	WriteUnlock();
}

// DudeDoc::NewStroke
// ------------------
// Creates a stroke.
PenStroke* DudeDoc::NewStroke()
{
	if (! WriteLock())
		return 0;
				
	PenStroke* pStrokeItem = new PenStroke(CurPenSize());
	AddStroke(pStrokeItem);
	SetModifiedFlag();

	WriteUnlock();
	return pStrokeItem;
}

// DudeDoc::UpdateAllViews
// -----------------------
// Informs all views (except for the designated view) that the designated
// stroke has been changed. (The stroke ptr may be 0, in which case possibly
// all strokes have been changed.)
//
// MFC NOTE: A stripped down version of CDocument::UpdateAllViews.
void DudeDoc::UpdateAllViews(PenStroke* pStroke, DudeView* except)
{
	if (! ReadLock())
		return;
		
	int32 numViews = CountViews();
	for (int32 i = 0; i<numViews; i++) {
		DudeView* view = ViewAt(i);
		if ((! view) || (view == except))
			continue;
			
		BWindow* win = view->Window();
		if (win) {
			BMessage msg(MSG_DOC_CONTENTS_CHANGED);
			if (pStroke) {
				msg.AddRect("bounds", pStroke->Bounds());
			}
			win->PostMessage(&msg, view);
		}
	}
	ReadUnlock();
}

// DudeDoc::UpdateMenus
// --------------------
// Set the state of menu items based on the document.
//
// MFC NOTE: Analagous to UPDATE_COMMAND_UI message handlers,
// this function is called periodically to keep the menus in
// sync with the document's state.
void DudeDoc::UpdateMenus(BMenu* pMenu)
{
	if (! ReadLock())
		return;
		
	// Edit>Clear All: enable iff the document contains anything
	BMenuItem* pItem;
	pItem = pMenu->FindItem(MSG_EDIT_CLEAR_ALL);
	if (pItem) {
		bool bEnabled = CountStrokes() != 0;
		if (pItem->IsEnabled() != bEnabled) {
			pItem->SetEnabled(bEnabled);
		}
	}
	// Pen>Thick Line: mark iff the current pen size is thick
	pItem = pMenu->FindItem(MSG_PEN_THICK_OR_THIN);
	if (pItem) {
		bool bMarked = m_bThickPen;
		if (pItem->IsMarked() != bMarked) {
			pItem->SetMarked(bMarked);
		}
	}
	ReadUnlock();
}



/////////////////////////////////////////////////////////////////////////////
// DudeDoc message handlers

// DudeDoc::OnEditClearAll
// -----------------------
// Clears away all items in the document.
void DudeDoc::OnEditClearAll()
{
	DeleteContents();
	UpdateAllViews(0);
}

// DudeDoc::OnPenThickOrThin
// -------------------------
// Toggles the pen width between thick and thin values.
void DudeDoc::OnPenThickOrThin()
{
	if (WriteLock()) {
		SetPenSize(! m_bThickPen);
		WriteUnlock();
	}
}

// DudeDoc::OnPenWidths
// --------------------
// Allows the user to edit the thin and thick pen widths.
void DudeDoc::OnPenWidths()
{
	if (! ReadLock())
		return;
		
	// MFC NOTE: In BeOS, windows are ALWAYS created on the free store
	// because they delete themselves when finished.
	PenWidthsDlg* dlg = new PenWidthsDlg;
	
	// Initialize dialog data
	dlg->m_fThinSize = m_fThinSize;
	dlg->m_fThickSize = m_fThickSize;

	ReadUnlock();
	
	// Invoke the dialog box
	uint32 res = dlg->DoModal();
		
	if (res == DLG_OK && WriteLock())
	{
		// retrieve the dialog data
		m_fThinSize = dlg->m_fThinSize;
		m_fThickSize = dlg->m_fThickSize;

		// Update the current pen size using the new pen size
		// definitions for "thick" and "thin".
		SetPenSize(m_bThickPen);
		
		WriteUnlock();
	}
			
	dlg->PostMessage(B_QUIT_REQUESTED);	// will cause the dialog to quit
										// and delete itself
}


// DudeDoc::OnSave
// ---------------
// Save the document to a file.
void DudeDoc::OnSave()
{
	if (m_bVirgin) {
		// hasn't been saved yet; do a Save As instead
		OnSaveAs();
	} else if (WriteLock()) {
		DoSave();
		WriteUnlock();
	}	
}

// DudeDoc::OnSaveAs
// -----------------
// Prompt to save the document to a user-specified file.
void DudeDoc::OnSaveAs()
{
	if (! WriteLock())
		return;
		
	if (! m_pSavePanel) {
		m_pSavePanel = new BFilePanel(B_SAVE_PANEL, new BMessenger(this), m_pEntryRef,
			B_FILE_NODE, false);
	} else {
		m_pSavePanel->SetPanelDirectory(m_pEntryRef);
	}
	m_pSavePanel->SetSaveText(m_pEntryRef->name);
	WriteUnlock();
	DoSavePanel(m_bWaitForSave);
}

// DudeDoc::SaveRequested
// ----------------------
// The user has selected a file to save into. Update the document and
// save the file.
void DudeDoc::SaveRequested(BMessage* message)
{
	if (! WriteLock())
		return;
		
	entry_ref dirRef;
	const char* name;
	if ((message->FindRef("directory", &dirRef) == B_OK)
		&& (message->FindString("name", &name) == B_OK))
	{
		SetRef(&dirRef, name);
		DoSave();
	}

	m_bCancelSave = false;
	m_bSaveDone = true;
	if (m_bWaitForSave)	{		// DoSavePanel is waiting
		delete_sem(m_saveSem);	// this allows DoSavePanel to finish
		m_saveSem = -1;
	}
	WriteUnlock();
}



/////////////////////////////////////////////////////////////////////////////
// DudeDoc implementation

// DudeDoc::NewName
// ----------------
// Creates a new (untitled) name for the document.
void DudeDoc::NewName()
{
	static uint32 nUntitled = 1; // next available untitled #
	char name[B_FILE_NAME_LENGTH];
	char cwd[B_PATH_NAME_LENGTH];
	getcwd(cwd, B_PATH_NAME_LENGTH - 1);

	entry_ref dirRef;
	get_ref_for_path(cwd, &dirRef);
	
	sprintf(name, "Untitled %lu", nUntitled++);
	SetRef(&dirRef, name);
}

// DudeDoc::SetRef(const entry_ref*)
// ---------------------------------
// Sets the document's entry reference based on a full entry specification.
void DudeDoc::SetRef(const entry_ref* ref)
{
	if (! m_pEntryRef) {
		m_pEntryRef = new entry_ref(*ref);
	} else {
		*m_pEntryRef = *ref;
	}
	char loopName[B_FILE_NAME_LENGTH + 2];
	sprintf(loopName, "d>%s", m_pEntryRef->name);
	SetName(loopName);
}

// DudeDoc::SetRef(const entry_ref*, const char*)
// ----------------------------------------------
// Sets the document's entry reference based on a directory specification and
// leaf name.
void DudeDoc::SetRef(const entry_ref* dirRef, const char* name)
{
	BDirectory dir(dirRef);
	BEntry entry(&dir, name);
	if (! m_pEntryRef) {
		m_pEntryRef = new entry_ref;
	}
	entry.GetRef(m_pEntryRef);
	char loopName[B_FILE_NAME_LENGTH + 2];
	sprintf(loopName, "d>%s", m_pEntryRef->name);
	SetName(loopName);
}

// DudeDoc::SetPenSize
// -------------------
// Sets the pen width to either thin or thick.
//
// MFC NOTE: Instead of storing 'pen' objects, we simply store the
// graphics parameter.
void DudeDoc::SetPenSize(bool thick)
{
	m_bThickPen = thick;
	m_fPenSize = thick ? m_fThickSize : m_fThinSize;
}

// DudeDoc::SetModifiedFlag
// ------------------------
// Notifies the document that its contents have changed.
void DudeDoc::SetModifiedFlag()
{
	m_bModified = true;
	UpdateWindowTitles();
}

// DudeDoc::ResetModifiedFlag
// --------------------------
// Resets the document to 'unmodified' (after a save).
void DudeDoc::ResetModifiedFlag()
{
	m_bModified = false;
	UpdateWindowTitles();
}

// DudeDoc::DoSave
// ---------------
// Write the document to the file we're currently pointing to.
void DudeDoc::DoSave()
{
	// open the file for writing
	BFile file(m_pEntryRef, B_WRITE_ONLY | B_ERASE_FILE | B_CREATE_FILE);
	if (file.InitCheck() != B_OK)
		return;
	
	// write a message archive to the file
	BMessage archive;
	status_t res = Archive(&archive);
	if (res != B_OK)
		return;
		
	archive.Flatten(&file);
	
	// set the file's MIME type
	BNodeInfo ni(&file);
	ni.SetType("application/x-vnd.Be-DTS.DoodleDocument");
	
	// we're no longer dirty or unsaved
	m_bVirgin = false;
	ResetModifiedFlag();
	
	// add an MRU entry
	BMessage message(MSG_ADD_MRU);
	message.AddRef("refs", m_pEntryRef);
	be_app->PostMessage(&message);
}

// DudeDoc::DoSavePanel
// --------------------
// Run the File>Save As panel. If synchronous is true,
// block until the panel is finished.
void DudeDoc::DoSavePanel(bool synchronous)
{
	// setup the reply mechanism
	if (! WriteLock())
		return;
	
	m_bCancelSave = false;
	m_bSaveDone = false;
	WriteUnlock();
	
	if (! synchronous) {
		// just show the panel and exit immediately
		m_pSavePanel->Show();
		return;
	}
	
	// synchronous block for save panel
	m_saveSem = create_sem(0, "Wait4SaveSem");	
	m_pSavePanel->Show();
	
	WaitForDelete(m_saveSem);
}

// DudeDoc::CancelSave
// -------------------
// The user has cancelled the Save As dialog. Notify the appropriate
// authorities.
void DudeDoc::CancelSave()
{
	if (! WriteLock())
		return;
		
	if (! m_bSaveDone) {
		// we never handled a Save As notification
		// so we're really cancelling					
		m_bCancelSave = true;
		if (m_bWaitForSave) {		// DoSavePanel is waiting
			delete_sem(m_saveSem);	// allows DoSavePanel to exit
			m_saveSem = -1;
		}
	}
	WriteUnlock();
}

// DudeDoc::SaveBeforeClose
// ------------------------
// Wait for the document to be saved. Returns true if the document
// was saved, or false if the user canceled the operation (e.g. in
// the Save As dialog).
bool DudeDoc::SaveBeforeClose()
{
	if (WriteLock()) {
		m_bWaitForSave = true;
		WriteUnlock();
	}
	
	OnSave();
	
	if (WriteLock()) {	
		m_bWaitForSave = false;
		WriteUnlock();
	}
	
	return (! m_bCancelSave);
}

// DudeDoc::Activate
// -----------------
// Activates all of the windows associated with the document.
void DudeDoc::Activate(bool active)
{
	// run from first to last so that, if we're activating,
	// the most recently created window will come out on top
	if (! ReadLock())
		return;
		
	int32 numWins = CountWindows();
	for (int32 i = 0; i<numWins; i++) {
		DudeWin* pWin = WindowAt(i);
		pWin->Show();
		pWin->Activate(active);
	}
	ReadUnlock();
}

// DudeDoc::UpdateWindowTitles
// ---------------------------
// Updates the window titles associated with the document.
void DudeDoc::UpdateWindowTitles()
{
	int32 numWins = CountWindows();
	for (int32 i = 0; i<numWins; i++) {
		// post message instead of changing title directly
		// so that we don't deadlock during File>Save
		WindowAt(i)->PostMessage(MSG_DOC_TITLE_CHANGED);
	}
}
