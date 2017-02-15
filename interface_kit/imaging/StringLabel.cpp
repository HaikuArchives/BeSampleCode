//	Copyright 1995 Be Specific Inc., All Rights Reserved.


#include "StringLabel.h"

#include <View.h>
#include <string.h>

AStringLabel::AStringLabel(const char *aString, const BFont &theFontInfo,
	const BPoint origin)
{
	fString = new char[strlen(aString)+1];
	strcpy(fString,aString);
	fOrigin = origin;
	fFontInfo = theFontInfo;
	fAlignment = B_ALIGN_LEFT;

	fHighColor.red = 0;
	fHighColor.green = 0;
	fHighColor.blue = 0;
	fHighColor.alpha = 0;

	fLowColor.red = 167;
	fLowColor.green = 167;
	fLowColor.blue = 167;
	fLowColor.alpha = 0;
	
	Invalidate();
}

AStringLabel::AStringLabel(const char *aString, const BPoint origin, 
	const float fontSize, const BFont *font)
{
	fString = new char[strlen(aString)+1];
	strcpy(fString,aString);
	fOrigin = origin;
	fAlignment = B_ALIGN_LEFT;
	
	fFontInfo = *font;	
	fFontInfo.SetSize(fontSize);	
	fHighColor.red = 0;
	fHighColor.green = 0;
	fHighColor.blue = 0;
	fHighColor.alpha = 0;
	
	fLowColor.red = 167;
	fLowColor.green = 167;
	fLowColor.blue = 167;
	fLowColor.alpha = 0;

	Invalidate();
}

//==========================================================
// Method: Destructor
// 
// The usual cleanup destructor.  Delete the memory for the 
// string that we copied in.
//==========================================================
AStringLabel::~AStringLabel()
{
	delete [] fString;
}

//==========================================================
// Method: Draw
//
// This is the meat and potatoes method.  It is completely
// responsible for drawing the string into the view according
// to the various flags that have been set.
//
// In this method we first check to see whether things need to
// be recalculated because one of the parameters changed.
// After that, we set the text parameters, move into position,
// and draw.
//==========================================================

void
AStringLabel::Draw(BView* aView)
{
	if (fNeedsCalculation)
		Recalculate(aView);
		
	// Setting font info
	aView->SetFont(&fFontInfo);
	
	aView->SetHighColor(fHighColor);
	aView->SetLowColor(fLowColor);
	aView->MovePenTo(fStartPoint);
	aView->DrawString(fString);
}


//==========================================================
// Method: Invalidate()
//
// Invalidate simply informs the label that before it draws
// the next time, it must re-calculate its origin.  We do it
// this way instead of calling the Recalculate method directly
// becuase the sizing information is dependent on what view
// we are drawing into.  We don't know that until we actually
// draw, so that is the proper place to do the calculation.
//==========================================================

void
AStringLabel::Invalidate()
{
	fNeedsCalculation = 1;
}

//==========================================================
// Method: Recalculate
//
// Here is where we re-calculate the drawing origin based on
// the alignment, and size of the text string.  The various
// text parameters are set on the view, then the StringWidth
// method is called.  Using this information we then adjust
// the fStartPoint variable accordingly.
//==========================================================

void
AStringLabel::Recalculate(BView* /* aView */)
{
	// Start by assuming left alignment	
	fStartPoint = fOrigin;
	
	long aLength = 0;
	
	switch (fAlignment)
	{
		case B_ALIGN_LEFT:
		break;
		
		case B_ALIGN_CENTER:
			// Setting font info so we can do calculations
			aLength = (long)ceil(fFontInfo.StringWidth(fString));			
			aLength = aLength / 2;
		break;
		
		case B_ALIGN_RIGHT:
			aLength = (long)ceil(fFontInfo.StringWidth(fString));
		break;
	}
	
	fStartPoint.x = fOrigin.x - aLength;
	fNeedsCalculation = 0;
}

//==========================================================
// Method: SetText
//
// Set the text to be displayed by the label.  We make a copy
// of the null terminated string that is passed in and then
// invalidate the label.
//==========================================================

void
AStringLabel::SetText(const char *aString)
{
	delete fString;
	fString = new char[strlen(aString)+1];
	strcpy(fString,aString);
	
	Invalidate();
}

//==========================================================
// Method: SetAlignment
//
// Alignment can be:
// ALIGN_LEFT, ALIGN_CENTER, ALIGN_RIGHT
// The alignment determines how the text is displayed relative
// to the origin that is set.
//                                               Origin
//                                                   |
// ALIGN_LEFT:                              Left Aligned Label
// ALIGN_CENTER:              Center Aligned
// ALIGN_RIGHT:         Right Aligned
//==========================================================

void
AStringLabel::SetAlignment(const alignment theAlignment)
{
	fAlignment = theAlignment;
	
	Invalidate();
}

//==========================================================
// Method: SetOrigin
// 
// The origin is the starting point for where the text
// will be displayed.  Depending on the type of alignment,
// The origin will either be at the left, center, or right
// of the text.
//==========================================================
 
void
AStringLabel::SetOrigin(const BPoint& origin)
{
	fOrigin = origin;

	Invalidate();
}


//==========================================================
// Method: SetFontInfo
//
// Sets the font info for the text in one method.  This 
// allows you to change all the text attributes at once rather
// than using the individuals text attribute setting methods.
//==========================================================

void
AStringLabel::SetFontInfo(const BFont &fontInfo)
{
	fFontInfo = fontInfo;

	Invalidate();
}

//==========================================================
//	Method: GetFontInfo
// 
// This method returns the current font info for the label.
// It is best to pass in a parameter to be filled in so that
// we don't have to make a copy to return, and so we can lock
// the label while we are retrieving the information if we 
// want to.  In a multi-threaded environment, this is a must.
//==========================================================

void
AStringLabel::GetFontInfo(BFont &info) const
{
	info = fFontInfo;
}

//==========================================================
// Methods: SetShear, SetSize, SetRotation, SetFontName
//
// Set the font attributes individually.  These methods
// are handy when you are only setting one of the attributes.
//==========================================================

void
AStringLabel::SetShear(const float shear)
{
	fFontInfo.SetShear(shear);

	Invalidate();
}

void
AStringLabel::SetSize(const float fontSize)
{
	fFontInfo.SetSize(fontSize);

	Invalidate();
}

void
AStringLabel::SetRotation(const float rotation)
{
	fFontInfo.SetRotation(rotation);
	
	Invalidate();
}

void
AStringLabel::SetFontName(const char *fontName)
{
	fFontInfo.SetFamilyAndStyle(fontName,NULL);
}

//==========================================================
//	Method: SetColor
//
// Sets the color of the label.  This allows you to have
// labels of any color.
//==========================================================

void
AStringLabel::SetColor(const rgb_color aColor)
{
	fColor = aColor;
}

void
AStringLabel::SetHighColor(const rgb_color aColor)
{
	fHighColor = aColor;
}

void
AStringLabel::SetLowColor(const rgb_color aColor)
{
	fLowColor = aColor;
}


