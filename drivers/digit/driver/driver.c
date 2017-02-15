/*******************************************************************************
/
/	File:			driver.c
/
/   Description:	The digit driver allows a client to read 16 bytes of data.
/                   Each byte has the same value (by default 0); the client
/                   can change this value with write or ioctl commands.
/
/	Copyright 1999, Be Incorporated.   All Rights Reserved.
/	This file may be used under the terms of the Be Sample Code License.
/
*******************************************************************************/

#include <drivers/Drivers.h>
#include <drivers/KernelExport.h>

#include <malloc.h>
#include <string.h>

#define DPRINTF(a) dprintf("digit: "); dprintf a

#define MAX_SIZE 16
#define DEVICE_NAME "misc/digit"

#define TOUCH(x) ((void)x)

/*********************************/

struct digit_state {
	uchar value;
};

static status_t
digit_open(const char *name, uint32 mode, void **cookie)
{
	struct digit_state *s;

	TOUCH(name); TOUCH(mode);

	DPRINTF(("open\n"));

	s = (struct digit_state*) malloc(sizeof(*s));
	if (!s)
		return B_NO_MEMORY;

	s->value = 0;

	*cookie = s;

	return B_OK;
}

static status_t
digit_close(void *cookie)
{
	TOUCH(cookie);

	DPRINTF(("close\n"));

	return B_OK;
}

static status_t
digit_free(void *cookie)
{
	DPRINTF(("free\n"));

	free(cookie);

	return B_OK;
}

static status_t
digit_ioctl(void *cookie, uint32 op, void *data, size_t len)
{
	TOUCH(len);

	DPRINTF(("ioctl\n"));

	if (op == 'set ') {
		struct digit_state *s = (struct digit_state *)cookie;
		s->value = *(uchar *)data;
		DPRINTF(("Set digit to %2.2x\n", s->value));
		return B_OK;
	}

	return ENOSYS;
}

static status_t
digit_read(void *cookie, off_t pos, void *buffer, size_t *len)
{
	struct digit_state *s = (struct digit_state *)cookie;

	DPRINTF(("read\n"));

	if (pos >= MAX_SIZE)
		*len = 0;
	else if (pos + *len > MAX_SIZE)
		*len = (size_t)(MAX_SIZE - pos);
	if (*len)
		memset(buffer, s->value, *len);
	return B_OK;
}

static status_t
digit_write(void *cookie, off_t pos, const void *buffer, size_t *len)
{
	struct digit_state *s = (struct digit_state *)cookie;

	TOUCH(pos);

	s->value = ((uchar *)buffer)[*len - 1];

	DPRINTF(("write: set digit to %2.2x\n", s->value));

	return B_OK;
}

/***************************/

status_t init_hardware()
{
	DPRINTF(("Do you digit?\n"));
	return B_OK;
}

const char **publish_devices()
{
	static const char *devices[] = {
		DEVICE_NAME, NULL
	};

	return devices;
}

device_hooks *find_device(const char *name)
{
	static device_hooks hooks = {
		&digit_open,
		&digit_close,
		&digit_free,
		&digit_ioctl,
		&digit_read,
		&digit_write,
		/* Leave select/deselect/readv/writev undefined. The kernel will
		 * use its own default implementation. The basic hooks above this
		 * line MUST be defined, however. */
		NULL,
		NULL,
		NULL,
		NULL
	};

	if (!strcmp(name, DEVICE_NAME))
		return &hooks;

	return NULL;
}

status_t init_driver()
{
	return B_OK;
}

void uninit_driver()
{
}
