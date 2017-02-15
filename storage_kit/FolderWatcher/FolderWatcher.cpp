//
// FolderWatcher.cpp
// -----------------
// Written by Scott Barta, Be Incorporated.
 
/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include "FolderWatcher.h"

#include <fs_attr.h>
#include <malloc.h>
#include <string.h>
#include <Looper.h>
#include <NodeInfo.h>
#include <NodeMonitor.h>
#include <Path.h>
#include <Locker.h>


BList FolderWatcher::mFolderWatcherList;
BList FolderWatcher::mMessageFilterList;
BLocker FolderWatcher::mListLocker;


// MessageFilterEntry
// ------------------
// The FolderWatcher maintains a list of these, which keep track
// of which BLoopers we have messsage filters inserted into.  The
// entries are reference counted, so they can be inserted only once into
// the BLooper but used multiple times; reference an entry before using it
// and dereference it when done; if Dereference() returns zero, then we're
// finished with it, so remove it from the BLooper and delete it and its
// list entry.
class MessageFilterEntry {
public:
					MessageFilterEntry(filter_hook hookFunction, BLooper *looper) :	
						mFilter(B_NODE_MONITOR, hookFunction), mLooper(looper), mRefCount(1) {}
			
	void			Reference() {mRefCount++;}
	bool			Dereference() {return (--mRefCount == 0);}

	BMessageFilter	mFilter;
	BLooper*		mLooper;

private:
	int32			mRefCount;
};


// NodeEntry
// ---------
// The FolderWatcher maintains a list of these, which allow us to match up node_ref
// values with the file name and other information.  Often, the only data we get in
// a Node Monitor notification is a node_ref, so we need to keep a table to turn this
// into something more useful.
class NodeEntry {
public:
				NodeEntry(const node_ref& ref, const char *name);
				~NodeEntry();

	node_ref	mRef;
	char*		mName;
	bool		mCorrectType;
};


// FolderWatcher::FolderWatcher
// ----------------------------
// Initialize data and add ourselves to the global list of FolderWatchers.  Don't do
// anything dramatic yet.
FolderWatcher::FolderWatcher(const BEntry& path, 
	const char *fileType,
	const char *appSig,
	BLooper *looper, 
	bool createDir) :
		mDirectory(&path),
		mDirEntry(0),
		mLooper(looper),
		mMessenger(looper),
		mCreateDir(createDir)
{
	// If we have to create the directory from scratch, store a BEntry to it
	// which we will use and dispose of in Init.  We need to do this because the
	// mDirectory member we set up in the constructor will be invalid, as it doesn't
	// point to a real directory, so we can't reconstitute the information from it.
	if (createDir) {
		mDirEntry = new BEntry(path);
		if (mDirEntry->InitCheck() != B_NO_ERROR) {
			delete mDirEntry;
			mDirEntry = 0;
		}
	}
		
	strcpy(mFileType, fileType);
	strcpy(mAppSig, appSig);

	// We won't worry about the case where this Lock() fails.
	mListLocker.Lock();
	mFolderWatcherList.AddItem(this);
	mListLocker.Unlock();
}


// FolderWatcher::~FolderWatcher
// -----------------------------
// Clean up any nodes we may still be watching, and remove ourselves from the
// global FolderWatcher list.  Clean up our message filter, too.
FolderWatcher::~FolderWatcher()
{
	node_ref nodeRef;
	// Stop watching our main directory.
	if (mDirectory.GetNodeRef(&nodeRef) == B_NO_ERROR)
		watch_node(&nodeRef, B_STOP_WATCHING, mMessenger);
	
	// Stop watching the files in our directory.
	for (int i = 0; i < mFileList.CountItems(); i++) {
		NodeEntry *nodeEntry = (NodeEntry *)mFileList.ItemAt(i);
		watch_node(&nodeEntry->mRef, B_STOP_WATCHING, mMessenger);
		delete nodeEntry;
	}

	// We won't worry about the case where this Lock() fails.
	mListLocker.Lock();
	mFolderWatcherList.RemoveItem(this);
	
	// Look for our entry in the message filter list and dereference it.
	// If the last user is done with it, then remove it from its BLooper and
	// trash it.
	for (int i = 0; i < mMessageFilterList.CountItems(); i++) {
		MessageFilterEntry *filterEntry = (MessageFilterEntry *)mMessageFilterList.ItemAt(i);
		if (filterEntry->mLooper == mLooper) {
			if (filterEntry->Dereference()) {
				mMessageFilterList.RemoveItem(filterEntry);
				mLooper->RemoveCommonFilter(&filterEntry->mFilter);	
				delete filterEntry;
			}
			break;
		}
	}

	mListLocker.Unlock();
}


