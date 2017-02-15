// LoggingConsumer.cpp
/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include "LoggingConsumer.h"
#include "LogWriter.h"
#include <media/MediaRoster.h>
#include <media/TimeSource.h>
#include <media/ParameterWeb.h>
#include <media/Buffer.h>
#include <kernel/OS.h>
#include <string.h>
#include <stdio.h>

// id's of the node's BParameters
const int32 INPUT_NULL_PARAM = 1;
const int32 LATENCY_PARAM = 2;
const int32 OUTPUT_NULL_PARAM = 3;
const int32 CPU_NULL_PARAM = 11;
const int32 CPU_SPIN_PARAM = 12;
const int32 PRIO_NULL_PARAM = 21;
const int32 PRIORITY_PARAM = 22;

// build the LoggingConsumer's BParameterWeb
static BParameterWeb* build_parameter_web()
{
	BParameterWeb* web = new BParameterWeb;

	BParameterGroup* mainGroup = web->MakeGroup("LoggingConsumer Parameters");
	BParameterGroup* group = mainGroup->MakeGroup("Latency Control");
	BParameter* nullParam = group->MakeNullParameter(INPUT_NULL_PARAM, B_MEDIA_NO_TYPE, "Latency", B_GENERIC);
	BParameter* latencyParam = group->MakeContinuousParameter(LATENCY_PARAM, B_MEDIA_NO_TYPE, "",
		B_GAIN, "msec", 5, 100, 5);
	nullParam->AddOutput(latencyParam);
	latencyParam->AddInput(nullParam);

	group = mainGroup->MakeGroup("CPU Percentage");
	nullParam = group->MakeNullParameter(CPU_NULL_PARAM, B_MEDIA_NO_TYPE, "CPU Spin Percentage", B_GENERIC);
	BContinuousParameter* cpuParam = group->MakeContinuousParameter(CPU_SPIN_PARAM, B_MEDIA_NO_TYPE, "",
		B_GAIN, "percent", 5, 80, 5);
	nullParam->AddOutput(cpuParam);
	cpuParam->AddInput(nullParam);

	group = mainGroup->MakeGroup("Priority");
	nullParam = group->MakeNullParameter(PRIO_NULL_PARAM, B_MEDIA_NO_TYPE, "Thread Priority", B_GENERIC);
	BDiscreteParameter* prioParam = group->MakeDiscreteParameter(PRIORITY_PARAM, B_MEDIA_NO_TYPE, "", B_GENERIC);
	prioParam->AddItem(5, "B_LOW_PRIORITY");
	prioParam->AddItem(10, "B_NORMAL_PRIORITY");
	prioParam->AddItem(15, "B_DISPLAY_PRIORITY");
	prioParam->AddItem(20, "B_URGENT_DISPLAY_PRIORITY");
	prioParam->AddItem(100, "B_REAL_TIME_DISPLAY_PRIORITY");
	prioParam->AddItem(110, "B_URGENT_PRIORITY");
	prioParam->AddItem(120, "B_REAL_TIME_PRIORITY");
	nullParam->AddOutput(prioParam);
	prioParam->AddInput(nullParam);

	return web;
}

// --------------------
// LoggingConsumer class implementation

//#pragma mark -

LoggingConsumer::LoggingConsumer(const entry_ref& logFile)
	:	BMediaNode("LoggingConsumer"),
		BBufferConsumer(B_MEDIA_UNKNOWN_TYPE),
		BControllable(),
		BMediaEventLooper(),
		mLogRef(logFile), mWeb(NULL),
		mLateBuffers(0),
		mLatency(50 * 1000),		// default to 50 milliseconds
		mSpinPercentage(0.10),		// default to spinning 10% of total latency
		mPriority(B_REAL_TIME_PRIORITY),
		mLastLatencyChange(0),
		mLastSpinChange(0),
		mLastPrioChange(0)
{
	// spin off the logging thread
	mLogger = new LogWriter(logFile);
}

LoggingConsumer::~LoggingConsumer()
{
	BMediaEventLooper::Quit();

	// BControllable::SetParameterWeb() deletes the old web for us
	SetParameterWeb(NULL);
	mWeb = NULL;

	// delete the logging thread only after the looper thread has quit, otherwise there's
	// a potential race condition with the looper thread trying to write to the now-
	// deleted log
	delete mLogger;
}

