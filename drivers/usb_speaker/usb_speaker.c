/* 	Copyright 2000 Be Incorporated. All Rights Reserved.
**	This file may be used under the terms of the Be Sample Code 
**	License.
*/

#include <OS.h>
#include <Drivers.h>
#include <KernelExport.h>
#include <stdlib.h>
#include <stdio.h>

#include <USB.h>
#include "audio.h"

#define DEBUG_DRIVER 1

#if DEBUG_DRIVER
#define DPRINTF(x) dprintf x
#else
#define DPRINTF(x) ((void)0)
#endif

/* ~44 kHz 16bit stereo */
#define SAMPLE_SIZE	4  
#define QS_OUT	(44*SAMPLE_SIZE*TS)

#define TS	4	/* 4 ms buffers */
#define NB	2

int qs_out = QS_OUT;

typedef struct audiodev audiodev;
typedef struct iso_channel iso_channel;	
typedef struct iso_packet iso_packet;

struct iso_packet
{
	struct iso_packet*		next;
	struct iso_channel*		channel;	
	void*					buffer;
	uint32					status;
	size_t					buffer_size;
	rlea*					rle_array;
};

struct iso_channel
{
	usb_pipe*	ep;
	
	iso_packet*	next_to_queue;
	iso_packet*	current_rw;
	size_t		remain;
	char*		buf_ptr;
	sem_id		num_available_packets;
	area_id		buffer_area;
	iso_packet	iso_packets[NB];
	
	int active;
};

struct audiodev {
	audiodev *next;
	
	int open;
	int number;
	const usb_device *dev;
	
	iso_channel	out_channel;
};

/* handy strings for referring to ourself */
#define ID "usb_speaker: "
static const char *drivername = "usb_speaker";
static const char *basename = "misc/usb_speaker/";

/* list of device instances and names for publishing */
static audiodev *device_list = NULL;
static sem_id dev_list_lock = -1;
static int device_count = 0;

static char **device_names = NULL;

/* handles for the USB bus manager */
static char *usb_name = B_USB_MODULE_NAME;
static usb_module_info *usb;

/* USB Isoch Transaction Stuff ------------------------------------------
**
** Create and Destroy an Isochronous "channel" and handle queueing
** packets, callbacks, etc.
**
*/

void
init_iso_channel(iso_channel* ch, usb_pipe* ep, size_t buf_size, bool is_in)
{
	int		pn;
	void*	big_buffer;

	ch->ep = ep;

	ch->buffer_area = create_area("usb_device_buffer", 
								  (void **)&big_buffer, 
								  B_ANY_KERNEL_ADDRESS, 
								  ((buf_size*NB) + B_PAGE_SIZE-1) & ~(B_PAGE_SIZE-1), 
								  B_CONTIGUOUS, 
								  B_READ_AREA | B_WRITE_AREA);
	
	DPRINTF((ID "buffer_area %d @ 0x%08x\n", ch->buffer_area, big_buffer));

	for(pn=0; pn<NB; pn++) {
		ch->iso_packets[pn].channel = ch;
		ch->iso_packets[pn].buffer = (char*)big_buffer + buf_size*pn;

		if(is_in) {
			ch->iso_packets[pn].rle_array = malloc(sizeof(rlea) + (4-1)*sizeof(rle));
			ch->iso_packets[pn].rle_array->length = 4;
		} else	{
			ch->iso_packets[pn].rle_array = NULL;
		}
			
		ch->iso_packets[pn].next = &ch->iso_packets[(pn+1)%NB];  
	}

	ch->current_rw = ch->next_to_queue = &ch->iso_packets[0];
	if(!is_in) ch->current_rw = &ch->iso_packets[NB-1];
	ch->num_available_packets = create_sem(0, "iso_channel");
	
	ch->remain = 0;
	ch->buf_ptr = 0;
	ch->active = 1;
}

void
uninit_iso_channel(iso_channel* ch)
{
	int	pn;
	
	ch->active = 0;
	
	for(pn=0; pn<NB; pn++) {
		free(ch->iso_packets[pn].rle_array);
	}
	
	delete_sem(ch->num_available_packets);
	delete_area(ch->buffer_area);
}

static void cb_notify_out(void *cookie, uint32 status, 
						  void *data, uint32 actual_len);
static void queue_packet_out(iso_channel* ch);

