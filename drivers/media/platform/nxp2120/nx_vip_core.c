/* linux/drivers/media/video/nexell/nx_vip_core.c
 *
 * Core file for Nexell NXP2120 camera(VIP) driver
 * Copyright (c) 2019 I4VINE Inc.
 * All right reserved by Juyoung Ryu <jyryu@i4vine.com>*
 *
 * Copyright (c) 2011~2018 I4VINE Inc.
 * All right reserved by Seungwoo Kim <ksw@i4vine.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/device.h>
#include <linux/clk.h>
#include <linux/i2c.h>
#include <linux/mutex.h>
#include <linux/poll.h>
#include <linux/wait.h>
#include <linux/fs.h>
#include <linux/irq.h>
#include <linux/mm.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/videodev2.h>
#include <linux/gpio/consumer.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/of_graph.h>
#include <linux/of_gpio.h>
#include <linux/slab.h> //kfree


#include <asm/io.h>
#include <asm/memory.h>

//#include <mach/platform.h>
//#include <mach/devices.h>
//#include <mach/soc.h>
#include "nx_vip_core.h"

#define BUFFER_X1280	0


/* ksw : always initialize camera for camera test board */
// #define CONFIG_ALWAYS_INITIALIZE_CAMERA


struct nx_vip_config nx_vip;

#if USE_VB2
static struct nx_video_buffer *to_vip_vb(struct vb2_v4l2_buffer *vb)
{
	return container_of(vb, struct nx_video_buffer, vb);
}
#endif

#define DUMP_REGISTER 1
void dump_register(struct nx_vip_control *ctrl)
{
#if (DUMP_REGISTER)
    struct NX_VIP_RegisterSet *pREG =
        (struct NX_VIP_RegisterSet*)ctrl->regs;

    if (!pREG)
    	return;

  //  DBGOUT("module=%d\n",ctrl->id);
    pr_info("base addr (%d)=0x%X\n", ctrl->id, ctrl->regs);

    pr_info("BASE ADDRESS: %p\n", pREG);
    pr_info(" VIP_CONFIG     = 0x%04x\n", pREG->VIP_CONFIG);
    pr_info(" VIP_HVINT      = 0x%04x\n", pREG->VIP_HVINT);
    pr_info(" VIP_SYNCCTRL   = 0x%04x\n", pREG->VIP_SYNCCTRL);
    pr_info(" VIP_SYNCMON    = 0x%04x\n", pREG->VIP_SYNCMON);
    pr_info(" VIP_VBEGIN     = 0x%04x\n", pREG->VIP_VBEGIN);
    pr_info(" VIP_VEND       = 0x%04x\n", pREG->VIP_VEND);
    pr_info(" VIP_HBEGIN     = 0x%04x\n", pREG->VIP_HBEGIN);
    pr_info(" VIP_HEND       = 0x%04x\n", pREG->VIP_HEND);
    pr_info(" VIP_FIFOCTRL   = 0x%04x\n", pREG->VIP_FIFOCTRL);
    pr_info(" VIP_HCOUNT     = 0x%04x\n", pREG->VIP_HCOUNT);
    pr_info(" VIP_VCOUNT     = 0x%04x\n", pREG->VIP_VCOUNT);
    pr_info(" VIP_CDENB      = 0x%04x\n", pREG->VIP_CDENB);
    pr_info(" VIP_ODINT      = 0x%04x\n", pREG->VIP_ODINT);
    pr_info(" VIP_IMGWIDTH   = 0x%04x\n", pREG->VIP_IMGWIDTH);
    pr_info(" VIP_IMGHEIGHT  = 0x%04x\n", pREG->VIP_IMGHEIGHT);
    pr_info(" CLIP_LEFT      = 0x%04x\n", pREG->CLIP_LEFT);
    pr_info(" CLIP_RIGHT     = 0x%04x\n", pREG->CLIP_RIGHT);
    pr_info(" CLIP_TOP       = 0x%04x\n", pREG->CLIP_TOP);
    pr_info(" CLIP_BOTTOM    = 0x%04x\n", pREG->CLIP_BOTTOM);
    pr_info(" DECI_TARGETW   = 0x%04x\n", pREG->DECI_TARGETW);
    pr_info(" DECI_TARGETH   = 0x%04x\n", pREG->DECI_TARGETH);
    pr_info(" DECI_DELTAW    = 0x%04x\n", pREG->DECI_DELTAW);
    pr_info(" DECI_DELTAH    = 0x%04x\n", pREG->DECI_DELTAH);
    pr_info(" DECI_CLEARW    = 0x%04x\n", pREG->DECI_CLEARW);
    pr_info(" DECI_CLEARH    = 0x%04x\n", pREG->DECI_CLEARH);
    pr_info(" DECI_LUSEG     = 0x%04x\n", pREG->DECI_LUSEG);
    pr_info(" DECI_CRSEG     = 0x%04x\n", pREG->DECI_CRSEG);
    pr_info(" DECI_CBSEG     = 0x%04x\n", pREG->DECI_CBSEG);
    pr_info(" DECI_FORMAT    = 0x%04x\n", pREG->DECI_FORMAT);
    pr_info(" DECI_ROTFLIP   = 0x%04x\n", pREG->DECI_ROTFLIP);
    pr_info(" DECI_LULEFT    = 0x%04x\n", pREG->DECI_LULEFT);
    pr_info(" DECI_CRLEFT    = 0x%04x\n", pREG->DECI_CRLEFT);
    pr_info(" DECI_CBLEFT    = 0x%04x\n", pREG->DECI_CBLEFT);
    pr_info(" DECI_LURIGHT   = 0x%04x\n", pREG->DECI_LURIGHT);
    pr_info(" DECI_CRRIGHT   = 0x%04x\n", pREG->DECI_CRRIGHT);
    pr_info(" DECI_CBRIGHT   = 0x%04x\n", pREG->DECI_CBRIGHT);
    pr_info(" DECI_LUTOP     = 0x%04x\n", pREG->DECI_LUTOP);
    pr_info(" DECI_CRTOP     = 0x%04x\n", pREG->DECI_CRTOP);
    pr_info(" DECI_CBTOP     = 0x%04x\n", pREG->DECI_CBTOP);
    pr_info(" DECI_LUBOTTOM  = 0x%04x\n", pREG->DECI_LUBOTTOM);
    pr_info(" DECI_CRBOTTOM  = 0x%04x\n", pREG->DECI_CRBOTTOM);
    pr_info(" DECI_CBBOTTOM  = 0x%04x\n", pREG->DECI_CBBOTTOM);
    pr_info(" CLIP_LUSEG     = 0x%04x\n", pREG->CLIP_LUSEG);
    pr_info(" CLIP_CRSEG     = 0x%04x\n", pREG->CLIP_CRSEG);
    pr_info(" CLIP_CBSEG     = 0x%04x\n", pREG->CLIP_CBSEG);
    pr_info(" CLIP_FORMAT    = 0x%04x\n", pREG->CLIP_FORMAT);
    pr_info(" CLIP_ROTFLIP   = 0x%04x\n", pREG->CLIP_ROTFLIP);
    pr_info(" CLIP_LULEFT    = 0x%04x\n", pREG->CLIP_LULEFT);
    pr_info(" CLIP_CRLEFT    = 0x%04x\n", pREG->CLIP_CRLEFT);
    pr_info(" CLIP_CBLEFT    = 0x%04x\n", pREG->CLIP_CBLEFT);
    pr_info(" CLIP_LURIGHT   = 0x%04x\n", pREG->CLIP_LURIGHT);
    pr_info(" CLIP_CRRIGHT   = 0x%04x\n", pREG->CLIP_CRRIGHT);
    pr_info(" CLIP_CBRIGHT   = 0x%04x\n", pREG->CLIP_CBRIGHT);
    pr_info(" CLIP_LUTOP     = 0x%04x\n", pREG->CLIP_LUTOP);
    pr_info(" CLIP_CRTOP     = 0x%04x\n", pREG->CLIP_CRTOP);
    pr_info(" CLIP_CBTOP     = 0x%04x\n", pREG->CLIP_CBTOP);
    pr_info(" CLIP_LUBOTTOM  = 0x%04x\n", pREG->CLIP_LUBOTTOM);
    pr_info(" CLIP_CRBOTTOM  = 0x%04x\n", pREG->CLIP_CRBOTTOM);
    pr_info(" CLIP_CBBOTTOM  = 0x%04x\n", pREG->CLIP_CBBOTTOM);
    pr_info(" VIP_SCANMODE   = 0x%04x\n", pREG->VIP_SCANMODE);
    pr_info(" CLIP_YUYVENB   = 0x%04x\n", pREG->CLIP_YUYVENB);
    pr_info(" CLIP_BASEADDRH = 0x%04x\n", pREG->CLIP_BASEADDRH);
    pr_info(" CLIP_BASEADDRL = 0x%04x\n", pREG->CLIP_BASEADDRL);
    pr_info(" CLIP_STRIDEH   = 0x%04x\n", pREG->CLIP_STRIDEH);
    pr_info(" CLIP_STRIDEL   = 0x%04x\n", pREG->CLIP_STRIDEL);
    pr_info(" VIP_VIP1       = 0x%04x\n", pREG->VIP_VIP1);     
    pr_info(" VIPCLKENB       = 0x%04x\n", pREG->VIPCLKENB);  
 //   DBGOUT(" VIPCLKGEN       = 0x%04x\n", pREG->VIPCLKGEN[0][0]);      
#endif
}


#if 0
static int nx_vip_set_output_format(struct nx_vip_control *ctrl,
					struct v4l2_pix_format *fmt)
{
	struct nx_vip_out_frame *frame = &ctrl->out_frame;
	int depth = -1;

	frame->width = fmt->width;
	frame->height = fmt->height;

	switch (fmt->pixelformat) {
	case V4L2_PIX_FMT_YUV420:
		frame->format = FORMAT_YCBCR420;
		frame->planes = 3;
		depth = 12;
		break;
	case V4L2_PIX_FMT_YUV444:
		frame->format = FORMAT_YCBCR444;
		frame->planes = 3;
		depth = 24;
		break;

	case V4L2_PIX_FMT_YUYV:
		frame->format = FORMAT_YCBCR422;
		frame->planes = 1;
		frame->order_1p = OUT_ORDER422_YCBYCR;
		depth = 16;
		break;

	case V4L2_PIX_FMT_YVYU:
		frame->format = FORMAT_YCBCR422;
		frame->planes = 1;
		frame->order_1p = OUT_ORDER422_YCRYCB;
		depth = 16;
		break;

	case V4L2_PIX_FMT_UYVY:
		frame->format = FORMAT_YCBCR422;
		frame->planes = 1;
		frame->order_1p = OUT_ORDER422_CBYCRY;
		depth = 16;
		break;

	case V4L2_PIX_FMT_VYUY:
		frame->format = FORMAT_YCBCR422;
		frame->planes = 1;
		frame->order_1p = OUT_ORDER422_CRYCBY;
		depth = 16;
		break;
	case V4L2_PIX_FMT_YUV422P:
		frame->format = FORMAT_YCBCR422;
		frame->planes = 3;
		depth = 16;
		break;
	default :
	    depth = -1;
	}
#if 0
	switch (fmt->field) {
	case V4L2_FIELD_INTERLACED:
	case V4L2_FIELD_INTERLACED_TB:
	case V4L2_FIELD_INTERLACED_BT:
		frame->scan = SCAN_TYPE_INTERLACE;
		break;
#if 0
	default:
		frame->scan = SCAN_TYPE_PROGRESSIVE;
		break;
	}
#endif
#endif
	return depth;
}

int nx_vip_set_output_frame(struct nx_vip_control *ctrl,
				struct v4l2_pix_format *fmt)
{
	struct nx_vip_out_frame *frame = &ctrl->out_frame;
	int depth = 0;
	int to_change = 0;
	int old_plane;

	if ((ctrl->out_frame.width != fmt->width) ||
		(ctrl->out_frame.height != fmt->height)) {
		to_change = 1;
	}
	old_plane = ctrl->out_frame.planes;

	depth = nx_vip_set_output_format(ctrl, fmt);

	if (old_plane != ctrl->out_frame.planes) {
		to_change |= 2;
	}

	if ((frame->addr[0].virt_y == NULL) && (depth > 0)) {
		if (nx_vip_alloc_output_memory(ctrl)) {
			err("cannot allocate memory\n");
		}
		//printk("cfn = %d, addr[0].virt_y = %x, addr[1].virt_y = %x, addr[2].virt_y = %x, addr[3].virt_y = %x",
	    //frame->cfn,frame->addr[0].virt_y, frame->addr[1].virt_y,
	    //frame->addr[2].virt_y, frame->addr[3].virt_y);
	} else {
		// If we want to change alloc mem's depth and/or width/height, we should stop the engine first.
		if (to_change) {
			nx_vip_stop_vip(ctrl);
			// Do we should stop IRQ also?
			// alloc output memory -- actually this is not allocate, just reassigning the buffer address.
			nx_vip_alloc_output_memory(ctrl);
		}
	}

	return depth;
}

