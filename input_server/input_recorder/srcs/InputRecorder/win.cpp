/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include <Application.h>
#include <Alert.h>
#include <Debug.h>
#include <WindowScreen.h>
#include <Path.h>
#include <OS.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <String.h>

#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <malloc.h>

#include "win.h"
#include "MessageWriter.h"
#include "main.h"
#include "view.h"

#define DEFAULT_TITLE "Input Recorder"

#define MESSAGEWRITER FlatMessageWriter

status_t call_device(port_id devicePort, BMessage *event);

TWin::TWin(BRect frame) : BWindow(frame, DEFAULT_TITLE, B_TITLED_WINDOW, B_NOT_ZOOMABLE | B_NOT_RESIZABLE)
{
	BRect rect = Bounds();
	BMenuBar *menu;
	BMenuItem *item;
	BMenu *sub;

	menu = new BMenuBar(rect, "MenuBar");
	sub = new BMenu("File");

	sub->AddItem(item = new BMenuItem("Open" B_UTF8_ELLIPSIS, new BMessage(T_OPEN), 'O'));
	item->SetTarget(be_app);

	sub->AddItem(item = new BMenuItem("Save" B_UTF8_ELLIPSIS, new BMessage(T_SAVE), 'S'));

	sub->AddItem(item = new BMenuItem("Save As" B_UTF8_ELLIPSIS, new BMessage(T_SAVEAS)));
	item->SetTarget(be_app);

	sub->AddSeparatorItem();

	sub->AddItem(new BMenuItem("Close", new BMessage(B_CLOSE_REQUESTED),'W'));
	item->SetTarget(be_app);
	menu->AddItem(sub);

	sub = new BMenu("Options");

	sub->AddItem(new BMenuItem("Loop", new BMessage(T_TOGGLE_LOOP),'L'));
	sub->AddItem(new BMenuItem("Lock", new BMessage(T_TOGGLE_LOCK),'K'));
	menu->AddItem(sub);

	AddChild(menu);

	rect = menu->Bounds();
	float hi = rect.bottom;
	rect = Bounds();
	rect.top += hi+ 1;

	BView *view = new TView(rect, "View", true, false, true);
	AddChild(view);
	
	_Filename.Unset();
	char buf[64];
	sprintf(buf,"/tmp/InputRecord.%Lx",(uint64)find_thread(NULL));
	__internal_filename.SetTo(buf);
	
	_LoopMode = false;
	_LockMode = false;
	_StopRequested = false;
	_FileDirty = false;
	_QuitWhenDone = false;
	_Thread = -1;
}

TWin::~TWin()
{
	if(_Thread!=-1) kill_thread(_Thread);
	unlink(__internal_filename.Path());
}

bool TWin::QuitRequested()
{
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}

void TWin::_SetLoopMode(bool state)
{
	_LoopMode = state;
	BMenuBar *main = KeyMenuBar();
	BMenuItem *item;
	if(main!=NULL){
		item = main->FindItem("Loop");
		if(item!=NULL) item->SetMarked(_LoopMode);
	}
}

void TWin::_SetLockMode(bool state)
{
	_LockMode = state;
	BMenuBar *main = KeyMenuBar();
	BMenuItem *item;
	if(main!=NULL){
		item = main->FindItem("Lock");
		if(item!=NULL) item->SetMarked(_LockMode);
	}
}

void TWin::_SetFileName(const char *file, bool dirty)
{
	if(file!=NULL) {
		if(*file!='\0') _Filename.SetTo(file,NULL,true);
		else _Filename.Unset();
	}
	_FileDirty = dirty;
	if(Lock()){
		BString str;
		if(_Filename.InitCheck()==B_OK) {
			str = _Filename.Leaf();
		}else{
			str = DEFAULT_TITLE;
		}
		if(_FileDirty) str.Append(" (*)");
		SetTitle(str.String());
		Unlock();
	}
}