// FolderWatcher::Init
// -------------------
// Take care of our message filter needs, create the directory if we're told to
// and it doesn't exist, and set up node monitors for the directory and its
// contents.
status_t FolderWatcher::Init()
{
	if (!mListLocker.Lock())
		return B_ERROR;
		
	// See if we already have a message filter installed into this BLooper.
	// If so, add a refcount to it.  If not, create and install one.
	MessageFilterEntry *filterEntry = 0;
	for (int i = 0; i < mMessageFilterList.CountItems(); i++) {
		MessageFilterEntry *tmp = (MessageFilterEntry *)mMessageFilterList.ItemAt(i);
		if (tmp->mLooper == mLooper) {
			filterEntry = tmp;
			break;
		}
	}
	if (filterEntry)
		filterEntry->Reference();
	else {
		filterEntry = new MessageFilterEntry(MessageFilter, mLooper);
		mMessageFilterList.AddItem(filterEntry);
		mLooper->AddCommonFilter(&filterEntry->mFilter);
	}

	mListLocker.Unlock();

	status_t status;
	
	// Create the directory if we're supposed to and it isn't there.
	if (mCreateDir && mDirEntry) {
		if (!mDirEntry->Exists()) {
			BPath path;
			status = mDirEntry->GetPath(&path);
			delete mDirEntry;
			if (status)
				return status;
			BDirectory temp;
			if ((status = temp.CreateDirectory(path.Path(), &mDirectory)))
				return status;
		}
	}
	
	// Set up a Node Monitor for the directory.
	node_ref nodeRef;
	if ((status = mDirectory.GetNodeRef(&nodeRef)))
		return status;
	if ((status = watch_node(&nodeRef, B_WATCH_DIRECTORY, mMessenger)))
		return status;
	
	// Set up Node Monitors for each matching file in
	// the directory, and send FileAdded messages for
	// each one.
	if ((status = mDirectory.Rewind()))
		return status;
	BEntry entry;
	while (mDirectory.GetNextEntry(&entry) == B_NO_ERROR) {
		// If you're modifying this to recurse into directories,
		// check entry.IsDirectory() and take action here.

		if (entry.IsFile() && MatchFileType(entry))
			WatchFile(entry, true);
	}	
	
	return B_NO_ERROR;
}


// FolderWatcher::MatchFileType
// ----------------------------
// Check the given file to see if it matches the type of the file that we've been
// told to monitor.  Based on the result, set the corresponding flag in the file's
// entry in our list.
bool FolderWatcher::MatchFileType(const BEntry& entry)
{
	BNode node(&entry);
	BNodeInfo nodeInfo(&node);
	char type[B_MIME_TYPE_LENGTH];
	bool retval;
	
	if (status_t err = nodeInfo.GetType(type) != B_NO_ERROR)
		retval = false;

	if (strcasecmp(type, mFileType) != 0)
		retval = false;

	retval = true;

	char filename[B_FILE_NAME_LENGTH];
	entry.GetName(filename);

	NodeEntry *nodeEntry = SearchFileList(filename);
	if (nodeEntry)
		nodeEntry->mCorrectType = retval;

	return retval;
}


