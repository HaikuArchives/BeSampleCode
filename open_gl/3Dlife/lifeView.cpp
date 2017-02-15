/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include "lifeView.h"
#include <unistd.h>

// rule controls
short newlifeLower = 5;
short newlifeUpper = 5;
short sustainlifeUpper = 5;
short sustainlifeLower = 4;

bool steadyFlag;
bool (*board)[BOARD_SIZE][BOARD_SIZE];
sem_id read_board_sem, write_board_sem;

static long lifeThread(lifeView *);
static long drawThread(lifeView *);
static void DoLife(bool (*last)[BOARD_SIZE][BOARD_SIZE], 
			bool (*next)[BOARD_SIZE][BOARD_SIZE]);


lifeView::lifeView(BRect R)
	: BGLView(R, "lifeglview", B_FOLLOW_ALL, 0, 
				BGL_RGB | BGL_DEPTH | BGL_DOUBLE )
{
	srand(getpid());
	continuous = false;
	singleStep = true;
	xangle = yangle = zangle = 0;
	_QuitRequested = false;
	
	// initialize the semaphores
	::read_board_sem = create_sem(0, "read");
	::write_board_sem = create_sem(1, "write");
}

lifeView::~lifeView()
{
	// empty
}

void
lifeView::AttachedToWindow()
{
	BGLView::AttachedToWindow();
	LockGL();

	// enable backface culling
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	
	glEnable(GL_DEPTH_TEST);

	glEnableClientState(GL_VERTEX_ARRAY);

	glShadeModel(GL_FLAT);
	glClearColor(0.0,0.0,0.0,0.0);
	
	glOrtho(-BOARD_SIZE,BOARD_SIZE,-BOARD_SIZE,BOARD_SIZE,
			-BOARD_SIZE,BOARD_SIZE);
	
	UnlockGL();

	// start a thread to calculate the board generations
	lifeTID = spawn_thread((thread_entry)lifeThread, "lifeThread", 
							B_NORMAL_PRIORITY, this);
	resume_thread(lifeTID);

	// start a thread which does all of the drawing
	drawTID = spawn_thread((thread_entry)drawThread, "drawThread",
								B_NORMAL_PRIORITY, this);
	resume_thread(drawTID);
}

void
lifeView::Initialize(int seed, bool (*lifeBoard)[BOARD_SIZE][BOARD_SIZE])
{
	// semi-random way of initializing the board
	srand(seed%32535);
	for(int i=0; i < BOARD_SIZE; i++)
		for(int j=0; j < BOARD_SIZE; j++)
			for(int k=0; k < BOARD_SIZE; k++)
			{
				if(rand() < RAND_MAX/8) 				 
				{
					lifeBoard[i][j][k] = 1;
				}
			}
	
	// glider (life 4555)

/*
	lifeBoard[11][10][10] = true;
	lifeBoard[12][10][10] = true;
	lifeBoard[11][11][10] = true;
	lifeBoard[12][11][10] = true;
	lifeBoard[10][10][11] = true;
	lifeBoard[13][10][11] = true;
	lifeBoard[10][11][11] = true;
	lifeBoard[13][11][11] = true;
	lifeBoard[11][12][11] = true;
	lifeBoard[12][12][11] = true;
*/

	// cross (life 4555)
/*
	lifeBoard[10][10][10] = 1;
	lifeBoard[10][11][11] = 1;
	lifeBoard[9][10][11] = 1;
	lifeBoard[11][10][11] = 1;
	lifeBoard[10][9][11] = 1;
	lifeBoard[10][10][12] = 1;
*/			
}

void
lifeView::MessageReceived(BMessage *msg)
{
	switch(msg->what)
	{
	 case X_SPIN_STOP:
	 	xspin = 0;
		break;
	 case X_SPIN_POS:
	 	xspin = 1.;
	 	break;
	 case X_SPIN_NEG:
	 	xspin = -1.;
	 	break;
	 case Y_SPIN_STOP:
	 	yspin = 0;
		break;
	 case Y_SPIN_POS:
	 	yspin = 1.;
	 	break;
	 case Y_SPIN_NEG:
	 	yspin = -1.;
	 	break;
	 case Z_SPIN_STOP:
	 	zspin = 0;
		break;
	 case Z_SPIN_POS:
	 	zspin = 1.;
	 	break;
	 case Z_SPIN_NEG:
	 	zspin = -1.;
	 	break;
	 case LIFE_4555:
		newlifeLower = 5;
		newlifeUpper =  5;
		sustainlifeUpper = 5;
		sustainlifeLower = 4;
	 	break;
	 case LIFE_5766:
		newlifeLower = 6;
		newlifeUpper =  6;
		sustainlifeUpper = 7;
		sustainlifeLower = 5;
	 	break;
	 case CONTINUOUS:
	 	continuous = (continuous) ? false : true;
	 	BMenuItem *item;
	 	msg->FindPointer("source", (void**)&item);
	 	item->SetMarked(continuous);
	 	break;
	 case SINGLE_STEP:
	 	singleStep = true;
	 	break;
	 default:
	 	BView::MessageReceived(msg);
	}
}

