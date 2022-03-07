

/***************************** Include Files *******************************/
#include "ctrl_resp_dma.h"
#include "xparameters.h"
#include "xil_cache.h"

#define CTRL_RESP_ADDR 	XPAR_CTRL_RESP_DMA_0_S00_AXI_BASEADDR


/************************** Function Definitions ***************************/
void ctrl_transfer(u32 base_addr, u32 data_len)
{
	CTRL_RESP_DMA_mWriteReg(CTRL_RESP_ADDR, MEM_RD_BASE_ADDR, base_addr);
	CTRL_RESP_DMA_mWriteReg(CTRL_RESP_ADDR, MEM_RD_LENGTH, data_len);
	CTRL_RESP_DMA_mWriteReg(CTRL_RESP_ADDR, MEM_RD_START, 0x01);
	CTRL_RESP_DMA_mWriteReg(CTRL_RESP_ADDR, MEM_RD_START, 0x00);
}

void resp_transfer(u32 base_addr, u32 * data_len)
{
	u32 transfer_length;
	CTRL_RESP_DMA_mWriteReg(CTRL_RESP_ADDR, MEM_WR_BASE_ADDR, base_addr);
	transfer_length = CTRL_RESP_DMA_mReadReg(CTRL_RESP_ADDR, MEM_WR_LENGTH);
	*data_len = transfer_length;
}


u32 get_resp_status()
{
	u32 status;
	status = CTRL_RESP_DMA_mReadReg(CTRL_RESP_ADDR, MEM_WR_STATUS);
	if(status == 0x1)
		CTRL_RESP_DMA_mWriteReg(CTRL_RESP_ADDR, MEM_WR_STATUS, 0x00);
	return status;
}
