/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include "sonic_vibes.h"
#include "sv_private.h"

#include <string.h>
#include <stdio.h>

#if !defined(_KERNEL_EXPORT_H)
#include <KernelExport.h>
#endif


#if DEBUG
#define KPRINTF(x) kprintf x
#else
#define KPRINTF(x)
#endif

EXPORT status_t init_hardware(void);
EXPORT status_t init_driver(void);
EXPORT void uninit_driver(void);
EXPORT const char ** publish_devices(void);
EXPORT device_hooks * find_device(const char *);


static char pci_name[] = B_PCI_MODULE_NAME;
static pci_module_info	*pci;
static char gameport_name[] = "generic/gameport/v1";
generic_gameport_module * gameport;
static char mpu401_name[] = B_MPU_401_MODULE_NAME;
generic_mpu401_module * mpu401;

#define DO_JOY 1
#define DO_MIDI 1
#define DO_PCM 1
#define DO_MUX 1
#define DO_MIXER 1

#if DO_MIDI
extern device_hooks midi_hooks;
#endif /* DO_MIDI */
#if DO_JOY
extern device_hooks joy_hooks;
#endif /* DO_JOY */
#if DO_PCM
extern device_hooks pcm_hooks;
#endif /* DO_PCM */
#if DO_MUX
extern device_hooks mux_hooks;
#endif /* DO_MUX */
#if DO_MIXER
extern device_hooks mixer_hooks;
#endif /* DO_MIXER */


int32 num_cards;
sonic_vibes_dev cards[NUM_CARDS];
int num_names;
char * names[NUM_CARDS*7+1];
/* vuchar *io_base; */


/* ----------
	PCI_IO_RD - read a byte from pci i/o space
----- */

uint8
PCI_IO_RD (int offset)
{
	return (*pci->read_io_8) (offset);
}


/* ----------
	PCI_IO_RD_32 - read a 32 bit value from pci i/o space
----- */

uint32
PCI_IO_RD_32 (int offset)
{
	return (*pci->read_io_32) (offset);
}
/* ----------
	PCI_IO_WR - write a byte to pci i/o space
----- */

void
PCI_IO_WR (int offset, uint8 val)
{
	(*pci->write_io_8) (offset, val);
}


/* detect presence of our hardware */
status_t 
init_hardware(void)
{
	int ix=0;
	pci_info info;
	status_t err = ENODEV;

	ddprintf(("sonic_vibes: init_hardware()\n"));

	if (get_module(pci_name, (module_info **)&pci))
		return ENOSYS;

	while ((*pci->get_nth_pci_info)(ix, &info) == B_OK) {
		if (info.vendor_id == SONIC_VIBES_VENDOR_ID &&
			info.device_id == SONIC_VIBES_DEVICE_ID) {
			err = B_OK;
		}
		ix++;
	}
#if defined(__POWERPC__) && 0
	{
		char		area_name [32];
		area_info	area;
		area_id		id;

		sprintf (area_name, "pci_bus%d_isa_io", info.bus);
		id = find_area (area_name);
		if (id < 0)
			err = id;
		else if ((err = get_area_info (id, &area)) == B_OK)
			io_base = area.address;
	}
#endif
		
	put_module(pci_name);

	return err;
}


void
set_direct(
	sonic_vibes_dev * card,
	int regno,
	uchar value,
	uchar mask)
{
	if (mask == 0) {
		return;
	}
	if (mask != 0xff) {
		uchar old = PCI_IO_RD(card->enhanced+regno);
		value = (value&mask)|(old&~mask);
		if (regno == 1) {	/* interrupt mask register */
			value = value | 0x7a;	/* always masked interrupts */
		}
	}
	PCI_IO_WR(card->enhanced+regno, value);
	ddprintf(("sonic_vibes: CM%02x  = %02x\n", regno, value));
}


