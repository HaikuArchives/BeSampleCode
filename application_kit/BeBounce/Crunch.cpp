/*
 File:	Crunch.cpp

 All the mathy routines are here.

	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef _DEBUG_H
#include <Debug.h>
#endif
#ifndef CRUNCH_H
#include "Crunch.h"
#endif

/*--------------------------------------------------------*/

/*
 What follows is the 'mathematical' part of the BeBounce demo.
 This doesn't contain any code specific to the BeBox. It
 just a bunch of geometry and math for doing the hit-testing
 and such required for the application.

 As such the authors take the liberty to leave this
 code sparsely commented. We're sure that some graphics/
 geometry wizard will point out that all these methods could
 be replaced with 3 lines of code. If that's the case
 then please let us know (and include a resume if you'd
 like).
*/

#include <math.h>

inline float rad_to_degree(float rad)
{
	return rad * 180.0 / M_PI;
}

inline float degree_to_rad(float degree)
{
	return degree * M_PI / 180.0;
}

/*--------------------------------------------------------*/

void DrawFrame(BView *v, BRect frame, float start,
	float end)
{
	frame.top--;

	float right = frame.right;
	float left = frame.left;
	float top = frame.top;
	float bottom = frame.bottom;

	float middle_v = (top + bottom) / 2.0;

	BPoint gapStart = CalcIntersection(start, frame);
	BPoint gapEnd = CalcIntersection(end, frame);
	
//	PRINT_OBJECT(frame);
//	PRINT_OBJECT(gapStart);
//	PRINT_OBJECT(gapEnd);

	// draw top half of right side. Look for range 0-45
	if (start < 45.0) {
		v->StrokeLine(BPoint(right, middle_v), gapStart);
		if (end < 45.0)
			v->StrokeLine(gapEnd, BPoint(right, top));
	} else {
		if (end < 45.0)
			v->StrokeLine(gapEnd, BPoint(right, top));
		else if (end > start)
			v->StrokeLine(BPoint(right, middle_v),
						BPoint(right, top));
	}

	// draw the 'top'. Look for angles in range 45-135
	if ((start < 45.0) && (end > 135.0)) {
	} else if ((start >= 45.0) && (start < 135.0)) {
		v->StrokeLine(BPoint(right, top), gapStart);
		if ((end >= 45.0) && (end < 135.0))
			v->StrokeLine(gapEnd, BPoint(left, top));
	} else {
		if ((end >= 45.0) && (end < 135.0))
			v->StrokeLine(gapEnd, BPoint(left, top));
		else
			v->StrokeLine(BPoint(right, top), BPoint(left, top));
	}

	// draw the 'left'. Look for angles in range 135-225
	if ((start < 135.0) && (end > 225.0)) {
	} else if ((start >= 135.0) && (start < 225.0)) {
		v->StrokeLine(BPoint(left, top), gapStart);
		if ((end >= 135.0) && (end < 225.0))
			v->StrokeLine(gapEnd, BPoint(left, bottom));
	} else {
		if ((end >= 135.0) && (end < 225.0))
			v->StrokeLine(gapEnd, BPoint(left, bottom));
		else
			v->StrokeLine(BPoint(left, top),
						BPoint(left, bottom));
	}

	// draw the 'bottom'. Look for angles in range 225-315
	if ((start < 225.0) && (end > 315.0)) {
	} else if ((start >= 225.0) && (start < 315.0)) {
		v->StrokeLine(BPoint(left, bottom), gapStart);
		if ((end >= 225.0) && (end < 315.0))
			v->StrokeLine(gapEnd, BPoint(right, bottom));
	} else {
		if ((end >= 225.0) && (end < 315.0))
			v->StrokeLine(gapEnd, BPoint(right, bottom));
		else
			v->StrokeLine(BPoint(left, bottom),
						BPoint(right, bottom));
	}

	// draw the lower half of 'right'. Look range 315-360
	if ((start >= 315.0) && (start < 360.0)) {
		v->StrokeLine(BPoint(right, bottom), gapStart);
		if ((end < 360.0) && (end >= 315.0))
			v->StrokeLine(gapEnd, BPoint(right, middle_v));
	} else {
		if ((end >= 315.0) && (end < 360.0))
			v->StrokeLine(gapEnd, BPoint(right, middle_v));
		else if (end > start)
			v->StrokeLine(BPoint(right, bottom),
						BPoint(right, middle_v));
	}
}

