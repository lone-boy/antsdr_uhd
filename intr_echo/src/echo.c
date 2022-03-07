/******************************************************************************
*
* Copyright (C) 2009 - 2014 Xilinx, Inc.  All rights reserved.
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* Use of the Software is limited solely to applications:
* (a) running on a Xilinx device, or
* (b) that interact with a Xilinx device through a bus or interconnect.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
* XILINX  BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
* WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
* OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*
* Except as contained in this notice, the name of the Xilinx shall not be used
* in advertising or otherwise to promote the sale, use or other dealings in
* this Software without prior written authorization from Xilinx.
*
******************************************************************************/

#include <stdio.h>
#include <string.h>

#include "lwip/err.h"
#include "lwip/udp.h"
#if defined (__arm__) || defined (__aarch64__)
#include "xil_printf.h"
#endif

// Include for timing
#include "xtime_l.h"

// Include our own definitions
#include "includes.h"


#include "xil_cache.h"

#include "fifo.h"

#include "iq_data_dma.h"
#include "ctrl_resp_dma.h"

#define 	TX_BUFFER_BASE 		0x2000000
#define 	RX_BUFFER_BASE		0x2300000

#define 	TX_META_BUFFER_BASE 	0x2600000
#define 	RX_META_BUFFER_BASE		0x3000000

#define IQ_DATA_ADDR XPAR_IQ_DATA_DMA_0_S00_AXI_BASEADDR
#define CTRL_RESP_ADDR 	XPAR_CTRL_RESP_DMA_0_S00_AXI_BASEADDR

extern int GetCentroidReady;

// Global arrays to store results
u16			WaveformArr[WAVE_SIZE_BYTES/2];
u8			IndArr[INDARR_SIZE_BYTES];

// Global variables for data flow
extern volatile u8      IndArrDone;
extern volatile u32	EthBytesReceived;
int*			IndArrPtr;
extern volatile u8	SendResults;
extern volatile u8   	DMA_TX_Busy;
extern volatile u8	Error;

// Global Variables for Ethernet handling
extern u16_t    	RemotePort;
extern ip_addr_t  	RemoteAddr;
extern struct udp_pcb 	send_pcb;
extern struct pbuf * psnd;
extern u16_t RemptePortData;
extern ip_addr_t RemoteAddrData;

extern struct udp_pcb		send_data_pcb;
extern struct pbuf *psnd_metedata;

extern fifo ctrl_recv_fifo;
extern fifo ctrl_send_fifo;
extern fifo data_recv_fifo;
extern fifo data_send_fifo;


extern u8 *recv_from_uhd;
extern u8 *recv_from_fpga;


extern u8 *recv_from_uhd_metadata;
extern u8 *recv_from_fpga_metadata;



u8 *recv = NULL;
u8 recv_flag;

microphase_e310_ctrl_data_t *recv_ctrl_data;
microphase_e310_ctrl_data_t *send_ctrl_data;



u32 intcnt = 0;
u32 recvcnt = 0;


void IQRxDmaIntrHandler(){

}
/* dma interr from fpga */
void ControlDmaIntrHandler()
{
	u32 length;
	resp_transfer(RX_BUFFER_BASE,&length);
	Xil_DCacheInvalidateRange((INTPTR)RX_BUFFER_BASE, length);
	let_fifo_in(&ctrl_send_fifo,recv_from_fpga,length);
}

/* recv_callback: function that handles responding to UDP packets */
void recv_callback(void *arg, struct udp_pcb *upcb,
                              struct pbuf *p, ip_addr_t *addr, u16_t port)
{

	if (!p) {
		udp_disconnect(upcb);
		return;
	}

	RemotePort = port;
	RemoteAddr = *addr;

	//just send data to fifo


