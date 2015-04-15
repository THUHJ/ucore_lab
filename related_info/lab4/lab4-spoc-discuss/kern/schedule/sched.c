#include <list.h>
#include <sync.h>
#include <proc.h>
#include <sched.h>
#include <assert.h>
int tick=0;
int dream[100];
int do_sleep(int time)
{
	cprintf("pid = %d, name = %s, status switch from runnable to sleep\n", current->pid, get_proc_name(current));
	dream[current->pid]=tick+time;
	current->state = PROC_SLEEPING;
	current->wait_state = WT_CHILD;
	schedule();
}

void
wakeup_proc(struct proc_struct *proc) {
	if (proc->state == PROC_UNINIT)
	{
		cprintf("pid = %d, status switch from uninit to runnable\n", proc->pid, get_proc_name(proc));
	}
	else
	{
		cprintf("pid = %d, name = %s, status switch from sleep to runnable\n", proc->pid, get_proc_name(proc));
	}

    assert(proc->state != PROC_ZOMBIE && proc->state != PROC_RUNNABLE);
    proc->state = PROC_RUNNABLE;
}

void
schedule(void) {
	tick++;
	struct proc_struct *proc;
	list_entry_t *list1 = &proc_list,  *le1 = list1;
	while ((le1 = list_next(le1)) != list1) {
			proc = le2proc(le1, list_link);
			int pid = proc->pid;
			if (dream[pid]==tick)
				wakeup_proc(proc);
		}

    bool intr_flag;
    list_entry_t *le, *last;
    struct proc_struct *next = NULL;
    local_intr_save(intr_flag);
    {
        current->need_resched = 0;
        last = (current == idleproc) ? &proc_list : &(current->list_link);
        le = last;
        do {
            if ((le = list_next(le)) != &proc_list) {
                next = le2proc(le, list_link);
                if (next->state == PROC_RUNNABLE) {
                    break;
                }
            }
        } while (le != last);
        if (next == NULL || next->state != PROC_RUNNABLE) {
            next = idleproc;
        }
        next->runs ++;
        if (next != current) {
            proc_run(next);
        }
    }
    local_intr_restore(intr_flag);
}

