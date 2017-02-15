/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

//////////////////////////////////////////////////////////////////////////////
// Driver
//   My goodness, but there's some work to do here.  I think we can probably
// bust this up a bit and fix it.  At the moment, I'm just going to clean
// it up a bit and see if I can modularize it.
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Includes //////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////


#include <KernelExport.h>
#include <PCI.h>
#include <graphic_driver.h>
#include <stdio.h>          // For sprintf().
#include <stdlib.h>         // For calloc() and free().

#include <registers.h>
#include <cardid.h>
#include <private.h> // This should probably be broken out.
#include "driver.h"


//////////////////////////////////////////////////////////////////////////////
// Globals ///////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

static DEVICE_DATA *pd;

static pci_module_info  *pci_bus;

static device_hooks graphics_device_hooks = 
{
  open_hook,
  close_hook,
  free_hook,
  control_hook,
  read_hook,
  write_hook,
  NULL,
  NULL
};

static uint16 mga_device_list[] = 
{
  MGA_2064W,
  MGA_1064S,
  MGA_2164W,
  MGA_2164W_AGP,
  MGA_G100_AGP,
  MGA_G200_AGP,
  0
};

static struct 
{
  uint16 vendor;
  uint16 *devices;
} SupportedDevices[] = 
{
  {MGA_VENDOR, mga_device_list},
  {0x0000, NULL}
};

static spinlock splok;

int32   api_version = 2;


//////////////////////////////////////////////////////////////////////////////
// Functions /////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Initialize Hardware
//   This sets up the hardware, or dies.  If it succeeds, it returns B_OK
// and is in charge of the device it controls.  If it returns anything
// else, it is unceremoniously tossed from memory.

status_t init_hardware(void)
{
  long          pci_index = 0;
  pci_info      pcii;

  int vendor;
  uint16 *devices;

  // choke if we can't find the PCI bus
  
  if(get_module(B_PCI_MODULE_NAME, (module_info **)&pci_bus) != B_OK)
    {
      return B_ERROR;
    }

  // while there are more pci devices
  
  for(;
      ((*pci_bus->get_nth_pci_info)(pci_index, &pcii) == B_NO_ERROR);
      pci_index++)
    {
      for(vendor = 0 ; SupportedDevices[vendor].vendor ; vendor++)
        {
          if(SupportedDevices[vendor].vendor == pcii.vendor_id)
            {
              for(devices = SupportedDevices[vendor].devices;
		  *devices ;
		  devices++)
                {
                  if(*devices == pcii.device_id)
                    {
		      put_module(B_PCI_MODULE_NAME);
		      return B_OK;
                    }
                }
            }
        }
    }

  put_module(B_PCI_MODULE_NAME);

  return B_ERROR;
}


//////////////////////////////////////////////////////////////////////////////
// Probe Devices
//

static void probe_devices(void)
{
  long pci_index = 0;
  uint32 count = 0;
  DEVICE_INFO *di = pd->di;

  // while there are more pci devices

  while((count < MAX_MGA_DEVICES) && 
        ((*pci_bus->get_nth_pci_info)(pci_index, &(di->pcii)) == B_NO_ERROR))
    {
      int vendor = 0;
                
      ddprintf(("MKD: checking pci index %d, device 0x%04x/0x%04x\n", 
                pci_index, 
                di->pcii.vendor_id, 
                di->pcii.device_id));

      // if we match a supported vendor

      while(SupportedDevices[vendor].vendor)
        {
          if (SupportedDevices[vendor].vendor == di->pcii.vendor_id)
            {
              uint16 *devices = SupportedDevices[vendor].devices;

              // while there are more supported devices

              while(*devices)
                {
                  // if we match a supported device

                  if(*devices == di->pcii.device_id )
                    {
                      // publish the device name
                      
                      sprintf(di->name,
                              "graphics%s/%04X_%04X_%02X%02X%02X",
                              GRAPHICS_DEVICE_PATH,
                              di->pcii.vendor_id,
                              di->pcii.device_id,
                              di->pcii.bus,
                              di->pcii.device,
                              di->pcii.function);

                      ddprintf(("MKD: making /dev/%s\n", di->name));

                      // remember the name

                      pd->device_names[count] = di->name;

                      // mark the driver as available for R/W open

                      di->is_open = 0;

                      // mark areas as not yet created

                      di->shared_area = -1;
                        
                      // mark pointer to shared data as invalid

                      di->si = NULL;

                      // inc pointer to device info

                      di++;

                      // inc count

                      count++;

                      // break out of this while loop

                      break;
                    }

                  // next supported device

                  devices++;
                }
            }

          vendor++;
        }

      // next pci_info struct, please

      pci_index++;
    }

  // propagate count

  pd->count = count;

  // terminate list of device names with a null pointer

  pd->device_names[pd->count] = NULL;
  ddprintf(("MKD probe_devices: %d supported devices\n", pd->count));
}


