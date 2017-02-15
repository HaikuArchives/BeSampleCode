/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/
/*
 * Common methods for ethernet drivers
 */
#include <EtherDevice.h>
#include <netdebug.h>
#include <string.h>
#include <errno.h>
#include <new>
#include <BeBuild.h>	// for _PR2_COMPATIBLE_

#if _PR2_COMPATIBLE_
int compat_open(const char *dev);
#else /* _PR2_COMPATIBLE_ */
#define compat_open(dev) open(dev, O_RDWR, 0)
#endif /* _PR2_COMPATIBLE_ */

int
EtherDevice::Init(
				  const char *devicelink,
				  char *params
				  )
{
	fd = compat_open(devicelink);
	if (fd < 0) {
		return (-1);
	}
	if (ioctl(fd, ETHER_INIT, params, 0) < 0) {
		dprintf("ether_init failed\n");
		close(fd);
		fd = -1;
		return (-1);
	}
	return (fd);
}

static int
toss(int fd)
{
	char junk[1536];

	return (read(fd, junk, sizeof(junk)));
}


BNetPacket *
EtherDevice::read_packet(void)
{
	BNetPacket *packet;
	int len;

	packet = NULL;
	try {
		/*
		 * Normal case: assume we don't run out of memory
		 */
		packet = AllocPacket();
		packet->SetSize(MaxPacketSize());
		len = read(fd, packet->Data(), packet->Size());
		if (len <= 0) {
			delete packet;
			return (NULL);
		}
		packet->SetSize(len);
	} catch(bad_alloc) {
		if (!nonblock) {
			/*
			 * If we are blocking, must do a blocking read
			 */
			if (toss(fd) <= 0) {
				return (NULL);
			}
		}
		if (packet != NULL) {
			/*
			 * A packet was allocated, but SetSize failed
			 */
			delete packet;
		}
		return (NULL);
	}
	return (packet);
}

/*
 * Here we attempt to read as many packets as we can without
 * blocking from the driver
 */
void
EtherDevice::fill_packetlist(void)
{
	BNetPacket *packet;
	int len;

	/*
	 * Turn on nonblocking
	 */
	nonblock = 1;
	ioctl(fd, ETHER_NONBLOCK, &nonblock, sizeof(nonblock));


	/*
	 * And keep reading packets until we would block (but don't
	 * actually block). 
	 */
	for (;;) {
		packet = read_packet();
		if (packet == NULL) {
			return; /* no more packets or out of memory */
		}

		try {
			packetlist.ChainToEnd(packet);
		} catch (bad_alloc) {
			delete packet;
			return; /* out of memory: stop reading */
		}
	}
}

BNetPacket *
EtherDevice::ReceivePacket(void)
{
	BNetPacket *packet;
	int len;

	packet = packetlist.RemoveHead();
	if (packet) {
		return (packet);
	}
	if (nonblock) {
		nonblock = 0;
		ioctl(fd, ETHER_NONBLOCK, &nonblock, sizeof(nonblock));
	}

	/*
	 * Wait for a packet
	 */
	do {
		packet = read_packet(); /* NULL if out of memory */
	} while (packet == NULL); 


	/*
	 * Got a packet, now check for more
	 */
	fill_packetlist();

	return (packet);
}

#if _PR2_COMPATIBLE_

int
compat_open(const char *dev)
{
	if (strcmp(dev, "/dev/tulip") == 0) {
		return (open("/dev/net/tulip", O_RDWR, 0));
	}
	if (strcmp(dev, "/dev/gce") == 0) {
		return (open("/dev/net/gce", O_RDWR, 0));
	}
	if (strcmp(dev, "/dev/ether") == 0) {
		return (open("/dev/net/ether", O_RDWR, 0));
	}
	if (strcmp(dev, "/dev/net/eepro100") == 0) {
		return (open("/dev/net/eepro100/0", O_RDWR, 0));
	}
	return (open(dev, O_RDWR, 0));
}

#endif /* _PR2_COMPATIBLE_ */