void
lifeView::Display(bool (*lifeBoard)[BOARD_SIZE][BOARD_SIZE], int genCount,
				bool steadyState)
{
	LockGL();
	
	if(lifeBoard == 0) 
	{
		// clear the board if there's nothing to show
		sprintf(genStr, "Calculating intial generation");
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		SwapBuffers();
	}
	else
	{
		if(steadyState)
			sprintf(genStr, "Generation: %d (steady state)", genCount);
		else
			sprintf(genStr, "Generation: %d", genCount);
	
	
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glPushMatrix();
		
		// nicer angle to view the board
		glRotatef(45, 1,1,1);
		
		// rotate for action!
		glRotatef(xangle, 1,0,0);
		glRotatef(yangle, 0,1,0);
		glRotatef(zangle, 0,0,1);
		
		// center the board
		float trans = BOARD_SIZE/2. *(1 + 0.2); // 0.2 = gap size
		glTranslatef(-trans,-trans,-trans);
	
		DrawFrame(lifeBoard);
	
		glPopMatrix();
		//printf("calling swap buffers...\n");
		SwapBuffers();	
	}
	
	UnlockGL();

}


void
lifeView::ExitThreads()
{
	status_t status;
	int32 count = 0;
	_QuitRequested = true;

	release_sem(write_board_sem);
	release_sem(read_board_sem);
	
	while (Window()->IsLocked()) {
		Window()->Unlock();
		count++;
	};
	wait_for_thread(drawTID, &status);
	wait_for_thread(lifeTID, &status);
	while (count--) Window()->Lock();
}

void
lifeView::SpinCalc()
{
	// increase the angles in the direction they are headed,
	// but keep them under 360 degrees
	if(xspin)
	{
		xangle += (int)(xspin * 2);
		xangle = xangle % 360;
	}
	if(yspin)
	{
		yangle += (int)(yspin * 2);
		yangle = yangle % 360;
	}
	if(zspin)
	{
		zangle += (int)(zspin * 2);
		zangle = zangle % 360;
	}
	
	
}

void
lifeView::DrawFrame(bool (*lifeBoard)[BOARD_SIZE][BOARD_SIZE])
{
	//printf("In DrawFrame...\n");
	// step through the board one cell at a time.  If a cell
	// is alive, draw it.
	for(int i=0; i < BOARD_SIZE; i++)
		for(int j=0; j < BOARD_SIZE; j++)
			for(int k=0; k < BOARD_SIZE; k++)
			{
				//printf("checking cell [%d,%d,%d]\n", i,j,k);
				if(lifeBoard[i][j][k])
				{
					//printf("calling draw cube!!\n");
					DrawCube(i,j,k);
				}
			}
}


void
lifeView::DrawCube(int x, int y, int z)
{
	//printf("In DrawCube (%d, %d, %d)\n", x, y, z);
	float fx,fy,fz;
	fx = x + x*0.2;  // some space between cubes
	fy = y + y*0.2;
	fz = z + z*0.2;

	GLfloat vertices[] = {fx, fy, fz,
							fx, fy+1, fz,
							fx+1, fy+1, fz,
							fx+1, fy, fz,
							fx, fy, fz+1, 
							fx, fy+1, fz+1,
							fx+1, fy+1, fz+1,
							fx+1, fy, fz+1};

	
	// each group of four vertices is a face of the cube
	static GLubyte allIndices[] = { 0, 1, 2, 3,
									3, 2, 6, 7,
									7, 6, 5, 4,
									4, 5, 1, 0,
									1, 5, 6, 2,
									3, 7, 4, 0 };

	glColor4f(1,0,0,.25);

	// draw all sides of the cube
	glVertexPointer(3, GL_FLOAT, 0, vertices);
	glDrawElements(GL_QUADS, 24, GL_UNSIGNED_BYTE, allIndices);

	// add a black outline to the cube
	glColor3f(0,0,0);
	glDrawElements(GL_LINE_LOOP, 24, GL_UNSIGNED_BYTE, allIndices);

}

