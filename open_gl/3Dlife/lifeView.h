/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include <GLView.h>
#include <View.h>
#include "common.h"
#include <stdio.h>
#include <MenuItem.h>
#include <stdlib.h>

const int BOARD_SIZE = 30;

class lifeView : public BGLView
{
 public:
	lifeView(BRect R);
	~lifeView();
	
	void AttachedToWindow();
	void MessageReceived(BMessage *msg);
	
	void Initialize(int seed, bool (*board)[BOARD_SIZE][BOARD_SIZE]);
 	void SpinCalc();
	void Display(bool (*board)[BOARD_SIZE][BOARD_SIZE], int genCount,
				 bool steadyState);
	void ExitThreads();
	
	inline void SetAllAngles(float ang); 
	inline bool spinning(); 
	inline bool continuousMode();
	inline void continuousMode(bool val);
	inline bool singleStepMode();
	inline void singleStepMode(bool val);
	inline bool QuitPending();
	inline void QuitPending(bool val);	
	
 private:
 
	void DrawCube(int x, int y, int z);
	void DrawFrame(bool (*board)[BOARD_SIZE][BOARD_SIZE]);
	
	char genStr[50];

	bool continuous;
	bool singleStep;
	bool _QuitRequested;
		
	thread_id drawTID; 
	thread_id lifeTID;

	float xspin, yspin, zspin;
	int xangle, yangle, zangle;
};

// inline functions
void lifeView::SetAllAngles(float ang) {xangle = yangle = zangle = (int)ang; }

bool lifeView::spinning() { return (xspin || yspin || zspin); }

bool lifeView::continuousMode() { return continuous; }
void lifeView::continuousMode(bool val) { continuous = val; }
bool lifeView::singleStepMode() { return singleStep; }
void lifeView::singleStepMode(bool val) { singleStep = val; }
bool lifeView::QuitPending() { return _QuitRequested; }
void lifeView::QuitPending(bool val) { _QuitRequested = val; }