int nx_vip_alloc_output_memory(struct nx_vip_control *ctrl)
{
	struct nx_vip_out_frame *info = &ctrl->out_frame;
	int bank;
	int width = info->width, height = info->height;
	unsigned long start_offset = 0;
	int buffer_changed = 0;
	struct nx_vip_frame_addr *addr;
	int cr_w, cr_h, cr_l, cr_t;
	int top;
	void *boot_alloc_mem;	
	
	cr_w = ctrl->v4l2.crop_current.width;
	cr_h = ctrl->v4l2.crop_current.height;
	cr_l = ctrl->v4l2.crop_current.left;
	cr_t = ctrl->v4l2.crop_current.top;

	if ((cr_w != width) && (cr_h != height))
	{
		ctrl->use_scaler = 1;
#if BUFFER_X1280
		ctrl->scaler->offset = 512 * 4096;
#else
		ctrl->scaler->offset = 1024 * 4096; // for 640X480 ->1024X512 for Y, 1024X512 for U,V
#endif
		//printk("cr_w=%d, width=%d cr_h=%d height=%d\n", cr_w, width, cr_h, height);
	} else {
		ctrl->use_scaler = 0;
		//printk("cr_w=%d cr_h=%d\n", cr_w, cr_h);
	}

	/* But with reserved mem as bootmem area, we could avoid complexity
	   of DMA coherent and memory leakage.
	   
	   2014. 02. 21. by KSW:
		 4096 stride would give 4 buffers with 1024 horizontal resolution.
		 Hence, 4Mbytes of buffer could handle 4 buffers with 1024X512 resolution.
		 And without scaler/decimator, 640X480 could be handled.
		 With Omnivision sensors, 640X480 and another 240 lines would need for color images,
		 then another 480 lines for scaler/decimator.
		 
		 Is it enough for MPEG4 driver? No.
		 MPEG4 uses minimum 14Mbyte...
	*/
	info->buf_size = width * height * 2;
// //////////////////////////////////////
	boot_alloc_mem = (void *)0xA0000000; /* Start of the DRAM/BLOCK address */
	
	start_offset = (dma_addr_t)boot_alloc_mem;
//	start_offset = 0;//(dma_addr_t)boot_alloc_mem;
	info("%s: start_offset=%X addr[0]=%X\n", __func__, info->addr[0].phys_y);

	if (info->fmt->num_sw_planes > 1) {
		// Then we must use Block memory buffer ?
		start_offset |= 0x20000000;
	}
	start_offset += ctrl->id * 8 * 1024 * 1024;
	if (start_offset != info->addr[0].phys_y)
		buffer_changed = 1;

	info->addr[0].phys_y = start_offset;                                         
	if (buffer_changed) {
		if (NULL != info->addr[0].virt_y) {
			iounmap(info->addr[0].virt_y);
		}
		info->addr[0].virt_y = ioremap(info->addr[0].phys_y, 8 * 1024 * 1024);
	}
	info("alloc boot mem vir=%x, phys=%x\n", (unsigned int)info->addr[0].virt_y, (unsigned int)info->addr[0].phys_y);
#if BUFFER_X1280
	info->nr_frames = 3;
	for (bank = 1; bank < info->nr_frames; bank++) {
		info->addr[bank].virt_y = info->addr[0].virt_y + 1280 * bank;
		info->addr[bank].phys_y = info->addr[0].phys_y + 1280 * bank;
		printk("bank=%d vir_y=%x phy_y=%x\n", bank, info->addr[bank].virt_y, info->addr[bank].phys_y);
	}

	info->scw = cr_w;
	info->sch = cr_h;
	
	top = (start_offset >> 12) & 0xFFF;

/*
	Buffer structures are:

	|--------   4096 stride  ---------|	
	+----+----+----+----+----+----+---+
	| 1280    | 1280    | 1280    |256|
	| LU(0)   | LU(1)   | LU(2)   |WAS|
	|         |         |         |TE | 1024
	|         |         |         |   |
	+----+----+----+----+----+----+---+
	|640 |640 |640 |640 |640 |640 |256|
	|CB0 |CR0 |CB1 |CR1 |CB2 |CR2 |   |
	|    |    |    |    |    |    |   | 1024
	|    |    |    |    |    |    |   |
	|    |    |    |    |    |    |   |
	+----+----+----+----+----+----+---+
	
	
	So It can handle Max 1280 X 1024 Y data, 640 X 1024 CB, 640 X 1024 CR data.
	
*/
	if (info->fmt->num_sw_planes > 1) {
		for (bank = 0; bank < info->nr_frames; bank++) {
			
			addr = &info->addr[bank];
			addr->lu_seg = addr->phys_y >> 24;
			addr->cb_seg = addr->lu_seg;
			addr->cr_seg = addr->lu_seg;
			addr->lu_left  = bank * 1280;
			addr->lu_top   = top;
			addr->lu_right = cr_w + bank * 1280;
			addr->lu_bottom= top + cr_h;
			addr->cb_left  = bank * 1280;
			addr->cb_top   = top + 1024;
			if (info->format == FORMAT_YCBCR422) {
				addr->cb_right = cr_w / 2 + bank * 1280;
				addr->cb_bottom= top + 1024 + cr_h;
				addr->cr_left  = 640 + bank * 1280;
				addr->cr_top   = top + 1024;
				addr->cr_right = 640 + cr_w /2 + bank * 1280;
				addr->cr_bottom= top + 1024 + cr_h;
			} else 
			if (info->format == FORMAT_YCBCR444) { // actualy we can't support this format
				addr->cb_right = cr_w + bank * 1280;
				addr->cb_bottom= top + 1024 + cr_h;
				addr->cr_left  = bank * 1280;
				addr->cr_top   = top + 2048;
				addr->cr_right = cr_w + bank * 1280;
				addr->cr_bottom= top + 2048 + cr_h;
			} else { /* YUV420 */
				addr->cb_right = cr_w / 2 + bank * 1280;
				addr->cb_bottom= top + 1024 + cr_h / 2;
				addr->cr_left  = 640 + bank * 1280;
				addr->cr_top   = top + 1024;
				addr->cr_right = 640 + cr_w / 2 + bank * 1280;
				addr->cr_bottom= top + 1024 + cr_h / 2;
			}
			if (ctrl->use_scaler) { // actually we can't support scaler either.
				addr->src_addr_lu = ((addr->phys_y) & 0xFF000000) | (0 << 12) | (bank * 1280);
				addr->src_addr_cb = ((addr->phys_y) & 0xFF000000) | (1024 << 12) | (bank * 1280);
				addr->src_addr_cr = ((addr->phys_y) & 0xFF000000) | (1024 << 12) | ((bank * 1280) + 640);
				addr->dst_addr_lu = ((addr->phys_y) & 0xFF000000) | (512  << 12) | (bank * 1280);
				addr->dst_addr_cb = ((addr->phys_y) & 0xFF000000) | ((512 + 1024) << 12)  | (bank * 1280);
				addr->dst_addr_cr = ((addr->phys_y) & 0xFF000000) | ((512 + 1024) << 12) | ((bank * 1280) + 640);
				addr->vir_addr_lu = addr->virt_y + 512 * 4096;
				printk("scaler, vir_lu=%x\n", addr->vir_addr_lu);
			} else {
				addr->vir_addr_lu = addr->virt_y;
				printk("no scaler, vir_lu=%x\n", addr->vir_addr_lu);
			}
			addr->vir_addr_cb = addr->vir_addr_lu + 1024 * 4096;
			addr->vir_addr_cr = addr->vir_addr_lu + 1024 * 4096 + 640;
			printk("vir lu =%x cb = %x cr = %x\n", addr->vir_addr_lu, addr->vir_addr_cb, addr->vir_addr_cr); 
		}
	}
#else
	info->nr_frames = 4;
	for (bank = 1; bank < info->nr_frames; bank++) {
		info->addr[bank].virt_y = info->addr[0].virt_y + 1024 * bank;
		info->addr[bank].phys_y = info->addr[0].phys_y + 1024 * bank;
	}

	info->scw = cr_w;
	info->sch = cr_h;
	
	top = (start_offset >> 12) & 0xFFF;
	
/*
	Buffer structures are:

	|------------   4096 stride  -----------|
	+----+----+----+----+----+----+----+----+
	| 1024    | 1024    | 1024    | 1024    |
	| LU(0)   | LU(1)   | LU(2)   | LU(3)   |
	|         |         |         |         | 512
	|         |         |         |         |
	+----+----+----+----+----+----+----+----+
	|CB0 |CR0 |CB1 |CR1 |CB2 |CR2 |CB3 |CR3 |
	|    |    |    |    |    |    |    |    |
	|    |    |    |    |    |    |    |    | 512
	|    |    |    |    |    |    |    |    |
	+----+----+----+----+----+----+----+----+
	
	
	So It can handle Max 1024 X 512 Y data, 512 X 512 CB, 512 X 512 CR data.
	
*/

	if (info->fmt->num_sw_planes > 1) {
		for (bank = 0; bank < info->nr_frames; bank++) {
			addr = &info->addr[bank];
			addr->lu_seg = addr->phys_y >> 24;
			addr->cb_seg = addr->lu_seg;
			addr->cr_seg = addr->lu_seg;
			addr->lu_left  = bank * 1024;
			addr->lu_top   = top;
			addr->lu_right = cr_w + bank * 1024;
			addr->lu_bottom= top + cr_h;
			addr->cb_left  = bank * 1024;
			addr->cb_top   = top + 512;
			if (info->format == FORMAT_YCBCR422) {
				addr->cb_right = cr_w / 2 + bank * 1024;
				addr->cb_bottom= top + 512 + cr_h;
				addr->cr_left  = 512 + bank * 1024;
				addr->cr_top   = top + 512;
				addr->cr_right = 512 + cr_w /2 + bank * 1024;
				addr->cr_bottom= top + 512 + cr_h;
			} else 
			if (info->format == FORMAT_YCBCR444) {
				addr->cb_right = cr_w + bank * 1024;
				addr->cb_bottom= top + 512 + cr_h;
				addr->cr_left  = bank * 1024;
				addr->cr_top   = top + 1024;
				addr->cr_right = cr_w + bank * 1024;
				addr->cr_bottom= top + 1024 + cr_h;
			} else { /* YUV420 */
				addr->cb_right = cr_w / 2 + bank * 1024;
				addr->cb_bottom= top + 512 + cr_h / 2;
				addr->cr_left  = 512 + bank * 1024;
				addr->cr_top   = top + 512;
				addr->cr_right = 512 + cr_w / 2 + bank * 1024;
				addr->cr_bottom= top + 512 + cr_h / 2;
			}
			if (ctrl->use_scaler) {
				addr->src_addr_lu = ((addr->phys_y) & 0xFF000000) | (0 << 12) | (bank * 1024);
				addr->src_addr_cb = ((addr->phys_y) & 0xFF000000) | (512 << 12) | (bank * 1024);
				addr->src_addr_cr = ((addr->phys_y) & 0xFF000000) | (512 << 12) | ((bank * 1024) + 512);
				addr->dst_addr_lu = ((addr->phys_y) & 0xFF000000) | (1024 << 12) | (bank * 1024);
				addr->dst_addr_cb = ((addr->phys_y) & 0xFF000000) | ((512 + 1024) << 12)  | (bank * 1024);
				addr->dst_addr_cr = ((addr->phys_y) & 0xFF000000) | ((512 + 1024) << 12) | ((bank * 1024) + 512);
				addr->vir_addr_lu = addr->virt_y + ctrl->scaler->offset;
			} else {
				addr->vir_addr_lu = addr->virt_y;	
			}
			addr->vir_addr_cb = addr->vir_addr_lu + 512 * 4096;
			addr->vir_addr_cr = addr->vir_addr_lu + 512 * 4096 + 512;
		}
	}
#endif

	memset(info->addr[0].virt_y, 0, 8 * 1024 * 1024);

	return 0;
}
  
void nx_vip_free_output_memory(struct nx_vip_out_frame *info)
{
	if (info->addr[0].virt_y) {
    	iounmap(info->addr[0].virt_y);
    	info->addr[0].virt_y = NULL;
	}
    info->addr[0].phys_y = 0;
    info->addr[1].virt_y = NULL;
    info->addr[1].phys_y = 0;
    info->addr[2].virt_y = NULL;
    info->addr[2].phys_y = 0;
    info->addr[3].virt_y = NULL;
    info->addr[3].phys_y = 0;
}
#endif
      

int nx_vip_alloc_frame_memory(struct nx_vip_control *ctrl)
{
	struct nx_video_frame *info = &ctrl->cap_frame;
	int bank;
	int width =  ctrl->cap_frame.width, height = ctrl->cap_frame.height;
	unsigned long start_offset = 0;
	int buffer_changed = 0;
	struct nx_vip_frame_addr *addr;
	int cr_w, cr_h, cr_l, cr_t;
	int top;
//	void *boot_alloc_mem;
//	boot_alloc_mem = (void*)0xA0000000;

	printk("%s enter \n", __func__);
	cr_w = ctrl->cap_frame.width;
	cr_h = ctrl->cap_frame.height;
	cr_l = ctrl->cap_frame.offs_h;
	cr_t = ctrl->cap_frame.offs_v;	
	printk("cr_w %d cr_h %d cr_l %d cr_t %d\n", cr_w, cr_h, cr_l, cr_t);
	
/*	cr_w = ctrl->v4l2.crop_current.width;
	cr_h = ctrl->v4l2.crop_current.height;
	cr_l = ctrl->v4l2.crop_current.left;
	cr_t = ctrl->v4l2.crop_current.top;*/

	if ((cr_w != width) && (cr_h != height))
	{
		ctrl->use_scaler = 1;
#if BUFFER_X1280
		ctrl->scaler->offset = 512 * 4096;
#else
		ctrl->scaler->offset = 1024 * 4096; // for 640X480 ->1024X512 for Y, 1024X512 for U,V
#endif
		//printk("cr_w=%d, width=%d cr_h=%d height=%d\n", cr_w, width, cr_h, height);
	} else {
		ctrl->use_scaler = 0;
		//printk("cr_w=%d cr_h=%d\n", cr_w, cr_h);
	}

	/* But with reserved mem as bootmem area, we could avoid complexity
	   of DMA coherent and memory leakage.
	   
	   2014. 02. 21. by KSW:
		 4096 stride would give 4 buffers with 1024 horizontal resolution.
		 Hence, 4Mbytes of buffer could handle 4 buffers with 1024X512 resolution.
		 And without scaler/decimator, 640X480 could be handled.
		 With Omnivision sensors, 640X480 and another 240 lines would need for color images,
		 then another 480 lines for scaler/decimator.
		 
		 Is it enough for MPEG4 driver? No.
		 MPEG4 uses minimum 14Mbyte...
	*/
//	info->buf_size = width * height * 2;

	start_offset = (dma_addr_t)ctrl->vip_dt_data.dma_mem;
	info("start_offset=%X addr[0]=%X\n",  start_offset,info->addr[0].phys_y);

	if (info->fmt->num_sw_planes > 1) {
		// Then we must use Block memory buffer ?
		start_offset |= 0x20000000;
	}
	start_offset += ctrl->id * 8 * 1024 * 1024;
	printk("ctrl->id  %d ,start_offset=0x%X\n", ctrl->id ,start_offset);
	if (start_offset != info->addr[0].phys_y)
		buffer_changed = 1;

	info->addr[0].phys_y = start_offset;
//	if (buffer_changed) {
		if (NULL != info->addr[0].virt_y) {
			iounmap(info->addr[0].virt_y);
		}
		info->addr[0].virt_y = ioremap(info->addr[0].phys_y, 8 * 1024 * 1024);
//	}
	info("alloc boot mem vir=%x, phys=%x\n", (unsigned int)info->addr[0].virt_y, (unsigned int)info->addr[0].phys_y);

	printk("===============================\n");
	info->nr_frames = 4;
	for (bank = 1; bank < info->nr_frames; bank++) {
		info->addr[bank].virt_y = info->addr[0].virt_y + 1024 * bank;
		info->addr[bank].phys_y = info->addr[0].phys_y + 1024 * bank;
		printk("virt_y 0x%X phys_y 0x%X\n", info->addr[bank].virt_y,info->addr[bank].phys_y);
	}

	info->scw = cr_w;
	info->sch = cr_h;
	printk("=========================cr_w %d, cr_h %d\n", cr_w, cr_h);
	top = (start_offset >> 12) & 0xFFF;
	
/*
	Buffer structures are:

	|------------   4096 stride  -----------|
	+----+----+----+----+----+----+----+----+
	| 1024    | 1024    | 1024    | 1024    |
	| LU(0)   | LU(1)   | LU(2)   | LU(3)   |
	|         |         |         |         | 512
	|         |         |         |         |
	+----+----+----+----+----+----+----+----+
	|CB0 |CR0 |CB1 |CR1 |CB2 |CR2 |CB3 |CR3 |
	|    |    |    |    |    |    |    |    |
	|    |    |    |    |    |    |    |    | 512
	|    |    |    |    |    |    |    |    |
	+----+----+----+----+----+----+----+----+
	
	
	So It can handle Max 1024 X 512 Y data, 512 X 512 CB, 512 X 512 CR data.
	
*/

	if (info->fmt->num_sw_planes > 1) {
		for (bank = 0; bank < info->nr_frames; bank++) {
			printk("info 0x%X\n", info);
			addr = &info->addr[bank];
			printk("info 0x%X\n", info);
			printk("=======================addr 0x%X \n",addr);		
			addr->lu_seg = addr->phys_y >> 24;
			addr->cb_seg = addr->lu_seg;
			addr->cr_seg = addr->lu_seg;
			addr->lu_left  = bank * 1024;
			addr->lu_top   = top;
			addr->lu_right = cr_w + bank * 1024;
			addr->lu_bottom= top + cr_h;
			addr->cb_left  = bank * 1024;
			addr->cb_top   = top + 512;
			printk("=======================top 0x%X \n",top);	
			if (ctrl->cap_frame.fmt->pixelformat == V4L2_PIX_FMT_YUV422P) {
				printk("================422P\n");
				addr->cb_right = cr_w / 2 + bank * 1024;
				addr->cb_bottom= top + 512 + cr_h;
				addr->cr_left  = 512 + bank * 1024;
				addr->cr_top   = top + 512;
				addr->cr_right = 512 + cr_w /2 + bank * 1024;
				addr->cr_bottom= top + 512 + cr_h;
			} else if (ctrl->cap_frame.fmt->pixelformat == V4L2_PIX_FMT_YUYV) {
				printk("================YUYV\n");
				addr->cb_right = cr_w / 2 + bank * 1024;
				addr->cb_bottom= top + 512 + cr_h;
				addr->cr_left  = 512 + bank * 1024;
				addr->cr_top   = top + 512;
				addr->cr_right = 512 + cr_w /2 + bank * 1024;
				addr->cr_bottom= top + 512 + cr_h;
			} else if (ctrl->cap_frame.fmt->pixelformat == V4L2_PIX_FMT_YUV444) {
				printk("================444\n");
				addr->cb_right = cr_w + bank * 1024;
				addr->cb_bottom= top + 512 + cr_h;
				addr->cr_left  = bank * 1024;
				addr->cr_top   = top + 1024;
				addr->cr_right = cr_w + bank * 1024;
				addr->cr_bottom= top + 1024 + cr_h;
			} else { /* YUV420 */
				printk("================420\n");
				addr->cb_right = cr_w / 2 + bank * 1024;
				addr->cb_bottom= top + 512 + cr_h / 2;
				addr->cr_left  = 512 + bank * 1024;
				addr->cr_top   = top + 512;
				addr->cr_right = 512 + cr_w / 2 + bank * 1024;
				addr->cr_bottom= top + 512 + cr_h / 2;
			}
			if (ctrl->use_scaler) {
				printk("use_scaler\n");
				addr->src_addr_lu = ((addr->phys_y) & 0xFF000000) | (0 << 12) | (bank * 1024);
				addr->src_addr_cb = ((addr->phys_y) & 0xFF000000) | (512 << 12) | (bank * 1024);
				addr->src_addr_cr = ((addr->phys_y) & 0xFF000000) | (512 << 12) | ((bank * 1024) + 512);
				addr->dst_addr_lu = ((addr->phys_y) & 0xFF000000) | (1024 << 12) | (bank * 1024);
				addr->dst_addr_cb = ((addr->phys_y) & 0xFF000000) | ((512 + 1024) << 12)  | (bank * 1024);
				addr->dst_addr_cr = ((addr->phys_y) & 0xFF000000) | ((512 + 1024) << 12) | ((bank * 1024) + 512);
				addr->vir_addr_lu = addr->virt_y + ctrl->scaler->offset;
			} else {
				addr->vir_addr_lu = addr->virt_y;	
			}
			addr->vir_addr_cb = addr->vir_addr_lu + 512 * 4096;
			addr->vir_addr_cr = addr->vir_addr_lu + 512 * 4096 + 512;
		}
	}


	memset(info->addr[0].virt_y, 0, 8 * 1024 * 1024);
	printk("%s exit\n", __func__);
	return 0;
}
  
