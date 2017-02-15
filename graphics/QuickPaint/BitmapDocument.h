/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef Bitmap_h
#define Bitmap_h

#include <InterfaceKit.h>

#define	bmsgBitmapDirty		'bmpd'
#define	bmsgLayerAdded		'laya'
#define	bmsgLayerDeleted	'layd'
#define	bmsgLayersReordered	'layr'

class bgr_color {
public:
	uint8 b,g,r,a;

	bgr_color &operator=(const rgb_color &rgb)
	{
		b = rgb.blue;
		g = rgb.green;
		r = rgb.red;
		a = rgb.alpha;
		return *this;
	};
};

#define BITMAP_CHANGED		0x00000001
#define LAYER_DELETED		0x00000002
#define LAYER_ADDED			0x00000004
#define LAYERS_REORDERED	0x00000008
#define LAYERS_CHANGED		0x0000000E
#define ALL_EVENTS			0xFFFFFFFF

#define NO_SQUARES			0x00000001

class Pool {
	private:

		struct PoolEntry {
			void *data;
			PoolEntry *next;
		};
		PoolEntry *		m_pool,*m_used,*m_block;
		sem_id			m_sem;
		BLocker			m_listLock;
		
	public:
		
						Pool(int32 count);
						~Pool();
		void * 			Grab();
		void  			Release(void *);
};

struct EditorStruct {
	BView *editor;
	uint32 eventMask;
};

struct LayerStruct {
	BView *view;
	BBitmap *bitmap;
	BRegion dirty;
};

class BBitmapDocument {

	protected:

		BList 		m_editors;
		BList 		m_layers;
		sem_id		m_readerLock;
		int32		m_width,m_height;
		Pool		m_tmps;
		BLocker		m_listLock;
		
		sem_id		m_dirtySem;
		int32		m_dirtyMarker;
		thread_id	m_dirtyNotifier;

	public:

					BBitmapDocument(int32 width, int32 height);
					BBitmapDocument(BFile *f);
					~BBitmapDocument();

		BBitmap *	RealBitmap(int32 layer=0) { return ((LayerStruct*)m_layers.ItemAt(layer))->bitmap; };

		void		Lock(int32 layer=-1);
		void		Unlock(int32 layer=-1);
		void		ReadLock();
		void		ReadUnlock();

		void		AddLayer(int32 before=-1, BBitmap *bmp=NULL);
		void		DeleteLayer(int32 which);
		void		MoveLayer(int32 layer, int32 dst);
		
		void		Compose(
						uint64 layerMask, BView *theView,
						BRect dstRect, BRect onBitmap, uint32 options);

		BRect 		Bounds() { return RealBitmap()->Bounds(); };
		void *		Bits() { return RealBitmap()->Bits(); };
		color_space	ColorSpace() { return RealBitmap()->ColorSpace(); };
		int32		BitsLength() { return RealBitmap()->BitsLength(); };
		int32		BytesPerRow() { return RealBitmap()->BytesPerRow(); };
		int32		Layers() { return m_layers.CountItems(); };
				
		BView *		View(int32 layer=0) { return ((LayerStruct*)m_layers.ItemAt(layer))->view; };
		void		SetDirty(BRegion *r, int32 layer=0);
		void		AddEditor(BView *v, uint32 eventMask=ALL_EVENTS);
		void		RemoveEditor(BView *v);

static	void		DirtyNotifierLaunch(BBitmapDocument *doc);
		void		DirtyNotifier();

		void		Load(BFile *f);
		void		Store(BFile *f);

		void		ResizeTo(float width, float height);

		bgr_color 	PixelAt(int x, int y) {
			return *((bgr_color*)(((char*)Bits()) + x*4 + y*BytesPerRow()));
		};
		rgb_color 	PixelAtRGB(int x, int y) {
			bgr_color c = *((bgr_color*)(((char*)Bits()) + x*4 + y*BytesPerRow()));
			rgb_color r;
			r.red = c.r;
			r.green = c.g;
			r.blue = c.b;
			r.alpha = c.a;
			return r;
		};
		
	private:
	
		void		BroadcastMessage(BMessage *msg, uint32 eventMask);
};

#endif
