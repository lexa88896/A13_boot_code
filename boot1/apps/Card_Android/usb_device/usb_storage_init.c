/*
**********************************************************************************************************************
*											        eGon
*						           the Embedded GO-ON Bootloader System
*									       eGON arm boot sub-system
*
*						  Copyright(C), 2006-2010, SoftWinners Microelectronic Co., Ltd.
*                                           All Rights Reserved
*
* File    : usb_storage_init.c
*
* By      :
*
* Version : V2.00
*
* Date	  :
*
* Descript:
**********************************************************************************************************************
*/
#include "usb_descriptor.h"
#include "usb_params.h"
#include "usb_storage.h"
#include "usb_register.h"

__u32 bus_irq_count = 0;
__u32 ep0_irq_count = 0;
__u32 eptx_irq_count = 0;
__u32 eprx_irq_count = 0;


usb_struct awxx_usb[USB_SIE_COUNT];

extern  __u32 usb_device_function(pusb_struct pusb);

void usb_clock_init(void)
{
	__u32 temp;
	__u32 i;

	//open phy0 sie clock
	temp = *(volatile unsigned int *)(0x1c20000 + 0x60);
	temp |= 0x01;
	*(volatile unsigned int *)(0x1c20000 + 0x60) = temp;

	//init phy
	temp = 0x01<<8;
	*(volatile unsigned int *)(0x1c20000 + 0xcc) = temp;
	for(i=0; i<0x100; i++);
	temp |= 0x01;
	*(volatile unsigned int *)(0x1c20000 + 0xcc) = temp;

	//init sram for usb0
	temp = *(volatile unsigned int *)(0x1c00000 + 0x04);
	temp |= 0x01;
	*(volatile unsigned int *)(0x1c00000 + 0x04) = temp;
}


