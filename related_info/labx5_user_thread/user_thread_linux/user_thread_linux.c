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
int a[1000000];
struct thread_struct * new_tcb()
{
	struct thread_struct *proc = malloc(sizeof(struct thread_struct));
	proc->stack  = malloc(4096*2)+4096*2;
	if (proc->stack==NULL)
	{
		printf("stack = null\n");
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
	printf("new thread,pid=%d, my parent=%d\n",proc->pid,proc->parent);
	return proc->pid;
}

void schedule()
{
	printf("\n-----schedlue\n");
	
	int i=0;
	for (i=0;i<t_num;i++)
	{
		printf("pid = %d,parent = %d\n",s[i]->pid,s[i]->parent);
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
		printf("WTF!!!!!!\n");
	}
	
	printf("schedule from %d to %d\n",current->pid,next->pid);
	current =next;
	printf("-----schedlue\n");
	switch_to(&(pre->context),&(next->context));
	
}
void yield()
{
	printf("pid=%d yield\n",current->pid);
	schedule();
}
void exit()
{
	current->state = PROC_ZOMBIE;
	printf("pid=%d exit\n",current->pid);
	yield();
}
void fun1()
{
	int pid = current->pid;
	printf("i am thread %d step1\n",pid);
	yield();
	printf("i am thread %d step2\n",pid);
	exit();
}
void wait()
{
	while (1)
	{
		int i;
		int p=1;
		for (i=0;i<t_num;i++)
			if (s[i]->parent==current->pid && s[i]->state!=PROC_ZOMBIE)
				p=0;
		if (p) break;
		printf("pid=%d is still waitting!\n",current->pid);
		yield();
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
		printf("pid = %d,parent = %d\n",s[i]->pid,s[i]->parent);
	}
	printf("i am thread one\n");
	wait(0);
	printf("all end\n");
}
