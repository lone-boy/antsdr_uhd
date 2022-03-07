/*
 * queue.c
 *
 *  Created on: 2022年3月5日
 *      Author: jcc
 */
#include "fifo.h"
#include "xil_printf.h"



void init_fifo(fifo *f){
	f->front = f->rear = 0;
}

u8 let_fifo_in(fifo *f,u8 *data_in,int length){
	if(16 == length){
		memcpy(f->d.data_16[f->rear],data_in,length);
	}
	else if(24 == length){
		memcpy(f->d.data_24[f->rear],data_in,length);
	}
	else if(1464 == length){
		memcpy(f->d.data_1472[f->rear],data_in,length);
	}
	else{
		xil_printf("fifo in failed,checkout the data size!\r\n");
		return 0;
	}
	f->rear = (f->rear + 1) % MAX_DEEP;
	return 1;
}

u8 let_fifo_out(fifo *f){
	if(f->rear == f->front){
		return 0;
	}
	f->front = (f->front + 1) % MAX_DEEP;
	return 1;
}

u32 get_fifo_num(fifo *f){
	if(f->rear == 0 && f->front == 0){
		return 0;
	}
	else if(f->rear < f->front){
		return f->rear + MAX_DEEP - f->front;
	}
	return f->rear - f->front;

}
