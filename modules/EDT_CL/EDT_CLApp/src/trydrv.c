#define NO_STRDUP
#define NO_STRCASECMP
#define NO_MAIN

#include <edtinc.h>
#include <libpdv.h>

unsigned char * pimage;

extern int initcam(int unit, int channel, char * cfgname);

char * config_name_freerun="ptm6710cl.cfg";
char * config_name_pw="ptm6710cl_pw.cfg";
/*============================================*/
int do_speed_test(int unit, int channel, int mode)
{
	int err;
	PdvDev* h=0;
	int image_size=0;
	int dma_size=0;
	unsigned char* buffer_ptrs[8];
	int const num_of_buffers=sizeof(buffer_ptrs)/sizeof(buffer_ptrs[0]);
	int loop_count=0;

	/* char initcam_command[32];
	unsigned char* all_buffers;
	int devid ;
	int i;  */ /* unused */
	int loops ;

        epicsTimeStamp start_time;

	edtInstall(unit);
	/* set up test for cameralink or dvk for example to customer */
	printf("initcam(%d, %d, %s)\n",unit, channel, mode?config_name_pw:config_name_freerun);
	if(mode)
            initcam(unit,channel,config_name_pw);
        else
            initcam(unit,channel,config_name_freerun);
	h=pdv_open_channel("pdv",unit,channel);
        if (h==0) return printf("pdv_open_channel() failed\n");
        else printf("pdv_open_channel() succeesed\n");

        pdv_set_timeout(h,0);
        if(mode) pdv_enable_external_trigger(h, PDV_PHOTO_TRIGGER); /* this is only needed for external trigger mode */

	dma_size=pdv_get_dmasize(h);
	image_size=pdv_get_imagesize(h);
	printf("image_size=%i: %ix%ix%i, dma_size=%i\n",image_size,pdv_get_width(h),pdv_get_height(h),pdv_get_depth(h),dma_size);
	if (image_size==0) return -1;
#if 0
	all_buffers=pdv_alloc(num_of_buffers*dma_size); if (all_buffers==0) return printf("pdv_alloc() failed\n");
	for (i=0; i<num_of_buffers; ++i) buffer_ptrs[i]=all_buffers+i*dma_size;
#endif
	err=pdv_set_buffers(h,num_of_buffers,NULL); if (err) return printf("pdv_set_buffers() failed\n");

	pdv_start_images(h,0);
        epicsTimeGetCurrent(&start_time);

	loops = 0 ;
	while (1)
	{
		errlogPrintf("Image is at %p\n",pimage=pdv_wait_image(h));
		if (++loop_count%120==0)
		{
			double time,frame_rate,byte_rate;
			int donecount ;
                        epicsTimeStamp end_time;
        		epicsTimeGetCurrent(&end_time);
			time=max(0.1,epicsTimeDiffInSeconds(&end_time, &start_time));
			donecount = edt_done_count(h) ;
			frame_rate=(double)donecount/time;
			byte_rate=frame_rate*dma_size/(1024*1024);
			printf("%i, %i, %f s, rate = %f f/s = %f MB/s\n",unit,donecount,time,frame_rate,byte_rate);
                        loops++;
		}
		/* if (loops > 1) break ; */
	}

	pdv_close(h) ;
#if 0
	free(all_buffers) ;
#endif
	return(0);
}

int do_speed_both(int dmy, int mode)
{
	int err;
	PdvDev* h=0;
	PdvDev* h2=0;
	int image_size=0;
	int dma_size=0;
	unsigned char* all_buffers;
	unsigned char* buffer_ptrs[8];
	unsigned char* all_buffers2;
	unsigned char* buffer_ptrs2[8];
	int const num_of_buffers=sizeof(buffer_ptrs)/sizeof(buffer_ptrs[0]);
	int loop_count=0, loops=0;
	/* int start_tick; */ /* unused */
	char initcam_command[32];
	int i;
	int unit = 0 ;

	edtInstall(unit);

	sprintf(initcam_command,"-u %i -c 2 -f %s",unit,mode?config_name_pw:config_name_freerun);
	printf("initcam(%s)\n",initcam_command);
	initcam(unit,2,mode?config_name_pw:config_name_freerun);
	h=pdv_open_channel("pdv",unit,2); if (h==0) return printf("pdv_open_channel() failed\n");
	pdv_set_timeout(h,0);

	dma_size=pdv_get_dmasize(h);
	image_size=pdv_get_imagesize(h);
	printf("image_size=%i: %ix%ix%i, dma_size=%i\n",image_size,pdv_get_width(h),pdv_get_height(h),pdv_get_depth(h),dma_size);
	if (image_size==0) return -1;

	all_buffers=pdv_alloc(num_of_buffers*dma_size); if (all_buffers==0) return printf("pdv_alloc() failed\n");
	for (i=0; i<num_of_buffers; ++i) buffer_ptrs[i]=all_buffers+i*dma_size;
	err=pdv_set_buffers(h,num_of_buffers,buffer_ptrs); if (err) return printf("pdv_set_buffers() failed\n");

	unit = 1 ;
	edtInstall(unit);

	sprintf(initcam_command,"-u %i -c 2 -f %s",unit,mode?config_name_pw:config_name_freerun);
	printf("initcam(%s)\n",initcam_command);
	initcam(unit,2,mode?config_name_pw:config_name_freerun);
	h2=pdv_open_channel("pdv",unit,2); if (h==0) return printf("pdv_open_channel() failed\n");
	pdv_set_timeout(h2,0);

	printf("image_size=%i: %ix%ix%i, dma_size=%i\n",image_size,pdv_get_width(h),pdv_get_height(h),pdv_get_depth(h),dma_size);
	if (image_size==0) return -1;

	all_buffers2=pdv_alloc(num_of_buffers*dma_size); if (all_buffers==0) return printf("pdv_alloc() failed\n");
	for (i=0; i<num_of_buffers; ++i) buffer_ptrs2[i]=all_buffers2+i*dma_size;
	err=pdv_set_buffers(h2,num_of_buffers,buffer_ptrs2); if (err) return printf("pdv_set_buffers() failed\n");

	pdv_start_images(h,0);
	pdv_start_images(h2,0);

/*	start_tick=tickGet(); */
	printf("working...\n");

	while (1)
	{
		pdv_wait_image(h);
		pdv_wait_image(h2);
		if (loop_count++%64==0)
		{
/*			double time,frame_rate,byte_rate;
			int donecount ;
			int donecount2 ;
			int tick=tickGet()-start_tick;
			if (tick==0) tick=1;
			time=(double)tick/sysClkRateGet();
			donecount = edt_done_count(h) ;
			donecount2 = edt_done_count(h2) ;
			frame_rate=(double)donecount/time;
			byte_rate=frame_rate*dma_size/(1024*1024);
			printf("%i-%i, %f s, rate = %f f/s = %f MB/s\n",
				donecount,donecount2,time,frame_rate,byte_rate);
*/
			if (loops++ > 20) break ;
		}
	}
	pdv_close(h) ;
	free(all_buffers) ;
	pdv_close(h2) ;
	free(all_buffers2) ;
	return 0;
}


