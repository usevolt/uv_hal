/*
 * @brief Virtual Comm port call back routines
 *
 * @note
 * Copyright(C) NXP Semiconductors, 2013
 * All rights reserved.
 *
 * @par
 * Software that is described herein is for illustrative purposes only
 * which provides customers with programming information regarding the
 * LPC products.  This software is supplied "AS IS" without any warranties of
 * any kind, and NXP Semiconductors and its licensor disclaim any and
 * all warranties, express or implied, including all implied warranties of
 * merchantability, fitness for a particular purpose and non-infringement of
 * intellectual property rights.  NXP Semiconductors assumes no responsibility
 * or liability for the use of the software, conveys no license or rights under any
 * patent, copyright, mask work right, or any other intellectual property rights in
 * or to any products. NXP Semiconductors reserves the right to make changes
 * in the software without notification. NXP Semiconductors also makes no
 * representation or warranty that such application will be suitable for the
 * specified use without further testing or modification.
 *
 * @par
 * Permission to use, copy, modify, and distribute this software and its
 * documentation is hereby granted, under NXP Semiconductors' and its
 * licensor's relevant copyrights in the software, without fee, provided that it
 * is used in conjunction with NXP Semiconductors microcontrollers.  This
 * copyright, permission, and disclaimer notice must appear in all copies of
 * this code.
 */
#include <string.h>
#include <uv_utilities.h>
#include "cdc_vcom.h"

#if CONFIG_TERMINAL_USBDVCOM

/*****************************************************************************
 * Private types/enumerations/variables
 ****************************************************************************/

/*****************************************************************************
 * Public types/enumerations/variables
 ****************************************************************************/

/**
 * Global variable to hold Virtual COM port control data.
 */
VCOM_DATA_T g_vCOM = {};

/*****************************************************************************
 * Private functions
 ****************************************************************************/


static USBD_HANDLE_T g_hUsb;
const  USBD_API_T *g_pUsbApi;

void USB_IRQHandler(void)
{
	USBD_API->hw->ISR(g_hUsb);
}

/* Find the address of interface descriptor for given class type. */
USB_INTERFACE_DESCRIPTOR *find_IntfDesc(const uint8_t *pDesc, uint32_t intfClass)
{
	USB_COMMON_DESCRIPTOR *pD;
	USB_INTERFACE_DESCRIPTOR *pIntfDesc = 0;
	uint32_t next_desc_adr;

	pD = (USB_COMMON_DESCRIPTOR *) pDesc;
	next_desc_adr = (uint32_t) pDesc;

	while (pD->bLength) {
		/* is it interface descriptor */
		if (pD->bDescriptorType == USB_INTERFACE_DESCRIPTOR_TYPE) {

			pIntfDesc = (USB_INTERFACE_DESCRIPTOR *) pD;
			/* did we find the right interface descriptor */
			if (pIntfDesc->bInterfaceClass == intfClass) {
				break;
			}
		}
		pIntfDesc = 0;
		next_desc_adr = (uint32_t) pD + pD->bLength;
		pD = (USB_COMMON_DESCRIPTOR *) next_desc_adr;
	}

	return pIntfDesc;
}


/* VCOM bulk EP_IN endpoint handler */
static ErrorCode_t VCOM_bulk_in_hdlr(USBD_HANDLE_T hUsb, void *data, uint32_t event)
{
	VCOM_DATA_T *pVcom = (VCOM_DATA_T *) data;

	if (event == USB_EVT_IN) {
		pVcom->tx_flags &= ~VCOM_TX_BUSY;
	}
	return LPC_OK;
}

