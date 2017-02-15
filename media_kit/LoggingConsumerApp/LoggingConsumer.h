// LoggingConsumer.h

/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef LoggingConsumer_H
#define LoggingConsumer_H 1

#include <media/BufferConsumer.h>
#include <media/Controllable.h>
#include <media/MediaEventLooper.h>
#include "LogWriter.h"

// forward declarations
class BTimeSource;
class BMediaAddOn;
class BParameterWeb;

// --------------------
// LoggingConsumer node class

class LoggingConsumer :
	public BBufferConsumer,
	public BControllable,
	public BMediaEventLooper
{
public:
	LoggingConsumer(const entry_ref& logFile);
	~LoggingConsumer();

// Our own logging-control methods
	void SetEnabled(log_what what, bool enable);
	void EnableAllMessages();
	void DisableAllMessages();

// A little bit of instrumentation
	long LateBuffers() const { return mLateBuffers; }
	void ResetLateBufferCount() { mLateBuffers = 0; }

// Methods from BMediaNode
	BMediaAddOn* AddOn(int32*) const;
	void SetRunMode(run_mode);
	void Preroll();
	void SetTimeSource(BTimeSource* time_source);
	status_t RequestCompleted(const media_request_info & info);

// Methods from BControllable
	status_t GetParameterValue(
		int32 id,
		bigtime_t* last_change,
		void* value,
		size_t* ioSize);

	void SetParameterValue(
		int32 id,
		bigtime_t when,
		const void* value,
		size_t size);

// Methods from BBufferConsumer
	status_t HandleMessage(
		int32 message,
		const void* data,
		size_t size );

// all of these are pure virtual in BBufferConsumer
	status_t AcceptFormat(
		const media_destination& dest,
		media_format* format);

	status_t GetNextInput(
		int32* cookie,
		media_input* out_input);

	void DisposeInputCookie( int32 cookie );

	void BufferReceived( BBuffer* buffer );

	void ProducerDataStatus(
		const media_destination& for_whom,
		int32 status,
		bigtime_t at_performance_time);

	status_t GetLatencyFor(
		const media_destination& for_whom,
		bigtime_t* out_latency,
		media_node_id* out_timesource);

	status_t Connected(
		const media_source& producer,	/* here's a good place to request buffer group usage */
		const media_destination& where,
		const media_format& with_format,
		media_input* out_input);

	void Disconnected(
		const media_source& producer,
		const media_destination& where);

	/* The notification comes from the upstream producer, so he's already cool with */
	/* the format; you should not ask him about it in here. */
	status_t FormatChanged(
		const media_source& producer,
		const media_destination& consumer, 
		int32 change_tag,
		const media_format& format);

	/* Given a performance time of some previous buffer, retrieve the remembered tag */
	/* of the closest (previous or exact) performance time. Set *out_flags to 0; the */
	/* idea being that flags can be added later, and the understood flags returned in */
	/* *out_flags. */
	status_t SeekTagRequested(
		const media_destination& destination,
		bigtime_t in_target_time,
		uint32 in_flags, 
		media_seek_tag* out_seek_tag,
		bigtime_t* out_tagged_time,
		uint32* out_flags);

// Methods from BMediaEventLooper

	void NodeRegistered();
	void Start( bigtime_t performance_time );
	void Stop( bigtime_t performance_time, bool immediate );
	void Seek( bigtime_t media_time, bigtime_t performance_time );
	void TimeWarp( bigtime_t at_real_time, bigtime_t to_performance_time );

	// The primary event processing method
	void HandleEvent(const media_timed_event *event, bigtime_t lateness, bool realTimeEvent = false);

// Workaround for a bug in the Metrowerks PPC compile
protected:
	status_t DeleteHook(BMediaNode* node);

// Private stuff -- various data we need for the logging implementation and parameter handling
private:
	entry_ref mLogRef;								// file that we're logging to
	media_input mInput;								// descriptor of our single input
	BParameterWeb* mWeb;						// description of our controllable parameters
	LogWriter* mLogger;								// the actual logging object that we use
	bigtime_t mSchedulingLatency;			// our scheduling latency (estimated at run time)
	long mLateBuffers;									// track how many late buffers we've gotten
	int32 mProducerDataStatus;					// whether our producer has data available

	// controllable parameters and their change history
	bigtime_t mLatency;								// our internal latency
	float mSpinPercentage;							// how much of our latency time to spin the CPU
	int32 mPriority;										// our control thread's priority
	bigtime_t mLastLatencyChange;			// when did we last change our latency?
	bigtime_t mLastSpinChange;					// when did we last change our CPU usage?
	bigtime_t mLastPrioChange;					// when did we last change thread priority?
};

#endif