/*--------------------------------------------------------*/

BPoint CalcIntersection(BPoint pt1, BPoint pt2,
	BRect the_rect, BPoint *new_sp)
{
	/*
	 Determine the point at which the line segment
	 defined by pt1 & pt2 and the rectangle (the_rect)
	 intersect. We assume that they do indeed intersect.
		
		eq for line:	y = m*x + b

		eq for m:		m = (y2 - y1) / (x2 - x1)

		eq for b:		b = y1 - (m * x1)
	*/

//	PRINT(("CalcIntersection:\n"));
//	PRINT_OBJECT(pt1);
//	PRINT_OBJECT(pt2);
//	PRINT_OBJECT(the_rect);

	BPoint p;
	
	if (pt1.x == pt2.x) {
		// check intersection with top (y = r.top)
		p.y = the_rect.top;
		p.x = pt1.x;
		if ((pt2.y < pt1.y) && the_rect.Contains(p) &&
			Between(pt1, pt2, p)) {
				new_sp->y *= -1;
				return p;
		}

		// check intersection with bottom (y = r.bottom)
		p.y = the_rect.bottom;
		p.x = pt1.x;
		if ((pt2.y > pt1.y) && the_rect.Contains(p) &&
			Between(pt1, pt2, p)) {
				new_sp->y *= -1;
				return p;
		}
	} 

	float	m = ((pt2.y - pt1.y) / (pt2.x - pt1.x));
	float	b = pt1.y - (m * pt1.x);

//	PRINT(("m = %.2f, b = %.2f\n", m, b));

	/*
	 We will check against each side of the rectangle.
	 Must realize that the math is dealing with real
	 lines, not line segments. So we must check that
	 the intersection occurs within the bounds of the rect.
	*/

	if (m != 0) {
		// check intersection with top (y = r.top)
		p.y = the_rect.top;
		p.x = (p.y - b) / m;
		if ((pt2.y < pt1.y) && the_rect.Contains(p) &&
			Between(pt1, pt2, p)) {
				new_sp->y *= -1;
				return p;
		}

		// check intersection with bottom (y = r.bottom)
		p.y = the_rect.bottom;
		p.x = (p.y - b) / m;
		if ((pt2.y > pt1.y) && the_rect.Contains(p) &&
			Between(pt1, pt2, p)) {
				new_sp->y *= -1;
				return p;
		}
	}

	// check for intersection with left (x = r.left)
	p.x = the_rect.left;
	p.y = (m * p.x) + b;
	if ((pt2.x < pt1.x) && the_rect.Contains(p) &&
		Between(pt1, pt2, p)) {
			new_sp->x *= -1;
			return p;
	}

	// check for intersection with right (x = r.right)
	p.x = the_rect.right;
	p.y = (m * p.x) + b;
	if ((pt2.x > pt1.x) && the_rect.Contains(p) &&
		Between(pt1, pt2, p)) {
			new_sp->x *= -1;
			return p;
	}
	
	PRINT(("BAD CalcIntersection:\n"));
	PRINT_OBJECT(pt1);
	PRINT_OBJECT(pt2);
	PRINT_OBJECT(the_rect);
	ASSERT(0);
	return B_ORIGIN;
}

/*--------------------------------------------------------*/

