/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include <malloc.h>
#include "PaintApp.h"
#include "BitmapDocument.h"

#define MAX_READER_COUNT 10000

void * Pool::Grab()
{
	acquire_sem(m_sem);
	m_listLock.Lock();
	PoolEntry *e = m_pool;
	m_pool = m_pool->next;
	e->next = m_used;
	m_used = e;
	void *data = e->data;
	m_listLock.Unlock();
	return data;
};

void Pool::Release(void *data)
{
	m_listLock.Lock();
	PoolEntry *e = m_used;
	m_used = m_used->next;
	e->next = m_pool;
	m_pool = e;
	e->data = data;
	m_listLock.Unlock();
	release_sem(m_sem);
};

Pool::Pool(int32 count)
{
	m_sem = create_sem(0,"poolsem");
	m_block = (PoolEntry*)malloc(sizeof(PoolEntry)*count);
	m_pool = m_used = NULL;
	for (int32 i=0;i<count;i++) {
		m_block[i].next = m_used;
		m_used = m_block+i;
	};
};

Pool::~Pool()
{
	delete_sem(m_sem);
	free(m_block);
};

void BBitmapDocument::Load(BFile * /* f */)
{
/*
	if (m_bitmap) delete m_bitmap;

	BRect r;
	color_space cs;
	int32 bitslen;
	f->Read(&r,sizeof(r));
	f->Read(&cs,sizeof(cs));
	f->Read(&bitslen,sizeof(bitslen));
	m_bitmap = new BBitmap(r,cs,TRUE);
	f->Read(m_bitmap->Bits(),bitslen);

	m_view = new BView(r,"BitmapDrawer",B_FOLLOW_ALL,0);
	m_bitmap->AddChild(m_view);
*/
};

void BBitmapDocument::Store(BFile * /* f */)
{
/*
	if (!m_bitmap) return;

	BRect r = m_bitmap->Bounds();
	color_space cs = m_bitmap->ColorSpace();
	int32 bitslen = m_bitmap->BitsLength();
	f->Write(&r,sizeof(r));
	f->Write(&cs,sizeof(cs));
	f->Write(&bitslen,sizeof(bitslen));
	f->Write(m_bitmap->Bits(),bitslen);
*/
};

void BBitmapDocument::ResizeTo(float /* width */, float /* height */)
{
};

BBitmapDocument::BBitmapDocument(BFile *f) : m_tmps(2)
{
	m_readerLock = create_sem(MAX_READER_COUNT,"readerLock");
	Load(f);
};

BBitmapDocument::BBitmapDocument(int32 width, int32 height) : m_tmps(2)
{
	m_width = width;
	m_height = height;
	AddLayer(0);
	m_readerLock = create_sem(MAX_READER_COUNT,"readerLock");
	m_dirtySem = create_sem(0,"dirtySem");
	m_dirtyMarker = 0;
	
	for (int32 i=0;i<2;i++) {
		BBitmap *bm = new BBitmap(BRect(0,0,m_width-1,m_height-1),ARGB_FORMAT,TRUE);
		BRect r(0,0,m_width-1,m_height-1);
		BView *v = new BView(r,"draw",B_FOLLOW_ALL,0);
		bm->Lock();
		bm->AddChild(v);
		bm->Unlock();
		m_tmps.Release(bm);
	};

	resume_thread(m_dirtyNotifier=spawn_thread((thread_entry)DirtyNotifierLaunch,"DirtyNotifier",B_NORMAL_PRIORITY,this));
};

BBitmapDocument::~BBitmapDocument()
{
	int32 dummy;
	delete_sem(m_dirtySem);
	wait_for_thread(m_dirtyNotifier,&dummy);
	while (m_layers.CountItems()) DeleteLayer(0);
	delete_sem(m_readerLock);
};

