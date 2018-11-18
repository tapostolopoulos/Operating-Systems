#include "tinyos.h"
#include "kernel_pipe.h"
#include "kernel_dev.h"
#include "kernel_streams.h"
#include "kernel_cc.h"
#include <string.h>
#include <assert.h>

static file_ops reader_ops = {
        .Open = NULL,
        .Read = rpipe_read,
        .Write = rpipe_write,
        .Close = rpipe_close
};

static file_ops writer_ops = {
        .Open = NULL,
        .Read = wpipe_read,
        .Write = wpipe_write,
        .Close = wpipe_close
};

int sys_Pipe(pipe_t* pipe)
{
	FCB *fcb_writer = NULL;
    FCB *fcb_reader = NULL;
    FCB *pipe_fcb_table[2];
    Fid_t pipe_fid_table[2];

    int reserved = FCB_reserve(2, pipe_fid_table, pipe_fcb_table);

    if (reserved==0) return -1;

    fcb_reader = pipe_fcb_table[0];
    fcb_writer = pipe_fcb_table[1];    

    pipe->read = pipe_fid_table[0];
    pipe->write = pipe_fid_table[1];
    
    PipeCB* pipcb = initialize_pipe(); //starting the pipe

    pipcb->fid_r = pipe->read;
    pipcb->fid_w = pipe->write;

    fcb_reader->streamfunc = &reader_ops; //give reader's ops
    fcb_reader->streamobj = pipcb;

    fcb_writer->streamfunc = &writer_ops; //give writer's ops
    fcb_writer->streamobj = pipcb;

    return 0;
}

PipeCB* initialize_pipe()
{
	PipeCB* pipcb = (PipeCB*) xmalloc(sizeof(PipeCB));

	pipcb->W = 0;
	pipcb->R = 0;
	pipcb->w_closed = 0;
	pipcb->r_closed = 0;
	pipcb->unread_bytes = 0;
	pipcb->free_bytes = Max_Pipe_Buffer;
    pipcb->cv_Empty = COND_INIT;
    pipcb->cv_Full = COND_INIT;

    return pipcb;
}
/**********************************************************
***************** PIPE_READERS OPS*************************
***********************************************************/
int rpipe_read(void* dev, char *buf, unsigned int size)
{
	PipeCB* pipe = (PipeCB*) dev;
	int read_bytes = 0;
	int i = 0;
    int m = 0;

	while(pipe->unread_bytes == 0 && pipe->w_closed == 0){
        kernel_broadcast(&pipe->cv_Empty);            //buffer is empty               
        kernel_wait(&pipe->cv_Full,SCHED_PIPE);       //sleep reader while !cv_full
    } 
    
    if(size>pipe->unread_bytes){
         m=pipe->unread_bytes;
    }else { m=size;}

    for(i=0;i<m;i++){
        if(pipe->R == Max_Pipe_Buffer) pipe->R=0;

        buf[i] = pipe->BUFFER[pipe->R];
        pipe->R++;
    	read_bytes++;
    	pipe->unread_bytes--;
    	pipe->free_bytes++;
    }  

    kernel_broadcast(&pipe->cv_Empty);   //we have read data and ready to write again

    return read_bytes;
}

int rpipe_close(void* dev)
{
	PipeCB* pipe = (PipeCB *) dev;

    if(pipe->w_closed) return 0;

    pipe->r_closed = 1;

    return -1;
}

int rpipe_write(void* dev, const char* buf, unsigned int size)
{
	return -1;
}
/********************************************************
**************** PIPE_WRITERS OPS************************
*********************************************************/
int wpipe_write(void* dev, const char* buf, unsigned int size)
{
	PipeCB* pipe = (PipeCB*) dev;
    int write_bytes = 0; 
    int i = 0;
    
    if(pipe->r_closed==1){ 
        pipe->w_closed = 1; 
    	return -1;
    }

    while(size > pipe->free_bytes){
        kernel_broadcast(&pipe->cv_Full);            //buffer is full  
        kernel_wait(&pipe->cv_Empty,SCHED_PIPE);     //sleep writer while !cv_empty
    }   
    
    for(i=0;i<size;i++){
        if(pipe->W == Max_Pipe_Buffer) pipe->W=0;

        pipe->BUFFER[pipe->W] = buf[i];        
    	pipe->W++;
    	write_bytes++;
    	pipe->unread_bytes++;
    	pipe->free_bytes--;
    }  

    kernel_broadcast(&pipe->cv_Full);   //we have write data and we are ready to read again

    return write_bytes;
}

int wpipe_close(void* dev)
{
	PipeCB* pipe = (PipeCB *) dev;

    if(pipe->r_closed) return 0;

    pipe->w_closed = 1;

    return -1;
}

int wpipe_read(void* dev, char *buf, unsigned int size)
{
	return -1;
}