// FolderWatcher::AddFile
// -----------------------
// Create a file in our folder with the filename and given attributes.  If the file
// already exists, it will be clobbered and replaced with a new file.  When the
// directory's node monitor picks up the new file, it will send notification back
// to the BLooper.
status_t FolderWatcher::AddFile(const char *filename,
	const BMessage& attrData)
{
	status_t status;
	
	BEntry entry(&mDirectory, filename);
	if ((status = entry.InitCheck()))
		return status;
		
	BFile file;
	if ((status = mDirectory.CreateFile(filename, &file)))
		return status;
	
	if ((status = WriteAttributes(file, attrData)))
		return status;
		
	// Set the file's type and preferred app.
	BNodeInfo nodeInfo(&file);
	if ((status = nodeInfo.SetType(mFileType)))
		return status;
	if ((status = nodeInfo.SetPreferredApp(mAppSig)))
		return status;
	return WatchFile(entry, true);
}


// FolderWatcher::RemoveFile
// -------------------------
// Delete the given file from our folder.  When the directory's node monitor picks
// up the deletion, it will send notification back.
status_t FolderWatcher::RemoveFile(const char *filename)
{
	status_t status;
	
	BEntry entry(&mDirectory, filename);
	if ((status = entry.InitCheck()))
		return status;
		
	return entry.Remove();
}


// FolderWatcher::ChangeFile
// -------------------------
// Write out the given attributes to the specified file in our folder.  The new
// attributes will overlay any previous attributes that were there, allowing you
// to change the value of just one attribute if you so wish by putting just one
// attribute spec into the BMessage.
status_t FolderWatcher::ChangeFile(const char *filename,
	const BMessage& attrData) const
{
	status_t status;
	
	BEntry entry(&mDirectory, filename);
	if ((status = entry.InitCheck()))
		return status;
	
	BFile file(&entry, B_WRITE_ONLY);
	return WriteAttributes(file, attrData);
}


// FolderWatcher::ReadAttributes
// -----------------------------
// Read the attributes from the given file and add them to the BMessage.
status_t FolderWatcher::ReadAttributes(const char *filename,
	BMessage& attrData) const
{
	status_t status;
	void *data = 0;
	
	
	BNode node(&mDirectory, filename);

	if ((status = node.InitCheck()))
		return status;
		
	// Iterate through all of the attributes in the node.
	// For each attribute, get its name, type, data, and size, and
	// create a corresponding entry in the message;
	if ((status = node.RewindAttrs()))
		return status;
	do {
		char attrName[B_ATTR_NAME_LENGTH];
		if (node.GetNextAttrName(attrName))
			break;

		attr_info attrInfo;
		if (node.GetAttrInfo(attrName, &attrInfo))
			break;
			
		if (data)
			free(data);
		data = malloc(attrInfo.size);
		if (!data)
			break;

		if (node.ReadAttr(attrName, attrInfo.type, 0, data, attrInfo.size) != attrInfo.size)
			break;
		
		if (attrData.AddData(attrName, attrInfo.type, data, attrInfo.size, false))
			break;
	} while (true);

	if (data)
		free(data);
	return B_NO_ERROR;
}


// FolderWatcher::WriteAttributes
// ------------------------------
// Write out the attributes contained in the BMessage to the given file.
status_t FolderWatcher::WriteAttributes(BFile& file,
	const BMessage& attrData) const
{
	status_t status;

	// Iterate through all of the bits of data in the message.
	// For each component, get its name, type, data, and size, and
	// create a corresponding attribute in the file.
	for (int i = 0; i < attrData.CountNames(B_ANY_TYPE); i++) {
		char *name;
		type_code type;
		if ((status = attrData.GetInfo(B_ANY_TYPE, i, &name, &type)))
			return status;
		const void *data;
		ssize_t size;
		if ((status = attrData.FindData(name, type, &data, &size)))
			return status;
		if (file.WriteAttr(name, type, 0, data, size) != size)
			return status;
	}
	return B_NO_ERROR;
}