//
// Log message filtering control
//

void 
LoggingConsumer::SetEnabled(log_what what, bool enable)
{
	mLogger->SetEnabled(what, enable);
}

void 
LoggingConsumer::EnableAllMessages()
{
	mLogger->EnableAllMessages();
}

void 
LoggingConsumer::DisableAllMessages()
{
	mLogger->DisableAllMessages();
}

//
// BMediaNode methods
//

//#pragma mark -

BMediaAddOn*
LoggingConsumer::AddOn(int32 *) const
{
	return NULL;
}

void 
LoggingConsumer::SetRunMode(run_mode mode)
{
	log_message logMsg;
	logMsg.now = TimeSource()->Now();
	logMsg.runmode.mode = mode;
	mLogger->Log(LOG_SET_RUN_MODE, logMsg);

	// The BMediaEventLooper tracks our run mode for us
	BMediaEventLooper::SetRunMode(mode);
}

void 
LoggingConsumer::Preroll()
{
	log_message logMsg;
	logMsg.now = TimeSource()->Now();
	mLogger->Log(LOG_PREROLL, logMsg);

	BMediaEventLooper::Preroll();
}

void 
LoggingConsumer::SetTimeSource(BTimeSource* time_source)
{
	log_message logMsg;
	logMsg.now = TimeSource()->Now();
	mLogger->Log(LOG_SET_TIME_SOURCE, logMsg);

	BMediaNode::SetTimeSource(time_source);
}

status_t 
LoggingConsumer::RequestCompleted(const media_request_info &info)
{
	log_message logMsg;
	logMsg.now = TimeSource()->Now();
	mLogger->Log(LOG_REQUEST_COMPLETED, logMsg);

	return BMediaNode::RequestCompleted(info);
}

//
// BControllable methods
//

//#pragma mark -

status_t 
LoggingConsumer::GetParameterValue(int32 id, bigtime_t* last_change, void* value, size_t* ioSize)
{
	log_message logMsg;
	logMsg.now = TimeSource()->Now();
	logMsg.param.id = id;
	mLogger->Log(LOG_GET_PARAM_VALUE, logMsg);

	// return an error if the caller hasn't reserved enough space for the parameter data.
	// we know that all of our parameters fit in a float or int32 (4 bytes), so we can just
	// check for it once here, instead of on a per-parameter basis
	if (*ioSize < sizeof(float)) return B_ERROR;

	// write out the designated parameter data
	switch (id)
	{
	case LATENCY_PARAM:
		*last_change = mLastLatencyChange;
		*((float*) value) = mLatency / 1000;		// the BParameter reads milliseconds, not microseconds
		*ioSize = sizeof(float);
		break;

	case CPU_SPIN_PARAM:
		*last_change = mLastSpinChange;
		*((float*) value) = mSpinPercentage;
		*ioSize = sizeof(float);
		break;

	case PRIORITY_PARAM:
		*last_change = mLastPrioChange;
		*((int32*) value) = mPriority;
		*ioSize = sizeof(int32);
		break;

	default:
		return B_ERROR;
	}

	return B_OK;
}

void 
LoggingConsumer::SetParameterValue(int32 id, bigtime_t performance_time, const void* value, size_t size)
{
	log_message logMsg;
	logMsg.now = TimeSource()->Now();
	logMsg.param.id = id;
	mLogger->Log(LOG_SET_PARAM_VALUE, logMsg);

	// if it's one of our parameters, enqueue a "set parameter" event for handling at the appropriate time
	switch (id)
	{
	case LATENCY_PARAM:
	case CPU_SPIN_PARAM:
	case PRIORITY_PARAM:
		{
			// We can't just push the value pointer in the event, because it might be a pointer to some stack-
			// based value somewhere, and won't be valid by the time this event is popped off of the queue.
			// This means we have to copy it into the event; the appropriate place to put it is in the user_data
			// field.  Now, we *should* never get a data block that's bigger than the user_data field, but we
			// want to avoid crashing if something goes wrong and we do anyway.  So, we clamp the data
			// size to the size of the event's user_data field.

			// The following gross expression replaces "sizeof(media_timed_event::user_data)" because the
			// Metrowerks compiler isn't savvy enough to accept the modern C++ syntax.
			if (size > sizeof(((media_timed_event*)0)->user_data))
			{
				//fprintf(stderr, "Woah!  Got a parameter change with a %lu-byte data block!\n", size);
				size = sizeof(((media_timed_event*)0)->user_data);
			}
			// Construct the media_timed_event using the constructor that stuffs the user_data
			// with the provided data; in our case, it's the parameter value data block.
			media_timed_event event(performance_time, BTimedEventQueue::B_PARAMETER,
				NULL, BTimedEventQueue::B_NO_CLEANUP, size, id, (char*) value, size);
			EventQueue()->AddEvent(event);
		}
		break;

	default:		// do nothing for other parameter IDs
		break;
	}
}

