/* skel.c
** 
** Skeleton SCSI Interface Module / BeOS R4.5
**
** Does not support the Frobozz Magic SCSI Controller (FMSC)
*/

/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include <BeBuild.h>
#include <iovec.h>
#include <ByteOrder.h>

#include <OS.h>
#include <KernelExport.h>
#include <PCI.h>
#include <CAM.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

typedef struct
{
	/* private data storage in the CCB */
	
/* SIM_PRIV (sim private data area)  Terms and Conditions:

 - the size of SIM_PRIV shall be such that sizeof(CCB_SIZE_UNION) = 1.5k
 - all CCB's shall be allocated from locked, contiguous memory
 - CCB's shall be aligned on 512 byte boundaries
 - SIM_PRIV will be >= 1408 bytes 
 - this provides 128  8byte sg entries (512mb worth of pages, worstcase fragmentation)
 - and 384 bytes for whatever else the SIM needs

 - These conditions are NULL and (void) where prohibited by law.
 - All sales are final.
 - Do not use near open flame.
*/	

	/* this "dummy" data member exists solely to pacify the compiler; empty
	* structs evoke an error message from mwcc.  Replace it with your real
	* private data.
	*/
	int dummy;

} FMSC_Priv;

typedef struct
{
	int num;
	
	/* information about an single instance of a controller goes here */
} FMSC;

#define PCI_VENDOR_FROBOZZCO 0x1234
#define PCI_DEVICE_FMSC 0x1234

/*
** Constants for the SIM 
*/
#define SIM_VERSION 0x01
#define HBA_VERSION 0x01

static char sim_vendor_name[]   = "FrobozzCo";        /* who wrote this driver */
static char hba_vendor_name[]   = "FrobozzCo";        /* who made the hardware */
static char controller_family[] = "Magic SCSI";       /* what family of products */

static pci_module_info *pci;
static cam_for_sim_module_info *cam;

static char	cam_name[] = B_CAM_FOR_SIM_MODULE_NAME;
static char	pci_name[] = B_PCI_MODULE_NAME;

static long init_fmsc(FMSC *fmsc)
{
	dprintf("fmsc%d: initializing hardware\n", fmsc->num);

	/* 
	** initialize the hardware -- return B_OK if all is well,
	** B_ERROR if things do not proceed according to plan
	**
	** Called only once per controller by the SCSI Bus Manager 
	*/
	
	return B_OK;
}


static long sim_execute_scsi_io(FMSC *fmsc, CCB_HEADER *ccbh)
{
    CCB_SCSIIO *ccb = (CCB_SCSIIO *) ccbh;
	uchar *cdb;
    int cdb_len;
	
	FMSC_Priv *priv;	
    uint32 priv_phys;
	
    physical_entry entries[2];

    cdb_len = ccb->cam_cdb_len;
    if(ccb->cam_ch.cam_flags & CAM_CDB_POINTER) {
		cdb = ccb->cam_cdb_io.cam_cdb_ptr;
    } else {
		cdb = ccb->cam_cdb_io.cam_cdb_bytes;
    }

		/* Get physical memory address of the "private area" */		
    get_memory_map((void *)ccb->cam_sim_priv, 4096, entries, 2);
    
	priv_phys = (uint32) entries[0].address;
	priv = (FMSC_Priv *) ccb->cam_sim_priv;

	/*
	** ccb->cam_ch.cam_target_id  -- SCSI Target ID
	** ccb->cam_ch.cam_target_lun -- SCSI Target Logical Unit Number
	** cdb                        -- SCSI Command Data Block
	** cdb_len                    -- SCSI Command Data Block Length
	** priv                       -- per-transaction private scratchpad
	** priv_phys                  -- physical (linear) address of the same
	** ccb->cam_dxfer_len         -- total bytes to transfer
	*/
	
	if(ccb->cam_ch.cam_flags & CAM_SCATTER_VALID){
		/* we're using scatter gather -- things just got trickier */
		iovec *iov = (iovec *) ccb->cam_data_ptr;
		
		/*
		** ccb->cam_sglist_cnt -- number of entries in the scatter/gather table
		** iov                 -- scatter/gather table: pointer/length pairs
		*/
	} else {	
		/*
		** ccb->cam_data_ptr   -- pointer to data buffer
		** ccb->cam_dxfer_len  -- total bytes to transfer
		*/
	}
	
	
	/* 
	** Execute IO and wait for completion or failure 
	*/
	
	if( 1 ) {
		/* transaction failed -- return an appropriate error code (see CAM.h) */
		ccb->cam_ch.cam_status = CAM_SEL_TIMEOUT;
		return B_ERROR;
		
	} else {
		/* transaction succeeded */
		
		ccb->cam_resid = 0;        /* report bytes not transferred */
		ccb->cam_scsi_status = 0;  /* report scsi status byte */

		/* note if a SCSI error occurred */
		if(ccb->cam_scsi_status) {
			ccb->cam_ch.cam_status = CAM_REQ_CMP_ERR;
		} else {
			ccb->cam_ch.cam_status = CAM_REQ_CMP;
		}
		
		if((ccb->cam_scsi_status == 2) && 
		   !(ccb->cam_ch.cam_flags & CAM_DIS_AUTOSENSE)){
			/* CHECK CONDITION returned and auto sense is desired */
			
			/*
			** Issue a Request Sense IO and report the results:
			** ccb->cam_sense_ptr -- Sense Data Block
			** ccb->cam_sense_len -- Size of Sense Data Block
			*/
			
			if( 0 ) {
				/* request sense succeeded */
				ccb->cam_ch.cam_status |= CAM_AUTOSNS_VALID;
				ccb->cam_sense_resid = 0; /* sense data not transferred */	
			} else {
				ccb->cam_ch.cam_status = CAM_AUTOSENSE_FAIL;
			}
		}
		return B_OK;
	}    
}


