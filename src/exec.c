#define _POSIX_C_SOURCE 200809L
#include "exec.h"

#include <errno.h>
#include <limits.h>
#ifndef PATH_MAX
#define PATH_MAX 4096
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#ifdef TITANX_DEBUG
#define DEBUG_LOG(...) fprintf(stderr, __VA_ARGS__)
#else
#define DEBUG_LOG(...) ((void)0)
#endif

typedef int (*builtin_handler_t)(char **argv, size_t argc);

typedef struct path_cache_entry {
    char *cmd;
    char *path;
    struct path_cache_entry *next;
} path_cache_entry_t;

static path_cache_entry_t *g_path_cache = NULL;
static char *g_cached_path_env = NULL;

static int builtin_cd(char **argv, size_t argc) {
    const char *target = NULL;

    if (argc > 1) {
        target = argv[1];
    } else {
        target = getenv("HOME");
    }

    if (target == NULL) {
        fprintf(stderr, "cd: missing target directory\n");
        return 1;
    }

    if (chdir(target) != 0) {
        perror("cd");
        return 1;
    }

    return 0;
}

static int builtin_pwd(char **argv, size_t argc) {
    (void)argv;
    (void)argc;

    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        perror("pwd");
        return 1;
    }

    printf("%s\n", cwd);
    return 0;
}

static int builtin_exit(char **argv, size_t argc) {
    int code = 0;

    if (argc > 1) {
        code = atoi(argv[1]);
    }

    exit(code);
}

typedef struct builtin_command {
    const char *name;
    builtin_handler_t handler;
} builtin_command_t;

static builtin_command_t g_builtins[] = {
    {"cd", builtin_cd},
    {"pwd", builtin_pwd},
    {"exit", builtin_exit},
};

static size_t builtin_count(void) {
    return sizeof(g_builtins) / sizeof(g_builtins[0]);
}

static builtin_handler_t find_builtin(const char *cmd) {
    for (size_t i = 0; i < builtin_count(); i++) {
        if (strcmp(cmd, g_builtins[i].name) == 0) {
            return g_builtins[i].handler;
        }
    }

    return NULL;
}

static void clear_path_cache(void) {
    path_cache_entry_t *cur = g_path_cache;
    while (cur != NULL) {
        path_cache_entry_t *next = cur->next;
        free(cur->cmd);
        free(cur->path);
        free(cur);
        cur = next;
    }

    g_path_cache = NULL;

    free(g_cached_path_env);
    g_cached_path_env = NULL;
}

static void refresh_path_cache_if_needed(void) {
    const char *path = getenv("PATH");
    if (path == NULL) {
        path = "";
    }

    if (g_cached_path_env != NULL && strcmp(g_cached_path_env, path) == 0) {
        return;
    }

    DEBUG_LOG("[debug] PATH changed; invalidating cache\n");
    clear_path_cache();
    g_cached_path_env = strdup(path);
}

static const char *find_cached_path(const char *cmd) {
    for (path_cache_entry_t *cur = g_path_cache; cur != NULL; cur = cur->next) {
        if (strcmp(cur->cmd, cmd) == 0) {
            return cur->path;
        }
    }

    return NULL;
}

static void remove_cached_path(const char *cmd) {
    path_cache_entry_t *prev = NULL;
    path_cache_entry_t *cur = g_path_cache;

    while (cur != NULL) {
        if (strcmp(cur->cmd, cmd) == 0) {
            if (prev == NULL) {
                g_path_cache = cur->next;
            } else {
                prev->next = cur->next;
            }
            free(cur->cmd);
            free(cur->path);
            free(cur);
            return;
        }

        prev = cur;
        cur = cur->next;
    }
}

static void cache_path(const char *cmd, const char *resolved_path) {
    remove_cached_path(cmd);

    path_cache_entry_t *entry = calloc(1, sizeof(*entry));
    if (entry == NULL) {
        return;
    }

    entry->cmd = strdup(cmd);
    entry->path = strdup(resolved_path);

    if (entry->cmd == NULL || entry->path == NULL) {
        free(entry->cmd);
        free(entry->path);
        free(entry);
        return;
    }

    entry->next = g_path_cache;
    g_path_cache = entry;
}

static const char *resolve_command_path(const char *cmd) {
    if (strchr(cmd, '/') != NULL) {
        return cmd;
    }

    refresh_path_cache_if_needed();

    const char *cached = find_cached_path(cmd);
    if (cached != NULL && access(cached, X_OK) == 0) {
        return cached;
    }

    if (cached != NULL) {
        remove_cached_path(cmd);
    }

    const char *path_env = getenv("PATH");
    if (path_env == NULL || *path_env == '\0') {
        return NULL;
    }

    char *path_copy = strdup(path_env);
    if (path_copy == NULL) {
        return NULL;
    }

    char *saveptr = NULL;
    for (char *token = strtok_r(path_copy, ":", &saveptr);
         token != NULL;
         token = strtok_r(NULL, ":", &saveptr)) {
        char full_path[PATH_MAX];
        if (snprintf(full_path, sizeof(full_path), "%s/%s", token, cmd) >= (int)sizeof(full_path)) {
            continue;
        }

        if (access(full_path, X_OK) == 0) {
            cache_path(cmd, full_path);
            free(path_copy);
            return find_cached_path(cmd);
        }
    }

    free(path_copy);
    return NULL;
}

static int execute_external(char **argv) {
    const char *cmd_path = resolve_command_path(argv[0]);

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return 1;
    }

    if (pid == 0) {
        if (cmd_path != NULL) {
            execv(cmd_path, argv);
        }

        execvp(argv[0], argv);
        perror("execvp");
        _exit(127);
    }

    int status = 0;
    while (waitpid(pid, &status, 0) < 0) {
        if (errno == EINTR) {
            continue;
        }

        perror("waitpid");
        return 1;
    }

    if (WIFEXITED(status)) {
        int code = WEXITSTATUS(status);
        if (code == 127 && strchr(argv[0], '/') == NULL) {
            DEBUG_LOG("[debug] command failed, invalidating cache for '%s'\n", argv[0]);
            remove_cached_path(argv[0]);
        }
        return code;
    }

    if (WIFSIGNALED(status)) {
        return 128 + WTERMSIG(status);
    }

    return 1;
}

int titanx_execute(char **argv, size_t argc) {
    if (argv == NULL || argc == 0 || argv[0] == NULL) {
        return 0;
    }

    builtin_handler_t builtin = find_builtin(argv[0]);
    if (builtin != NULL) {
        DEBUG_LOG("[debug] running builtin '%s'\n", argv[0]);
        return builtin(argv, argc);
    }

    return execute_external(argv);
}
