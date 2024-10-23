PATH_WORK = $(CURDIR)/../..
BUILDDIR = $(PATH_WORK)/build/test/util

# Compiler to use
CC = gcc

# Compiler flags
CFLAGS = -Wall -Wextra -Werror -g -I$(PATH_WORK)/src

# Linker flags
LDFLAGS = -L$(PATH_WORK)/build -lyuyu -Wl,-rpath,$(PATH_WORK)/src

# Source files
SRCS = $(wildcard *.c)

# Executables
PROGS = $(patsubst %.c,$(BUILDDIR)/%,$(SRCS))

all: $(PROGS)

# Pattern rule for executables
$(BUILDDIR)/%: %.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -rf $(BUILDDIR)

.PHONY: all clean