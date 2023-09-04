/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include <Application.h>
#include <Roster.h>
#include <StorageKit.h>
#include <Alert.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "Dictionary.h"
#include "ThesaurusWindow.h"


Dictionary dictionary;

int
main(int argc, const char **argv)
{
	BApplication thesaurusApp("application/x-vnd.Be.Thesaurus");

	app_info appInfo;
	thesaurusApp.GetAppInfo(&appInfo);
	entry_ref thesaurusRef = appInfo.ref;
	thesaurusRef.set_name(".");
	BDirectory exeDir(&thesaurusRef);
	BPath thesaurusPath(&exeDir, "thesaurus");

	const char *inputFile = thesaurusPath.Path();
	const char *app = 0;
	int window = 0;
	const char *viewName = "0";

	int nextArg = 1;
	if (argc > 1) {
		while (nextArg < argc) {
			const char *option = argv[nextArg++];
			if (strcmp(option, "--help") == 0) {
				fprintf(stderr, "Arguments:\n");
				fprintf(stderr, " --help                  Help\n");
				fprintf(stderr, " --file <filename>       Load thesaurus from this file\n");
				fprintf(stderr, " --app <app name>        Check this application\n");
				fprintf(stderr, " --window <window num>   Check this numbered window of the app\n");
				fprintf(stderr, " --view <view name>      View to check\n");
				fprintf(stderr, "\n");
				fprintf(stderr, "   For example, to check the first StyledEdit window, type:\n");
				fprintf(stderr, "   %s --app StyledEdit --window 0 --view text\n", argv[0]);
				return 0;
			} else if (strcmp(option, "--file") == 0) {
				if (nextArg >= argc) {
					fprintf(stderr, "You need to specify a filename!\n");
					return -1;
				}
				
				inputFile = argv[nextArg++];
			} else if (strcmp(option, "--app") == 0) {
				if (nextArg >= argc) {
					fprintf(stderr, "You need to specify an application name!\n");
					return -1;
				}
				
				app = argv[nextArg++];
			} else if (strcmp(option, "--window") == 0) {
				if (nextArg >= argc) {
					fprintf(stderr, "You need to specify an window number!\n");
					return -1;
				}

				window = atoi(argv[nextArg++]);
			} else if (strcmp(option, "--view") == 0) {
				if (nextArg >= argc) {
					fprintf(stderr, "You need to specify a view list!\n");
					return -1;
				}

				viewName = argv[nextArg++];
			} else {
				fprintf(stderr, "Unrecognized option: %s\n", argv[nextArg - 1]);
				fprintf(stderr, "Try %s --help\n", argv[0]);
				return -1;
			}
		}	
	}

	BMessenger viewMessenger;
	if (app) {
		BRoster roster;
		BList appList;
		roster.GetAppList(&appList);
		for (int appNum = 0; appNum < appList.CountItems(); appNum++) {
			app_info info;
			roster.GetRunningAppInfo((team_id) appList.ItemAt(appNum), &info);
			
			if (strcmp(info.ref.name, app) == 0) {
				BMessenger msgn(info.signature);
				BMessage testMessage(B_GET_PROPERTY);
				testMessage.AddSpecifier("Messenger");	// Property of BHandler
				testMessage.AddSpecifier("View", viewName);			
				testMessage.AddSpecifier("Window", window);
			
				BMessage reply;
				if (msgn.SendMessage(&testMessage, &reply, 1000000, 1000000) == B_OK) {
					if (reply.FindMessenger("result", &viewMessenger) != B_OK) {
						fprintf(stderr, "Couldn't find view\n");
						return -1;
					}
					
					break;
				} else {
					fprintf(stderr, "Couldn't talk to app\n");
					return -1;
				}
			}		
		}	
	}
	
	ThesaurusWindow *mainWindow = new ThesaurusWindow;
	mainWindow->Show();

	// Read dictionary.
	status_t err = dictionary.ReadFromFile(inputFile);
	if ( err != B_OK) {
		BString errMsg = "Error reading data file:\n";
		errMsg += strerror(err);
		(new BAlert("Error!", errMsg.String(), "Quit"))->Go();
		return -1;
	}

	mainWindow->EnableChecking();

	if (app)
		mainWindow->SetMessenger(viewMessenger);

	thesaurusApp.Run();
}