void usb_clock_exit(void)
{
	__u32 temp;
	__u32 i;

	//init sram for usb0
	temp = *(volatile unsigned int *)(0x1c00000 + 0x04);
	temp &= ~0x01;
	*(volatile unsigned int *)(0x1c00000 + 0x04) = temp;

	//init phy
	temp = *(volatile unsigned int *)(0x1c20000 + 0xcc);
	temp &= ~(0x01 << 8);
	*(volatile unsigned int *)(0x1c20000 + 0xcc) = temp;
	for(i=0; i<0x100; i++);
	temp &= ~0x01;
	*(volatile unsigned int *)(0x1c20000 + 0xcc) = temp;

	//open phy0 sie clock
	temp = *(volatile unsigned int *)(0x1c20000 + 0x60);
	temp &= ~0x01;
	*(volatile unsigned int *)(0x1c20000 + 0x60) = temp;
}
/*
************************************************************************************************************
*
*                                             function
*
*    函数名称：
*
*    参数列表：
*
*    返回值  ：
*
*    说明    ：
*
*
************************************************************************************************************
*/
void usb_params_init(void)
{
	__u32 i;

	usb_clock_init();

	awxx_usb[0].index = 0;
	awxx_usb[0].reg_base = 	0x01c13000;
	awxx_usb[0].irq_no = 38;
	awxx_usb[0].drq_no = 0x04;

	awxx_usb[0].role = USB0_ROLE;  //USB_ROLE_HST; //USB_ROLE_UNK
	awxx_usb[0].speed = USB0_SPEED;

	if(awxx_usb[0].speed==USB_SPEED_HS)
	{
		awxx_usb[0].device.dev_desc   = USB_HS_BULK_DevDesc;
		awxx_usb[0].device.config_desc= USB_HS_BULK_ConfigDesc;
	}
	else
	{
		awxx_usb[0].device.dev_desc   = USB_FS_BULK_DevDesc;
		awxx_usb[0].device.config_desc= USB_FS_BULK_ConfigDesc;
	}
	for(i=0; i<4; i++)
	{
		awxx_usb[0].device.str_desc[i]   = USB_StrDec0[i];
	}

	awxx_usb[0].device.intf_desc  = USB_HS_BULK_ConfigDesc;
	awxx_usb[0].device.endp_desc  = USB_HS_BULK_ConfigDesc;
	awxx_usb[0].device.dev_qual   = USB_DevQual;
	awxx_usb[0].device.otg_desc   = USB_OTGDesc;

	//awxx_usb[0].device.MaxLUN = USB_MAX_LUN;
	//awxx_usb[0].device.MaxLUN = wBoot_part_count() - 1;
	awxx_usb[0].device.MaxLUN = wBoot_part_count(1) - 1;
	__inf("part count = %d\n", awxx_usb[0].device.MaxLUN + 1);

	//awxx_usb[0].device.bo_memory_base =   USB_BO_DEV0_MEM_BASE;                           // USB_BO_DEV0_MEM_BASE;
	//awxx_usb[0].device.bo_capacity = USB_DEV0_TOTAL_CAP;  //MB
	//awxx_usb[0].device.bo_seccnt = usb_get_capacity();                  //USB_DEV0_SEC_CNT;
	//awxx_usb[0].device.bo_bufbase = USB_BO_DEV0_BUF_BASE;	                                 //??
	awxx_usb[0].device.bo_memory_base = (__u32)wBoot_malloc(128 * 1024);		//use to storage user data
	awxx_usb[0].device.bo_bufbase = (__u32)wBoot_malloc(64 * 1024);				//use to usb ping-pang buffer

	awxx_usb[0].ep0_flag = 0;
	awxx_usb[0].ep0_xfer_state = USB_EP0_SETUP;

	//dma
	awxx_usb[0].dma = wBoot_dma_request(1);
	if(!awxx_usb[0].dma)
	{
		__inf("usb error: request dma fail\n");
	}
}
/*
************************************************************************************************************
*
*                                             function
*
*    函数名称：
*
*    参数列表：
*
*    返回值  ：
*
*    说明    ：
*
*
************************************************************************************************************
*/
void usb_irq_handler(pusb_struct pusb)
{
	//__u32 index;
	__u32 temp;
	__u32 i;

	//bus interrupt
	temp = usb_get_bus_interrupt_status(pusb);
	usb_clear_bus_interrupt_status(pusb, temp);
	if(temp & USB_BUSINT_SOF)
	{
		pusb->sof_count ++;
		temp &= ~USB_BUSINT_SOF;
	}

	if(temp & usb_get_bus_interrupt_enable(pusb))
	{
		pusb->busirq_status |= temp;
		pusb->busirq_flag ++;
		bus_irq_count ++;
	}

	//tx interrupt
	temp = usb_get_eptx_interrupt_status(pusb);
	usb_clear_eptx_interrupt_status(pusb, temp);
	if(temp&0x01)
	{
		pusb->ep0_flag ++;
		ep0_irq_count ++;
	}
	if(temp&0xfffe)
	{
		for(i=0; i<USB_MAX_EP_NO; i++)
		{
			if(temp & (0x2<<i)) pusb->eptx_flag[i] ++;
		}
		eptx_irq_count ++;
	}

	//rx interrupt
	temp = usb_get_eprx_interrupt_status(pusb);
	usb_clear_eprx_interrupt_status(pusb, temp);
	if(temp&0xfffe)
	{
		for(i=0; i<USB_MAX_EP_NO; i++)
		{
			if(temp & (0x2<<i))
			{
				pusb->eprx_flag[i] ++;
			}
		}
		eprx_irq_count ++;
	}

	return;
}
/*
************************************************************************************************************
*
*                                             function
*
*    函数名称：
*
*    参数列表：
*
*    返回值  ：
*
*    说明    ：
*
*
************************************************************************************************************
*/
static void usb0_irq_handler(void)
{
	usb_irq_handler(&awxx_usb[0]);
}
/*
************************************************************************************************************
*
*                                             function
*
*    函数名称：
*
*    参数列表：
*
*    返回值  ：
*
*    说明    ：
*
*
************************************************************************************************************
*/
static void reg_usb_irq_handler(void)
{
	wBoot_InsINT_Func(awxx_usb[0].irq_no, (int *)usb0_irq_handler, 0);
	wBoot_EnableInt(awxx_usb[0].irq_no);
}
/*
************************************************************************************************************
*
*                                             function
*
*    函数名称：
*
*    参数列表：
*
*    返回值  ：
*
*    说明    ：
*
*
************************************************************************************************************
*/
void usb_struct_init(pusb_struct pusb)
{
	__u32 i=0;

	pusb->sof_count = 0;

	pusb->busirq_flag = 0;
	pusb->busirq_status = 0;

	pusb->connect = 0;
	pusb->reset = 0;
	pusb->suspend = 1;

	pusb->rst_cnt = 0;
	pusb->cur_fsm = 0;
	pusb->fsm_cnt = 0;

	pusb->ep0_flag = 0;
	pusb->ep0_xfer_state = USB_EP0_SETUP;
	pusb->ep0_maxpktsz = 64;

	for(i=0; i<USB_MAX_EP_NO; i++)
	{
		pusb->eptx_flag[i] = 0;
		pusb->eprx_flag[i] = 0;
		pusb->eptx_xfer_state[i] = USB_EPX_SETUP;
		pusb->eprx_xfer_state[i] = USB_EPX_SETUP;
	}

	pusb->device.func_addr = 0x00;
	pusb->device.bo_state = USB_BO_IDLE;
	pusb->device.bo_ep_in = 1;
	pusb->device.bo_ep_out = 1;


	pusb->timer = USB_IDLE_TIMER;
	pusb->loop = 0;

	//pusb->dma.valid = 0;
	//pusb->dma.type = DMAT_DDMA;                    //

	bus_irq_count = 0;
	ep0_irq_count = 0;
	eptx_irq_count = 0;
	eprx_irq_count = 0;

	return;
}
/*
************************************************************************************************************
*
*                                             function
*
*    函数名称：
*
*    参数列表：
*
*    返回值  ：
*
*    说明    ：
*
*
************************************************************************************************************
*/
void usb_init(pusb_struct pusb)
{
	//__u32 i=0;
	//__u32 temp;

	usb_struct_init(pusb);

	//usb_set_phytune(pusb);
	usb_drive_vbus(pusb, 0, pusb->index);

	usb_force_id(pusb, 1);

	if(pusb->speed==USB_SPEED_FS)
		usb_high_speed_disable(pusb);
	else
		usb_high_speed_enable(pusb);

	usb_suspendm_enable(pusb);

	usb_vbus_src(pusb, 0x1);
	usb_release_vbus(pusb);
	usb_force_vbus(pusb, 1);

	usb_select_ep(pusb, 0);
	usb_ep0_flush_fifo(pusb);

	__inf("USB Device!!\n");

	pusb->role = USB_ROLE_DEV;

	usb_clear_bus_interrupt_enable(pusb, 0xff);
	usb_set_bus_interrupt_enable(pusb, 0xf7);
	usb_set_eptx_interrupt_enable(pusb, 0x3f);
	usb_set_eprx_interrupt_enable(pusb, 0x3e);

	pusb->otg_dev = USB_OTG_B_DEVICE;

	return;
}
/*
************************************************************************************************************
*
*                                             function
*
*    函数名称：
*
*    参数列表：
*
*    返回值  ：
*
*    说明    ：
*
*
************************************************************************************************************
*/
__u32 usb_start(void)
{
	usb_params_init();

	reg_usb_irq_handler();

	usb_init(&awxx_usb[0]);

	return 1;
}
/*
************************************************************************************************************
*
*                                             function
*
*    函数名称：
*
*    参数列表：
*
*    返回值  ：
*
*    说明    ：
*
*
************************************************************************************************************
*/
__u32 usb_run(void)
{
	__s32 status;

	while(1)
	{
		usb_device_function(&awxx_usb[0]);
		status = wBoot_key_get_status();
		if(status > 0)
		{
			break;
		}
	}
	usb_clock_exit();

	return 1;
}



















