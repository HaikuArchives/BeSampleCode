#ifndef DRAW_H
#define DRAW_H

#include <Application.h>


const uint32 msg_WindowClosed = 'Mwcl';


class DrawApp : public BApplication {
public:
					DrawApp();
	virtual			~DrawApp();

	void			OpenOpenPanel();

	virtual void	ReadyToRun();
	virtual void	ArgvReceived(int32 argc, char **argv);
	virtual void	MessageReceived(BMessage *message);
	virtual	void	RefsReceived(BMessage *message);

private:
	BFilePanel*		fOpenPanel;

	static int32	sNumWindows;
};


#endif //DRAW_H