// FolderWatcher::WatchFile
// ------------------------
// Set up a node monitor on the file, create an entry containing the node
// and file information we need to keep, and save the entry in our file list.
status_t FolderWatcher::WatchFile(const BEntry& entry, bool sendNotification)
{
	// NOTE:  If we are called upon to watch a file immediately after we get
	// a Node Monitor B_ENTRY_CREATED message on it, its type may not be set
	// yet.  This can happen when copying a file in the Tracker, for instance --
	// we get the creation notification first, but attributes get written later.
	// We don't just want to ignore this file, because it may be valid, but
	// we don't want to assume it will be valid, either.  We need to watch it
	// and send notification back to the class' user later if the type attribute
	// gets set to what we want.  We'll watch the node, but flag it in our list
	// as having the incorrect type and not send notification back just yet.
	status_t status;
	
	char filename[B_FILE_NAME_LENGTH];
	entry.GetName(filename);

	// If we're already watching this file, return without error.
	if (SearchFileList(filename)) {
		return B_NO_ERROR;
	}
			
	node_ref dummyRef;
	NodeEntry *nodeEntry = new NodeEntry(dummyRef, filename);
	if ((status = entry.GetNodeRef(&nodeEntry->mRef)))
		return status;

	// Watch the file and add it to our list.
	if ((status = watch_node(&nodeEntry->mRef, B_WATCH_ATTR, mMessenger))) {
		return status;
	}

	mFileList.AddItem(nodeEntry);
	
	if (sendNotification && MatchFileType(entry)) {
		BMessage msg;
		SendEntryAddedMessage(msg, filename, true);
	}
		
	return B_NO_ERROR;
}


// FolderWatcher::StopWatching
// ---------------------------
// Remove the node monitor from the given file and the file's entry from our file
// list.
status_t FolderWatcher::StopWatching(const char *filename)
{
	NodeEntry *entry = SearchFileList(filename);
	if (entry) {
		mFileList.RemoveItem(entry);
		status_t result = watch_node(&entry->mRef, B_STOP_WATCHING, mMessenger);
		delete entry;
		return result;
	}
	return B_ERROR;
}


// FolderWatcher::SendEntryAddedMessage
// ------------------------------------
// Modify the supplied BMessage to turn it into a kFileAdded message suitable for
// this class' user, using the filename given to us.  If we are instructed to do
// so, we can actually send the message, or we can rely on the caller to take
// care of it (despite the name of this function).
void FolderWatcher::SendEntryAddedMessage(BMessage& message, const char *filename, bool send) const
 {
	message.MakeEmpty();
	message.what = kFileAdded;
	message.AddString("Filename", filename);
	message.AddPointer("FolderWatcher", this);

	BMessage attrData;
	if (ReadAttributes(filename, attrData) == B_NO_ERROR)
		message.AddMessage("AttrData", &attrData);

	if (send)
		mMessenger.SendMessage(&message);
}


// FolderWatcher::SendEntryChangedMessage
// --------------------------------------
// Modify the supplied BMessage to turn it into a kFileChanged message suitable for
// this class' user, using the filename given to us.  If we are instructed to do
// so, we can actually send the message, or we can rely on the caller to take
// care of it (despite the name of this function).
void FolderWatcher::SendEntryChangedMessage(BMessage& message, const char *filename, bool send) const
{
	// Since this message is so like the EntryAddedMessage, we'll use its code.
	SendEntryAddedMessage(message, filename, false);
	message.what = kFileChanged;

	if (send)
		mMessenger.SendMessage(&message);
}


