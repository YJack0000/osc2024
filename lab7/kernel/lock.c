#include <kernel/lock.h>
#include <kernel/bsp_port/irq.h>

static unsigned long long lock_count = 0;
void lock()
{
    disable_irq();
    lock_count++;
}

void unlock()
{
    lock_count--;
    if (lock_count == 0)
        enable_irq();
}
