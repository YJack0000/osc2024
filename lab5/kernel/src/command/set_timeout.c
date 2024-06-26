#include <kernel/commands.h>
#include <kernel/io.h>
#include <kernel/timer.h>

void print_timeout(int delay) {
    print_string("\nTimeout: ");
    print_d((const int)delay);
    print_string("s\n");
}

void _set_timeout(int argc, char **argv) {
    if (argc < 2) {
        print_string("\nUsage: set_timeout <timeout>\n");
        return;
    }

    print_string("\nSetting timeout: ");
    print_d(atoi(argv[1]));
    print_string("s\n");

    unsigned long timeout = atoi(argv[1]);
    set_timeout((void *)print_timeout, (void *)timeout, atoi(argv[1]));
}

command_t set_timeout_command = {.name = "set_timeout",
                                      .description = "set a timeout",
                                      .function = &_set_timeout};