uchar
get_direct(
	sonic_vibes_dev * card,
	int regno)
{
	uchar ret = PCI_IO_RD(card->enhanced+regno);
	return ret;
}


void
set_indirect(
	sonic_vibes_dev * card,
	int regno,
	uchar value,
	uchar mask)
{
	PCI_IO_WR(card->enhanced+0x04, regno);
	EIEIO();
	if (mask == 0) {
		return;
	}
	if (mask != 0xff) {
		uchar old = PCI_IO_RD(card->enhanced+0x05);
		value = (value&mask)|(old&~mask);
	}
	PCI_IO_WR(card->enhanced+0x05, value);
	EIEIO();
	ddprintf(("sonic_vibes: CMX%02x = %02x\n", regno, value));
}


uchar
get_indirect(
	sonic_vibes_dev * card,
	int regno)
{
	uchar ret;
	PCI_IO_WR(card->enhanced+0x04, regno);
	EIEIO();
	ret = PCI_IO_RD(card->enhanced+0x05);
	return ret;
}


#if 0
void dump_card(sonic_vibes_dev * card)
{
	int ix;
	dprintf("\n");
	dprintf("CM:   ");
	for (ix=0; ix<6; ix++) {
		if (ix == 2 || ix == 3) dprintf("   ");
		else dprintf(" %02x", get_direct(card, ix));
	}
	for (ix=0; ix<0x32; ix++) {
		if (!(ix & 7)) {
			dprintf("\nCMX%02x:", ix);
		}
		dprintf(" %02x", get_indirect(card, ix));
	}
	dprintf("\n");
	dprintf("\n");
}
#else
void dump_card(sonic_vibes_dev * card)
{
}
#endif


static void
disable_card_interrupts(
	sonic_vibes_dev * card)
{
	set_direct(card, 0x01, 0xff, 0xff);
}

static void
find_used_pci_function_io(const pci_info* info, uint8* used_io)
{
	int				max_base_reg;
	const ulong*	base_registers;		
/*	const ulong*	base_registers_pci;	*/
	const ulong*	base_register_sizes;	
	const uchar*	base_register_flags;
	int 			i;

	switch(info->header_type & PCI_header_type_mask)
	{
		case PCI_header_type_generic:
			max_base_reg		= 6;
			base_registers		= info->u.h0.base_registers;		
			base_register_sizes = info->u.h0.base_register_sizes;	
			base_register_flags = info->u.h0.base_register_flags;
			break;
			
		case PCI_header_type_PCI_to_PCI_bridge:
			max_base_reg		= 2;
			base_registers		= info->u.h1.base_registers;		
			base_register_sizes = info->u.h1.base_register_sizes;	
			base_register_flags = info->u.h1.base_register_flags;
			break;
			
		default:
			return;	/* BUGBUG handle CardBus bridges */
	} 

	for (i = 0; i < max_base_reg; i++)
	{
	   		if ((!(base_register_flags[i] & PCI_address_space)) ||
				(base_register_sizes[i] == 0))
				continue;
			memset(used_io+(base_registers[i]&0xffff) , 1, (base_register_sizes[i]&0xffff));
	}	
}


static void
find_used_pci_bus_io(int bus, uint8* used_io)
{
	pci_info info;
	int ix=0;

	while((*pci->get_nth_pci_info)(ix++, &info) == B_OK)
	{
		/* we are interested here only in devices/functions on the current bus */
		if(info.bus != bus)
			continue;

		find_used_pci_function_io(&info, used_io);					
	
		/* recursively scan downstream PCI busses connected by PCI-PCI bridges */
		if ((info.header_type & PCI_header_type_mask) == PCI_header_type_PCI_to_PCI_bridge &&
			(info.u.h1.primary_bus == bus))
			find_used_pci_bus_io(info.u.h1.secondary_bus, used_io);
	}
}


static bool
is_conflicting(uint16 base, const uint8* used_io)
{
	uint	i;
	
	for(i=0; i<64; i++)
		if(used_io[base+i] != 0)
			return TRUE;
	
	return FALSE;
} 


