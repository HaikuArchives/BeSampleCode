#ifndef _MEDIA_SLIDER_H
#define _MEDIA_SLIDER_H

#include <View.h>


class MediaView;
class BBitmap;


class _MediaSlider_ : public BView {
public:
					_MediaSlider_(BRect frame, MediaView *owner);
	virtual			~_MediaSlider_();

	void			SetTotalTime(bigtime_t totalTime);
	void			SetCurTime(bigtime_t curTime);

	virtual void	AttachedToWindow();
	virtual void	Draw(BRect updateRect);
	virtual void	MouseDown(BPoint where);
	virtual void	MouseMoved(BPoint where, uint32 code, const BMessage *message);
	virtual void	MouseUp(BPoint where);
	virtual void	FrameResized(float width, float height);

private:
	bigtime_t		PointToTime(BPoint where);
	void			UpdateThumb(bool force);
	void			ResetBitmap();

private:
	MediaView*		fOwner;
	BBitmap*		fBitmap;
	BView*			fOffscreenView;
	bigtime_t		fTotalTime;
	bigtime_t		fCurTime;
	BRect			fThumbRect;
	bool			fMouseDown;
};


#endif
