#include "Ball.h"
#include <GLView.h>
#include <InterfaceKit.h>
#include <stdio.h>
#include <StringView.h>
#include <stdlib.h>
#include <string.h>

#include <math.h>
#include <stdlib.h>

#define COS(X)	 cos( (X) * 3.14159/180.0 )
#define SIN(X)	 sin( (X) * 3.14159/180.0 )

class BallView : public BGLView
{
	GLuint	Ball;
	GLenum	Mode;
	GLfloat	Zrot, Zstep;
	GLfloat	Xpos, Ypos;
	GLfloat	Xvel, Yvel;
	GLfloat	Xmin, Xmax;
	GLfloat	Ymin, Ymax;
	GLfloat	G;

public:
			BallView(BRect r, char *name, ulong resizingMode, ulong options);
	void	AttachedToWindow();
	void	UpdateModel();
	void	DrawBall();
	GLuint	MakeBall();
	void	Advance();
};

BallView::BallView(BRect r, char *name, ulong resizingMode, ulong options)
 : BGLView(r, name, resizingMode, 0, options)
{
	Zrot = 0.0;
	Zstep = 6.0;
	Xpos = 0.0;
	Ypos = 1.0;
	Xvel = 0.2;
	Yvel = 0.0;
	Xmin = -4.0;
	Xmax = 4.0;
	Ymin = -3;
	Ymax = 4.0;
	G = -0.05;

	// clear the screen
	LockGL();
	Ball = MakeBall();
	glCullFace(GL_BACK);
	glEnable(GL_CULL_FACE);
	glEnable(GL_DITHER);
	glShadeModel(GL_FLAT);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(-6.0, 6.0, -6.0, 6.0, -6.0, 6.0);
	glMatrixMode(GL_MODELVIEW);
	UnlockGL();
}

void BallView::AttachedToWindow()
{
	LockGL();
	BRect	r = Bounds();
	GLint	size = r.IntegerHeight() + 1;
	GLint	offs = (r.IntegerWidth() + 1 - size) / 2;
	glViewport(offs, 0, size, size);
	glDrawBuffer(GL_FRONT);
	glClear(GL_COLOR_BUFFER_BIT);
	glDrawBuffer(GL_BACK);
	UnlockGL();
}

void BallView::UpdateModel()
{
	static float vel0 = -100.0;

	Zrot += Zstep;

	Xpos += Xvel;
	if (Xpos >= Xmax) {
		Xpos = Xmax;
		Xvel = -Xvel;
		Zstep = -Zstep;
	}
	if (Xpos <= Xmin) {
		Xpos = Xmin;
		Xvel = -Xvel;
		Zstep = -Zstep;
	}
	Ypos += Yvel;
	Yvel += G;
	if (Ypos < Ymin) {
		if (vel0 == -100.0)
			vel0 = fabs(Yvel);
		Yvel = vel0;
	}
}

void BallView::DrawBall()
{
	glClear(GL_COLOR_BUFFER_BIT);

	glPushMatrix();
	glTranslatef(Xpos, Ypos, 0.0);
	glScalef(1.0, 1.0, 1.0);
	glRotatef(Zrot, 0.0, 0.0, 1.0);
	glRotatef(Zrot, 1.0, 0.0, 0.0);
	glRotatef(Zrot, 0.0, 0.0, 1.0);

	glCallList(Ball);

	glPopMatrix();

	glFlush();
	SwapBuffers();
}

GLuint BallView::MakeBall()
{
	GLuint list;
	GLfloat a, b;
	GLfloat da = 18.0, db = 18.0;
	GLfloat radius = 1.0;
	GLuint color;
	GLfloat x, y, z;

	list = glGenLists(1);

	glNewList(list, GL_COMPILE);

	color = 0;
	for(a = -90.0; a + da <= 90.0; a += da)
	{
		glBegin(GL_QUAD_STRIP);
		for(b = 0.0; b <= 360.0; b += db)
		{

			if(color)
				glColor3f(1.0, 0.0, 0.0);
			else
				glColor3f(1.0, 1.0, 1.0);

			x = COS(b) * COS(a);
			y = SIN(b) * COS(a);
			z = SIN(a);
			glVertex3f(x, y, z);

			x = radius * COS(b) * COS(a + da);
			y = radius * SIN(b) * COS(a + da);
			z = radius * SIN(a + da);
			glVertex3f(x, y, z);

			color = 1 - color;
		}
		glEnd();
	}

	glEndList();

	return list;
}

void BallView::Advance()
{
	LockGL();
	UpdateModel();
	DrawBall();
	UnlockGL();
}


// MAIN INSTANTIATION FUNCTION
extern "C" _EXPORT BScreenSaver *instantiate_screen_saver(BMessage *message, image_id image)
{
	return new Ball(message, image);
}

void Ball::StartConfig(BView *view)
{
	view->AddChild(new BStringView(BRect(10, 10, 200, 35), B_EMPTY_STRING, "Ball: a simple GL screen saver"));
}

status_t Ball::StartSaver(BView *v, bool preview)
{
	if(preview)
	{
		ball = 0;
		return B_ERROR;
	}
	else
	{
		SetTickSize(50000);
	
		ball = new BallView(v->Bounds(), "objectView", B_FOLLOW_NONE, BGL_RGB | BGL_DEPTH | BGL_DOUBLE);
		v->AddChild(ball);

		return B_OK;
	}
}

void Ball::StopSaver()
{
	if(ball)
		ball->EnableDirectMode(false);
}

void Ball::DirectConnected(direct_buffer_info *info)
{
	ball->DirectConnected(info);
	ball->EnableDirectMode(true);
}

void Ball::DirectDraw(int32)
{
	ball->Advance();
}

Ball::Ball(BMessage *message, image_id id)
 : BScreenSaver(message, id)
{
}