static uint16
find_unused_IO(const sonic_vibes_dev * card)
{
	pci_info	info;
	int			ix =0;
	
	extern	void* smalloc(size_t s);
	extern  void  sfree(void* ptr);
	

	if(card->info.bus == 0) /* the card isn't downstream of a PCI-PCI bridge */
	{
		/* we're appropriating some ISA space here... */
		/* need kernel support to do it right */
		return 0x5280+0x40*(card-cards);
	}

	/* find the PCI-PCI bridge for the bus */
	while((*pci->get_nth_pci_info)(ix++, &info) == B_OK)
	{
		if ((info.header_type & PCI_header_type_mask) == PCI_header_type_PCI_to_PCI_bridge &&
			info.u.h1.secondary_bus == card->info.bus)
		{
			uint8*		used_io;
			int		base;
			int		i;
			const int	window_base = ((info.u.h1.io_limit &0xF0) << 8);

			used_io = smalloc(64*1024);
			if(used_io == NULL)
				return 0;
			memset(used_io, 0, 64*1024);
			/* find io space used by all devices on this bus and all downstream busses */
			find_used_pci_bus_io(info.bus, used_io);
			/* add io space of other SonicVibes cards */
			for(i=0; i<num_cards; i++)
				memset(used_io + (cards[num_cards].dma_base&0xffff), 1, 64);
			/* find nonconflicting base inside the bridge window */
			for(base = ((info.u.h1.io_limit &0xF0) << 8) + 0x1000-32; (base >= window_base) && is_conflicting(base, used_io); base -= 64)
				;
							
			if(base < window_base)	/* the whole bridge io window is used */
				base = 0;
		
			sfree(used_io);
			return base;
		}
	}	

	/* if we are here then we are not after a PCI-PCI bridge, probably on a peer PCI bus */	
	/* we're appropriating some ISA space here... */
	/* need kernel support to do it right */
	return 0x5280+0x40*(card-cards);
}


static status_t
setup_dma(
	sonic_vibes_dev * card)
{
	/* we're appropriating some ISA space here... */
	/* need kernel support to do it right */
	const uint16	base = find_unused_IO(card);
	ddprintf(("sonic_vibes: dma base is 0x%04x\n", base));
	if(0 == base)
		return B_DEV_RESOURCE_CONFLICT;
	(*pci->write_pci_config)(card->info.bus, card->info.device, 
		card->info.function, 0x40, 4, ((uint32)base)|0x1);
	/* dma_c is configured 16 bytes above dma_a */
	(*pci->write_pci_config)(card->info.bus, card->info.device, 
		card->info.function, 0x48, 4, ((uint32)base)|0x11);
	card->dma_base = base;
	return B_OK;
}


static void
set_default_registers(
	sonic_vibes_dev * card)
{
	static uchar values[] = {
		0x13, 0x00, 0xff,	/* turn off playback */
		0x16, 0x00, 0xff,	/* turn off loopback */
		0x0e, 0x80, 0xff,	/* mute mixer out */
		0x0f, 0x80, 0xff,
		0x52, 0x33, 0xff,	/* change mode to stereo 16 */
		0x00, 0x80, 0xff,	/* turn on line input */
		0x01, 0x80, 0xff,
		0x10, 0x00, 0xff, 	/* turn on PCM output */
		0x11, 0x00, 0xff,
		0x2c, 0x80, 0xff,	/* turn off SRS */
		0x2d, 0x00, 0xff,
		0x04, 0x08, 0xff,	/* turn on CD through */
		0x05, 0x08, 0xff,
		0x0e, 0x00, 0xff,	/* turn on mixer out */
		0x0f, 0x00, 0xff
	};
	uchar * ptr = values;

	set_direct(card, 0x00, 0x21, 0xff);	/* enhanced mode! */

	while (ptr < values+sizeof(values)) {
		set_indirect(card, ptr[0], ptr[1], ptr[2]);
		ptr += 3;
	}
}


