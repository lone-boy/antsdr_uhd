#undef LWIP_TCP
#undef LWIP_DHCP
#undef CHECKSUM_CHECK_UDP
#undef LWIP_CHECKSUM_ON_COPY
#undef CHECKSUM_GEN_UDP

//INCLUDES
#include <stdio.h>
#include "xtime_l.h"

#include "xparameters.h"

#include "netif/xadapter.h"

#include "platform.h"
#include "platform_config.h"
#if defined (__arm__) || defined(__aarch64__)
#include "xil_printf.h"
#endif

#include "lwip/udp.h"
#include "xil_cache.h"

// Includes for interrupt controller
#include "xscugic.h"
#include "xil_exception.h"

// Include our own definitions
#include "includes.h"

#include "fifo.h"

#include "iq_data_dma.h"
#include "ctrl_resp_dma.h"

/* Defined by each RAW mode application */
void print_app_header();
int start_application();

/* Missing declaration in lwIP */
void lwip_init();

/* set up netif stuctures */
static struct netif server_netif;
struct netif *echo_netif;


// Interrupt Controller define statements (see xparameters.h)
#define GETCENTROID_INTR_ID		XPAR_FABRIC_CTRL_RESP_DMA_0_MEM_WR_DONE_IRQ_INTR
#define IQ_RX_INTR_ID 			XPAR_FABRIC_IQ_DATA_DMA_0_MEM_WR_DONE_IRQ_INTR
#define INTC_DEVICE_ID 			XPAR_SCUGIC_SINGLE_DEVICE_ID
#define INTC_HANDLER			XScuGic_InterruptHandler
#define INTC					XScuGic
static  INTC Intc;
int	GetCentroidReady;

// Global Variables to store results and handle data flow
int			Centroid;

// Global variables for data flow
volatile u8		IndArrDone;
volatile u32		EthBytesReceived;
volatile u8		SendResults;
volatile u8   		DMA_TX_Busy;
volatile u8		Error;

// Global Variables for Ethernet handling
u16_t    		RemotePort;
ip_addr_t  	RemoteAddr;

u16_t 			RemptePortData;
ip_addr_t RemoteAddrData;
struct udp_pcb 		send_pcb;
struct pbuf * psnd;

struct udp_pcb		send_data_pcb;
struct pbuf *psnd_metedata;

u8 *recv_from_uhd;
u8 *send_to_fpga;
u8 *recv_from_fpga;


u8 *recv_from_uhd_metadata;
u8 *send_to_fpga_metadata;
u8 *recv_from_fpga_metadata;


fifo ctrl_recv_fifo;
fifo ctrl_send_fifo;
fifo data_recv_fifo;
fifo data_send_fifo;

#define 	TX_BUFFER_BASE 		0x2000000
#define 	RX_BUFFER_BASE		0x2300000

#define 	TX_META_BUFFER_BASE 	0x2600000
#define 	RX_META_BUFFER_BASE		0x3000000

// Other function prototypes
extern void ControlDmaIntrHandler();
extern void IQRxDmaIntrHandler();
static int SetupIntrSystem(INTC * IntcInstancePtr, u16 GetCentroidIntrId, u16 IQRxDmaIntrID);
static void DisableIntrSystem(INTC * IntcInstancePtr, u16 GetCentroidIntrId, u16 IQRxDmaIntrID);

static void swap_ntohl(u8 *metadata,u8 *swap_data,int length);


static void swap_ntohl(u8 *metadata,u8 *swap_data,int length){
	//there is length bytes 32bit = 4 bytes;
	int loop;
	loop = length / 4;
	int i;
	for(i=0;i<loop;i++){
		swap_data[i*4] = metadata[i*4+3];swap_data[i*4+1] = metadata[i*4+2];
		swap_data[i*4+2] = metadata[i*4+1];swap_data[i*4+3] = metadata[i*4];
	}
}


// initPeripherals() - set up the DMA and GetCentroid cores
void initPeripherals(){
	/* Set up Interrupt system  */
	int status = SetupIntrSystem(&Intc, GETCENTROID_INTR_ID, IQ_RX_INTR_ID);
	if (status != XST_SUCCESS) {
		xil_printf("Failed Interrupt Setup for GetCentroid!\r\n");
	}

}

/* print_ip: function to print out ip address */
void print_ip(char *msg, ip_addr_t *ip)
{
	print(msg);
	xil_printf("%d.%d.%d.%d\n\r", ip4_addr1(ip), ip4_addr2(ip), 
			ip4_addr3(ip), ip4_addr4(ip));
}

