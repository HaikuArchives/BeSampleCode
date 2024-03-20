/* LiveQueryWindow.h */
/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include <Window.h>
#include <Query.h>
#include <Node.h>
#include <List.h>
#include <Locker.h>
#include <StringItem.h>
#include <TextView.h>


class BList;
class BLocker;
class BListView;
class BStringItem;
class BStringView;
class BTextControl;
class BTextView;

//struct for caching query entry info
typedef struct live_ref {
	dev_t 			dev;		//device id of the entry
	ino_t 			node;		//node id of the entry
	dev_t 			pdev;		//device id of the entry's parent
	ino_t 			pnode;		//node id of the entry's parent
	char *			name;		//name of the entry
	BStringItem *	item;		//path of the entry for fValidView
	status_t		status;		//whether the entry could be accessed
	status_t		state;		//why it is in the list it is in
};

//debug printing of live_ref contents
void live_ref_to_stream(live_ref *ref);

typedef struct live_ref_ptr {
	node_ref		node;
	live_ref *		ref;
};

//message constants
const int32	FILES_TYPE =	'?fil';
const int32	FOLDERS_TYPE =	'?fol';
const int32	SOURCE_TYPE	=	'?src';
const int32	FETCH =			'?fch';

class LiveQueryWindow : public BWindow {
	public:
								LiveQueryWindow();
		virtual					~LiveQueryWindow();
		virtual void			MessageReceived(BMessage *msg);
		virtual bool			QuitRequested();
		
		//static entry function for our query thread
		static int32			QueryThreadFunc(void *arg);
		
	private:
		//fetch the items from the query
		//placing them in the valid or zombie lists as appropriate
		void					FetchLive();
		//clear the query, valid and zombie lists
		void					ClearResults();

		//functions to handle query and node monitor update messages
		void					EntryCreated(BMessage *msg);
		void					EntryRemoved(BMessage *msg);
		void					EntryMoved(BMessage *msg);
		void					EntryChanged(int32 opcode, BMessage *msg);
		
		//actually update the ref and instantiate it to get the info we need
		//if necessary, move between valid and zombie lists
		void					UpdateEntry(live_ref *rec, bool valid);
		//initial creation of a new entry
		void					NewEntry(live_ref *rec);
		//delete and clear out an entry ref
		//including stopping the node monitor
		void					DeleteEntry(live_ref *ref);

		//functions for finding a live ref in a given list
		status_t				FindCachedEntry(node_ref node, live_ref **ref, BList *list);
		static bool				FindLiveRef(void *item, void *rec);

		//move a live ref from one list to another
		void					ValidToZombie(live_ref *ref);
		void					ZombieToValid(live_ref *ref);

		//update the count of valid items
		void					UpdateCount();
		
		BTextView *				fQueryText;
		BListView *				fValidView;
		BTextControl *			fNameString;
		BStringView *			fCountString;
		
		BQuery *				fQuery;
		BList 					fValidList;
		BList 					fZombieList;
		BLocker					fTrackingLock;
		
		thread_id				fThreadID;
		int32					fFileType;
		int32					fCount;
		
		bool					fOKToQuit;
		bool					fTryingToQuit;
};


