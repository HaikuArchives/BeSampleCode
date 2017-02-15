/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef _pot_h
#define _pot_h

#include <Control.h>

// Class: BPot
// -----------
// The only control suitable for medicinal uses.
class BPot : public BControl
{
public:
	BPot(BRect frame, const char *name,
		const char *label, BMessage *message,
		int32 minValue, int32 maxValue,
		uint32 resizingMode = B_FOLLOW_LEFT | B_FOLLOW_TOP,
		uint32 flags = B_FRAME_EVENTS | B_WILL_DRAW | B_NAVIGABLE);
	virtual ~BPot();
	
	void GetValueRange(int32* min, int32* max) const;
	void SetValueRange(int32 min, int32 max);
	void SetLimitLabels(const char *minLabel,
		const char *maxLabel);
	const char *MinLimitLabel() const;
	const char *MaxLimitLabel() const;
	
	void SetValue(int32 newValue);
	float SetAngle(float newDegrees);
	float Angle() const;
	virtual int32 ValueForAngle(float degrees) const;
	virtual float AngleForValue(int32 value) const;
	
	void SetMarked(bool marked);
	bool IsMarked() const;
	
	virtual void AttachedToWindow();
	virtual void MessageReceived(BMessage* msg);
	virtual void MouseDown(BPoint where);
	virtual void MouseMoved(BPoint where, uint32 code,
		const BMessage *dragDropMsg);
	virtual void MouseUp(BPoint where);	
	
	virtual void Draw(BRect updateRect);
	
	void DrawBoilerplate(BRect updateRect);
	void DrawLabels(BRect updateRect);
	void DrawKnob(BRect updateRect);

	float CalcAngleIncr(BPoint prev, BPoint cur);
	float GetDragAngle(BPoint prev, BPoint cur);
	
	BPoint KnobOrigin() const;
	BRect KnobRect() const;
	float KnobRadius() const;
	
private:
	int32 m_fMinValue, m_fMaxValue;
	float m_fMinAngle, m_fMaxAngle;
	float m_fDragAngle;
	char *m_szMin, *m_szMax;
	bool m_bMark;
	
	BRect m_rectKnob;
	
	BPoint m_ptPrev;
};

#endif /* _pot_h */
