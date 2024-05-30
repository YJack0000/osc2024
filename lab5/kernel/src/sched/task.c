#include <kernel/io.h>
#include <kernel/irq_entry.h>
#include <kernel/memory.h>
#include <kernel/sched.h>

static struct task_struct init_task = INIT_TASK;
struct task_struct *current = &(init_task);
struct task_struct *task[NR_TASKS] = {
    &(init_task),
};
int nr_tasks = 1;

void preempt_disable(void) { current->preempt_count++; }

void preempt_enable(void) { current->preempt_count--; }

void _schedule(void) {
    preempt_disable();
    int next, c;
    struct task_struct *p;
    while (1) {
        c = -1;
        next = 0;
        for (int i = 0; i < NR_TASKS;
             i++) {  // find the task with the maximum counter
            p = task[i];
            if (p && p->state == TASK_RUNNING && p->counter > c) {
                c = p->counter;
                next = i;
            }
        }
        if (c) {  // found & break for switch to the task
            break;
        }
        // if no task is found, increase the counter of all tasks
        for (int i = 0; i < NR_TASKS; i++) {
            p = task[i];
            if (p) {
                p->counter = (p->counter >> 1) + p->priority;
            }
        }
    }
    switch_to(task[next]);
    preempt_enable();
}
void schedule(void)
{
	current->counter = 0;
	_schedule();
}

void switch_to(struct task_struct * next) 
{
    print_string("\r\n[SWITCH] ");
    print_d(current->pid);
    print_string(" -> ");
    print_d(next->pid);
    print_string("\r\n");
	if (current == next) 
		return;
	struct task_struct * prev = current;
	current = next;
	cpu_switch_to(prev, next);
}

void schedule_tail(void) {
	preempt_enable();
}


void timer_tick()
{
	--current->counter;
	if (current->counter>0 || current->preempt_count >0) {
		return;
	}
	current->counter=0;
	enable_irq();
	_schedule();
	disable_irq();
}

void kill_zombies()
{
	for (int i = 0; i < NR_TASKS; i++) {
		if (task[i] && task[i]->state == TASK_ZOMBIE) {
			struct task_struct * p = task[i];
			task[i] = 0;
			print_string("\r\n[KILL] zombies, pid: "); print_d(p->pid);
			// kfree(p);
		}
	}
}

void exit_process()
{
	preempt_disable();
	current->state = TASK_ZOMBIE;
	// kfree((void*)current->stack);
	preempt_enable();
	schedule();
}
