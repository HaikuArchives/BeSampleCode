// main.c
// ------
// A bare-bones graphics demo that uses direct driver access.
// This is intended to run on an absolute bare-bones install
// of BeOS (w/o app_server). See the newsletter article for
// details.
//
// Copyright 2000, Be Incorporated.   All Rights Reserved.
// This file may be used under the terms of the Be Sample Code License.

#include <OS.h>
#include <image.h>
#include <FindDirectory.h>
#include <graphic_driver.h>
#include <Accelerant.h>
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <malloc.h>
#include <errno.h>
#include <stdlib.h>

int init_file_handles(void) {
	freopen("/dev/null", "r", stdin);
	freopen("/boot/var/log/demo.out", "w", stdout);
	freopen("/boot/var/log/demo.err", "w", stderr);
	/* */
	putenv("LIBRARY_PATH=/boot/beos/system/lib");
	putenv("ADDON_PATH=/boot/beos/system/add-ons");
	return B_OK;
}

int pick_device(const char *apath) {
	DIR				*d;
	struct dirent	*e;
	char name_buf[1024];
	int fd = -1;

	/* open directory apath */
	d = opendir(apath);
	if (!d) return B_ERROR;
	/* get a list of devices, filtering out ".", "..", and "stub" */
	/* the only reason stub is disabled is that I know stub (aka R3-style) drivers don't support wait for retrace */
	while ((e = readdir(d)) != NULL) {
		if (!strcmp(e->d_name, ".") || !strcmp(e->d_name, "..") || !strcmp(e->d_name, "stub"))
			continue;
		strcpy(name_buf, apath);
		strcat(name_buf, "/");
		strcat(name_buf, e->d_name);
		fd = open(name_buf, B_READ_WRITE);
		if (fd >= 0) break;
	}
	closedir(d);
	return fd;
}

image_id load_accelerant(int fd, GetAccelerantHook *hook) {
	status_t result;
	image_id image = -1;
	int i;
	char
		signature[1024],
		path[PATH_MAX];
	struct stat st;
	const static directory_which vols[] = {
		B_USER_ADDONS_DIRECTORY,
		B_BEOS_ADDONS_DIRECTORY
	};

	/* get signature from driver */
	result = ioctl(fd, B_GET_ACCELERANT_SIGNATURE, &signature, sizeof(signature));
	if (result != B_OK) goto done;
	printf("B_GET_ACCELERANT_SIGNATURE returned ->%s<-\n", signature);

	// note failure by default
	for(i=0; i < sizeof (vols) / sizeof (vols[0]); i++) {

		/* ---
			compute directory path to common or beos addon directory on
			floppy or boot volume
		--- */

		printf("attempting to get path for %d (%d)\n", i, vols[i]);
		if (find_directory (vols[i], -1, false, path, PATH_MAX) != B_OK) {
			printf("find directory failed\n");
			continue;
		}

		strcat (path, "/accelerants/");
		strcat (path, signature);

		printf("about to stat(%s)\n", path);
		// don't try to load non-existant files
		if (stat(path, &st) != 0) continue;
		printf("Trying to load accelerant: %s\n", path);
		// load the image
		image = load_add_on(path);
		if (image >= 0) {
			printf("Accelerant loaded!\n");
			// get entrypoint from accelerant
			result = get_image_symbol(image, B_ACCELERANT_ENTRY_POINT,
#if defined(__INTEL__)
				B_SYMBOL_TYPE_ANY,
#else
				B_SYMBOL_TYPE_TEXT,
#endif
				(void **)hook);
			if (result == B_OK) {
				init_accelerant ia;
				printf("Entry point %s() found\n", B_ACCELERANT_ENTRY_POINT);
				ia = (init_accelerant)(*hook)(B_INIT_ACCELERANT, NULL);
				printf("init_accelerant is 0x%08lx\n", (uint32)ia);
				if (ia && ((result = ia(fd)) == B_OK)) {
					// we have a winner!
					printf("Accelerant %s accepts the job!\n", path);
					break;
				} else {
					printf("init_accelerant refuses the the driver: %ld\n", result);
				}
			} else {
				printf("Couldn't find the entry point :-(\n");
			}
			// unload the accelerant, as we must be able to init!
			unload_add_on(image);
		}
		if (image < 0) printf("image failed to load with reason %.8lx (%s)\n", image, strerror(image));
		// mark failure to load image
		image = -1;
	}

	printf("Add-on image id: %ld\n", image);

done:
	return image;
}