static void 
cb_notify_out(void *cookie, uint32 status, void *data, uint32 actual_len)
{
	iso_packet* const packet = (iso_packet*) cookie;
	iso_channel*	const channel = packet->channel;
	int			num_queued_packets;
	static bigtime_t	t, t0;
	static 				c;
	int					i;
	int32				sc;

	packet->status = status;
	packet->buffer_size = qs_out;

	if((status == B_OK) && channel->active) {
		queue_packet_out(channel);
		channel->current_rw = packet;

		// hack for startup time
		get_sem_count(channel->num_available_packets, &sc);	
		if(sc < 2) release_sem(channel->num_available_packets);
	}
}

static void
queue_packet_out(iso_channel* ch)
{
	iso_packet*	packet = ch->next_to_queue;
	status_t	s;	

	//dprintf("queue_isochronous_out(%d, %p, %p)\n", qs_out, packet->buffer, packet->rle_array);
	if(s = usb->queue_isochronous(ch->ep, packet->buffer, qs_out,
								  NULL, TS, cb_notify_out, packet)) {
		dprintf(ID "packet out %p status %d\n", packet, s);
	} else {
		ch->next_to_queue = packet->next;
	}
}

void 
start_iso_channel_out(iso_channel* channel)
{
	int pn;
	
	for(pn=0; pn<NB; pn++) {
		queue_packet_out(channel);
	}
}

void 
stop_iso_channel(iso_channel* channel)
{
	usb->cancel_queued_transfers(channel->ep);
}


/* These rather inelegant routines are used to assign numbers to
** device instances so that they have unique names in devfs.
*/

static uint32 device_numbers = 0;

static int 
get_number()
{
	int num;
	
	for(num = 0; num < 32; num++) {
		if(!(device_numbers & (1 << num))){
			device_numbers |= (1 << num);
			return num;
		}
	}
	
	return -1;
}

static void
put_number(int num)
{
	device_numbers &= ~(1 << num);
}

/* Device addition and removal ---------------------------------------
**
** add_device() and remove_device() are used to create and tear down
** device instances.  They are driver by the callbacks device_added()
** and device_removed() which are invoked by the USB bus manager.
*/

static audiodev * 
add_device(const usb_device *dev, const usb_configuration_info *conf)
{
	audiodev *ad = NULL;
	int num,i,ifc,alt;
	const usb_interface_info *ii;
	status_t st;
	usb_pipe *out;
	
	DPRINTF((ID "add_device(%p, %p)\n", dev, conf));
	
	if((num = get_number()) < 0) {
		return NULL;
	}
	
	for(ifc = 0; ifc < conf->interface_count; ifc++){
		for(alt = 0; alt < conf->interface[ifc].alt_count; alt++) {
			ii = &conf->interface[ifc].alt[alt];
			
			/* does it have an AudioStreaming interface? */
			if(ii->descr->interface_class != AC_AUDIO) continue;
			if(ii->descr->interface_subclass != AC_AUDIOSTREAMING) continue;
			if(ii->endpoint_count != 1) continue;
			
			/* ignore input endpoints */
			if(ii->endpoint[0].descr->endpoint_address & 0x80) continue;
			
			/* does it support 2 channel, 16 bit audio? */
			for(i = 0; i < ii->generic_count; i++){
				usb_format_type_descr *ft = (usb_format_type_descr*) ii->generic[i];
				if(ft->type != AC_CS_INTERFACE) continue;
				if(ft->subtype != AC_FORMAT_TYPE) continue;
				if(ft->length < sizeof(usb_format_type_descr)) continue;
				if(ft->num_channels != 2) continue;
				if(ft->subframe_size != 2) continue;
				if(ft->sample_freq_type != 0) continue;
				
				dprintf(ID "found a 2ch, 16bit isoch output (ifc=%d, alt=%d, mp=%d)\n",
						ifc, alt, ii->endpoint[0].descr->max_packet_size);
				
				goto got_one;
			}
		}
	}
	
fail:
	put_number(num);
	if(ad) free(ad);
	return NULL;

got_one:			
	if((ad = (audiodev *) malloc(sizeof(audiodev))) == NULL) goto fail;
	
	ad->dev = dev;
	ad->number = num;
	ad->open = 0;
	
	if((st = usb->set_alt_interface(dev, ii)) != B_OK) {
		dprintf(ID "set_alt_interface(0) returns %d\n", st);
		goto fail;
	}

	if((st = usb->set_configuration(dev,conf)) != B_OK) {
		dprintf(ID "set_configuration() returns %d\n", st);
		goto fail;
	}
	
	out = ii->endpoint[0].handle;		
	DPRINTF((ID "added %p (out=%p) (/dev/%s%d)\n", ad, out, basename, num));
	
	if((st = usb->set_pipe_policy(out, NB, TS, SAMPLE_SIZE)) != B_OK){
		dprintf(ID "set_pipe_policy(out) returns %d\n", st);
		goto fail;
	}

	init_iso_channel(&ad->out_channel, out, qs_out, FALSE);
	start_iso_channel_out(&ad->out_channel);

	/* add it to the list of devices so it will be published, etc */	
	acquire_sem(dev_list_lock);
	ad->next = device_list;
	device_list = ad;
	device_count++;
	release_sem(dev_list_lock);
	
	return ad;
}

