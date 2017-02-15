#ifndef _FILMSTRIP_H_
#define _FILMSTRIP_H_

#include <Bitmap.h>
#include <MediaFile.h>
#include <MediaTrack.h>

/* A FilmStrip object knows how to get images
** (single frames) out of a movie file.
*/

// FilmStrip mode constants
enum {
	kFrame,	// next frame is literally the next frame
	kTime	// next frame is mFrameInterval from current
};

enum {
	NOT_A_MOVIE = B_ERRORS_END + 1
};

class FilmStrip {
public:
	FilmStrip(const entry_ref& ref);
	FilmStrip(const entry_ref& ref,const int32 mode,
		const bigtime_t grabInterval);
	virtual ~FilmStrip(void);
	
	status_t InitCheck(void) const { return mStatus; }
	virtual BBitmap* NextFrame(void);
	static status_t SniffRef(const entry_ref& ref);
	const bigtime_t Duration(void) const;

	// getters & setters
	const bigtime_t& FrameInterval(void) const;
	void SetFrameInterval(const bigtime_t& time);
	const media_format& Format(void) const;
	const int32 Mode(void) const;
	void SetMode(const int32 mode);
	
private:
	virtual status_t SetTo(const entry_ref& ref);
	// disallow use of:
	FilmStrip(void);
	FilmStrip& operator=(const FilmStrip& rhs);
	FilmStrip(const FilmStrip& rhs);

	status_t mStatus;
	BMediaFile* mFile;
	BMediaTrack* mTrack;
	BBitmap* mFrame;
	bigtime_t mFrameInterval;
	media_format mFormat;
	int32 mMode;
};

#endif // #ifndef _FILMSTRIP_H_