#include <kernel/bsp_port/entry.h>
#include <kernel/bsp_port/ramfs.h>
#include <kernel/commands.h>
#include <kernel/io.h>
#include <kernel/memory.h>
#include <lib/stdlib.h>
#include <lib/string.h>

void _ls_command(int argc, char **argv) {
    file_list_t *file_list = ramfs_get_file_list();
    for (int i = 0; i < file_list->file_count; i++) {
        puts("\n");
        puts(file_list->file_names[i]);
        puts(" ");
        print_d(file_list->file_sizes[i]);
        puts(" bytes");
    }
    puts("\n");
}

command_t ls_command = {.name = "cpio_ls",
                        .description = "list the files in ramfs",
                        .function = &_ls_command};

void _cat_command(int argc, char **argv) {
    if (argc < 2) {
        puts("\nUsage: cat <file_name>\n");
        return;
    }
    char *file_name = argv[1];

    unsigned long file_size = ramfs_get_file_size(file_name);
    char *file_contents = kmalloc(file_size);
    ramfs_get_file_contents(file_name, file_contents);

    if (file_contents) {
        puts("\n");
        for (int i = 0; i < file_size - 1; i++) {
            print_char(file_contents[i]);
        }
    } else {
        puts("\nFile not found\n");
    }
}

command_t cat_command = {.name = "cpio_cat",
                         .description = "print the contents of a file",
                         .function = &_cat_command};

// void _exec_command(int argc, char **argv) {
//     if (argc < 2) {
//         puts("\nUsage: exec <filename>");
//         return;
//     }
//
//     char *file_name = argv[1];
//     char *file_contents = ramfs_get_file_contents(file_name);
//     uint32_t file_size = ramfs_get_file_size(file_name);
//     if (file_contents == NULL) {
//         puts("[ERROR] File not found: ");
//         puts(file_name);
//         puts("\n");
//         return;
//     }
//     puts("\n[INFO] Executing file: ");
//     puts(file_name);
//     puts("\n");
//
//     char *user_program = (void *)USER_PROGRAM_BASE;
//     memcpy(user_program, file_contents, file_size);
//     from_el1_to_el0(USER_PROGRAM_BASE, USER_STACK_POINTER_BASE);
// }
//
// command_t exec_command = {.name = "exec",
//                                .description = "execute a user program",
//                                .function = &_exec_command};
