/* inquiry.cpp - SCSIProbe for the commandline, more or less */
/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <Directory.h>
#include <Entry.h>
#include <Path.h>
#include <scsi.h>
#include <CAM.h>

/* table with textual descriptions of the inquiry data's device type */
char *devtype[] = { 
	"Disk   ", "Tape   ", "Printer", "CPU    ", "WORM   ", 
	"CD-ROM ", "Scanner", "Optical", "Changer", "Comm   ", "Unknown"
};

/* open a raw device, issue an inquiry command, print the results
** (an a header if it's the first time).
*/
void inquiry(const char *dev)
{
	static int header = 1;
	int fd,e;
	int path,targ,lun,type;
	raw_device_command rdc;
	uchar data[36], sense[16];

	if(!strncmp("/dev/bus/scsi/",dev,14)){
		sscanf(dev,"/dev/bus/scsi/%d/%d/%d/raw",&path,&targ,&lun);
	} else {
		path = targ = lun = 0;
	}

	/* fill out our raw device command data */
	rdc.data = data;
	rdc.data_length = 36;
	rdc.sense_data = sense;
	rdc.sense_data_length = 0;
	rdc.timeout = 1000000;
	rdc.flags = B_RAW_DEVICE_DATA_IN;
	rdc.command_length = 6;
	rdc.command[0] = 0x12;
	rdc.command[1] = 0x00;
	rdc.command[2] = 0x00;	
	rdc.command[3] = 0x00;	
	rdc.command[4] = 36;
	rdc.command[5] = 0x00;	

	if((fd = open(dev,0)) < 0) return;
	e = ioctl(fd, B_RAW_DEVICE_COMMAND, &rdc, sizeof(rdc));
	close(fd);
	if((e != 0) || (rdc.cam_status != CAM_REQ_CMP)) return;

	if(header){
		printf("Bus ID  LUN  Type    Vendor   Device           Rev \n"
			   "--- --- ---  ------- -------- ---------------- ----\n");
		header = 0;
	}
	
	type = data[0] & 0x1F;
	if(type > 9) type = 10;
	printf("%3d %3d %3d  %7s %8.8s %16.16s %4.4s\n", path, targ, lun, 
		devtype[type], data + 8, data + 16, data + 32);
}

/* recursively wander down from a path, looking for "raw" devices to call
** inquiry on.
*/
void walkpath(const char *path)
{
	BDirectory dir(path);
	if(dir.InitCheck() == B_OK){
		BEntry entry;
		dir.Rewind();
		while(dir.GetNextEntry(&entry) >= 0) {
			BPath name;
			entry.GetPath(&name);
			if(entry.IsDirectory()) {
				walkpath(name.Path());
			} else if(!strcmp(name.Leaf(),"raw")){
				inquiry(name.Path());
			}
		}
	}			
}

/* handle a commandline arg or just walk the directory tree looking for devices */
int main(int argc, char *argv[])
{
	if(argc != 2) {
		walkpath("/dev/bus/scsi");
	} else {
		inquiry(argv[1]);
	}
	return 0;
}
