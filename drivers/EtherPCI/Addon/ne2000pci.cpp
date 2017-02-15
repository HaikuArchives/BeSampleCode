/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include "generic.h"

static const char TITLE[] = "Novell NE2000 compatible adapter (PCI) Sample";
static const char CONFIG[] = "ne2000pci";
static const char LINK[] = "/dev/net/etherpci";


extern "C" _EXPORT BNetDevice *
open_device(const char *device)
{
	GenericDevice *dev;

	dev = new GenericDevice();
	if (dev->Start(device) < B_NO_ERROR) {
		delete dev;
		return (NULL);
	}
	return (dev);
}

extern "C" _EXPORT BNetConfig *
open_config(const char *device)
{
	return (new GenericConfig(TITLE, CONFIG, LINK));
}

