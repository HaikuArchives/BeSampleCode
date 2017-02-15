/*******************************************************************************
/
/	File:			testdigit.c
/
/   Description:	A simple application that tests the digit driver.
/
/	Copyright 1999, Be Incorporated.   All Rights Reserved.
/	This file may be used under the terms of the Be Sample Code License.
/
*******************************************************************************/

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <support/SupportDefs.h>

static void dump(uchar *b, int c);

int main()
{
	int fd;
	status_t err;
	uchar buffer[128], c;

	fd = open("/dev/misc/digit", O_RDWR);
	if (fd < 0) {
		printf("Error opening device\n");
		return 1;
	}

	err = read_pos(fd, 0LL, buffer, sizeof(buffer));
	if (err < 0)
		printf("Error reading device (%s)\n", strerror(err));
	else {
		printf("Read buffer 1:\n");
		dump(buffer, err);
	}

	c = 1;
	printf("Setting digit to %2.2x\n", c);
	err = ioctl(fd, 'set ', &c, sizeof(c));
	if (err < 0)
		printf("Error setting digit\n");

	err = read_pos(fd, 0LL, buffer, sizeof(buffer));
	if (err < 0)
		printf("Error reading device (%s)\n", strerror(err));
	else {
		printf("Read buffer 1:\n");
		dump(buffer, err);
	}

	printf("Setting digit to %2.2x\n", '9');
	err = write(fd, "123456789", 9);
	if (err < 0)
		printf("Error setting digit\n");

	err = read_pos(fd, 0LL, buffer, sizeof(buffer));
	if (err < 0)
		printf("Error reading device (%s)\n", strerror(err));
	else {
		printf("Read buffer 1:\n");
		dump(buffer, err);
	}

	close(fd);

	return 0;
}

static void dump(uchar *b, int c)
{
	int i;
	for (i=0;i<c;i++) {
		printf("%2.2x ", *(b++));
		if ((i & 15) == 15)
			printf("\n");
	}
	printf("\n");
}