//////////////////////////////////////////////////////////////////////////////
// Initialize Driver
//   This sets up the driver, or dies.  If it succeeds, it returns B_OK
// and is in charge of the device it controls.  If it returns anything
// else, it is unceremoniously tossed from memory.

status_t init_driver(void) 
{
  // get a handle for the pci bus

  if (get_module(B_PCI_MODULE_NAME, (module_info **)&pci_bus) != B_OK)
    {
      return B_ERROR;
    }

  // driver private data

  pd = (DEVICE_DATA *)calloc(1, sizeof(DEVICE_DATA));
  
  if(!pd)
    {
      put_module(B_PCI_MODULE_NAME);
      return B_ERROR;
    }
        
  // initialize the benaphore

  pd->ben = 0;
  pd->sem = create_sem(0, "MGA Kernel Driver");

  // init the spinlock

  splok = 0;
        
  // find all of our supported devices

  probe_devices();

  return B_OK;
}


//////////////////////////////////////////////////////////////////////////////
// Publish Devices
//    This returns a list of supported devices.

const char **publish_devices(void)
{
  return (const char **)pd->device_names;
}


//////////////////////////////////////////////////////////////////////////////
// Find the Device
//    This looks for the device that matches the input string.

device_hooks *find_device(const char *name)
{
  int index = 0;

  while(pd->device_names[index])
    {
      if(strcmp(name, pd->device_names[index]) == 0)
        {
          return &graphics_device_hooks;
        }
      index++;
    }
  return NULL;
}


//////////////////////////////////////////////////////////////////////////////
// Uninitialize the Driver
//    Shut everything down.

void uninit_driver(void)
{
  // free the driver data

  delete_sem(pd->sem);
  free(pd);
  pd = NULL;

  // put the pci module away

  put_module(B_PCI_MODULE_NAME);
}


//////////////////////////////////////////////////////////////////////////////
// Map the Device
//

static status_t map_device(DEVICE_INFO *di)
{
  // default: frame buffer in [0], control regs in [1]

  int regs = 1;
  int fb   = 0;

  char buffer[B_OS_NAME_LENGTH];
  SHARED_INFO *si = di->si;
  uint32 tmpUlong;
  pci_info *pcii = &(di->pcii);

  // enable memory mapped IO
  
  tmpUlong = get_pci(STORM_DEVCTRL, 4);
  tmpUlong &= 0xfffffff8; // Turn off bus mastering, memory mapping, and i/o port mapping.
//  tmpUlong &= 0xfffffffe;	// Turn off i/o port mapping.
  tmpUlong |= 0x00000002; // Turn on memory mapping.
  set_pci(STORM_DEVCTRL, 4, tmpUlong);
        
  if(((di->pcii.device_id == MGA_2064W)) ||
     ((di->pcii.device_id == MGA_1064S) && (di->pcii.revision <= 2)))
    {
      // control regs in [0], frame buffer in [1]

      regs = 0;
      fb   = 1;
    }

  // map the areas

  sprintf(buffer,
          "%04X_%04X_%02X%02X%02X regs",
          di->pcii.vendor_id,
          di->pcii.device_id,
          di->pcii.bus,
          di->pcii.device,
          di->pcii.function);

  si->regs_area
    = map_physical_memory(buffer,
                          (void *) di->pcii.u.h0.base_registers[regs],
                          di->pcii.u.h0.base_register_sizes[regs],
                          B_ANY_KERNEL_ADDRESS,
                          B_READ_AREA + B_WRITE_AREA,
                          (void **)&(si->regs));

  // return the error if there was some problem

  if(si->regs_area < 0) return si->regs_area;
        
  sprintf(buffer,
          "%04X_%04X_%02X%02X%02X framebuffer",
          di->pcii.vendor_id,
          di->pcii.device_id,
          di->pcii.bus, di->pcii.device,
          di->pcii.function);

  si->fb_area 
    = map_physical_memory(buffer,
                          (void *) di->pcii.u.h0.base_registers[fb],
                          di->pcii.u.h0.base_register_sizes[fb],
#if defined(__INTEL__)
                          B_ANY_KERNEL_ADDRESS | B_MTR_WC,
#else
                          B_ANY_KERNEL_BLOCK_ADDRESS,
#endif
                          B_READ_AREA + B_WRITE_AREA,
                          &(si->framebuffer));

#if defined(__INTEL__)
  if(si->fb_area < 0)   // try to map this time without write combining
    {
      si->fb_area 
        = map_physical_memory(buffer,
                              (void *) di->pcii.u.h0.base_registers[fb],
                              di->pcii.u.h0.base_register_sizes[fb],
                              B_ANY_KERNEL_ADDRESS,
                              B_READ_AREA + B_WRITE_AREA,
                              &(si->framebuffer));
    }
#endif
                
  // if there was an error, delete our other area

  if(si->fb_area < 0) 
    {
      delete_area(si->regs_area);
      si->regs_area = -1;
    }

  // remember the DMA address of the frame buffer for BDirectWindow purposes

  si->framebuffer_pci = (void *) di->pcii.u.h0.base_registers_pci[fb];

  // in any case, return the result

  return si->fb_area;
}


