/*
**  scsi_raw.c
**
**	The SCSI raw interface driver.
*/
/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/
 
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <string.h>

#include <SupportDefs.h>
#include <OS.h>
#include <CAM.h>
#include "Drivers.h"
#include <KernelExport.h>
#include "scsi.h"
#include <Errors.h>

#define ddprintf

static cam_for_driver_module_info	*cam;
static const char cam_name[] = B_CAM_FOR_DRIVER_MODULE_NAME;
			
typedef struct
{
	int path;
	int target;
	int lun;
}  raw_scsi;

int decode_scsi_ptl(char *name, int *path, int *targ, int *lun);


static status_t
scsi_raw_open (const char *name, uint32 flags, void **c)
{
	raw_scsi **cookie = (raw_scsi **) c;
	raw_scsi *d = NULL;     /* -> sci info */
	int	retval = -1;        /* assume failure */
	
	/* set up private data for this particular device */
	if (!(d = (raw_scsi *) malloc (sizeof(raw_scsi)))) {
		goto exit;
	}

	/* bus/scsi/ is 9 characters... */
	if(decode_scsi_ptl((char *) (name + 9), &(d->path), &(d->target), &(d->lun))) {
		goto exit;
	}
	retval = 0;
	*cookie = d;

exit:
	if (d && retval)
		free (d);

	return retval;
}


static status_t
scsi_raw_close (void *d)
{
	return 0;
}


static status_t
scsi_raw_free (void *d)
{
	free(d);
	return 0;
}


status_t do_raw_command(cam_for_driver_module_info *cam,
						raw_device_command *rdc, 
						uint path, uint targ, uint lun);

/*
**	scsi_disk_control handles control messages from the driver subsystem.
*/
static status_t
scsi_raw_control(void *d_var, uint32 msg, void *s1, size_t len)
{
	raw_scsi *d = (raw_scsi *) d_var;
	raw_device_command *rdc = (raw_device_command *) s1;
	
	if(msg != B_RAW_DEVICE_COMMAND){
		return B_DEV_INVALID_IOCTL;
	}

	return do_raw_command(cam, rdc, d->path, d->target, d->lun);
}

static status_t
scsi_raw_read (void *d_var, off_t pos, void *buf, size_t *len)
{
	return B_ERROR;	
}


static status_t
scsi_raw_write (void *d_var, off_t pos, const void *buf, size_t *len)
{
	return B_ERROR;
}


static device_hooks scsi_raw_device = {
	scsi_raw_open,					/* -> open entry */
	scsi_raw_close,					/* -> close entry */
	scsi_raw_free,					/* -> free cookie */
	scsi_raw_control,				/* -> control entry */
	scsi_raw_read,					/* -> read entry */
	scsi_raw_write					/* -> write entry */
};

#define	MAX_DEV				64

static int
get_max_path (CCB_HEADER *ccbh)
{
	uchar max_path;

	ccbh->cam_func_code = XPT_PATH_INQ;
	ccbh->cam_path_id = 0xFF;	/* return only highest path id */
	(*cam->xpt_action) (ccbh);		/* do path inquiry */
	if (ccbh->cam_status != CAM_REQ_CMP)
		return -1;
	max_path = ((CCB_PATHINQ *)ccbh)->cam_hpath_id;
	return (max_path == 0xFF) ? -1 : max_path;
}

_EXPORT
status_t 
init_hardware(void)
{
	return B_OK;
}

_EXPORT
status_t
init_driver()
{
	if(get_module(cam_name, (module_info **)&cam) != B_OK)
		return B_ERROR;

	return B_NO_ERROR;
}

#include "devlistmgr.c"


_EXPORT
void
uninit_driver()
{
	free_devnames();
	put_module(cam_name);
}

_EXPORT
const char **
publish_devices()
{
	char		tmp[64];
	CCB_HEADER	*ccbh;				/* -> ccb */
	int			path, targ, lun;	/* path, target, lun ids */
	int			max_path;			/* highest known path id */
	
	free_devnames();
	cam->minfo.rescan();
	
	if (!(ccbh = (*cam->xpt_ccb_alloc)())){
		return NULL;
	}

	if ((max_path = get_max_path (ccbh)) < 0) { /* get highest path id */
		(*cam->xpt_ccb_free) (ccbh);
		return NULL;
	}


	ccbh->cam_func_code = XPT_GDEV_TYPE;
    ((CCB_GETDEV *)ccbh)->cam_inq_data = NULL;

	/* for each path id, look for disks */
	for (path = 0; path <= max_path ; path++) {
		ccbh->cam_path_id = path;
		for (targ = 0; targ < 16; targ++) {
			ccbh->cam_target_id = targ;
			for (lun = 0; lun < 8; lun++) {
				ccbh->cam_target_lun = lun;
				(*cam->xpt_action)(ccbh);	/* do get_device_type */
				if(ccbh->cam_status != CAM_REQ_CMP) continue;
				sprintf(tmp, "bus/scsi/%d/%d/%d/raw", path, targ, lun);
				add_devname(tmp);
			}
		}
	}

	(*cam->xpt_ccb_free) (ccbh);

	return get_devnames();
}

_EXPORT device_hooks *
find_device(const char *name)
{
	return &scsi_raw_device;
}

_EXPORT int32 api_version = B_CUR_DRIVER_API_VERSION;