/* print_ip_settings: function to print out the ip settings */
void print_ip_settings(ip_addr_t *ip, ip_addr_t *mask, ip_addr_t *gw)
{
	print_ip("Board IP: ", ip);
	print_ip("Netmask : ", mask);
	print_ip("Gateway : ", gw);
}

/* print_app_header: function to print a header at start time */
void print_app_header()
{
	xil_printf("\n\r\n\r------Microphase uhd------\n\r");
}


/* main */
int main()
{

	/* Allocate structures for the ip address, netmask, and gateway */
	ip_addr_t ipaddr, netmask, gw;


	int status = 0;

	/* The mac address of the board. this should be unique per board */
	unsigned char mac_ethernet_address[] =
	{ 0x00, 0x0a, 0x35, 0x00, 0x01, 0x02 };

	/* Use the same structure for the server and the echo server */
	echo_netif = &server_netif;

	/* Initialize the platform */
	init_platform();

	// Initialize the DMA and GetCentroid peripherals
	initPeripherals();

	/* initialize IP addresses to be used */
	IP4_ADDR(&ipaddr,  192, 168,   2, 25);
	IP4_ADDR(&netmask, 255, 255, 255,  0);
	IP4_ADDR(&gw,      192, 168,   2,  1);

	/* Print out the simple application header */
	print_app_header();

	/* Initialize the lwip for UDP */
	lwip_init();

  	/* Add network interface to the netif_list, and set it as default */
	if (!xemac_add(echo_netif, &ipaddr, &netmask,
			&gw, mac_ethernet_address,
			PLATFORM_EMAC_BASEADDR)) {
		xil_printf("Error adding N/W interface\n\r");
		return -1;
	}
	netif_set_default(echo_netif);

	/* Now enable interrupts */
	platform_enable_interrupts();

	/* Specify that the network if is up */
	netif_set_up(echo_netif);

	/* Print out the ip settings */
	print_ip_settings(&ipaddr, &netmask, &gw);

	/* Start the application (web server, rxtest, txtest, etc..) */
	status = start_application();
	if (status != 0){
		xil_printf("Error in start_application() with code: %d\n\r", status);
		goto ErrorOrDone;
	}

	send_to_fpga = (u8 *)TX_BUFFER_BASE;
	recv_from_fpga = (u8 *)RX_BUFFER_BASE;
	send_to_fpga_metadata = (u8 *)TX_META_BUFFER_BASE;
	recv_from_fpga_metadata = (u8 *)RX_META_BUFFER_BASE;


	/* Receive and process packets */
	while (Error==0) {
		/* Receive packets */
		xemacif_input(echo_netif);
		/* fifo from uhd should send to fpga */
		if(get_fifo_num(&ctrl_recv_fifo) > 0){
			//change the net_to_host metadata
			swap_ntohl(ctrl_recv_fifo.d.data_16[ctrl_recv_fifo.front],send_to_fpga,ctrl_recv_fifo.length[ctrl_recv_fifo.front]);
			//send the data to fpga
			Xil_DCacheFlushRange((UINTPTR)send_to_fpga,16);
			let_fifo_out(&ctrl_recv_fifo);

			ctrl_transfer(TX_BUFFER_BASE,16);
			usleep(100);
		}
		//send uhd
		if(get_fifo_num(&ctrl_send_fifo) > 0){
			psnd = pbuf_alloc(PBUF_TRANSPORT,24,PBUF_ROM);
//			send_to_uhd = ctrl_send_fifo.d.data_24[ctrl_send_fifo.front];
			swap_ntohl(recv_from_fpga,ctrl_send_fifo.d.data_24[ctrl_send_fifo.front],ctrl_send_fifo.length[ctrl_send_fifo.front]);
			psnd->payload = (void *)ctrl_send_fifo.d.data_24[ctrl_send_fifo.front];
			let_fifo_out(&ctrl_send_fifo);
			udp_sendto(&send_pcb, psnd, &RemoteAddr, RemotePort);

			pbuf_free(psnd);
			usleep(100);
		}
		if(get_fifo_num(&data_recv_fifo) > 0){
			swap_ntohl(data_recv_fifo.d.data_1472[data_recv_fifo.front],send_to_fpga_metadata,data_recv_fifo.length[data_recv_fifo.front]);
						//send the data to fpga
			Xil_DCacheFlushRange((UINTPTR)send_to_fpga_metadata,data_recv_fifo.length[data_recv_fifo.front]);
			let_fifo_out(&data_recv_fifo);
			tx_iq_transfer(TX_META_BUFFER_BASE,data_recv_fifo.length[data_recv_fifo.front]);
			usleep(300);
		}

	}
	// Jump point for failure
	ErrorOrDone:
	xil_printf("Catastrophic Error! Shutting down and exiting...\n\r");
	// Disable the GetCentroid interrupts and disconnect from the GIC
	DisableIntrSystem(&Intc, GETCENTROID_INTR_ID, IQ_RX_INTR_ID);

	/* never reached */
	cleanup_platform();

	return 0;
}
/*****************************************************************************/
/*
*
* This is the GetCentroid interrupt handler function
*
* It gets the interrupt status from the hardware, acknowledges it, and if any
* error happens, it resets the hardware.
*
* If this is an ap_done interrupt, we set SendResults=1 to indicate a value 
* is ready to be sent.
* 
* If this is an ap_ready interrupt, we set GetCentroidReady to indicate that 
* it is ready for a new calculation. 
*
* @param	Callback is a pointer to the instance of the GetCentroid object
*
* @return	None.
*
* @note		None.
*
******************************************************************************/


