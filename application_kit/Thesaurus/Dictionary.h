/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

//
//	Stores a list of words and synonyms for them.

#ifndef DICTIONARY_H
#define DICTIONARY_H

#include <String.h>


class BFile;

const int32 MAX_WORD_LENGTH = 256;
const int 	BUFFER_SIZE = 1024;
const int32 NUM_BUCKETS = 6151ul;	
	// Some prime numbers
	// 	53, 97, 193, 389, 769, 1543, 3079, 6151, 12289, 24593

struct Synonym {
	BString value;
};

class Dictionary {
public:

					Dictionary();
					~Dictionary();
	void			Clear();
	status_t		ReadFromFile(const char*);
	BList*			FindAlternateWords(const char*);
	void 			AddItem(const char*, BList*);

private:

	uint32 			HashString(const char*);
	
	// Helpers for ReadFromFile
	char 			GetNextChar();
	void 			PushBack();
	const char 		*ReadUntil(char);

	// State for reading data file	
	char 			fReadBuffer[BUFFER_SIZE];
	int 			fBufferReadPos;
	int 			fBufferLength;
	BFile 			*fInputFile;		
	char 			fTokenBuffer[MAX_WORD_LENGTH];
	int 			fTokenBufferPos;	
	bool 			fEOF;

	struct HashNode {
		inline HashNode(const char *key, BList *synonyms, HashNode *next)
			:	fKey(key), fSynonymList(synonyms), fNext(next)
		{}

		inline ~HashNode() {
			for (Synonym *s = (Synonym *)fSynonymList->RemoveItem(0L);
				s; s = (Synonym *)fSynonymList->RemoveItem(0L))
				delete s;
		}
		
		BString		fKey;
		BList		*fSynonymList;
		HashNode 	*fNext;	
	} **fBuckets;
};


#endif