BPoint CalcIntersection(float angle, BRect the_rect)
{
	/*
	 Return the point at which a line leaving the center
	 of rect at specified angle will intersect the given
	 rectangle.

	 We will base everything off the center of the rect.
	 We treat the center as the origin (0,0).
	 'dh' is 1/2 the width of the rect.
	 'dv' is 1/2 the height of the rect.
	 'slope' will the slope of a line at the given angle.
	*/

//	PRINT(("--- Calc Intersection ---\n"));
//	PRINT(("angle = %.1f, ", angle)); PRINT_OBJECT(the_rect);

	BPoint center = rect_center(the_rect);

	float	dh;
	float	dv;
	float	slope;
	float	tmp = angle;
	long	quad = 1;

	dh = the_rect.Width() / 2.0;
	dv = the_rect.Height() / 2.0;

	if (angle == 90.0) {
		return BPoint(0, dv) + center;
	}

	if (angle == 270.0) {
		return BPoint(0, -dv) + center;
	}
	
	if (angle < 90.0) {
		tmp = angle;
		quad = 1;
	} else if ((angle >= 90.0) && (angle < 180.0)) {
		tmp = 180 - angle;
		quad = 2;
	} else if ((angle >= 180.0) && (angle < 270.0)) {
		tmp = angle - 180;
		quad = 3;
	} else if ((angle >= 270.0) && (angle <= 360.0)) {
		tmp = 360 - angle;
		quad = 4;
	}
	
	slope = tan(degree_to_rad(tmp));
	
	/*
	 Determine the intersection between the line at given
	 angle and both the line defining the top side and the
	 left side of the rect.
		equation for 'angle' line	:	y = slope * x
		equation for 'top'			:	y = dv;
		equation for 'right'		:	x = dh;
	
	Only one of these intersection points will actually
	lie on the qrect. That's the point we want. The other
	will lie outside the qrect.
	*/
	
	BPoint	top_pt;
	BPoint	right_pt;
	BPoint	pt;

	top_pt.y = dv;
	top_pt.x = dv / slope;

	right_pt.x = dh;
	right_pt.y = slope * dh;
	
	// if the 'y' coord of this point is too high.
	if (right_pt.y > dv)
		pt = top_pt;
	else
		pt = right_pt;
	
	// 'pt' is intersection, normalized to the 1st quadrant,
	// relative to the center of 'the_rect'

	// move 'pt' into appropriate quadrant (based on 'angle')
	switch (quad) {
		case 1:	break;
		case 2:	pt.x = -pt.x; break;
		case 3:	pt.x = -pt.x; pt.y = -pt.y; break;
		case 4:	pt.y = -pt.y; break;
	}

	// translate 'pt' into view's coordinate system
	pt.y = -pt.y;
	pt = pt + center;
	
	return pt;
}

/*--------------------------------------------------------*/

void CalcGapAngles(BRect rect1, BRect rect2,
	float *start_angle, float *end_angle)
{
	BPoint center = rect_center(rect1);

	if (rect2.Intersects(rect1)) {
		*start_angle = -1.0;
		*end_angle = -1.0;
		return;
	}

	/*
	 Given a point 'center' and rect 'rect2':

                 ________
                |        |
        x       |        |
                |        |
                |        |
                 -------- 
	 
	 calculate the apparent size of the rect, from the point,
	 in degrees.
	*/

	long	tl_quad;
	long	tr_quad;
	long	bl_quad;
	long	br_quad;

	float top_left_angle = CalcAngle(center,
								rect2.LeftTop(), &tl_quad);
	float top_right_angle = CalcAngle(center,
								rect2.RightTop(), &tr_quad);
	float bot_left_angle = CalcAngle(center,
								rect2.LeftBottom(), &bl_quad);
	float bot_right_angle = CalcAngle(center,
								rect2.RightBottom(), &br_quad);
	
	if (bl_quad == 1) {
		// completely in quad 1
		*start_angle = bot_right_angle;
		*end_angle = top_left_angle;
	} else if ((bl_quad == 2) && (br_quad == 1)) {
		// spanning quad 1 and 2
		*start_angle = bot_right_angle;
		*end_angle = bot_left_angle;
	} else if (br_quad == 2) {
		// completely in quad 2
		*start_angle = top_right_angle;
		*end_angle = bot_left_angle;
	} else if ((tr_quad == 2) && (br_quad == 3)) {
		// spanning quad 2 and 3
		*start_angle = top_right_angle;
		*end_angle = bot_right_angle;
	} else if (tr_quad == 3) {
		// completely in quad 3
		*start_angle = top_left_angle;
		*end_angle = bot_right_angle;
	} else if ((tl_quad == 3) && (tr_quad == 4)) {
		// spanning quad 3 and 4
		*start_angle = top_left_angle;
		*end_angle = top_right_angle;
	} else if (tl_quad == 4) {
		// completely in quad 4
		*start_angle = bot_left_angle;
		*end_angle = top_right_angle;
	} else if ((tl_quad == 1) && (bl_quad == 4)) {
		// spanning quad 4 and 1
		*start_angle = bot_left_angle;
		*end_angle = top_left_angle;
	} else {
		ASSERT(0);
	}

//	PRINT(("(%d) start_angle	= %.1f\n", find_thread(NULL), *start_angle));
//	PRINT(("(%d) end_angle	= %.1f\n", find_thread(NULL), *end_angle));
}

