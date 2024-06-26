#include <kernel/commands.h>
#include <kernel/console.h>
#include <kernel/io.h>
#include <lib/stdlib.h>
#include <lib/string.h>

static command_t commands[64];
static int num_commands;

static void read_command(char *);
static void save_command(char *);
static char *load_command(int);
static int parse_command(char *, char **);
void _print_help();
void _clear_command();

void init_console() {
    num_commands = 0;
    register_all_commands();
}

void run_console() {
    char input[256];

    while (1) {
        print_string("\n>>> ");

        read_command(input);
        save_command(input);
        int argc = 0;
        char *argv[16];
        argc = parse_command(input, argv);

        if (argc == 0) {
            continue;
        }
        if (strcmp(argv[0], "help") == 0) {
            _print_help();
            continue;
        }
        if (strcmp(argv[0], "clear") == 0) {
            _clear_command();
            continue;
        }
        int i = 0;
        for (i = 0; i < num_commands; i++) {
            if (strcmp(argv[0], commands[i].name) == 0) {
                commands[i].function(argc, argv);
                break;
            }
        }
        // print_h(i);
        if (i == num_commands) {
            print_string("\ncommand not found\n");
        }
    }
}

void register_command(command_t *command) {
    commands[num_commands] = *command;
    num_commands++;
}

void register_all_commands() {
    register_command(&hello_command);
    register_command(&mailbox_command);
    register_command(&reboot_command);
    register_command(&ls_command);
    register_command(&cat_command);
    register_command(&exec_command);
    register_command(&test_malloc_command);
    register_command(&async_io_demo_command);
    register_command(&set_timeout_command);
    register_command(&test_kmalloc_command);
    register_command(&test_kfree_command);
    register_command(&test_multi_thread_command);
    register_command(&test_thread_command);
}

static void read_command(char *x) {
    char input_char;
    x[0] = 0;
    int input_index = 0;
    while ((input_char = read_c()) != '\n') {
        if (input_char == 127 ||
            input_char == 8) {  // Handle delete and backspace
            if (input_index > 0) {
                input_index--;  // Move back one position
                print_string("\b \b");
            }
            continue;
        } else if (input_char == 27) {  // Handle arrow keys, ESC [ A or B
            input_char = read_c();
            if (input_char == 91) {  // [
                input_char = read_c();
                if (input_char == 65) {  // Up arrow
                    char *command = load_command(1);
                    if (command[0] == 0) {
                        continue;
                    }
                    for (int i = 0; i < input_index; i++) {
                        print_string("\b \b");
                    }
                    print_string(command);
                    strcpy(x, command);
                    input_index = strlen(x);
                    continue;
                } else if (input_char == 66) {  // Down arrow
                    char *command = load_command(-1);
                    if (command[0] == 0) {
                        continue;
                    }
                    for (int i = 0; i < input_index; i++) {
                        print_string("\b \b");
                    }
                    print_string(command);
                    strcpy(x, command);
                    input_index = strlen(x);
                    continue;
                }
            }
        }

        x[input_index] = input_char;
        input_index += 1;
        print_char(input_char);
    }
    if (x[input_index - 1] == '\n') {
        x[input_index - 1] = '\0';
    } else {
        x[input_index] = '\0';
    }
}

// using circular buffer to handle command history
static char command_history[16][64];
static int command_history_index = 0;
static void save_command(char *command) {
    if (command[0] == 0) {  // handle empty command
        return;
    }
    strcpy(command_history[command_history_index], command);
    command_history_index = (command_history_index + 1) % 16;
}
static char *load_command(int change) {  // change = 1 for next, -1 for previous
    change = change < 0 ? 1 : -1;
    if (command_history_index == 0 && change == -1) {
        return "";
    }
    if (command_history_index == 15 && change == 1) {
        return "";
    }

    int next_index = (command_history_index + change + 16) % 16;
    if (command_history[next_index][0] == 0) {  // don't load empty command
        return "";
    }

    command_history_index = next_index;
    return command_history[command_history_index];
}

static int parse_command(char *command, char **argv) {
    int argc = 0;

    char *token = strtok(command, " ");
    while (token != NULL) {
        argv[argc] = token;
        argc++;
        token = strtok(NULL, " ");
    }
    return argc;
}

void _print_help() {
    print_string("\n");

    print_string("help : print this help menu\n");
    for (int i = 0; i < num_commands; i++) {
        print_string(commands[i].name);
        print_string(" : ");
        print_string(commands[i].description);
        print_string("\n");
    }
}

void _clear_command() { print_string("\033[2J\033[1;1H"); }
