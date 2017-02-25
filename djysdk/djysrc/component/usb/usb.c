﻿//-----------------------------------------------------------------------------
// Copyright (c) 2014, SHENZHEN PENGRUI SOFT CO LTD. All rights reserved.

// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:

// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
// 3. As a constituent part of djyos,do not transplant it to other software
//    without specific prior written permission.

// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//-----------------------------------------------------------------------------
// Copyright (c) 2014 著作权由深圳鹏瑞软件有限公司所有。著作权人保留一切权利。
//
// 这份授权条款，在使用者符合以下三条件的情形下，授予使用者使用及再散播本
// 软件包装原始码及二进位可执行形式的权利，无论此包装是否经改作皆然：
//
// 1. 对于本软件源代码的再散播，必须保留上述的版权宣告、本条件列表，以
//    及下述的免责声明。
// 2. 对于本套件二进位可执行形式的再散播，必须连带以文件以及／或者其他附
//    于散播包装中的媒介方式，重制上述之版权宣告、本条件列表，以及下述
//    的免责声明。
// 3. 本软件作为都江堰操作系统的组成部分，未获事前取得的书面许可，不允许移植到非
//    都江堰操作系统环境下运行。

// 免责声明：本软件是本软件版权持有人以及贡献者以现状（"as is"）提供，
// 本软件包装不负任何明示或默示之担保责任，包括但不限于就适售性以及特定目
// 的的适用性为默示性担保。版权持有人及本软件之贡献者，无论任何条件、
// 无论成因或任何责任主义、无论此责任为因合约关系、无过失责任主义或因非违
// 约之侵权（包括过失或其他原因等）而起，对于任何因使用本软件包装所产生的
// 任何直接性、间接性、偶发性、特殊性、惩罚性或任何结果的损害（包括但不限
// 于替代商品或劳务之购用、使用损失、资料损失、利益损失、业务中断等等），
// 不负任何责任，即在该种使用已获事前告知可能会造成此类损害的情形下亦然。
//-----------------------------------------------------------------------------

#include <stdio.h>
#include "./stm32_usb_host_library/core/inc/usbh_core.h"
#include "./stm32_usb_host_library/class/msc/inc/usbh_msc.h"
#include "cpu_peri_int_line.h"
#include <int.h>

u8 IAP_USB_Ready = 0;
extern u8 MSC_DeviceReady;
extern u32 OTG_HS_IRQHandler(u32 Data);
extern s32 USB_Register(const char *ChipName, s32 Clean);

typedef enum {
	APPLICATION_IDLE = 0,
	APPLICATION_READY,
	APPLICATION_DISCONNECT,
}MSC_ApplicationTypeDef;

USBH_HandleTypeDef hUSBHost;
MSC_ApplicationTypeDef Appli_state = APPLICATION_IDLE;

static void USBH_UserProcess(USBH_HandleTypeDef *phost, uint8_t id);

//-----------------------------------------------------------------------------
//功能: USB协议栈线程
//参数:
//返回:
//备注:
//-----------------------------------------------------------------------------
u32 USB_Threat(void)
{
	/* Init Host Library */
	USBH_Init(&hUSBHost, USBH_UserProcess, 0);
	
	Int_Register(CN_INT_LINE_OTG_HS);
	Int_SetClearType(CN_INT_LINE_OTG_HS, CN_INT_CLEAR_AUTO);
	Int_IsrConnect(CN_INT_LINE_OTG_HS, OTG_HS_IRQHandler);
	Int_SettoAsynSignal(CN_INT_LINE_OTG_HS);
	Int_ClearLine(CN_INT_LINE_OTG_HS);
	Int_RestoreAsynLine(CN_INT_LINE_OTG_HS);
	
	/* Add Supported Class */
	USBH_RegisterClass(&hUSBHost, USBH_MSC_CLASS); // 注册MSC类
	
	printf("\r\nUSB threat begin\r\n");

	/* Start Host Process */
	USBH_Start(&hUSBHost); // 开中断，Vbus上电
	
	/* Run Application (Blocking mode) */
	while (1)
	{
		/* USB Host Background task */
		USBH_Process(&hUSBHost);
	}
}

//-----------------------------------------------------------------------------
//功能:
//参数:
//返回:
//备注:
//-----------------------------------------------------------------------------
static void USBH_UserProcess(USBH_HandleTypeDef *phost, uint8_t id)
{
	switch(id)
	{ 
		case HOST_USER_SELECT_CONFIGURATION:
			USBH_UsrLog ("UserProcess:host user select configuration.");
			break;
	
		case HOST_USER_DISCONNECTION:
			USBH_UsrLog ("UserProcess:host user disconnection.");
			Appli_state = APPLICATION_DISCONNECT;
			IAP_USB_Ready = 0;
			break;
	
		case HOST_USER_CLASS_SELECTED:
			USBH_UsrLog ("UserProcess:host user class selected.");
			break;
	
		case HOST_USER_CLASS_ACTIVE:
			USBH_UsrLog ("UserProcess:host user class active.");
			if(MSC_DeviceReady)
				IAP_USB_Ready = 1;

			Appli_state = APPLICATION_READY;
			break;
	
		case HOST_USER_CONNECTION:
			USBH_UsrLog ("UserProcess:host user connection");
			break;
	
		default:
			break; 
	}
}

//-----------------------------------------------------------------------------
//功能:
//参数:
//返回:
//备注:hemin delete, move to cpu_peri_hal.c
//-----------------------------------------------------------------------------
//void HAL_Delay(__IO uint32_t Delay)
//{
//	Djy_DelayUs(Delay*1000); // TODO
//}

//-----------------------------------------------------------------------------
//功能:
//参数:
//返回: "0" -- 成功; "-1" -- 失败;
//备注:
//-----------------------------------------------------------------------------
s32 ModuleInstall_USB(void)
{
	u16 USB_StackThreatID;
	s32 Res;

	Res = USB_Register("usb", 0);
	if(Res)
	{
		printf("\r\nUSB has been registered.");
		return (-1);
	}

	USB_StackThreatID = Djy_EvttRegist(EN_CORRELATIVE, CN_PRIO_RRS, 0, 0,
					USB_Threat, NULL, 0x2000, "usb stack service");
	//事件的两个参数暂设为0,如果用shell启动,可用来采集shell命令行参数
	if(USB_StackThreatID != CN_EVTT_ID_INVALID)
	{
		Djy_EventPop(USB_StackThreatID, NULL, 0, 0, 0, 0);
		return (0);
	}

	return (-1);
}

