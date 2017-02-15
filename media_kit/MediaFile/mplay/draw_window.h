/*
	
	HelloWindow.h
	
	Copyright 1997 Be Incorporated, All Rights Reserved.
	
*/

#ifndef DRAW_WINDOW_H
#define DRAW_WINDOW_H

#include <Window.h>


class DrawWindow : public BWindow {
public:
					DrawWindow(BRect frame, const char *path); 
					~DrawWindow();
};


#endif //DRAW_WINDOW_H
