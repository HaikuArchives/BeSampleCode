/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef __TWIN_H
#define __TWIN_H 1

#include <Window.h>

class TWin: public BWindow
{
public:
	TWin(BRect frame);
	virtual ~TWin();

	virtual bool QuitRequested();
	virtual void MessageReceived(BMessage *msg);
private:
	void _SetInputLock(bool state, port_id commandPort);
	void _SetLoopMode(bool state);
	void _SetLockMode(bool state);
	void _SetFileName(const char *file, bool dirty);
	void _SetFileName(const char *file) { _SetFileName(file, _FileDirty); };

	status_t _StartThread ( int32 which );
	static int32 _StartRecordThread(void *arg);
	static int32 _StartPlayThread(void *arg);
	int32 _RecordThread(void);
	int32 _PlayThread(void);
	thread_id _Thread;

	status_t _CopyTmpFileToSaveFile(void);

	BPath _Filename;
	BPath __internal_filename;
	bool _QuitWhenDone;
	bool _LoopMode;
	bool _LockMode;
	bool _StopRequested;
	bool _FileDirty;
};

#endif // __TWIN_H
