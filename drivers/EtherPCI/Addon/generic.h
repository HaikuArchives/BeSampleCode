/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include "netconfig.h"
#include <stdio.h>
#include <stdlib.h>
#include <socket.h>
#include <netdb.h>
#include "ether_driver.h"
#include <fcntl.h>
#include "netdebug.h"
#include <string.h>
#include <unistd.h>

#include <Box.h>
#include <Button.h>
#include "EtherDevice.h"
#include <Handler.h>
#include <MessageFilter.h>
#include <NetDevice.h>
#include <Screen.h>
#include <StringView.h>
#include <Window.h>

class GenericDevice : public EtherDevice {
public:
	int Start(const char *device);
};

class GenericConfig : public BNetConfig {
public:
	GenericConfig(const char *title, const char *config, const char *link) {
		this->title = title;
		this->config = config;
		this->link = link;
	}
	int GetPrettyName(char *pretty_name, int len) {
		int totlen = strlen(title) + 1;
		if (len < totlen) {
			return (-totlen);
		}
		strcpy(pretty_name, title);
		return (totlen);
	}
	bool IsIpDevice(void) { return (0); }
	status_t Config(const char *device, net_settings *ncw,
					BCallbackHandler *dallback, bool autoconf);

private:
	const char *title;
	const char *config;
	const char *link;
};