void BBitmapDocument::AddLayer(int32 before, BBitmap *bmp)
{
	if (before == -1) before = m_layers.CountItems();

	LayerStruct *l = new LayerStruct;
	l->bitmap = new BBitmap(BRect(0,0,m_width-1,m_height-1),ARGB_FORMAT,TRUE);
	BRect r(0,0,m_width-1,m_height-1);
	l->view = new BView(r,"BitmapDrawer",B_FOLLOW_ALL,B_SUBPIXEL_PRECISE);

	l->bitmap->Lock();
	l->bitmap->AddChild(l->view);
	l->view->SetHighColor(255,255,255,0);
	l->view->SetDrawingMode(B_OP_COPY);
	l->view->FillRect(r);
	if (bmp) {
		BRect sb = bmp->Bounds();
		BRect db = l->bitmap->Bounds();
		BPoint offs;
		offs.x = floor((db.IntegerWidth()+1)/2 - (sb.IntegerWidth()+1)/2);
		offs.y = floor((db.IntegerHeight()+1)/2 - (sb.IntegerHeight()+1)/2);
		db = sb;
		db.OffsetBy(offs);
		l->view->DrawBitmap(bmp,db);
	};
	l->view->Sync();
	l->bitmap->Unlock();

	m_listLock.Lock();
	m_layers.AddItem(l,before);
	m_listLock.Unlock();
	
	BMessage msg(bmsgLayerAdded);
	msg.AddInt32("layer",before);
	BroadcastMessage(&msg,LAYER_ADDED);
};

void BBitmapDocument::DeleteLayer(int32 which)
{
	m_listLock.Lock();
	LayerStruct *l = (LayerStruct*)m_layers.ItemAt(which);
	if (l) {
		l->bitmap->Lock();
		delete l->bitmap;
		m_layers.RemoveItem(which);
		BMessage msg(bmsgLayerDeleted);
		msg.AddInt32("layer",which);
		BroadcastMessage(&msg,LAYER_DELETED);
	};
	m_listLock.Unlock();
};

void BBitmapDocument::MoveLayer(int32 layer, int32 dst)
{
	m_listLock.Lock();
	LayerStruct *l = (LayerStruct*)m_layers.RemoveItem(layer);
	int32 newDst = dst;
	m_layers.AddItem(l,newDst);
	BMessage msg(bmsgLayersReordered);
	msg.AddInt32("layer",layer);
	msg.AddInt32("newLoc",dst);
	BroadcastMessage(&msg,LAYERS_REORDERED);
	m_listLock.Unlock();
}

void BBitmapDocument::AddEditor(BView *v, uint32 eventMask)
{
	EditorStruct *e = new EditorStruct;
	e->editor = v;
	e->eventMask = eventMask;
	m_editors.AddItem(e);
};

void BBitmapDocument::RemoveEditor(BView *v)
{
	for (int32 i=0;i<m_editors.CountItems();i++) {
		EditorStruct *e = (EditorStruct*)m_editors.ItemAt(i);
		if (e->editor == v) {
			m_editors.RemoveItem(i);
			delete e;
			return;
		};
	};
};

#define RECT_SIZE 10
void BBitmapDocument::Compose(uint64 layerMask, BView *v, BRect dstRect, BRect onBitmap, uint32 options)
{
	static rgb_color flip[2] = {{220,220,220,255},{120,120,120,255}};
	static int32 flop[2] = {0,RECT_SIZE};
	int32 right = (int32)onBitmap.right;
	int32 bottom = (int32)onBitmap.bottom;
	int32 left,top,startLeft,startTop,flipflop,offset;
	
	startLeft = ((int32)onBitmap.left)/RECT_SIZE; 
	startTop = ((int32)onBitmap.top)/RECT_SIZE;
	flipflop = (startTop+startLeft)%2;
	startLeft *= RECT_SIZE;
	startTop *= RECT_SIZE;
	
	BBitmap * bm = (BBitmap*)m_tmps.Grab();
	BView *tv = bm->FindView("draw");
	bm->Lock();
	
	BRegion reg;
	reg.Set(onBitmap);
	tv->ConstrainClippingRegion(&reg);
	tv->SetDrawingMode(B_OP_COPY);

	if (options & NO_SQUARES) {
		tv->SetHighColor(255,255,255,255);
		tv->FillRect(onBitmap);
	} else {
		for (int32 i=0;i<2;i++) {
			offset = flop[flipflop];
			tv->SetHighColor(flip[i]);
			top = startTop;
			while (top <= bottom) {
				left = startLeft + offset;
				while (left <= right) {
					tv->FillRect(BRect(left,top,left+RECT_SIZE-1,top+RECT_SIZE-1));
					left += 2*RECT_SIZE;
				};
				top += RECT_SIZE;
				if (offset) offset = 0; else offset = RECT_SIZE;
			};
			if (flipflop) flipflop = 0; else flipflop = 1;
		};
	};
	
	tv->SetBlendingMode(B_PIXEL_ALPHA,B_ALPHA_OVERLAY);
	tv->SetDrawingMode(B_OP_ALPHA);
	
	m_listLock.Lock();
	for (int32 i=m_layers.CountItems()-1;i>=0;i--) {
		if ((layerMask>>i) & 1) {
			LayerStruct *l = (LayerStruct*)m_layers.ItemAt(i);
			tv->DrawBitmap(l->bitmap);
		};
	};
	m_listLock.Unlock();

	tv->Sync();
	bm->Unlock();
	v->DrawBitmap(bm,dstRect);
	m_tmps.Release(bm);
};

