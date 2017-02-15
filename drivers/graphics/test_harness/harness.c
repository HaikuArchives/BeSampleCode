/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include <FindDirectory.h>
#include <OS.h>
#include <image.h>
#include <graphic_driver.h>
#include <Accelerant.h>
#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <malloc.h>
#include <errno.h>

static uint8 my_hand_cursor_xor[] = {
#if 0
	0xf0, 0x0f,
	0x80, 0x01,
	0x80, 0x01,
	0x80, 0x01,
	0x00, 0x00,
	0x00, 0x00,
	0x0f, 0xf0,
	0x08, 0x10,
	0x08, 0x10,
	0x0f, 0xf0,
	0x00, 0x00,
	0x00, 0x00,
	0x80, 0x01,
	0x80, 0x01,
	0x80, 0x01,
	0xf0, 0x0f
#else
0x0,0x0,0x0,0x0,0x38,0x0,0x24,0x0,0x24,0x0,0x13,0xe0,0x12,0x5c,0x9,
0x2a,0x8,0x1,0x3c,0x1,0x4c,0x1,0x42,0x1,0x30,0x1,0xc,0x1,0x2,0x0,
0x1,0x0
#endif

};
static uint8 my_hand_cursor_and[] = {
#if 0
	0xf0, 0x0f,
	0x80, 0x01,
	0x80, 0x01,
	0x80, 0x01,
	0x00, 0x00,
	0x00, 0x00,
	0x0f, 0xf0,
	0x08, 0x10,
	0x08, 0x10,
	0x0f, 0xf0,
	0x00, 0x00,
	0x00, 0x00,
	0x80, 0x01,
	0x80, 0x01,
	0x80, 0x01,
	0xf0, 0x0f
#else
0x0,0x0,0x0,0x0,0x38,0x0,0x3c,0x0,0x3c,0x0,0x1f,0xe0,0x1f,0xfc,0xf,
0xfe,0xf,0xff,0x3f,0xff,0x7f,0xff,0x7f,0xff,0x3f,0xff,0xf,0xff,0x3,0xfe,
0x1,0xf8
#endif
};

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

int pick_from_list(const char *msg, char **list) {
	int count = 1;
	int choice;
	puts(msg);
	while (*list) {
		printf(" %2.0d) %s\n", count, *list);
		list++;
		count++;
	}
	while (1) {
		printf("Enter [1-%d]: ", count-1); fflush(stdout);
		scanf("%d", &choice);
		if ((choice >= 0) && (choice < count)) break;
	}
	return choice - 1;
}

/* return a file descriptor of the desired device, or -1 on failure */
int pick_device(const char *apath) {
	int count = 0;
	int choice;
	DIR				*d;
	struct dirent	*e;
	char
		name_buf[1024],
		message[256],
		*names[16],
		*name = name_buf;

	/* open directory apath */
	d = opendir(apath);
	if (!d) return B_ERROR;
	/* get a list of devices, filtering out . and .. */
	while ((e = readdir(d)) != NULL) {
		if (!strcmp(e->d_name, ".") || !strcmp(e->d_name, ".."))
			continue;
		strcpy(name, e->d_name);
		names[count++] = name;
		name += strlen(name)+1;
	}
	closedir(d);
	names[count] = NULL;		
	/* if there are no devices, return error */
	if (count == 0) return B_ERROR;
	/* prompt user for which device */
	sprintf(message, "Choose from these devices found in %s:", apath);
	choice = pick_from_list(message, names);
	if (choice >= 0) {
		sprintf(message, "%s/%s", apath, names[choice]);
		return open(message, B_READ_WRITE);
	}
	return B_ERROR;
}

image_id load_accelerant(int fd, GetAccelerantHook *hook) {
	status_t result;
	image_id image;
	int i;
	char
		signature[1024],
		path[PATH_MAX];
	struct stat st;
	const static directory_which vols[] = {
		B_USER_ADDONS_DIRECTORY,
		B_COMMON_ADDONS_DIRECTORY,
		B_BEOS_ADDONS_DIRECTORY
	};

	/* get signature from driver */
	result = ioctl(fd, B_GET_ACCELERANT_SIGNATURE, &signature, sizeof(signature));
	if (result != B_OK) goto done;
	printf("B_GET_ACCELERANT_SIGNATURE returned ->%s<-\n", signature);

	// note failure by default
	image = -1;
	for(i=0; i < sizeof (vols) / sizeof (vols[0]); i++) {

		/* ---
			compute directory path to common or beos addon directory on
			floppy or boot volume
		--- */

		if (find_directory (vols[i], -1, false, path, PATH_MAX) != B_OK)
			continue;

		strcat (path, "/accelerants/");
		strcat (path, signature);

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
		if (image < 0) printf("image failed to load :-(\n");
		// mark failure to load image
		image = -1;
	}

	printf("Add-on image id: %ld\n", image);

done:
	return image;
}

