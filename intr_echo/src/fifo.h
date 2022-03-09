/*
 * queue.h
 *
 *  Created on: 2022年3月5日
 *      Author: jcc
 */

#ifndef SRC_QUEUE_H_
#define SRC_QUEUE_H_

#include "xil_types.h"
#include "memory.h"

#define MAX_DEEP 500
#define CTR_RECV_SIZE 16
#define CTR_SEND_SIZE 24

//408
#define DATA_SIZE 1472

typedef struct{
	union data{
		 u8 data_16[MAX_DEEP][CTR_RECV_SIZE];
		 u8 data_24[MAX_DEEP][CTR_SEND_SIZE];
		 u8 data_1472[MAX_DEEP][DATA_SIZE];
	}d;

	volatile u32 length[MAX_DEEP];
	volatile u32 front;
	volatile u32 rear;
}fifo;

void init_fifo(fifo *f);

u8 let_fifo_in(fifo *f,u8 *data_in,int length);

u8 let_fifo_out(fifo *f);

u32 get_fifo_num(fifo *f);
#endif /* SRC_QUEUE_H_ */
