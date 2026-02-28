#ifndef COMMANDS_H
#define COMMANDS_H

#include <stdio.h>

struct shell_context {
    int should_exit;
    FILE *out;
    FILE *err;
};

typedef int (*command_handler_t)(struct shell_context *ctx, int argc, char **argv);

struct command_entry {
    const char *name;
    const char *description;
    command_handler_t handler;
};

void command_registry_init(void);
void command_registry_destroy(void);
int command_registry_register(const char *name, const char *description, command_handler_t handler);
const struct command_entry *command_registry_find(const char *name);
size_t command_registry_count(void);
const struct command_entry *command_registry_entries(void);

int command_registry_register_defaults(void);
int command_execute_line(struct shell_context *ctx, const char *line);

#endif
