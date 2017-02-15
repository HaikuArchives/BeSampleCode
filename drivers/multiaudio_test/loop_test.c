/*
	loop_test.c
	Tests the multi-channel audio driver API.

	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include <stdio.h>
#include <OS.h>
#include "multi_audio.h"

#define MAX_CHANNELS	32

char dev_name_1[] = "/dev/audio/multi/sonorus/1";
char dev_name_2[] = "/dev/audio/multi/gina/1";

//
// Waste some non-stack memory
//
buffer_desc		play_buffer_list0[MAX_CHANNELS];
buffer_desc		play_buffer_list1[MAX_CHANNELS];
buffer_desc 	*play_buffer_desc[2] = { play_buffer_list0, play_buffer_list1 };
buffer_desc		record_buffer_list0[MAX_CHANNELS];
buffer_desc		record_buffer_list1[MAX_CHANNELS];
buffer_desc 	*record_buffer_desc[2] = { record_buffer_list0, record_buffer_list1 };

int main(int nargs,char *args[])
{
	int 	driver_handle,half_buffer_size;
	int32	*pPlay[2][MAX_CHANNELS];
	int32	*pRecord[2][MAX_CHANNELS];	
	int 	rval,i,j,num_outputs,num_inputs,num_channels;
	int 	is_gina = 0;
	uint32 	cntr = 0;

	char dev_name[256];
	multi_description		MD;
	multi_channel_info		MCI[MAX_CHANNELS];
	multi_buffer_list 		MBL;
	multi_buffer_info		MBI;
	multi_channel_enable 	MCE;
	uint32					mce_enable_bits;
	multi_format_info 		MFI;

	//
	// "process" cl vars
	//
	i = nargs;
	while(i--)
	{
		if (*args[i] == '-')
		{
			switch (args[i][1])
			{
				case 'g':
				case 'G':
					is_gina = 1;
					break;
				default:
				 	break;					
			}
		}
	}

	//
	// Open the driver
	//
	if(is_gina)
	{
		strcpy(dev_name, dev_name_2);
	}
	else
	{	
		strcpy(dev_name, dev_name_1);
	}
	driver_handle = open(dev_name,O_RDWR);
	if (-1 == driver_handle)
	{
		printf("Failed to open %s\n",dev_name);
		return -1;
	}
	printf("\nOpened %s\n",dev_name);
	
	//
	// Get description
	//
	MD.info_size = sizeof(MD);
	MD.request_channel_count = MAX_CHANNELS;
	MD.channels = MCI;
	rval = ioctl(driver_handle,B_MULTI_GET_DESCRIPTION,&MD,0);
	if (B_OK != rval)
	{
		printf("Failed on B_MULTI_GET_DESCRIPTION\n");
		goto exit;
	}

	printf("Friendly name:\t%s\nVendor:\t\t%s\n",
				MD.friendly_name,MD.vendor_info);
	printf("%ld outputs\t%ld inputs\n%ld out busses\t%ld in busses\n",
				MD.output_channel_count,MD.input_channel_count,
				MD.output_bus_channel_count,MD.input_bus_channel_count);
	printf("\nChannels\n"
			 "ID\tKind\tDesig\tConnectors\n");

	for (i = 0 ; i < (MD.output_channel_count + MD.input_channel_count); i++)
	{
		printf("%ld\t%d\t%lu\t0x%lx\n",MD.channels[i].channel_id,
											MD.channels[i].kind,
											MD.channels[i].designations,
											MD.channels[i].connectors);
	}			 
	printf("\n");
	
	printf("Output rates\t\t0x%lx\n",MD.output_rates);
	printf("Input rates\t\t0x%lx\n",MD.input_rates);
	printf("Max CVSR\t\t%.0f\n",MD.max_cvsr_rate);
	printf("Min CVSR\t\t%.0f\n",MD.min_cvsr_rate);
	printf("Output formats\t\t0x%lx\n",MD.output_formats);
	printf("Input formats\t\t0x%lx\n",MD.input_formats);
	printf("Lock sources\t\t0x%lx\n",MD.lock_sources);
	printf("Timecode sources\t0x%lx\n",MD.timecode_sources);
	printf("Interface flags\t\t0x%lx\n",MD.interface_flags);
	printf("Control panel string:\t\t%s\n",MD.control_panel);
	printf("\n");
	
	num_outputs = MD.output_channel_count;
	num_inputs = MD.input_channel_count;
	num_channels = num_outputs + num_inputs;
	
	// Get and set enabled buffers
	MCE.info_size = sizeof(MCE);
	MCE.enable_bits = (uchar *) &mce_enable_bits;
	rval = ioctl(driver_handle,B_MULTI_GET_ENABLED_CHANNELS,&MCE,sizeof(MCE));
	if (B_OK != rval)
	{
		printf("Failed on B_MULTI_GET_ENABLED_CHANNELS len is 0x%x\n",sizeof(MCE));
		goto exit;
	}
	
	mce_enable_bits = (1 << num_channels) - 1;
	MCE.lock_source = B_MULTI_LOCK_INTERNAL;
	rval = ioctl(driver_handle,B_MULTI_SET_ENABLED_CHANNELS,&MCE,0);
	if (B_OK != rval)
	{
		printf("Failed on B_MULTI_SET_ENABLED_CHANNELS 0x%x 0x%x\n", MCE.enable_bits, *(MCE.enable_bits));
		goto exit;
	}
	
	//
	// Set the sample rate
	//
	MFI.info_size = sizeof(MFI);
	MFI.output.rate = B_SR_44100;
	MFI.output.cvsr = 0;
	MFI.output.format = B_FMT_24BIT;
	MFI.input.rate = MFI.output.rate;
	MFI.input.cvsr = MFI.output.cvsr;
	MFI.input.format = MFI.output.format;
	rval = ioctl(driver_handle,B_MULTI_SET_GLOBAL_FORMAT,&MFI,0);
	if (B_OK != rval)
	{
		printf("Failed on B_MULTI_SET_GLOBAL_FORMAT\n");
		goto exit;
	}
	
	//
	// Do the get buffer ioctl
	//
	MBL.info_size = sizeof(MBL);
	MBL.request_playback_buffer_size = 0;           //use the default......
	MBL.request_playback_buffers = 2;
	MBL.request_playback_channels = num_outputs;
	MBL.playback_buffers = (buffer_desc **) play_buffer_desc;	
	MBL.request_record_buffer_size = 0;           //use the default......
	MBL.request_record_buffers = 2;
	MBL.request_record_channels = num_inputs;
	MBL.record_buffers = (buffer_desc **) record_buffer_desc;		
	rval = ioctl(driver_handle, B_MULTI_GET_BUFFERS, &MBL, 0);

	if (B_OK != rval)
	{
		printf("Failed on B_MULTI_GET_BUFFERS\n");
		goto exit;
	}
	
	for (i = 0; i<2; i++)
	{
		for (j=0; j < num_outputs; j++)
		{
			pPlay[i][j] = (int32 *) MBL.playback_buffers[i][j].base;
			memset(pPlay[i][j],0,MBL.return_playback_buffer_size * 4 /*samps to bytes*/);
			printf("Play buffers[%d][%d]: %p\n",i,j,pPlay[i][j]);
		}
	}	
	for (i = 0; i<2; i++)
	{
		for (j=0; j < num_inputs; j++)
		{
			pRecord[i][j] = (int32 *) MBL.record_buffers[i][j].base;
			printf("Record buffers[%d][%d]: %p\n",i,j,pRecord[i][j]);			
		}
	}	
	half_buffer_size = MBL.return_playback_buffer_size ;
	printf("\n");
	printf("half buffer size is 0x%x\n", half_buffer_size);
	MBI.info_size = sizeof(MBI);
 	ioctl(driver_handle, B_MULTI_BUFFER_EXCHANGE, &MBI, 0);
	ioctl(driver_handle, B_MULTI_BUFFER_EXCHANGE, &MBI, 0);	

	//
	// Do it. 
	//
	printf("Now looping (ctl c to quit)......)\n");
	set_thread_priority(find_thread(NULL), B_REAL_TIME_PRIORITY);

	while (1)
	{
		for ( i = 0; i < half_buffer_size; i ++ )
		{
			pPlay[cntr&1][0][i] = pRecord[cntr&1][0][i];
			pPlay[cntr&1][1][i] = pRecord[cntr&1][1][i];
			pPlay[cntr&1][2][i] = pRecord[cntr&1][2][i];
			pPlay[cntr&1][3][i] = pRecord[cntr&1][3][i];
			if (!is_gina)
			{
				pPlay[cntr&1][4][i] = pRecord[cntr&1][4][i];
				pPlay[cntr&1][5][i] = pRecord[cntr&1][5][i];
				pPlay[cntr&1][6][i] = pRecord[cntr&1][6][i];
				pPlay[cntr&1][7][i] = pRecord[cntr&1][7][i];
				pPlay[cntr&1][8][i] = pRecord[cntr&1][8][i];
				pPlay[cntr&1][9][i] = pRecord[cntr&1][9][i];
			}
			else
			{
				pPlay[cntr&1][4][i] = pRecord[cntr&1][0][i];
				pPlay[cntr&1][5][i] = pRecord[cntr&1][1][i];
				pPlay[cntr&1][6][i] = pRecord[cntr&1][2][i];
				pPlay[cntr&1][7][i] = pRecord[cntr&1][3][i];
				pPlay[cntr&1][8][i] = pRecord[cntr&1][0][i];
				pPlay[cntr&1][9][i] = pRecord[cntr&1][1][i];
			}
		}
		ioctl(driver_handle, B_MULTI_BUFFER_EXCHANGE, &MBI, 0);
		cntr++;
	}

exit:	
	close(driver_handle);
	return 0;
}
