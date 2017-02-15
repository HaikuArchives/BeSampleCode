/* Filters.h */
/* Three concrete classes of StringFilter */

/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef FILTERS_H
#define FILTERS_H

#include "StringFilter.h"
#include "StringFilterProtocol.h"


const uint32 LOWERCASE = 'lwrc';
extern const char *lowercase_name;

class LowerCase : public StringFilter {
	public:
							LowerCase();
		virtual				~LowerCase();
	private:
		void				Filter(BString &string);
		void				Opcodes(BMessage *reply);					
};


const uint32 UPPERCASE = 'uprc';
extern const char * uppercase_name;

class UpperCase : public StringFilter {
	public:
							UpperCase();
		virtual				~UpperCase();
	private:
		void				Filter(BString &string);
		void				Opcodes(BMessage *reply);					
};


const uint32 MIXEDCASE = 'mixc';
extern const char *mixedcase_name;

class MixedCase : public StringFilter {
	public:
							MixedCase();
		virtual				~MixedCase();
	private:
		void				Filter(BString &string);
		void				Opcodes(BMessage *reply);					
};

#endif