/* VCOM bulk EP_OUT endpoint handler */
static ErrorCode_t VCOM_bulk_out_hdlr(USBD_HANDLE_T hUsb, void *data, uint32_t event)
{
	VCOM_DATA_T *pVcom = (VCOM_DATA_T *) data;

	switch (event) {
	case USB_EVT_OUT:
		pVcom->rx_count = USBD_API->hw->ReadEP(hUsb, USB_CDC_OUT_EP, pVcom->rx_buff);
		if (pVcom->rx_flags & VCOM_RX_BUF_QUEUED) {
			pVcom->rx_flags &= ~VCOM_RX_BUF_QUEUED;
			if (pVcom->rx_count != 0) {
				pVcom->rx_flags |= VCOM_RX_BUF_FULL;
			}

		}
		else if (pVcom->rx_flags & VCOM_RX_DB_QUEUED) {
			pVcom->rx_flags &= ~VCOM_RX_DB_QUEUED;
			pVcom->rx_flags |= VCOM_RX_DONE;
		}
		break;

	case USB_EVT_OUT_NAK:
		/* queue free buffer for RX */
		if ((pVcom->rx_flags & (VCOM_RX_BUF_FULL | VCOM_RX_BUF_QUEUED)) == 0) {
			USBD_API->hw->ReadReqEP(hUsb, USB_CDC_OUT_EP, pVcom->rx_buff, VCOM_RX_BUF_SZ);
			pVcom->rx_flags |= VCOM_RX_BUF_QUEUED;
		}
		break;

	default:
		break;
	}

	return LPC_OK;
}

/* Set line coding call back routine */
static ErrorCode_t VCOM_SetLineCode(USBD_HANDLE_T hCDC, CDC_LINE_CODING *line_coding)
{
	VCOM_DATA_T *pVcom = &g_vCOM;

	/* Called when baud rate is changed/set. Using it to know host connection state */
	pVcom->tx_flags = VCOM_TX_CONNECTED;	/* reset other flags */

	return LPC_OK;
}

/*****************************************************************************
 * Public functions
 ****************************************************************************/

/* Virtual com port init routine */
ErrorCode_t vcom_init(void) {

	USBD_API_INIT_PARAM_T usb_param;
	USB_CORE_DESCS_T desc;
	ErrorCode_t ret = LPC_OK;

	Chip_USB_Init();

	/* initialize USBD ROM API pointer. */
	g_pUsbApi = (const USBD_API_T *) LPC_ROM_API->pUSBD;

	/* initialize call back structures */
	memset((void *) &usb_param, 0, sizeof(USBD_API_INIT_PARAM_T));
	usb_param.usb_reg_base = LPC_USB0_BASE;
	/*	WORKAROUND for artf44835 ROM driver BUG:
	    Code clearing STALL bits in endpoint reset routine corrupts memory area
	    next to the endpoint control data. For example When EP0, EP1_IN, EP1_OUT,
	    EP2_IN are used we need to specify 3 here. But as a workaround for this
	    issue specify 4. So that extra EPs control structure acts as padding buffer
	    to avoid data corruption. Corruption of padding memory doesn’t affect the
	    stack/program behaviour.
	 */
	usb_param.max_num_ep = 3 + 1;
	usb_param.mem_base = USB_STACK_MEM_BASE;
	usb_param.mem_size = USB_STACK_MEM_SIZE;

	/* Set the USB descriptors */
	desc.device_desc = (uint8_t *) &USB_DeviceDescriptor[0];
	desc.string_desc = (uint8_t *) &USB_StringDescriptor[0];
	/* Note, to pass USBCV test full-speed only devices should have both
	   descriptor arrays point to same location and device_qualifier set to 0.
	 */
	desc.high_speed_desc = (uint8_t *) &USB_FsConfigDescriptor[0];
	desc.full_speed_desc = (uint8_t *) &USB_FsConfigDescriptor[0];
	desc.device_qualifier = 0;

	/* USB Initialization */
	ret = USBD_API->hw->Init(&g_hUsb, &desc, &usb_param);
	if (ret == LPC_OK) {

		USBD_CDC_INIT_PARAM_T cdc_param;
		uint32_t ep_indx;

		g_vCOM.hUsb = g_hUsb;
		memset((void *) &cdc_param, 0, sizeof(USBD_CDC_INIT_PARAM_T));
		cdc_param.mem_base = usb_param.mem_base;
		cdc_param.mem_size = usb_param.mem_size;
		cdc_param.cif_intf_desc = (uint8_t *) find_IntfDesc(desc.high_speed_desc,
				CDC_COMMUNICATION_INTERFACE_CLASS);
		cdc_param.dif_intf_desc = (uint8_t *) find_IntfDesc(desc.high_speed_desc,
				CDC_DATA_INTERFACE_CLASS);
		cdc_param.SetLineCode = VCOM_SetLineCode;

		ret = USBD_API->cdc->init(g_hUsb, &cdc_param, &g_vCOM.hCdc);

		if (ret == LPC_OK) {
			/* allocate transfer buffers */
			g_vCOM.rx_buff = (uint8_t *) cdc_param.mem_base;
			cdc_param.mem_base += VCOM_RX_BUF_SZ;
			cdc_param.mem_size -= VCOM_RX_BUF_SZ;

			/* register endpoint interrupt handler */
			ep_indx = (((USB_CDC_IN_EP & 0x0F) << 1) + 1);
			ret = USBD_API->core->RegisterEpHandler(g_hUsb, ep_indx,
					VCOM_bulk_in_hdlr, &g_vCOM);
			if (ret == LPC_OK) {
				/* register endpoint interrupt handler */
				ep_indx = ((USB_CDC_OUT_EP & 0x0F) << 1);
				ret = USBD_API->core->RegisterEpHandler(g_hUsb, ep_indx,
						VCOM_bulk_out_hdlr, &g_vCOM);

			}
			/* update mem_base and size variables for cascading calls. */
			usb_param.mem_base = cdc_param.mem_base;
			usb_param.mem_size = cdc_param.mem_size;
		}
	}

	if (ret == LPC_OK) {
		/*  enable USB interrupts */
		NVIC_EnableIRQ(USB0_IRQn);
		/* now connect */
		USBD_API->hw->Connect(g_hUsb, 1);
	}

	return ret;
}