static void
make_device_names(
	sonic_vibes_dev * card)
{
	char * name = card->name;
	sprintf(name, "sonic_vibes/%ld", card-cards+1);

#if DO_MIDI
	sprintf(card->midi.name, "midi/%s", name);
	names[num_names++] = card->midi.name;
#endif /* DO_MIDI */
#if DO_JOY
	sprintf(card->joy.name1, "joystick/%s", name);
	names[num_names++] = card->joy.name1;
#endif /* DO_JOY */
#if DO_PCM
	/* sonic_vibes DMA doesn't work when physical NULL isn't NULL from PCI */
	/* this is a hack to not export bad devices on BeBox hardware */
	if ((*pci->ram_address)(NULL) == NULL) {
		sprintf(card->pcm.name, "audio/raw/%s", name);
		names[num_names++] = card->pcm.name;
		sprintf(card->pcm.oldname, "audio/old/%s", name);
		names[num_names++] = card->pcm.oldname;
	}
#endif /* DO_PCM */
#if DO_MUX
	sprintf(card->mux.name, "audio/mux/%s", name);
	names[num_names++] = card->mux.name;
#endif /* DO_MUX */
#if DO_MIXER
	sprintf(card->mixer.name, "audio/mix/%s", name);
	names[num_names++] = card->mixer.name;
#endif /* DO_MIXER */
	names[num_names] = NULL;
}


/* We use the SV chip in ISA DMA addressing mode, which is 24 bits */
/* so we need to find suitable, locked, contiguous memory in that */
/* physical address range. */

static status_t
find_low_memory(
	sonic_vibes_dev * card)
{
	size_t low_size = (MIN_MEMORY_SIZE+(B_PAGE_SIZE-1))&~(B_PAGE_SIZE-1);
	physical_entry where;
	size_t trysize;
	area_id curarea;
	void * addr;
	char name[DEVNAME];

	sprintf(name, "%s_low", card->name);
	if (low_size < MIN_MEMORY_SIZE) {
		low_size = MIN_MEMORY_SIZE;
	}
	trysize = low_size;

	curarea = find_area(name);
	if (curarea >= 0) {	/* area there from previous run */
		area_info ainfo;
		ddprintf(("sonic_vibes: testing likely candidate...\n"));
		if (get_area_info(curarea, &ainfo)) {
			ddprintf(("sonic_vibes: no info\n"));
			goto allocate;
		}
		/* test area we found */
		trysize = ainfo.size;
		addr = ainfo.address;
		if (trysize < low_size) {
			ddprintf(("sonic_vibes: too small (%lx)\n", trysize));
			goto allocate;
		}
		if (get_memory_map(addr, trysize, &where, 1) < B_OK) {
			ddprintf(("sonic_vibes: no memory map\n"));
			goto allocate;
		}
		if ((uint32)where.address & 0xff000000) {
			ddprintf(("sonic_vibes: bad physical address\n"));
			goto allocate;
		}
		if (ainfo.lock < B_FULL_LOCK || where.size < low_size) {
			ddprintf(("sonic_vibes: lock not contiguous\n"));
			goto allocate;
		}
dprintf("sonic_vibes: physical %p  logical %p\n", where.address, ainfo.address);
		goto a_o_k;
	}

allocate:
	if (curarea >= 0) {
		delete_area(curarea); /* area didn't work */
		curarea = -1;
	}
	ddprintf(("sonic_vibes: allocating new low area\n"));

	curarea = create_area(name, &addr, B_ANY_KERNEL_ADDRESS, 
		trysize, B_LOMEM, B_READ_AREA | B_WRITE_AREA);
	ddprintf(("sonic_vibes: create_area(%lx) returned %lx logical %p\n", 
		trysize, curarea, addr));
	if (curarea < 0) {
		goto oops;
	}
	if (get_memory_map(addr, low_size, &where, 1) < 0) {
		delete_area(curarea);
		curarea = B_ERROR;
		goto oops;
	}
	ddprintf(("sonic_vibes: physical %p\n", where.address));
	if ((uint32)where.address & 0xff000000) {
		delete_area(curarea);
		curarea = B_ERROR;
		goto oops;
	}
	if ((((uint32)where.address)+low_size) & 0xff000000) {
		delete_area(curarea);
		curarea = B_ERROR;
		goto oops;
	}
	/* hey, it worked! */
	if (trysize > low_size) {	/* don't waste */
		resize_area(curarea, low_size);
	}

oops:
	if (curarea < 0) {
		dprintf("sonic_vibes: failed to create low_mem area\n");
		return curarea;
	}
a_o_k:
	ddprintf(("sonic_vibes: successfully found or created low area!\n"));
	card->low_size = low_size;
	card->low_mem = addr;
	card->low_phys = (vuchar *)where.address;
	card->map_low = curarea;
	return B_OK;
}