static const char *spaceToString(uint32 cs) {
	const char *s;
	switch (cs) {
#define s2s(a) case a: s = #a ; break
		s2s(B_RGB32);
		s2s(B_RGBA32);
		s2s(B_RGB32_BIG);
		s2s(B_RGBA32_BIG);
		s2s(B_RGB16);
		s2s(B_RGB16_BIG);
		s2s(B_RGB15);
		s2s(B_RGBA15);
		s2s(B_RGB15_BIG);
		s2s(B_RGBA15_BIG);
		s2s(B_CMAP8);
		s2s(B_GRAY8);
		s2s(B_GRAY1);
		s2s(B_YCbCr422);
		s2s(B_YCbCr420);
		s2s(B_YUV422);
		s2s(B_YUV411);
		s2s(B_YUV9);
		s2s(B_YUV12);
		default:
			s = "unknown"; break;
#undef s2s
	}
	return s;
}

void dump_mode(display_mode *dm) {
	display_timing *t = &(dm->timing);
	printf("  pixel_clock: %ldKHz\n", t->pixel_clock);
	printf("            H: %4d %4d %4d %4d\n", t->h_display, t->h_sync_start, t->h_sync_end, t->h_total);
	printf("            V: %4d %4d %4d %4d\n", t->v_display, t->v_sync_start, t->v_sync_end, t->v_total);
	printf(" timing flags:");
	if (t->flags & B_BLANK_PEDESTAL) printf(" B_BLANK_PEDESTAL");
	if (t->flags & B_TIMING_INTERLACED) printf(" B_TIMING_INTERLACED");
	if (t->flags & B_POSITIVE_HSYNC) printf(" B_POSITIVE_HSYNC");
	if (t->flags & B_POSITIVE_VSYNC) printf(" B_POSITIVE_VSYNC");
	if (t->flags & B_SYNC_ON_GREEN) printf(" B_SYNC_ON_GREEN");
	if (!t->flags) printf(" (none)\n");
	else printf("\n");
	printf(" refresh rate: %4.2f\n", ((double)t->pixel_clock * 1000) / ((double)t->h_total * (double)t->v_total));
	printf("  color space: %s\n", spaceToString(dm->space));
	printf(" virtual size: %dx%d\n", dm->virtual_width, dm->virtual_height);
	printf("dispaly start: %d,%d\n", dm->h_display_start, dm->v_display_start);

	printf("   mode flags:");
	if (dm->flags & B_SCROLL) printf(" B_SCROLL");
	if (dm->flags & B_8_BIT_DAC) printf(" B_8_BIT_DAC");
	if (dm->flags & B_HARDWARE_CURSOR) printf(" B_HARDWARE_CURSOR");
	if (dm->flags & B_PARALLEL_ACCESS) printf(" B_PARALLEL_ACCESS");
//	if (dm->flags & B_SUPPORTS_OVERLAYS) printf(" B_SUPPORTS_OVERLAYS");
	if (!dm->flags) printf(" (none)\n");
	else printf("\n");
}

