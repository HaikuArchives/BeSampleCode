/* FilterView.cpp */
/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/
#include <Polygon.h>
#include <Message.h>
#include "FilterView.h"
#include "MsgVals.h"

const float EPSILON = 0.01;
#define min(x,y) ((x) < (y) ? (x) : (y))

/*-------------------------------------------------------------------------*/

FilterView::FilterView(BRect R)
	: BView(R, "filter", B_FOLLOW_ALL, B_WILL_DRAW)
{
	/* Set some initial values */
	curmass = 0.50;
	curdrag = 0.46;
	width = 0.50;
	fillmode = true;     /* fillmode = false means wireframe */
	fixed_angle = true;
	zzz = 10000;		/* sleep for this many usecs between mouse reads */
	bigX = R.Width();
	bigY = R.Height();
	scaleToPage = true;
}
/*-------------------------------------------------------------------------*/

void
FilterView::MessageReceived(BMessage* msg)
{
	int32 val;
	switch(msg->what)
	{
	 /* Tweak Window messages... */
	 case MASS_CHG:
		msg->FindInt32("Mass", &val);
		curmass = (float)val/100.0;	
	 	break;
	 	
	 case DRAG_CHG:
	 	msg->FindInt32("Drag", &val);
	 	curdrag = (float)val/100.0;
	 	break;
	 	
	 case WIDTH_CHG:
	 	msg->FindInt32("Width", &val);
	 	width = (float)val/100.0;
	 	break;
	 	
	 case SLEEPAGE_CHG:
	 	msg->FindInt32("Sleepage", &val);
	 	val = MIN_SLEEP + (MAX_SLEEP - val);
	 	zzz = val;
	 	break;
	 
	 case FILL_CHG:
	 	toggle(fillmode); 
	 	break;
	 
	 case ANGLE_CHG:	 	
	 	toggle(fixed_angle);
	 	break;
	
	 /* Color Window messages... */ 	
	 case COLOR_CHG:
		int16 red, green, blue;
		msg->FindInt16("red", &red);
		msg->FindInt16("green", &green);
		msg->FindInt16("blue", &blue);
		SetHighColor(red,green,blue);
	 	break;

	 /* other messages */
	 case CLEAR_SCREEN:
	 	ClearScreen();
	 	break;
	 	
	 default:
	 	BView::MessageReceived(msg);
	 	break;
	}
}
/*-------------------------------------------------------------------------*/

void
FilterView::MouseDown(BPoint point)
{
	uint32 buttons=0;
// 	bool flag=0;
  	float mx, my;
	BRect B(Bounds());
	   
  	GetMouse(&point, &buttons, true);

	if(buttons == B_PRIMARY_MOUSE_BUTTON)
	{
		mx = (float)point.x / B.right;
        my = (float)point.y / B.bottom;
        m.Set(mx,my);	
        setpos(m);
        odel.Set(0,0);
    
    	/* loop through until the button is no longer held down */
		while(1)
        {  
    		GetMouse(&point, &buttons, true);
        	if(buttons != B_PRIMARY_MOUSE_BUTTON) break;
	
	 	    mx = (float)point.x / B.right;
	    	my = (float)point.y / B.bottom;
	      	m.Set(mx,my);
	
	      	if(Apply(m, curmass, curdrag)) DrawSegment();

          	snooze(zzz);
		}
	}
	else if (buttons == B_SECONDARY_MOUSE_BUTTON)
	{
		ClearScreen();
	}
}
/*-------------------------------------------------------------------------*/

void
FilterView::ClearScreen()
{
	/* clears the screen by deleting the list of polygons */
	int32 count = polyList.CountItems(); 
	for(int i=0; i<count; i++)
	{
		delete (polyList.FirstItem());
		/* the list is compacted after removing an item, so here */
		/* we keep removing the first item in the list */
		polyList.RemoveItem(0L);
	}
	Invalidate();
}
/*-------------------------------------------------------------------------*/