void BBitmapDocument::BroadcastMessage(BMessage *msg, uint32 eventMask)
{
	for (int i=0; i<m_editors.CountItems(); i++) {
		EditorStruct *e = (EditorStruct*)m_editors.ItemAt(i);
		if (e->eventMask & eventMask) {
			BLooper *loop = e->editor->Looper();
			if (loop) loop->PostMessage(msg,e->editor);
		};
	};	
};

void BBitmapDocument::DirtyNotifierLaunch(BBitmapDocument *doc)
{
	doc->DirtyNotifier();
};

void BBitmapDocument::DirtyNotifier()
{
	BMessage msg(bmsgBitmapDirty);
	msg.AddInt32("layer",-1);
	while (1) {
		if (acquire_sem(m_dirtySem) != B_OK) return;
		m_listLock.Lock();
		m_dirtyMarker = 0;
		for (int32 i=0;i<m_layers.CountItems();i++) {
			LayerStruct *l = (LayerStruct*)m_layers.ItemAt(i);
			if (l->dirty.Frame().IsValid()) {
				msg.ReplaceInt32("layer",i);
				msg.RemoveName("rect");
				for (int i=0;i<l->dirty.CountRects();i++)
					msg.AddRect("rect",l->dirty.RectAt(i));
				BroadcastMessage(&msg,BITMAP_CHANGED);
				l->dirty.MakeEmpty();
			};
		};
		m_listLock.Unlock();
	};
};

void BBitmapDocument::SetDirty(BRegion *r, int32 layer)
{
	m_listLock.Lock();
	LayerStruct *l = (LayerStruct*)m_layers.ItemAt(layer);
	l->dirty.Include(r);
	m_listLock.Unlock();
	if (!atomic_add(&m_dirtyMarker,1)) release_sem(m_dirtySem);
};

void BBitmapDocument::Lock(int32 layer)
{
	acquire_sem_etc(m_readerLock,MAX_READER_COUNT,0,0);
	if (layer == -1) {
		for (int32 i=0;i<m_layers.CountItems();i++) {
			LayerStruct *l = (LayerStruct*)m_layers.ItemAt(i);
			l->bitmap->Lock();
		};
	} else {
		LayerStruct *l = (LayerStruct*)m_layers.ItemAt(layer);
		if (l) l->bitmap->Lock();
	};
};

void BBitmapDocument::Unlock(int32 layer)
{
	if (layer == -1) {
		for (int32 i=0;i<m_layers.CountItems();i++) {
			LayerStruct *l = (LayerStruct*)m_layers.ItemAt(i);
			l->bitmap->Unlock();
		};
	} else {
		LayerStruct *l = (LayerStruct*)m_layers.ItemAt(layer);
		if (l) l->bitmap->Unlock();
	};
	release_sem_etc(m_readerLock,MAX_READER_COUNT,0);
};

void BBitmapDocument::ReadLock()
{
	acquire_sem(m_readerLock);
};

void BBitmapDocument::ReadUnlock()
{
	release_sem(m_readerLock);
};