status_t get_and_set_mode(GetAccelerantHook gah, display_mode *dm) {

	accelerant_mode_count gmc;
	uint32 mode_count;
	get_mode_list gml;
	display_mode *mode_list, target, high, low;
	propose_display_mode pdm;
	status_t result = B_ERROR;
	set_display_mode sdm;

	/* find the propose mode hook */
	pdm = gah(B_PROPOSE_DISPLAY_MODE, NULL);
	if (!pdm) {
		printf("No B_PROPOSE_DISPLAY_MODE\n");
		goto exit0;
	}
	/* and the set mode hook */
	sdm = gah(B_SET_DISPLAY_MODE, NULL);
	if (!sdm) {
		printf("No B_SET_DISPLAY_MODE\n");
		goto exit0;
	}

	/* how many modes does the driver support */
	gmc = gah(B_ACCELERANT_MODE_COUNT, NULL);
	if (!gmc) {
		printf("No B_ACCELERANT_MODE_COUNT\n");
		goto exit0;
	}
	mode_count = gmc();
	printf("mode_count = %lu\n", mode_count);
	if (mode_count == 0) goto exit0;

	/* get a list of graphics modes from the driver */
	gml = gah(B_GET_MODE_LIST, NULL);
	if (!gml) {
		printf("No B_GET_MODE_LIST\n");
		goto exit0;
	}
	mode_list = (display_mode *)calloc(sizeof(display_mode), mode_count);
	if (!mode_list) {
		printf("Couldn't calloc() for mode list\n");
		goto exit0;
	}
	if (gml(mode_list) != B_OK) {
		printf("mode list retrieval failed\n");
		goto free_mode_list;
	}

	/* take the first mode in the list */
	target = high = low = *mode_list;
	/* make as tall a virtual height as possible */
	target.virtual_height = high.virtual_height = 0xffff;
	/* propose the display mode */
	if (pdm(&target, &low, &high) == B_ERROR) {
		printf("propose_display_mode failed\n");
		goto free_mode_list;
	}
	printf("Target display mode: \n");
	dump_mode(&target);
	/* we got a display mode, now set it */
	if (sdm(&target) == B_ERROR) {
		printf("set display mode failed\n");
		goto free_mode_list;
	}
	/* note the mode and success */
	*dm = target;
	result = B_OK;
	
free_mode_list:
	free(mode_list);
exit0:
	return result;
}

void get_frame_buffer(GetAccelerantHook gah, frame_buffer_config *fbc) {
	get_frame_buffer_config gfbc;
	gfbc = gah(B_GET_FRAME_BUFFER_CONFIG, NULL);
	gfbc(fbc);
}

sem_id get_sem(GetAccelerantHook gah) {
	accelerant_retrace_semaphore ars;
	ars = gah(B_ACCELERANT_RETRACE_SEMAPHORE, NULL);
	return ars();
}

void set_palette(GetAccelerantHook gah) {
	set_indexed_colors sic;
	sic = gah(B_SET_INDEXED_COLORS, NULL);
	if (sic) {
		/* booring grey ramp for now */
		uint8 map[3 * 256];
		uint8 *p = map;
		int i;
		for (i = 0; i < 256; i++) {
			*p++ = i;
			*p++ = i;
			*p++ = i;
		}
		sic(256, 0, map, 0);
	}
}