static void 
remove_device(audiodev *ad)
{
	if(ad->out_channel.active) stop_iso_channel(&ad->out_channel);
	uninit_iso_channel(&ad->out_channel);
	put_number(ad->number);
	free(ad);
}

static status_t 
device_added(const usb_device *dev, void **cookie)
{
	const usb_configuration_info *conf;
	audiodev *ad;
	int i;

	DPRINTF((ID "device_added(%p,...)\n", dev));

	if(conf = usb->get_nth_configuration(dev, 0)) {
		if((ad = add_device(dev, conf)) != NULL){
			*cookie = (void*) ad;
			return B_OK;
		}
	}
	return B_ERROR;
}


static status_t 
device_removed(void *cookie)
{
	audiodev *ad = (audiodev *) cookie;
	int i;

	DPRINTF((ID "device_removed(%p)\n",ad));
	
	acquire_sem(dev_list_lock);

	/* mark it as inactive and encourage IO to finish */	
	ad->out_channel.active = 0;
	delete_sem(ad->out_channel.num_available_packets);

	/* remove it from the list of devices */	
	if(ad == device_list){
		device_list = ad->next;
	} else {
		audiodev *n;
		for(n = device_list; n; n = n->next){
			if(n->next == ad){
				n->next = ad->next;
				break;
			}
		}
	}
	device_count--;
	
	/* tear it down if it's not open -- 
	   otherwise the last device_free() will handle it */
		
	if(ad->open == 0){
		remove_device(ad);
	} else {
		DPRINTF((ID "device /dev/%s%d still open -- marked for removal\n",
				 basename,ad->number));
	}
	
	release_sem(dev_list_lock);
	
	return B_OK;
}

/* Device Hooks -----------------------------------------------------------
**
** Here we implement the posixy driver hooks (open/close/read/write/ioctl)
*/

static status_t
device_open(const char *dname, uint32 flags, void **cookie)
{	
	audiodev *ad;
	int n;
	
	n = atoi(dname + strlen(basename)); 
	
	DPRINTF((ID "device_open(\"%s\",%d,...)\n",dname,flags));
	
	acquire_sem(dev_list_lock);
	for(ad = device_list; ad; ad = ad->next){
		if(ad->number == n){
			if(ad->out_channel.active) {
				ad->open++;
				*cookie = ad;	
				release_sem(dev_list_lock);
				return B_OK;
			} else {
				dprintf(ID "device is going offline. cannot open\n");
			}
		}
	}
	release_sem(dev_list_lock);
	return B_ERROR;
}
 
static status_t
device_close (void *cookie)
{
	audiodev *ad = (audiodev*) cookie;
	DPRINTF((ID "device_close() name = \"%s%d\"\n",basename,ad->number));
	return B_OK;
}

static status_t
device_free(void *cookie)
{
	audiodev *ad = (audiodev *) cookie;
	
	DPRINTF((ID "device_free() name = \"%s%d\"\n",basename,ad->number));
	
	acquire_sem(dev_list_lock);
	ad->open--;
	if((ad->open == 0) && (ad->out_channel.active == 0)) remove_device(ad);
	release_sem(dev_list_lock);
	
	return B_OK;
}

static status_t
device_read(void *cookie, off_t pos, void *buf, size_t *count)
{
	return B_ERROR;
}