void nx_vip_free_frame_memory(struct nx_video_frame *frame)
{
	if (frame->addr[0].virt_y) {
    	iounmap(frame->addr[0].virt_y);
    	frame->addr[0].virt_y = NULL;
	}
    frame->addr[0].phys_y = 0;
    frame->addr[1].virt_y = NULL;
    frame->addr[1].phys_y = 0;
    frame->addr[2].virt_y = NULL;
    frame->addr[2].phys_y = 0;
    frame->addr[3].virt_y = NULL;
    frame->addr[3].phys_y = 0;
}



/* Set default format at the sensor and host interface */
static int nx_capture_set_default_format(struct nx_vip_control *ctrl)
{
	struct v4l2_format fmt = {
		.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE,
		.fmt.pix_mp = {
			.width		= ctrl->vip_dt_data.def_width,
			.height		= ctrl->vip_dt_data.def_height,
			.pixelformat	= V4L2_PIX_FMT_YUV422P,
			.field		= V4L2_FIELD_NONE,
			.colorspace	= V4L2_COLORSPACE_SRGB,
		},
	};
	printk("%s enter\n", __func__);
	
	return nx_capture_set_format(ctrl, &fmt);
}

static int nx_video_buf_to_addr(struct nx_vip_control *ctrl, struct nx_video_buffer *buf, struct nx_vip_frame_addr *addr, int offset)
{	
	//u32 height;
	printk("%s enter addr 0x%X\n", __func__, addr);
	addr->phys_y = buf->paddr.y + offset;
	addr->phys_cr = buf->paddr.cr + offset/2;
	addr->phys_cb = buf->paddr.cb + offset/2;
//	printk("phys_y 0x%X cr 0x%X cb 0x%X offset 0x%X\n", buf->paddr.y, buf->paddr.cb, buf->paddr.cr, offset);
//	printk("phys_y 0x%X cr 0x%X cb 0x%X\n", addr->phys_y, addr->phys_cr, addr->phys_cb);
//	pr_debug("%s: paddr_y=%X, seg=%X\n", __func__, buf->paddr.y, buf->paddr.y >> 24); 
	addr->lu_seg = buf->paddr.y >> 30;
	addr->lu_left  =  buf->paddr.y & 0x7FFF;
	addr->lu_top = (buf->paddr.y >> 15) & 0x7FFF;
//	printk("phys_y 0x%X cr 0x%X cb 0x%X\n", addr->lu_seg, addr->lu_left, addr->lu_top);
	
	addr->cr_seg = buf->paddr.cr >> 30;
	addr->cr_left  =  buf->paddr.cr & 0x7FFF;
	addr->cr_top = (buf->paddr.cr >> 15) & 0x7FFF;      
	                                                               
	addr->cb_seg = buf->paddr.cb >> 30;                              
	addr->cb_left  =  buf->paddr.cb & 0x7FFF;
	addr->cb_top = (buf->paddr.cb >> 15) & 0x7FFF;
	
//	pr_debug("luseg=%X\n", addr->lu_seg);

	addr->lu_right = addr->lu_left + buf->stride[0];
	addr->cr_right = addr->cr_left + buf->stride[1];
	addr->cb_right = addr->cb_left + buf->stride[2];

	addr->lu_bottom = addr->lu_top + ctrl->cap_frame.heights[0];
	addr->cr_bottom = addr->cr_top + ctrl->cap_frame.heights[1];
	addr->cb_bottom = addr->cb_top + ctrl->cap_frame.heights[2];
	printk("%s exit\n", __func__);
	return 0;
}


static void nx_vip_setup_memory_region(struct nx_vip_control *ctrl, int setup_all)
{
	int bank = ctrl->buf_index;
	int clip_offset = 0;           
	struct nx_vip_frame_addr faddr, *addr;	 
	struct nx_video_buffer *buf;	  
	int offset = 0;
	int ww, hh;
	
	printk("%s enter bank %d\n", __func__, bank);
//	printk("ctrl->cur_frm 0x%X \n", ctrl->cur_frm);   
#if USE_VB2
	buf = ctrl->cur_frm;             
//	printk("buf phys 0x%X \n", buf->paddr.y);
	nx_video_buf_to_addr(ctrl, buf, &faddr, offset);     
	
	addr = &faddr;
//	printk("addr 0x%X  addr->phys_y 0x%X faddr.phys_y 0x%X\n", addr, addr->phys_y, faddr.phys_y);
	printk("lu_seg 0x%X cr_seg 0x%X cb_seg 0x%X \n", addr->lu_seg, addr->cr_seg, addr->cb_seg);
#else
	addr = &ctrl->cap_frame.addr[bank];   //  ????????????????????이거왜안됨    
    printk("addr 0x%X phy_y 0x%X ctrl->cap_frame.planes %d\n",addr, addr->phys_y, ctrl->cap_frame.planes);
    
#endif
	if (ctrl->use_clipper)
		clip_offset = 0;
	ww = ctrl->cap_frame.width;
	hh = ctrl->cap_frame.height;

	if (setup_all || ctrl->cap_frame.planes == 1) {    
		ctrl->regs->CLIP_BASEADDRH = ctrl->cap_frame.addr[bank].phys_y >> 16;
		ctrl->regs->CLIP_BASEADDRL = ctrl->cap_frame.addr[bank].phys_y & 0xFFFF;
	}

    if (setup_all || ctrl->cap_frame.planes > 1) { 
		if (setup_all) {                              
			ctrl->regs->CLIP_LUSEG	 = addr->lu_seg;   
			ctrl->regs->CLIP_CRSEG	 = addr->cr_seg;
			ctrl->regs->CLIP_CBSEG	 = addr->cb_seg;
			ctrl->regs->DECI_LUSEG	 = addr->lu_seg;
			ctrl->regs->DECI_CRSEG	 = addr->cr_seg;
			ctrl->regs->DECI_CBSEG	 = addr->cb_seg;      
			printk("lu_seg 0x%X cr_seg 0x%X cb_seg 0x%X \n", addr->lu_seg, addr->cr_seg, addr->cb_seg);       
		}
		
		if (ctrl->use_clipper) {
			ctrl->regs->CLIP_LULEFT	 = addr->lu_left;
			ctrl->regs->CLIP_LUTOP	 = addr->lu_top + clip_offset;
			ctrl->regs->CLIP_LURIGHT = addr->lu_left + ww;
			ctrl->regs->CLIP_LUBOTTOM= addr->lu_bottom + clip_offset;

			ctrl->regs->CLIP_CBLEFT	 = addr->cb_left;
			ctrl->regs->CLIP_CBTOP	 = addr->cb_top + clip_offset;
			ctrl->regs->CLIP_CBRIGHT = addr->cb_right;
			ctrl->regs->CLIP_CBBOTTOM= addr->cb_bottom + clip_offset;
				
			ctrl->regs->CLIP_CRLEFT	 = addr->cr_left;
			ctrl->regs->CLIP_CRTOP	 = addr->cr_top + clip_offset;
			ctrl->regs->CLIP_CRRIGHT = addr->cr_right;
			ctrl->regs->CLIP_CRBOTTOM= addr->cr_bottom + clip_offset;			
		}
	}
	printk("%s exit\n", __func__);	
}

/* scaler related macros */
#define HEIGHT_BITPOS	16
#define WIDTH_BITPOS	0
#define BUSY_MASK		(1 << 24)

#define SCALER_SETIMAGESIZE(dwSrcWidth, dwSrcHeight, dwDestWidth, dwDestHeight)\
{ \
	ctrl->scaler->regs->SCSRCSIZEREG = ( ( dwSrcHeight - 1 ) << HEIGHT_BITPOS ) | ( ( dwSrcWidth - 1 ) << WIDTH_BITPOS ); \
	ctrl->scaler->regs->SCDESTSIZEREG = ( ( dwDestHeight - 1 ) << HEIGHT_BITPOS ) | ( ( dwDestWidth - 1 ) << WIDTH_BITPOS ); \
	ctrl->scaler->regs->DELTAXREG = ( dwSrcWidth  * 0x10000 ) / ( dwDestWidth  );\
	ctrl->scaler->regs->DELTAYREG = ( dwSrcHeight * 0x10000 ) / ( dwDestHeight );\
}

#define SCALER_SETSRCADDR(Addr) \
{ \
	ctrl->scaler->regs->SCSRCADDREG	= Addr; \
}

#define SCALER_SETDSTADDR(Addr) \
{ \
	ctrl->scaler->regs->SCDESTADDREG = Addr; \
}
#define SCALER_ISBUSY() ( 0 != (ctrl->scaler->regs->SCINTREG & BUSY_MASK))



void nx_vip_start_vip(struct nx_vip_control *ctrl)
{
	int cam_w, cam_h;
	int out_w, out_h;
	int cr_w, cr_h;
	int cr_l, cr_t;
//    int stride_y, stride_uv;	
	int val;
                                                
	printk("%s enter\n", __func__);
 
	cam_w = ctrl->cap_frame.f_width;
	cam_h = ctrl->cap_frame.f_height;
	out_w = ctrl->cap_frame.o_width;
	out_h = ctrl->cap_frame.o_height;
	ctrl->use_clipper = 1;
	
	/*cr_w = ctrl->v4l2.crop_current.width;
	cr_h = ctrl->v4l2.crop_current.height;
	cr_l = ctrl->v4l2.crop_current.left;
	cr_t = ctrl->v4l2.crop_current.top;*/
	cr_w = ctrl->cap_frame.width;
	cr_h = ctrl->cap_frame.height;
	cr_l = ctrl->cap_frame.offs_h;
	cr_t = ctrl->cap_frame.offs_v;	
	
    pr_debug("ow:%d oh:%d cw:%d ch:%d\n", out_w, out_h, cam_w, cam_h);
    pr_debug("crw:%d crh:%d crl:%d crt:%d\n", cr_w, cr_h, cr_l, cr_t);
    
 //   stride_y = ctrl->cap_frame.stride[0];
 //   stride_uv = ctrl->cap_frame.stride[1];
   
	if ((cr_w != out_w) || (cr_h != out_h)) {  
	   ctrl->use_scaler = 1;
	}
 
	if (ctrl->use_clipper) {
		//printk("start vip %d, w=%d,h=%d\n", ctrl->id, cam_w, cam_h);
		ctrl->regs->VIP_IMGWIDTH = cam_w + 2;
		ctrl->regs->VIP_IMGHEIGHT = cam_h;
		ctrl->regs->CLIP_LEFT = cr_l;
		ctrl->regs->CLIP_RIGHT = cr_l + cr_w;
		ctrl->regs->CLIP_TOP = cr_t;
		ctrl->regs->CLIP_BOTTOM = cr_t + cr_h;
	} else {
		/* Stream On */
		ctrl->scaler->started = 0;
		ctrl->regs->VIP_IMGWIDTH = cam_w + 2;
		ctrl->regs->VIP_IMGHEIGHT = cam_h;
		ctrl->regs->CLIP_LEFT = 0;
		ctrl->regs->CLIP_RIGHT = cam_w;
		ctrl->regs->CLIP_TOP = 0;
		ctrl->regs->CLIP_BOTTOM = cam_h;
	}
 
	ctrl->regs->VIP_VBEGIN = 0;
	ctrl->regs->VIP_VEND	  = 1;
	ctrl->regs->VIP_HBEGIN = 12;
	ctrl->regs->VIP_HEND	  = 18;
	ctrl->regs->CLIP_STRIDEH = 0;
	ctrl->regs->CLIP_STRIDEL = 4096;
 
	ctrl->buf_index = 0;
	nx_vip_setup_memory_region(ctrl, 1);
	printk("cap_frame.planes %d\n", ctrl->cap_frame.planes);
	if (ctrl->cap_frame.planes > 1) {
		ctrl->regs->CLIP_YUYVENB = 0; /* Disnable YUV422 mode */
		if (ctrl->cap_frame.fmt->pixelformat == V4L2_PIX_FMT_YUV420M) {
			ctrl->regs->CLIP_FORMAT = 0; /* YUV 4:2:0 */
		} else 
		if (ctrl->cap_frame.fmt->pixelformat == V4L2_PIX_FMT_YUV422P) {
			printk("CLIP_FORMAT 1\n");
			ctrl->regs->CLIP_FORMAT = 1; /* YUV 4:2:2  */
		} else {
			ctrl->regs->CLIP_FORMAT = 2; /* YUV 4:4:4  */
		}
	} else {
		ctrl->regs->CLIP_YUYVENB = 1; /* Enable YUV422 linear mode */
	}
	
	if (ctrl->cap_frame.planes > 1) {
		val = 0;
		if (ctrl->use_clipper)
			val |= 2;
                   
		val |= 0x100; /* Enable Seperator */
	} else {
		val = 0;
	}
	ctrl->regs->VIP_CDENB  |= val;
	ctrl->regs->VIP_CONFIG |= 1; /* Enable VIP */
//	 printk("VIP_CDENB = %x\n", ctrl->regs->VIP_CDENB);
//	FSET_CAPTURE(ctrl);
	dump_register(ctrl);	
	printk("%s exit\n", __func__);	
}

