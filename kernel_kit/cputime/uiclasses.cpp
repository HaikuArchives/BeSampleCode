// cputime - a cpu-usage measuring tool
// Copyright 2000, Be Incorporated.   All Rights Reserved.
// This file may be used under the terms of the Be Sample Code License.

#include "uiclasses.h"

#include "datadefs.h"

extern ulong numthreads;
extern ulong timeindex;
extern bigtime_t starttime;
extern bigtime_t stoptime;
extern Ruler *ruler;
extern CPUView *cpuview;
extern Scroller *scroller;
extern DisplayView *displayview;


int mag=1;

Ruler::Ruler()
	: BView(BRect(),"",B_WILL_DRAW,B_FOLLOW_LEFT_RIGHT)
{
	SetViewColor(250,250,250);
}

void Ruler::AttachedToWindow()
{
	BRect wb=Window()->Bounds();
	ResizeTo(wb.Width()-200-1,19);
	MoveTo(0,0);
}

void Ruler::Draw(BRect rect)
{
	float samplespersecond=(timeindex/*-2*/)/((stoptime-starttime)/1E6)*mag;

	for(int i=int(rect.left/samplespersecond-20);i<rect.right/samplespersecond+20;i++)
	{
		if(i!=0 && (i%5)==0)
		{
			char num[10];
			sprintf(num,"%d:%02d",i/60,i%60);
			DrawString(num,BPoint(i*samplespersecond-StringWidth(num)/2,10));
			StrokeLine(BPoint(i*samplespersecond,20),BPoint(i*samplespersecond,13));
		}
		else
			StrokeLine(BPoint(i*samplespersecond,20),BPoint(i*samplespersecond,16));
	}
}


CPUView::CPUView()
	: BView(BRect(),"",B_WILL_DRAW|B_FULL_UPDATE_ON_RESIZE|B_FRAME_EVENTS,B_FOLLOW_ALL_SIDES)
{
	SetViewColor(180,180,180);
}

void CPUView::AttachedToWindow()
{
	BRect wb=Window()->Bounds();
	ResizeTo(wb.Width()-200-1,wb.Height()-B_H_SCROLL_BAR_HEIGHT-20-1);
	MoveTo(0,20);
}
			
void CPUView::Draw(BRect rect)
{
	rgb_color col[]=
	{
		{255,0,0},
		{0,255,0},
		{0,0,255},
		{255,255,0},
		{255,0,255},
		{0,255,255},
		{255,255,255},
		{0,0,0}
	};
	
	for(int j=int(max_c(2,rect.left-2));j<=min_c(rect.right+2,timeindex*mag);j++)
	{
		float h=Bounds().Height();
		float x=j;
		float y=h;

		for(uint i=0;i<numthreads;i++)
		{
			if(thread[i].displayit)
			{
				float t=(thread[i].times[j/mag].usertime+thread[i].times[j/mag].kerntime)/1E5;
				
				if(t>0)
				{
					SetHighColor(col[i]);
					StrokeLine(BPoint(x,y),BPoint(x,y-long(t*h)));
					y-=(long(t*h)+1);
				}
			}
		}
	}
}

void CPUView::FrameResized(float x, float)
{
	scroller->SetRange(0,max_c(0,timeindex*mag-x));
	scroller->SetProportion(x/(timeindex*mag));
	displayview->Invalidate();
}
//======================================================
Scroller::Scroller()
	:BScrollBar(BRect(0,0,100,10), "", NULL, 0,100,B_HORIZONTAL)
{
}

void Scroller::AttachedToWindow()
{
	BRect wb=Window()->Bounds();
	ResizeTo(wb.Width()-200-1,B_H_SCROLL_BAR_HEIGHT);
	MoveTo(0,wb.Height()-B_H_SCROLL_BAR_HEIGHT);
	
	float x=cpuview->Bounds().Width()+1;
	SetRange(0,max_c(0,timeindex-x));
	SetProportion(x/timeindex);
}
		
void Scroller::ValueChanged(float newvalue)
{
	cpuview->ScrollTo(newvalue,0);
	ruler->ScrollTo(newvalue,0);
	displayview->Invalidate();

}
//=======================================================================

