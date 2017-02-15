/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include <Directory.h>
#include <File.h>
#include <FindDirectory.h>
#include <Message.h>
#include <Path.h>
#include <Entry.h>
#include "dudeprefs.h"

/////////////////////////////////////////////////////////////////////////////
// DudePrefs static methods

// DudePrefs::Load
// ---------------
// Creates a new set of prefs from the global preferences file.
// Returns the new set of prefs if successful, or 0 if the file
// could not be found or read properly.
//
// MFC NOTE: Instead of storing this information in the Registry,
// we store it in a file within the user settings directory; e.g.
// /boot/home/config/settings/Doodle/GlobalPrefs.
DudePrefs* DudePrefs::Load()
{
	BPath prefPath;
	status_t res;
	res = find_directory(B_USER_SETTINGS_DIRECTORY, &prefPath);
	if (res != B_OK)
		return 0;
	
	if (prefPath.Append("Doodle/GlobalPrefs") != B_OK)
		return 0;
	
	BFile prefFile(prefPath.Path(), B_READ_ONLY);
	res = prefFile.InitCheck();
	if (res != B_OK)
		return 0;

	
	BMessage archive;
	if (archive.Unflatten(&prefFile) != B_OK)
		return 0;

	DudePrefs* pPrefs = dynamic_cast<DudePrefs*>
		(instantiate_object(&archive));

	return pPrefs;
}

// DudePrefs::Instantiate
// ----------------------
// Creates a new preferences object from an archive.
BArchivable* DudePrefs::Instantiate(BMessage* archive)
{
	if (validate_instantiation(archive, "DudePrefs"))
		return new DudePrefs(archive);
	else
		return 0;
}

/////////////////////////////////////////////////////////////////////////////
// DudePrefs construction, destruction, operators

// DudePrefs::DudePrefs(BMessage*)
// -------------------------------
// Constructs a preferences object from a message archive.
//
// MFC NOTE: This is how BArchivable objects are 'unserialized.' This is
// similar to Serialize where CArchive::IsStoring() == false. Compare this
// function to its counterpart, DudePrefs::Archive.
DudePrefs::DudePrefs(BMessage* archive)
	: BArchivable(), m_nMRUFiles(5)
{
	int32 val;
	if (archive->FindInt32("Max MRU Entries", &val) == B_OK) {
		if (val < 0)
			m_nMRUFiles = 0;
		else if (val > 10)
			m_nMRUFiles = 10;
		else
			m_nMRUFiles = val;
	}
	int32 numRefs = 0;
	entry_ref ref;
	while (archive->FindRef("MRU Entries", numRefs++, &ref) == B_OK)
		AddMRUFile(&ref);
}

DudePrefs::DudePrefs()
	: BArchivable(), m_nMRUFiles(5)
{ }

DudePrefs::~DudePrefs()
{
	entry_ref* pRef = RemoveMRU(0);
	while (pRef) {
		delete pRef;
		pRef = RemoveMRU(0);
	}
}

/////////////////////////////////////////////////////////////////////////////
// DudePrefs accessors

int32 DudePrefs::GetMaxMRUFiles() const
{
	return m_nMRUFiles;
}

void DudePrefs::SetMaxMRUFiles(int32 max)
{
	m_nMRUFiles = max;
}

const entry_ref* DudePrefs::MRUAt(int32 index) const
{
	return static_cast<entry_ref*>(m_MRUEntries.ItemAt(index));
}

int32 DudePrefs::CountMRUs() const
{
	return m_MRUEntries.CountItems();
}



/////////////////////////////////////////////////////////////////////////////
// DudePrefs overrides

// DudePrefs::Archive
// ------------------
// Saves the state of the preferences object into the archive.
//
// MFC NOTE: This is how BArchivable objects are 'serialized.' This is
// similar to Serialize where CArchive::IsStoring() == true. Compare this
// function to its counterpart, DudePrefs::DudePrefs(BMessage*).
status_t DudePrefs::Archive(BMessage* archive, bool deep) const
{
	status_t res = BArchivable::Archive(archive, deep);
	if (res != B_OK)
		return res;
		
	archive->AddInt32("Max MRU Entries", m_nMRUFiles);
	int32 numEntries = CountMRUs();
	for (int32 i=0; i < numEntries && i < m_nMRUFiles; i++) {
		archive->AddRef("MRU Entries", MRUAt(i));
	}
	return B_OK;
}



/////////////////////////////////////////////////////////////////////////////
// DudePrefs operations

// DudePrefs::AddMRUFile
// ---------------------
// Adds the reference to the list of most recently used files.
void DudePrefs::AddMRUFile(const entry_ref* newRef)
{
	entry_ref* pRef;
	int32 maxRefs = GetMaxMRUFiles();
	
	// trim the buffer to the correct length
	while (CountMRUs() > maxRefs) {
		pRef = RemoveMRU(0);
		if (pRef)
			delete pRef;
	}

	if (maxRefs == 0)
		return;
			
	// add only if unique
	int32 numRefs = CountMRUs();		
	for (int32 i=0; i<numRefs; i++) {
		const entry_ref* curRef = MRUAt(i);
		if (*curRef == *newRef)
			return;
	}

	// add the item, killing first item in buffer if maxed	
	if (numRefs == GetMaxMRUFiles()) {
		pRef =  RemoveMRU(0);
		if (pRef)
			delete pRef;
	}
	
	pRef = new entry_ref(*newRef);
	m_MRUEntries.AddItem(pRef);
}

// DudePrefs::Save
// ---------------
// Saves the set of prefs to the global preferences file.
//
// MFC NOTE: Instead of storing this information in the Registry,
// we store it in a file within the user settings directory; e.g.
// /boot/home/config/settings/Doodle/GlobalPrefs.
void DudePrefs::Save()
{
	// find the directory, creating it if necessary
	BPath prefPath;
	status_t res;
	res = find_directory(B_USER_SETTINGS_DIRECTORY, &prefPath, true);
	if (res != B_OK)
		return;
			
	res = prefPath.Append("Doodle");
	if (res != B_OK)
		return;
		
	res = create_directory(prefPath.Path(), 0777);
	if (res != B_OK)
		return;

	// find the file, creating it if necessary	
	res = prefPath.Append("GlobalPrefs");
	if (res != B_OK)
		return;
	
	BFile file(prefPath.Path(), B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
	res = file.InitCheck();
	if (res != B_OK)
		return;

	// write the data to the file	
	BMessage archive;
	if (Archive(&archive) != B_OK)
		return;
	
	archive.Flatten(&file);
}



/////////////////////////////////////////////////////////////////////////////
// DudePrefs implementation

entry_ref* DudePrefs::RemoveMRU(int32 index)
{
	return static_cast<entry_ref*>(m_MRUEntries.RemoveItem(index));
}
