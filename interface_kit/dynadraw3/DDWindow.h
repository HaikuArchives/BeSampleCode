/* DDWindow.h */
/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/
#include <Window.h>
#include "FilterView.h"

class DDWindow : public BWindow
{
 public:
 	DDWindow();
  	void MessageReceived(BMessage* msg);
	bool QuitRequested();
	
	inline int32 Mass()  const;
	inline int32 Drag()  const;
	inline int32 Width() const;
	inline int32 Sleep() const;
	inline rgb_color Color() const; 
	
	/* printing functions */
	void DoPrint();
	status_t SetUpPage();

 private:
 	FilterView* fv;
 	BMessage* printSettings;
	BMenuItem* scaleItem; 	

};

/* inline functions */
int32 DDWindow::Mass()  const { return fv->Mass(); }
int32 DDWindow::Drag()  const { return fv->Drag(); }
int32 DDWindow::Width() const { return fv->Width(); }
int32 DDWindow::Sleep() const { return fv->Sleep(); }
rgb_color DDWindow::Color() const { return fv->HighColor(); }