void nx_vip_stop_vip(struct nx_vip_control *ctrl)
{
	printk("%s enter\n", __func__);
    /* Stream Off */
    //info("stop vip\n");
	ctrl->regs->VIP_CONFIG &= ~1; /* Disable VIP */
    ctrl->regs->VIP_CDENB  &= ~(0x100 | 0x03); /* Disable Seperator, Clipper and Decimator */
	if (ctrl->use_scaler) {
		while(SCALER_ISBUSY())
			msleep(1);
	}
	printk("%s exit\n", __func__);	
}

void nx_vip_restart_vip(struct nx_vip_control *ctrl)
{
	nx_vip_stop_vip(ctrl);
	nx_vip_start_vip(ctrl);
}

#if 0
int nx_vip_change_resolution(struct nx_vip_control *ctrl,
					enum nx_vip_cam_res_t res)
{
	struct nx_vip_camera *cam = ctrl->in_cam;

//	nx_vip_stop_decimator(ctrl);
	nx_vip_i2c_command(ctrl, I2C_CAM_RESOLUTION, res);

	switch (res) {
	case CAM_RES_QVGA:
		info("resolution changed to QVGA (320x240) mode\n");
		cam->cur_width = 320;
		cam->cur_height = 240;
		break;

	case CAM_RES_QSVGA:
		info("resolution changed to QSVGA (400x300) mode\n");
		cam->cur_width = 400;
		cam->cur_height = 300;
		break;

	case CAM_RES_VGA:
		info("resolution changed to VGA (640x480) mode\n");
		cam->cur_width = 640;
		cam->cur_height = 480;
		break;

	case CAM_RES_SVGA:
		info("resolution changed to SVGA (800x600) mode\n");
		cam->cur_width = 800;
		cam->cur_height = 600;
		break;

	case CAM_RES_XGA:
		info("resolution changed to XGA (1024x768) mode\n");
		cam->cur_width = 1024;
		cam->cur_height = 768;
		break;

	case CAM_RES_SXGA:
		info("resolution changed to SXGA (1280x1024) mode\n");
		cam->cur_width = 1280;
		cam->cur_height = 1024;
		break;

	case CAM_RES_UXGA:
		info("resolution changed to UXGA (1600x1200) mode\n");
		cam->cur_width = 1600;
		cam->cur_height = 1200;
		break;

	case CAM_RES_DEFAULT:
		cam->cur_width = cam->def_width;
		cam->cur_height = cam->def_height;
		break;
	case CAM_RES_MAX:
		cam->cur_width = cam->max_width;
		cam->cur_height = cam->max_height;
		break;
	default:
		/* nothing to do */
		return -1;
	}
	return 0;
}
#endif

static irqreturn_t nx_vip_irq(int irq, void *dev_id)
{
	struct nx_vip_control *ctrl = (struct nx_vip_control *)dev_id;
	enum v4l2_field field;	
#if USE_VB2
	struct vb2_v4l2_buffer *vbuf = &ctrl->cur_frm->vb;

	struct vb2_buffer *vb = &vbuf->vb2_buf;	
	struct nx_video_buffer *buf = to_vip_vb(vbuf);
#endif
//	struct NX_GPIO_RegisterSet *gpioa = (struct NX_GPIO_RegisterSet *)IO_ADDRESS(PHY_BASEADDR_GPIO);
//	struct NX_GPIO_RegisterSet *gpioa = (struct NX_GPIO_RegisterSet *)ioremap_nocache(PHY_BASEADDR_GPIO,40);
	int status,st2;
	printk("=====================%s enter\n", __func__);
	spin_lock(&ctrl->irqlock);	
	status = ctrl->regs->VIP_HVINT;
	st2 = ctrl->regs->VIP_ODINT;
	ctrl->regs->VIP_HVINT |= 3; /* All pending IRQ clear */
	ctrl->regs->VIP_ODINT |= 1;

//    printk("==========status %d st2 %d ctrl->use_clipper %d\n", status, st2,ctrl->use_clipper);
	if ((status & 1)) { /* VSYNC */
//	    if (IS_CAPTURE(ctrl)) {  
		{  
			if (ctrl->use_clipper == 0) {
//				FSET_HANDLE_IRQ(ctrl);
				ctrl->buf_index++;
				if (ctrl->buf_index >= ctrl->cap_frame.nr_frames)
					ctrl->buf_index = 0;
				while (ctrl->cap_frame.skip_frames[ctrl->buf_index]) {
					ctrl->buf_index++;
					if (ctrl->buf_index >= ctrl->cap_frame.nr_frames)
						ctrl->buf_index = 0;
				}
				spin_unlock(&ctrl->irqlock);				
				nx_vip_setup_memory_region(ctrl, 0);
		//		wake_up_interruptible(&ctrl->waitq);
			}
		}
	}                                        
	if (ctrl->use_clipper) {
		if ((st2 & 1)) { /* CLIPPER/DECIMATOR DONE */
//			FSET_HANDLE_IRQ(ctrl);
			
	        ctrl->buf_index++;
	        if (ctrl->buf_index >= ctrl->cap_frame.nr_frames)
				ctrl->buf_index = 0;
	        while (ctrl->cap_frame.skip_frames[ctrl->buf_index]) {
				ctrl->buf_index++;
				if (ctrl->buf_index >= ctrl->cap_frame.nr_frames)
					ctrl->buf_index = 0;
			}
			spin_unlock(&ctrl->irqlock);
	        nx_vip_setup_memory_region(ctrl, 0);
// buffer done
#if USE_VB2			
			if(!list_empty(&ctrl->bufs)){
				//vip_vb2_buffer_complete(ctrl);

				spin_lock(&ctrl->irqlock);	
				vb->timestamp = ktime_get_ns();
				vbuf->sequence = ctrl->buf_index++;
				vb2_buffer_done(vb, VB2_BUF_STATE_DONE);
				ctrl->cur_frm = list_entry(ctrl->bufs.next, struct nx_video_buffer, list);
				list_del_init(&ctrl->cur_frm->list);	
				
			}
			spin_unlock(&ctrl->irqlock);
#endif
		
		/*	buf->dma_addr[0] = vb2_dma_contig_plane_dma_addr(&ctrl->cur_frm->vb.vb2_buf,0);
			buf->dma_addr[1] = vb2_dma_contig_plane_dma_addr(&ctrl->cur_frm->vb.vb2_buf,1);
			buf->dma_addr[2] =	vb2_dma_contig_plane_dma_addr(&ctrl->cur_frm->vb.vb2_buf,2);
		
			printk("dma_addr 0x%X 0x%X 0x%X\n", buf->dma_addr[0], buf->dma_addr[1], buf->dma_addr[2]);	*/					
			
		/*		spin_lock(&ctrl->irqlock);
			
		if(!list_empty(&ctrl->bufs) && (ctrl->cur_frm == ctrl->next_frm)){
				//vip_vb2_next_buffer(ctrl);
				struct nx_video_buffer *buf;	
	
				
				ctrl->next_frm = list_entry(ctrl->bufs.next, struct nx_video_buffer, list);
				
				list_del(&ctrl->next_frm->list);
				buf = ctrl->next_frm;
			
				buf->dma_addr[0] = vb2_dma_contig_plane_dma_addr(&ctrl->cur_frm->vb.vb2_buf,0);
				buf->dma_addr[1] = vb2_dma_contig_plane_dma_addr(&ctrl->cur_frm->vb.vb2_buf,1);
				buf->dma_addr[2] =	vb2_dma_contig_plane_dma_addr(&ctrl->cur_frm->vb.vb2_buf,2);
				printk("dma_addr 0x%X 0x%X 0x%X\n", buf->dma_addr[0], buf->dma_addr[1], buf->dma_addr[2]);		
				
			}
			spin_unlock(&ctrl->irqlock);
			*/

// if cur_frm == next_frm is
// next buffer	        

#if 0
			spin_lock(&ctrl->irqlock);
			if(!list_empty(&ctrl->bufs)){
				int i;
				struct nx_video_frame *frame;
				struct vb2_v4l2_buffer *vbuf = &ctrl->cur_frm->vb;
				struct vb2_buffer *vb = &vbuf->vb2_buf;
				struct nx_video_buffer *buf;	
				frame = &ctrl->cap_frame;
				
				//buf_prepare
				printk("vb 0x%X  vbuf 0x%X\n", vb, vbuf);
				buf = (struct nx_video_buffer *)vb;
				
				ctrl->next_frm = list_entry(ctrl->bufs.next, struct nx_video_buffer, list);
				ctrl->cur_frm = ctrl->next_frm;
				list_del(&ctrl->next_frm->list);
				buf = ctrl->cur_frm;
			//	for (i = 0; i < frame->fmt->num_sw_planes; i++) {
					buf->dma_addr[0] = vb2_dma_contig_plane_dma_addr(&ctrl->cur_frm->vb.vb2_buf,0);
					buf->dma_addr[1] = vb2_dma_contig_plane_dma_addr(&ctrl->cur_frm->vb.vb2_buf,1);
					buf->dma_addr[2] =	vb2_dma_contig_plane_dma_addr(&ctrl->cur_frm->vb.vb2_buf,2);
					printk("dma_addr 0x%X 0x%X 0x%X\n", buf->dma_addr[0], buf->dma_addr[1], buf->dma_addr[2]);
				//}
				//vpfe_schedule_next_buffer(ctrl);
			}			
			spin_unlock(&ctrl->irqlock);
#endif
		//	wake_up_interruptible(&ctrl->waitq);
	    }
	}
   	printk("%s exit\n", __func__);
	return IRQ_HANDLED;
}


static irqreturn_t nx_scaler_irq(int irq, void *dev_id)
{
	struct nx_scaler *scaler = (struct nx_scaler *) dev_id;
	struct nx_vip_control *ctrl = scaler->current_ctrl;
	int bank = ctrl->scaler->current_bank;
	int width  = ctrl->out_frame.width;
	int height = ctrl->out_frame.height;
	int clipw = ctrl->out_frame.scw;
	int cliph = ctrl->out_frame.sch;
	struct nx_vip_frame_addr *addr;      

	scaler->regs->SCINTREG |= 0x100;             
	/*if (!IS_CAPTURE(ctrl))
	{		scaler->started = 0;
		return IRQ_HANDLED;
	}  */
	scaler->done++;
	addr = &ctrl->out_frame.addr[bank];
	if (ctrl->out_frame.format == FORMAT_YCBCR422) {
		clipw >>= 1;
		width >>=1;
	} else
	if (ctrl->out_frame.format == FORMAT_YCBCR420) {
		clipw >>=1;
		width >>=1;
		cliph >>=1;
		height >>=1;
	} else
	if (ctrl->out_frame.format == FORMAT_YCBCR444) {
		// do nothing...
	}

	switch (scaler->done) {
	case 1:	// Y complete
		SCALER_SETIMAGESIZE(clipw, cliph, width, height);
		SCALER_SETSRCADDR(addr->src_addr_cb);
		SCALER_SETDSTADDR(addr->dst_addr_cb);
		ctrl->scaler->regs->SCRUNREG = 1;
		break;
	case 2: // Cb complete
		SCALER_SETIMAGESIZE(clipw, cliph, width, height);
		SCALER_SETSRCADDR(addr->src_addr_cr);
		SCALER_SETDSTADDR(addr->dst_addr_cr);
		ctrl->scaler->regs->SCRUNREG = 1;
		break;
	case 3: // Cr complete
		if (ctrl->auto_laser_ctrl)
		{
			if (ctrl->buf_index == 3)
			{
				//FSET_SCALER_IRQ(ctrl);
			}
		}
		else
		{
			//FSET_SCALER_IRQ(ctrl);
		}

		scaler->started = 0;

		wake_up_interruptible(&scaler->waitq);
		break;
	}

	return IRQ_HANDLED;
}



static int nx_vip_parse_dt(struct platform_device *pdev, struct device_node *np, int id, struct nx_vip_control *ctrl)
{
	int ret,val;
	struct device *dev = &pdev->dev;
	//struct device_node *np = dev->of_node;
	struct device_node *np_remote, *np_scaler, *ep, *np_reserved;
//	struct nx_platform_vip *pdata = pdata;
//	struct nx_vip_control *ctrl = ctrl;	
	struct resource res, res_scaler, res_reserved;
	struct pinctrl_dev *pin;
	struct pinctrl_state *state;
	char clk_names[5] = {0, };
	char reset_names[12] = {0, };
	int retval;
	unsigned int size;
	u32 clk_base;
	printk("================%s enter \n", __func__);
	ctrl->vip_dt_data.id = id;
	ctrl->vip_dt_data.gpio_reset = devm_gpiod_get_optional(&pdev->dev, "gpio_reset", GPIOD_OUT_LOW);
	if (NULL == ctrl->vip_dt_data.gpio_reset) {   
		pr_err("%s: gpio gpio_reset is not assigned.\n", __func__);
	} else if (IS_ERR(ctrl->vip_dt_data.gpio_reset)) {
		pr_err("%s: gpio gpio_reset read is failed.\n", __func__);
	} else {
		// successful read
		gpiod_direction_output(ctrl->vip_dt_data.gpio_reset, 1);
		gpiod_set_value(ctrl->vip_dt_data.gpio_reset, 0);
		retval = gpiod_get_value(ctrl->vip_dt_data.gpio_reset);             
		printk("==============retval %d\n", retval);
		ctrl->gpio_reset = retval;
		
	/*	gpiod_set_value(pdata->gpio_reset, 1);
		retval = gpiod_get_value(pdata->gpio_reset);
		printk("==============retval1 %d\n", retval);		*/
	}	
	np = pdev->dev.of_node;
	printk("np 0x%X pdev->dev.of_node 0x%X\n", np, pdev->dev.of_node);
	
	np_reserved = of_parse_phandle(np, "memory-region", 0);
	if (!np_reserved) {
		pr_err("=============================%s: memory-region of_parse_phandle failed.\n", __func__);
	}
	
	ret = of_address_to_resource(np_reserved, 0, &res_reserved);
	if (ret)
		return ret;
	
	ctrl->vip_dt_data.dma_mem = res_reserved.start;
	ctrl->vip_dt_data.dma_mem_size = resource_size(&res_reserved);
	printk("dma_mem 0x%X dma_mem_size 0x%X\n", ctrl->vip_dt_data.dma_mem,ctrl->vip_dt_data.dma_mem_size);
	
	
	/////////////////////////
	
	if (of_property_read_u32(np, "source_sel", &ctrl->source_sel)) {
		dev_err(dev, "failed to get dt source_sel\n");
		return -EINVAL;
	}
	
	if (of_property_read_u32(np, "use_scaler", &ctrl->use_scaler)) {
		dev_err(dev, "failed to get dt use_scaler\n");
		return -EINVAL;
	}	
	printk("=============use_scaler %d\n",ctrl->use_scaler);
	
	/*ret = of_property_read_u32(np, "id", &id);
	printk("id1 %d\n", id);*/
	
#if 1
//	np = of_find_matching_node(NULL,nx_vip_of_match);
	ret = of_address_to_resource(np, 0, &res);
	if (ret) {
		dev_err(dev, "failed to get vip base address\n");
		return -ENXIO;
	}
	printk("vip base addr 0x%X\n", res.start);
	ctrl->regs = (struct NX_VIP_RegisterSet *) ioremap_nocache(res.start, resource_size(&res));
	if (!ctrl->regs) {
		dev_err(dev, "failed to ioremap vip regs \n");
		return -EBUSY;
	}	
	printk("vip base addr remap 0x%X\n", ctrl->regs);
	
#else
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);

	ctrl->regs = devm_ioremap_resource(dev, res);
	if (!ctrl->regs) {
		dev_err(dev, "failed to ioremap\n");
		return -EBUSY;
	}