static status_t
device_write(void *cookie, off_t pos, const void *buf, size_t *count)
{
	audiodev* ad = (audiodev*) cookie; 
	iso_channel* channel = &ad->out_channel;
	status_t st;

#if 0
	DPRINTF((ID "device_write(%p,%Ld,0x%x,%d) name = \"%s%d\"\n",
			 cookie, pos, buf, *count, basename, ad->number));
#endif
		
	if(channel->remain == 0) {
		st = acquire_sem_etc(channel->num_available_packets, 1, B_RELATIVE_TIMEOUT, 4*1000*1000);
		if(st) {
			if(st == B_TIMED_OUT) {
				dprintf("st = B_TIMED_OUT\n");
				*count = 0;
			}
			return st;
		}
	
		if(channel->current_rw == channel->next_to_queue) dprintf("e");
		
		channel->remain = channel->current_rw->buffer_size;
		channel->buf_ptr = channel->current_rw->buffer;
	}
	
	if(channel->remain >= *count) {
		memcpy(channel->buf_ptr, buf, *count);
		channel->remain -= *count;
		channel->buf_ptr += *count;
	} else {
		memcpy(channel->buf_ptr, buf, channel->remain);
		*count = channel->remain;
		channel->remain = 0;
		channel->buf_ptr = NULL;
	}
	
	return B_OK;
}

static status_t
device_control (void *cookie, uint32 msg, void *arg1, size_t len)
{
	return B_ERROR;
}


/* Driver Hooks ---------------------------------------------------------
**
** These functions provide the glue used by DevFS to load/unload
** the driver and also handle registering with the USB bus manager
** to receive device added and removed events
*/

static usb_notify_hooks notify_hooks = 
{
	&device_added,
	&device_removed
};

usb_support_descriptor supported_devices[] =
{
	{ AC_AUDIO, 0, 0, 0, 0}
};

_EXPORT status_t 
init_hardware(void)
{
	return B_OK;
}

_EXPORT status_t
init_driver(void)
{
	int i;
	DPRINTF((ID "init_driver(), built %s %s\n", __DATE__, __TIME__));
	
#if DEBUG_DRIVER
	if(load_driver_symbols(drivername) == B_OK) {
		DPRINTF((ID "loaded symbols\n"));
	} else {
		DPRINTF((ID "no symbols for you!\n"));
	}
#endif
			
	if(get_module(usb_name,(module_info**) &usb) != B_OK){
		dprintf(ID "cannot get module \"%s\"\n",usb_name);
		return B_ERROR;
	} 
	
	if((dev_list_lock = create_sem(1,"dev_list_lock")) < 0){
		put_module(usb_name);
		return dev_list_lock;
	}
	
	usb->register_driver(drivername, supported_devices, 1, NULL);
	usb->install_notify(drivername, &notify_hooks);
	
	return B_OK;
}

_EXPORT void
uninit_driver(void)
{
	int i;
	
	DPRINTF((ID "uninit_driver()\n"));
	
	usb->uninstall_notify(drivername);
	
	delete_sem(dev_list_lock);

	put_module(usb_name);

	if(device_names){
		for(i=0;device_names[i];i++) free(device_names[i]);
		free(device_names);
	}
	
}

_EXPORT const char**
publish_devices()
{
	audiodev *ad;
	int i;
	
	DPRINTF((ID "publish_devices()\n"));
	
	if(device_names){
		for(i=0;device_names[i];i++) free((char *) device_names[i]);
		free(device_names);
	}
	
	acquire_sem(dev_list_lock);	
	device_names = (char **) malloc(sizeof(char*) * (device_count + 1));	
	if(device_names){
		for(i = 0, ad = device_list; ad; ad = ad->next){
			if(ad->out_channel.active){
				if(device_names[i] = (char *) malloc(strlen(basename) + 4)){
					sprintf(device_names[i],"%s%d",basename,ad->number);
					DPRINTF((ID "publishing: \"/dev/%s\"\n",device_names[i]));
					i++;
				}
			}
		}
		device_names[i] = NULL;
	}
	release_sem(dev_list_lock);
	
	return (const char **) device_names;
}

static device_hooks DeviceHooks = {
	device_open, 			
	device_close, 			
	device_free,			
	device_control, 		
	device_read,			
	device_write			 
};

_EXPORT device_hooks*
find_device(const char* name)
{
	return &DeviceHooks;
}
