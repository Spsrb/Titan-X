CC ?= cc
CFLAGS ?= -std=c11 -O2 -Wall -Wextra -Werror
LDFLAGS ?=

SRC := src/main.c src/exec.c
OBJ := $(SRC:.c=.o)

all: titanx

titanx: $(OBJ)
	$(CC) $(OBJ) -o $@ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) titanx

.PHONY: all clean package


package:
	./scripts/package.sh 1.0
