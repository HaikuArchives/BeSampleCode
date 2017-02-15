/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include <stdio.h>
#include <string.h>

#include <Application.h>
#include <FindDirectory.h>
#include <Directory.h>
#include <FilePanel.h>
#include <Message.h>
#include <Path.h>

#include "MessageWriter.h"
#include "win.h"
#include "main.h"

#define APP_SIG "application/x-vnd.Be.InputRecorder"

#define APP_VERSION "0.3"
/*
	Version Hisory:
	0.3 - released with the R4.5 CD sample code
	0.2 - released with Steven Black's March nerwsletter article.
	0.1.x - lacked version information
*/

TApp::TApp() : BApplication(APP_SIG)
{	
	BPath path;
	find_directory(B_USER_SETTINGS_DIRECTORY, &path);
	path.Append("InputRecorder");
	path.Append("records");
	create_directory(path.Path(),0755);

	BMimeType mime;
	status_t err;
	if((err = mime.SetTo(DATA_TYPE))!=B_OK){
		fprintf(stderr,"%s:%d - err: (0x%lx) %s\n",__FILE__,__LINE__,err,strerror(err));
	} else if ( !mime.IsInstalled()) {
		err = mime.Install();
		err = mime.SetShortDescription("Input Record");
		err = mime.SetLongDescription("Record of Input Events");
		err = mime.SetPreferredApp(APP_SIG);
	}

	BRect r(50,50,300,120);
	BWindow *w = new TWin(r);
	w->Show();

	_Load = new BFilePanel(B_OPEN_PANEL);
	_Load->SetPanelDirectory(path.Path());
	_Load->Window()->SetTitle("Load InputRecord");

	_Save = new BFilePanel(B_SAVE_PANEL);
	_Save->SetPanelDirectory(path.Path());
	_Save->Window()->SetTitle("New InputRecord");
}

void TApp::ArgvReceived(int32 argc, char **argv)
{
	int32 filename = -1;
	bool doloop = false;
	bool dolock = false;
	bool quitonstop = true;
	BMessage *mess;
	for(int32 i = 1; i < argc ; i++){
		if(!strcmp(argv[i],"--file") || !strcmp(argv[i],"-f")){
			if(++i < argc) filename = i;
		}else if(!strcmp(argv[i],"--loop") || !strcmp(argv[i],"-l")){
			doloop = true;
		}else if(!strcmp(argv[i],"--lock") || !strcmp(argv[i],"-k")){
			dolock = true;
		}else if(!strcmp(argv[i],"--unlock") || !strcmp(argv[i],"-u")){
			mess = new BMessage(T_SET_INPUT_LOCK);
			WindowAt(0)->PostMessage(mess);
		}else if(!strcmp(argv[i],"--quit") || !strcmp(argv[i],"-q")){
			quitonstop = true;
		}else if(!strcmp(argv[i],"--no-loop") || !strcmp(argv[i],"-nl")){
			doloop = false;
		}else if(!strcmp(argv[i],"--no-lock") || !strcmp(argv[i],"-nk")){
			dolock = false;
		}else if(!strcmp(argv[i],"--no-quit") || !strcmp(argv[i],"-nq")){
			quitonstop = false;
		}else if(!strcmp(argv[i],"--version")){
			printf("Version %s\n", APP_VERSION);
			Quit();
		}else if(!strcmp(argv[i],"--help") || !strcmp(argv[i],"-h")){
			printf(
				"--version : version information\n"
				"--file | -f : next argument is the input file\n"
				"--loop | -l : do this input forever\n"
				"--lock | -k : accept no user input while running\n"
				"--unlock | -u : force an unlock of user-input\n"
				"--quit | -q : quit when done\n"
				"--no-loop | -nl : do not do this input forever\n"
				"--no-lock | -nk : accept user input while running\n"
				"--no-quit | -nq : do not quit when done\n"
				"--help | -h : this help message\n"
			);
			Quit();
		}else {
			printf("Unknown argument:%s\n",argv[i]);
			Quit();
		}
	}
	mess = new BMessage(T_START_INPUT);
	if(filename!=-1) mess->AddString("Path",argv[filename]);
	mess->AddBool("LoopState",doloop);
	mess->AddBool("AutoPlay",filename==-1?false:true);
	mess->AddBool("QuitOnStop",quitonstop);
	mess->AddBool("LockInput",dolock);

	status_t err;
	if((err = WindowAt(0)->PostMessage(mess)) != B_OK) printf("Error : (0x%lx) %s\n", err, strerror(err));

}

void TApp :: MessageReceived(BMessage *msg)
{
	switch(msg->what){
	case T_OPEN:
		_Load->Show();
		break;
	case T_SAVE:
	case T_SAVEAS:
		_Save->Show();
		break;
	case B_SAVE_REQUESTED: {
		uint32 type; 
		int32 count;
		entry_ref ref;
		BPath path;
	
		type = B_ANY_TYPE;
		msg->GetInfo("directory", &type, &count); 
		if ( type == B_REF_TYPE && msg->FindRef("directory", 0, &ref) == B_OK ) { 
			BEntry entry(&ref, true);
			if ( entry.GetPath(&path)==B_OK){
				const char *name;
				if(msg->FindString("name",&name) == B_OK){
					path.Append(name);
					BMessage *mess = new BMessage(T_SAVE);
					mess->AddString("Path",path.Path());
					WindowAt(0)->PostMessage(mess);
				}
			}
		} 
		break;
	}
	default:
		BApplication::MessageReceived(msg);
		break;
	}
}

void TApp :: RefsReceived(BMessage *msg)
{
	uint32 type; 
	int32 count;
	entry_ref ref;
	BPath path;

	type = B_ANY_TYPE;
	msg->GetInfo("refs", &type, &count); 
	if ( type != B_REF_TYPE ) return; 

	for ( long i = --count; i >= 0; i-- ) { 
		if ( msg->FindRef("refs", i, &ref) == B_OK ) { 
			BEntry entry(&ref, true);
			if ( entry.IsFile() && entry.GetPath(&path)==B_OK){
				BMessage *mess = new BMessage(T_START_INPUT);
				mess->AddString("Path",path.Path());
				mess->AddBool("AutoPlay",false);
				WindowAt(0)->PostMessage(mess);
			}
		} 
	} 
}

int main(int argc, char **argv)
{
	be_app = new TApp();
	be_app->Run();
	delete be_app;
	return (0);
}

