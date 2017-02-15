// LogWriter.cpp
/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include "LogWriter.h"
#include <stdio.h>
#include <string.h>

// this is the simpleminded implementation of a lookup function;
// could also use some sort of map.  this has the advantage of not
// requiring any runtime initialization.
static const char* log_what_to_string(log_what code)
{
	const char* s = "Unknown log_what code!";
	switch (code)
	{
	case LOG_QUIT : s = "LOG_QUIT"; break;
	case LOG_SET_RUN_MODE  : s = "LOG_SET_RUN_MODE"; break;
	case LOG_PREROLL : s = "LOG_PREROLL"; break;
	case LOG_SET_TIME_SOURCE : s = "LOG_SET_TIME_SOURCE"; break;
	case LOG_REQUEST_COMPLETED : s = "LOG_REQUEST_COMPLETED"; break;
	case LOG_GET_PARAM_VALUE : s = "LOG_GET_PARAM_VALUE"; break;
	case LOG_SET_PARAM_VALUE : s = "LOG_SET_PARAM_VALUE"; break;
	case LOG_FORMAT_SUGG_REQ : s = "LOG_FORMAT_SUGG_REQ"; break;
	case LOG_FORMAT_PROPOSAL : s = "LOG_FORMAT_PROPOSAL"; break;
	case LOG_FORMAT_CHANGE_REQ : s = "LOG_FORMAT_CHANGE_REQ"; break;
	case LOG_SET_BUFFER_GROUP : s = "LOG_SET_BUFFER_GROUP"; break;
	case LOG_VIDEO_CLIP_CHANGED : s = "LOG_VIDEO_CLIP_CHANGED"; break;
	case LOG_GET_LATENCY : s = "LOG_GET_LATENCY"; break;
	case LOG_PREPARE_TO_CONNECT : s = "LOG_PREPARE_TO_CONNECT"; break;
	case LOG_CONNECT : s = "LOG_CONNECT"; break;
	case LOG_DISCONNECT : s = "LOG_DISCONNECT"; break;
	case LOG_LATE_NOTICE_RECEIVED : s = "LOG_LATE_NOTICE_RECEIVED"; break;
	case LOG_ENABLE_OUTPUT : s = "LOG_ENABLE_OUTPUT"; break;
	case LOG_SET_PLAY_RATE : s = "LOG_SET_PLAY_RATE"; break;
	case LOG_ADDITIONAL_BUFFER : s = "LOG_ADDITIONAL_BUFFER"; break;
	case LOG_LATENCY_CHANGED : s = "LOG_LATENCY_CHANGED"; break;
	case LOG_HANDLE_MESSAGE : s = "LOG_HANDLE_MESSAGE"; break;
	case LOG_ACCEPT_FORMAT : s = "LOG_ACCEPT_FORMAT"; break;
	case LOG_BUFFER_RECEIVED : s = "LOG_BUFFER_RECEIVED"; break;
	case LOG_PRODUCER_DATA_STATUS : s = "LOG_PRODUCER_DATA_STATUS"; break;
	case LOG_CONNECTED : s = "LOG_CONNECTED"; break;
	case LOG_DISCONNECTED : s = "LOG_DISCONNECTED"; break;
	case LOG_FORMAT_CHANGED : s = "LOG_FORMAT_CHANGED"; break;
	case LOG_SEEK_TAG : s = "LOG_SEEK_TAG"; break;
	case LOG_REGISTERED : s = "LOG_REGISTERED"; break;
	case LOG_START : s = "LOG_START"; break;
	case LOG_STOP : s = "LOG_STOP"; break;
	case LOG_SEEK : s = "LOG_SEEK"; break;
	case LOG_TIMEWARP : s = "LOG_TIMEWARP"; break;
	case LOG_HANDLE_EVENT : s = "LOG_HANDLE_EVENT"; break;
	case LOG_HANDLE_UNKNOWN : s = "LOG_HANDLE_UNKNOWN"; break;
	case LOG_BUFFER_HANDLED : s = "LOG_BUFFER_HANDLED"; break;
	case LOG_START_HANDLED : s = "LOG_START_HANDLED"; break;
	case LOG_STOP_HANDLED : s = "LOG_STOP_HANDLED"; break;
	case LOG_SEEK_HANDLED : s = "LOG_SEEK_HANDLED"; break;
	case LOG_WARP_HANDLED : s = "LOG_WARP_HANDLED"; break;
	case LOG_DATA_STATUS_HANDLED : s = "LOG_DATA_STATUS_HANDLED"; break;
	case LOG_SET_PARAM_HANDLED : s = "LOG_SET_PARAM_HANDLED"; break;
	case LOG_INVALID_PARAM_HANDLED : s = "LOG_INVALID_PARAM_HANDLED"; break;
	}
	return s;
}

