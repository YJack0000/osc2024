// #include <kernel/bsp_port/ramfs.h>
#include <kernel/io.h>
#include <kernel/memory.h>
#include <kernel/sched.h>
#include <lib/stddef.h>
#include <lib/string.h>
#include <kernel/timer.h>
#include <kernel/sched.h>
#include <kernel/commands.h>
#include "../cpio.h"


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

void _exec_command(int argc, char *argv[])
{

    cpio_newc_header* head = (void*)(uint64_t)0x8000000;

    uint32_t head_size = sizeof(cpio_newc_header);

    char *target_name = "syscall.img";

    while(1)
    {
        int namesize = strtol(head->c_namesize, 16, 8);
        int filesize = strtol(head->c_filesize, 16, 8);

        char *filename = (void*)head + head_size;

        uint32_t offset = head_size + namesize;
        if(offset % 4 != 0) offset = ((offset/4 +1)*4);

        if(strcmp(filename, "TRAILER!!!") == 0){
            print_string("\nFile not found");
            break;
        }
        else if(strcmp(filename, target_name) == 0){
            /* The filedata is appended after filename */
            char *filedata = (void*)head + offset;

            print_string("\r\n[INFO] File found: "); print_string(target_name); print_string(" in CPIO at "); print_h((unsigned long)filedata);
            print_string("\r\n[INFO] File Size: "); print_h(filesize);

            void* user_program_addr = kmalloc(filesize+THREAD_SIZE); // extra page in case
            if(user_program_addr == NULL) return;
            memset((void*)user_program_addr, 0, filesize+THREAD_SIZE);
            for(int i=0; i<filesize; i++){
                ((char*)user_program_addr)[i] = filedata[i];
            }

            preempt_disable(); // leads to get the correct current task

            current->state = TASK_STOPPED;

            unsigned long tmp;
            asm volatile("mrs %0, cntkctl_el1" : "=r"(tmp));
            tmp |= 1;
            asm volatile("msr cntkctl_el1, %0" : : "r"(tmp));

            copy_process(PF_KTHREAD, (unsigned long)&kp_user_mode, (unsigned long)user_program_addr, 0);

            preempt_enable();
            break;
        }

        if(filesize % 4 != 0) filesize = (filesize/4 +1)*4;
        head = (void*)head + offset + filesize;
    }
    return;
}

command_t exec_command = {.name = "exec",
                           .description = "execute a user program",
                           .function = &_exec_command};
