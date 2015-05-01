#include <stdio.h>
#define PROC_NAME_LEN               15
struct context {
    int eip;
    int esp;
    int ebx;
    int ecx;
    int edx;
	int esi;
    int edi;
    int ebp;
};
enum proc_state {
    PROC_UNINIT = 0,  // uninitialized
    PROC_SLEEPING,    // sleeping
    PROC_RUNNABLE,    // runnable(maybe running)
    PROC_ZOMBIE,      // almost dead, and wait parent proc to reclaim his resource
};
struct thread_struct {
    enum proc_state state;                      // thread state
    int pid;                                    // thread ID
    int runs;                                   // the running times of Proces
    int stack;                           // thread kernel stack
    struct context context;                     // Switch here to run process
    char name[PROC_NAME_LEN + 1];               // Process name
	int parent;
	int tnum;
    
};
void switch_to(struct context *from, struct context *to);
struct thread_struct* s[100];
struct thread_struct* current;
int t_num=0;
int p_num=0;
int get_pid()
{
	return p_num++;
}
int a[409600];
struct thread_struct * new_tcb()
{
	struct thread_struct *proc = a+t_num*4096+80;
	
	proc->stack  = a+t_num*4096+4096;
	if (proc->stack==NULL)
	{
		cprintf("stack = null\n");
	}
	proc->context.esp = proc->stack;
	proc->context.ebp = proc->stack;
	proc->state = PROC_RUNNABLE;
	proc->pid = get_pid();
	proc->tnum = t_num;
	proc->parent = -1;
	s[t_num]=proc;
	t_num=t_num+1;
	return proc;
}
int new_thread(void (*fun)())
{
	struct thread_struct *proc = new_tcb();
	proc->context.eip = fun;
	proc->parent = current->pid;
	cprintf("new thread,pid=%d, my parent=%d\n",proc->pid,proc->parent);
	return proc->pid;
}

void thread_schedule()
{
	cprintf("\n-----schedlue\n");
	
	int i=0;
	for (i=0;i<t_num;i++)
	{
		cprintf("pid = %d,parent = %d  state=%d\n",s[i]->pid,s[i]->parent,s[i]->state);
	}
	struct thread_struct *next=NULL;
	struct thread_struct *pre=current;
	for (i=current->tnum+1;i<t_num;i++)
	{
		
		if (s[i]->state==PROC_RUNNABLE)
		{
			next = s[i];
			break;
		}
	}
	if (next==NULL)
	for (i=0;i<current->tnum+1;i++)
	{
		
		if (s[i]->state==PROC_RUNNABLE)
		{
			next = s[i];
			break;
		}
	}
	if (next==NULL)
	{
		cprintf("WTF!!!!!!\n");
	}
	
	cprintf("schedule from %d to %d\n",current->pid,next->pid);
	current =next;
	cprintf("-----schedlue\n");
	switch_to(&(pre->context),&(next->context));
	
}
void thread_yield()
{
	cprintf("pid=%d yield\n",current->pid);
	thread_schedule();
}
void thread_exit()
{
	current->state = PROC_ZOMBIE;
	cprintf("pid=%d exit\n",current->pid);
	thread_yield();
}
void fun1()
{
	int pid = current->pid;
	cprintf("i am thread %d step1\n",pid);
	thread_yield();
	cprintf("i am thread %d step2\n",pid);
	thread_exit();
}
void thread_wait()
{
	while (1)
	{
		int i;
		int p=1;
		for (i=0;i<t_num;i++)
			if (s[i]->parent==current->pid && s[i]->state!=PROC_ZOMBIE)
				p=0;
		if (p) break;
		cprintf("pid=%d is still waitting!\n",current->pid);
		thread_yield();
	}
}
int main()
{
	current = new_tcb();
	int ret = new_thread(fun1);
	ret = new_thread(fun1);
	int i=0;
	for (i=0;i<t_num;i++)
	{
		cprintf("pid = %d,parent = %d  state=%d\n",s[i]->pid,s[i]->parent,s[i]->state);
	}
	cprintf("i am thread one\n");
	thread_wait(0);
	cprintf("all end\n");
}