typedef struct {
		uint32	opcode;
		char	*command_name;
} command;
#define MK_CMD(x) { x, #x }

enum {
	T_PICK_A_MODE = B_ACCELERANT_PRIVATE_START+10000
};

static command commands[] = {
	/* mode configuration */
	MK_CMD(B_ACCELERANT_MODE_COUNT),			/* required */
	MK_CMD(B_GET_MODE_LIST),			/* required */
	MK_CMD(T_PICK_A_MODE),
	MK_CMD(B_PROPOSE_DISPLAY_MODE),	/* optional */
	MK_CMD(B_SET_DISPLAY_MODE),		/* required */	
	MK_CMD(B_GET_DISPLAY_MODE),		/* required */
	MK_CMD(B_GET_FRAME_BUFFER_CONFIG),	/* required */
	MK_CMD(B_GET_PIXEL_CLOCK_LIMITS),	/* required */
	MK_CMD(B_MOVE_DISPLAY),				/* optional */
	MK_CMD(B_SET_INDEXED_COLORS),		/* required if driver supports 8bit indexed modes */
	MK_CMD(B_ACCELERANT_RETRACE_SEMAPHORE),
	MK_CMD(B_SET_DPMS_MODE),			/* optional */

	/* cursor managment */
	MK_CMD(B_MOVE_CURSOR),				/* optional */
	MK_CMD(B_SET_CURSOR_SHAPE),			/* optional */
	MK_CMD(B_SHOW_CURSOR),				/* optional */

	/* synchronization */
	MK_CMD(B_ACQUIRE_ENGINE),
	MK_CMD(B_RELEASE_ENGINE),
	MK_CMD(B_WAIT_ENGINE_IDLE),

	/* 2D acceleration */
	MK_CMD(B_SCREEN_TO_SCREEN_BLIT),
	MK_CMD(B_FILL_RECTANGLE),
	MK_CMD(B_INVERT_RECTANGLE),
	MK_CMD(B_FILL_SPAN)
};
#define CMD_SIZE (sizeof(commands) / sizeof(command))

void missing_feature(char *s) {
	printf("Accelerant doesn't implement required feature %s\n", s);
}

void missing_option(char *s) {
	printf("Accelerant doesn't implement optional feature %s\n", s);
}

void failed_with_reason(char *s, status_t r) {
	printf("%s failed with reason %ld (0x%08lx)\n", s, r, r);
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
	if (!dm->flags) printf(" (none)\n");
	else printf("\n");
}

display_mode * pick_a_mode(display_mode *mode_list, uint32 mode_count) {
	char **modes = (char **)calloc(sizeof(char *),mode_count+1);
	char buffer[128];
	int mode;
	display_mode *dm = NULL;
	if (!modes) {
		printf("Couldn't allocate enough RAM for pick-a-mode list.\n");
		goto done;
	}
	for (mode = 0; mode < mode_count; mode++) {
		display_mode *dm = mode_list + mode;
		sprintf(buffer, "%dx%d@%ldKHz (%dx%d virtual) %s", dm->timing.h_display, dm->timing.v_display, dm->timing.pixel_clock,
			dm->virtual_width, dm->virtual_height, spaceToString(dm->space));
		modes[mode] = strdup(buffer);
	}
	modes[mode] = NULL;
	mode = pick_from_list("Select a display mode:", modes);
	if (mode >= 0) {
		dm = mode_list + mode;
		dump_mode(dm);
	}
	for (mode = 0; mode < mode_count; mode++)
		free(modes[mode]);
	free(modes);
done:
	return dm;
}