#endif
	ret = of_irq_get(np, 0);
	if (ret < 0) {
		dev_err(dev, "failed to get irq num\n");
		return -EBUSY;
	}
	ctrl->irq = ret;
	printk("irq %d\n", ret);

	if (id >= NX_VIP_MAX_CTRLS) {
		dev_err(dev, "invalid module: %d\n", id);
		return -EINVAL;
	}

	snprintf(clk_names, sizeof(clk_names), "vipclk%d", id);
	ctrl->clock = of_clk_get(np, 0);
	printk("clock %d\n", ctrl->clock);
	if (IS_ERR(ctrl->clock)) {
		dev_err(dev, "failed to of_clk_get for %s\n", clk_names);
		return -ENODEV;
	}
/*
	snprintf(reset_names, sizeof(reset_names), "vip%d-reset", id);
	vip_cap->vip_rst = __of_reset_control_get(np, reset_names, 0, 0);
	//vip_cap->vip_rst = devm_reset_control_get(dev, reset_names); 
	if (IS_ERR(vip_cap->vip_rst)) {
		dev_err(dev, "failed to get reset control\n");
		return -ENODEV;
	}
*/
	/* get clock base from dt */
	if (0 == of_property_read_u32(np, "clk-base", &clk_base)) {
		printk("clk-base 0x%X\n", ctrl->clk_base);		
		ctrl->clk_base = ioremap_nocache(clk_base, 0x1000);
		printk("clk-base remap 0x%X\n", ctrl->clk_base);
		
	} else {
		ctrl->clk_base = NULL;
	}

	printk("==================##scaler============\n");
//scaler	
#if 0
	res_scaler = platform_get_resource(pdev, IORESOURCE_MEM, 0);

	ctrl->scaler->regs = (struct NX_VIP_RegisterSet *)devm_ioremap_resource(dev, &res_scaler);
	if (!ctrl->scaler->regs) {
		dev_err(dev, "failed to ioremap\n");
		return -EBUSY;
	}
#else
    
    np_scaler = of_find_compatible_node(NULL, NULL, "nexell,nxp2120-scaler");
	if (!np_scaler){
		printk("error==========");
	}else{
		printk("np_scaler 0x%X name %s full_name %s\n", np_scaler, np_scaler->name, np_scaler->full_name);
	}
    

	ret = of_address_to_resource(np_scaler, 0, &res_scaler);
	if (ret) {
		dev_err(dev, "scaler failed to get base address\n");
		return -ENXIO;
	}
	printk("scaler base addr 0x%X\n", res_scaler.start);
	ctrl->scaler->regs = (struct NX_SCALER_RegisterSet *) ioremap_nocache(res_scaler.start, resource_size(&res_scaler));
	if (!ctrl->scaler->regs) {
		dev_err(dev, "failed to ioremap scaler regs \n");
		return -EBUSY;
	}
#endif
	ret = of_irq_get(np_scaler, 0);
	if (ret < 0) {
		dev_err(dev, "failed to get scaler irq num\n");
		return -EBUSY;
	}
	printk("scaler irq %d\n", ret);	
	ctrl->scaler->irq = ret;

	

#if 0
	/* common property */
	if (of_property_read_u32(ep, "data_order", &vip_cap->vin_data.data_order)) {
		dev_err(dev, "failed to get dt data_order\n");
		return -EINVAL;
	}
	if (of_property_read_u32(ep, "monochrome", &val)) {
		dev_err(dev, "failed to get dt monochrome\n");
		return -EINVAL;
	}
	vip_cap->vin_data.monochrome = (bool) val;
#endif
	printk("scaler done\n");
	printk("np->name %s\n", np->name);
	
	ep = of_graph_get_next_endpoint(np, NULL);
	printk("ep 0x%X\n", ep);
	if (!ep)  {
		dev_err(dev, "endpoint device at %s not found\n", ep->full_name);
		return -EINVAL;
	}
	printk("ep name %s\n", ep->full_name);
	
	if (of_property_read_u32(ep, "data_order",&ctrl->vip_dt_data.data_order)) {
		dev_err(dev, "failed to get dt data_order\n");
		return -EINVAL;
	}
	printk("data_order %d\n",ctrl->vip_dt_data.data_order);	
	if (of_property_read_u32(ep, "external_sync",&ctrl->vip_dt_data.external_sync)) {
		dev_err(dev, "failed to get dt external_sync\n");
		return -EINVAL;
	}
	printk("external_sync %d\n",ctrl->vip_dt_data.external_sync);	
	
	if (of_property_read_u32(ep, "h_frontporch", &ctrl->vip_dt_data.h_frontporch)) {
		dev_err(dev, "failed to get dt h_frontporch\n");
		return -EINVAL;
	}
	printk("h_frontporch %d\n",ctrl->vip_dt_data.h_frontporch);	
	
	if (of_property_read_u32(ep, "h_syncwidth", &ctrl->vip_dt_data.h_syncwidth)) {
		dev_err(dev, "failed to get dt h_syncwidth\n");
		return -EINVAL;
	}
	printk("h_syncwidth %d\n",ctrl->vip_dt_data.h_syncwidth);	
	
	if (of_property_read_u32(ep, "h_backporch", &ctrl->vip_dt_data.h_backporch)) {
		dev_err(&pdev->dev, "failed to get dt h_backporch\n");
		return -EINVAL;
	}
	printk("h_backporch %d\n",ctrl->vip_dt_data.h_backporch);	
	
	if (of_property_read_u32(ep, "v_backporch", &ctrl->vip_dt_data.v_backporch)) {
		dev_err(&pdev->dev, "failed to get dt v_backporch\n");
		return -EINVAL;
	}
	printk("v_backporch %d\n",ctrl->vip_dt_data.v_backporch);	
	
	if (of_property_read_u32(ep, "v_syncwidth", &ctrl->vip_dt_data.v_syncwidth)) {
		dev_err(dev, "failed to get dt v_syncwidth\n");
		return -EINVAL;
	}
	printk("v_syncwidth %d\n",ctrl->vip_dt_data.v_syncwidth);	
	
	if (of_property_read_u32(ep, "clock_invert", &val)) {
		dev_err(dev, "failed to get dt clock_invert\n");
	}else{
		ctrl->vip_dt_data.clock_invert = (bool)val;
		printk("read dt clock_invert %d\n",val);
	}
	
	if (ret = of_property_read_u32(ep, "interlace", &val)) {
		dev_err(dev, "failed to get dt interlace\n");
	}else{
		ctrl->vip_dt_data.interlace = (bool)val;
		printk("read dt interlace %d\n",val);
	}
	
	if (of_property_read_u32(ep, "power_enable", &ctrl->vip_dt_data.power_enable)) {
		dev_err(dev, "failed to get dt power_enable\n");
		return -EINVAL;
	}
	printk("power_enable %d\n",ctrl->vip_dt_data.power_enable);	
	
	if (of_property_read_u32(ep, "max_width", &ctrl->vip_dt_data.max_width)) {
		dev_err(dev, "failed to get dt max_width\n");
		return -EINVAL;
	}
	printk("max_width %d\n",ctrl->vip_dt_data.max_width);	
	
	if (of_property_read_u32(ep, "max_height", &ctrl->vip_dt_data.max_height)) {
		dev_err(dev, "failed to get dt max_height\n");
		return -EINVAL;
	}
	printk("max_height %d\n",ctrl->vip_dt_data.max_height);	

	if (of_property_read_u32(ep, "def_width", &ctrl->vip_dt_data.def_width)) {
		dev_err(dev, "failed to get dt def_width\n");
		return -EINVAL;
	}
	printk("def_width %d\n",ctrl->vip_dt_data.def_width);	
	
	if (of_property_read_u32(ep, "def_height", &ctrl->vip_dt_data.def_height)) {
		dev_err(dev, "failed to get dt def_height\n");
		return -EINVAL;
	}
	printk("def_height %d\n",ctrl->vip_dt_data.def_height);	
	
	
	
	/*np_node = of_find_compatible_node(NULL, NULL, "nexell,nxp2120-vip");
	printk("np name1 %s np 0x%X\n", np_node->full_name, np_node);
	if(!np_node){
		printk("=======================error np1 0x%X\n", np_node);	
	}*/
	
	np_remote = of_graph_get_remote_port_parent(ep);

	if(!np_remote){
		dev_err(&pdev->dev, "Remote device at %s not found\n", ep->full_name);
		of_node_put(ep);
		return 0;
	}else{
		printk("np_remote 0x%X full_name %s\n", np_remote->full_name);
	}
	
	
	if (1 == id) {
		int num_sda, num_scl;
		struct gpio_desc *sda, *scl;
		num_sda = of_get_named_gpio(np_remote->parent, "gpios", 0);
		num_scl = of_get_named_gpio(np_remote->parent, "gpios", 1);
		pr_debug("num_sda = %d\n", num_sda);
		pr_debug("num_scl = %d\n", num_scl);
		sda = gpio_to_desc(num_sda);
		scl = gpio_to_desc(num_scl);
//		cap_i2c_init(sda, scl);
	}
	
	
	ctrl->asd[0] = devm_kzalloc(&pdev->dev, sizeof(struct v4l2_async_subdev),
		                     GFP_KERNEL);
	if(!ctrl->asd[0]){
		of_node_put(np_remote);
		ctrl->asd[0]= NULL;
		dev_err(&pdev->dev, "np_remote put error\n");
		of_node_put(ep);
		return 0;
	}else{
		printk("ctrl->asd 0x%X\n", ctrl->asd[0]);
	}
	
	ctrl->asd[0]->match_type = V4L2_ASYNC_MATCH_OF;
	ctrl->asd[0]->match.of.node = np_remote;
	of_node_put(np_remote);
	of_node_put(np_scaler);
	of_node_put(ep);
	printk("================%s exit\n", __func__);
	return 0;
}

static void nx_vip_enable(struct nx_vip_control *ctrl)
{
	printk("%s enter\n", __func__);
	/* Enable Clock */
	ctrl->regs->VIPCLKENB = 0x0F; /* PCLK_ALWAYS and BCLK_DYNAMIC */
	ctrl->regs->VIPCLKGEN[0][0] = 0x8000 | (0x03 << 2); /* OUTDISABLE, ICLK */
	/* Configuration */
	ctrl->regs->VIP_CONFIG &= ~(0x03<<2);	
	ctrl->regs->VIP_CONFIG = 0x02; //8bit interface,
	ctrl->regs->VIP_CONFIG |= (ctrl->vip_dt_data.data_order << 2); // YCbYCr CrYCbY order

	printk("%s exit\n", __func__);	
}



static int nx_vip_unregister_controller(struct platform_device *pdev)
{
	struct nx_vip_control *ctrl;
//	struct nx_platform_vip *pdata;
	int id = pdev->id;

	ctrl = &nx_vip.ctrl[id];

	nx_vip_free_frame_memory(&ctrl->cap_frame);

//	pdata = to_vip_plat(ctrl->dev);

	iounmap(ctrl->regs);

	memset(ctrl, 0, sizeof(*ctrl));

	return 0;
}

static int nx_vip_mmap(struct file* file, struct vm_area_struct *vma)
{
	struct nx_vip_control *ctrl = video_drvdata(file);//filp->private_data;
	struct nx_video_frame *frame = &ctrl->cap_frame;
	static int plane_n=0;

	u32 size = vma->vm_end - vma->vm_start;
	u32 pfn;
	
	//u32 total_size = frame->size[0]+frame->size[1]+frame->size[2];//buf_size; //??????????
	u32 total_size = frame->size[plane_n];

	//u32 total_size = frame->size[0];	
	printk("%s enter size[%d] %d, size %d\n", __func__, plane_n, total_size, size );
	plane_n++;
	if(plane_n > ctrl->cap_frame.fmt->num_sw_planes){
		plane_n = 0;
	}	
	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
	vma->vm_flags |= VM_DONTEXPAND | VM_DONTDUMP;//VM_RESERVED;

	/* page frame number of the address for a source frame to be stored at. */
	pfn = __phys_to_pfn(frame->addr[vma->vm_pgoff].phys_y);

	if (size > total_size) {
		err("the size of mapping is too big\n");
		return -EINVAL;
	}

	if ((vma->vm_flags & VM_WRITE) && !(vma->vm_flags & VM_SHARED)) {
		err("writable mapping must be shared\n");
		return -EINVAL;
	}

	if (remap_pfn_range(vma, vma->vm_start, pfn, size, vma->vm_page_prot)) {
		err("mmap fail\n");
		return -EINVAL;
	}
	printk("%s exit\n", __func__);
	return 0;
}

