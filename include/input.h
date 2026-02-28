#ifndef INPUT_H
#define INPUT_H

#include <stddef.h>

char *input_read_line(const char *prompt);
void input_history_add(const char *line);
size_t input_history_count(void);
const char *input_history_get(size_t index);
void input_history_clear(void);

#endif