// FolderWatcher::SendEntryRemovedMessage
// --------------------------------------
// Modify the supplied BMessage to turn it into a kFileRemoved message suitable for
// this class' user, using the filename given to us.  If we are instructed to do
// so, we can actually send the message, or we can rely on the caller to take
// care of it (despite the name of this function).
void FolderWatcher::SendEntryRemovedMessage(BMessage& message, const char *filename, bool send) const
{
	message.MakeEmpty();
	message.what = kFileRemoved;
	message.AddString("Filename", filename);
	message.AddPointer("FolderWatcher", this);

	if (send)
		mMessenger.SendMessage(&message);
}


// FolderWatcher::SearchFileList
// -----------------------------
// Search the list of watched files to find one that matches the supplied node_ref.
// If it is found, then set filename to point to its name and return the index
// of the node entry in the list.
int32 FolderWatcher::SearchFileList(const node_ref& nodeRef, const char **filename) const
{
	for (int i = 0; i < mFileList.CountItems(); i++) {
		NodeEntry *listEntry = (NodeEntry *)mFileList.ItemAt(i);
		if (listEntry->mRef == nodeRef) {
			if (filename)
				*filename = listEntry->mName;
			return i;
		}
	}
	return -1;
}


// FolderWatcher::SearchFileList
// -----------------------------
// Search the list of watched files to find one that matches the supplied filename.
// If it is found, return a pointer to its node entry in the file list.
NodeEntry* FolderWatcher::SearchFileList(const char *filename) const
{
	for (int i = 0; i < mFileList.CountItems(); i++) {
		NodeEntry *listEntry = (NodeEntry *)mFileList.ItemAt(i);
		if (strcmp(listEntry->mName, filename) == 0) {
			return listEntry;
		}
	}
	return 0;
}


// FolderWatcher::FindFolderWatcher
// --------------------------------
// Search through the global list of FolderWatchers to see if one of them is watching the file
// with the given node ref.  If so, then fill out the filename parameter with the filename of the
// file and return the FolderWatcher that is responsible for it.
FolderWatcher* FolderWatcher::FindFolderWatcher(const node_ref& nodeRef, const char **filename)
{
	if (!mListLocker.Lock())
		return 0;
	for (int i = 0; i < mFolderWatcherList.CountItems(); i++) {
		FolderWatcher *watcher = (FolderWatcher *)mFolderWatcherList.ItemAt(i);
		if (watcher->SearchFileList(nodeRef, filename) >= 0) {
			mListLocker.Unlock();
			return watcher;
		}
	}
	mListLocker.Unlock();
	return 0;
}


// FolderWatcher::FindFolderWatcher
// --------------------------------
// Search through the global list of FolderWatchers to see if one of them is watching the given
// directory.  If so, return a pointer to it.
FolderWatcher* FolderWatcher::FindFolderWatcher(const BDirectory& directory)
{
	if (!mListLocker.Lock())
		return 0;
		
	BEntry entry1,entry2;
	directory.GetEntry(&entry1);
	
	for (int i = 0; i < mFolderWatcherList.CountItems(); i++) {
		FolderWatcher *folderWatcher = (FolderWatcher *)mFolderWatcherList.ItemAt(i);
		folderWatcher->mDirectory.GetEntry(&entry2);
		
		if (entry1 == entry2)
			return folderWatcher;
	}
		
	mListLocker.Unlock();
	return 0;
}


