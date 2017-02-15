/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include <stdlib.h>
#include <stdio.h>
#include <File.h>
#include <SupportKit.h>
#include "Dictionary.h"

Dictionary::Dictionary()
	:	fBufferReadPos(0),
		fBufferLength(0),
		fInputFile(0),
		fEOF(false)
{
	fBuckets = new HashNode* [NUM_BUCKETS]; 
	memset((void*)fBuckets, 0, NUM_BUCKETS * sizeof(HashNode*));
}

Dictionary::~Dictionary()
{
	Clear();
}

void
Dictionary::Clear()
{
	for (int32 bucket = 0; bucket < NUM_BUCKETS; bucket++) {
		HashNode *node = fBuckets[bucket];		
		while (node) {
			HashNode *nextNode = node->fNext;
			delete node;
			node = nextNode;		
		}
	}
	
	delete [] fBuckets;
}

char
Dictionary::GetNextChar()
{
	if (fBufferReadPos >= fBufferLength) {
		fBufferLength = fInputFile->Read(fReadBuffer, BUFFER_SIZE);
		if (fBufferLength <= 0) {
			fEOF = true;
			return 0;
		}
		
		fBufferReadPos = 0;
	}

	return fReadBuffer[fBufferReadPos++];
}

void 
Dictionary::PushBack()
{
	ASSERT(fBufferReadPos > 0);
	fBufferReadPos--;
}

const char *
Dictionary::ReadUntil(char delimiter)
{
	fTokenBufferPos = 0;
	while (true) {
		char c = GetNextChar();
		if (c == delimiter)
			PushBack();
			
		if (c == delimiter || c == 0) {
			fTokenBuffer[fTokenBufferPos] = 0;
			return fTokenBuffer;
		}
		
		// Truncate words that are too long
		if (fTokenBufferPos < MAX_WORD_LENGTH)
			fTokenBuffer[fTokenBufferPos++] = c;
	}
}


status_t
Dictionary::ReadFromFile(const char *filename)
{
	fInputFile = new BFile(filename, B_READ_ONLY);
	if (fInputFile->InitCheck() != B_OK) {
		PRINT(("Error %s occured opening file\n", strerror(
			fInputFile->InitCheck())));
		return fInputFile->InitCheck();
	}

	while (!fEOF) {
		char c = GetNextChar();
		if (c == 10)
			break;		// End of file
			
		PushBack();

		// Get string
		const char *name = ReadUntil(':');	
		BList *wordList = new BList(10);
		AddItem(name, wordList); 

		GetNextChar();	// Eat colon

		// Read Synonym List
		while (!fEOF) {
			char c = GetNextChar();
			if (c == 0)
				break;
				
			else if (c == '-') {
				// Type Name
				GetNextChar(); 	// Eat Space
				ReadUntil(' ');
				GetNextChar(); 	// Space
				GetNextChar(); 	// Dash
				GetNextChar(); 	// Comma

				// Ignore type of word(verb, adjective, etc.)
				
			} else if (c == 10) {

				// End of list
				break;
			} else {
				// Synonym
				PushBack();
				const char *value = ReadUntil(',');
				Synonym *a = new Synonym;

				a->value = value;
				wordList->AddItem((void*) a);
				GetNextChar(); 	// Comma
			}		
		}
	}
	
	delete fInputFile;
	fInputFile = 0;
	return B_OK;
}

BList*
Dictionary::FindAlternateWords(const char *string)
{
	for (HashNode *node = fBuckets[HashString(string) % NUM_BUCKETS];
		node != 0; node = node->fNext) {
		if (strcmp(string, node->fKey.String()) == 0)
			return node->fSynonymList;		
	}

	return 0;
}

void 
Dictionary::AddItem(const char *string, BList *datum)
{
	uint32 bucket = HashString(string) % NUM_BUCKETS;	
	fBuckets[bucket] = new HashNode(string, datum, fBuckets[bucket]);
}

uint32 
Dictionary::HashString(const char *string)
{
	register uint32	hash = 0;
	for (register const char *c = string; *c; c++)
		hash = (hash << 7) ^ (hash >> 24) ^ (*c);
	
	return hash;
}