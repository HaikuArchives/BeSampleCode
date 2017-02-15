//	Copyright 1995 Be Specific Inc., All Rights Reserved.

// $Revision: 1.1 $
#ifndef BeSpecific_GMCGRAPHICS
#define BeSpecific_GMCGRAPHICS

#include <View.h>

class AStringLabel;

//---- AMonthGraphic -----------------------------------------------------------

class AMonthGraphic
{
public:
					AMonthGraphic(BView *aView);
	virtual			~AMonthGraphic();
				
	virtual void			Invalidate();
	virtual void 			Draw (BView* aView);

			void			SetSize(BPoint newSize);
			
protected:
	virtual void			Init();
	virtual void 			CalibrateFontSizes(const long sizeIndex);
	virtual void			CalculateFontSizes();
	virtual void			CalculateColumnLabel(AStringLabel&,int);
	virtual void			CalculateDayLabels();
	virtual	void			CalculateParameters();
	virtual AStringLabel *	CreateDayLabel(const int column);

private:			
			double GridMargin;
			BPoint GridTopLeft;
			BPoint	GridCellSize;
			BPoint GridBottomRight;
			BRect GridRect;
			BPoint		fSize;			// Size of the graphic

	BView	*fView;			// The View that we will be drawing into	
	const char 		**fDayNameArray;
	long			fSizeIndex;
	float			fMonthFontSize;
	float			fNumberFontSize;
	
	bool			fImageNeedsUpdate;

    // The String labels
	AStringLabel			*fdateLabel;
    AStringLabel			*fColumn0Label;
    AStringLabel			*fColumn1Label;
    AStringLabel			*fColumn2Label;
    AStringLabel			*fColumn3Label;
    AStringLabel			*fColumn4Label;
    AStringLabel			*fColumn5Label;
    AStringLabel			*fColumn6Label;

};

#endif
