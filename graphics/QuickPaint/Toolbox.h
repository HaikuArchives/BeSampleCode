/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef Toolbox_h
#define Toolbox_h

#include "Tool.h"
#include "Matrix.h"

class BToolbox : public BMatrix
{
	protected:

		BTool **		m_tools;
		
	public:
						BToolbox(BRect r, int32 xMatrix, int32 yMatrix);
						~BToolbox();
				
		status_t		AddTool(BTool *tool);
		virtual	void	Select(int newSelection);
		virtual	void	DrawItem(int item, BRect itemRect);
				void	InvokeTool(BMessage *msg);
};

class BToolboxWindow : public BWindow
{
	public:
						BToolboxWindow(BRect r);

		BToolbox *		Toolbox() { return m_toolbox; };
		BColorControl *	ColorSelector() { return m_colorSelector; };

	private:
	
		BToolbox *		m_toolbox;
		BColorControl *	m_colorSelector;
};

#endif
