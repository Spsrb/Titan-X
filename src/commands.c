#include "commands.h"

#include "input.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static struct command_entry *g_commands = NULL;
static size_t g_command_count = 0;
static size_t g_command_cap = 0;

static int builtin_help(struct shell_context *ctx, int argc, char **argv);
static int builtin_clear(struct shell_context *ctx, int argc, char **argv);
static int builtin_cd(struct shell_context *ctx, int argc, char **argv);
static int builtin_pwd(struct shell_context *ctx, int argc, char **argv);
static int builtin_history(struct shell_context *ctx, int argc, char **argv);
static int builtin_exit(struct shell_context *ctx, int argc, char **argv);

static int command_grow(void) {
    size_t next_cap = g_command_cap == 0 ? 16 : g_command_cap * 2;
    struct command_entry *next = realloc(g_commands, next_cap * sizeof(*next));
    if (!next) {
        return -1;
    }
    g_commands = next;
    g_command_cap = next_cap;
    return 0;
}

void command_registry_init(void) {
    g_commands = NULL;
    g_command_count = 0;
    g_command_cap = 0;
}

void command_registry_destroy(void) {
    free(g_commands);
    g_commands = NULL;
    g_command_count = 0;
    g_command_cap = 0;
}

int command_registry_register(const char *name, const char *description, command_handler_t handler) {
    if (!name || !description || !handler) {
        return -1;
    }

    if (command_registry_find(name)) {
        return -1;
    }

    if (g_command_count == g_command_cap && command_grow() != 0) {
        return -1;
    }

    g_commands[g_command_count].name = name;
    g_commands[g_command_count].description = description;
    g_commands[g_command_count].handler = handler;
    g_command_count++;

    return 0;
}

const struct command_entry *command_registry_find(const char *name) {
    if (!name) {
        return NULL;
    }

    for (size_t i = 0; i < g_command_count; ++i) {
        if (strcmp(g_commands[i].name, name) == 0) {
            return &g_commands[i];
        }
    }

    return NULL;
}

size_t command_registry_count(void) {
    return g_command_count;
}

const struct command_entry *command_registry_entries(void) {
    return g_commands;
}

static int parse_line(const char *line, int *argc_out, char ***argv_out) {
    size_t argv_cap = 8;
    int argc = 0;
    char **argv = malloc(argv_cap * sizeof(*argv));
    if (!argv) {
        return -1;
    }

    size_t i = 0;
    while (line[i] != '\0') {
        while (line[i] == ' ' || line[i] == '\t') {
            i++;
        }
        if (line[i] == '\0') {
            break;
        }

        char quote = 0;
        size_t token_cap = 32;
        size_t token_len = 0;
        char *token = malloc(token_cap);
        if (!token) {
            goto fail;
        }

        while (line[i] != '\0') {
            char c = line[i];
            if (!quote && (c == ' ' || c == '\t')) {
                break;
            }
            if (c == '\'' || c == '"') {
                if (quote == 0) {
                    quote = c;
                    i++;
                    continue;
                }
                if (quote == c) {
                    quote = 0;
                    i++;
                    continue;
                }
            }

            if (token_len + 1 >= token_cap) {
                token_cap *= 2;
                char *next = realloc(token, token_cap);
                if (!next) {
                    free(token);
                    goto fail;
                }
                token = next;
            }

            token[token_len++] = c;
            i++;
        }

        token[token_len] = '\0';

        if ((size_t)argc == argv_cap) {
            argv_cap *= 2;
            char **next_argv = realloc(argv, argv_cap * sizeof(*argv));
            if (!next_argv) {
                free(token);
                goto fail;
            }
            argv = next_argv;
        }

        argv[argc++] = token;
    }

    *argc_out = argc;
    *argv_out = argv;
    return 0;

fail:
    for (int j = 0; j < argc; ++j) {
        free(argv[j]);
    }
    free(argv);
    return -1;
}

static void free_argv(int argc, char **argv) {
    for (int i = 0; i < argc; ++i) {
        free(argv[i]);
    }
    free(argv);
}

int command_registry_register_defaults(void) {
    if (command_registry_register("help", "Show built-in commands.", builtin_help) != 0) return -1;
    if (command_registry_register("clear", "Clear the terminal screen.", builtin_clear) != 0) return -1;
    if (command_registry_register("cd", "Change current working directory.", builtin_cd) != 0) return -1;
    if (command_registry_register("pwd", "Print current working directory.", builtin_pwd) != 0) return -1;
    if (command_registry_register("history", "Show command history.", builtin_history) != 0) return -1;
    if (command_registry_register("exit", "Exit the shell.", builtin_exit) != 0) return -1;
    return 0;
}

int command_execute_line(struct shell_context *ctx, const char *line) {
    int argc = 0;
    char **argv = NULL;

    if (!ctx || !line) {
        return -1;
    }

    if (!ctx->out) {
        ctx->out = stdout;
    }
    if (!ctx->err) {
        ctx->err = stderr;
    }

    if (parse_line(line, &argc, &argv) != 0) {
        fprintf(ctx->err, "parse error\n");
        return -1;
    }

    if (argc == 0) {
        free(argv);
        return 0;
    }

    const struct command_entry *entry = command_registry_find(argv[0]);
    if (!entry) {
        fprintf(ctx->err, "unknown command: %s\n", argv[0]);
        free_argv(argc, argv);
        return -1;
    }

    int rc = entry->handler(ctx, argc, argv);
    free_argv(argc, argv);
    return rc;
}

static int builtin_help(struct shell_context *ctx, int argc, char **argv) {
    (void)argc;
    (void)argv;

    for (size_t i = 0; i < g_command_count; ++i) {
        fprintf(ctx->out, "%-10s %s\n", g_commands[i].name, g_commands[i].description);
    }

    return 0;
}

static int builtin_clear(struct shell_context *ctx, int argc, char **argv) {
    (void)argc;
    (void)argv;
    fprintf(ctx->out, "\x1b[2J\x1b[H");
    return 0;
}

static int builtin_cd(struct shell_context *ctx, int argc, char **argv) {
    (void)ctx;
    const char *target = NULL;

    if (argc < 2) {
        target = getenv("HOME");
        if (!target) {
            target = "/";
        }
    } else {
        target = argv[1];
    }

    if (chdir(target) != 0) {
        perror("cd");
        return -1;
    }

    return 0;
}

static int builtin_pwd(struct shell_context *ctx, int argc, char **argv) {
    (void)argc;
    (void)argv;

    char path[PATH_MAX];
    if (!getcwd(path, sizeof(path))) {
        perror("pwd");
        return -1;
    }

    fprintf(ctx->out, "%s\n", path);
    return 0;
}

static int builtin_history(struct shell_context *ctx, int argc, char **argv) {
    (void)argc;
    (void)argv;

    size_t count = input_history_count();
    for (size_t i = 0; i < count; ++i) {
        const char *entry = input_history_get(i);
        fprintf(ctx->out, "%4zu  %s\n", i + 1, entry ? entry : "");
    }

    return 0;
}

static int builtin_exit(struct shell_context *ctx, int argc, char **argv) {
    (void)argc;
    (void)argv;
    ctx->should_exit = 1;
    return 0;
}