	/* Keep track of the control block so we can send data back in the main while loop */
	send_pcb = *upcb;
	psnd = pbuf_alloc(PBUF_TRANSPORT, 36, PBUF_REF);
	//if just checkout not fifo
	if(p->len == 36){
		//check the pack
		recv_ctrl_data = (microphase_e310_ctrl_data_t *)p->payload;
		if(ntohl(recv_ctrl_data->id) == MICROPHASE_CTRL_ID_WAZZUP_BR0){
			memset(recv_ctrl_data,0x00,sizeof(microphase_e310_ctrl_data_t));
			memset(send_ctrl_data,0x00,sizeof(microphase_e310_ctrl_data_t));
			send_ctrl_data->id = htonl((uint32_t)MICROPHASE_CTRL_ID_WAZZUP_DUDE);
			send_ctrl_data->proto_ver = htonl((uint32_t)MICROPHASE_E310_F2_COMPAT_NUM);
			psnd->payload = (void *)send_ctrl_data;
			udp_sendto(&send_pcb, psnd, &RemoteAddr, RemotePort);
		}
	}
	// much data use fifo
	if(p->len == 16){
		recv_from_uhd = (u8 *)p->payload;

		if(recv_from_uhd[3]&0x80){
			let_fifo_in(&ctrl_recv_fifo,recv_from_uhd,p->len);
		}
	}


	/* free the received pbuf */
	pbuf_free(psnd);
	pbuf_free(p);
	return;

}


void recv_meta_data_callback(void *arg, struct udp_pcb *upcb,
        struct pbuf *p, ip_addr_t *addr, u16_t port)
{
	if(!p){
		udp_disconnect(upcb);
		return;
	}

	RemptePortData = port;
	RemoteAddrData = *addr;

	send_data_pcb = *upcb;

	if(p->len == 1464){
		//recv_from_uhd_metadata = (u8 *)p->payload;
		let_fifo_in(&data_recv_fifo,(u8 *)p->payload,1464);
	}

	pbuf_free(p);
	return;

}


/* start_application: function to set up UDP listener */
int start_application()
{

	init_fifo(&ctrl_recv_fifo);
	init_fifo(&ctrl_send_fifo);
	init_fifo(&data_recv_fifo);
	init_fifo(&data_send_fifo);
	CTRL_RESP_DMA_mWriteReg(CTRL_RESP_ADDR, MEM_WR_BASE_ADDR, RX_BUFFER_BASE);
	IQ_DATA_DMA_mWriteReg(IQ_DATA_ADDR, IQ_MEM_WR_BASE_ADDR, RX_META_BUFFER_BASE);


	memset(recv_ctrl_data,0x00,sizeof(microphase_e310_ctrl_data_t));
	memset(send_ctrl_data,0x00,sizeof(microphase_e310_ctrl_data_t));
	/* Declare some structures and variables */
	err_t err;
	struct udp_pcb* pcb;
	struct udp_pcb* data_pcb;
	unsigned port = FF_UDP_PORT;
	unsigned dataport = DATA_DDP_PORT;

	/* Create new udp PCB structure */
	pcb = udp_new();
	if (!pcb) {
		xil_printf("Error creating PCB. Out of Memory\n\r");
		return -1;
	}

	/* Bind to specified @port */
	err = udp_bind(pcb, IP_ADDR_ANY, port);
	if (err != ERR_OK) {
		xil_printf("Unable to bind to port %d: err = %d\n\r", port, err);
		return -2;
	}
	data_pcb = udp_new();
	if (!data_pcb) {
		xil_printf("Error creating data_pcb. Out of Memory\n\r");
		return -1;
	}

	err = udp_bind(data_pcb,IP_ADDR_ANY,dataport);
	if (err != ERR_OK) {
		xil_printf("Unable to bind to port %d: err = %d\n\r", dataport, err);
		return -2;
	}
	/* set the receive callback for this connection */
	udp_recv(pcb, (void *)recv_callback, NULL);

	udp_recv(data_pcb,(void *)recv_meta_data_callback,NULL);
	/* Print out information about the connection */
	xil_printf("udp echo server started @ port %d  @port %d\n\r", port,dataport);

	return 0;

}