/*****************************************************************************/
/*
*
* This function sets up the interrupt system so interrupts can occur for the
* GetCentroid custom IP module.
*
* @param	IntcInstancePtr is a pointer to the instance of the INTC.
* @param	AxiDmaPtr is a pointer to the instance of the DMA engine
* @param	TxIntrId is the TX channel Interrupt ID.
* @param	GetCentroidPtr is a pointer to the instance of the DMA engine
* @param	GetCentroidIntrId is the GetCentroid Device Interrupt ID.
*
* @return
*		- XST_SUCCESS if successful,
*		- XST_FAILURE.if not succesful
*
* @note		None.
*
******************************************************************************/
static int SetupIntrSystem(INTC * IntcInstancePtr, u16 GetCentroidIntrId, u16 IQRxDmaIntrID)
{

	// Declare variables for storage
	int Status;

	// Generic Interrupt Controller Config and Init
	XScuGic_Config *IntcConfig;
	Xil_ExceptionInit();

	/* Enable interrupts from the hardware */
	Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT,
			(Xil_ExceptionHandler)INTC_HANDLER,
			(void *)IntcInstancePtr);

	Xil_ExceptionEnable();

	/*
	 * Initialize the interrupt controller driver so that it is ready to
	 * use (look it up by its device ID, which is given in xparameters_ps.h
	 * as #define XPAR_SCUGIC_SINGLE_DEVICE_ID 0U.
	 */
	xil_printf("Initializing Interrupts\n\r");
	IntcConfig = XScuGic_LookupConfig(INTC_DEVICE_ID);
	if (NULL == IntcConfig) {
		return XST_FAILURE;
	}

	Status = XScuGic_CfgInitialize(IntcInstancePtr, IntcConfig,
					IntcConfig->CpuBaseAddress);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	/*
	 * Set priority for the TX DMA and GetCentroid Interrupts
	 */
	XScuGic_SetPriorityTriggerType(IntcInstancePtr, GetCentroidIntrId, 0x90, 0x3);
	XScuGic_SetPriorityTriggerType(IntcInstancePtr, IQRxDmaIntrID, 0x90, 0x3);

	/*
	 * GetCentroid - INTERRUPT ON READY AND DONE
	 * Connect the device driver handler that will be called when an
	 * interrupt for the device occurs, the handler defined above performs
	 * the specific interrupt processing for the device.
	 */
	Status = XScuGic_Connect(IntcInstancePtr, GetCentroidIntrId,
				(Xil_InterruptHandler)ControlDmaIntrHandler,
				NULL);

	Status = XScuGic_Connect(IntcInstancePtr, IQRxDmaIntrID,
				(Xil_InterruptHandler)IQRxDmaIntrHandler,
				NULL);
	if (Status != XST_SUCCESS) {
		return Status;
	}

	/*
	 * Enable the interrupts
	 */
	XScuGic_Enable(IntcInstancePtr, GetCentroidIntrId);
	XScuGic_Enable(IntcInstancePtr, IQRxDmaIntrID);

	return XST_SUCCESS;
}

/*****************************************************************************/
/**
*
* This function disables the interrupts for DMA engine.
*
* @param	IntcInstancePtr is the pointer to the INTC component instance
* @param	GetCentroidIntrId is interrupt ID associated GetCentroid
* @param	GetCentroidPtr is a pointer to the GetCentroid object
* @param	TxIntrId is interrupt ID associated with AXI DMA TX
* @return	None.
*
* @note		None.
*
******************************************************************************/
static void DisableIntrSystem(INTC * IntcInstancePtr, u16 GetCentroidIntrId, u16 IQRxDmaIntrID)
{
	// Now disconnect the interrupt systems from the GIC
	XScuGic_Disconnect(IntcInstancePtr, GetCentroidIntrId);
	XScuGic_Disconnect(IntcInstancePtr, IQRxDmaIntrID);

}