//
// BBufferConsumer methods
//

//#pragma mark -

status_t 
LoggingConsumer::HandleMessage(int32 message, const void *data, size_t size)
{
	log_message logMsg;
	logMsg.now = TimeSource()->Now();
	mLogger->Log(LOG_HANDLE_MESSAGE, logMsg);

	// HandleMessage() is where you implement any private message protocols
	// that you want to use.  When messages are written to your node's control
	// port that are not recognized by any of the node superclasses, they'll be
	// passed to this method in your node's implementation for handling.  The
	// LoggingConsumer node doesn't support any private messages, so we just
	// return an error, indicating that the message wasn't handled.
	return B_ERROR;
}

// all of these next methods are pure virtual in BBufferConsumer

status_t 
LoggingConsumer::AcceptFormat(const media_destination& dest, media_format* format)
{
	log_message logMsg;
	logMsg.now = TimeSource()->Now();
	mLogger->Log(LOG_ACCEPT_FORMAT, logMsg);

	// return an error if this isn't really our one input's destination
	if (dest != mInput.destination) return B_MEDIA_BAD_DESTINATION;

	// the destination given really is our input, and we accept any kind of media data,
	// so now we just confirm that we can handle whatever the producer asked for.
	return B_OK;
}

status_t 
LoggingConsumer::GetNextInput(int32* cookie, media_input* out_input)
{
	// we have a single hardcoded input that can accept any kind of media data
	if (0 == *cookie)
	{
		mInput.format.type = B_MEDIA_UNKNOWN_TYPE;		// accept any format
		*out_input = mInput;
		*cookie = 1;
		return B_OK;
	}
	else return B_BAD_INDEX;
}

void 
LoggingConsumer::DisposeInputCookie(int32 /*cookie*/ )
{
	// we don't use any kind of state or extra storage for iterating over our
	// inputs, so we don't have to do any special disposal of input cookies.
}

void 
LoggingConsumer::BufferReceived(BBuffer* buffer)
{
	bigtime_t bufferStart = buffer->Header()->start_time;
	bigtime_t now = TimeSource()->Now();
	bigtime_t how_early = bufferStart - EventLatency() - SchedulingLatency() - now;

	log_message logMsg;
	logMsg.now = now;
	logMsg.buffer_data.start_time = bufferStart;
	logMsg.buffer_data.offset = how_early;
	logMsg.buffer_data.mode = RunMode();
	mLogger->Log(LOG_BUFFER_RECEIVED, logMsg);

	// There's a special case here with handling B_MEDIA_PARAMETERS buffers.
	// These contain sets of parameter value changes, with their own performance
	// times embedded in the buffers.  So, we want to dispatch those parameter
	// changes as their own events rather than pushing this buffer on the queue to
	// be handled later.
	if (B_MEDIA_PARAMETERS == buffer->Header()->type)
	{
		ApplyParameterData(buffer->Data(), buffer->SizeUsed());
		buffer->Recycle();
	}
	else		// ahh, it's a regular media buffer, so push it on the event queue
	{
		status_t err;
		media_timed_event event(buffer->Header()->start_time, BTimedEventQueue::B_HANDLE_BUFFER,
			buffer, BTimedEventQueue::B_RECYCLE_BUFFER);
		err = EventQueue()->AddEvent(event);

		// HandleEvent() will recycle the buffer.  However, if we incurred an error trying to
		// put the event into the queue, we have to recycle it ourselves, since HandleEvent()
		// will never see the buffer in that case.
		if (err) buffer->Recycle();
	}
}

