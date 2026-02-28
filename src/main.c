#include <stdio.h>
#include <string.h>

#define INPUT_SIZE 256

static int dispatch_command(const char *input)
{
    if (strcmp(input, "exit") == 0) {
        return 0;
    }

    if (input[0] != '\0') {
        printf("Unknown command: %s\n", input);
    }

    return 1;
}

int main(void)
{
    char input[INPUT_SIZE];

    while (1) {
        printf("TitanX> ");
        fflush(stdout);

        if (fgets(input, sizeof(input), stdin) == NULL) {
            putchar('\n');
            break;
        }

        input[strcspn(input, "\n")] = '\0';

        if (!dispatch_command(input)) {
            break;
        }
    }

    return 0;
}
