
#include "tinyos.h"
#include "kernel_sched.h"
#include "kernel_proc.h"
#include "kernel_threads.h"
#include "kernel_cc.h"
#include <assert.h>


/** 
  @brief Create a new thread in the current process.
  */
Tid_t sys_CreateThread(Task task, int argl, void* args)
{
  PTCB* ptcb = allocate_ptcb(task,argl,args);

  ptcb->pcb = CURPROC;
   
  if(task!=NULL){
    TCB* tcb = spawn_thread(ptcb->pcb,start_thread);
    ptcb->tcb=tcb;
    ptcb->pcb->ptcb_counter++;
    tcb->owner_ptcb=ptcb;
    wakeup(ptcb->tcb);
  }
  
  rlnode_init(&ptcb->PTCB_node,ptcb);
  rlist_push_back(&CURPROC->PTCB_list,&ptcb->PTCB_node);
  
	return (Tid_t) ptcb;
}

PTCB* allocate_ptcb(Task task, int argl, void* args) /*initialiaze ptcb*/
{
  PTCB* ptcb = (PTCB*) xmalloc(sizeof(PTCB));

  ptcb->pcb_task=task;
  ptcb->argl=argl;
  ptcb->args=args;
  ptcb->exited=0;
  ptcb->detatched=0;
  ptcb->cv_join_thread=COND_INIT;
  ptcb->ref_count=0;

  return ptcb;
}

void start_thread(){

    int exitval;

    Task call = CURPTCB->pcb_task;
    int argl = CURPTCB->argl;
    void* args = CURPTCB->args;

    exitval = call(argl,args);
    ThreadExit(exitval);

}

/**
  @brief Return the Tid of the current thread.
 */
Tid_t sys_ThreadSelf()
{
	return (Tid_t) CURPTCB;
}

/**
  @brief Join the given thread.
  */
int sys_ThreadJoin(Tid_t tid, int* exitval)
{
  PTCB* ptcb = (PTCB*) tid;
  
  if((rlist_find(&ptcb->pcb->PTCB_list,ptcb,NULL) != NULL)) { //check if ptcb exist
     
    if (((Tid_t) ptcb != sys_ThreadSelf()) && (ptcb->detatched == 0) && (CURTHREAD->state != STOPPED)) { //check if ptcb is not detached and CURTHREAD not stopped
                                                                                                         //check if ptcb is not the same with CURPTCB
            ptcb->ref_count++;                                                                
            
            while(ptcb->exited ==0 && ptcb->detatched ==0) {kernel_wait(&ptcb->cv_join_thread,SCHED_USER);} //condition for waiting threads
            
            if (exitval != NULL) *exitval = ptcb->exit_val; //check if thread has returned            
            
            ptcb->ref_count--;                                       
            deletePTCB(ptcb);     //try to delete ptcb                                  
            return 0;             // we have success        
    }

   deletePTCB(ptcb);  //delete in case of threads joining the first ptcb
  }

	return -1; // we have fail
}

void deletePTCB(PTCB* ptcb){

    if(ptcb->ref_count == 0 && ptcb->exited == 1){   /** We release PTCB when thread is not joined with any other thread and state is exited **/
        rlist_remove(&ptcb->PTCB_node);
        free(ptcb);
    }
}

/**
  @brief Detach the given thread.
  */
int sys_ThreadDetach(Tid_t tid)
{
  PTCB* ptcb = (PTCB*) tid;
    
  if((rlist_find(&ptcb->pcb->PTCB_list,ptcb,NULL) != NULL) && (ptcb->exited == 0)){         /*thread exists and is not exited*/
     ptcb->detatched = 1;
     ptcb->ref_count =0;
     Cond_Broadcast(&ptcb->cv_join_thread);
     return 0;
  }else { return -1; } 
}

/**
  @brief Terminate the current thread.
  */
void sys_ThreadExit(int exitval)
{
  CURPTCB->exit_val = exitval; 

  CURPTCB->exited=1;

  Cond_Broadcast(&CURPTCB->cv_join_thread);

  kernel_sleep(EXITED,SCHED_USER);
}

