// $Revision: 1.1 $
//	Copyright (C)1995 Be Specific, Inc.  All rights reserved.


/*
	Author: William Adams
	Creation Date: 20 Aug 93
	
	Purpose:
	Implements the Gregorian View of a month
	Sunday = 0, Saturday = 6

	Sun Mon Tue Wed Thu Fri Sat
*/

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <Window.h>

#include "GMCGraphics.h"
#include "StringLabel.h"

#include <View.h>




//=======================================================

static const char *GMCsingledaynames[] =
{
	"S",
	"M",
	"T",
	"W",
	"T",
	"F",
	"S"
};

static const char *GMCshortdaynames[] =
{
	"Sun",
	"Mon",
	"Tue",
	"Wed",
	"Thu",
	"Fri",
	"Sat"
};

static const char *GMCdaynames[] =
{
	"Sunday",
	"Monday",
	"Tuesday",
	"Wednesday",
	"Thursday",
	"Friday",
	"Saturday"
};

static const char *GMCshortmonthnames[] =
{"",
	"Jan",
	"Feb",
	"Mar",
	"Apr",
	"May",
	"Jun",
	"Jul",
	"Aug",
	"Sep",
	"Oct",
	"Nov",
	"Dec"};

static const char *GMCmonthnames[] =
{"",
	"January",
	"February",
	"March",
	"April",
	"May",
	"June",
	"July",
	"August",
	"September",
	"October",
	"November",
	"December"};

static const int GMCmonthlengths[] = {31,28,31,30,31,30,31,31,30,31,30,31};

static AStringLabel* GDayTextArray[32];

static inline bool GMCisleap(int aYear)
{
    int newYear = aYear;
	bool leaping = FALSE;
	
    if (newYear < 100)
		newYear += 1900;

	leaping = (((((newYear)%4) == 0 && (((newYear)%100)!=0)) ||
		((newYear)%400)==0));
		
	return leaping;
}

static inline long GMCdaysInMonth(int m, int y)
{
    if ((m==2)&& GMCisleap(y))
        return 29;
    else
        return GMCmonthlengths[m-1];
}


//........................................................................................
AMonthGraphic::AMonthGraphic(BView *aView)
	: fSize(100,100),
	fView(aView),
	fSizeIndex(0),
	fdateLabel(0),
	fColumn0Label(0),
	fColumn1Label(0),
	fColumn2Label(0),
	fColumn3Label(0),
	fColumn4Label(0),
	fColumn5Label(0),
	fColumn6Label(0)
{		
	fView->Window()->Lock();
	Init();
	fView->Window()->Unlock();
}


//........................................................................................
AMonthGraphic::~AMonthGraphic()
{
}


//........................................................................................
void
AMonthGraphic::Init()
{
	GridMargin = 4.0;
	GridTopLeft = BPoint(11.0, 43.0);
	GridCellSize = BPoint(34.0,28.0);
	GridBottomRight = BPoint(GridTopLeft.x+(7*GridCellSize.x),
				GridTopLeft.y+(6*GridCellSize.y));
	GridRect = BRect(GridTopLeft, GridBottomRight);	
	
    // Get the correct text to be displayed
	long value = 1;
	
	if (value > 12 || value < 0) 
		value = 1;
    
    const char *monthname = GMCmonthnames[value];
    char datestring[64]={0};
	sprintf(datestring,"%s -  %ld",monthname, 1996L);
	
	fdateLabel = new AStringLabel (datestring, BPoint(14,26),fMonthFontSize);

	// Create the day labels for each column
	fColumn0Label = CreateDayLabel(0);
	fColumn1Label = CreateDayLabel(1);
	fColumn2Label = CreateDayLabel(2);
	fColumn3Label = CreateDayLabel(3);
	fColumn4Label = CreateDayLabel(4);
	fColumn5Label = CreateDayLabel(5);
	fColumn6Label = CreateDayLabel(6);
	
	// Fill in the initial text array
	for (long counter=0; counter < 32; counter++)
	{
		char daystring[12]={0};
		sprintf(daystring,"%ld",counter);
		
		GDayTextArray[counter] = new AStringLabel(daystring,BPoint(0,0),fNumberFontSize);
	}
	
	// Set the initial size
	BPoint boundsSize(fView->Bounds().Width(),fView->Bounds().Height());
	SetSize(boundsSize);
}

void AMonthGraphic::SetSize(BPoint size)
{
	fView->Window()->Lock();
	fSize = size;
	Invalidate();
	fView->Window()->Unlock();
}

void AMonthGraphic::Invalidate()
{	
	// Do the resizing magic
	// Re-calculate the various size things
	CalculateParameters();
	CalculateDayLabels();
	
	// Make sure the view draws itself
	fImageNeedsUpdate = 1;
	
}