//////////////////////////////////////////////////////////////////////////////
// Unmap the Device
//

static void unmap_device(DEVICE_INFO *di)
{
  SHARED_INFO *si = di->si;
  uint32 tmpUlong;
  pci_info *pcii = &(di->pcii);

  // disable memory mapped IO

  tmpUlong = get_pci(STORM_DEVCTRL, 4);

  tmpUlong &= 0xfffffff8; // Turn off bus mastering too.
  set_pci(STORM_DEVCTRL, 4, tmpUlong);

  if(si->regs_area >= 0) delete_area(si->regs_area);

  if(si->fb_area >= 0) delete_area(si->fb_area);

  si->regs_area = si->fb_area = -1;
  si->framebuffer = NULL;
  si->regs = NULL;
}


//////////////////////////////////////////////////////////////////////////////
// Thread Interupt Work
// Performs hardware register sets that need to be synchronized to the VBI.
// Operations are requested by setting "flags" appropriately.
//
// NOTE: Most of what's in here doesn't need to be synchronized to the VBI.
// In fact, IIRC only changing the start address does, and that's only for
// cards with the TI DAC.
//

static int32 thread_interrupt_work(int32 *flags,
                                  vuchar *regs,
                                  SHARED_INFO *si)
{
  int32 result = B_HANDLED_INTERRUPT;
  if(atomic_and(flags, ~MKD_SET_START_ADDR) & MKD_SET_START_ADDR)
    {
      uchar tmpByte;

      // move start_addr

      STORM8W(VGA_CRTC_INDEX, 0x0c);
      STORM8W(VGA_CRTC_DATA, ((si->start_addr >> 8) & 0xff));
      STORM8W(VGA_CRTC_INDEX, 0x0d);
      STORM8W(VGA_CRTC_DATA, (si->start_addr & 0xff));
      STORM8W(VGA_CRTCEXT_INDEX, 0);
      STORM8R(VGA_CRTCEXT_DATA, tmpByte);
      tmpByte &= 0xf0;
      tmpByte |= (uchar)((si->start_addr & 0x0f0000) >> 16);
      STORM8W(VGA_CRTCEXT_DATA, tmpByte);
    }

  if(atomic_and(flags, ~MKD_MOVE_CURSOR) & MKD_MOVE_CURSOR)
    {
      uint16 cx = si->cursor_x;
      uint16 cy = si->cursor_y;

      // move cursor

      DAC8W(TVP3026_CUR_XLOW, cx & 0xff);
      DAC8W(TVP3026_CUR_YLOW, cy & 0xff);
      DAC8W(TVP3026_CUR_XHI, (cx >> 8) & 0xff);
      DAC8W(TVP3026_CUR_YHI, (cy >> 8) & 0xff);
    }

  if(atomic_and(flags, ~MKD_SET_CURSOR) & MKD_SET_CURSOR)
    {
      switch(si->device_id)
        {
        case MGA_2064W:
        case MGA_2164W:
        case MGA_2164W_AGP:
          {
            int i;
            uint8 tmpByte;
            uint8 *cursor0 = si->cursor0;
            uint8 *cursor1 = si->cursor1;

            DAC8W(TVP3026_INDEX, TVP3026_CURSOR_CTL);
            DAC8R(TVP3026_DATA, tmpByte);
            tmpByte &= 0xf3; // cursor ram address top two bits == 0
            DAC8W(TVP3026_DATA, tmpByte);
            DAC8W(TVP3026_INDEX, 0x00); // start with cursor color 0

            for(i = 0; i < 512; i++)
              {
                DAC8W(TVP3026_CUR_RAM, cursor0[i]);
              }

            for (i = 0; i < 512; i++)
              {
                DAC8W(TVP3026_CUR_RAM, cursor1[i]);
              }
          } break;
        }
      // HACK - not doing anything for the 1064, G100, and G200!
    }

  if(atomic_and(flags, ~MKD_PROGRAM_CLUT) & MKD_PROGRAM_CLUT)
    {
      // program clut

      uint16 count = si->color_count;
      uint16 first_color = si->first_color;
      uint8 *src = si->color_data + (first_color * 3);

      // program clut

      DAC8W(TVP3026_INDEX, (uchar)first_color);

      while(count--)
        {
          DAC8W(TVP3026_COL_PAL, *src++);
          DAC8W(TVP3026_COL_PAL, *src++);
          DAC8W(TVP3026_COL_PAL, *src++);
        }
    }

  // release the vblank semaphore

  if(si->vblank >= 0)
    {
      int32 blocked;

      if((get_sem_count(si->vblank, &blocked) == B_OK) && (blocked < 0))
        {
          release_sem_etc(si->vblank, -blocked, B_DO_NOT_RESCHEDULE);
          result = B_INVOKE_SCHEDULER;
        }
    }
  return result;
}


