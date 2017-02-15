/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <Debug.h>
#include <List.h>
#include <Message.h>
#include <OS.h>

#include <add-ons/input_server/InputServerFilter.h>

extern "C" _EXPORT BInputServerFilter* instantiate_input_filter();

#define FILTER_ADD 'Fadd'
#define FILTER_REMOVE 'Frem'
#define FILTER_PASSMODE 'Fpas'
#define FILTER_INPUT 'Finp'

class InputRecorderFilter : public BInputServerFilter 
{
public:
	InputRecorderFilter();
	virtual ~InputRecorderFilter();
	virtual	filter_result Filter(BMessage *message, BList *outList);
private:
	thread_id _WatchThread;
	static int32 _StartWatchPort(void *arg);
	void _WatchPort(void); 

	struct port_ll {
		team_id team;
		port_id port;
		struct port_ll *next;
	};

	port_ll *_First; 
	port_id _CommandPort; 
	int32 _InputLockMode;
};

BInputServerFilter* instantiate_input_filter()
{
	return (new InputRecorderFilter());
}


InputRecorderFilter::InputRecorderFilter()
{
	_InputLockMode = 0;
	_First = NULL;
	_CommandPort = create_port(100,"InputFilterPort");
	if(_CommandPort < 0) {
		_sPrintf("inputRecorderFilter: create_port error: (0x%x) %s\n",_CommandPort,strerror(_CommandPort));
	}
	_WatchThread = spawn_thread(_StartWatchPort, "InputRecorderFilter", B_REAL_TIME_DISPLAY_PRIORITY+4, this);
	resume_thread(_WatchThread);
}

InputRecorderFilter::~InputRecorderFilter()
{
	kill_thread(_WatchThread);
	port_ll *n;
	while(_First!=NULL){
		n = _First->next;
		delete _First;
		_First = n;
	}
	delete_port(_CommandPort);
}

int32 InputRecorderFilter::_StartWatchPort(void *arg)
{
	InputRecorderFilter *self = (InputRecorderFilter*)arg;
	self->_WatchPort();
	return (B_NO_ERROR);
}

void InputRecorderFilter::_WatchPort(void)
{
	int32 code;
	ssize_t length;
	char *buffer;
	BMessage *event;
	status_t err;

	while (true) {
		// Block until we find the size of the next message
		length = port_buffer_size(_CommandPort);
		buffer = new char[length];
		event = NULL;
		event = new BMessage();
		err = read_port(_CommandPort, &code, buffer, length);
		if(err != length) {
			if(err >= 0) {
				_sPrintf("inputRecorderFilter: failed to read full packet (read %u of %u)\n",err,length);
			} else {
				_sPrintf("inputRecorderFilter: read_port error: (0x%x) %s\n",err,strerror(err));
			}
		} else if ((err = event->Unflatten(buffer)) < 0) {
			_sPrintf("inputRecorderFilter: (0x%x) %s\n",err,strerror(err));
		} else {
			port_ll *ll = NULL;
			port_id port;
			team_id team;
			switch(event->what){
				case FILTER_ADD: {
					if(event->FindInt32("RecorderPort", &port)==B_OK){
						if(event->FindInt32("RecorderTeam", &team)!=B_OK){
							port_info pinfo;
							if((err = get_port_info(port, &pinfo))==B_OK){
								team = pinfo.team;
							}else{
								_sPrintf("inputRecorderFilter: get_port_info(%d) : (0x%x) %s\n",port,err,strerror(err));
								team = -1;
								port = -1;
							}
						}
						if(team != -1 && port != -1){
							ll = new port_ll;
							ll->port = port;
							ll->team = team;
							ll->next = _First;
							_First = ll;
							ll = NULL;
						}
					}
					break;
				}
				case FILTER_REMOVE: {
					port_id port;
					port_ll *cur;
					port_ll *par = NULL;
					if(event->FindInt32("RecorderPort", &port)==B_OK){
						cur = _First;
						while(cur!=NULL && cur->port != port){
							par = cur;
							cur = cur->next;
						}
						if(cur!=NULL){
							port_ll *tmp;
							if(par!=NULL){
								tmp = cur->next;
								delete cur;
								par->next = tmp;
							} else {
								tmp = cur->next;
								delete cur;
								_First = tmp;
							}
						}
					}
					break;
				}
				case FILTER_PASSMODE: {
					bool mode;
					if(event->FindBool("FilterRealDevices", &mode)==B_OK){
						if(mode) _InputLockMode++;
						else _InputLockMode--;
						if(_InputLockMode < 0) _InputLockMode = 0;
						// _sPrintf("inputRecorderFilter: _InputLockMode = %d\n",_InputLockMode);
					}
					break;
				}
				default: {
					_sPrintf("inputRecorderFilter: get_port_info(%d) : (0x%x) %s\n",port,err,strerror(err));
				}
			}
			delete buffer;
			if(event!=NULL) {
				delete(event);
				event = NULL;
			}
		}
	}
}


filter_result InputRecorderFilter::Filter(BMessage *message, BList *outList)
{
	port_info pinfo;
	status_t err;
	filter_result res = B_DISPATCH_MESSAGE;
	if(_InputLockMode && message->FindInt32("FilterPass",&err)!=B_OK && message->what!=B_MODIFIERS_CHANGED) {
		res = B_SKIP_MESSAGE;
	} else if(_First!=NULL){
		size_t length = message->FlattenedSize();
		char *stream=(char*)malloc(length);
		if(stream){
			port_ll *cur = _First;
			port_ll *par = NULL;
			while(cur!=NULL){
				port_id port = cur->port;
				if((err = get_port_info(port, &pinfo))==B_OK){
					if(cur->team != pinfo.team){
						port_ll *tmp;
						tmp = cur->next;
						delete cur;
						if(par==NULL) _First = tmp;
						else par->next = tmp;
						cur = tmp;
						_sPrintf("inputRecorderFilter: port (%d) now owned by different team\n",port, err,strerror(err));
						port = -1;
					} else {
						if ( message->Flatten(stream, length) == B_OK) {
							write_port(port, FILTER_INPUT, stream, length);
						}
					}
				}else{
					port_ll *tmp;
					tmp = cur->next;
					delete cur;
					if(par==NULL) _First = tmp;
					else par->next = tmp;
					cur = tmp;
					_sPrintf("inputRecorderFilter: get_port_info(%d) : (0x%x) %s\n",port,err,strerror(err));
					port = -1;
				}
				if(port!=-1){
					par = cur;
					cur = cur->next;
				}
			}
			free(stream);
		}
	}	
	return (res);
}
