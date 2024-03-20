//
// FolderWatcher.cpp
// -----------------
// Written by Scott Barta, Be Incorporated.

/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/
 
#include <Directory.h>
#include <List.h>
#include <Locker.h>
#include <MessageFilter.h>
#include <Messenger.h>

class NodeEntry;

// FolderWatcher
// -------------
// This class sets up a Node Monitor on a folder and all of the files within it.  It
// inserts a message filter into a BLooper that you supply and intercepts the Node
// Monitor messages sent to that BLooper, transforming them into a more useful form.
// For information on how to use this class, see the Be Newsletter, Volume II, Issue 19,
// "The Tracker is Your Friend".
class FolderWatcher {
public:
						FolderWatcher(const BEntry& path, 
							  		  const char *fileType,
							  		  const char *appSig,
							  		  BLooper *looper, 
							  		  bool createDir = false);
						~FolderWatcher();
	
	status_t			Init();
	
	status_t			AddFile(const char *filename,
								const BMessage& attrData);
	status_t			RemoveFile(const char *filename);
	status_t			ChangeFile(const char *filename,
						 		   const BMessage &attrData) const;
	
static const uint32		kFileAdded   = 'FWfa';
static const uint32		kFileRemoved = 'FWfr';
static const uint32		kFileChanged = 'FWfc';
	
	
protected:
	status_t			WatchFile(const BEntry& entry, bool sendNotification);
	status_t			StopWatching(const char *filename);

	status_t			ReadAttributes(const char *filename,
									   BMessage& attrData) const;
	status_t			WriteAttributes(BFile& file,
										const BMessage& attrData) const;

	int32				SearchFileList(const node_ref& nodeRef,
									   const char **filename = 0) const;
	NodeEntry*			SearchFileList(const char *filename) const;
	bool				MatchFileType(const BEntry& entry);

	void				SendEntryAddedMessage(BMessage &message,
											  const char *filename,
											  bool send) const;
	void				SendEntryRemovedMessage(BMessage& message,
												const char *filename,
												bool send) const;
	void				SendEntryChangedMessage(BMessage& message,
												const char *filename,
												bool send) const;

static filter_result	MessageFilter(BMessage *message,
									  BHandler **target,
									  BMessageFilter *filter);

static FolderWatcher*	FindFolderWatcher(const BDirectory& directory);
static FolderWatcher*	FindFolderWatcher(const node_ref& nodeRef,
										  const char **filename = 0);
	
	BDirectory			mDirectory;
	BEntry				*mDirEntry;
	BList				mFileList;
	char				mFileType[B_MIME_TYPE_LENGTH];
	char				mAppSig[B_MIME_TYPE_LENGTH];
	BLooper*			mLooper;
	BMessenger			mMessenger;
	bool				mCreateDir;
	
static BList			mFolderWatcherList;
static BList			mMessageFilterList;
static BLocker			mListLocker;
};