static unsigned int nx_vip_poll(struct file *file, struct poll_table_struct *wait)
{
	struct nx_vip_control *ctrl =  video_drvdata(file);//filp->private_data;
	int ret;
	u32 mask = 0;
	pr_debug("%s enter\n", __func__);

#if USE_VB2
	ret = vb2_poll(&ctrl->vb2_q, file, wait);
#endif
	pr_debug("%s enter\n", __func__);
	return ret;
}
#if 1
static
ssize_t nx_vip_read(struct file *file, char *buf, size_t count, loff_t *pos)
{
	struct nx_vip_control *ctrl = video_drvdata(file);//filp->private_data;
	size_t end;
	char *ptr, *pp, *bf;
	int ww,hh, i;
	int ret;
	int bank;
	struct nx_vip_frame_addr *addr;
	
	printk("%s enter ctrl->cap_frame.width %d \n", __func__,ctrl->cap_frame.width  );
#if 0
	if (ctrl->use_scaler) {
		if (!IS_IRQ_HANDLING(ctrl) || ctrl->sc_done != 3) {
			if (wait_event_interruptible(ctrl->waitq, IS_IRQ_HANDLING(ctrl) && (ctrl->sc_done == 3)))
					return -ERESTARTSYS;
		}
		FSET_STOP(ctrl);
	} else {
		if (!IS_IRQ_HANDLING(ctrl)) {
			if (wait_event_interruptible(ctrl->waitq, IS_IRQ_HANDLING(ctrl)))
					return -ERESTARTSYS;
		}

		FSET_STOP(ctrl);
	}
#else
#if 0
	if (!IS_IRQ_HANDLING(ctrl)) {
		if (wait_event_interruptible(ctrl->waitq, IS_IRQ_HANDLING(ctrl)))
				return -ERESTARTSYS;
	}
#endif
#endif

	//end = min_t(size_t, ctrl->cap_frame.buf_size, count);
//	end = min_t(size_t, ctrl->cap_frame.size[0],count);//+ctrl->cap_frame.size[1]+ctrl->cap_frame.size[2], count); ///??????
	end = min_t(size_t, ctrl->cap_frame.size[0]+ctrl->cap_frame.size[1]+ctrl->cap_frame.size[2], count); ///??????
	//ptr = nx_vip_get_current_frame(ctrl);
	spin_lock(&ctrl->irqlock);		
	bank = (ctrl->buf_index-1);
	if (bank < 0)
		bank = ctrl->cap_frame.nr_frames-1;
	
	// dequeue buffer
	ctrl->cap_frame.skip_frames[bank] = 1;
	addr = &ctrl->cap_frame.addr[bank];
	spin_unlock(&ctrl->irqlock);	
	if (ctrl->use_scaler) {
		// start scaler
		//printk("USE SCALER!!\n");
		ctrl->scaler->current_bank = bank;
		ctrl->scaler->current_ctrl = ctrl;
	
		if (ctrl->scaler->started) {
			printk("Scaler started?\n");
			/* if another process using scaler, wait till it goes free. */
		//	if (wait_event_interruptible(ctrl->scaler->waitq, IS_SCALER_HANDLING(ctrl)))
		//		return -ERESTARTSYS;
		}
		if ((ctrl->scaler->started == 0)) {
			ctrl->scaler->done = 0;
			SCALER_SETIMAGESIZE(ctrl->in_frame.width, ctrl->in_frame.height, ctrl->out_frame.width, ctrl->out_frame.height);
			SCALER_SETSRCADDR(addr->src_addr_lu);
			SCALER_SETDSTADDR(addr->dst_addr_lu);
			//printk("0: src=%x, dst=%x\n", ctrl->out_frame.addr[bank].phys_y, ctrl->out_frame.addr[bank].phys_y + ctrl->scaler->offset);
			/*printk("run:%X\n", ctrl->scaler->regs->SCRUNREG);
			printk("cfg:%X\n", ctrl->scaler->regs->SCCFGREG	);
			printk("int:%X\n", ctrl->scaler->regs->SCINTREG	);
			printk("srca:%X\n", ctrl->scaler->regs->SCSRCADDREG);
			printk("srcs:%X\n", ctrl->scaler->regs->SCSRCSIZEREG);
			printk("dsta:%X\n", ctrl->scaler->regs->SCDESTADDREG);
			printk("dsts:%X\n", ctrl->scaler->regs->SCDESTSIZEREG);
			printk("dex:%X\n", ctrl->scaler->regs->DELTAXREG);
			printk("dey:%X\n", ctrl->scaler->regs->DELTAYREG);
			printk("hys:%X\n", ctrl->scaler->regs->HVSOFTREG);
			printk("clk:%X\n", ctrl->scaler->regs->CLKENB); */
			ctrl->scaler->regs->SCINTREG |= 0x100;
			ctrl->scaler->regs->SCINTREG |= 0x10000;
			ctrl->scaler->regs->SCRUNREG = 1;
			ctrl->scaler->started = 1;
		//	if (wait_event_interruptible(ctrl->scaler->waitq, IS_SCALER_HANDLING(ctrl)))
		//		return -ERESTARTSYS;
		} else {
			// something goes weird. Restart?
			return -EAGAIN;
		}
		
	//UNMASK_SCALER(ctrl);

		// queue buffer
		ctrl->cap_frame.skip_frames[bank] = 0;
		ptr = addr->vir_addr_lu;
	} else {
		ptr = addr->virt_y;
	}
	/* This is bit complex that stride is 4096 */
	ww = ctrl->cap_frame.width;
	hh = ctrl->cap_frame.height;
	pp = ptr;
	bf = buf;
	if (1 == ctrl->cap_frame.planes) {
	    ww *= 2;
	    for (i=0; i<hh; i++) {
	        ret = copy_to_user(bf, pp, ww);
	        bf += ww;
	        pp += 4096;
	    }
	} else {
	    /* Copy Y component */

	    for (i=0; i<hh; i++) {
	        ret = copy_to_user(bf, pp, ww);
	        bf += ww;
	        pp += 4096;
	    }
	    //pp = ptr + ctrl->out_frame.addr[bank].cb_top * 4096 +
	    //		ctrl->out_frame.addr[bank].cb_left;
	    pp = addr->vir_addr_cb;
	    //printk("ptr2=%x\n", pp);
	    /* Copy Cr/Cb component */
	    if (V4L2_PIX_FMT_YUV420 == ctrl->cap_frame.fmt->pixelformat) {
	        hh >>= 1;
	    }
	    ww >>= 1;
	    for (i=0; i<hh; i++) {
	        ret = copy_to_user(bf, pp, ww);
	        bf += ww;
	        pp += 4096;
	    }
	    //pp = ptr + ctrl->out_frame.addr[bank].cr_top * 4096 +
	    //		ctrl->out_frame.addr[bank].cr_left;
	    pp = addr->vir_addr_cr;
	    //printk("ptr3=%x\n", pp);
	    for (i=0; i<hh; i++) {
	        ret = copy_to_user(bf, pp, ww);
	        bf += ww;
	        pp += 4096;
	    }
	}
	if (!ctrl->use_scaler) {
		// queue buffer for not using scaler.
		ctrl->cap_frame.skip_frames[bank] = 0;
	} 
		
/*	if (ctrl->time_stamp_on)
	{
		if (count >= (ctrl->cap_frame.buf_size + sizeof(unsigned long)))
			ret = copy_to_user(bf, &ctrl->time_stamp[(ctrl->buf_index-1) & 0x03], sizeof(unsigned long));
	}*/
	printk("%s exit\n", __func__);
	return end;
}
#endif
#if 0
static
ssize_t nx_vip_write(struct file *filp, const char *b, size_t c, loff_t *offset)
{
	return 0;
}
#endif
static void nx_vip_init_vip_hw(struct nx_vip_control *ctrl)
{
	u32 reg;
//	struct nx_platform_vip *pdata;

//	pdata = to_vip_plat(ctrl->dev);
//	if (pdata->cfg_gpio)
//		pdata->cfg_gpio();

	/* Enable IRQ ? */
	ctrl->regs->VIP_VIP1 = ctrl->source_sel; /* This would set VIP0 for CAM0, VIP1 for CAM1. */
	reg = ctrl->regs->VIP_HVINT;
	//ctrl->regs->VIP_HVINT = reg | 0x100; /* Enable V sync IRQ */
	ctrl->regs->VIP_HVINT &= ~0x300; /* disable h/vsync */
	if (ctrl->use_clipper)
		ctrl->regs->VIP_ODINT = 0x100;
	else
		ctrl->regs->VIP_ODINT = 0x100; /* Disable Clipper/Decimator Complete IRQ */
	ctrl->regs->VIP_HVINT |= 3; /* All pending IRQ clear */
	ctrl->regs->VIP_ODINT |= 1;
	//nx_vip_reset(ctrl);
	reg = ctrl->scaler->regs->SCINTREG;
	ctrl->scaler->regs->SCINTREG = reg | 0x100;
	// Interrupt enable...
	ctrl->scaler->regs->SCINTREG = reg | 0x10000;
}

static void nx_vip_init_laser_port(void)
{
/*	struct NX_GPIO_RegisterSet *gpioa = (struct NX_GPIO_RegisterSet *)ioremap_nocache(PHY_BASEADDR_GPIO,40);

	gpioa->GPIOxOUTENB = (0x01 << 20);
    gpioa->GPIOxALTFN[1] &= (~(3 << 8));*/
}

static void nx_vip_init_scaler(struct nx_vip_control *ctrl)
{
	printk("%s enter\n", __func__);
	ctrl->scaler->regs->SCRUNREG		= 0x00000000;
	ctrl->scaler->regs->SCCFGREG		= 0x00000000;
	ctrl->scaler->regs->SCINTREG		= 0x00000100;
	ctrl->scaler->regs->SCSRCADDREG		= 0x00000000;
	ctrl->scaler->regs->SCSRCSIZEREG	= 0x00000000;
	ctrl->scaler->regs->SCDESTADDREG	= 0x00000000;
	ctrl->scaler->regs->SCDESTSIZEREG	= 0x00000000;
	ctrl->scaler->regs->DELTAXREG		= 0x00000000;
	ctrl->scaler->regs->DELTAYREG		= 0x00000000;
	ctrl->scaler->regs->HVSOFTREG		= 0x00000000;
	ctrl->scaler->regs->CLKENB			= 0x00000000;

	//ctrl->scaler->irq = INTNUM_OF_SCALER_MODULE; //NX_SCALER_GetInterruptNumber();
//	printk("ctrl->scaler->irq  %d\n", ctrl->scaler->irq );
	ctrl->scaler->regs->CLKENB	 = 0x0000000B;
	ctrl->scaler->regs->SCRUNREG = 0;

	ctrl->scaler->regs->SCCFGREG = 0x00000003;

	ctrl->scaler->done = 0;
	ctrl->scaler->started = 0;
	printk("%s exit\n", __func__);
}

static inline struct nx_vip_control *to_ctrl_stream(struct video_device *vdev)
{
	return container_of(vdev, struct nx_vip_control, vdev);
}

static int nx_vip_open(struct file *file) /* Theres no inode information since Ver 2.X */
{
//	struct video_device *vdev = video_devdata(file);
//	struct nx_vip_control *ctrl = to_ctrl_stream(vdev);	
	struct nx_vip_control *ctrl = video_drvdata(file);
	int id, ret;                                             

	printk("============%s enter ctrl 0x%X id %d\n",__func__, ctrl, ctrl->id);
	//id = MINOR(inode->i_rdev); /* now inode info is not from Open... */
	//id = iminor(file->f_path.dentry->d_inode);
	
	ret = v4l2_fh_open(file);
	
	ret = nx_capture_set_default_format(ctrl);
	// Now set active camera here... for 0? No. id
	if (1) { //0 == nx_vip_set_active_camera(ctrl, id)) {
		//nx_vip_init_camera(ctrl);
		{
#if 0
			u32 reg;

			/* Enable IRQ ? */
			ctrl->regs->VIP_VIP1 = ctrl->source_sel; /* This would set VIP0 for CAM0, VIP1 fir CAM1. */
			reg = ctrl->regs->VIP_HVINT; 
			//ctrl->regs->VIP_HVINT = reg | 0x100; /* Enable V sync IRQ */
			ctrl->regs->VIP_HVINT &= ~0x300; /* disable h/vsync */
			if (ctrl->use_clipper)
				ctrl->regs->VIP_ODINT = 0x100;
			else
				ctrl->regs->VIP_ODINT = 0x100; /* Disable Clipper/Decimator Complete IRQ */
			ctrl->regs->VIP_HVINT |= 3; /* All pending IRQ clear */
			ctrl->regs->VIP_ODINT |= 1;
			//nx_vip_reset(ctrl);
			reg = ctrl->scaler->regs->SCINTREG;
			ctrl->scaler->regs->SCINTREG = reg | 0x100;
			// Interrupt enable...
			ctrl->scaler->regs->SCINTREG = reg | 0x10000;
#else
			nx_vip_init_vip_hw(ctrl);
			if (ctrl->id == 0)
				nx_vip_init_scaler(ctrl);
#endif
		}
		if (ctrl->auto_laser_ctrl)
		{
			nx_vip_init_laser_port();
		}
/*
		if (id) {
			if (request_irq(ctrl->irq, nx_vip_irq1, 0, ctrl->name, ctrl))
				err("VIP1 request_irq failed\n");
		} else {
			if (request_irq(ctrl->irq, nx_vip_irq0, 0, ctrl->name, ctrl))
				err("VIP0 request_irq failed\n");
		}*/
	} else {
		return -EBUSY;
	}
#if 0
	/* Enable Clock */
	ctrl->regs->VIPCLKENB = 0x0F; /* PCLK_ALWAYS and BCLK_DYNAMIC */
	ctrl->regs->VIPCLKGEN[0][0] = 0x8000 | (0x03 << 2); /* OUTDISABLE, ICLK */
	/* Configuration */
	ctrl->regs->VIP_CONFIG = 0x02 | (0x00 << 2); /* 8bit interface, CrYCbY order */
#else
	nx_vip_enable(ctrl);
#endif
	// Do camera power on?
	return 0;

//resource_busy:
//	mutex_unlock(&ctrl->lock);
	printk("============%s exit\n",__func__);
	return ret;
}


static int nx_vip_release(struct file *file)
{
	struct nx_vip_control *ctrl = video_drvdata(file);
	int id;
	int ret;
	bool last_open;	
	printk("===============%s enter\n", __func__);

	mutex_lock(&ctrl->lock);
#if USE_VB2
	last_open = v4l2_fh_is_singular_file(file);
	ret = _vb2_fop_release(file, NULL);	
	
	if(last_open){
		printk("last_open\n");
		nx_vip_stop_vip(ctrl);
		nx_vip_free_frame_memory(&ctrl->cap_frame);
	}
#else
	nx_vip_stop_vip(ctrl);
	nx_vip_free_frame_memory(&ctrl->cap_frame);

#endif

//	free_irq(ctrl->irq, ctrl);

	//if ((atomic_read(&nx_vip.ctrl[0].in_use)==0) && (atomic_read(&nx_vip.ctrl[1].in_use)==0))
	printk("===============%s ret 0x%X exit\n", __func__, ret);	
	mutex_unlock(&ctrl->lock);
	return ret;
}

#if USE_VB2
/*
 * nxp_vb2_queue_setup - Callback function for buffer setup.
 * @vq: vb2_queue ptr
 * @nbuffers: ptr to number of buffers requested by application
 * @nplanes:: contains number of distinct video planes needed to hold a frame
 * @sizes[]: contains the size (in bytes) of each plane.
 * @alloc_devs: ptr to allocation context
 *
 * This callback function is called when reqbuf() is called to adjust
 * the buffer count and buffer size
 */
static int nxp_vb2_queue_setup(struct vb2_queue *vq,
			    unsigned int *num_buffers, unsigned int *num_planes,
			    unsigned int sizes[], struct device *alloc_devs[])
{
	struct nx_vip_control *ctrl = vb2_get_drv_priv(vq);	
	struct nx_video_frame *frame;
	int i;
	int ret;
	
	printk("%s enter\n", __func__);
	frame = &ctrl->cap_frame;	
	if(IS_ERR(frame))
		return PTR_ERR(frame);
		
	printk("frame->width %d frame->height %d frame->fmt->depth[i] %d\n",frame->width ,frame->height,frame->fmt->depth[0]);
	
	ret = _set_plane_size(frame, sizes);
    if (ret < 0) {
        pr_err("%s: failed to _set_plane_size()\n", __func__);
        return ret;
    }	
	*num_planes = frame->fmt->num_planes;

	printk("num_planes %d\n",frame->fmt->num_planes);
	
	for(i=0; i < *num_planes; i++){
		sizes[i] = (frame->f_width * frame->f_height * frame->fmt->depth[i])/8;
		//alloc_devs[i] = vip_cap->alloc_ctx;
		//pr_debug("allocator 0x%X 0x%X 0x%X 0x%X\n", (unsigned int)&allocators[i],(unsigned int)allocators[i],(unsigned int)vip_cap->alloc_ctx,(unsigned int)&vip_cap->alloc_ctx);
	}
	
	printk("%s exit\n", __func__);	
	return ret;
}