// FolderWatcher::MessageFilter
// ----------------------------
// This is the hook function of the message filter that we install in the BLooper of our
// client.  It intercepts Node Monitor messages, and if they apply to one of the nodes we are
// monitoring, we munge the message and convert it into a format that our client expects.  As
// necessary, we may also send additional messages if the situation warrants.
filter_result FolderWatcher::MessageFilter(BMessage *message,
	BHandler **target,
	BMessageFilter *filter)
{
	int32 opcode;
	if (message->FindInt32("opcode", &opcode) == B_NO_ERROR) {
		switch(opcode) {
			case B_ATTR_CHANGED: {
				ino_t node;
				dev_t device;
				if (message->FindInt32("device", &device) == B_NO_ERROR &&
					message->FindInt64("node", &node) == B_NO_ERROR) {
					const char *filename;
					node_ref nodeRef;
					nodeRef.device = device;
					nodeRef.node = node;
					FolderWatcher *folderWatcher;
					
					if ((folderWatcher = FindFolderWatcher(nodeRef, &filename))) {
						// Take special care here.  If this node is being monitored but isn't
						// marked as having the correct type (see FolderWatcher::WatchFile), 
						// then see if its type has changed to something favorable, and send
						// a kFileAdded message instead of a kFileChanged message.  If it is
						// marked as the correct type, but the type changes to something
						// unfavorable (an unlikely case), then send a kFileRemoved message.
						message->MakeEmpty();
						
						NodeEntry *nodeEntry = folderWatcher->SearchFileList(filename);
						if (nodeEntry) {						
							BEntry entry(&folderWatcher->mDirectory, filename);
							if (entry.InitCheck() == B_NO_ERROR) {
								if (!nodeEntry->mCorrectType && folderWatcher->MatchFileType(entry)) {
									nodeEntry->mCorrectType = true;
									folderWatcher->SendEntryAddedMessage(*message, filename, false);
								} else if (nodeEntry->mCorrectType && !(folderWatcher->MatchFileType(entry))) {
									folderWatcher->SendEntryRemovedMessage(*message, filename, false);
									nodeEntry->mCorrectType = false;
								} else if (nodeEntry->mCorrectType){
									folderWatcher->SendEntryChangedMessage(*message, filename, false);
								}
							}
						}
					}
				}	
				break;
			}

			case B_ENTRY_MOVED: {
				ino_t fromDirectory;
				ino_t toDirectory;
				dev_t device;
				ino_t node;
				const char *tmpFrom;
				const char *tmpTo;
				if (message->FindInt32("device", &device) == B_NO_ERROR &&
					message->FindInt64("from directory", &fromDirectory) == B_NO_ERROR &&
					message->FindInt64("to directory", &toDirectory) == B_NO_ERROR &&
					message->FindInt64("node", &node) == B_NO_ERROR &&
					message->FindString("name", &tmpTo) == B_NO_ERROR) {
					
					char toName[B_FILE_NAME_LENGTH];
					strcpy(toName, tmpTo);
					
					node_ref nodeRef;
					nodeRef.node = node;
					nodeRef.device = device;

					// Get the filename stored for the node when we started watching it.
					// We can't report the filename reported to us in the message, because
					// the filename has changed if the item was moved into the trash and an
					// identically named item already existed there.
					
					// Handle the case where a file has been moved from a folder we're watching.
					// It may or may not being moved *to* a folder we're watching.
					if (FindFolderWatcher(nodeRef, &tmpFrom)) {
						char fromName[B_FILE_NAME_LENGTH];
						strcpy(fromName, tmpFrom);
						
						// Create BEntry and BDirectory objects for the nodes and their parents,
						// respectively.
						entry_ref fromEntryRef(device, fromDirectory, fromName);
						entry_ref toEntryRef(device, toDirectory, toName);
						BEntry fromEntry(&fromEntryRef);
						BEntry toEntry(&toEntryRef);
						BDirectory fromDirectory;
						BDirectory toDirectory;
						
						if (fromEntry.InitCheck() == B_NO_ERROR &&
							toEntry.InitCheck() == B_NO_ERROR && 
							fromEntry.GetParent(&fromDirectory) == B_NO_ERROR &&
							toEntry.GetParent(&toDirectory) == B_NO_ERROR) {
							
							bool messageChanged = false;							
							
							// See if there's a FolderWatcher for the From directory.  If so,
							// send a notification message back.
							FolderWatcher *folderWatcher = FindFolderWatcher(fromDirectory);
							if (folderWatcher && folderWatcher->SearchFileList(nodeRef) != -1) {
								folderWatcher->StopWatching(fromName);
								if (folderWatcher->MatchFileType(toEntry)) {
									folderWatcher->SendEntryRemovedMessage(*message, fromName, false);
									messageChanged = true;
								}
							}

							// See if there's a FolderWatcher for the To directory.  If so,
							// send a notification message back.
							folderWatcher = FindFolderWatcher(toDirectory);
							if (folderWatcher && folderWatcher->SearchFileList(nodeRef) == -1) {
								folderWatcher->WatchFile(toEntry, false);
								if (folderWatcher->MatchFileType(toEntry))
									if (!messageChanged)
										folderWatcher->SendEntryAddedMessage(*message, toName, false);
									else {
										// We've already used up the message we were sent, but
										// we need to send another message.  Build one and fire
										// it off.  Don't overwrite the old message, because it
										// will be passed on to the BLooper when this filter is
										// done with it; thus, the message we are creating here
										// will be received after the kEntryRemoved message.
										BMessage msg;
										folderWatcher->SendEntryAddedMessage(msg, toName, true);
									}								
							}
						}
						break;
					} else {
						// The file has been moved into a folder we're watching, but we're ignorant
						// of the folder it's coming from.  Convert this message into a
						// B_ENTRY_CREATED message and fall through to the code that handles it.
						message->what = B_ENTRY_CREATED;
						message->AddInt64("directory", toDirectory);
					}
				} else
					// We couldn't extract all of the necessary information from the message.
					// Bail out.
					break;			
			}
			
			case B_ENTRY_CREATED: {	
				ino_t directory;
				dev_t device;
				const char *name;
				if (message->FindInt32("device", &device) == B_NO_ERROR &&
					message->FindInt64("directory", &directory) == B_NO_ERROR &&
					message->FindString("name", &name) == B_NO_ERROR) {
					char filename[B_FILE_NAME_LENGTH];
					strcpy(filename, name);
					entry_ref entryRef(device, directory, filename);
					BEntry entry(&entryRef);
					BDirectory directory;
					
					// Find the parent directory of the node that was created.  Find a FolderWatcher
					// for it and send the notification.
					if (entry.InitCheck() == B_NO_ERROR && entry.GetParent(&directory) == B_NO_ERROR) {
						FolderWatcher *folderWatcher = FindFolderWatcher(directory);
						if (folderWatcher) {
							folderWatcher->WatchFile(entry, false);
							if (folderWatcher->MatchFileType(entry))
								folderWatcher->SendEntryAddedMessage(*message, filename, false);
						}
					}
				}
				break;
			}

			case B_ENTRY_REMOVED: {		
				ino_t directory;
				dev_t device;
				ino_t node;
				if (message->FindInt32("device", &device) == B_NO_ERROR &&
					message->FindInt64("directory", &directory) == B_NO_ERROR &&
					message->FindInt64("node", &node) == B_NO_ERROR) {
					const char *tmpName;
					node_ref nodeRef;
					nodeRef.device = device;
					nodeRef.node = node;
					FolderWatcher *folderWatcher;

					// Find the parent directory of the node that was deleted.  Find a FolderWatcher
					// for it and send the notification.
					if ((folderWatcher = FindFolderWatcher(nodeRef, &tmpName))) {
						char filename[B_FILE_NAME_LENGTH];
						strcpy(filename, tmpName);
						NodeEntry *nodeEntry = folderWatcher->SearchFileList(filename);
						if (nodeEntry && nodeEntry->mCorrectType)
							folderWatcher->SendEntryRemovedMessage(*message, filename, false);

						folderWatcher->StopWatching(filename);
						
					}
				}	
				break;
			}

		}
	}
	return B_DISPATCH_MESSAGE;
}


// NodeEntry::NodeEntry
// --------------------
NodeEntry::NodeEntry(const node_ref& ref, const char *name) : mRef(ref)
{
	mName = (char *)malloc(strlen(name) + 1);
	strcpy(mName, name);
	mCorrectType = false;
}


// NodeEntry::~NodeEntry
// ---------------------
NodeEntry::~NodeEntry()
{
	free(mName);
}

