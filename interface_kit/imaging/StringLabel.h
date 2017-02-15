/*
//	Copyright 1995 Be Specific Inc., All Rights Reserved.
*/

#ifndef STRINGLABEL_H
#define STRINGLABEL_H

#include <Point.h>
#include <View.h>
#include <Font.h>

//==========================================================
// Class: AStringLabel
//
// This class serves as a lightweight way of drawing a string into any 
// view.  It is more lightweight than using a View, and it has more 
// parameters for displaying the text than a BStringView.
//==========================================================

class AStringLabel 
{
public:
				AStringLabel(const char *aString, const BFont &, 
					const BPoint origin=BPoint(0,0));
				AStringLabel(const char *aString, const BPoint origin=BPoint(0,0), 
					const float fontSize=12.0, const BFont *font=be_plain_font);

		virtual ~AStringLabel();
		
		void	Draw(BView* aView);
		
		void	Invalidate();
		void	Recalculate(BView *);
		
		void	SetText(const char *aString);
		void	SetAlignment(const alignment theAlignment);
		void	SetOrigin(const BPoint& origin);
		void	SetFontInfo(const BFont &);
		void	SetFontName(const char *aFontName);
		void	SetShear(const float shear=90);
		void	SetSize(const float fontSize);
		void	SetRotation(const float rotation);
		void	SetHighColor(const rgb_color);
		void	SetLowColor(const rgb_color);
		void	SetColor(const rgb_color);
		
		void	GetFontInfo(BFont &info) const;
		
protected:
	BFont			fFontInfo;
	rgb_color		fColor;
	rgb_color		fHighColor;
	rgb_color		fLowColor;
	alignment		fAlignment;
	char			*fString;
	BPoint		fOrigin;
	BPoint		fStartPoint;
	int			fNeedsCalculation;
	
private:
};

#endif

