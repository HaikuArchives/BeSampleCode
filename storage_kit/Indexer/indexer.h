/* indexer.h */
/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef INDEXER_H
#define INDEXER_H

#include "Defs.h"

#include <Application.h>
#include <posix/sys/stat.h>

const char ABOUT_INFO[] = "Indexer:\n\tSample Application from Be Developer Technical Support";

class Indexer : public BApplication {
	public:
							Indexer();
							~Indexer();
							
		void				ReadyToRun();
		void				MessageReceived(BMessage *msg);
		void				ArgvReceived(int32 argc, char **argv);
		void				RefsReceived(BMessage *msg);
		void				AboutRequested();

	private:
		status_t			EvaluateRef(entry_ref &ref);
		status_t			HandleVolume(entry_ref &ref, struct stat &st, BDirectory &dir);
		status_t			HandleDirectory(entry_ref &ref, struct stat &st, BDirectory &dir);
		status_t			HandleFile(entry_ref &ref, struct stat &st);
		status_t			HandleLink(entry_ref &ref, struct stat &st);

};

#endif
