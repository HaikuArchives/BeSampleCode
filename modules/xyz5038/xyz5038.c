/* xyz5038.c
 *
 * A module for the XYZ Systems Model 5038 Squarepusher chip.
 *
 * Copyright 1999, Be Incorporated.   All Rights Reserved.
 * This file may be used under the terms of the Be Sample Code License.
 */

#include <OS.h>
#include <KernelExport.h>
#include "xyz5038.h"

// since this is not a real CPU, we'll emulate the registers
static int32 foo = 0;
static int32 bar = 0;

// locking for the fabricated race condition in write_*().
// if this were real hardware, you'd probably be disabling
// interrupts and acquiring a spinlock here.
static int32 lock_count = 0;
static sem_id lock_sem = -1;

static void
lock()
{
	if (atomic_add(&lock_count, 1) > 0)
		acquire_sem(lock_sem);
}

static void
unlock()
{
	if (atomic_add(&lock_count, -1) > 1)
		release_sem(lock_sem);
}

static status_t
std_ops(int32 op, ...)
{
	dprintf("std_ops 0x%x\n", op);
	
	switch(op) {
	case B_MODULE_INIT:
		// fail if we can't create a lock
		lock_sem = create_sem(0, "XYZ5038 lock");
		if (lock_sem < B_OK)
			return lock_sem;
		return B_OK;
	case B_MODULE_UNINIT:
		delete_sem(lock_sem);
		return B_OK;
	default:
		return B_ERROR;
	}
}

static int32
read_foo()
{
	return foo;
}

static int32
read_bar()
{
	return bar;
}

static int32
write_foo(int32 new_value)
{
	int32 old_value;

	lock();
	old_value = read_foo();
	foo = new_value;
	unlock();

	return old_value;
}

static int32
write_bar(int32 new_value)
{
	int32 old_value;

	lock();
	old_value = read_bar();
	bar = new_value;
	unlock();

	return old_value;
}

static xyz5038_module_info xyz5038_module = 
{
	// the module_info struct
	{
		XYZ5038_MODULE_NAME,
		// if your module can't be unloaded for some reason,
		// set this to be B_KEEP_LOADED instead of zero.
		0,
		std_ops
	},
	read_foo,
	read_bar,
	write_foo,
	write_bar
};

_EXPORT xyz5038_module_info *modules[] =
{
	&xyz5038_module,
	NULL
};