void 
LoggingConsumer::ProducerDataStatus(const media_destination& for_whom, int32 status, bigtime_t at_performance_time)
{
	log_message logMsg;
	logMsg.now = TimeSource()->Now();
	logMsg.data_status.status = status;
	mLogger->Log(LOG_PRODUCER_DATA_STATUS, logMsg);

	if (for_whom == mInput.destination)
	{
		media_timed_event event(at_performance_time, BTimedEventQueue::B_DATA_STATUS,
			&mInput, BTimedEventQueue::B_NO_CLEANUP, status, 0, NULL);
		EventQueue()->AddEvent(event);
	}
}

status_t 
LoggingConsumer::GetLatencyFor(const media_destination& for_whom, bigtime_t* out_latency, media_node_id* out_timesource)
{
	// make sure this is one of my valid inputs
	if (for_whom != mInput.destination) return B_MEDIA_BAD_DESTINATION;

	// report internal latency + downstream latency here, NOT including scheduling latency.
	// we're a final consumer (no outputs), so we have no downstream latency.
	*out_latency = mLatency;
	*out_timesource = TimeSource()->ID();
	return B_OK;
}

status_t 
LoggingConsumer::Connected(
	const media_source& producer,
	const media_destination& where,
	const media_format& with_format,
	media_input* out_input)
{
	log_message logMsg;
	logMsg.now = TimeSource()->Now();
	mLogger->Log(LOG_CONNECTED, logMsg);

	if (where != mInput.destination) return B_MEDIA_BAD_DESTINATION;

	// calculate my latency here, because it may depend on buffer sizes/durations, then
	// tell the BMediaEventLooper how early we need to get the buffers
	SetEventLatency(mLatency);

	// record useful information about the connection, and return success
	mInput.source = producer;
	*out_input = mInput;
	return B_OK;
}

void 
LoggingConsumer::Disconnected(
	const media_source& producer,
	const media_destination& where)
{
	log_message logMsg;
	logMsg.now = TimeSource()->Now();
	mLogger->Log(LOG_DISCONNECTED, logMsg);

	// wipe out our input record
	::memset(&mInput, 0, sizeof(mInput));
}

status_t 
LoggingConsumer::FormatChanged(
	const media_source& producer,
	const media_destination& consumer,
	int32 change_tag,
	const media_format& format)
{
	log_message logMsg;
	logMsg.now = TimeSource()->Now();
	mLogger->Log(LOG_FORMAT_CHANGED, logMsg);

	return B_OK;
}

status_t 
LoggingConsumer::SeekTagRequested(
	const media_destination& destination,
	bigtime_t in_target_time,
	uint32 in_flags,
	media_seek_tag* out_seek_tag,
	bigtime_t* out_tagged_time,
	uint32* out_flags)
{
	log_message logMsg;
	logMsg.now = TimeSource()->Now();
	mLogger->Log(LOG_SEEK_TAG, logMsg);

	return B_OK;
}

//
// BMediaEventLooper virtual methods
//

//#pragma mark -

void 
LoggingConsumer::NodeRegistered()
{
	log_message logMsg;
	logMsg.now = TimeSource()->Now();
	mLogger->Log(LOG_REGISTERED, logMsg);

	// Set our priority and start the BMediaEventLooper's thread
	SetPriority(mPriority);
	Run();

	// Initialize as much of our input as we can, now that the Media Kit really "knows" about us
	mInput.destination.port = ControlPort();
	mInput.destination.id = 0;
	mInput.node = Node();
	::strcpy(mInput.name, "Logged input");

	// publish our parameter web
	mWeb = build_parameter_web();
	SetParameterWeb(mWeb);
}

void 
LoggingConsumer::Start(bigtime_t performance_time)
{
	log_message logMsg;
	logMsg.now = TimeSource()->Now();
	mLogger->Log(LOG_START, logMsg);

	BMediaEventLooper::Start(performance_time);
}