//////////////////////////////////////////////////////////////////////////////
// Matrox Interupt
//

static int32 matrox_interrupt(void *data)
{
  int32 handled = B_UNHANDLED_INTERRUPT;
  DEVICE_INFO *di = (DEVICE_INFO *)data;
  SHARED_INFO *si = di->si;
  int32 *flags = &(si->flags);
  vuchar *regs;

  // is someone already handling an interrupt for this device?

  if(atomic_or(flags, MKD_HANDLER_INSTALLED) & MKD_HANDLER_INSTALLED)
    {
      return handled;
    }

  // get regs for STORMx() macros

  regs = si->regs;

  // did this card cause an interrupt

  if(STORM32(STORM_STATUS) & STORM_VLINE_INT)
    {
      // do our stuff

      handled = thread_interrupt_work(flags, regs, si);

      // clear the interrupt status

      STORM32W(STORM_ICLEAR, 0xffffffff);

    }

  // note that we're not in the handler any more

  atomic_and(flags, ~MKD_HANDLER_INSTALLED);

  return handled;                               
}


//////////////////////////////////////////////////////////////////////////////
// Fake Interupt Thread Function
//

static int32 fake_interrupt_thread_func(void *_di)
{
  DEVICE_INFO *di = (DEVICE_INFO *)_di;
  SHARED_INFO *si = di->si;

  int32 *flags = &(si->flags);

  vuchar *regs = si->regs;

  bigtime_t last_sync;
  bigtime_t this_sync;
  bigtime_t diff_sync;

  uint32 counter = 1;

  // a lie, but we have to start somewhen

  last_sync = system_time() - 8333;

  ddprintf(("fake_interrupt_thread_func begins\ndi: 0x%08x\nsi: 0x%08x\nflags: 0x%08x\n", di, si, flags));

  // loop until notified

  while(atomic_and(flags, -1) & MKD_HANDLER_INSTALLED)
    {
      // see if "interrupts" are enabled

      if((volatile int32)(di->can_interrupt))
        {
          // poll the retrace flag

          STORM8POLL(VGA_INSTS1, 0, 0x08); // spin

          // get the system_time

          this_sync = system_time();

          // do our stuff

          thread_interrupt_work(flags, regs, si);
        }
      else 
        {
          // get the system_time

          this_sync = system_time();
        }

      // find out how long it took

      diff_sync = this_sync - last_sync;

      // back off a little so we're sure to catch the retrace

      diff_sync -= diff_sync / 10;

      // impose some limits so we can recover from refresh rate changes
      // Supported refresh rates are 48 Hz - 120 Hz, so these limits should
      // be slightly wider.

      if(diff_sync < 8000)
        {
          diff_sync = 8000; // not less than 1/125th of sec
        }

      if(diff_sync > 16666)
        {
          diff_sync = 20000; // not more than 1/40th of sec
        }

      if((counter++ & 0x01ff) == 0)
        {
          diff_sync >>= 2; // periodically quarter the wait to resync
        }

      // update for next go-around

      last_sync = this_sync;

      // snooze until our next retrace

      snooze_until(this_sync + diff_sync, B_SYSTEM_TIMEBASE);
    }

  ddprintf(("fake_interrupt_thread_func ends with flags = 0x%08x\n", *flags));

  // gotta return something

  return B_OK;
}


