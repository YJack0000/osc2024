#include <kernel/bsp_port/irq.h>
#include <kernel/bsp_port/mailbox.h>
#include <kernel/bsp_port/ramfs.h>
#include <kernel/syscall.h>
#include <kernel/bsp_port/uart.h>
#include <kernel/io.h>
#include <kernel/memory.h>
#include <kernel/sched.h>
#include <kernel/signal.h>
#include <bsp/asm/traps.h>
#include <lib/string.h>
#include <lib/utils.h>

extern void child_ret_from_fork();

int sys_getpid() { return get_current()->pid; }

size_t sys_uart_read(char *buf, size_t size) {
    int i = 0;
    while (i < size) buf[i++] = uart_getc();
    return i;
}

size_t sys_uart_write(const char *buf, size_t size) {
    int i = 0;
while (i < size) uart_send(buf[i++]);
    return i;
}

int sys_exec(const char *file_name, char *const argv[]) {
    unsigned long file_size = ramfs_get_file_size(file_name);
    char *file_contents = kmalloc(file_size);
    ramfs_get_file_contents(file_name, file_contents);

    if (file_contents == NULL) {
        puts("[ERROR] File not found: ");
        puts(file_name);
        puts("\n");
        return 0;
    }
    puts("\n[INFO] Executing file: ");
    puts(file_name);
    puts("\n");

    char *user_program = kmalloc(file_size);
    get_current()->state = TASK_STOPPED;
    memcpy(user_program, file_contents, file_size);
    get_current()->sigpending = 0;
    memset(get_current()->sighand, 0, sizeof(get_current()->sighand));
    get_current()->context.lr = (unsigned long)user_program;
    return 0;
}

int sys_fork(trap_frame *tf) {
    disable_irq();
    struct task_struct *parent = get_current();
    struct task_struct *child = kthread_create(0);

    // child->parent_pid = parent->pid;
    // parent->child_count++;
    // Copy the parent's memory
    memcpy(child->stack, parent->stack, STACK_SIZE);
    memcpy(child->user_stack, parent->user_stack, STACK_SIZE);
    memcpy(child->sighand, parent->sighand, sizeof(parent->sighand));

    unsigned long sp_off = (unsigned long)tf - (unsigned long)parent->stack;
    trap_frame *child_trap_frame = (trap_frame *)(child->stack + sp_off);

    child->context.sp = (unsigned long)child_trap_frame;
    child->context.lr = (unsigned long)child_ret_from_fork;

    unsigned long sp_el0_off = tf->sp_el0 - (unsigned long)parent->user_stack;
    child_trap_frame->sp_el0 = (unsigned long)child->user_stack + sp_el0_off;
    child_trap_frame->x0 = 0;

    enable_irq();
    return child->pid;
}

void sys_exit() { kthread_exit(); }

int sys_mbox_call(unsigned char ch, unsigned int *mbox) {
    return mailbox_call(ch, mbox);
}

void sys_kill(int pid) { kthread_stop(pid); }

void sys_signal(int signum, void (*handler)()) { signal(signum, handler); }

void sys_sigkill(int pid, int sig) { sigkill(pid, sig); }

void sys_sigreturn(trap_frame *regs) {
    // Restore the sigframe
    memcpy(regs, &get_current()->sigframe, sizeof(trap_frame));
    kfree(get_current()->sig_stack);
    get_current()->sighandling = 0;
    return;  // Jump to the previous context (user program) after eret
}

// emit the syscall instruction
static int getpid() {
    long pid = -1;
    asm volatile("mov x8, 0");
    asm volatile("svc 0");
    asm volatile("mov %0, x0" : "=r"(pid));
    return pid;
}

static int fork() {
    long ret = -1;
    asm volatile("mov x8, 4");
    asm volatile("svc 0");
    asm volatile("mov %0, x0" : "=r"(ret));
    return ret;
}

static void exit() {
    asm volatile("mov x8, 5");
    asm volatile("svc 0");
}