void 
LoggingConsumer::Stop(bigtime_t performance_time, bool immediate)
{
	log_message logMsg;
	logMsg.now = TimeSource()->Now();
	mLogger->Log(LOG_STOP, logMsg);

	BMediaEventLooper::Stop(performance_time, immediate);
}

void 
LoggingConsumer::Seek(bigtime_t media_time, bigtime_t performance_time)
{
	log_message logMsg;
	logMsg.now = TimeSource()->Now();
	mLogger->Log(LOG_SEEK, logMsg);

	BMediaEventLooper::Seek(media_time, performance_time);
}

void 
LoggingConsumer::TimeWarp(bigtime_t at_real_time, bigtime_t to_performance_time)
{
	log_message logMsg;
	logMsg.now = TimeSource()->Now();
	mLogger->Log(LOG_TIMEWARP, logMsg);

	BMediaEventLooper::TimeWarp(at_real_time, to_performance_time);
}

void 
LoggingConsumer::HandleEvent(const media_timed_event *event, bigtime_t /* lateness */, bool /* realTimeEvent */)
{
	log_message logMsg;
	logMsg.now = TimeSource()->Now();
	mLogger->Log(LOG_HANDLE_EVENT, logMsg);

	switch (event->type)
	{
	case BTimedEventQueue::B_HANDLE_BUFFER:
		{
			BBuffer* buffer = const_cast<BBuffer*>((BBuffer*) event->pointer);
			if (buffer)
			{
				media_header* hdr = buffer->Header();
				if (hdr->destination == mInput.destination.id)
				{
					bigtime_t now = TimeSource()->Now();
					bigtime_t perf_time = hdr->start_time;

					// the how_early calculate here doesn't include scheduling latency because
					// we've already been scheduled to handle the buffer
					bigtime_t how_early = perf_time - mLatency - now;

					// logMsg.now is already set
					logMsg.buffer_data.start_time = perf_time;
					logMsg.buffer_data.offset = how_early;
					logMsg.buffer_data.mode = RunMode();
					mLogger->Log(LOG_BUFFER_HANDLED, logMsg);

					// if the buffer is late, we ignore it and report the fact to the producer
					// who sent it to us
					if ((RunMode() != B_OFFLINE) &&				// lateness doesn't matter in offline mode...
						(RunMode() != B_RECORDING) &&		// ...or in recording mode
						(how_early < 0))
					{
						mLateBuffers++;
						NotifyLateProducer(mInput.source, -how_early, perf_time);
					}
					else
					{
						// burn some percentage of our stated latency in CPU time (controlled by
						// a BParameter).  this simulates a user-configurable amount of CPU cost
						// associated with the consumer.
						bigtime_t spin_start = ::system_time();
						bigtime_t spin_now = spin_start;
						bigtime_t usecToSpin = bigtime_t(mSpinPercentage / 100.0 * mLatency);
						while (spin_now - spin_start < usecToSpin)
						{
							for (long k = 0; k < 1000000; k++) { /* intentionally blank */ }
							spin_now = ::system_time();
						}
					}

					// Offline mode requires some special handling.  When running offline the producer
					// doesn't just spit buffers as fast as it can (and block waiting for downstream buffers
					// to be recycled), it waits for the consumer to explicitly request them.  This means
					// that it can react to other control messages while it's waiting to send new buffers.
					if ((B_OFFLINE == RunMode()) && (B_DATA_AVAILABLE == mProducerDataStatus))
					{
						status_t err = RequestAdditionalBuffer(mInput.source, buffer);
						if (err)
						{
							logMsg.error.error = err;
							mLogger->Log(LOG_ERROR, logMsg);
						}
					}
				}
				else
				{
					//fprintf(stderr, "* Woah!  Got a buffer for a different destination!\n");
				}

				// we're done "processing the buffer"; now recycle it and return to the loop
				buffer->Recycle();
			}
		}
		break;

	case BTimedEventQueue::B_PARAMETER:
		{
			size_t dataSize = size_t(event->data);
			int32 param = int32(event->bigdata);
			logMsg.param.id = param;

			// handle the message if there's sufficient data provided.  we only check against
			// sizeof(float) because all of our parameters happen to be 4 bytes.  if various
			// parameters took different amounts of data, we'd check the size on a per-parameter
			// basis.
			if (dataSize >= sizeof(float)) switch (param)
			{
			case LATENCY_PARAM:
				{
					float value = *((float*) event->user_data);
					mLatency = bigtime_t(value* 1000);
					mLastLatencyChange = logMsg.now;

					// my latency just changed, so reconfigure the BMediaEventLooper
					// to give me my events at the proper time
					SetEventLatency(mLatency);

					// tell the producer that my latency changed, and broadcast a message
					// about the parameter change to any applications that may be looking
					// for it through the BMediaRoster::StartWatching() mechanism.
					//
					// if we had more than one input, we'd need to tell *all* producers about
					// the change in our latency.
					SendLatencyChange(mInput.source, mInput.destination, EventLatency() + SchedulingLatency());
					BroadcastNewParameterValue(logMsg.now, param, &value, sizeof(value));

					// log the new latency value, for recordkeeping
					logMsg.param.value = value;
					mLogger->Log(LOG_SET_PARAM_HANDLED, logMsg);
				}
				break;

			case CPU_SPIN_PARAM:
				{
					float value = *((float*) event->user_data);
					mSpinPercentage = value;
					mLastSpinChange = logMsg.now;
					BroadcastNewParameterValue(logMsg.now, param, &value, sizeof(value));
					logMsg.param.value = value;
					mLogger->Log(LOG_SET_PARAM_HANDLED, logMsg);
				}
				break;

			case PRIORITY_PARAM:
				{
					mPriority = *((int32*) event->user_data);
					// DO NOT use ::set_thead_priority() to directly alter the node's control
					// thread priority.  BMediaEventLooper tracks the priority itself and recalculates
					// the node's scheduling latency whenever SetPriority() is called.  This is VERY
					// important for correct functioning of a node chain.  You should *only* alter a
					// BMediaEventLooper's priority by calling its SetPriority() method.
					SetPriority(mPriority);

					mLastPrioChange = logMsg.now;
					BroadcastNewParameterValue(logMsg.now, param, &mPriority, sizeof(mPriority));
					logMsg.param.value = (float) mPriority;
					mLogger->Log(LOG_SET_PARAM_HANDLED, logMsg);
				}
				break;

			// log the fact that we "handled" a "set parameter" event for a
			// nonexistent parameter
			default:
				mLogger->Log(LOG_INVALID_PARAM_HANDLED, logMsg);
				break;
			}
		}
		break;

	case BTimedEventQueue::B_START:
		// okay, let's go!
		mLogger->Log(LOG_START_HANDLED, logMsg);
		break;

	case BTimedEventQueue::B_STOP:
		mLogger->Log(LOG_STOP_HANDLED, logMsg);
		// stopping implies not handling any more buffers.  So, we flush all pending
		// buffers out of the event queue before returning to the event loop.
		EventQueue()->FlushEvents(0, BTimedEventQueue::B_ALWAYS, true, BTimedEventQueue::B_HANDLE_BUFFER);
		break;

	case BTimedEventQueue::B_SEEK:
		// seeking the log doesn't make any sense, so we just log that we handled the seek
		// and return without doing anything else
		mLogger->Log(LOG_SEEK_HANDLED, logMsg);
		break;

	case BTimedEventQueue::B_WARP:
		// similarly, time warps aren't meaningful to the logger, so just record it and return
		mLogger->Log(LOG_WARP_HANDLED, logMsg);
		break;

	case BTimedEventQueue::B_DATA_STATUS:
		// we track our producer's data status for cases like offline, where we need to
		// consult it in order to decide whether to request another buffer of data
		mProducerDataStatus = event->data;
		logMsg.data_status.status = event->data;
		mLogger->Log(LOG_DATA_STATUS_HANDLED, logMsg);
		break;

	default:
		// hmm, someone enqueued a message that we don't understand.  log and ignore it.
		logMsg.unknown.what = event->type;
		mLogger->Log(LOG_HANDLE_UNKNOWN, logMsg);
		break;
	}
}

// Workaround for a Metrowerks PPC compiler bug
status_t 
LoggingConsumer::DeleteHook(BMediaNode *node)
{
	return BMediaEventLooper::DeleteHook(node);
}