//////////////////////////////////////////////////////////////////////////////
// Drop to Power Saving Mode
//

void idle_device(DEVICE_INFO *di)
{
  // put device in lowest power consumption state
  // HACK - do nothing.
}


//////////////////////////////////////////////////////////////////////////////
// Lock/Unlock the Driver

static void lock_driver(void)
{
  if (atomic_add(&pd->ben, 1) >= 1)
    acquire_sem(pd->sem);
}


static void unlock_driver(void)
{
  if (atomic_add(&pd->ben, -1) > 1)
    release_sem(pd->sem);
}


//////////////////////////////////////////////////////////////////////////////
// Lock/Unlock the Device
// NOTE: We apparently aren't using these anywhere in the driver.
//

#if 0
static void lock_device(device_info *di)
{
  if (atomic_add(&di->ben, 1) >= 1)
    acquire_sem(di->sem);
}


static void unlock_device(device_info *di)
{
  if (atomic_add(&di->ben, -1) > 1)
    release_sem(di->sem);
}
#endif


//////////////////////////////////////////////////////////////////////////////
// Opens a graphics device.
//

static status_t open_hook(const char* name,
                          uint32 flags,
                          void** cookie)
{
  int32 index = 0;
  DEVICE_INFO *di;
  SHARED_INFO *si;
  thread_id     thid;
  thread_info   thinfo;
  status_t      result = B_OK;
  vuchar                *regs;
  char shared_name[B_OS_NAME_LENGTH];

  ddprintf(("MKD open_hook(%s, %d, 0x%08x)\n", name, flags, cookie));

  // find the device name in the list of devices
  // we're never passed a name we didn't publish
  // [Chris:] ...We hope.

  while((pd->device_names[index]) && 
        (strcmp(name, pd->device_names[index]) != 0))
    {
      index++;
    }

  // for convienience

  di = &(pd->di[index]);

  // make sure no one else has write access to the common data

  lock_driver();

  // if it's already open for writing

  if(di->is_open)
    {
      unlock_driver();
      return B_BUSY;
    }

  // create the shared area

  sprintf(shared_name,
          "%04X_%04X_%02X%02X%02X shared",
          di->pcii.vendor_id,
          di->pcii.device_id,
          di->pcii.bus,
          di->pcii.device,
          di->pcii.function);

  di->shared_area = create_area(shared_name,
                                (void **)&(di->si),
                                B_ANY_KERNEL_ADDRESS,
                                B_PAGE_SIZE,
                                B_FULL_LOCK, B_READ_AREA | B_WRITE_AREA);

  if(di->shared_area < 0)
    {
      unlock_driver();
      return di->shared_area;
    }

  // save a few dereferences
  
  si = di->si;

  // save the vendor and device IDs

  si->vendor_id = di->pcii.vendor_id;
  si->device_id = di->pcii.device_id;
  si->revision = di->pcii.revision;

  // map the device

  result = map_device(di);

  if(result < 0) goto free_shared;

  result = B_OK;

  // create a semaphore for vertical blank management

  si->vblank = create_sem(0, di->name);

  if (si->vblank < 0)
    {
      result = si->vblank;
      goto unmap;
    }

  // change the owner of the semaphores to the opener's team

  thid = find_thread(NULL);
  get_thread_info(thid, &thinfo);
  set_sem_owner(si->vblank, thinfo.team);

  // assign local regs pointer for STORMxx() macros

  regs = si->regs;

  // disable and clear any pending interrupts - probably not required, but...
  
  STORM32W(STORM_IEN, 0);
  STORM32W(STORM_ICLEAR, 0xffffffff);

  si->interrupt_line = di->pcii.u.h0.interrupt_line;

  if ((di->pcii.u.h0.interrupt_pin == 0x00) || (di->pcii.u.h0.interrupt_line == 0xff))
    {
      // fake some kind of interrupt with a thread

      di->can_interrupt = FALSE;
      si->flags = MKD_HANDLER_INSTALLED;

      di->tid = spawn_kernel_thread(fake_interrupt_thread_func,
                                    "MKD fake interrupt",
                                    B_REAL_TIME_DISPLAY_PRIORITY,
                                    di);
      
      if(di->tid < 0) goto delete_the_sem;

      resume_thread(di->tid);
    }
  else
    {
      install_io_interrupt_handler(di->pcii.u.h0.interrupt_line, matrox_interrupt, (void *)di, 0);
    }

  // mark the device open

  di->is_open++;

  // send the cookie to the opener

  *cookie = di;
        
  goto done;

delete_the_sem:
  delete_sem(si->vblank);

unmap:
  unmap_device(di);

free_shared:
  // clean up our shared area

  delete_area(di->shared_area);
  di->shared_area = -1;
  di->si = NULL;

done:
  // end of critical section

  unlock_driver();

  // all done, return the status

  ddprintf(("open_hook returning 0x%08x\n", result));
  return result;
}