ControlView::ControlView()
	: BView(BRect(),"",B_WILL_DRAW,B_FOLLOW_TOP_BOTTOM|B_FOLLOW_RIGHT)
{
	SetViewColor(220,220,220);
}

void ControlView::AttachedToWindow()
{
	BWindow *win=Window();
	BRect wb=win->Bounds();
	ResizeTo(200,wb.Height()-30);
	MoveTo(wb.Width()-200,0);

	for(uint i=0;i<numthreads;i++)
	{
		char label[B_OS_NAME_LENGTH+10];
		sprintf(label,"%d: %s",i,thread[i].name);
		BMessage *newmes=new BMessage('thrd');
		newmes->AddInt32("index",i);
		BCheckBox *chk;
		AddChild(chk=new BCheckBox(BRect(10,40+i*15,1000,50+i*15),"",label,newmes));
		chk->SetValue(thread[i].displayit);
		chk->SetTarget(this);
	}
	BButton *but;
	AddChild(but=new BButton(BRect(80,10,140,15),"","zoom out",new BMessage('mag-')));
	but->SetTarget(this);
	AddChild(but=new BButton(BRect(10,10,70,15),"","zoom in",new BMessage('mag+')));
	but->SetTarget(this);
}


void ControlView::MessageReceived(BMessage *mes)
{
	switch(mes->what)
	{
		float x;

		case 'thrd':
			{
				int32 index=mes->FindInt32("index");
				BCheckBox *box=(BCheckBox*)ChildAt(index);
				thread[index].displayit=box->Value();
				cpuview->Invalidate();
				displayview->Invalidate();
			}
			break;

		case 'mag+':
			if(mag<8)
				mag++;

			x=cpuview->Bounds().Width()+1;
			scroller->SetRange(0,max_c(0,timeindex*mag-x));
			scroller->SetProportion(x/(timeindex*mag));

			cpuview->Invalidate();
			ruler->Invalidate();
			displayview->Invalidate();
			break;

		case 'mag-':
			if(mag>1)
				mag--;

			x=cpuview->Bounds().Width()+1;
			scroller->SetRange(0,max_c(0,timeindex*mag-x));
			scroller->SetProportion(x/(timeindex*mag));

			cpuview->Invalidate();
			ruler->Invalidate();
			displayview->Invalidate();
			break;

		default:
			BView::MessageReceived(mes);
			break;
	}
}


DisplayView::DisplayView()
	: BView(BRect(),"",B_WILL_DRAW,B_FOLLOW_TOP_BOTTOM|B_FOLLOW_RIGHT)
{
	SetViewColor(220,220,220);
}

void DisplayView::AttachedToWindow()
{
	BWindow *win=Window();
	BRect wb=win->Bounds();
	ResizeTo(200,30);
	MoveTo(wb.Width()-200,wb.Height()-29);
}

void DisplayView::Draw(BRect)
{
	char out[200];

	float offset=cpuview->Bounds().left/mag;
	float start=scroller->Value()/mag;
	float end=start+offset+cpuview->Bounds().right/mag;

	if(end>timeindex)
		end=timeindex;

	bigtime_t totalelapsed=bigtime_t((end-start)*(stoptime-starttime)/(timeindex));

	float totaluser=0;
	float totalkern=0;
	for(uint i=0;i<numthreads;i++)
	{
		if(thread[i].displayit)
		{
			for(uint j=uint(start);j<end;j++)
			{
				totaluser+=thread[i].times[j].usertime;
				totalkern+=thread[i].times[j].kerntime;
			}
		}
	}
	
	BRect bounds=Bounds();
	DrawString("visible threads in visible area used",BPoint(7,bounds.bottom-17));
	sprintf(out,"%.1f%% CPU (%.1f%% user/%.1f%% kernel)",
				(totaluser+totalkern)*100/totalelapsed,
				totaluser*100/totalelapsed,totalkern*100/totalelapsed
				);
	DrawString(out,BPoint(7,bounds.bottom-5));
}