/* Virtual com port buffered read routine */
uint32_t vcom_bread(uint8_t *pBuf, uint32_t buf_len)
{
	VCOM_DATA_T *pVcom = &g_vCOM;
	uint16_t cnt = 0;
	/* read from the default buffer if any data present */
	if (pVcom->rx_count) {
		cnt = (pVcom->rx_count < buf_len) ? pVcom->rx_count : buf_len;
		memcpy(pBuf, pVcom->rx_buff, cnt);
		pVcom->rx_rd_count += cnt;

		/* enter critical section */
		NVIC_DisableIRQ(USB0_IRQn);
		if (pVcom->rx_rd_count >= pVcom->rx_count) {
			pVcom->rx_flags &= ~VCOM_RX_BUF_FULL;
			pVcom->rx_rd_count = pVcom->rx_count = 0;
		}
		/* exit critical section */
		NVIC_EnableIRQ(USB0_IRQn);
	}
	return cnt;

}

/* Virtual com port read routine */
ErrorCode_t vcom_read_req(uint8_t *pBuf, uint32_t len)
{
	VCOM_DATA_T *pVcom = &g_vCOM;

	/* check if we queued Rx buffer */
	if (pVcom->rx_flags & (VCOM_RX_BUF_QUEUED | VCOM_RX_DB_QUEUED)) {
		return ERR_BUSY;
	}
	/* enter critical section */
	NVIC_DisableIRQ(USB0_IRQn);
	/* if not queue the request and return 0 bytes */
	USBD_API->hw->ReadReqEP(pVcom->hUsb, USB_CDC_OUT_EP, pBuf, len);
	/* exit critical section */
	NVIC_EnableIRQ(USB0_IRQn);
	pVcom->rx_flags |= VCOM_RX_DB_QUEUED;

	return LPC_OK;
}

/* Gets current read count. */
uint32_t vcom_read_cnt(void)
{
	VCOM_DATA_T *pVcom = &g_vCOM;
	uint32_t ret = 0;

	if (pVcom->rx_flags & VCOM_RX_DONE) {
		ret = pVcom->rx_count;
		pVcom->rx_count = 0;
	}

	return ret;
}

/* Virtual com port write routine*/
uint32_t vcom_write(char *pBuf, uint32_t len)
{
	VCOM_DATA_T *pVcom = &g_vCOM;
	uint32_t ret = 0;

	if ( (pVcom->tx_flags & VCOM_TX_CONNECTED) && ((pVcom->tx_flags & VCOM_TX_BUSY) == 0) ) {
		pVcom->tx_flags |= VCOM_TX_BUSY;

		/* enter critical section */
		NVIC_DisableIRQ(USB0_IRQn);
		ret = USBD_API->hw->WriteEP(pVcom->hUsb, USB_CDC_IN_EP, (uint8_t*) pBuf, len);
		/* exit critical section */
		NVIC_EnableIRQ(USB0_IRQn);
	}

	return ret;
}


#endif