//////////////////////////////////////////////////////////////////////////////
// Read Hook
//    read_hook - does nothing, gracefully

static status_t read_hook(void* dev,
                          off_t pos,
                          void* buf,
                          size_t* len)
{
  *len = 0;
  return B_NOT_ALLOWED;
}


//////////////////////////////////////////////////////////////////////////////
// Write Hook
//    write_hook - does nothing, gracefully

static status_t write_hook(void* dev,
                           off_t pos,
                           const void* buf,
                           size_t* len)
{
  *len = 0;
  return B_NOT_ALLOWED;
}


//////////////////////////////////////////////////////////////////////////////
// Close Hook
//    close_hook - does nothing, gracefully

static status_t close_hook(void* dev)
{
  ddprintf(("MKD close_hook(%08x)\n", dev));

  // we don't do anything on close: there might be dup'd fd

  return B_NO_ERROR;
}


//////////////////////////////////////////////////////////////////////////////
// Free Hook
//    free_hook - close down the device

static status_t free_hook(void* dev)
{
  DEVICE_INFO *di = (DEVICE_INFO *)dev;
  SHARED_INFO   *si = di->si;
  vuchar *regs = si->regs;

  ddprintf(("MKD free_hook() begins...\n"));

  // lock the driver

  lock_driver();

  // disable interrupts for this card

  STORM32W(STORM_IEN, 0);

  // clear interrupt status

  STORM32W(STORM_ICLEAR, 0xffffffff);

  if ((di->pcii.u.h0.interrupt_pin == 0x00) || (di->pcii.u.h0.interrupt_line == 0xff))
    {
      // stop our interrupt faking thread

      di->can_interrupt = FALSE;
      si->flags = 0; // Among other things, sets MKD_HANDLER_INSTALLED to 0.
    }
  else // remove interrupt handler
    {
      remove_io_interrupt_handler(di->pcii.u.h0.interrupt_line, matrox_interrupt, di);
    }

  // mark the device available

  di->is_open--;
  // delete the semaphores, ignoring any errors ('cause the owning
  // team may have died on us)

  delete_sem(si->vblank);
  si->vblank = -1;

  // free regs and framebuffer areas

  unmap_device(di);

  // clean up our shared area

  delete_area(di->shared_area);
  di->shared_area = -1;
  di->si = NULL;

  // unlock the driver

  unlock_driver();
  ddprintf(("MKD free_hook() ends.\n"));

  // all done

  return B_OK;
}


//////////////////////////////////////////////////////////////////////////////
// Control Hook
//    control_hook - where the real work is done

