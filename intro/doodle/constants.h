/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef _constants_h
#define _constants_h

#include <SupportDefs.h>
#include <Point.h>
#include <InterfaceDefs.h>

/////////////////////////////////////////////////////////////////////////////
// Message constants
// MFC NOTE: Analagous to ID_xxx constants in Visual C++.
// Note that the values are twice as big.

const uint32 MSG_FILE_NEW				= 'mFNW';
const uint32 MSG_FILE_OPEN				= 'mFOP';
const uint32 MSG_FILE_SAVE				= 'mFSV';
const uint32 MSG_FILE_SAVE_AS			= 'mFSA';
const uint32 MSG_FILE_PRINT				= 'mFPR';
const uint32 MSG_FILE_PAGE_SETUP		= 'mFPS';
const uint32 MSG_EDIT_CLEAR_ALL			= 'mECA';
const uint32 MSG_PEN_THICK_OR_THIN		= 'mPTT';
const uint32 MSG_PEN_WIDTHS				= 'mPWD';
const uint32 MSG_WIN_NEW_WINDOW			= 'mWNW';
const uint32 MSG_WIN_CASCADE			= 'mWCS';
const uint32 MSG_WIN_TILE				= 'mWTH';
const uint32 MSG_WIN_TILE_VERTICAL		= 'mWTV';

// Message: Update document title (MSG_DOC_TITLE_CHANGED)
// What: 'mDTC'
const uint32 MSG_DOC_TITLE_CHANGED		= 'mDTC';

// Message: Add MRU Item (MSG_ADD_MRU)
// What: 'mMRU'
// Contents:
// "refs" (Ref): List of entry references to add to MRU list.
const uint32 MSG_ADD_MRU				= 'mMRU';

// Message: MSG_DOC_CONTENTS_CHANGED
// What: 'mVUP'
// Contents:
// "bounds" (Rect): Rectangle to be invalidated
const uint32 MSG_DOC_CONTENTS_CHANGED	= 'mVUP';

/////////////////////////////////////////////////////////////////////////////
// Dialog message constants

const uint32 DLG_OK						= 'mDOK';
const uint32 DLG_CANCEL					= 'mDCL';
const uint32 DLG_PWD_DEFAULT			= 'mDPD';

/////////////////////////////////////////////////////////////////////////////
// Common Colors

// Constant: BKG_GREY
// normal BeOS background grey
const rgb_color BKG_GREY			= { 216, 216, 216 };

// Constant: TOOLBAR_GREY
// grey for the toolbar background, slightly darker than normal
// for better visibility
const rgb_color TOOLBAR_GREY		= { 200, 200, 200 };

const rgb_color PURE_WHITE			= { 255, 255, 255 };
const rgb_color PURE_BLACK			= { 0, 0, 0 };

/////////////////////////////////////////////////////////////////////////////
// Other constants

const float DEFAULT_THIN_SIZE = 2.0f;
const float DEFAULT_THICK_SIZE = 5.0f;
const bigtime_t PULSE_EVENT_RATE = 100000; // 1/10 sec.

#endif /* _constants_h */