void TWin::MessageReceived(BMessage *msg)
{
	switch(msg->what) {
		case T_SET_INPUT_LOCK: {
			bool state = false;
			msg->FindBool("State",&state);
			_SetInputLock(state, -1);
			break;
		}
		case T_SAVE: {
			const char *file;
			if(msg->FindString("Path",&file)==B_OK) _SetFileName(file, true);
			if(_FileDirty){
				if(_Filename.InitCheck()==B_OK){
					_CopyTmpFileToSaveFile();
				} else {
					BMessage *mess = new BMessage(*msg);
					be_app->PostMessage(mess);
				}
			}
			break;
		}
		case T_TOGGLE_LOOP: {
			_SetLoopMode(!_LoopMode);
			break;
		}
		case T_TOGGLE_LOCK: {
			_SetLockMode(!_LockMode);
			break;
		}
		case T_START_INPUT: {
			const char *file;
			bool b;
			if(msg->FindString("Path",&file)==B_OK) _SetFileName(file, false);
			if(msg->FindBool("QuitOnStop",&_QuitWhenDone)!=B_OK) _QuitWhenDone = false;
			if(msg->FindBool("LoopState",&b)==B_OK) _SetLoopMode(b);
			if(msg->FindBool("LockInput",&b)==B_OK) _SetLockMode(b);
			if(msg->FindBool("AutoPlay",&b)!=B_OK) b = false;
			if(b) _StartThread(T_PLAY);
			break;
		}
		case B_SIMPLE_DATA: {
			uint32 type;
			int32 count = -1;
			msg->GetInfo("refs", &type, &count); 
			if ( count>0 && type == B_REF_TYPE ) {
				BMessage *mess = new BMessage(*msg);
				mess->what = B_REFS_RECEIVED;
				be_app->PostMessage(msg);
			}
			break;
		}

		case T_RECORD: {
			_SetFileName(NULL, true);
			_StartThread(T_RECORD);
			break;
		}
			
		case T_PLAY: { 
			_StartThread(T_PLAY);
			break;	 
		}
						 	 
		case T_STOP: {
			_StopRequested = true;
			break;
		}
		default: {
			BWindow::MessageReceived(msg); 
			break;
		}
	}
}

status_t TWin::_CopyTmpFileToSaveFile(void)
{
	BFile internal(__internal_filename.Path(),B_READ_ONLY);
	BFile real(_Filename.Path(),B_WRITE_ONLY|B_ERASE_FILE|B_CREATE_FILE);
	const int32 blocksize = 65536;
	char *attr = (char*)malloc(B_ATTR_NAME_LENGTH);
	char *data = (char*)malloc(blocksize);
	if(attr!=NULL && data!=NULL){
		ssize_t read, writ = 0;
		off_t offset = 0;
		do {
			read = internal.ReadAt(offset, data, blocksize);
			if(read > 0) writ = real.WriteAt(offset, data, read);
			else if(read<0) fprintf(stderr, "%s:%d - failed to read offset %Lx (0x%lx) %s\n", __FILE__, __LINE__, offset, read, strerror(read));
			if(read>0 && writ< 0) fprintf(stderr, "%s:%d - failed to write offset %Lx (0x%lx) %s\n", __FILE__, __LINE__, offset, writ, strerror(writ));
			offset += blocksize;
		}while(read > 0);
		status_t err;
		internal.RewindAttrs();
		do {
			err = internal.GetNextAttrName(attr);
			if(!err) {
				read = internal.ReadAttr(attr, 0, 0, data, blocksize);
				if(read > 0) writ = real.WriteAttr(attr, 0, 0, data, read);
				else fprintf(stderr, "%s:%d - failed to read (0x%lx) %s\n", __FILE__, __LINE__, read, strerror(read));
				if(read>0 && writ< 0) fprintf(stderr, "%s:%d - failed to write offset %Lx (0x%lx) %s\n", __FILE__, __LINE__, offset, writ, strerror(writ));
			}
		} while(!err);
		_SetFileName(NULL, false);
	}
	if(data) free(data);
	if(attr) free(attr);
	// should actually return error code
	return(B_OK);
}

status_t TWin::_StartThread (int32 which)
{
	if(_Thread!=-1) {
		BAlert *al = new BAlert("Again", "Already recording or playing.","Oops");
		fprintf(stderr, "%s:%d - _Thread not -1, assuming thread is still recording/playing.\n",__FILE__,__LINE__);
		al->Go(NULL);
		return(B_ERROR);
	} else {
		_StopRequested = false;
		switch(which){
		case T_RECORD:
			_Thread = spawn_thread (_StartRecordThread,"RecordThread",B_NORMAL_PRIORITY,this);
			break;
		case T_PLAY:
			_Thread = spawn_thread (_StartPlayThread,"PlayThread",B_NORMAL_PRIORITY,this);
			break;
		}
		return ( resume_thread (_Thread) );
	}
}

