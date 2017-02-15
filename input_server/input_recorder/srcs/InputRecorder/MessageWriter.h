/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef __MESSAGE_WRITER_H
#define __MESSAGE_WRITER_H 1

#include <AppFileInfo.h>
#include <File.h>

#ifndef DATA_TYPE
#define DATA_TYPE "application/x-vnd.Be.InputRecord"
#endif
#ifndef DATA_VERSION
#define DATA_VERSION 1
#endif


/* Version differences:
 *   The first version used resource files. This approach was abandoned as a
 *     bad idea. This is the format of the first availible version of the
 *     input_recorder, from the time of Baron Arnold's newsletter article,
 *     until Steven Black's newsletter article with source.
 *
 *     Compatibility with this version has been dropped completely. Using
 *     resource files for anything other than application data is explicitly
 *     not recommended.
 *
 *   The file format version 0 was used in the version of the input_recorder
 *     first distributed with source code. It was a rush job, as most of the
 *     input_recorder had to be rewritten. Though the code design was for
 *     reuse, it had hard coded numbers for the data type, and the code didn't
 *     have any virtual functions or the goodness they provide. It didn't use
 *     any derived classes. It used a 32-bit number to describe the type of
 *     data in the file. The badness of this should be apparent.
 *
 *   The file format version 1, uses a free-form string describing the
 *     originating class which started a particular format. It needed to be
 *     flexable, and I'm unimaginative. The source at that time was changed to
 *     use virtual classes and make it easier to use these classes elsewhere.
*/

enum {
	T_BAD_VERSION = B_ERRORS_END+1
};

/*
Why two modes to write in?
1. Attributes are officially recommended for some of this type of data, so
    they're availible. However, zip files are limited to 64k of attributes per
    file. This can easily be exceeded by input records.
2. Regular files. They aren't as flexable as attributes, but they can be made
    to work. As we do not need the flexibility of Attributes, they are quite
    sufficient.
*/

/* Another note:
    Post-R4.0 the values recieved by the InputFilters were changed. (They're
    absolute instead of relative. This makes a lot of things easier.) This
    *may* break compatibility with the InputRecords you saved in R4.0. It has
    not been very well tested.

    (At Be, we do not keep input records for very long, as if any window
    location changes, the whole file is useless.)

    Input Records saved in any BeOS version after R4.0 will *not* work in
    earlier versions.
*/

const uint32 T_MAX_HEADER_SIZE = 4096;

class MessageWriter
{
public:	
	MessageWriter (void);
	virtual ~MessageWriter ();
	status_t InitCheck();
	virtual void ResetFilePointer(void) =0;
	virtual bool HaveMore(void) =0;
	virtual status_t Write(BMessage *msg) =0;
	virtual status_t Read(BMessage *msg) =0;
	
protected:
	status_t _Error; // internal error code
};

class FlatMessageWriter : public MessageWriter
{
public:	
	FlatMessageWriter (const char *name, int32 mode);
	~FlatMessageWriter ();
	virtual void ResetFilePointer(void);
	virtual bool HaveMore(void);
	virtual status_t Write(BMessage *msg);
	virtual status_t Read(BMessage *msg);
private:
	// Vendor/Classname identifier for data type
	static const char *_VendorClass;
	
	off_t _EndOfHeader; 
	off_t _LengthOfFile; 
	BFile *_File;
};

class AttributeMessageWriter : public MessageWriter
{
public:	
	AttributeMessageWriter (const char *name, int32 mode);
	~AttributeMessageWriter ();
	virtual void ResetFilePointer(void);
	virtual bool HaveMore(void);
	virtual status_t Write(BMessage *msg);
	virtual status_t Read(BMessage *msg);
private:
	// base name of attribute
	static const char *_AttrBase; 
	// Vendor/Classname identifier for data type
	static const char *_VendorClass;
	
	int32 _Item;
	BFile *_File; 
};

#endif // __MESSAGE_WRITER_H
