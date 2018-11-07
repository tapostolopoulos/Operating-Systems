#ifndef __KERNEL_THREADS_H
#define __KERNEL_THREADS_H

#include "tinyos.h"
#include "kernel_sched.h"
#include "util.h"
#include "kernel_streams.h"

#define CURPTCB  (CURTHREAD->owner_ptcb)

typedef struct process_thread_control_block{
	TCB* tcb; /*the thread of PTCB*/
	PCB* pcb; /*the pcb of PTCB*/

	Task pcb_task; /*the tast of parent */

	int argl; /*arguments of task*/
	void* args; /*       >>         */

	int exited; /*ptcb is exited*/
	int detatched; /*ptcb is detached*/
	int exit_val; /*value when thread is exited*/

	CondVar cv_join_thread; /*cv for joined threads*/
	int ref_count;         /*counter for listed joined threads*/

	rlnode PTCB_node;
} PTCB;

PTCB* allocate_ptcb(Task task, int argl, void* args); /*initialiaze ptcb*/

void start_thread();

void deletePTCB(PTCB* ptcb);

#endif
