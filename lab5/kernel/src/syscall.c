#include "cpio.h"
#include <kernel/bsp_port/mailbox.h>
// #include <kernel/bsp_port/ramfs.h>
#include <kernel/bsp_port/uart.h>
#include <kernel/io.h>
#include <kernel/memory.h>
#include <kernel/sched.h>
#include <lib/stddef.h>
#include <lib/string.h>

extern struct task_struct* current;

void sys_debug_msg(const char* message) {
    print_string("[Sys DEBUG] ");
    print_string(message);
    print_string("\n");
}

int sys_getpid() {
    // char buf[100];
    // sprintf(buf, "Getting PID: %d\n", current->pid);
    // sys_debug_msg(buf);
    // print_d(current->pid);
    return current->pid;
}

size_t sys_uartread(char buf[], size_t size) {
    // sys_debug_msg("Reading from UART");
    for (size_t i = 0; i < size; i++) {
        buf[i] = uart_recv();
    }
    return size;
}

size_t sys_uartwrite(const char buf[], size_t size) {
    // sys_debug_msg("Writing to UART");
    for (size_t i = 0; i < size; i++) {
        uart_send(buf[i]);
    }
    return size;
}
static int hextoi(char *s, int n) {
    int r = 0;
    while (n-- > 0) {
        r = r << 4;
        if (*s >= 'A')
            r += *s++ - 'A' + 10;
        else if (*s >= '0')
            r += *s++ - '0';
    }
    return r;
}

static uint32_t strtol(const char *sptr, uint32_t base, int size)
{
    uint32_t ret = 0;
    int i=0;
    
    while((sptr[i] != '\0') && (i<size)){
        if(base == 16){
            if(sptr[i] >= '0' && sptr[i] <= '9'){
                ret = ret * 16 + (sptr[i] - '0');
            }
            else if(sptr[i] >= 'a' && sptr[i] <= 'f'){
                ret = ret * 16 + (sptr[i] - 'a' + 10);
            }
            else if(sptr[i] >= 'A' && sptr[i] <= 'F'){
                ret = ret * 16 + (sptr[i] - 'A' + 10);
            }
            else{
                break;
            }
        }
        else if(base == 8 || base == 2){
            if(sptr[i] >= '0' && sptr[i] <= '9'){
                ret = ret * base + (sptr[i] - '0');
            }
            else{
                break;
            }
        }
        i++;
    }
    return ret;
}


int sys_exec(const char *name, char *const argv[]) // [TODO]
{
    // print_string("[INFO] Executing file: ");
    // print_string(name);
    // print_string("\n");
    cpio_newc_header* head = (void*)(uint64_t)0x08000000; // arbitrary address (start of the initramfs

    uint32_t head_size = sizeof(cpio_newc_header);

    while(1)
    {
        int namesize = strtol(head->c_namesize, 16, 8);
        int filesize = strtol(head->c_filesize, 16, 8);

        char *filename = (void*)head + head_size;

        uint32_t offset = head_size + namesize;
        if(offset % 4 != 0) offset = ((offset/4 +1)*4);

        if(strcmp(filename, "TRAILER!!!") == 0){
            // printf("\r\n[ERROR] File not found");
            break;
        }
        else if(strcmp(filename, name) == 0){
            /* The filedata is appended after filename */
            char *filedata = (void*)head + offset;
            void* user_program_addr = kmalloc(filesize+THREAD_SIZE); // extra page in case
            if(user_program_addr == NULL) return -1;
            memset((void *)user_program_addr, 0, filesize+THREAD_SIZE);
            for(int i=0; i<filesize; i++){
                ((char*)user_program_addr)[i] = filedata[i];
            }

            preempt_disable(); // leads to get the correct current task

            struct pt_regs *cur_regs = task_pt_regs(current);
            cur_regs->pc = (unsigned long)user_program_addr;
            cur_regs->sp = current->stack;

            preempt_enable();
            break;  // won't reach here
        }

        if(filesize % 4 != 0) filesize = (filesize/4 +1)*4;
        head = (void*)head + offset + filesize;
    }
    return -1; // only failure
}

int sys_fork() // [TODO]
{
    // print_string("\r\n[INFO] Forking process");
    unsigned long stack = (unsigned long)kmalloc(THREAD_SIZE);
    if((void*)stack == NULL) return -1;
    memset((void*)stack, 0, THREAD_SIZE);
    return copy_process(0, 0, 0, stack);
}

void sys_exit(int status) {
    // sys_debug_msg("Exiting process");
    exit_process();
}

int sys_mbox_call(unsigned char ch, unsigned int *mbox) // [TODO]
{
    // print_string("\r\n[INFO] Mailbox call");
    unsigned int mesg = (((unsigned int)(unsigned long)mbox) & ~0xf) | (ch & 0xf);
    while(*MAILBOX_STATUS & MAILBOX_FULL){   // // Check if Mailbox 0 status register’s full flag is set. if MAILBOX_STATUS == 0x80000001, then error parsing request buffer 
        asm volatile("nop");
    }

    *MAILBOX_WRITE = mesg;

    while(1){
        while(*MAILBOX_STATUS & MAILBOX_EMPTY){
            asm volatile("nop");
        }
        if(mesg == *MAILBOX_READ){
            return mbox[1] == REQUEST_SUCCEED;
        }
    }
    print_string("\r\n[INFO] Mailbox call end");

    return 0;
}

void sys_kill(int pid) {
    struct task_struct* p;
    for (int i = 0; i < NR_TASKS; i++) {
        if (task[i] == NULL) continue;  // (task[i] == NULL) means no more tasks
        p = task[i];
        if (p->pid == pid) {
            preempt_disable();
            print_string("\r\nKilling process: ");
            print_d(pid);
            p->state = TASK_ZOMBIE;
            preempt_enable();
            return;
        }
    }
    print_string("\r\nProcess not found: ");
    print_d(pid);
    return;
}

void* const sys_call_table[] = {sys_getpid,    sys_uartread, sys_uartwrite,
                                sys_exec,      sys_fork,     sys_exit,
                                sys_mbox_call, sys_kill};
