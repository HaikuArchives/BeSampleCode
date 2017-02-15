/*
	
	FileDock.h

*/

/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef _FileDock_h
#define _FileDock_h

#include <View.h>
#include <Invoker.h>

class FileDock : public BView, public BInvoker
{
public:
	FileDock(BRect r, const char* name);
	virtual ~FileDock();
	
	virtual void Draw(BRect updateRect);
	
	virtual void MouseDown(BPoint where);
	virtual void MouseMoved(BPoint where, uint32 transit,
		const BMessage* dragMsg);
	virtual void MouseUp(BPoint where);
	virtual void MessageReceived(BMessage* message);
	
	virtual void AttachedToWindow();
	virtual void GetPreferredSize(float* width, float* height);
	virtual void ResizeToPreferred();
	
	virtual status_t Invoke(BMessage* message = 0);
	
	const entry_ref* GetRef() const { return m_ref; }
	
protected:
	virtual bool IsValidRef(const entry_ref* ref);
	
private:
	BRect IconFrame() const;
	
	void DrawFrame(BRect updateRect);
	void DrawIcon(BRect updateRect);
	
	void SetRef(const entry_ref* ref);
	void SetIcon(const entry_ref* ref);
	
	void HandleDrop(BMessage* message);
	BBitmap* MakeDragBitmap();
	
	entry_ref* m_ref;
	BBitmap* m_icon;
	bool m_trackMouse;
	bool m_highlight;
	const BMessage* m_dragMsg;
};

#endif /* _FileDock_h */