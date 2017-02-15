/*
 File:	Crunch.h

 API for all the 'mathy' routines

	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef CRUNCH_H
#define CRUNCH_H

#include <math.h>

#ifndef _POINT_H
#include <Point.h>
#endif
#ifndef _RECT_H
#include <Rect.h>
#endif
#ifndef _VIEW_H
#include <View.h>
#endif

void	CalcGapAngles(BRect rect1, BRect rect2,
			float *start_angle, float *end_angle);
BPoint	CalcIntersection(float angle, BRect the_rect);
float	CalcAngle(BPoint pt1, BPoint pt2, long *quad);
void	DrawFrame(BView *v, BRect frame, float angleStart, float angleEnd);
BPoint	CalcIntersection(BPoint pt1, BPoint pt2, BRect the_rect,
			BPoint *new_speed);
float	CalcGapPosition(float angle, float start, float end);
BPoint	CalcInitialPosition(float rel_loc, float start, float end,
			BRect bounds);

inline float  segment_length(BPoint p1, BPoint p2)
{
	float dx = p2.x - p1.x;
	float dy = p2.y - p1.y;
	return (sqrt((dx * dx) + (dy * dy)));
}

inline BPoint rect_center(BRect rect)
{
	return BPoint((rect.right + rect.left) / 2.0,
					(rect.top + rect.bottom) / 2.0);
}

bool Between(BPoint p1, BPoint p2, BPoint p3);

#endif
