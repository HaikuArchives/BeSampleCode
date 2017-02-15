/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef PaintApp_h
#define PaintApp_h

#include <AppKit.h>
#include <InterfaceKit.h>
#include <ByteOrder.h>

#define bmsgToolInvocation		'tinv'
#define bmsgSave 				'save'
#define bmsgSaveAs 				'sava'
#define bmsgOpenView 			'nwvw'
#define bmsgOpenBitmap			'obmp'
#define bmsgNewBitmap 			'nbmp'
#define bmsgCurrentLayer		'lcur'
#define bmsgLayerMask			'lmsk'
#define bmsgAddLayer			'ladd'
#define bmsgRemoveLayer			'lrem'
#define bmsgShowLayer			'lshw'
#define bmsgHideLayer			'lhid'
#define bmsgExtents 			'exts'
#define bmsgLoadImage 			'limg'
#define bmsgLoadImageCallback	'licb'

#if B_HOST_IS_LENDIAN
#define ARGB_FORMAT B_RGBA32_LITTLE
struct ARGBPixel { uint8 b,g,r,a; };
#else
#define ARGB_FORMAT B_RGBA32_BIG
struct ARGBPixel { uint8 a,r,g,b; };
#endif

#define max(a,b) ((a)>(b))?a:b
#define min(a,b) ((a)<(b))?a:b

class BToolbox;

extern BColorControl 	*gColorSelector;
extern BToolbox			*gToolbox;

class PaintApp : public BApplication
{
	public:
						PaintApp();
						~PaintApp();
					
		static	bool	CanDoFormat(const char *mimeType);
		virtual	void	MessageReceived(BMessage *msg);
};

#endif
