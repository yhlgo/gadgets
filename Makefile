# Compiler
CC = gcc

# Compiler flags
CFLAGS = -fPIC -Wall -Wextra -O2 -g

# Linker flags
LDFLAGS = -shared

# Directory for build artifacts
BUILDDIR = build

# Library name
LIBNAME = $(BUILDDIR)/libyuyu.so

# Source files
SRCS = $(wildcard *.c)
SRCS += $(wildcard */*.c) # Add this line to include .c files in subdirectories

# Object files
OBJS = $(patsubst %.c,$(BUILDDIR)/%.o,$(SRCS))

all: $(LIBNAME)

$(LIBNAME): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

$(BUILDDIR)/%.o: %.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILDDIR)

.PHONY: all clean