// Logging thread function
int32 LogWriterLoggingThread(void* arg)
{
	LogWriter* obj = static_cast<LogWriter*>(arg);
	port_id port = obj->mPort;

	// event loop

	bool done = false;
	do
	{
		log_message msg;
		int32 what;
		status_t n_bytes = ::read_port(port, &what, &msg, sizeof(log_message));
		if (n_bytes > 0)
		{
			obj->HandleMessage((log_what) what, msg);
			if (LOG_QUIT == what) done = true;
		}
		else
		{
			fprintf(stderr, "LogWriter failed (%s) in ::read_port()!\n", strerror(n_bytes));
		}
	} while (!done);

	// got the "quit" message; now we're done
	return 0;
}

// --------------------
// LogWriter class implementation
LogWriter::LogWriter(ostream* logStream)
	:	mLogStream(logStream)
{
	mPort = ::create_port(64, "LogWriter");
	mLogThread = ::spawn_thread(&LogWriterLoggingThread, "LogWriter", B_NORMAL_PRIORITY, this);
	::resume_thread(mLogThread);
}

LogWriter::~LogWriter()
{
	printf("LogWriter::~LogWriter() called\n");
	status_t err;
	log_message msg;

	Log(LOG_QUIT, msg);
	::wait_for_thread(mLogThread, &err);
}

// Log a message
//
// This method, called by the client, really just enqueues a message to the writer thread,
// which will deal with it in the HandleMessage() method.
void 
LogWriter::Log(log_what what, const log_message& data)
{
	bigtime_t now = ::system_time();
	log_message& nc_data = const_cast<log_message&>(data);
	nc_data.tstamp = now;
	::write_port(mPort, (int32) what, &data, sizeof(log_message));
}

// Enable or disable a particular log_what code's output
void 
LogWriter::SetEnabled(log_what what, bool enable)
{
	if (enable)	 mFilters.erase(what);
	else mFilters.insert(what);
}

// enabling everything means just clearing out the filter set
void 
LogWriter::EnableAllMessages()
{
	mFilters.clear();
}