/*--------------------------------------------------------*/

float CalcAngle(BPoint pt1, BPoint pt2, long *quad)
{
	float	slope;
	float	angle;
	
//	PRINT(("--- Calc Angle ---\n"));
//	PRINT_OBJECT(pt1);
//	PRINT_OBJECT(pt2);

	if (pt1.x != pt2.x) {
		slope = fabs((pt1.y - pt2.y) / (pt1.x - pt2.x));
		
		/*
		 Turn the slope into an angle. The basic formula
		 to convert to angle between 0 and 90 degrees is:
				angle = arctan(slope);
		 
		 The above slope is really abs(slope). After
		 calculating the angle we'll adjust.
		*/
		angle = rad_to_degree(atan(slope));

		// adjust 'angle' into the correct 'quadrant'
		if (pt1.y > pt2.y) {
			// we're in quad 1 or 2
			if (pt1.x < pt2.x) {
				angle = angle;					// quad 1
				*quad = 1;
			} else {
				angle = 180.0 - angle;			// quad 2
				*quad = 2;
			}
		} else {
			// we're in quad 3 or 4
			if (pt1.x < pt2.x) {
				angle = 360.0 - angle;			// quad 4
				*quad = 4;
			} else {
				angle = 180.0 + angle;			// quad 3
				*quad = 3;
			}
		}

	} else {
		// x coord's are equal --> infinite slope
		if (pt1.y > pt2.y) {
			angle = 90.0;
			*quad = 2;
		} else {
			angle = 270.0;
			*quad = 4;
		}
	}
	
//	PRINT(("angle = %.1f, quad=%d\n", angle, *quad));
	return angle;
}

/*--------------------------------------------------------*/

bool Between(BPoint p1, BPoint p2, BPoint p3)
{
	// determine if p3 is between p1 & p2 (but not == to p1)

	float minx;
	float maxx;

	if (p1.x < p2.x) {
		minx = p1.x;
		maxx = p2.x;
	} else {
		minx = p2.x;
		maxx = p1.x;
	}

	float miny;
	float maxy;
	if (p1.y < p2.y) {
		miny = p1.y;
		maxy = p2.y;
	} else {
		miny = p2.y;
		maxy = p1.y;
	}

	return ((minx <= p3.x) && (p3.x <= maxx) &&
			(miny <= p3.y) && (p3.y <= maxy));
}

/*--------------------------------------------------------*/
static inline float corner_case(float start, float end)
{
	if ((start < 45.0) && (end > 45.0)) {
		return 45.0;
	}
	if ((start < 135.0) && (end > 135.0)) {
		return 135.0;
	}
	if ((start < 225.0) && (end > 225.0)) {
		return 225.0;
	}
	if ((start < 315.0) && (end > 315.0)) {
		return 315.0;
	}

	return 0.0;
}

/*--------------------------------------------------------*/

float CalcGapPosition(float angle, float start, float end)
{
	// calculate the relative location of the 'hit'

	/*
	 need to special case when the gap goes around a corner.
	 In this situation the initial position of the 'ball'
	 in the other window is determined using different logic.
	*/

	float	corner;
	float	relative;

	end = (end > start) ? end : end + 360.0;
	if (angle < start)
		angle += 360.0;


	if ((corner = corner_case(start, end)) != 0.0) {
		// use a negative number to indicate a corner case.
		relative = -1.0 * ((angle - start) / (end - start));
	} else {
		relative = (angle - start) / (end - start);
	}

	return relative;
}

/*--------------------------------------------------------*/

BPoint CalcInitialPosition(float rel_loc, float start,
	float end, BRect bounds)
{
	/*
	 rel_loc is the relative location of ball between the
	 start and end of the gap.
	*/

	end = (end > start) ? end : end + 360.0;

	float	angle;

	if (rel_loc < 0.0) {
		// calc new position relative to start of gap
		angle = start + (-rel_loc * (end - start));
	} else {
		// calc new position relative to end of gap
		angle = end - (rel_loc * (end - start));
	}

	if (angle < 0.0)
		angle += 360.0;
	else if (angle > 360.0)
		angle -= 360.0;
	
	BPoint position = CalcIntersection(angle, bounds);

//	PRINT(("angle = %.2f, ", angle)); PRINT_OBJECT(position);

	return position;
}