int32 TWin::_StartRecordThread(void *arg)
{
	TWin *self = (TWin *)arg;
	return (self->_RecordThread() );
}

int32 TWin::_RecordThread(void)
{
	MessageWriter *msw;
	BMessage cmd;
	BAlert *alert;

	port_id commandPort = find_port("InputFilterPort");
	if(commandPort < 0){
		fprintf(stderr, "InputRecorder: unable to find filter port\n");
		alert = new BAlert("InputRecorderError", "InputRecorder: Unable to find InputFilter", "Gasp");
		return (B_ERROR);
	}

	BMessage *mess = new BMessage(T_SET_BUTTONS);
	mess->AddBool("Record", false);
	mess->AddBool("Stop", true);
	mess->AddBool("Play", false);
	BView *view = FindView("View");
	if(view!=NULL) PostMessage(mess, view);
	else delete mess;

	port_id messagePort = create_port(2000,"InputRecorderPort"); 

	thread_info tinfo;
	cmd.what = FILTER_ADD;
	cmd.AddInt32("RecorderPort", messagePort);
	if(get_thread_info(find_thread(NULL),&tinfo)==B_OK){
		cmd.AddInt32("RecorderTeam", tinfo.team);
	}
	call_device(commandPort, &cmd);

	long code, length;
	char *buffer ;
	BMessage event;
	int32 eventWhat, lastEventWhat = -1;
	int64 eventWhen, lastEventWhen = -1;

	msw = new MESSAGEWRITER(__internal_filename.Path(), B_WRITE_ONLY);
	status_t err = B_OK;
	if((err = msw->InitCheck())!=B_OK) {
		printf("%s:%d - Failed to open file for write %s: (0x%lx) %s\n", __FILE__, __LINE__, __internal_filename.Path(), err, strerror(err));
	}

	set_mouse_position(320, 240);
	BPoint last(320, 240);
	BPoint cur(0,0);

	while(! _StopRequested) {
		length = port_buffer_size_etc(messagePort, B_TIMEOUT, 250);

		if (length != B_TIMED_OUT) {			
			buffer = (char*)malloc(length);
		
			read_port(messagePort, &code, buffer, length);
			event.Unflatten(buffer);
			eventWhat = event.what;
			event.FindInt64("when", &eventWhen);
			if(event.FindPoint("where", &cur)==B_OK){
				event.AddInt32("x", cur.x - last.x);
				event.AddInt32("y", last.y - cur.y);
				// Yes, 'y' is backwards. It's to allow backwards compatibility
				//   with a bug. (Steven Black)
				event.RemoveName("where");
				last = cur;
			}
			// seems that I receive the same BMessage twice, really don't know why (David)
			if ((eventWhen != lastEventWhen) || (eventWhat != lastEventWhat)) {
				if (msw->Write(&event) != B_OK) printf("Error writing file\n");
				lastEventWhen = eventWhen;
				lastEventWhat = eventWhat;
			} else {
				// I don't think this really occurs. (Steven Black)
				printf("Recieved: When: %Lx, last %Lx; What: %lx, last  %lx\n", eventWhen,
					lastEventWhen, eventWhat, lastEventWhat);
			}
			free(buffer);
		}
	}

	cmd.what = FILTER_REMOVE;
	call_device(commandPort, &cmd);

	delete msw;
	delete_port(messagePort);

	_Thread = -1;

	mess = new BMessage(T_SET_BUTTONS);
	mess->AddBool("Record", true);
	mess->AddBool("Stop", false);
	mess->AddBool("Play", true);
	view = FindView("View");
	if(view!=NULL) PostMessage(mess, view);
	else delete mess;

	return (true);	
}

long TWin::_StartPlayThread(void *arg)
{
	TWin *self = (TWin *)arg;
	return (self->_PlayThread() );
}