/* 
** sim_path_inquiry returns info on the target/lun.
*/
static long sim_path_inquiry(FMSC *fmsc, CCB_HEADER *ccbh)
{
    CCB_PATHINQ	*ccb;
	dprintf("fmsc%d: path inquiry\n",fmsc->num);
    
    ccb = (CCB_PATHINQ *) ccbh;
    
    ccb->cam_version_num = SIM_VERSION;
    ccb->cam_target_sprt = 0;
    ccb->cam_hba_eng_cnt = 0;
    memset (ccb->cam_vuhba_flags, 0, VUHBA);
    ccb->cam_sim_priv = SIM_PRIV;
    ccb->cam_async_flags = 0;
    ccb->cam_initiator_id = 7; /* controller SCSI ID */
	ccb->cam_hba_inquiry = 0; /* PI_WIDE_16 if Wide SCSI */
    strncpy (ccb->cam_sim_vid, sim_vendor_name, SIM_ID);
    strncpy (ccb->cam_hba_vid, hba_vendor_name, HBA_ID);
    ccb->cam_osd_usage = 0;
    ccbh->cam_status = CAM_REQ_CMP;
    return B_OK;
}

/*
** sim_extended_path_inquiry returns info on the target/lun.
*/
static long sim_extended_path_inquiry(FMSC *fmsc, CCB_HEADER *ccbh)
{
    CCB_EXTENDED_PATHINQ *ccb;
    
    sim_path_inquiry(fmsc, ccbh);
    ccb = (CCB_EXTENDED_PATHINQ *) ccbh;
    sprintf(ccb->cam_sim_version, "%d.0", SIM_VERSION);
    sprintf(ccb->cam_hba_version, "%d.0", HBA_VERSION);
    strncpy(ccb->cam_controller_family, controller_family, FAM_ID);
    strncpy(ccb->cam_controller_type, "FMSC1000", TYPE_ID);
    return B_OK;
}

/*
** scsi_sim_action performes the scsi i/o command embedded in the
** passed ccb.
**
** The target/lun ids are assumed to be in range.
*/
static long sim_action(FMSC *fmsc, CCB_HEADER *ccbh)
{
	ccbh->cam_status = CAM_REQ_INPROG;
	switch(ccbh->cam_func_code){
	case XPT_SCSI_IO:
		return sim_execute_scsi_io(fmsc,ccbh);
	case XPT_PATH_INQ:
		return sim_path_inquiry(fmsc,ccbh);
	case XPT_EXTENDED_PATH_INQ:
		return sim_extended_path_inquiry(fmsc, ccbh);
	default:
		ccbh->cam_status = CAM_REQ_INVALID;
		return B_ERROR;
	}
}

