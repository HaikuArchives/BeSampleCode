#ifndef _FILMSTRIPAPP_H_
#define _FILMSTRIPAPP_H_

#include <app/Application.h>
#include "FilmStripWindow.h"

class FilmStripApp : public BApplication {
public:
	FilmStripApp(void);
	virtual ~FilmStripApp(void);
	
	virtual void RefsReceived(BMessage* msg);
	virtual void ReadyToRun(void);
	
private:
	FilmStripWindow* mFSWindow;
};

#endif // #ifndef _FILMSTRIPAPP_H_