// disabling everything is more tedious -- we have to add them all to the
// filter set, one by one
void 
LogWriter::DisableAllMessages()
{
//	mFilters.insert(LOG_QUIT);				// don't disable our quit messages
	mFilters.insert(LOG_SET_RUN_MODE);
	mFilters.insert(LOG_PREROLL);
	mFilters.insert(LOG_SET_TIME_SOURCE);
	mFilters.insert(LOG_REQUEST_COMPLETED);
	mFilters.insert(LOG_GET_PARAM_VALUE);
	mFilters.insert(LOG_SET_PARAM_VALUE);
	mFilters.insert(LOG_FORMAT_SUGG_REQ);
	mFilters.insert(LOG_FORMAT_PROPOSAL);
	mFilters.insert(LOG_FORMAT_CHANGE_REQ);
	mFilters.insert(LOG_SET_BUFFER_GROUP);
	mFilters.insert(LOG_VIDEO_CLIP_CHANGED);
	mFilters.insert(LOG_GET_LATENCY);
	mFilters.insert(LOG_PREPARE_TO_CONNECT);
	mFilters.insert(LOG_CONNECT);
	mFilters.insert(LOG_DISCONNECT);
	mFilters.insert(LOG_LATE_NOTICE_RECEIVED);
	mFilters.insert(LOG_ENABLE_OUTPUT);
	mFilters.insert(LOG_SET_PLAY_RATE);
	mFilters.insert(LOG_ADDITIONAL_BUFFER);
	mFilters.insert(LOG_LATENCY_CHANGED);
	mFilters.insert(LOG_HANDLE_MESSAGE);
	mFilters.insert(LOG_ACCEPT_FORMAT);
	mFilters.insert(LOG_BUFFER_RECEIVED);
	mFilters.insert(LOG_PRODUCER_DATA_STATUS);
	mFilters.insert(LOG_CONNECTED);
	mFilters.insert(LOG_DISCONNECTED);
	mFilters.insert(LOG_FORMAT_CHANGED);
	mFilters.insert(LOG_SEEK_TAG);
	mFilters.insert(LOG_REGISTERED);
	mFilters.insert(LOG_START);
	mFilters.insert(LOG_STOP);
	mFilters.insert(LOG_SEEK);
	mFilters.insert(LOG_TIMEWARP);
	mFilters.insert(LOG_HANDLE_EVENT);
//	mFilters.insert(LOG_HANDLE_UNKNOWN);			// don't disable the "unknown message" messages
	mFilters.insert(LOG_BUFFER_HANDLED);
	mFilters.insert(LOG_START_HANDLED);
	mFilters.insert(LOG_STOP_HANDLED);
	mFilters.insert(LOG_SEEK_HANDLED);
	mFilters.insert(LOG_WARP_HANDLED);
	mFilters.insert(LOG_DATA_STATUS_HANDLED);
	mFilters.insert(LOG_SET_PARAM_HANDLED);
	mFilters.insert(LOG_INVALID_PARAM_HANDLED);
}

// Writer thread's message handling function -- this is where messages are actuall
// formatted and written to the log file
void 
LogWriter::HandleMessage(log_what what, const log_message& msg)
{
//	char buf[256];		// scratch buffer for building logged output

	// if we've been told to ignore this message type, just return without doing anything else
	if (mFilters.find(what) != mFilters.end()) return;

	// always write the message's type and timestamp
	*mLogStream << left << setw(24) << log_what_to_string(what);
	*mLogStream << " : realtime = " << msg.tstamp << ", perftime = " << msg.now << endl;
	
	// put any special per-message-type handling here
	switch (what)
	{
	case LOG_QUIT:
		*mLogStream << "\tLogWriter thread terminating" << endl;
		break;

	case LOG_BUFFER_RECEIVED:
		if (msg.buffer_data.offset < 0)
		{
			*mLogStream << "\tstart = " << msg.buffer_data.start_time;
			*mLogStream << ", offset = " << msg.buffer_data.offset << '\n';
			*mLogStream << "\tBuffer received *LATE*" << endl;
		}
		break;

	case LOG_SET_PARAM_HANDLED:
		*mLogStream << "\tparam id = " << msg.param.id;
		*mLogStream << ", value = " << msg.param.value << endl;
		break;

	case LOG_INVALID_PARAM_HANDLED:
	case LOG_GET_PARAM_VALUE:
		*mLogStream << "\tparam id = " << msg.param.id << endl;
		break;

	case LOG_BUFFER_HANDLED:
		*mLogStream << "\tstart = " << msg.buffer_data.start_time;
		*mLogStream << ", offset = " << msg.buffer_data.offset << endl;
		if (msg.buffer_data.offset < 0)
		{
			*mLogStream << "\tBuffer handled *LATE*" << endl;
		}
		break;

	case LOG_DATA_STATUS_HANDLED:
		*mLogStream << "\tstatus = " << int(msg.data_status.status) << endl;
		break;

	case LOG_HANDLE_UNKNOWN:
		*mLogStream << "\tUNKNOWN EVENT CODE: " << int(msg.unknown.what) << endl;
		break;

	default:
		break;
	}
}
