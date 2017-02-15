/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef _dudeprefs_h
#define _dudeprefs_h

/////////////////////////////////////////////////////////////////////////////
// Class: DudePrefs
// ----------------
// Doodle's set of preferences.

#include <Archivable.h>
#include <List.h>

class DudePrefs : BArchivable
{
// static methods
public:
	static DudePrefs*	Load();
	
	static __declspec(dllexport) BArchivable*
		Instantiate(BMessage* archive);
	
// construction, destruction, operators
public:
						DudePrefs();
private:
						DudePrefs(BMessage* archive);
public:
	virtual 			~DudePrefs();
	
// accessors
public:
	int32				GetMaxMRUFiles() const;
	void				SetMaxMRUFiles(int32 max);
	const entry_ref*	MRUAt(int32 index) const;
	int32				CountMRUs() const;

// overrides
private:
	virtual status_t	Archive(BMessage* archive, bool deep = true) const;

// operations
public:
	void				AddMRUFile(const entry_ref* newRef);
	void				Save();

// implementation
private:
	entry_ref*			RemoveMRU(int32 index);

// data members
private:
	int32				m_nMRUFiles;
	BList				m_MRUEntries;
};

#endif /* _dudeprefs_h */