/*
** Allocate the actual memory for the cardinfo object 
*/
static FMSC *create_cardinfo(int num, int iobase, int irq)
{
	FMSC *fmsc;
	
	if(!(fmsc = (FMSC *) malloc(sizeof(FMSC)))) return NULL;

	fmsc->num = num;
	
	/* note any other settings, allocate resources, etc */
		
    return fmsc;
}


/*
** Multiple Card Cruft
*/
#define MAXCARDS 4

static FMSC *cardinfo[MAXCARDS] = { NULL, NULL, NULL, NULL };

static long sim_init0(void)                { return init_fmsc(cardinfo[0]); }
static long sim_init1(void)                { return init_fmsc(cardinfo[1]); }
static long sim_init2(void)                { return init_fmsc(cardinfo[2]); }
static long sim_init3(void)                { return init_fmsc(cardinfo[3]); }
static long sim_action0(CCB_HEADER *ccbh)  { return sim_action(cardinfo[0],ccbh); }
static long sim_action1(CCB_HEADER *ccbh)  { return sim_action(cardinfo[1],ccbh); }
static long sim_action2(CCB_HEADER *ccbh)  { return sim_action(cardinfo[2],ccbh); }
static long sim_action3(CCB_HEADER *ccbh)  { return sim_action(cardinfo[3],ccbh); }

static long (*sim_init_funcs[MAXCARDS])(void) = {
    sim_init0, sim_init1, sim_init2, sim_init3 
};

static long (*sim_action_funcs[MAXCARDS])(CCB_HEADER *) = {
    sim_action0, sim_action1, sim_action2, sim_action3
};


/*
** Detect the controller and register the SIM with the CAM layer.
** returns the number of controllers installed...
*/
int sim_install_fmsc(void)
{
    int i, iobase, irq;
    int cardcount = 0;
    pci_info h;
    CAM_SIM_ENTRY entry;

	dprintf("fmsc: looking for controllers on the PCI bus\n");
	    
    for(i = 0; (*pci->get_nth_pci_info)(i, &h) == B_OK; i++) {
	
		/* detect compatible cards that we wish to support */
        if((h.vendor_id == PCI_VENDOR_FROBOZZCO) &&
		    (h.device_id == PCI_DEVICE_FMSC)) {

            iobase = h.u.h0.base_registers[0];
            irq = h.u.h0.interrupt_line;

			dprintf("fmsc%d: controller @ 0x%08x, irq %d\n", cardcount, iobase, irq);
			
            if(cardcount == MAXCARDS){
                dprintf("fmsc: too many controllers!\n");
				break;
            }

            if((cardinfo[cardcount] = create_cardinfo(cardcount,iobase,irq))){
				/* if this worked, register us with the SCSI Bus Manager */
                entry.sim_init = sim_init_funcs[cardcount];
                entry.sim_action = sim_action_funcs[cardcount];
                (*cam->xpt_bus_register)(&entry);
                cardcount++;
            } else {
                dprintf("fmsc%d: cannot allocate cardinfo\n",cardcount);
            }
        }
    }

    return cardcount;
}

/*
** Module Startup/Teardown
** - get the SCSI Bus Manager Module and PCI Bus Manager Module
** - put them when we're finished
*/
static status_t std_ops(int32 op, ...)
{
	switch(op) {
	case B_MODULE_INIT:
		if (get_module(pci_name, (module_info **) &pci) != B_OK)
			return B_ERROR;

		if (get_module(cam_name, (module_info **) &cam) != B_OK) {
			put_module(pci_name);
			return B_ERROR;
		}

		/* if there are more than 0 controllers detected, we succeeded */
		if(sim_install_fmsc()) return B_OK;

		/* otherwise, we failed -- fall through */
	case B_MODULE_UNINIT:
		put_module(pci_name);
		put_module(cam_name);
		return B_OK;
	
	default:
		return B_ERROR;
	}
}


/*
** Declare our module_info so we can be loaded as a kernel module
*/

static
sim_module_info sim_fmsc_module = {
	{ "busses/scsi/fmsc/v1", 0, &std_ops }
};

_EXPORT 
module_info *modules[] =
{
	(module_info *) &sim_fmsc_module,
	NULL
};
