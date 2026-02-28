#include "input.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

static char **g_history = NULL;
static size_t g_history_count = 0;
static size_t g_history_cap = 0;

static int history_grow(void) {
    size_t next_cap = g_history_cap == 0 ? 32 : g_history_cap * 2;
    char **next = realloc(g_history, next_cap * sizeof(*next));
    if (!next) {
        return -1;
    }
    g_history = next;
    g_history_cap = next_cap;
    return 0;
}

void input_history_add(const char *line) {
    if (!line || line[0] == '\0') {
        return;
    }

    if (g_history_count > 0 && strcmp(g_history[g_history_count - 1], line) == 0) {
        return;
    }

    if (g_history_count == g_history_cap && history_grow() != 0) {
        return;
    }

    g_history[g_history_count] = strdup(line);
    if (!g_history[g_history_count]) {
        return;
    }

    g_history_count++;
}

size_t input_history_count(void) {
    return g_history_count;
}

const char *input_history_get(size_t index) {
    if (index >= g_history_count) {
        return NULL;
    }

    return g_history[index];
}

void input_history_clear(void) {
    for (size_t i = 0; i < g_history_count; ++i) {
        free(g_history[i]);
    }
    free(g_history);
    g_history = NULL;
    g_history_count = 0;
    g_history_cap = 0;
}

static int line_buffer_reserve(char **buffer, size_t *capacity, size_t needed) {
    if (needed <= *capacity) {
        return 0;
    }

    size_t next_cap = *capacity == 0 ? 128 : *capacity * 2;
    while (next_cap < needed) {
        next_cap *= 2;
    }

    char *next = realloc(*buffer, next_cap);
    if (!next) {
        return -1;
    }

    *buffer = next;
    *capacity = next_cap;
    return 0;
}

static void redraw_line(const char *prompt, const char *buffer) {
    printf("\r\x1b[2K%s%s", prompt, buffer);
    fflush(stdout);
}

char *input_read_line(const char *prompt) {
    struct termios original;
    if (tcgetattr(STDIN_FILENO, &original) != 0) {
        return NULL;
    }

    struct termios raw = original;
    raw.c_lflag &= ~(ICANON | ECHO);
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;

    if (tcsetattr(STDIN_FILENO, TCSANOW, &raw) != 0) {
        return NULL;
    }

    size_t cap = 0;
    size_t len = 0;
    char *buffer = NULL;
    char *draft = NULL;
    ssize_t history_index = -1;

    if (line_buffer_reserve(&buffer, &cap, 1) != 0) {
        tcsetattr(STDIN_FILENO, TCSANOW, &original);
        return NULL;
    }
    buffer[0] = '\0';

    printf("%s", prompt);
    fflush(stdout);

    for (;;) {
        char ch;
        ssize_t read_size = read(STDIN_FILENO, &ch, 1);
        if (read_size <= 0) {
            break;
        }

        if (ch == '\n' || ch == '\r') {
            printf("\n");
            break;
        }

        if (ch == 127 || ch == 8) {
            if (len > 0) {
                len--;
                buffer[len] = '\0';
                redraw_line(prompt, buffer);
            }
            continue;
        }

        if (ch == '\x1b') {
            char seq[2];
            if (read(STDIN_FILENO, &seq[0], 1) <= 0 || read(STDIN_FILENO, &seq[1], 1) <= 0) {
                continue;
            }

            if (seq[0] == '[' && (seq[1] == 'A' || seq[1] == 'B')) {
                if (g_history_count == 0) {
                    continue;
                }

                if (!draft) {
                    draft = strdup(buffer);
                    if (!draft) {
                        continue;
                    }
                }

                if (seq[1] == 'A') {
                    if (history_index + 1 < (ssize_t)g_history_count) {
                        history_index++;
                    }
                } else if (history_index >= 0) {
                    history_index--;
                }

                const char *entry = NULL;
                if (history_index >= 0) {
                    size_t idx = g_history_count - 1 - (size_t)history_index;
                    entry = g_history[idx];
                } else {
                    entry = draft;
                }

                size_t entry_len = strlen(entry);
                if (line_buffer_reserve(&buffer, &cap, entry_len + 1) != 0) {
                    continue;
                }

                memcpy(buffer, entry, entry_len + 1);
                len = entry_len;
                redraw_line(prompt, buffer);
            }
            continue;
        }

        if (ch < 32) {
            continue;
        }

        if (line_buffer_reserve(&buffer, &cap, len + 2) != 0) {
            break;
        }

        buffer[len++] = ch;
        buffer[len] = '\0';
        redraw_line(prompt, buffer);
    }

    tcsetattr(STDIN_FILENO, TCSANOW, &original);
    free(draft);

    return buffer;
}