void AMonthGraphic::CalibrateFontSizes(const long sizeIndex)
{
	#ifdef DEBUG
	printf("CalibrateFontSizes - %d %d\n",fSizeIndex, sizeIndex);
	#endif
	
	// If the new index is the same as the old, then do nothing
	if (sizeIndex == fSizeIndex)
		return;
		
	switch (sizeIndex)
	{
		case 100:
			fNumberFontSize = 8.0;
			fMonthFontSize = 10.0;
		break;
		
		case 140:
			fNumberFontSize = 9.0;
			fMonthFontSize = 12.0;
		break;
		
		case 378:
			fNumberFontSize = 10.0;
			fMonthFontSize = 14.0;
		break;
		
		case 500:
			fNumberFontSize = 12.0;
			fMonthFontSize = 18.0;
		
		break;
		
		default:
			fNumberFontSize = 18.0;
			fMonthFontSize = 24.0;
		break;
	}
	
	#ifdef DEBUG
	printf("Calibrate - Resetting Text for numbers\n");
	#endif
	
	// Set the point size of the number displays
	for (long counter=0; counter < 32; counter++)
	{
		GDayTextArray[counter]->SetSize(fNumberFontSize);
	}
	
	fSizeIndex = sizeIndex;
}

void
AMonthGraphic::CalculateFontSizes()
{
	BRect bounds = fView->Bounds();
	long sizeIndex = 100;
	
	if (bounds.Width() < 100.0)
	{
		sizeIndex = 100;
	} else if (bounds.Width() < 140.0)
	{
		sizeIndex = 140;
	} else if (bounds.Width() < 378.0)
	{
		sizeIndex = 378;
	}else if (bounds.Width() < 500.0)
	{
		sizeIndex = 500;
	}else
	{
		sizeIndex = 1000;
	}
	
	CalibrateFontSizes(sizeIndex);
}

void
AMonthGraphic::CalculateParameters()
{
	// Do the calculation of the month stuff first
	// because everything else depends on it.
	CalculateFontSizes();

	// Re-Calculate the grid sizes and whatnot based on the clipped
	// size of the view.  Thus we can shrink the display to always
	// fit within the confines of the displayed area.
	BRect bounds = fView->Bounds();

	GridTopLeft = BPoint(GridMargin, fMonthFontSize+fNumberFontSize+GridMargin*3);
	GridCellSize= BPoint((bounds.Width()-GridMargin*2)/7,
						  ((bounds.Height()-GridMargin*2-GridTopLeft.y)/6));
	GridBottomRight = BPoint(GridTopLeft.x+(7*GridCellSize.x),
						GridTopLeft.y+(6*GridCellSize.y));
	GridRect	= BRect(GridTopLeft, GridBottomRight);
}

void
AMonthGraphic::CalculateColumnLabel(AStringLabel &theLabel, int theColumn)
{
	// Calculate an origin such that the text will be centered
	BPoint center( GridTopLeft.x+theColumn*GridCellSize.x + (GridCellSize.x/2), GridTopLeft.y-3.0);
	
	theLabel.SetText(fDayNameArray[theColumn]);
    theLabel.SetOrigin(center);
	theLabel.SetSize(fNumberFontSize);
	theLabel.SetAlignment(B_ALIGN_CENTER);
}

void
AMonthGraphic::CalculateDayLabels()
{
	const char **monthnameArray;

	// determine the array to use for weekday names based on the width
	// of the columns.
	if (GridCellSize.x < 20.0)
	{
		fDayNameArray = GMCsingledaynames;
		monthnameArray = GMCshortmonthnames;
	} else if (GridCellSize.x < 54.0)
	{
		fDayNameArray = GMCshortdaynames;
		monthnameArray = GMCshortmonthnames;
	} else
	{
		fDayNameArray = GMCdaynames;
		monthnameArray = GMCmonthnames;
	}

	// Create and assign a name for the month label
    const char *monthname = GMCmonthnames[1];
    char datestring[64]={0};
	sprintf(datestring,"%s -  %ld",monthname, 1996L);

	fdateLabel->SetText(datestring);
    fdateLabel->SetOrigin(BPoint (14.0,fMonthFontSize + GridMargin));
	fdateLabel->SetSize(fMonthFontSize);

	// Now Setup the day headings
	CalculateColumnLabel(*fColumn0Label, 0);
	CalculateColumnLabel(*fColumn1Label, 1);
	CalculateColumnLabel(*fColumn2Label, 2);
	CalculateColumnLabel(*fColumn3Label, 3);
	CalculateColumnLabel(*fColumn4Label, 4);
	CalculateColumnLabel(*fColumn5Label, 5);
	CalculateColumnLabel(*fColumn6Label, 6);
}


AStringLabel *
AMonthGraphic::CreateDayLabel(const int column)
{
	const char **daynameArray;

	// determine the array to use for weekday names based on the width
	// of the columns.
	// If they are less than 58, then use the short names
	if (GridCellSize.x < 54.0)
	    daynameArray = GMCshortdaynames;
	else
		daynameArray = GMCdaynames;

	AStringLabel *newLabel = new AStringLabel(daynameArray[column]);
	//newLabel->SetColor(TTextColorStyle::GetBlack());
    newLabel->SetOrigin(BPoint (column*GridCellSize.x +GridTopLeft.x,40.0));
	newLabel->SetSize(fNumberFontSize);

	return newLabel;
}


