#lab5X  在用户态完成线程切换和线程管理
黄杰 2012011272  
袁源 2012011294   
杜鹃 2012011354

##设计思路
原本的ucore实现线程切换和管理是在内核完成的，题目要求的在用户态完成线程切换，即把一些切换线程所需的工作写到用户程序中（本实现中，为user_thread.c文件），再用ucore加载此用户程序即可实现。而切换线程所需的工作主要包括，线程控制块的创建，线程资源的分配（内存管理，堆栈空间等）等；而线程管理所需的工作主要包括，线程的调度函数schedule，以及涉及到线程状态转换的函数wait、yield等。

##算法实现  
说明：实现文件为 user_thread.c文件。

1. 主要数据结构  
 - 线程控制块  
 是ucore中的proc_struc的简化，去掉了一些不必要的变量。

```
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
```  


 - context上下文  
 与ucore里的实现一样，保存的寄存器信息。  

```
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
```
 

2.  为线程分配空间    
本程序专门开辟了一片连续的空间作为创建的线程需要占用的空间（全局数组a[409600] ）。新建线程时，主要的实现函数是：  
  
```
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
```
其中第一句`struct thread_struct *proc = a+t_num*4096+80;`是为整个线程分配空间，t_num是线程的数量，同时记录在a[]数组中的位置信息；4096是为每个线程的stack开辟的空间；因为每个一个thread_struct大小是72字节，所以为线程本身另留了80字节的空间。  
第二句`proc->stack  = a+t_num*4096+4096;`是记录stack的位置。之后的exp和ebp保存了stack的值。  
本程序中为了实现更为简便清晰，采用数组的方式保存所有的线程，而不是像ucore中用list串联起来。具体实现为全局变量 `struct thread_struct* s[100];`。在`new_tcb`函数中，`s[t_num]=proc;`此句将新建的线程添加到数组中，使得之后可以进行线程的管理。  

3. 线程的调度管理  
主要实现函数：  
thread_schedule() 调度函数
thread_yield() 调用schedule函数
thread_exit() 子进程结束，转换状态
thread_wait() 父进程等待子进程结束  

实现过程中，为简化过程，线程状态只设置了3个：  
```
enum proc_state {
    PROC_SLEEPING,    // sleeping
    PROC_RUNNABLE,    // runnable(maybe running)
    PROC_ZOMBIE,      // almost dead, and wait parent proc to reclaim his resource
};  
```  
在新建一个线程时，直接将其状态设置为runnable可调度。  
在线程exit时，状态转换为zombie。  

schedule函数主要做的工作：  
- 找到下一个可以run的线程  
- 调用switch函数进行线程的切换。switch函数实现依旧为汇编程序实现。



##测试及结果   
用户程序的测试代码：  
创建了两个线程（调用两边new_thread)
```
int main()
{
	current = new_tcb();
	int ret = new_thread(fun1);
	ret = new_thread(fun1);
	int i=0;
	for (i=0;i<t_num;i++)
	{
		cprintf("pid = %d,parent = %d\n",s[i]->pid,s[i]->parent);
	}
	cprintf("i am thread one\n");
	thread_wait(0);
	cprintf("all end\n");
}  
```  
每个线程的功能为实现fun函数，fun函数主要打印一下线程信息：  
```
void fun1()
{
	int pid = current->pid;
	cprintf("pid_before= %d\n",current->pid);
	cprintf("i am thread %d step1\n",pid);
	cprintf("pid_after= %d\n",current->pid);
	thread_yield();
	cprintf("i am thread %d step2\n",pid);
	thread_exit();
}  
```  
运行程序的方式：
在ucore中 make run-user_thread 即可，ucore会将其作为用户程序加载进去。  
注意，因为用户程序调用到了switch_to函数，所以需要把switch_to.s加到lib文件夹中。  

打印出的信息如下：  
//内的为补充说明的内容  
```
++ setup timer interrupts
kernel_execve: pid = 2, name = "user_thread".
//初始的线程的编号为0，在其中新建了两个子线程
new thread,pid=1, my parent=0//子线程的编号一个为1
new thread,pid=2, my parent=0//另一个子线程的编号为2
pid = 0,parent = -1  state=2//打印三个线程的内容和状态，2表示runnable
pid = 1,parent = 0  state=2
pid = 2,parent = 0  state=2
i am thread one
pid=0 is still waitting!//在创建完两个子线程后，父线程进入wait的状态，等到子线程都运行完了才结束。
pid=0 yield

-----schedlue//线程调度的过程
pid = 0,parent = -1  state=2
pid = 1,parent = 0  state=2
pid = 2,parent = 0  state=2
schedule from 0 to 1//从线程0调度到线程1
-----schedlue
i am thread 1 step1//线程1运行一步后，调用yield放弃对cpu的使用
pid=1 yield

-----schedlue//在yield中调用schedule进行线程调度
pid = 0,parent = -1  state=2
pid = 1,parent = 0  state=2
pid = 2,parent = 0  state=2
schedule from 1 to 2//从1到2
-----schedlue
i am thread 2 step1//线程2运行了一步
pid=2 yield

-----schedlue
pid = 0,parent = -1  state=2
pid = 1,parent = 0  state=2
pid = 2,parent = 0  state=2
schedule from 2 to 0//从线程2调度到线程0
-----schedlue
pid=0 is still waitting!//父线程判断了下子线程的运行状况，发现子线程还在继续运行，就继续等待。
pid=0 yield

-----schedlue
pid = 0,parent = -1  state=2
pid = 1,parent = 0  state=2
pid = 2,parent = 0  state=2
schedule from 0 to 1
-----schedlue
i am thread 1 step2//线程1运行第二步
pid=1 exit//运行完后结束线程
pid=1 yield

-----schedlue
pid = 0,parent = -1  state=2
pid = 1,parent = 0  state=3
pid = 2,parent = 0  state=2
schedule from 1 to 2//从1调度到2
-----schedlue
i am thread 2 step2
pid=2 exit//2运行完第二步，然后就结束了线程
pid=2 yield

-----schedlue
pid = 0,parent = -1  state=2
pid = 1,parent = 0  state=3
pid = 2,parent = 0  state=3
schedule from 2 to 0//在线程0中，此时父进程判断出子进程都运行完了，结束等待
-----schedlue
all end//最终结束运行
all user-mode processes have quit.

```  
以上所有的线程创建、调度等工作都是在用户态完成的，不能被内核感知到。




PS : 在实现过程中，我们首先实现了一个可在linux环境下运行的程序（见user_thread_linux.c文件）。user_thread.c的实现，是在user_thread_linux.c 基础上，增加了相关的内存分配部分。（ucore中没有malloc函数，在这里用了全局数组a来开辟内存的方式，简化了对内存分配的实现）运行此程序还需要用到switch_to.s文件。  
运行此程序可用我们写好的makefile文件,makefile文件非常简单：  
```
cc switch_to.s user_thread_linux.c -m32 -o user 
```

