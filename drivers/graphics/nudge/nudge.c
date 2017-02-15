/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

int main(int argc, char **argv) {

	int fd;
	ssize_t wrote;

	if (argc != 2) {
		fprintf(stderr, "USAGE: %s driver_file_name (no path)\n", argv[0]);
		return -1;
	}
	fd = open("/dev", O_WRONLY);
	if (fd < 0) {
		fprintf(stderr, "Couldn't open /dev: %s (0x%08x)\n", strerror(errno), errno);
		return -1;
	}	
	wrote = write(fd, argv[1], strlen(argv[1]));
	if (wrote != strlen(argv[1]))
		fprintf(stderr, "Couldn't write name to /dev: %s (0x%08x)\n", strerror(errno), errno);	
	close(fd);
	return wrote;
}