void paint_for_blit(display_mode *dm, frame_buffer_config *fbc) {
	switch (dm->space & ~0x3000) {
	case B_CMAP8: {
		int16 x, y;
		uint8 *fb = (uint8 *)fbc->frame_buffer;
		printf(" frame buffer is 8bpp\n");
		/* make a checkerboard pattern */
		for (y = 0; y < (dm->virtual_height >> 1); y++) {
			for (x = 0; x < (dm->virtual_width >> 1); x++) {
				fb[x] = 0;
			}
			for (; x < dm->virtual_width; x++) {
				fb[x] = 0xff;
			}
			fb += fbc->bytes_per_row;
		}
		for (; y < dm->virtual_height; y++) {
			for (x = 0; x < (dm->virtual_width >> 1); x++) {
				fb[x] = 0xff;
			}
			for (; x < dm->virtual_width; x++) {
				fb[x] = x;	// 0
			}
			fb += fbc->bytes_per_row;
		}
		fb = (uint8 *)(((uint8 *)fbc->frame_buffer) + fbc->bytes_per_row);
		fb += 1;
		for (y = 0; y < 40; y++) {
			for (x = 0; x < 40; x++) {
				fb[x] = 0x77;
			}
			fb = (uint8 *)(((uint8 *)fb) + fbc->bytes_per_row);
		}
		fb = (uint8 *)(((uint8 *)fbc->frame_buffer) + fbc->bytes_per_row * 11);
		fb += 11; 
		for (y = 0; y < 20; y++) {
			for (x = 0; x < 20; x++) {
				fb[x] = 0;
			}
			fb = (uint8 *)(((uint8 *)fb) + fbc->bytes_per_row);
		}
	} break;
	case B_RGB16_BIG:
	case B_RGB16_LITTLE: {
		int x, y;
		uint16 *fb = (uint16 *)fbc->frame_buffer;
		printf(" frame buffer is 16bpp\n");
		/* make a checkerboard pattern */
		for (y = 0; y < (dm->virtual_height >> 1); y++) {
			for (x = 0; x < (dm->virtual_width >> 1); x++) {
				fb[x] = 0;
			}
			for (; x < dm->virtual_width; x++) {
				fb[x] = 0xffff;
			}
			fb = (uint16 *)(((uint8 *)fb) + fbc->bytes_per_row);
		}
		for (; y < dm->virtual_height; y++) {
			for (x = 0; x < (dm->virtual_width >> 1); x++) {
				fb[x] = 0xffff;
			}
			for (; x < dm->virtual_width; x++) {
				fb[x] = 0;
			}
			fb = (uint16 *)((uint8 *)fb + fbc->bytes_per_row);
		}
		fb = (uint16 *)(((uint8 *)fbc->frame_buffer) + fbc->bytes_per_row);
		fb += 1;
		for (y = 0; y < 40; y++) {
			for (x = 0; x < 40; x++) {
				fb[x] = 0x7777;
			}
			fb = (uint16 *)(((uint8 *)fb) + fbc->bytes_per_row);
		}
		fb = (uint16 *)(((uint8 *)fbc->frame_buffer) + fbc->bytes_per_row * 11);
		fb += 11; 
		for (y = 0; y < 20; y++) {
			for (x = 0; x < 20; x++) {
				fb[x] = 0;
			}
			fb = (uint16 *)(((uint8 *)fb) + fbc->bytes_per_row);
		}
	} break;
	case B_RGB15_BIG:
	case B_RGBA15_BIG:
	case B_RGB15_LITTLE:
	case B_RGBA15_LITTLE: {
		int x, y;
		uint16 *fb = (uint16 *)fbc->frame_buffer;
		uint16 pixel;
		printf(" frame buffer is 15bpp\n");
		/* make a checkerboard pattern */
		for (y = 0; y < (dm->virtual_height >> 1); y++) {
			for (x = 0; x < (dm->virtual_width >> 1); x++) {
				fb[x] = 0;
			}
			for (; x < dm->virtual_width; x++) {
				fb[x] = 0x7fff;
			}
			fb = (uint16 *)(((uint8 *)fb) + fbc->bytes_per_row);
		}
		for (; y < dm->virtual_height; y++) {
			for (x = 0; x < (dm->virtual_width >> 1); x++) {
				fb[x] = 0x7fff;
			}
			for (; x < dm->virtual_width; x++) {
				fb[x] = 0;
			}
			fb = (uint16 *)((uint8 *)fb + fbc->bytes_per_row);
		}
		fb = (uint16 *)(((uint8 *)fbc->frame_buffer) + fbc->bytes_per_row);
		fb += 1;
		for (y = 0; y < 42; y++) {
			pixel = 0x7777;
			if (y != 40)
			for (x = 0; x < 42; x++) {
				if (x != 40) fb[x] = pixel += 0x0011;
			}
			fb = (uint16 *)(((uint8 *)fb) + fbc->bytes_per_row);
		}
		fb = (uint16 *)(((uint8 *)fbc->frame_buffer) + fbc->bytes_per_row * 11);
		fb += 11; 
		for (y = 0; y < 20; y++) {
			for (x = 0; x < 20; x++) {
				fb[x] = 0;
			}
			fb = (uint16 *)(((uint8 *)fb) + fbc->bytes_per_row);
		}
	} break;
	case B_RGB32_BIG:
	case B_RGBA32_BIG:
	case B_RGB32_LITTLE:
	case B_RGBA32_LITTLE: {
		int x, y;
		uint32 *fb = (uint32 *)fbc->frame_buffer;
		printf(" frame buffer is 32bpp\n");
		/* make a checkerboard pattern */
		for (y = 0; y < (dm->virtual_height >> 1); y++) {
			for (x = 0; x < (dm->virtual_width >> 1); x++) {
				fb[x] = 0;
			}
			for (; x < dm->virtual_width; x++) {
				fb[x] = 0xffffffff;
			}
			fb = (uint32 *)((uint8 *)fb + fbc->bytes_per_row);
		}
		for (; y < dm->virtual_height; y++) {
			for (x = 0; x < (dm->virtual_width >> 1); x++) {
				fb[x] = 0xffffffff;
			}
			for (; x < dm->virtual_width; x++) {
				fb[x] = 0;
			}
			fb = (uint32 *)((uint8 *)fb + fbc->bytes_per_row);
		}
		fb = (uint32 *)(((uint8 *)fbc->frame_buffer) + fbc->bytes_per_row);
		fb += 1;
		for (y = 0; y < 40; y++) {
			for (x = 0; x < 40; x++) {
				fb[x] = 0x77777777;
			}
			fb = (uint32 *)(((uint8 *)fb) + fbc->bytes_per_row);
		}
		fb = (uint32 *)(((uint8 *)fbc->frame_buffer) + fbc->bytes_per_row * 11);
		fb += 11; 
		for (y = 0; y < 20; y++) {
			for (x = 0; x < 20; x++) {
				fb[x] = 0;
			}
			fb = (uint32 *)(((uint8 *)fb) + fbc->bytes_per_row);
		}
	} break;
	default:
		printf("YIKES! frame buffer shape unknown!\n");
	}
}