void fork_test() {
    puts("Fork Test (pid = ");
    print_d(getpid());
    puts(")\n");
    int cnt = 1;
    int ret = 0;
    if ((ret = fork()) == 0) {
        puts("first child pid: ");
        print_d(getpid());
        puts(", cnt: ");
        print_d(cnt);
        puts(", ptr: ");
        print_h((unsigned long long)&cnt);
        puts("\n");
        cnt++;
        if ((ret = fork()) != 0) {
            puts("first child pid: ");
            print_d(getpid());
            puts(", cnt: ");
            print_d(cnt);
            puts(", ptr: ");
            print_h((unsigned long long)&cnt);
            puts("\n");
        } else {
            while (cnt < 5) {
                puts("second child pid: ");
                print_d(getpid());
                puts(", cnt: ");
                print_d(cnt);
                puts(", ptr: ");
                print_h((unsigned long long)&cnt);
                puts("\n");
                for (int i = 0; i < 1000000; i++);
                cnt++;
            }
        }
        exit();
    } else {
        puts("parent here, pid ");
        print_d(getpid());
        puts(", child ");
        print_d(ret);
        puts("\n");
    }
    exit();
}

void run_fork_test() {
    asm volatile("msr spsr_el1, %0" ::"r"(0x0));
    asm volatile("msr elr_el1, %0" ::"r"(fork_test));
    asm volatile("msr sp_el0, %0" ::"r"(get_current()->context.sp));
    asm volatile("mov sp, %0" ::"r"(get_current()->stack + STACK_SIZE));
    asm volatile("eret");
}

int sys_open(trap_frame *tf, const char *pathname, int flags)
{
    char abs_path[MAX_PATH_NAME];
    strcpy(abs_path, pathname);
    // update abs_path
    get_absolute_path(abs_path, get_current()->cwd);
    for (int i = 0; i < MAX_FD; i++)
    {
        // find a usable fd
        if(!get_current()->fdtable->fds[i])
        {
            if(vfs_open(abs_path, flags, &get_current()->fdtable->fds[i])!=0)
            {
                break;
            }

            tf->x0 = i;
            return i;
        }
    }

    tf->x0 = -1;
    return -1;
}

int sys_close(trap_frame *tf, int fd)
{
    // find an opened fd
    if(get_current()->fdtable->fds[fd])
    {
        vfs_close(get_current()->fdtable->fds[fd]);
        get_current()->fdtable->fds[fd] = 0;
        tf->x0 = 0;
        return 0;
    }

    tf->x0 = -1;
    return -1;
}

long sys_write(trap_frame *tf, int fd, const void *buf, unsigned long count)
{
    // find an opened fd
    if (get_current()->fdtable->fds[fd])
    {
        tf->x0 = vfs_write(get_current()->fdtable->fds[fd], buf, count);
        return tf->x0;
    }

    tf->x0 = -1;
    return tf->x0;
}

long sys_read(trap_frame *tf, int fd, void *buf, unsigned long count)
{
    // find an opened fd
    if (get_current()->fdtable->fds[fd])
    {
        tf->x0 = vfs_read(get_current()->fdtable->fds[fd], buf, count);
        return tf->x0;
    }

    tf->x0 = -1;
    return tf->x0;
}

long sys_mkdir(trap_frame *tf, const char *pathname, unsigned mode)
{
    char abs_path[MAX_PATH_NAME];
    strcpy(abs_path, pathname);
    get_absolute_path(abs_path, get_current()->cwd);
    tf->x0 = vfs_mkdir(abs_path);
    return tf->x0;
}

long sys_mount(trap_frame *tf, const char *src, const char *target, const char *filesystem, unsigned long flags, const void *data)
{
    char abs_path[MAX_PATH_NAME];
    strcpy(abs_path, target);
    get_absolute_path(abs_path, get_current()->cwd);

    tf->x0 = vfs_mount(abs_path,filesystem);
    return tf->x0;
}

long sys_chdir(trap_frame *tf, const char *path)
{
    char abs_path[MAX_PATH_NAME];
    strcpy(abs_path, path);
    get_absolute_path(abs_path, get_current()->cwd);
    strcpy(get_current()->cwd, abs_path);

    return 0;
}

long sys_lseek64(trap_frame *tf, int fd, long offset, int whence)
{
    if(whence == SEEK_SET) // used for dev_framebuffer
    {
        get_current()->fdtable->fds[fd]->f_pos = offset;
        tf->x0 = offset;
    }
    else // other is not supported
    {
        tf->x0 = -1;
    }

    return tf->x0;
}

// ioctl 0 will be use to get info
// there will be default value in info
// if it works with default value, you can ignore this syscall
long sys_ioctl(trap_frame *tf, int fb, unsigned long request, void *info)
{
    printf("\r\n[SYSCALL] ioctl - fb: %d, request: %x, info: %x\r\n", fb, request, info);
    tf->x0 = 0;
    return tf->x0;
}
