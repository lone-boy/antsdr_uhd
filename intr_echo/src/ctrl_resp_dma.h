
#ifndef CTRL_RESP_DMA_H
#define CTRL_RESP_DMA_H


/****************** Include Files ********************/
#include "xil_types.h"
#include "xstatus.h"
#include "xil_io.h"

#define CTRL_RESP_DMA_S00_AXI_SLV_REG0_OFFSET 0
#define CTRL_RESP_DMA_S00_AXI_SLV_REG1_OFFSET 4
#define CTRL_RESP_DMA_S00_AXI_SLV_REG2_OFFSET 8
#define CTRL_RESP_DMA_S00_AXI_SLV_REG3_OFFSET 12
#define CTRL_RESP_DMA_S00_AXI_SLV_REG4_OFFSET 16
#define CTRL_RESP_DMA_S00_AXI_SLV_REG5_OFFSET 20
#define CTRL_RESP_DMA_S00_AXI_SLV_REG6_OFFSET 24
#define CTRL_RESP_DMA_S00_AXI_SLV_REG7_OFFSET 28
#define CTRL_RESP_DMA_S00_AXI_SLV_REG8_OFFSET 32
#define CTRL_RESP_DMA_S00_AXI_SLV_REG9_OFFSET 36
#define CTRL_RESP_DMA_S00_AXI_SLV_REG10_OFFSET 40
#define CTRL_RESP_DMA_S00_AXI_SLV_REG11_OFFSET 44
#define CTRL_RESP_DMA_S00_AXI_SLV_REG12_OFFSET 48
#define CTRL_RESP_DMA_S00_AXI_SLV_REG13_OFFSET 52
#define CTRL_RESP_DMA_S00_AXI_SLV_REG14_OFFSET 56
#define CTRL_RESP_DMA_S00_AXI_SLV_REG15_OFFSET 60

#define MEM_WR_BASE_ADDR		CTRL_RESP_DMA_S00_AXI_SLV_REG0_OFFSET
#define MEM_WR_LENGTH 			CTRL_RESP_DMA_S00_AXI_SLV_REG1_OFFSET
#define MEM_WR_STATUS 			CTRL_RESP_DMA_S00_AXI_SLV_REG2_OFFSET

#define MEM_RD_START 			CTRL_RESP_DMA_S00_AXI_SLV_REG3_OFFSET
#define MEM_RD_BASE_ADDR		CTRL_RESP_DMA_S00_AXI_SLV_REG4_OFFSET
#define MEM_RD_LENGTH 			CTRL_RESP_DMA_S00_AXI_SLV_REG5_OFFSET

void ctrl_transfer(u32 base_addr, u32 data_len);
void resp_transfer(u32 base_addr, u32 * data_len);
u32 get_resp_status();


/**************************** Type Definitions *****************************/
/**
 *
 * Write a value to a CTRL_RESP_DMA register. A 32 bit write is performed.
 * If the component is implemented in a smaller width, only the least
 * significant data is written.
 *
 * @param   BaseAddress is the base address of the CTRL_RESP_DMAdevice.
 * @param   RegOffset is the register offset from the base to write to.
 * @param   Data is the data written to the register.
 *
 * @return  None.
 *
 * @note
 * C-style signature:
 * 	void CTRL_RESP_DMA_mWriteReg(u32 BaseAddress, unsigned RegOffset, u32 Data)
 *
 */
#define CTRL_RESP_DMA_mWriteReg(BaseAddress, RegOffset, Data) \
  	Xil_Out32((BaseAddress) + (RegOffset), (u32)(Data))

/**
 *
 * Read a value from a CTRL_RESP_DMA register. A 32 bit read is performed.
 * If the component is implemented in a smaller width, only the least
 * significant data is read from the register. The most significant data
 * will be read as 0.
 *
 * @param   BaseAddress is the base address of the CTRL_RESP_DMA device.
 * @param   RegOffset is the register offset from the base to write to.
 *
 * @return  Data is the data from the register.
 *
 * @note
 * C-style signature:
 * 	u32 CTRL_RESP_DMA_mReadReg(u32 BaseAddress, unsigned RegOffset)
 *
 */
#define CTRL_RESP_DMA_mReadReg(BaseAddress, RegOffset) \
    Xil_In32((BaseAddress) + (RegOffset))

/************************** Function Prototypes ****************************/
/**
 *
 * Run a self-test on the driver/device. Note this may be a destructive test if
 * resets of the device are performed.
 *
 * If the hardware system is not built correctly, this function may never
 * return to the caller.
 *
 * @param   baseaddr_p is the base address of the CTRL_RESP_DMA instance to be worked on.
 *
 * @return
 *
 *    - XST_SUCCESS   if all self-test code passed
 *    - XST_FAILURE   if any self-test code failed
 *
 * @note    Caching must be turned off for this function to work.
 * @note    Self test may fail if data memory and device are not on the same bus.
 *
 */
XStatus CTRL_RESP_DMA_Reg_SelfTest(void * baseaddr_p);

#endif // CTRL_RESP_DMA_H
