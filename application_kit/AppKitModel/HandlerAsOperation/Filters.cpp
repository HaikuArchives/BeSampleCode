/* Filters.cpp */
/* Three concrete classes of StringFilter */

/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include "Filters.h"
#include <String.h>
#include <Message.h>

/* Lower Case */
const char * lowercase_desc = "Convert to lowercase";
const char * lowercase_name = "LowerCase";

LowerCase::LowerCase()
	:	StringFilter(lowercase_name)
{
}

LowerCase::~LowerCase()
{
}

void 
LowerCase::Filter(BString &string)
{
	string = string.ToLower();
}

void 
LowerCase::Opcodes(BMessage *reply)
{
	reply->AddString("operations", lowercase_name);
	reply->AddInt32("opcodes", LOWERCASE);
	reply->AddString("descriptions", lowercase_desc);
}


/* UpperCase */
const char * uppercase_desc = "Convert to uppercase";
const char * uppercase_name = "UpperCase";

UpperCase::UpperCase()
	: StringFilter(uppercase_name)
{
}

UpperCase::~UpperCase()
{
}

void 
UpperCase::Filter(BString &string)
{
	string = string.ToUpper();
}

void 
UpperCase::Opcodes(BMessage *reply)
{
	reply->AddString("operations", uppercase_name);
	reply->AddInt32("opcodes", UPPERCASE);
	reply->AddString("descriptions", uppercase_desc);
}

/* MixedCase */
const char * mixedcase_desc = "Convert to mixed case";
const char * mixedcase_name = "MixedCase";

MixedCase::MixedCase()
	: StringFilter(mixedcase_name)
{
}

MixedCase::~MixedCase()
{
}

void 
MixedCase::Filter(BString &string)
{
	string = string.CapitalizeEachWord();
}

void 
MixedCase::Opcodes(BMessage *reply)
{
	reply->AddString("operations", mixedcase_name);
	reply->AddInt32("opcodes", MIXEDCASE);
	reply->AddString("descriptions", mixedcase_desc);
}

