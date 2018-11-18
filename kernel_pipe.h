#ifndef __KERNEL_PIPE_H
#define __KERNEL_PIPE_H

#include "kernel_dev.h"
#include "tinyos.h"

#define Max_Pipe_Buffer 32768 //8192 = 8KB //16384 = 16KB //24576 = 24KB

typedef struct pipe_control_block
{
	char BUFFER[Max_Pipe_Buffer]; // buffer for transfering data

	uint W,R; //pointers for write and read functions

	int w_closed; //need them to check when reader or writer are  closed
	int r_closed;

	int unread_bytes; //counter for bytes that have not been read
	int free_bytes;   //counter for free bytes on which we can write

	FCB* reader; // reader of pipe
	FCB* writer; // writer of pipe

	CondVar cv_Empty; //cv for checking if pipe_buffer is Empty
	CondVar cv_Full; //cv for checking if pipe_buffer is Full

	Fid_t fid_r;
    Fid_t fid_w;

}PipeCB; 

PipeCB* initialize_pipe();
/**************************************************************/
int rpipe_read(void* dev, char *buf, unsigned int size);

int rpipe_close(void* dev);

int rpipe_write(void* dev, const char* buf, unsigned int size);
/**************************************************************/
int wpipe_read(void* dev, char *buf, unsigned int size);

int wpipe_write(void* dev, const char* buf, unsigned int size);

int wpipe_close(void* dev);

#endif