static status_t
setup_sonic_vibes(
	sonic_vibes_dev * card)
{
	status_t err = B_OK;
/*	cpu_status cp; */

	ddprintf(("sonic_vibes: setup_sonic_vibes(%p)\n", card));

	if ((card->pcm.init_sem = create_sem(1, "sv pcm init")) < B_OK)
		goto bail;
	if ((*mpu401->create_device)(card->info.u.h0.base_registers[3], &card->midi.driver,
		0, midi_interrupt_op, &card->midi) < B_OK)
		goto bail3;
	if ((*gameport->create_device)(card->info.u.h0.base_registers[4], &card->joy.driver) < B_OK)
		goto bail4;
	ddprintf(("midi %p  gameport %p\n", card->midi.driver, card->joy.driver));
	card->midi.card = card;

	err = find_low_memory(card);
	if (err < B_OK) {
		goto bail5;
	}

	//cp = disable_interrupts();
	//acquire_spinlock(&card->hardware);

	make_device_names(card);
	card->enhanced = card->info.u.h0.base_registers[1];
	ddprintf(("sonic_vibes: %s enhanced at %x\n", card->name, card->enhanced));

	ddprintf(("sonic_vibes: revision %x\n", get_indirect(card, 0x15)));

	disable_card_interrupts(card);
	if (setup_dma(card) != B_OK)
	{
		dprintf("sonic vibes: can't setup DMA\n");
		goto bail6;
	}
	
	set_default_registers(card);

	//release_spinlock(&card->hardware);
	//restore_interrupts(cp);

	return B_OK;

bail6:
	//	deallocate low memory	
bail5:
	(*gameport->delete_device)(card->joy.driver);
bail4:
	(*mpu401->delete_device)(card->midi.driver);
bail3:
	delete_sem(card->pcm.init_sem);
bail:
	return err < B_OK ? err : B_ERROR;
}


static int
debug_vibes(
	int argc,
	char * argv[])
{
	int ix = 0;
	if (argc == 2) {
		ix = parse_expression(argv[1])-1;
	}
	if (argc > 2 || ix < 0 || ix >= num_cards) {
		dprintf("sonic_vibes: dude, you gotta watch your syntax!\n");
		return -1;
	}
	dprintf("%s: enhanced registers at 0x%x\n", cards[ix].name, 
		cards[ix].enhanced);
	dprintf("%s: open %ld   dma_a at 0x%x   dma_c 0x%x\n", cards[ix].pcm.name, 
		cards[ix].pcm.open_count, cards[ix].pcm.dma_a, cards[ix].pcm.dma_c);
	if (cards[ix].pcm.open_count) {
		dprintf("    dma_a: 0x%lx+0x%lx   dma_c: 0x%lx+0x%lx\n", 
			PCI_IO_RD_32((int)cards[ix].pcm.dma_a), PCI_IO_RD_32((int)cards[ix].pcm.dma_a+4),
			PCI_IO_RD_32((int)cards[ix].pcm.dma_c), PCI_IO_RD_32((int)cards[ix].pcm.dma_c+4));
	}
	return 0;
}