/*
//........................................................................................
	Method: Draw
 
	Purpose:  Draw the day numbers and any other shading specific to the days.

*/
void
AMonthGraphic::Draw(BView *aPort)
{
/*
	
	// Re-Calculate the grid sizes and whatnot based on the clipped
	// size of the view.  Thus we can shrink the display to always
	// fit within the confines of the displayed area.
*/
	// BRect clipRect = fView->Bounds();
	
		
	if (fImageNeedsUpdate)
		Invalidate();
	CalculateDayLabels();

	// Use B_OP_OVER so that things show up nicely
	aPort->SetDrawingMode(B_OP_OVER);

	// FIRST DRAW THE BACKGROUND
	// Draw a light gray background

	aPort->SetHighColor(167,167,167);	
	aPort->FillRect(fView->Bounds());
	
	// Draw the outline that will hold the grid
	aPort->SetHighColor(255,255,255);	
	aPort->FillRect(GridRect);
	aPort->SetHighColor(0,0,0);	
	aPort->StrokeRect(GridRect);
		
    // Draw the text label for the month
	fdateLabel->Draw(aPort);
	
	// Draw the day labels
	fColumn0Label->Draw(aPort);
	fColumn1Label->Draw(aPort);
	fColumn2Label->Draw(aPort);
	fColumn3Label->Draw(aPort);
	fColumn4Label->Draw(aPort);
	fColumn5Label->Draw(aPort);
	fColumn6Label->Draw(aPort);
	
	// Draw the vertical lines in the grid.
	float xStart = GridTopLeft.x+1.0;
	float xEnd = GridBottomRight.x-1.0;
	float yStart = GridTopLeft.y+1.0;
	float yEnd = GridBottomRight.y-1.0;
	int counter = 0;

	
	// If the cell height is greater than 15.0 then draw the vertical
	// lines, otherwise they just get in the way.
	if (GridCellSize.y > 15.0)
	{
		rgb_color blackColor={0,0,0,0};
		fView->BeginLineArray(7);	
		for (counter = 1; counter < 7; counter++)
		{
			BPoint startPoint(xStart+(counter*GridCellSize.x)-2,yStart);
			BPoint endPoint(xStart+(counter*GridCellSize.x)-2,yEnd);
			//fView->StrokeLine(startPoint,endPoint);
			fView->AddLine(startPoint, endPoint, blackColor);
		}
		fView->EndLineArray();
	}
	
	// Draw the 5 horizontal lines in the grid.
	for (counter = 1; counter < 6; counter++)
	{	    
		BPoint startPoint(xStart,yStart+(counter*GridCellSize.y));
		BPoint endPoint(xEnd,yStart+(counter*GridCellSize.y));
		fView->StrokeLine(startPoint,endPoint);
	}

	// NOW DRAW EVERYTHING ELSE	
	// Get the day of the week of the first day in the month
	int dow = 1;
	
	// Get the number of days in the month
	int maxdays = GMCdaysInMonth(1,1996);
	
    int currentRow = 0;
	// int currentColumn = 0;

	// Draw the values of the current selection
    int dayctr = 0;
	int tempDow = dow;
	
	// Here we go through each day of the month and find out what cell it should
	// be displayed in and put a number in the top left corner of the cell
	for (dayctr = 0; dayctr < maxdays; dayctr++)
	{
		float columnX = tempDow * GridCellSize.x + GridTopLeft.x + 2.0;
		float rowY = currentRow * GridCellSize.y + GridTopLeft.y + 1.0 + fNumberFontSize;
		// Add a little extra for the font height
		
		// If it's one of the weekend days, draw them in blue
		if ((tempDow == 0) || (tempDow == 6))
		{
			// If the day is a weekend day, then draw it in blue
			rgb_color blueColor = {0,0,255,0};
			GDayTextArray[dayctr+1]->SetColor(blueColor);
			GDayTextArray[dayctr+1]->SetOrigin(BPoint(columnX,rowY));
			GDayTextArray[dayctr+1]->Draw(aPort);
		} else
		{
			rgb_color blackColor = {0,0,0,0};
			GDayTextArray[dayctr+1]->SetColor(blackColor);
			GDayTextArray[dayctr+1]->SetOrigin(BPoint(columnX,rowY));
			GDayTextArray[dayctr+1]->Draw(aPort);
		}
		
		
	    // Increment the day of the week, if it is already  saturday == 6,
		// reset it to sunday == 0
	    if (tempDow == 6)
		{
			tempDow = 0;
			currentRow++;
		}else
		    tempDow++;
	}
	fImageNeedsUpdate = 0;

}

