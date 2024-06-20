#include <kernel/bsp_port/fbdev.h>
#include <kernel/bsp_port/uart.h>
#include <kernel/fs/dev_framebuffer.h>
#include <kernel/fs/vfs.h>
/*#include <kernel/lock.h>*/
#include <kernel/memory.h>
#include <lib/string.h>

struct file_operations dev_framebuffer_operations = {
    dev_framebuffer_write,   (void *)dev_framebuffer_deny,
    dev_framebuffer_open,    dev_framebuffer_close,
    dev_framebuffer_lseek64, (void *)dev_framebuffer_deny};

int init_dev_framebuffer() {
    int err = fb_init();
    if (err != 0) {
        uart_puts("Failed to init framebuffer\n");
        return err;
    }

    return register_dev(&dev_framebuffer_operations);
}

int dev_framebuffer_write(struct file *file, const void *buf, size_t len) {
    /*lock();*/
    int err = fb_write(file->f_pos, buf, len);
    if (err != 0) {
        uart_puts("Failed to write to framebuffer\n");
        return err;
    }
    file->f_pos += len;
    /*unlock();*/
    return len;
}

int dev_framebuffer_open(struct vnode *file_node, struct file **target) {
    (*target)->f_pos = 0;
    (*target)->vnode = file_node;
    (*target)->f_ops = &dev_framebuffer_operations;
    return 0;
}

int dev_framebuffer_close(struct file *file) {
    kfree(file);
    return 0;
}

long dev_framebuffer_lseek64(struct file *file, long offset, int whence) {
    if (whence == SEEK_SET) {
        file->f_pos = offset;
        return file->f_pos;
    }
    return -1;
}

int dev_framebuffer_deny() { return -1; }
