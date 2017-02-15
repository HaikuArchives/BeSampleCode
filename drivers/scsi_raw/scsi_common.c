/* scsi_common.c
** 
** Shared scsi utils.  
**
** 02/07/99 - swetland
*/

/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include <stdio.h>
#include <string.h>

#include <SupportDefs.h>
#include <OS.h>
#include <CAM.h>
#include <Drivers.h>
#include <KernelExport.h>
#include <scsi.h>


/* used by scsi_dsk too, cannot be static */

int decode_scsi_ptl(char *name, int *path, int *targ, int *lun)
{
	*path = 0;
	*targ = 0;
	*lun = 0;

	while(isdigit(*name)) {
		*path = (*path) * 10 + (*name - '0');
		name++;
	}
	if(*name != '/') return 1;
	name++;
		
	while(isdigit(*name)) {
		*targ = (*targ) * 10 + (*name - '0');
		name++;
	}
	if(*name != '/') return 1;
	name++;

	while(isdigit(*name)) {
		*lun = (*lun) * 10 + (*name - '0');
		name++;
	}
	if(*name != '/') return 1;

	return 0;
}

 

#define LOCKED_DATA   0x01
#define LOCKED_SENSE  0x02

status_t 
do_raw_command(cam_for_driver_module_info *cam,
			   raw_device_command *rdc, uint path, uint targ, uint lun)
{
	CCB_HEADER *ccbh = NULL;
	CCB_SCSIIO *ccb;
	status_t result = B_ERROR;
	int mem_locked = 0;
	
	/* sanity check the command length */
	if((rdc->command_length != 6) &&
	   (rdc->command_length != 10) &&
	   (rdc->command_length != 12)) {
	   	dprintf("raw: bad command_length (%d)\n",rdc->command_length);
	   	goto drc_exit;
	}

	/* make sure data is there and lockable */
	if(rdc->data_length) {
		if(lock_memory(rdc->data, rdc->data_length, B_DMA_IO | B_READ_DEVICE)) {
			dprintf("raw: cannot lock data 0x%08x/%d\n",
					rdc->data,rdc->data_length);
			goto drc_exit;
		} else {
			mem_locked |= LOCKED_DATA;
		}
	}

	/* lock sense data if we've got it */
	if(rdc->sense_data_length){
		if(lock_memory(rdc->sense_data, rdc->sense_data_length, 
						B_DMA_IO | B_READ_DEVICE)) {
			dprintf("raw: cannot lock sense data 0x%08x/%d\n",
					rdc->sense_data,rdc->sense_data_length);
			goto drc_exit;
		} else {
			mem_locked |= LOCKED_SENSE;
		}
	}

	if(!(ccbh = (*cam->xpt_ccb_alloc)())) {
		goto drc_exit;
	}
	
	ccbh->cam_path_id = path;
	ccbh->cam_target_id = targ;
	ccbh->cam_target_lun = lun;
	ccbh->cam_flags = ((rdc->flags & B_RAW_DEVICE_DATA_IN) ? CAM_DIR_IN : CAM_DIR_OUT );

	ccb = (CCB_SCSIIO *) ccbh;
	ccb->cam_dxfer_len = rdc->data_length;
	ccb->cam_data_ptr = (uchar *) rdc->data;
	ccb->cam_sense_len = rdc->sense_data_length;
	ccb->cam_sense_ptr = rdc->sense_data;
	ccb->cam_cdb_len = rdc->command_length;
	memcpy(ccb->cam_cdb_io.cam_cdb_bytes, rdc->command, rdc->command_length);
		
	/* make it so */
	(*cam->xpt_action) (ccbh);

	rdc->scsi_status = ccb->cam_scsi_status;
	rdc->cam_status = ccb->cam_ch.cam_status;

	result = B_OK;
	
drc_exit:
	if((result == B_OK) && (rdc->flags & B_RAW_DEVICE_REPORT_RESIDUAL)){
		rdc->sense_data_length = rdc->sense_data_length - ccb->cam_sense_resid;
		rdc->data_length = rdc->data_length - ccb->cam_resid;
	}	
	if(ccbh) {
		(*cam->xpt_ccb_free)(ccbh);
	}
	if(mem_locked & LOCKED_DATA) {
		unlock_memory(rdc->data, rdc->data_length, B_DMA_IO|B_READ_DEVICE);
	}
	if(mem_locked & LOCKED_SENSE) {
		unlock_memory(rdc->sense_data, rdc->sense_data_length, 
						  B_DMA_IO|B_READ_DEVICE);
	}
	return result;
}
 

