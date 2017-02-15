/* 
** devlistmgr.c
**
** Code to handle a dynamic list of device names for devices that publish a
** potentially unbounded list of devices (scsi_dsk, scsi_cd, scsi_raw, etc)
**
** 02/07/99 - swetland
**
*/
 
/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/
#include <stdio.h>
#include <malloc.h>


typedef struct _devname
{
	struct _devname *next;
	char name[1];
} devname;

static int devcount = 0;
static char **devnames = NULL;
static devname *devlist = NULL;

static void free_devnames(void)
{
	devname *d,*n;
	if(devnames) free(devnames);
	devnames = NULL;
	for(d = devlist; d; d=n){
		n = d->next;
		free(d);
	}
	devlist = NULL;
	devcount = 0;
}

static void add_devname(char *name)
{
	devname *d = (devname *) malloc(strlen(name) + sizeof(devname));
	if(d){
		strcpy(d->name,name);
		devcount++;
		d->next = devlist;
		devlist = d;
	}
}

static const char **get_devnames(void)
{
	if(devcount && !devnames){
		if((devnames = (char **) malloc(sizeof(char*) * (devcount+1)))){
			int i;
			devname *d;
			for(d=devlist,i=0;i<devcount;i++){
				devnames[i] = d->name;
				d = d->next;
			}
			devnames[i] = NULL;
		}
	}
	return (const char **) devnames;
}