status_t animate(GetAccelerantHook gah, display_mode *dm) {
	frame_buffer_config fbc;
	sem_id retrace = get_sem(gah);
	move_display_area mda = gah(B_MOVE_DISPLAY, NULL);
	int i;
	
	/* set the palette if in an 8bpp indexed mode */
	if (dm->space == B_CMAP8) set_palette(gah);
	/* find out about the frame buffer */
	get_frame_buffer(gah, &fbc);
	/* paint the display */
	paint_for_blit(dm, &fbc);
	/* wait a while */
	if (mda)
	for (i = 0; i < dm->virtual_height - dm->timing.v_display; i++) {
		mda(0, i);
		if (retrace >= 0) acquire_sem(retrace);
		else snooze(1000000 / 59);
	}
	else snooze(1000000 * 60);
	return B_OK;
}

int32 render_func(void *arg) {
	int fd;
	GetAccelerantHook gah;
	image_id image;
	uninit_accelerant ua;
	display_mode dm;

	/* find a graphic device to open */
	fd = pick_device("/dev/graphics");
	if (fd < 0) {
		printf("Can't open device: %s (%s)\n", strerror(fd), strerror(errno));
		return fd;
	}
	/* load the accelerant */
	image = load_accelerant(fd, &gah);
	if (image < 0) goto close_driver;

	/* get and set a display mode */
	if (get_and_set_mode(gah, &dm) != B_OK) goto close_accelerant;

	/* do the animation */
	animate(gah, &dm);

close_accelerant:
	/* shut down the accelerant */
	ua = gah(B_UNINIT_ACCELERANT, NULL);
	if (ua) ua();

	/* unload add-on */
	unload_add_on(image);

close_driver:
	/* close the driver */
	close(fd);
	return B_OK;
}

int main(int argc, char **argv)
{
	thread_id render;
	status_t result;

	/* give ourselves the standard file handles */
	init_file_handles();

	/* spawn the rendering thread */
	render = spawn_thread(render_func, "render", B_REAL_TIME_DISPLAY_PRIORITY, NULL);
	/* wait for the threads to finish */
	if (render && (resume_thread(render) == B_OK))
		wait_for_thread(render, &result);

	/* all done */
	return B_OK;
}
