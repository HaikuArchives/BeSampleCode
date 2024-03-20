/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef _dudedoc_h
#define _dudedoc_h

/////////////////////////////////////////////////////////////////////////////
// Class: DudeDoc
// --------------
// The document (i.e. data).
//
// MFC NOTE: There is no direct analog in the BeOS kits to MFC documents
// and document templates. So, we have to provide these ourselves. I derive
// from a class BLooper to obtain multithreaded and message handling
// capabilities, as well as storage capabilities which are similar to
// serialization. Also note that this class contains the stuff which would,
// in MFC, be provided by a document template.

#include <Menu.h>
#include <FilePanel.h>
#include <Locker.h>
#include <Looper.h>
#include <Entry.h>
#include "MultiLocker.h"

class PenStroke;
class DudeWin;
class DudeView;

class DudeDoc : public BLooper
{
// static methods
public:
	static status_t		CreateDocument();
	static status_t		CreateDocument(const entry_ref* ref);
	
	static int32		CountDocuments();
	static DudeDoc*		DocumentAt(int32 index);

	static __declspec(dllexport) BArchivable*
		Instantiate(BMessage* archive);	

// static implemenation
private:
	static void			AddDocument(DudeDoc* document);
	static void			RemoveDocument(DudeDoc* document);
	static bool			ActivateExistingDocument(const entry_ref* ref);
	
// static data
private:
	static BList		s_docList;			// list of open documents in the app
	static BLocker		s_listLock;


// construction, destruction, operators
public:
						DudeDoc();
private:
						DudeDoc(BMessage* archive);
public:
	virtual				~DudeDoc();
	
// overrides
public:
	virtual status_t	Archive(BMessage* archive, bool deep = true) const;
	virtual void		MessageReceived(BMessage* message);
	virtual bool		QuitRequested();
	
// Doodle-specific accessors
public:
	// miscellaneous
	BRect				Bounds() const { return m_docBounds; }
	bool				IsModified() const { return m_bModified; }
	const char*			Title() const { return m_pEntryRef->name; }
	float				CurPenSize() const { return m_fPenSize; }
	
	// stroke list
	void				AddStroke(PenStroke* stroke);
	int32				CountStrokes() const;
	PenStroke*			StrokeAt(int32 index) const;
	PenStroke*			RemoveStroke(int32 index);

// general document accessors	
public:
	// data locking
	bool				ReadLock();
	void				ReadUnlock();
	bool				WriteLock();
	void				WriteUnlock();
	bool				IsReadLocked();
	bool				IsWriteLocked();
	
	// print settings
	BMessage*			PrintSettings() const;
	void				SetPrintSettings(BMessage* settings);
	
	// window list	
	void				AddWindow(DudeWin* pWin);
	int32				CountWindows() const;
	int32				IndexOfWindow(DudeWin* pWin) const;
	DudeWin*			WindowAt(int32 index) const;
	void				RemoveWindow(DudeWin* pWin);

	// view list	
	void				AddView(DudeView* pView);
	int32				CountViews() const;
	DudeView*			ViewAt(int32 index) const;
	void				RemoveView(DudeView* pView);
	
// operations
public:
	void				DeleteContents();
	PenStroke*			NewStroke();
	void				SetModifiedFlag();
	void				UpdateAllViews(PenStroke* stroke = 0, DudeView* except = 0);
	void				UpdateMenus(BMenu* pMenu);
	
// message handlers
private:
	void				OnEditClearAll();
	void				OnPenThickOrThin();
	void				OnPenWidths();
	void				OnSave();
	void				OnSaveAs();
	void				SaveRequested(BMessage* message);
	
// implementation
private:
	void				NewName();
	void				SetRef(const entry_ref* ref);
	void				SetRef(const entry_ref* dirRef, const char* name);
	void				SetPenSize(bool thick);
	void				ResetModifiedFlag();

	void				DoSave();
	void				DoSavePanel(bool synchronous);
	void				CancelSave();
	bool				SaveBeforeClose();
	
	void				Activate(bool active = true);
	void				UpdateWindowTitles();
	
// data members
private:
	// Doodle-specific data	
	float				m_fThinSize;		// size when thin is selected
	float				m_fThickSize;		// size when thick is selected
	float				m_fPenSize;			// current user-selected pen size
	bool				m_bThickPen;		// true iff current pen is thick
	BRect				m_docBounds;		// data bounds
	BList				m_strokeList;		// the list of strokes
	
	// Miscellanous document data
	bool				m_bVirgin;			// true iff document has never been saved
	bool				m_bModified;		// true iff document has been modified w/o save
	entry_ref*			m_pEntryRef;		// the "apparent" entry for this document
											// includes the "title" of the document
	BMessage*			m_pPrintSettings;	// the page setup information
	MultiLocker			m_dataLock;
		
	// File>Save data
	BFilePanel*			m_pSavePanel;		// the "save as" dialog box
	sem_id				m_saveSem;			// semaphore that controls save before exit
	bool				m_bWaitForSave;		// whether we need to wait for save to finish
	bool				m_bSaveDone;		// marks when Save As is completed
	bool				m_bCancelSave;		// whether Save As was cancelled
	
	// "Document template" data
	BList				m_winList;			// the list of windows assoc. w/ document
	BList				m_viewList;			// the list of views assoc. w/ document
	
};

#endif /* _dudedoc_h */