status_t
init_driver(void)
{
	int ix=0;
	pci_info info;
	num_cards = 0;

	ddprintf(("sonic_vibes: init_driver()\n"));
	load_driver_symbols("sonic_vibes");

	if (get_module(pci_name, (module_info **) &pci))
		return ENOSYS;

	if (get_module(gameport_name, (module_info **) &gameport)) {
		put_module(pci_name);
		return ENOSYS;
	}
	ddprintf(("MPU\n"));
	if (get_module(mpu401_name, (module_info **) &mpu401)) {
		put_module(gameport_name);
		put_module(pci_name);
		return ENOSYS;
	}

	ddprintf(("MPU: %p\n", mpu401));

	while ((*pci->get_nth_pci_info)(ix, &info) == B_OK) {
		if (info.vendor_id == SONIC_VIBES_VENDOR_ID &&
			info.device_id == SONIC_VIBES_DEVICE_ID) {
			if (num_cards == NUM_CARDS) {
				dprintf("Too many SonicVibes cards installed!\n");
				break;
			}
			memset(&cards[num_cards], 0, sizeof(sonic_vibes_dev));
			cards[num_cards].info = info;
			if (setup_sonic_vibes(&cards[num_cards])) {
				dprintf("Setup of SonicVibes %ld failed\n", num_cards+1);
			}
			else {
				num_cards++;
			}
		}
		ix++;
	}
	if (!num_cards) {
		KPRINTF(("no cards\n"));
		put_module(mpu401_name);
		put_module(gameport_name);
		put_module(pci_name);
		ddprintf(("sonic_vibes: no suitable cards found\n"));
		return ENODEV;
	}

#if DEBUG
	add_debugger_command("vibes", debug_vibes, "vibes [card# (1-n)]");
#endif
	return B_OK;
}


static void
teardown_sonic_vibes(
	sonic_vibes_dev * card)
{
	static uchar regs[] = {
		0x10, 0x88, 0xff,	/* mute PCM out */
		0x11, 0x88, 0xff,
		0x13, 0x00, 0xff,	/* turn off playback */
	};
	uchar * ptr = regs;
	cpu_status cp;

	/* remove created devices */
	(*gameport->delete_device)(card->joy.driver);
	(*mpu401->delete_device)(card->midi.driver);

	cp = disable_interrupts();
	acquire_spinlock(&card->hardware);

	while (ptr < regs+sizeof(regs)) {
		set_indirect(card, ptr[0], ptr[1], ptr[2]);
		ptr += 3;
	}
	disable_card_interrupts(card);

	release_spinlock(&card->hardware);
	restore_interrupts(cp);

	delete_sem(card->pcm.init_sem);
}


void
uninit_driver(void)
{
	int ix, cnt = num_cards;
	num_cards = 0;

	ddprintf(("sonic_vibes: uninit_driver()\n"));
	remove_debugger_command("vibes", debug_vibes);

	for (ix=0; ix<cnt; ix++) {
		teardown_sonic_vibes(&cards[ix]);
	}
	memset(&cards, 0, sizeof(cards));
	put_module(mpu401_name);
	put_module(gameport_name);
	put_module(pci_name);
}


const char **
publish_devices(void)
{
	int ix = 0;
	ddprintf(("sonic_vibes: publish_devices()\n"));

	for (ix=0; names[ix]; ix++) {
		ddprintf(("sonic_vibes: publish %s\n", names[ix]));
	}
	return (const char **)names;
}