bool 
FilterView::Apply(BPoint m, float curmass, float curdrag)
{
    float mass, drag;
    float fx, fy;

/* calculate mass and drag */
    mass = flerp(1.0, 160.0,curmass);
    drag = flerp(0.00,0.5,curdrag*curdrag);

/* calculate force and acceleration */
    fx = m.x-cur.x;
    fy = m.y-cur.y;
    acc = sqrt(fx*fx+fy*fy);
    if(acc<0.000001)
	  return false;
    accx = fx/mass;
    accy = fy/mass;

/* calculate new velocity */
    velx += accx;
    vely += accy;
    vel = sqrt(velx*velx+vely*vely);
    angx = -vely;
    angy = velx;
    if(vel<0.000001) 
      return false;

/* calculate angle of drawing tool */
    
    angx /= vel;
    angy /= vel;
    
    if(fixed_angle) 
    {
	  angx = 0.6;
	   angy = 0.2;
	}

	
/* apply drag */
    velx = velx*(1.0-drag);
    vely = vely*(1.0-drag);

/* update position */
	last = cur;
	cur.Set(cur.x+velx, cur.y+vely);
    
    return true;
}

/*-------------------------------------------------------------------------*/
void 
FilterView::DrawSegment()
{
 	BPoint del; 
    BPoint polypoints[4];
    float wid;
    float px, py, nx, ny;
	BRect B(Bounds());
	
	/* calculate the width of the moving pen */
    wid = 0.04-vel;
    wid = wid*width;
    if(wid<0.00001)
    	wid = 0.00001;
 
    del.x = angx*wid;
    del.y = angy*wid;

    px = last.x;
    py = last.y;
    nx = cur.x;
    ny = cur.y;

	/* keep track of the farthest points */
	if(ceil(nx*B.right) > bigX) bigX = ceil(nx*B.right);
	if(ceil(ny*B.bottom) > bigY) bigY = ceil(ny*B.bottom);
	
	/* set up the points of the polygon */
    polypoints[0].Set((B.right*(px+odel.x)), (B.bottom*(py+odel.y)));
    polypoints[1].Set((B.right*(nx+del.x)), (B.bottom*(ny+del.y)));
    polypoints[2].Set((B.right*(nx-del.x)), (B.bottom*(ny-del.y)));
    polypoints[3].Set((B.right*(px-odel.x)), (B.bottom*(py-odel.y)));

	polyList.AddItem(new BPolygon(polypoints, 4));

	/* actually draw the polygon.  we stroke the polygon even if */
	/* fillmode is true, because if we do not, "thin" polygons will */
	/* not be drawn. */
	if(fillmode) 
	{
		FillPolygon(polypoints, 4);
	}
	
	StrokePolygon(polypoints,4);

	odel = del;

}
/*-------------------------------------------------------------------------*/

void
FilterView::Draw(BRect updateRect)
{
	BPolygon* poly;
	
	if(IsPrinting() && scaleToPage)
	{
		/* determine scale factor */
		float xscale = Bounds().Width() / bigX;
		float yscale = Bounds().Height() / bigY;
		float scale = min(xscale,yscale);
		SetScale(scale-EPSILON); 
	}
	
	int numItems = polyList.CountItems();
	
	/* doesn't really matter, but pull the fillmode check out of */
	/* the loop so we don't check it numItems number of times... */	
	if(fillmode)
	{
		for(int i=0; i < numItems; i++)
		{
			poly = (BPolygon*)polyList.ItemAt(i);
			FillPolygon(poly);
			StrokePolygon(poly);
		}
	}
	else
	{
		for(int i=0; i < numItems; i++)
		{
			poly = (BPolygon*)polyList.ItemAt(i); 
			StrokePolygon(poly);
		}	
	}

}

/*-------------------------------------------------------------------------*/

void 
FilterView::setpos(BPoint point)
{
  cur = point;
  last = point;
  velx = (float)0;
  vely = (float)0;
  accx = (float)0;
  accy = (float)0;
}
/*-------------------------------------------------------------------------*/