static status_t control_hook(void* dev,
                             uint32 msg,
                             void *buf,
                             size_t len)
{
  DEVICE_INFO *di = (DEVICE_INFO *)dev;
  SHARED_INFO   *si = di->si;
  status_t result = B_DEV_INVALID_IOCTL;

  // ddprintf(("ioctl: %d, buf: 0x%08x, len: %d\n", msg, buf, len));

  switch(msg)
    {
      // the only PUBLIC ioctl

    case B_GET_ACCELERANT_SIGNATURE:
      {
        char *sig = (char *)buf;

        // strcpy(sig, "application/x-vnd.Be-matrox.accelerant");

        strcpy(sig, "matrox.accelerant");
        result = B_OK;
      }
      break;
                
      // PRIVATE ioctl from here on

    case MGA_GET_PRIVATE_DATA:
      {
        mga_get_private_data *gpd = (mga_get_private_data *)buf;

        if(gpd->magic == MGA_PRIVATE_DATA_MAGIC)
          {
            gpd->si = si;
            result = B_OK;
          }
      }
      break;

    case MGA_GET_PCI:
      {
        MGA_GET_SET_PCI *gsp = (MGA_GET_SET_PCI *)buf;
      
        if(gsp->magic == MGA_PRIVATE_DATA_MAGIC)
          {
            pci_info *pcii = &(di->pcii);
            gsp->value = get_pci(gsp->offset, gsp->size);
            result = B_OK;
          }
      }
      break;

    case MGA_SET_PCI:
      {
        MGA_GET_SET_PCI *gsp = (MGA_GET_SET_PCI *)buf;

        if(gsp->magic == MGA_PRIVATE_DATA_MAGIC)
          {
            pci_info *pcii = &(di->pcii);
            set_pci(gsp->offset, gsp->size, gsp->value);
            result = B_OK;
          }
      }
      break;

    case MGA_MAP_ROM:
      {
        MGA_SET_BOOL_STATE *mr = (MGA_SET_BOOL_STATE *)buf;

        if(mr->magic == MGA_PRIVATE_DATA_MAGIC)
          {
            pci_info *pcii = &di->pcii;

            if(mr->do_it)
              {
                // map the rom into the frame buffer

                uint32 temp;

                // turn on bios in OPTION

                set_pci(STORM_OPTION,
                        4,
                        0x40000000 | get_pci(STORM_OPTION, 4));

                // enable bios in frame buffer memory (PCI address space)

                if(((pcii->device_id == MGA_2064W)) ||
                   ((pcii->device_id == MGA_1064S) && (pcii->revision <= 2)))
                  {
                    // swapped base and control regs from PCI default

                    temp = get_pci(STORM_MGABASE2, 4);
                  }
                else
                  {
                    temp = get_pci(STORM_MGABASE1, 4);
                  }

                temp &= PCI_rom_address_mask;
                set_pci(STORM_ROMBASE, 4, 0x01L | temp);

                ddprintf(("MKD: mapping ROMBASE to 0x%08x\n", temp));
              }
            else
              {
                // unmap the rom from the frame buffer
                // turn off bios in OPTION

                set_pci(STORM_OPTION,
                        4,
                        0xbfffffff & get_pci(STORM_OPTION, 4));

                // disable bios

                set_pci(STORM_ROMBASE, 4, 0L);

                ddprintf(("MKD: ROMBASE mapping disabled\n"));
              }

            result = B_OK;
          }
      }
      break;

    case MGA_RUN_INTERRUPTS:
      {
        MGA_SET_BOOL_STATE *ri = (MGA_SET_BOOL_STATE *)buf;

        if(ri->magic == MGA_PRIVATE_DATA_MAGIC)
          {
			if ((di->pcii.u.h0.interrupt_pin == 0x00) || (di->pcii.u.h0.interrupt_line == 0xff))
              {
                di->can_interrupt = ri->do_it;
              }
            else
              {
                vuchar *regs = si->regs;

                if(ri->do_it)
                  {
                    // resume interrupts

                    STORM32W(STORM_IEN, STORM_VLINE_INT);
                  }
                else
                  {
                    // disable interrupts

                    STORM32W(STORM_IEN, 0);
                    STORM32W(STORM_ICLEAR, 0xffffffff);
                  }
              }

            result = B_OK;
          }
      }
      break;
    }

  return result;
}


//////////////////////////////////////////////////////////////////////////////
// This Is The End Of The File ///////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
