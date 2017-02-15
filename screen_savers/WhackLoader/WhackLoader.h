#include <ScreenSaver.h>
#include <Path.h>
#include <OS.h>

typedef void (*frame_whacker)(
			int32 frame_num,
			int32 *pixel_count,
			uchar *framebuffer,
			int32 bytes_per_row,
			clipping_rect *clipping_rects,
			int32 nrects,
			clipping_rect window_bounds,
			color_space mode);

class SetupView;

// Main module class
class WhackLoader : public BScreenSaver
{
	BPath				whack;
	uint8				*_framebuffer;	
	int32				_bytes_per_row;
	color_space			_mode;
	clipping_rect		_bounds;
	image_id			image;
	frame_whacker		whacker;
	SetupView			*setup;

public:
				WhackLoader(BMessage *message, image_id image);

	void		StartConfig(BView *view);

	status_t	StartSaver(BView *v, bool preview);
	void		StopSaver();
	void		DirectConnected(direct_buffer_info *info);
	void		DirectDraw(int32 frame);

	status_t	SaveState(BMessage *into) const;
};