device_hooks *
find_device(
	const char * name)
{
	int ix;

	ddprintf(("sonic_vibes: find_device(%s)\n", name));

	for (ix=0; ix<num_cards; ix++) {
#if DO_MIDI
		if (!strcmp(cards[ix].midi.name, name)) {
			return &midi_hooks;
		}
#endif /* DO_MIDI */
#if DO_JOY
		if (!strcmp(cards[ix].joy.name1, name)) {
			return &joy_hooks;
		}
#endif /* DO_JOY */
#if DO_PCM
		if (!strcmp(cards[ix].pcm.name, name)) {
			return &pcm_hooks;
		}
		if (!strcmp(cards[ix].pcm.oldname, name)) {
			return &pcm_hooks;
		}
#endif /* DO_PCM */
#if DO_MUX
		if (!strcmp(cards[ix].mux.name, name)) {
			return &mux_hooks;
		}

#endif /* DO_MUX */
#if DO_MIXER
		if (!strcmp(cards[ix].mixer.name, name)) {
			return &mixer_hooks;
		}
#endif /* DO_MIXER */
	}
	ddprintf(("sonic_vibes: find_device(%s) failed\n", name));
	return NULL;
}

int32	api_version = B_CUR_DRIVER_API_VERSION;

static int32
sonic_vibes_interrupt(
	void * data)
{
	cpu_status cp = disable_interrupts();
	sonic_vibes_dev * card = (sonic_vibes_dev *)data;
	uchar status;
	int32 handled = B_UNHANDLED_INTERRUPT;

/*	KTRACE(); / * */
	acquire_spinlock(&card->hardware);

	status = get_direct(card, 0x02);

#if DEBUG
/*	kprintf("%x\n", status); / * */
#endif
#if DO_PCM
	if (status & 0x04) {
		if (dma_c_interrupt(card)) {
			handled = B_INVOKE_SCHEDULER;
		}
		else {
			handled = B_HANDLED_INTERRUPT;
		}
	}
	if (status & 0x01) {
		if (dma_a_interrupt(card)) {
			handled = B_INVOKE_SCHEDULER;
		}
		else {
			handled = B_HANDLED_INTERRUPT;
		}
	}
#endif
#if DO_MIDI
	if (status & 0x80) {
		if (midi_interrupt(card)) {
			handled = B_INVOKE_SCHEDULER;
		} else {
			handled = B_HANDLED_INTERRUPT;
		}
	}
#endif

	/*  Sometimes, the Sonic Vibes will receive a byte of Midi data...
	**  And duly note it in the MPU401 status register...
	**  And generate an interrupt...
	**  But not bother setting the midi interrupt bit in the ISR.
	**  Thanks a lot, S3.
	*/
	if(handled == B_UNHANDLED_INTERRUPT){	
		if (midi_interrupt(card)) {
			handled = B_INVOKE_SCHEDULER;
		}
	}
	
/*	KTRACE(); / * */
	release_spinlock(&card->hardware);
	restore_interrupts(cp);

	return handled;
//	return (handled == B_INVOKE_SCHEDULER) ? B_HANDLED_INTERRUPT : handled;
}


void
increment_interrupt_handler(
	sonic_vibes_dev * card)
{
	KPRINTF(("sonic_vibes: increment_interrupt_handler()\n"));
	if (atomic_add(&card->inth_count, 1) == 0) {
	// !!!
		KPRINTF(("sonic_vibes: intline %d int %p\n", card->info.u.h0.interrupt_line, sonic_vibes_interrupt));
		install_io_interrupt_handler(card->info.u.h0.interrupt_line,
			sonic_vibes_interrupt, card, 0);
	}
}


void
decrement_interrupt_handler(
	sonic_vibes_dev * card)
{
	KPRINTF(("sonic_vibes: decrement_interrupt_handler()\n"));
	if (atomic_add(&card->inth_count, -1) == 1) {
		KPRINTF(("sonic_vibes: remove_io_interrupt_handler()\n"));
		remove_io_interrupt_handler(card->info.u.h0.interrupt_line, sonic_vibes_interrupt, card);
	}
}