static long 
drawThread(lifeView *mv)
{

	bool (*displayBoard)[BOARD_SIZE][BOARD_SIZE];
	displayBoard = 0;
	

	int generations = -1;
	while(!(mv->QuitPending()))
	{
		mv->SpinCalc();
		mv->Display(displayBoard, generations, steadyFlag);
		
		if(mv->continuousMode() && steadyFlag)
		{
			mv->continuousMode(false);
		}
						
		if(mv->continuousMode() || mv->singleStepMode())
		{
			if(acquire_sem_etc(read_board_sem, 1, B_TIMEOUT, 
								100) != B_TIMED_OUT)
			{
				//printf("got semaphore...\n");
				if(displayBoard) 
				{
					delete []displayBoard;
				}
			
				displayBoard = board;
				board = NULL;
				generations++;
				mv->singleStepMode(false);
				release_sem(write_board_sem);
			}
		}
		else
		{
			// if the display isn't rotating, give the cpu a break!
			if(!mv->spinning())
				snooze(500000);		
		}
	}
	
	return 1;
}

static long
lifeThread(lifeView *mv)
{
	
	bool (*nextGen)[BOARD_SIZE][BOARD_SIZE];		
	bool (*prevGen)[BOARD_SIZE][BOARD_SIZE];
	
	prevGen = new bool[BOARD_SIZE][BOARD_SIZE][BOARD_SIZE];
	mv->Initialize(getpid(), prevGen);	
	
	while(!(mv->QuitPending()))
	{
		nextGen = new bool[BOARD_SIZE][BOARD_SIZE][BOARD_SIZE];	

		acquire_sem(write_board_sem);
		if(mv->QuitPending())
		{
			// semaphore was released from ExitThreads
			break;
		}
		
		// calculate the next generation
		DoLife(prevGen, nextGen);
		
		// "post" the next generation		
		board = nextGen;
		prevGen = nextGen;

		release_sem(read_board_sem);
	}
	return 1;
}

static void
DoLife(bool (*last)[BOARD_SIZE][BOARD_SIZE], 
		bool (*next)[BOARD_SIZE][BOARD_SIZE])
{
	//printf(" In DoLife()...\n");
	bool steadyBit = true;
	
	short neighbors;
	bool curCell;

	for(int i= 0; i < BOARD_SIZE; i++)
		for(int j=0; j < BOARD_SIZE; j++)
			for(int k=0; k < BOARD_SIZE; k++)
			{
				curCell = last[i][j][k];
				// count living neighbors
				neighbors = 0;
				for(int l = i-1; l < i+2; l++)
					for(int m = j-1; m < j+2; m++)
						for(int n = k-1; n < k+2; n++)
						{
																
							if((l < 0) || (l > BOARD_SIZE-1) || (m < 0) ||
								(m > BOARD_SIZE-1) || (n < 0) || (n >BOARD_SIZE-1))
							{
								// off-board condition
								continue;
							}
							else
							{
								if(last[l][m][n])
								{
									neighbors++;
								}
							}
						}
				
//				printf("neighbors = %d\n", neighbors);		

				// determine fate of next generation
				if(curCell)
				{
//					printf("deciding fate of living cell\n");
					// don't add ourselves to neighbor count...
					neighbors -= curCell;
				
					if((neighbors <= sustainlifeUpper) &&
						(neighbors >= sustainlifeLower))
					{
//						printf("cell (%d,%d,%d) remains alive...\n", i,j,k);
						next[i][j][k] = 1;
					}
				}
				else
				{
//					printf("deciding fate of dead cell\n");
					if((neighbors <= newlifeUpper) && 
						(neighbors >= newlifeLower))
					{
//						("cell (%d,%d,%d) comes alive...\n",i,j,k);
						next[i][j][k] = 1;
					}
				}
				
				if(steadyBit && (curCell != next[i][j][k]))
				{
					steadyBit = false;
				}
			}	

			
	if(steadyBit != ::steadyFlag)			
	{
		steadyFlag = steadyBit;
	}
}	

