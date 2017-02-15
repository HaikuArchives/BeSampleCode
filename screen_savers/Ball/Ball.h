#include <ScreenSaver.h>

class BallView;

// Main module class
class Ball : public BScreenSaver
{
	BallView	*ball;

public:
				Ball(BMessage *message, image_id image);

	void		StartConfig(BView *view);

	status_t	StartSaver(BView *v, bool preview);
	void		StopSaver();
	void		DirectConnected(direct_buffer_info *info);
	void		DirectDraw(int32 frame);
};
