/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include <stdio.h>
#include <string.h>
#include <OS.h>
#include <TimeSource.h>
#include "array_delete.h"
#include "NodeLooper.h"

static status_t _node_looper(void* obj);
struct _loop_init_data {
	port_id port;
	MediaNodeWrapper node;
};
struct _loop_start_data {
	bigtime_t tpStart;
	bigtime_t tmStart;
	bigtime_t tmDuration;
};
enum {
	LOOP_START = 0x1000,
	LOOP_STOP
};
static const size_t LOOP_MAX_MSG_SIZE = 64;

NodeLooper::NodeLooper(MediaNodeWrapper node, bigtime_t start, bigtime_t dur)
	: m_node(node), m_tmStart(start), m_tmDuration(dur)
{
	printf("NodeLooper::NodeLooper()\n");
	m_port = create_port(3, "NodeLooper Port");
	_loop_init_data* data = new _loop_init_data;
	data->port = m_port;
	data->node = m_node;
	m_running = false;
	m_looper = spawn_thread(_node_looper, "NodeLooper Thread", B_NORMAL_PRIORITY, data);
	resume_thread(m_looper);
}

NodeLooper::~NodeLooper()
{
	printf("NodeLooper::~NodeLooper()\n");
	Stop();
	close_port(m_port);
}

void NodeLooper::Start()
{
	if (m_running)
		return;
		
	_loop_start_data data;
	data.tmStart = m_tmStart;
	data.tmDuration = m_tmDuration;
	
	bigtime_t latency;
	m_node.GetLatency(&latency);

	BTimeSource* ts = m_node.MakeTimeSource();	
	
	// make sure seek gets handled first!
	data.tpStart = ts->Now()+latency+1000;
	printf("NodeLooper: start @ %Ld\n", data.tpStart);	
	m_node.Seek(data.tmStart, data.tpStart-1);
	m_node.Start(data.tpStart);
	ts->Release();
	m_running = true;

	write_port(m_port, LOOP_START, &data, sizeof(_loop_start_data));
}

void NodeLooper::Stop()
{
	if (! m_running)
		return;
	
	printf("NodeLooper: stop\n");	
	m_node.Stop(0, true);
	m_running = false;
	
	write_port(m_port, LOOP_STOP, 0, 0);
}

status_t _node_looper(void* obj)
{
	_loop_init_data* initData = static_cast<_loop_init_data*>(obj);
	port_id port = initData->port;
	MediaNodeWrapper node = initData->node;
	delete initData;
	
	bool running = false;
	int32 what;
	uint8* buf = new uint8[LOOP_MAX_MSG_SIZE];
	array_delete<uint8> delbuf(buf);
	
	bigtime_t tpLastSeek = 0LL, tmStart = 0LL, tmDuration = 0LL, timeout = 0LL;
	while (true) {
		timeout = (running) ? 0 : 1000000;
		ssize_t err = read_port_etc(port, &what, buf, LOOP_MAX_MSG_SIZE, B_TIMEOUT, timeout);
		if (err >= 0) {
			switch (what) {
			case LOOP_START:
				{
					printf("NodeLooper: start\n");
					_loop_start_data* data = (_loop_start_data*)(buf);
					tpLastSeek = data->tpStart;
					tmStart = data->tmStart;
					tmDuration = data->tmDuration;
				}
				running = true;
				break;
			case LOOP_STOP:
				printf("NodeLooper: stop\n");
				running = false;
				break;
			default:
				break;
			}
		} else if (! (err == B_TIMED_OUT || err == B_WOULD_BLOCK)) {
			// either port has been closed or unknown error: time to quit
			break;
		}
		
		if (running) {
			tpLastSeek += tmDuration;
			printf("NodeLooper: looping at %Ld...\n", tpLastSeek);
			node.Seek(tmStart, tpLastSeek);
			node.Sync(tpLastSeek);		
			printf("NodeLooper: sync complete.\n");
		}
	}
	return B_OK;
}