static int __set_plane_size(struct nx_vip_control *ctrl, struct vb2_buffer *vb)
{
	int i;

	if(ctrl->cap_frame.fmt == NULL)
		return -EINVAL;
	
	pr_info("ctrl 0x%X num_sw_planes %d\n",(unsigned int)ctrl, ctrl->cap_frame.fmt->num_sw_planes);
	ctrl->cap_frame.planes = ctrl->cap_frame.fmt->num_sw_planes;
	for(i=0; i<ctrl->cap_frame.fmt->num_sw_planes; i++){
		unsigned long size = ctrl->cap_frame.size[i];
		
		if(vb2_plane_size(vb, i) <size){
			pr_debug("size %d\n", (int)size);
			pr_err("Buffer is small (%ld < %ld)\n",
				vb2_plane_size(vb, i), size);
			return -EINVAL;
		}
		vb2_set_plane_payload(vb,i,size);
	}
	return 0;
}


static int _fill_nx_video_buffer(struct nx_vip_control *ctrl, struct nx_video_buffer *buf)
{
    int i;
    //u32 type = vb->vb2_queue->type;
    struct vb2_buffer *vb = &buf->vb;
    struct nx_video_frame *frame;
    struct vb2_plane *plane;
    bool is_separated;

    pr_info("%s enter\n", __func__);
    printk("nx_video_buffer 0x%X\n", buf);
    frame = &ctrl->cap_frame;
    is_separated = frame->fmt->is_separated;
    printk("frame 0x%X is_separated %d planes %d\n", frame, is_separated, frame->fmt->num_sw_planes);
       
    for (i = 0; i < frame->fmt->num_sw_planes; i++) {
        if (i == 0 || is_separated) {
            buf->dma_addr[i] = vb2_dma_contig_plane_dma_addr(vb, i);
            printk("dma_addr 0x%X\n", buf->dma_addr[i]);
            //buf->vir_addr[i] = plane_vaddr(vb, i);
        //    buf->vir_addr[i] = phys_to_virt(buf->dma_addr[i]);
        //    printk("vir_addr 0x%X\n", buf->vir_addr[i]);
        
        } else {
         	 buf->dma_addr[i] = vb2_dma_contig_plane_dma_addr(vb, i);
         	 printk("dma_addr 0x%X\n", buf->dma_addr[i]);
        //	 buf->vir_addr[i] = phys_to_virt(buf->dma_addr[i]);
        //	 printk("vir_addr 0x%X\n", buf->vir_addr[i]);
        }

        buf->stride[i] = frame->stride[i];
        pr_info("[BUF plane %d] paddr(0x%x), vaddr(0x%x), s(%d), size(%d)\n",
                i, buf->dma_addr[i], buf->vir_addr[i], buf->stride[i], (int)frame->size[i]);
    }
    pr_info("%s exit\n", __func__);
    return 0;
}

/*
 * nxp_vb2_buf_prepare :  callback function for buffer prepare
 * @vb: ptr to vb2_buffer
 *
 * This is the callback function for buffer prepare when vb2_qbuf()
 * function is called. The buffer is prepared and user space virtual address
 * or user address is converted into  physical address
 */
static int nxp_vb2_buf_prepare(struct vb2_buffer *vb)
{
	struct vb2_v4l2_buffer *vbuf = to_vb2_v4l2_buffer(vb);	          
	struct nx_vip_control *ctrl = vb2_get_drv_priv(vb->vb2_queue);	
	int index = vb->index;
#if USE_VB2
	struct nx_video_buffer *buf = to_vip_vb(vbuf);	
	printk("vb 0x%X  vbuf 0x%X\n", vb, vbuf);	
#endif
	printk("%s enter index %d\n", __func__, index);
	

//	buf = (struct nx_video_buffer *)vb;
	
	__set_plane_size(ctrl, vb); 
	_fill_nx_video_buffer(ctrl, buf);
	INIT_LIST_HEAD(&buf->list);
	
	printk("%s exit\n", __func__);	
	return 0;	
}

/*
 * nxp_vb2_buf_queue : Callback function to add buffer to DMA queue
 * @vb: ptr to vb2_buffer
 */
static void nxp_vb2_buf_queue(struct vb2_buffer *vb)
{
	struct vb2_v4l2_buffer *vbuf = to_vb2_v4l2_buffer(vb);
	struct nx_vip_control *ctrl = vb2_get_drv_priv(vb->vb2_queue);
	struct nx_video_buffer *buf = to_vip_vb(vbuf);	
	unsigned long flags;
	
	printk("%s enter\n", __func__);	
//	buf = (struct nx_video_buffer *)vb;
	printk("buf 0x%X\n", buf);
	spin_lock_irqsave(&ctrl->slock, flags);
	list_add_tail(&buf->list, &ctrl->bufs);
	spin_unlock_irqrestore(&ctrl->slock, flags);
	
	printk("%s exit\n", __func__);		
}

static void nxp_buf_cleanup(struct vb2_buffer *vb)
{
	struct vb2_v4l2_buffer *vbuf = to_vb2_v4l2_buffer(vb);
	struct nx_vip_control *ctrl = vb2_get_drv_priv(vb->vb2_queue);
	struct nx_video_buffer *buf = to_vip_vb(vbuf);
	unsigned long flags;

	spin_lock_irqsave(&ctrl->slock, flags);
	list_del_init(&buf->list);
	spin_unlock_irqrestore(&ctrl->slock, flags);
}


#if 1
/*
 * nxp_start_streaming : Starts the DMA engine for streaming
 * @vb: ptr to vb2_buffer
 * @count: number of buffers
 */
static int nxp_start_streaming(struct vb2_queue *vq, unsigned int count)
{
	struct nx_vip_control *ctrl = vb2_get_drv_priv(vq);
	struct nx_video_buffer *buf, *tmp;	
	unsigned long flags;
	dma_addr_t addr, addr1, addr2;
	int ret;
	printk("%s enter\n", __func__);	
	

#if 1	
	spin_lock_irqsave(&ctrl->slock, flags);
	printk("cur_frm 0x%X\n", ctrl->cur_frm);
	/* Get the next frame from the buffer queue */
	ctrl->cur_frm = list_entry(ctrl->bufs.next,
				    struct nx_video_buffer, list);
	printk("cur_frm 0x%X\n", ctrl->cur_frm);
	/* Remove buffer from the buffer queue */
	list_del_init(&ctrl->cur_frm->list);

	/*buf = ctrl->cur_frm;
	buf->dma_addr[0] = vb2_dma_contig_plane_dma_addr(&ctrl->cur_frm->vb.vb2_buf,0);
	buf->dma_addr[1] = vb2_dma_contig_plane_dma_addr(&ctrl->cur_frm->vb.vb2_buf,1);
	buf->dma_addr[2] =	vb2_dma_contig_plane_dma_addr(&ctrl->cur_frm->vb.vb2_buf,2);
	printk("dma_addr 0x%X 0x%X 0x%X\n", buf->dma_addr[0], buf->dma_addr[1], buf->dma_addr[2]);*/
		
	spin_unlock_irqrestore(&ctrl->slock, flags);
	
	ret = v4l2_subdev_call(ctrl->subdev, video, s_stream, 1);
	if (ret < 0) {
		printk("Error in attaching interrupt handle\n");
		goto err;
	}	
#endif
	printk("%s exit\n", __func__);		
	return 0;
	
err:
	list_for_each_entry_safe(buf, tmp, &ctrl->bufs, list) {
		list_del(&buf->list);
		vb2_buffer_done(&buf->vb.vb2_buf, VB2_BUF_STATE_QUEUED);
	}
	return ret;
}


static void nxp_stop_streaming(struct vb2_queue *vq)
{
	struct nx_vip_control *ctrl = vb2_get_drv_priv(vq);
	unsigned long flags;
	int ret;
	printk("%s enter\n", __func__);
	ret = v4l2_subdev_call(ctrl->subdev, video, s_stream, 0);	
	if (ret && (ret != -ENOIOCTLCMD) && ret != -ENODEV)
		v4l2_err(&ctrl->v4l2_dev, "stream off failed in subdev\n");

	
	/* release all active buffers */
	if (ctrl->cur_frm)
		vb2_buffer_done(&ctrl->cur_frm->vb.vb2_buf,
				VB2_BUF_STATE_ERROR);

	while (!list_empty(&ctrl->bufs)) {
		ctrl->cur_frm = list_entry(ctrl->bufs.next,
						struct nx_video_buffer, list);
		list_del_init(&ctrl->cur_frm->list);
		vb2_buffer_done(&ctrl->cur_frm->vb.vb2_buf,
				VB2_BUF_STATE_ERROR);	
	
	}
	
	
	
/*	spin_lock_irqsave(&ctrl->slock, flags);	
	if (ctrl->cur_frm == ctrl->next_frm) {
		vb2_buffer_done(&ctrl->cur_frm->vb.vb2_buf,
				VB2_BUF_STATE_ERROR);
	} else {
		if (ctrl->cur_frm != NULL)
			vb2_buffer_done(&ctrl->cur_frm->vb.vb2_buf,
					VB2_BUF_STATE_ERROR);
		if (ctrl->next_frm != NULL)
			vb2_buffer_done(&ctrl->next_frm->vb.vb2_buf,
					VB2_BUF_STATE_ERROR);
	}

	while (!list_empty(&ctrl->bufs)) {
		ctrl->next_frm = list_entry(ctrl->bufs.next,
						struct nx_video_buffer, list);
		list_del_init(&ctrl->next_frm->list);
		vb2_buffer_done(&ctrl->next_frm->vb.vb2_buf,
				VB2_BUF_STATE_ERROR);
	}	
	spin_unlock_irqrestore(&ctrl->slock, flags);	
*/	
	printk("%s exit\n", __func__);	
}
#endif


static const struct vb2_ops nxp_vb2_ops = {
	.queue_setup		= nxp_vb2_queue_setup,
/*	.buf_init		= mcam_vb_sg_buf_init,*/
	.buf_prepare		= nxp_vb2_buf_prepare,
	.buf_queue		= nxp_vb2_buf_queue,
	.buf_cleanup		= nxp_buf_cleanup,
	.start_streaming	= nxp_start_streaming,
	.stop_streaming		= nxp_stop_streaming,
	.wait_prepare		= vb2_ops_wait_prepare,
	.wait_finish		= vb2_ops_wait_finish,
};
#endif //USE_VB2

static const struct v4l2_file_operations nx_vip_fops = {
	.owner = THIS_MODULE,
	.open = nx_vip_open,
	.release = nx_vip_release,
/*	.ioctl = video_ioctl2,*/
	.read = nx_vip_read,//vb2_fop_read,//
/*	.write = nx_vip_write,*/
	.poll = nx_vip_poll,	
	.unlocked_ioctl	= video_ioctl2,		
#if USE_VB2
	.mmap = vb2_fop_mmap,//nx_vip_mmap,
#else
	.mmap = nx_vip_mmap,
#endif

};

static void nx_vip_vdev_release(struct video_device *vdev)
{
	printk("===============%s enter\n", __func__);
	
	kfree(vdev);
	printk("===============%s exit\n", __func__);
}


#if 0
static int nx_vip_init_global(struct platform_device *pdev)
{
	/* test pattern */
	nx_vip.camera[test_pattern.id] = test_pattern;

	return 0;
}

static struct v4l2_subdev * _register_sensor(struct nx_vip_control *ctrl,
struct nx_vip_camera_info *cam_info)//, struct nxp_v4l2_spi_board_info *spi_board_info)
{
    struct v4l2_subdev *sensor = NULL;
    struct i2c_adapter *adapter;
//    struct spi_master *master;
 //   struct media_entity *input;
    
    char *name;
    int adap_id, addr;
    
    //u32 pad;
    //u32 flags;
    //int ret;
    // psw0523 add for urbetter...
    // TODO
    static int sensor_index = 0;
    pr_debug("%s enter\n", __func__);
    adapter = i2c_get_adapter(cam_info->i2c_adaptor_id);
	if (!adapter) {
		pr_err("nx-vip-core: %s: unable to get i2c adapter %d for device %s\n",
				__func__,
				cam_info->i2c_adaptor_id,
				cam_info->board_info->type);
		return NULL;
	}
	pr_debug("nx-vip-core: %s i2c_get_adapter\n",__func__);
	sensor = v4l2_i2c_new_subdev_board(&ctrl->v4l2_dev,//me->get_v4l2_device(me),
			adapter, cam_info->board_info, NULL);
	
	pr_debug("nx-vip-core:  v4l2_i2c_new_subdev_board 0x%X\n", (unsigned int)sensor);
	
	if (!sensor) {
		pr_err("%s: unable to register subdev %s\n",
				__func__, cam_info->board_info->type);
		return NULL;
	}
	sensor->host_priv = cam_info;
	name = cam_info->board_info->type;
	adap_id = cam_info->i2c_adaptor_id;
	addr = cam_info->board_info->addr; 
	
	pr_debug("nx-vip-core:  %s %d 0x%X\n", name, adap_id, addr);

	//_set_sensor_entity_name(sensor_index, name, adap_id, addr);
    pr_debug("%s exit\n",__func__);
  //  me->vip_cap.subdev = sensor;
    return sensor;
}
#endif


static int nx_async_complete(struct v4l2_async_notifier *notifier)
{
	
	
	return 0;
}

static int nx_async_bound(struct v4l2_async_notifier *notifier, struct v4l2_subdev *subdev, struct v4l2_async_subdev *asd)
{
	struct device_node *np, *np_remote, *ep;
	struct nx_vip_control *ctrl;	
	int id = -1;
	int ret;
	printk("=========================%s enter\n", __func__);
	np = asd->match.of.node;
	
	ep = of_graph_get_next_endpoint(np, NULL);

	if (!ep)  {
		err("endpoint device at %s not found\n", ep->full_name);
		return -EINVAL;
	}else{
		printk("ep full_name %s\n", ep->full_name);
	}

	np_remote = of_graph_get_remote_port_parent(ep);

	if(!np_remote){
		err("Remote device at %s not found\n", np_remote->full_name);
		of_node_put(ep);
		return 0;
	}else{
		printk("np_remote full_name %s \n", np_remote->full_name);
	}
	
	ret = of_property_read_u32(np_remote, "id", &id);
	printk("ret %d\n", ret);
	if(id < 0){
		printk("=====================get vip id %d failed\n",id);
	}
	printk("id %d\n", id);

	ctrl = &nx_vip.ctrl[id];
	printk("==============ctrl 0x%X, ctrl->asd[0] 0x%X ctrl->asd[0]->match.of.node 0x%X, asd->match.of.node 0x%X\n" 
		,ctrl,ctrl->asd[0], ctrl->asd[0]->match.of.node,asd->match.of.node);
	
	if(ctrl->asd[0] && (ctrl->asd[0]->match.of.node == asd->match.of.node)){
		printk("subdev 0x%X\n subdev->name %s subdev->of_node->name %s", 
			subdev, &subdev->name, subdev->of_node->name);
		ctrl->subdev = subdev;
		printk("ctrl->subdev 0x%X\n", ctrl->subdev);
	}

	of_node_put(np_remote);
	
	of_node_put(ep);	
//	if(!subdev->of_node->name)
//		printk("subdev->of_node->name %s\n", subdev->of_node->name);
	printk("=========================%s exit\n", __func__);
	return 0;	
}