long TWin::_PlayThread(void)
{
	port_id devicePort;
	BMessage event;
	bool first_pass=true;
	bool end_of_file;
	bigtime_t lasttime = -1, newtime, offtime;
	MessageWriter *msr;

	BAlert *al;
	devicePort = find_port("InputDevicePort");
	if(devicePort < 0){
		fprintf(stderr, "InputRecorder: unable to find device port\n");
		al = new BAlert("InputRecorderError", "InputRecorder: unable to find device port", "Gasp");
		al->Go();
		return(B_ERROR);
	}

	port_id commandPort = find_port("InputFilterPort");
	if(commandPort < 0){
		fprintf(stderr, "InputRecorder: unable to find filter port, unable to lock\n");
		commandPort = -1;
	}

	BMessage *mess = new BMessage(T_SET_BUTTONS);
	mess->AddBool("Record", false);
	mess->AddBool("Stop", !_LoopMode);
	mess->AddBool("Play", false);
	BView *view = FindView("View");
	if(view!=NULL) PostMessage(mess, view);
	else delete mess;

	end_of_file = false;
	status_t err;
	
	const char *name = __internal_filename.Path();
	if(_Filename.InitCheck()==B_OK && !_FileDirty) name = _Filename.Path();
	msr = new MESSAGEWRITER(name,B_READ_ONLY);
	if((err = msr->InitCheck())!=B_OK) {
		printf("%s:%d - Failed to open file for read %s: (0x%lx) %s\n", __FILE__, __LINE__, name, err, strerror(err));
	}

	if(_LockMode){
		fprintf(stderr, "Locking input...");
		_SetInputLock(true, commandPort);
	}

	first_pass = true;
	while(! _StopRequested ) {
		if(modifiers() & B_SCROLL_LOCK){
			_StopRequested = true;
			continue;
		}
		
		if (first_pass) {
			BMessage initMessage(B_MOUSE_MOVED);
			initMessage.AddInt64("when", system_time());
			initMessage.AddPoint("where", BPoint(320, 240));
			initMessage.AddInt32("buttons", 0); 
			initMessage.AddInt32("modifiers", 0);
			initMessage.AddInt32("FilterPass", 0);
			call_device(devicePort, &initMessage);
			
			msr->ResetFilePointer();
		}

		if (msr->HaveMore()) {
			if(msr->Read(&event) != B_OK){
				fprintf(stderr, "InputRecorder: resource read error\n");
				al = new BAlert("InputRecorderError", "InputRecorder: input record read error", "Gasp");
				al->Go();
				_StopRequested = true;
			} else {
				event.AddInt32("FilterPass", 0);
				event.FindInt64("when",&newtime);
				offtime = newtime - lasttime;
				lasttime = newtime;

				if(first_pass){
					first_pass = false;
				}else {
					snooze(offtime); 
				}
				event.ReplaceInt64("when",system_time());
				if(call_device(devicePort, &event)!=B_OK){
					fprintf(stderr, "InputRecorder: failed to send packet\n");
					al = new BAlert("InputRecorderError", "InputRecorder: failed to send packet", "Gasp");
					al->Go();
					_StopRequested = true;
				}
			}

		} else {
			if(!_StopRequested && _LoopMode) first_pass = true;
			else _StopRequested = true;
		}
	}
	delete msr;

	if(_LockMode){
		fprintf(stderr, "Unlocking input...");
		_SetInputLock(false, commandPort);
	}

	if(_QuitWhenDone) QuitRequested();

	_Thread = -1;

	mess = new BMessage(T_SET_BUTTONS);
	mess->AddBool("Record", true);
	mess->AddBool("Stop", false);
	mess->AddBool("Play", true);
	view = FindView("View");
	if(view!=NULL) PostMessage(mess, view);
	else delete mess;

	return true;
}

void TWin::_SetInputLock(bool state, port_id commandPort)
{
	if(commandPort < 0){
		commandPort = find_port("InputFilterPort");
	}
	if(commandPort < 0){
		fprintf(stderr, "InputRecorder: unable to find filter port, unable to set lock state\n");
	}else{
		BMessage cmd(FILTER_PASSMODE);
		cmd.AddBool("FilterRealDevices", state);
		call_device(commandPort, &cmd);
	}
}

status_t call_device(port_id devicePort, BMessage *event)
{
	status_t err = B_OK;
	if(devicePort < 0) return(devicePort);
	size_t size = event->FlattenedSize();
	char *buffer = (char*)malloc(size);
	if(buffer!=NULL){
		event->Flatten(buffer, size);
		if((err = write_port(devicePort, 0, buffer, size))!=B_OK){
			fprintf(stderr, "Error finding writing to device port: (0x%lx) %s\n", err, strerror(err));
		}
		free(buffer);
	}else{
		if(errno < 0) err = errno;
		if(!err) err = B_ERROR;
		fprintf(stderr, "Failed to allocate space for device buffer: (0x%lx) %s\n", err, strerror(err));
	}
	return(err);
}

