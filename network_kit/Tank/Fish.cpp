/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include <Application.h>
#include "TankWindow.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int
main(int argc, char **argv)
{
		uint8	tank_x=3, tank_y=3;
		
		if(argc < 2) {
			printf("Usage: Tank -x <int> -y <int>\n");
			printf("  locates the tank at specified location in TankSpace.\n");
			printf("  default is (3,3)\n");
		}
		
		/* poor-man's parser [aka Conan-search] */
		for(int i=1; i< argc; i++) {
			if (strcmp(argv[i], "-x") == 0) 	{ tank_x=atoi(argv[i+1]); }
			if (strcmp(argv[i], "-y") == 0) 	{ tank_y=atoi(argv[i+1]); }
		}

        BApplication theApp("application/x-AJH-fish");
		TankWindow	*theWindow;
		theWindow = new TankWindow(tank_x, tank_y);

		theWindow->Show();
        theApp.Run();

        exit(0);
}