static
struct nx_vip_control *nx_vip_register_controller(struct platform_device *pdev, struct device_node *np, int id)
{
//	struct nx_platform_vip *pdata;
	struct nx_vip_control *ctrl;
	struct video_device *vdev;	
#if USE_VB2
	struct vb2_queue *vbq = NULL;
#endif
	char* s;


	//int i = NX_VIP_MAX_CTRLS - 1;
	int i;
	int ret;
	printk("================%s enter np 0x%X\n", __func__, np);


//	pdata = to_vip_plat(&pdev->dev);
//	printk("pdata1 0x%X\n", pdata);
	
	
//	np = pdev->dev.of_node;
	ctrl = &nx_vip.ctrl[id];
	ctrl->id = id;
	ctrl->dev = &pdev->dev;
	/* scaler is only one, so two ctrl must reference same scaler structure.*/
	ctrl->scaler = &nx_vip.scaler;	
	
	nx_vip_parse_dt(pdev, np, id, ctrl);	
	//vb2_dma_contig_set_max_seg_size(&pdev->dev, DMA_BIT_MASK(32));
#if USE_VB2	
	vbq = &ctrl->vb2_q;
	printk("===========vbq 0x%X\n",vbq);	
	vbq->type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
	vbq->io_modes = VB2_MMAP | VB2_READ |VB2_DMABUF; 
	vbq->drv_priv = ctrl;
	vbq->ops = &nxp_vb2_ops;
	vbq->mem_ops = &vb2_dma_contig_memops;
	vbq->buf_struct_size = sizeof(struct nx_video_buffer);
	vbq->timestamp_flags = V4L2_BUF_FLAG_TIMESTAMP_MONOTONIC;
	vbq->lock = &ctrl->lock;
	vbq->min_buffers_needed = 1;
	vbq->dev = &pdev->dev;

	ret = vb2_queue_init(vbq);
	printk("===========ret 0x%X\n",ret);	
	if (ret) {
		pr_err("%s: failed to vb2_queue_init()\n", __func__);
	}
	

	/* init video dma queues */
	INIT_LIST_HEAD(&ctrl->bufs);	
#endif	
	mutex_init(&ctrl->lock);
	vdev = &ctrl->vdev;
	
	vdev->v4l2_dev = &ctrl->v4l2_dev;
	vdev->fops = &nx_vip_fops;
	
	vdev->ioctl_ops = &nx_vip_v4l2_ops;
	vdev->minor = -1;
	vdev->release = video_device_release_empty;	
	vdev->lock = &ctrl->lock;
#if USE_VB2
	vdev->queue = vbq;
#endif
	video_set_drvdata(vdev, ctrl);
	printk("================vdev 0x%X\n",(unsigned int)vdev);
#if 0	
	ctrl->vd = &nx_vip_video_device[id];
	ctrl->vd->v4l2_dev = &ctrl->v4l2_dev;
	s = video_device_node_name(ctrl->vd);
	if(s)
		printk("==========video_device node_name %s\n", s);
	
	sprintf(ctrl->name, "%s%d", NX_VIP_NAME, id);
	strcpy(ctrl->vd->name, ctrl->name);
	
	
	s = video_device_node_name(ctrl->vd);
	if(s)
		printk("==========video_device node_name1 %s\n", s);
//	ctrl->vd->minor = id;
#endif
//	ctrl->out_frame.nr_frames = ctrl->vip_dt_data.buff_count;
	//ctrl->out_frame.skip_frames = pdata->skip_count;
	for (i=0; i< NX_VIP_MAX_FRAMES; i++)
		ctrl->cap_frame.skip_frames[i] = 0;
	

	ctrl->streamon = 0;

	

	atomic_set(&ctrl->in_use, 0);
	//mutex_init(&ctrl->lock);
	/*init_waitqueue_head(&ctrl->waitq);
	if (id == 0)
		init_waitqueue_head(&ctrl->scaler->waitq);
*/
//	nx_vip_parse_dt(pdev, np, id, ctrl);

#if 0	
	/* get resource for io memory */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		err("failed to get io memory region\n");
		return NULL;
	}

	/* request mem region */
	res = request_mem_region(res->start, res->end - res->start + 1, pdev->name);
	if (!res) {
		err("failed to request io memory region\n");
	    return NULL;
	}

	/* ioremap for register block */
	ctrl->regs = (struct NX_VIP_RegisterSet*)ioremap(res->start, res->end - res->start + 1);

	if (!ctrl->regs) {
		err("failed to remap io region\n");
		return NULL;
	}
#endif
//	ctrl->use_scaler = 0;
	if (id == 0) { 
		ctrl->scaler->done = 0;
		ctrl->scaler->started = 0;
	//	ctrl->scaler->regs = (struct NX_SCALER_RegisterSet*)ioremap_nocache(PHY_BASEADDR_SCALER_MODULE,0x40);//, sizeof(struct NX_SCALER_RegisterSet));
	}


	/* irq */
//	ctrl->irq = platform_get_irq(pdev, 0);

	/*if (id) {
		if (request_irq(ctrl->irq, nx_vip_irq1, IRQF_DISABLED, ctrl->name, ctrl))
			err("request_irq failed\n");
	} else {
	    if (request_irq(ctrl->irq, nx_vip_irq0, IRQF_DISABLED, ctrl->name, ctrl))
		    err("request_irq failed\n");
	}*/
	ret = devm_request_irq(&pdev->dev, ctrl->irq, nx_vip_irq,
			0, pdev->name, ctrl);	
	printk("request irq ret 0x%X\n", ret);
	if (ret) {
		dev_err(&pdev->dev, "failed to install irq (%d)\n", ret);
			//goto err_clk;
	}	

#if 0
	/* Enable Clock */
	ctrl->regs->VIPCLKENB = 0x0F; /* PCLK_ALWAYS and BCLK_DYNAMIC */
	ctrl->regs->VIPCLKGEN[0][0] = 0x8000 | (0x03 << 2); /* OUTDISABLE, ICLK */
	/* Configuration */
	ctrl->regs->VIP_CONFIG = 0x02 | (0x00 << 2); /* 8bit interface, CrYCbY order */
#else
	nx_vip_enable(ctrl);
#endif
	printk("================%s exit\n", __func__);
	return ctrl;
}

static int nx_vip_probe(struct platform_device *pdev)
{
//	struct nx_platform_vip *pdata;	
	struct nx_vip_control *ctrl;
	struct v4l2_subdev *sensor;
	struct device_node *np;	

	int ret;
	int id;

	np = pdev->dev.of_node;
	printk("np name %s np 0x%X\n", np->full_name, np);
	if(!np){
		printk("=======================error np 0x%X\n", np);	
	}
	
	ret = of_property_read_u32(np, "id", &id);
	printk("id %d\n", id);
	printk("pdev->id %d\n", pdev->id);

	/*nx_priv = devm_kzalloc(&pdev->dev, sizeof(*nx_priv), GFP_KERNEL);
	if(!nx_priv)
		return -ENOMEM;	
*/
/*	pdata = devm_kzalloc(&pdev->dev, sizeof(*pdata), GFP_KERNEL);
	if(!pdata)
		return -ENOMEM;		
	printk("pdata 0x%X\n" , pdata);	
	
	pdev->dev.platform_data = pdata;*/

//	platform_get_drvdata(// ///////////////////로 pdata가져와야 하는건지?
//	pdata->id = id;
	ctrl = nx_vip_register_controller(pdev, pdev->dev.of_node, id);
	
	printk("================ctrl 0x%X \n", ctrl);
	if (!ctrl) {
		err("cannot register nx v4l2 controller\n");
		goto err_register;
	}
	ctrl->pdev = pdev;
	

	
	
//	pdata = to_vip_plat(&pdev->dev);
	/*if (pdata->cfg_gpio)
		pdata->cfg_gpio();*/
//	printk("pdata 0x%X\n", pdata);
//	ctrl->pdata = pdata;
//	ctrl->gpio_reset = gpiod_get_value(pdata->gpio_reset);
	printk("==============ctrl->gpio_reset %d\n", ctrl->gpio_reset);
	ctrl->laser_ctrl = 0;
	ctrl->time_stamp_on = ctrl->vip_dt_data.time_stamp_on;
	ctrl->time_stamp_offset = -4; /* this is default. */
//	ctrl->source_sel = pdata->cam->in_port;

	/* things to initialize once */
	if (ctrl->id == 0) {
		/* setup no camera is registered. */
		// to do
/*		memset(&nx_vip.camera[0], 0, sizeof(struct nx_vip_camera_ex));
		memset(&nx_vip.camera[1], 0, sizeof(struct nx_vip_camera_ex));
		ret = nx_vip_init_global(pdev);
		if (ret)
			goto err_global;*/
	}
	
	ret = v4l2_device_register(&pdev->dev, &ctrl->v4l2_dev);
	if (ret) {
		goto err_v4l;
	}
	
/*	ret = devm_request_irq(&pdev->dev, ctrl->irq, nx_vip_irq0,
			0, pdev->name, ctrl);	
		
	if (ret) {
		dev_err(&pdev->dev, "failed to install irq (%d)\n", ret);
			//goto err_clk;
	}*/

	/* scaler initialize */
	if (ctrl->id == 0) {
		nx_vip_init_scaler(ctrl);
		printk("request scaler irq\n");
		if (request_irq(ctrl->scaler->irq, nx_scaler_irq, 0, "nx-scaler", ctrl->scaler)) {
				err("scaler request_irq failed\n");
		}
		printk("request scaler irq done\n");		
	}

	ret = video_register_device(&ctrl->vdev, VFL_TYPE_GRABBER, -1);
	printk("video_register_device ret 0x%X\n", ret);
	if(ret < 0){
		pr_err("%s : failed\n", __func__);
		return ret;
	}
	/* register sensor and subddev */
/*	sensor = _register_sensor(ctrl, pdata->cam);
	if (NULL != sensor) {
		nx_vip.camera[ctrl->id].subdev = sensor;
	}
	
	ctrl->cam_ex = &nx_vip.camera[ctrl->id];
	ctrl->cam_ex->cur_width = 0; //ctrl->pdata->cam->def_width;
	ctrl->cam_ex->cur_height = 0; //ctrl->pdata->cam->def_height;
	ctrl->cam_ex->def_width = ctrl->pdata->cam->def_width;
	ctrl->cam_ex->def_height = ctrl->pdata->cam->def_height;
	ctrl->cam_ex->max_width = ctrl->pdata->cam->max_width;
	ctrl->cam_ex->max_height = ctrl->pdata->cam->max_height;

	info("controller %d registered successfully\n", ctrl->id);*/

	ctrl->notifier.subdevs = &ctrl->asd[0];
	ctrl->notifier.num_subdevs = 1;
	ctrl->notifier.bound = nx_async_bound;
	ctrl->notifier.complete = nx_async_complete;
	ret = v4l2_async_notifier_register(&ctrl->v4l2_dev, &ctrl->notifier);
	if(ret){
		pr_err("Error registering async notifier\n");
		ret = -EINVAL;
		v4l2_device_unregister(&ctrl->v4l2_dev);
		return ret;
	}	
	return 0;

	
err_v4l:
	v4l2_device_unregister(&ctrl->v4l2_dev);
/*err_global:
	nx_vip_unregister_controller(pdev);*/

err_register:
	return -EINVAL;

}

static int nx_vip_remove(struct platform_device *pdev)
{
	int id = pdev->id;
	struct nx_scaler *scaler;
	printk("===============%s enter\n", __func__);
	if (id == 0) {
		scaler = &nx_vip.scaler;

		scaler->regs->SCRUNREG			= 0x00000000;
		scaler->regs->SCCFGREG			= 0x00000000;
		scaler->regs->SCINTREG			= 0x00000100;
		scaler->regs->SCSRCADDREG		= 0x00000000;
		scaler->regs->SCSRCSIZEREG		= 0x00000000;
		scaler->regs->SCDESTADDREG		= 0x00000000;
		scaler->regs->SCDESTSIZEREG	= 0x00000000;
		scaler->regs->DELTAXREG		= 0x00000000;
		scaler->regs->DELTAYREG		= 0x00000000;
		scaler->regs->HVSOFTREG		= 0x00000000;
		scaler->regs->CLKENB			= 0x00000000;

		free_irq(scaler->irq, scaler);		
	}
	nx_vip_unregister_controller(pdev);
	printk("===============%s exit\n", __func__);
	return 0;
}

#if defined(CONFIG_PM)

int nx_vip_suspend(struct device *dev)
{
	return 0;
}

int nx_vip_resume(struct device *dev)
{
#if 0
	struct platform_device *pdev = to_platform_device(dev);
	
	
#else
	struct nx_vip_control *ctrl;
//	struct i2c_client *client = v4l2_get_subdevdata(ctrl->cam_ex->subdev);
	int id = dev->id;
	printk("===============%s enter\n", __func__);
	ctrl = &nx_vip.ctrl[id];

	info("nx vip%d resume\n", ctrl->id);

	if (ctrl->gpio_reset) {
	//	ctrl->gpio_reset(ctrl->pwrdn);
	}
//	if (IS_CAPTURE(ctrl))
	{
		//nx_vip_i2c_command(ctrl, I2C_CAM_INIT, 0);
	}
	nx_vip_init_vip_hw(ctrl);

	nx_vip_enable(ctrl);

	if (ctrl->id == 0)
		nx_vip_init_scaler(ctrl);

//	if (IS_CAPTURE(ctrl))
	{
		nx_vip_start_vip(ctrl);
	}
#endif
	printk("===============%s exit\n", __func__);
	return 0;
}


static struct dev_pm_ops nx_vip_pm_ops = {
	.suspend = nx_vip_suspend,
	.resume = nx_vip_resume,
};
#endif


static const struct of_device_id nx_vip_of_match[] = {
	{.compatible = "nexell,nxp2120-vip",},
	{},
};
/*
static const struct of_device_id nx_scaler_of_match[] = {
	{.compatible = "nexell,nxp2120-scaler",},
	{},
};
*/
MODULE_DEVICE_TABLE(of, nx_vip_of_match);


static struct platform_driver nx_vip_driver = {
	.probe = nx_vip_probe,
	.remove = nx_vip_remove,
	.driver = {
		.name = "nx-vip",
#if defined(CONFIG_PM)		
		.pm = &nx_vip_pm_ops,
#endif
		.of_match_table = of_match_ptr(nx_vip_of_match),
	},
};
module_platform_driver(nx_vip_driver);


#if 0
static struct platform_driver nx_vip_driver = {
	.probe		= nx_vip_probe,
	.remove		= nx_vip_remove,
	.suspend	= nx_vip_suspend,
	.resume	    = nx_vip_resume,
	.driver		= {
		.name	= "nx-v4l2",
		.owner	= THIS_MODULE,
	},
};

static int nx_vip_register(void)
{
	platform_driver_register(&nx_vip_driver);

	return 0;
}

static void nx_vip_unregister(void)
{
	platform_driver_unregister(&nx_vip_driver);
}

module_init(nx_vip_register);
module_exit(nx_vip_unregister);
#endif
MODULE_AUTHOR("Seungwoo Kim <ksw@i4vine.com>");
MODULE_DESCRIPTION("Nexell nxp2120 v4l2 driver");
MODULE_LICENSE("GPL");
