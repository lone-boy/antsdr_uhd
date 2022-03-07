

/***************************** Include Files *******************************/
#include "iq_data_dma.h"
#include "xparameters.h"
#include "xil_cache.h"

#define IQ_DATA_ADDR XPAR_IQ_DATA_DMA_0_S00_AXI_BASEADDR

/************************** Function Definitions ***************************/
void tx_iq_transfer(u32 base_addr, u32 data_len)
{
	IQ_DATA_DMA_mWriteReg(IQ_DATA_ADDR, IQ_MEM_RD_BASE_ADDR, base_addr);
	IQ_DATA_DMA_mWriteReg(IQ_DATA_ADDR, IQ_MEM_RD_LENGTH, data_len);
	IQ_DATA_DMA_mWriteReg(IQ_DATA_ADDR, IQ_MEM_RD_START, 0x01);
	IQ_DATA_DMA_mWriteReg(IQ_DATA_ADDR, IQ_MEM_RD_START, 0x00);
}

void rx_iq_transfer(u32 base_addr, u32 * data_len)
{
	u32 transfer_length;
	IQ_DATA_DMA_mWriteReg(IQ_DATA_ADDR, IQ_MEM_WR_BASE_ADDR, base_addr);
	transfer_length = IQ_DATA_DMA_mReadReg(IQ_DATA_ADDR, IQ_MEM_WR_LENGTH);
	*data_len = transfer_length;
}
