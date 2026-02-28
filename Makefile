CC ?= gcc
CFLAGS ?= -O2 -pipe -Wall -Wextra -std=c11
LDFLAGS ?=
TARGET := titanx
SRC := src/main.c

.PHONY: all run clean

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET) $(LDFLAGS)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)