void exercise_driver(GetAccelerantHook gah) {
	/* make list of commands for picker */
	char **cmds = (char **)calloc(sizeof(char *), 1 + CMD_SIZE);
	int i;
	status_t
		result;
	uint32
		mode_count = 0;
	display_mode
		dm,
		*mode_list = NULL;
	engine_token
		*et;
	bool
		valid_mode = FALSE,
		engine_acquired = FALSE;

	if (!cmds) {
		printf("Couldn't allocate memory for command name list!\n");
		return;
	}
	for (i = 0; i < CMD_SIZE; i++)
		cmds[i] = commands[i].command_name;
	cmds[i] = NULL;
	while ((i = pick_from_list("Pick a command:", cmds)) >= 0) {
		switch (commands[i].opcode) {
			case B_ACCELERANT_MODE_COUNT: {
				accelerant_mode_count gmc = gah(B_ACCELERANT_MODE_COUNT, NULL);
				if (!gmc) {
					missing_feature(cmds[i]);
					break;
				}
				mode_count = gmc();
				printf("Device supports %ld modes\n", mode_count);
			} break;
			case B_GET_MODE_LIST: {
				get_mode_list gml = gah(B_GET_MODE_LIST, NULL);
				if (!gml) {
					missing_feature(cmds[i]);
					break;
				}
				if (mode_count == 0) {
					printf("Do B_ACCELERANT_MODE_COUNT first!\n");
					break;
				}
				if (mode_list) free(mode_list);
				mode_list = (display_mode *)calloc(sizeof(display_mode), mode_count);
				if (!mode_list) {
					printf("Couldn't allocate enough RAM for display_mode list.\n");
					break;
				}
				result = gml(mode_list);
				printf("Mode list retrieved with result %ld\n", result);
			} break;
			case T_PICK_A_MODE: {
				if (mode_list) {
					display_mode *adm = pick_a_mode(mode_list, mode_count);
					if (adm) {
						dm = *adm;
						valid_mode = TRUE;
					}
				} else printf("Do B_GET_MODE_LIST first!\n");
			} break;
			case B_SET_DISPLAY_MODE: {
				set_display_mode sdc = gah(B_SET_DISPLAY_MODE, NULL);
				if (!sdc) {
					missing_feature(cmds[i]);
					break;
				}
				if (!valid_mode) {
					printf("Do T_PICK_A_MODE or B_PROPOSE_DISPLAY_MODE first!\n");
					break;
				}
				result = sdc(&dm);
				printf("set_display_mode() completed with result %ld\n", result);
			} break;
			case B_GET_DISPLAY_MODE: {
				get_display_mode gdc = gah(B_GET_DISPLAY_MODE, NULL);
				if (!gdc) {
					missing_feature(cmds[i]);
					break;
				}
				result = gdc(&dm);
				if (result != B_OK) {
					printf("get_display_mode() failed: %ld (0x%08lx)\n", result, result);
					break;
				}
				printf("get_display_mode() suceeded!\n");
				valid_mode = TRUE;
				dump_mode(&dm);
			} break;
			case B_GET_FRAME_BUFFER_CONFIG: {
				frame_buffer_config fbc;
				get_frame_buffer_config gfbc = gah(B_GET_FRAME_BUFFER_CONFIG, NULL);
				if (!gfbc) {
					missing_feature(cmds[i]);
					break;
				}
				if (!valid_mode) {
					printf("Do T_PICK_A_MODE or B_PROPOSE_DISPLAY_MODE first!\n");
					break;
				}
				result = gfbc(&fbc);
				if (result != B_OK) {
					printf("get_frame_buffer_config() failed: %ld (0x%08lx)\n", result, result);
					break;
				}
				printf("get_frame_buffer_config() suceeded!\n");
				printf(" frame buffer: 0x%08lx\n", (uint32)fbc.frame_buffer);
				printf("   dma buffer: 0x%08lx\n", (uint32)fbc.frame_buffer_dma);
				printf("bytes per row: %ld\n\n", fbc.bytes_per_row);
				switch (dm.space & ~0x3000) {
				case B_CMAP8: {
					int16 x, y;
					uint8 *fb = (uint8 *)fbc.frame_buffer;
					printf(" frame buffer is 8bpp\n");
					/* make a checkerboard pattern */
					for (y = 0; y < (dm.virtual_height >> 1); y++) {
						for (x = 0; x < (dm.virtual_width >> 1); x++) {
							fb[x] = 0;
						}
						for (; x < dm.virtual_width; x++) {
							fb[x] = 0xff;
						}
						fb += fbc.bytes_per_row;
					}
					for (; y < dm.virtual_height; y++) {
						for (x = 0; x < (dm.virtual_width >> 1); x++) {
							fb[x] = 0xff;
						}
						for (; x < dm.virtual_width; x++) {
							fb[x] = 0;
						}
						fb += fbc.bytes_per_row;
					}
				} break;
				case B_RGB16_BIG:
				case B_RGB16_LITTLE:
				case B_RGB15_BIG:
				case B_RGBA15_BIG:
				case B_RGB15_LITTLE:
				case B_RGBA15_LITTLE: {
					int x, y;
					uint16 *fb = (uint16 *)fbc.frame_buffer;
					printf(" frame buffer is 16bpp\n");
					/* make a checkerboard pattern */
					for (y = 0; y < (dm.virtual_height >> 1); y++) {
						for (x = 0; x < (dm.virtual_width >> 1); x++) {
							fb[x] = 0;
						}
						for (; x < dm.virtual_width; x++) {
							fb[x] = 0xffff;
						}
						fb = (uint16 *)(((uint8 *)fb) + fbc.bytes_per_row);
					}
					for (; y < dm.virtual_height; y++) {
						for (x = 0; x < (dm.virtual_width >> 1); x++) {
							fb[x] = 0xffff;
						}
						for (; x < dm.virtual_width; x++) {
							fb[x] = 0;
						}
						fb = (uint16 *)((uint8 *)fb + fbc.bytes_per_row);
					}
				} break;
				case B_RGB32_BIG:
				case B_RGBA32_BIG:
				case B_RGB32_LITTLE:
				case B_RGBA32_LITTLE: {
					int x, y;
					uint32 *fb = (uint32 *)fbc.frame_buffer;
					printf(" frame buffer is 32bpp\n");
					/* make a checkerboard pattern */
					for (y = 0; y < (dm.virtual_height >> 1); y++) {
						for (x = 0; x < (dm.virtual_width >> 1); x++) {
							fb[x] = 0;
						}
						for (; x < dm.virtual_width; x++) {
							fb[x] = 0xffffffff;
						}
						fb = (uint32 *)((uint8 *)fb + fbc.bytes_per_row);
					}
					for (; y < dm.virtual_height; y++) {
						for (x = 0; x < (dm.virtual_width >> 1); x++) {
							fb[x] = 0xffffffff;
						}
						for (; x < dm.virtual_width; x++) {
							fb[x] = 0;
						}
						fb = (uint32 *)((uint8 *)fb + fbc.bytes_per_row);
					}
				} break;
				default:
					printf("YIKES! frame buffer shape unknown!\n");
				}
			} break;
			case B_GET_PIXEL_CLOCK_LIMITS: {
				get_pixel_clock_limits gpcl = gah(B_GET_PIXEL_CLOCK_LIMITS, NULL);
				uint32 low, high;
				if (!gpcl) {
					missing_feature(cmds[i]);
					break;
				}
				result = gpcl(&dm, &low, &high);
				if (result != B_OK) {
					printf("get_pixel_clock_limits() failed: %ld (0x%08lx)\n", result, result);
					break;
				}
				printf("Minimum pixel clock: %lu\nMaximum pixel clock: %lu\n", low, high);
			} break;
			case B_MOVE_DISPLAY: {
				move_display_area mda = gah(B_MOVE_DISPLAY, NULL);	/* should we be passing the display mode? */
				int top, left;
				if (mda) {
					printf("Enter new coordinates: "); fflush(stdout);
					scanf("%d,%d", &left, &top);
					/* no range checking for now */
					result = mda(left, top);
					printf("move_display_area() returned %ld (0x%08lx)\n", result, result);
				} else {
					missing_option(cmds[i]);
				}
			} break;
			case B_SET_INDEXED_COLORS: {
				set_indexed_colors sic = gah(B_SET_INDEXED_COLORS, NULL); /* should we be passing display mode */
				if (sic) {
					char *list[] = {"Red CLUT", "Green CLUT", "Blue CLUT", "Grey CLUT", "Be CLUT", NULL};
					int choice;
					choice = pick_from_list("Set cursor visibility:", list);
					if (choice >= 0) {
						uint8 color_data[256 * 3], *cd = color_data;
						int j, k;
						for (j = 0; j < 256; j++) {
							for (k = 0; k < 3; k++) {
								*cd = 0;
								if ((k == choice) || (choice == 3)) *cd = (uint8)j;
								cd++;
							}
						}
						sic(256, 0, color_data, 0);
					}
				} else {
					missing_option(cmds[i]);
				}
			} break;
			case B_ACCELERANT_RETRACE_SEMAPHORE: {
				accelerant_retrace_semaphore ars = gah(B_ACCELERANT_RETRACE_SEMAPHORE, NULL);
				sem_id sid;
				status_t result;
				if (!ars) {
					missing_feature(cmds[i]);
					break;
				}
				sid = ars();
				if (sid < B_OK) {
					printf("Bad semaphore ID returned: %ld\n", sid);
					break;
				}
				if (!valid_mode) {
					printf("No video mode selected, skipping retrace check.\n");
				}
				result = acquire_sem_etc(sid, 1, B_TIMEOUT, 1000000L);
				if (result != B_OK) {
					printf("Semaphore acquisition failed: %ld (0x%08lx)\n", result, result);
					break;
				}
				printf("Retrace semaphore acquired!\n");
			} break;
			case B_SET_CURSOR_SHAPE: {
				if (dm.flags & B_HARDWARE_CURSOR) {
					set_cursor_shape scs = gah(B_SET_CURSOR_SHAPE, NULL);
					if (!scs) {
						missing_option(cmds[i]);
						break;
					}
					{
					int i;
					uint8 hand_and[32];
					for (i = 0; i < 32; i++) hand_and[i] = ~my_hand_cursor_and[i];
					result = scs(16, 16, 2, 2, hand_and, my_hand_cursor_xor);
					}
					if (result != B_OK)
						failed_with_reason("B_SET_CURSOR_SHAPE", result);
				} else printf("Mode does not support hardware cursor!\n");
			} break;
			case B_MOVE_CURSOR: {
				if (dm.flags & B_HARDWARE_CURSOR) {
					move_cursor mc = gah(B_MOVE_CURSOR, NULL);
					if (!mc) {
						missing_option(cmds[i]);
						break;
					}
					mc((uint16)(dm.timing.h_display >> 1), (uint16)(dm.timing.v_display >> 1));
				} else printf("Mode does not support hardware cursor!\n");
			} break;
			case B_SHOW_CURSOR: {
				if (dm.flags & B_HARDWARE_CURSOR) {
					show_cursor sc = gah(B_SHOW_CURSOR, NULL);
					char *list[] = {"Hide Cursor", "Show Cursor", NULL};
					int choice;
					if (!sc) {
						missing_option(cmds[i]);
						break;
					}
					choice = pick_from_list("Set cursor visibility:", list);
					if (choice >= 0) sc(choice == 1);
				} else printf("Mode does not support hardware cursor!\n");
			} break;
			case B_ACQUIRE_ENGINE: {
				acquire_engine ae = gah(B_ACQUIRE_ENGINE, NULL);
				if (!ae) {
					missing_feature(cmds[i]);
					break;
				}
				result = ae(0,0, NULL, &et);
				if (result != B_OK) {
					failed_with_reason("acquire_engine()", result);
					et = NULL;
					break;
				}
				engine_acquired = TRUE;
				printf("acceleration engine acquired: 0x%08lx\n", (uint32)et);
			} break;
			case B_RELEASE_ENGINE: {
				release_engine re = gah(B_RELEASE_ENGINE, NULL);
				if (!re) {
					missing_feature(cmds[i]);
					break;
				}
				if (!engine_acquired) {
					printf("Do B_ACQUIRE_ENGINE first!\n");
					break;
				}
				result = re(et, NULL);
				if (result != B_OK) {
					failed_with_reason("release_engine()", result);
					break;
				}
				engine_acquired = FALSE;
				printf("acceleration engine released.\n");
			} break;
			case B_WAIT_ENGINE_IDLE: {
				wait_engine_idle wei = gah(B_WAIT_ENGINE_IDLE, NULL);
				if (!wei) {
					missing_feature(cmds[i]);
					break;
				}
				if (!engine_acquired) {
					printf("Do B_ACQUIRE_ENGINE first!\n");
					break;
				}
				wei();
				printf("Acceleration engine idle.\n");
			} break;
			case B_FILL_RECTANGLE: {
				fill_rectangle rf = gah(B_FILL_RECTANGLE, &dm);
				fill_rect_params rfp[16];
				int j;
				if (!rf) {
					missing_option(cmds[i]);
					break;
				}
				if (!engine_acquired) {
					printf("Do B_ACQUIRE_ENGINE first!\n");
					break;
				}
				rfp[0].left = rfp[0].top = 0;
				rfp[0].right = dm.timing.h_display - 1;
				rfp[0].bottom = dm.timing.v_display - 1;
				rf(et, 0, rfp, 1);
				for (j = 0; ((dm.timing.h_display-1) >> (j+1)) && ((dm.timing.v_display-1) >> (j+1)); j++) {
					rfp[j].right = (dm.timing.h_display - 1) >> j;
					rfp[j].left = (dm.timing.h_display - 1) >> (j+1);
					rfp[j].bottom = (dm.timing.v_display - 1) >> j;
					rfp[j].top = (dm.timing.v_display - 1) >> (j+1);
					printf("   rfp[%d] = %d,%d to %d,%d\n", j, rfp[j].left, rfp[j].top, rfp[j].right, rfp[j].bottom);
				}
				rf(et, 0xffffffff, rfp, (uint32)j);
			} break;
			case B_INVERT_RECTANGLE: {
				uint16 left, top, right, bottom, h_delta, v_delta;
				fill_rect_params rfp[16];
				int j;
				invert_rectangle ri = gah(B_INVERT_RECTANGLE, &dm);
				if (!ri) {
					missing_option(cmds[i]);
					break;
				}
				if (!engine_acquired) {
					printf("Do B_ACQUIRE_ENGINE first!\n");
					break;
				}
				left = 0; right = dm.timing.h_display - 1;
				top = 0; bottom = dm.timing.v_display - 1;
				h_delta = dm.timing.h_display >> 4;
				v_delta = dm.timing.v_display >> 4;
				for (j = 0; j < 8; j++) {
					rfp[j].left = left;
					rfp[j].top = top;
					rfp[j].right = right;
					rfp[j].bottom = bottom;
					left += h_delta;
					right -= h_delta;
					top += v_delta;
					bottom -= v_delta;
				}
				ri(et, rfp, 8);
			} break;
			case B_FILL_SPAN: {
				fill_span sf = gah(B_FILL_SPAN, &dm);
				uint16 *spans, *sp;
				int j, k, w;
				if (!sf) {
					missing_option(cmds[i]);
					break;
				}
				if (!engine_acquired) {
					printf("Do B_ACQUIRE_ENGINE first!\n");
					break;
				}
				sp = spans = (uint16*)calloc(dm.timing.v_display * 3, sizeof(uint16));
				if (!spans) {
					printf("Couldn't allocate RAM for span list :-(\n");
					break;
				}
				for (j = dm.timing.v_display, k = 0, w = dm.timing.h_total >> 1; j > 0; j--) {
					*sp++ = j-1;
					*sp++ = k;
					*sp++ = k + w;
					if (j > dm.timing.v_display / 2) k++;
					else k--;
					if (j & 1) w--;
				}
				sf(et, 0x55555555, spans, dm.timing.v_display);
				free(spans);
			} break;
			default: {
				printf("Ooops: %s not implemented yet.\n", cmds[i]);
			} break;
		}
	}
	/* clean up */
	if (cmds) free(cmds);
	if (mode_list) free(mode_list);
}

#define T_GRAPHICS_DEVICE_DIR "/dev/graphics"
int main(int argc, char **argv) {
	int fd;
	GetAccelerantHook gah;
	image_id image;

	/* pick a device */
	fd = pick_device(T_GRAPHICS_DEVICE_DIR);
	if (fd < 0) {
		fprintf(stderr, "Can't open device: %s (%s)\n", strerror(fd), strerror(errno));
		return fd;
	}
	/* load the accelerant */
	image = load_accelerant(fd, &gah);
	if (image < 0) goto close_driver;

	exercise_driver(gah);

	/* uninit accelerant */
	{
	uninit_accelerant ua = gah(B_UNINIT_ACCELERANT, NULL);
	if (ua) ua();
	}
	/* unload add-on */
	unload_add_on(image);	
close_driver:
	/* close the driver */
	close(fd);
	return B_OK;
}
